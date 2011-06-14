#pragma once

#include <asm/thread_info.h>
#include <linux/sched.h>

int fs_init(void);
void fs_exit(void);

/* trace channels */
struct rchan;
extern struct rchan *syscall_chan;
extern struct rchan *kprobes_chan;
extern struct rchan *inode_chan;
extern struct rchan *pid_chan;
extern struct rchan *pathid_chan;
extern struct rchan *pathidtable_chan;
extern struct rchan *sock_chan;

struct syscall_chan_info {
    size_t base;
    size_t offset;
    int nr;
    int ro;
    uint64_t sid;
};

extern struct syscall_chan_info syscall_chan_info[NR_CPUS];
extern int retro_trace_flag;
