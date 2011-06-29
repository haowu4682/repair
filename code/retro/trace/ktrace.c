/*
 * syscall interposition.
 *
 * for most syscalls, just simply replace their pointers in sys_call_table;
 * the prototype is
 *     asmlinkage long (*)(long, long, long, long, long, long)
 *
 * however, there are seven special syscalls, referred by ptregscall, each
 * of which is implemented by a stub_* function in assembly code, along
 * with a sys_* function in C.	to intercept it, search for
 *     CALL sys_*
 * in stub_*, and replace the target address with a trace function.
 *
 * note that kprobes doesn't work for syscall interposition in general,
 * because it generates a trampoline for each call --- if a syscall is
 * blocking, kprobes may exceed the maximum number of trampolines and fail
 * to trace the syscall.
 */

#include <asm/unistd.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/stop_machine.h>

#include "config.h"

#include "nm.h"
#include "util.h"
#include "dbg.h"
#include "hook.h"

#include "ktrace.h"
#include "kprobes.h"
#include "syscall.h"
#include "fs.h"

// XXX
//
// load this list when loading the retro
//
static char * RETRO_WHITE_LIST[] = { "/gcc", "/cc1", "/ld", "/as", NULL };

static
long retro_trace_enter(long nr, long args[6], long *retp, uint64_t *idp)
{
	int cond = HOOK_CONT;

	dbg(trace, "enter, nr:%d, pid:%d, flag:%d",
	    (int) nr, task_pid_nr(current), (int) retro_trace_flag);

	if (retro_trace_flag != TRACE_RECORD_EXIT) {
		cond = syscall_enter(task_pid_nr(current), nr, args, retp, idp);
		if (retro_trace_flag == TRACE_RECORD_ENTRY) {
			return HOOK_SKIP;
		}
	} else {
		dbg(trace, "%s", "skip trace_enter()");
	}

#ifdef RETRO_OPT_DET
	// check if deterministic process
	if (retro_trace_flag == TRACE_RECORD && nr == __NR_execve) {
		char * s = getname((const char *)args[0]);
		if (!IS_ERR(s)) {
			char ** proc = &RETRO_WHITE_LIST[0];
			while (*proc) {
				if (strstr(s, *proc)) {
					dbg(det, "set %s (by %s rule) deterministic:%d",
					    s, *proc, (int)current->pid);
					set_deterministic(current);
					break;
				}
				proc ++;
			}
			putname(s);
		}
	}
#endif
	return HOOK_CONT;
}

static
void retro_trace_exit(long nr, long args[6], long ret, uint64_t id)
{
	dbg(trace, "exit, nr:%d", (int) nr);

	if (retro_trace_flag != TRACE_RECORD_ENTRY) {
		syscall_exit(task_pid_nr(current), nr, args, ret, id);
	} else {
		dbg(trace, "%s", "skip trace_exit()");
	}

#ifdef RETRO_OPT_DET
	/* make deterministic inherent */
	if (unlikely(nr == __NR_vfork && ret != -1)) {
		if (is_deterministic(current)) {
			struct task_struct * task
				= pid_task(find_get_pid(ret), PIDTYPE_PID);
			if (task) {
				dbg(det, "inherent deterministic:%d", (int)ret);
				set_deterministic(task);
			}
		}
	}
#endif
}

static int __init retro_init(void)
{
	/* init relay & kprobes channels */
	if (fs_init())
		return -EEXIST;
	
	if (kprobes_init()) {
		fs_exit();
		return -EEXIST;
	}

	dbg(welcome, "%s:%s\n", __DATE__, __TIME__);

	return register_hooks(retro_trace_enter, retro_trace_exit);
}

static void __exit retro_exit(void)
{
	unregister_hooks();
	kprobes_exit();
	fs_exit();

	msg(welcome, "=========\n");
}

module_init(retro_init);
module_exit(retro_exit);

MODULE_LICENSE("Dual MIT/GPL");
