#include <asm/unistd.h>
#include <linux/delay.h>
#include <linux/file.h>

#include "syscall.h"
#include "util.h"
#include "dbg.h"
#include "btrfs.h"

//haowu: a clock value starting from 0
unsigned current_clock = 0;

static struct syscall syscalls[] = {
	#include "trace_syscalls.inc"
};

struct syscall * syscall_get(int nr)
{
	struct syscall *sc;
	if (nr < 0 || nr >= sizeof(syscalls)/sizeof(syscalls[0]))
		return NULL;
	sc = &syscalls[nr];
	if (!sc->name) /* not intercepted */
		return NULL;
	return sc;
}

static int handle_inode_drop(int nr, long args[6])
{
	int path;
	int (*hdlr)(int dfd, const char __user *pathname) = NULL;
	int dfd = AT_FDCWD;

	switch (nr) {
	case __NR_unlink:
		dfd = AT_FDCWD;
		hdlr = handle_unlinkat;
		path = 0;
		break;
	case __NR_unlinkat:
		if (args[2] & ~AT_REMOVEDIR)
			return 0;

		if (args[2] & AT_REMOVEDIR) {
			dfd = args[0];
			hdlr = handle_rmdirat;
			path = 1;
		} else {
			dfd = args[0];
			hdlr = handle_unlinkat;
			path = 1;
		}
		break;
	case __NR_rmdir:
		hdlr = handle_rmdirat;
		path = 0;
		break;
	}

	if ( hdlr ) {
		return hdlr(dfd, (char*) args[path]);
	}

	return -1;
}

enum { reserve_bytes = 128*1024 };

extern struct file *fget_light(unsigned int fd, int *fput_needed);

static long _record(int usage, pid_t pid, int nr, long args[6],
		    long ret, long *retp, uint64_t *sidp)
{
	struct syscall *sc;
	const struct sysarg *arg;

	int i, used[7] = {0};
	char buf[128];
	size_t len;
	unsigned cpu;

	struct rchan *chans[] = {
	    kprobes_chan, syscall_chan, inode_chan, pid_chan,
	    pathid_chan, pathidtable_chan
	};

 retry:
	preempt_disable();
	cpu = smp_processor_id();

	for (i = 0; i < sizeof(chans) / sizeof(chans[0]); i++) {
	    /* copied from relay_reserve in include/linux/relay.h */
	    struct rchan_buf *buf = chans[i]->buf[cpu];
	    if (unlikely(buf->offset + reserve_bytes >
			 buf->chan->subbuf_size)) {
		size_t length = relay_switch_subbuf(buf, reserve_bytes);
		if (!length) {
		    preempt_enable();
		    dbg(info, "retro buffers full, sleeping..");
		    msleep(100);
		    goto retry;
		}
	    }
	}

	/* if tracing syscall */
	sc = syscall_get(nr);
	if (unlikely(!sc)) {
		preempt_enable();
		return HOOK_CONT;
	}

	/* record the index of the open inode by open() syscall */
	if (unlikely(nr == __NR_open && usage == 1 && ret >= 0)) {
		struct file *file;
		int fput_needed;
		struct inode *inode;

		file = fget_light(ret, &fput_needed);
		if (file) {
			inode = file->f_path.dentry->d_inode;
			index_inode(inode);
			fput_light(file, fput_needed);
		}
	}

	/* update offsets for indexing */
	{
		struct rchan_buf *buf = syscall_chan->buf[cpu];
		syscall_chan_info[cpu].offset = syscall_chan_info[cpu].base + buf->offset;
	}

	/* pick a unique syscall ID on entry */
  /*
   * TODO XXX
   * There's a pretty subtle bug lurking here. Namely, in the cases in which we
   * are tracing only sysexits, this function is only called with usage=0, so
   * *sidp will contain garbage!
   */
	if (usage == 0) {
		static DEFINE_PER_CPU_SHARED_ALIGNED(uint64_t, sid_ctr);
		uint64_t *ctrp = &__get_cpu_var(sid_ctr);
		*sidp = (((uint64_t) cpu) << 48) | ((*ctrp)++);
	}

	/* record mmap info to skip loading */
	syscall_chan_info[cpu].ro = 0;
	if (unlikely(nr == __NR_mmap)) {
		if (!(args[2] & 2)) {		/* No PROT_WRITE */
			syscall_chan_info[cpu].ro = 1;
		} else if (args[3] & 2) {	/* PROT_WRITE but MAP_PRIVATE */
			syscall_chan_info[cpu].ro = 1;
		}
	}

	/* start real recording */
	len = vartime(buf);
	len += uvarint(pid, buf + len);
	len += uvarint(usage, buf + len);
	len += uvarint(nr, buf + len);

	memcpy(buf + len, sidp, sizeof(uint64_t));
	len += sizeof(uint64_t);

	if (usage == 1 && sc->nr == __NR_execve) {
		len += uvarint(current->cred->uid, buf + len);
		len += uvarint(current->cred->gid, buf + len);
		len += uvarint(current->cred->euid, buf + len);
		len += uvarint(current->cred->egid, buf + len);
	}

	syscall_log(buf, len);

	syscall_chan_info[cpu].nr = nr;
	syscall_chan_info[cpu].sid = *sidp;

	/* index pid for the record */
	index_pid(pid);

  /* Write arguments */
	arg = sc->args;
	for (i = 0; i < sc->nargs; ++i, ++arg) {
		struct sysarg a = *arg;
		if (a.usage != usage || used[i])
			continue;
		if (a.ty == sysarg_iovec) {
			if (a.usage) {
				if (ret > 0) {
					a.aux = args[i+1];
					a.ret = ret;
				}
			} else {
				/* very vulnerable from user inputs */
				used[i+1] = 1;
				a.aux = args[i+1];
				a.ret = UIO_MAXIOV*PAGE_SIZE;
			}
		}
		if (a.ty == sysarg_buf || a.ty == sysarg_buf_det) {
			if (a.usage) {
        // Length of the buffer depends on the kernel.
				if (ret > 0) {
					used[6] = 1;
					a.aux = ret;
				} else
					a.aux = 0;
			} else {
        // Length of the buffer is passed in by the user.
				used[i+1] = 1;
				a.aux = args[i+1];
			}
		} else if (a.ty == sysarg_struct) {
			if (a.usage) {
				a.aux = 0;
				if (ret == 0) {
					if (sc->args[i+1].ty == sysarg_psize_t) {
						int size = 0;
						get_user(size, (int *)args[i+1]);
						a.aux = size;
					}
				}
			} else {
				if (sc->args[i+1].ty == sysarg_uint) {
					used[i+1] = 1;
					a.aux = args[i+1];
				}
			}
		} else if (a.ty == sysarg_sha1) {
      // Akin to the case of sysarg_buf.
			if (a.usage) {
				a.aux = ret;
			} else {
				a.aux = args[i+1];
			}
		} else if (a.ty == sysarg_path_at || a.ty == sysarg_rpath_at) {
			/* test AT_SYMLINK_(NO)FOLLOW, always the last argument */
			if (a.aux && (a.aux & args[sc->nargs - 1])) {
				/* toggle */
				if (a.ty == sysarg_path_at)
					a.ty = sysarg_rpath_at;
				else
					a.ty = sysarg_path_at;
			}
			a.aux = args[i-1];
		}
		a.ty(args[i], &a);
	}

    // haowu: Assign the clock value to the syscall and increase the value
    sc->clock_number = current_clock++;
	if (usage && !used[6])
		sc->ret(ret, NULL);

  /* ipopov:
   * At this point, hopefully we're done.
   */

	preempt_enable();

	if (usage == 0 && retp) {
		if (handle_inode_drop(sc->nr, args) >= 0) {
			*retp = 0;
			return HOOK_SKIP;
		}
	}

	if (sc->nr == NR_snapshot && retp) {
		*retp = take_snapshot((const char *) args[0],
				      (const char *) args[1],
				      (const char *) args[2]);
		return HOOK_SKIP;
	}

	return HOOK_CONT;
}

long syscall_enter(pid_t pid, int nr, long args[6], long *retp, uint64_t *sidp)
{
	return _record(0, pid, nr, args, 0, retp, sidp);
}

void syscall_exit(pid_t pid, int nr, long args[6], long ret, uint64_t sid)
{
	_record(1, pid, nr, args, ret, 0, &sid);
}
