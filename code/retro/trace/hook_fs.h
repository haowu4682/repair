#pragma once

#include <asm/thread_info.h>
#include <linux/sched.h>

#include "config.h"

int  hook_fs_init(void);
void hook_fs_exit(void);

/* hooking options */
#define HOOK_PF_GLOBAL  (0)
#define HOOK_PF_PROCESS (1)

/* trace helpers */
#define TRACE_COND() ((hook_enabled_flag)                   \
        && (task_tgid_nr(current) != hook_ctl_pid)          \
        && (hook_process_flag == HOOK_PF_GLOBAL || current->ptrace))

/* trace flags */
extern int hook_enabled_flag;
extern int hook_process_flag;
extern pid_t hook_ctl_pid;
