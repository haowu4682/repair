/*
 * Filesystem interface to user space.
 *
 * relayfs-based data channels and debugfs controls.
 */

#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/relay.h>
#include <linux/file.h>
#include <linux/magic.h>
#include <linux/kallsyms.h>
#include <linux/namei.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/stop_machine.h>
#include "fs.h"
#include "nm.h"
#include "util.h"
#include "dbg.h"

struct rchan *syscall_chan;
struct rchan *kprobes_chan;
struct rchan *pid_chan;
struct rchan *inode_chan;
struct rchan *pathid_chan;
struct rchan *pathidtable_chan;
struct rchan *sock_chan;

struct syscall_chan_info syscall_chan_info[NR_CPUS];

static size_t subbuf_size = 512 * 1024;
static size_t n_subbufs = 4;

static struct dentry *dir;
static struct dentry *d_reset;
static struct dentry *d_flush;
static struct dentry *d_trace;

int retro_trace_flag = 0;

/* trace file  */
static struct dentry *create_buf_file_handler(const char *filename,
                                              struct dentry *parent,
                                              int mode,
                                              struct rchan_buf *buf,
                                              int *is_global)
{
	return debugfs_create_file(filename, mode, parent, buf, &relay_file_operations);
}

static int remove_buf_file_handler(struct dentry *dentry)
{
	debugfs_remove(dentry);
	return 0;
}

static int subbuf_start_handler_other(struct rchan_buf *buf, void *subbuf,
				      void *prev_subbuf, size_t prev_padding)
{
	if (relay_buf_full(buf)) {
		printk(KERN_ERR "other buffer full!\n");
		return 0;
	}
	return 1; /* continue logging */
}

static int subbuf_start_handler_syscall(struct rchan_buf *buf, void *subbuf,
					void *prev_subbuf, size_t prev_padding)
{
	if (relay_buf_full(buf)) {
		printk(KERN_ERR "syscall buffer full!\n");
		return 0;
	}
	if (likely(prev_subbuf))
		syscall_chan_info[buf->cpu].base += (subbuf_size - prev_padding);
	return 1; /* continue logging */
}

static struct rchan_callbacks relay_callbacks = {
	.subbuf_start    = subbuf_start_handler_other,
	.create_buf_file = create_buf_file_handler,
	.remove_buf_file = remove_buf_file_handler,
};

static struct rchan_callbacks syscall_callbacks = {
	.subbuf_start    = subbuf_start_handler_syscall,
	.create_buf_file = create_buf_file_handler,
	.remove_buf_file = remove_buf_file_handler,
};

static ssize_t retro_reset_chan(struct file *file, const char __user *ubuf,
				size_t count, loff_t *off)
{
	int i;
	struct rchan *chans[] = { syscall_chan, inode_chan, pid_chan, pathid_chan,
				  pathidtable_chan, sock_chan};

	/* init relay buffer */
	for (i = 0; i < sizeof(chans) / sizeof(chans[0]); i++) {
		if (!chans[i])
			continue;
		relay_flush(chans[i]);
		relay_reset(chans[i]);
	}

	return 0;
}

/* functions */
static struct file_operations retro_reset_fps = {
	.open  = nonseekable_open,
	.write = retro_reset_chan,
};

static ssize_t retro_flush_chan(struct file *file, const char __user *ubuf,
				size_t count, loff_t *off)
{
	int i;
	struct rchan *chans[] = { syscall_chan, inode_chan, pid_chan, pathid_chan,
				  pathidtable_chan, sock_chan};

	/* init relay buffer */
	for (i = 0; i < sizeof(chans) / sizeof(chans[0]); i++) {
		if (!chans[i])
			continue;
		relay_flush(chans[i]);
	}

	return 0;
}

/* functions */
static struct file_operations retro_flush_fps = {
	.open  = nonseekable_open,
	.write = retro_flush_chan,
};

int fs_init(void)
{
	dir = debugfs_create_dir("retro", NULL);
	if (!dir) {
		printk(KERN_ERR "'retro' directory creation failed\n");
		goto fail;
	}

	d_reset = debugfs_create_file("reset", 0600, dir, 0, &retro_reset_fps);
	if (!d_reset) {
		printk(KERN_ERR "'reset' file creation failed\n");
		goto fail;
	}

	d_flush = debugfs_create_file("flush", 0600, dir, 0, &retro_flush_fps);
	if (!d_flush) {
		printk(KERN_ERR "'flush' file creation failed\n");
		goto fail;
	}

	d_trace = debugfs_create_u32("trace", 0600, dir, &retro_trace_flag);
	if (!d_trace) {
		printk(KERN_ERR "'trace' file creation failed\n");
		goto fail;
	}

#define OPEN_CHAN(name, fn)						\
	({								\
		name##_chan =						\
			relay_open(#name, dir, subbuf_size,		\
				   n_subbufs, &fn##_callbacks, NULL);	\
		if (!name##_chan) {					\
			err("'%s' channel creation failed", #name);	\
			goto fail;					\
		}							\
	})								\

	OPEN_CHAN(syscall    , syscall);
	OPEN_CHAN(inode      , relay  );
	OPEN_CHAN(pid        , relay  );
	OPEN_CHAN(kprobes    , relay  );
	OPEN_CHAN(pathid     , relay  );
	OPEN_CHAN(pathidtable, relay  );
	OPEN_CHAN(sock	     , relay  );
	
#undef OPEN_CHAN
	
	return 0;

fail:
	fs_exit();
	return -1;
}

void fs_exit(void)
{

#define CLOSE_CHAN(name)				\
	({						\
		if (name##_chan) {			\
			relay_flush(name##_chan);	\
			relay_close(name##_chan);	\
			name##_chan = NULL;		\
		}					\
	})						\

	CLOSE_CHAN(syscall);
	CLOSE_CHAN(inode);
	CLOSE_CHAN(pid);
	CLOSE_CHAN(kprobes);
	CLOSE_CHAN(pathid);
	CLOSE_CHAN(pathidtable);
	CLOSE_CHAN(sock);

#undef CLOSE_CHAN

	if (d_trace)
		debugfs_remove(d_trace);
	if (d_reset)
		debugfs_remove(d_reset);
	if (d_flush)
		debugfs_remove(d_flush);
	if (dir) {
		debugfs_remove(dir);
		dir = NULL;
	}
}
