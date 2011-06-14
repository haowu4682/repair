#pragma once

#include <linux/ktime.h>
#include <linux/magic.h>
#include <linux/relay.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/unistd.h>

#include "config.h"

#include "nm.h"
#include "dbg.h"

#include "fs.h"
#include "pathid.h"
#include "sysarg.h"

struct syscall {
	int nr;
	const char *name;
	argtype_t ret;
	size_t nargs;
	struct sysarg args[6];
};

struct syscall_index {
  size_t offset; // Offset into syscalls file where the record indexed
                 // by this begins.
  uint16_t nr;   // Syscall number (e.g. NR_open).
  uint32_t sid;  // A unique id corresponding to this syscall
                 // invocation.
} __attribute__((__packed__));

/* The following _record structs will be made into Berkeley DB records by the
 * userspace retroctl daemon. The database structure is key -> value, where the
 * value is the 'struct syscall_index' entry, and the key is the other data in
 * the _record struct. For example,
 *  dev_xor_ino -> syscall_index
 *  pid -> syscall_index
 *  etc...
 */

struct inode_index_record {
  struct syscall_index s;
  uint32_t dev_xor_ino;
} __attribute__((__packed__));

struct pid_index_record {
  struct syscall_index s;
  uint32_t pid;
} __attribute__((__packed__));

struct sock_index_record {
  struct syscall_index s;
  uint32_t ip;
  uint32_t port;
} __attribute__((__packed__));

struct pathid_index_record {
  struct syscall_index s;
  char pathid[PATHID_LEN];
} __attribute__((__packed__));


long syscall_enter(pid_t pid, int nr, long args[6], long *retp, uint64_t *idp);
void syscall_exit (pid_t pid, int nr, long args[6], long  ret , uint64_t  id );

int handle_unlinkat(int dfd, const char __user *pathname);
int handle_rmdirat (int dfd, const char __user *pathname);

typedef void (*__ptrace_notify)(int exit_code);

#define RETRO_PID_MARK (0x80)
#define is_deterministic(task) ((task)->flags & RETRO_PID_MARK)
#define set_deterministic(task) ((task)->flags |= RETRO_PID_MARK)

const struct syscall *syscall_get(int nr);

static inline void syscall_log(const void *buf, size_t len)
{
	__relay_write(syscall_chan, buf, len);
}

static inline void populate_syscall_index(int cpu, struct syscall_index *s) {
	s->offset = syscall_chan_info[cpu].offset;
	s->nr     = syscall_chan_info[cpu].nr;
	s->sid    = syscall_chan_info[cpu].sid;
}

static inline void index_inode(struct inode *inode)
{
	dev_t dev;
	struct inode_index_record rec;
	int cpu = smp_processor_id();

	if (inode->i_sb->s_magic == BTRFS_SUPER_MAGIC &&
		inode->i_op && inode->i_op->getattr) {
		struct dentry dfake;
		struct kstat stat;

		dfake.d_inode = inode;
		inode->i_op->getattr(0, &dfake, &stat);
		dev = stat.dev;
	} else {
		dev = inode->i_sb->s_dev;
	}

	populate_syscall_index(cpu, &rec.s);
	rec.dev_xor_ino = new_encode_dev(dev) ^ inode->i_ino;

	if (syscall_chan_info[cpu].ro)
		rec.dev_xor_ino ^= 0x80000000UL;

	__relay_write(inode_chan, &rec, sizeof(rec));
}

static inline void index_pid(pid_t pid)
{
	struct pid_index_record rec;
	int cpu = smp_processor_id();

	if (pid < 0)
		return;

	populate_syscall_index(cpu, &rec.s);
	rec.pid = pid;
	__relay_write(pid_chan, &rec, sizeof(rec));
}

static inline void index_pathid(const char* pathid) {
  struct pathid_index_record rec;
	int cpu = smp_processor_id();

	populate_syscall_index(cpu, &rec.s);
	memcpy(rec.pathid, pathid, PATHID_LEN);
	__relay_write(pathid_chan, &rec, sizeof(rec));
}

static inline void index_sock(uint32_t ip, uint32_t port) {
	struct sock_index_record rec;
	int cpu = smp_processor_id();

	populate_syscall_index(cpu, &rec.s);
	rec.ip		= ip;
	rec.port	= port;
	
	__relay_write(sock_chan, &rec, sizeof(rec));
}
