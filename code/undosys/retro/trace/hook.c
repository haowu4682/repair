/*
 * syscall interposition.
 *
 * for most syscalls, just simply replace their pointers in sys_call_table;
 * the prototype is
 *	   asmlinkage long (*)(long, long, long, long, long, long)
 *
 * however, there are seven special syscalls, referred by ptregscall, each
 * of which is implemented by a stub_* function in assembly code, along
 * with a sys_* function in C.	to intercept it, search for
 *	   CALL sys_*
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

#include "nm.h"
#include "util.h"
#include "dbg.h"

#include "hook.h"
#include "hook_fs.h"
#include "hook_ptregscall.h"

struct hook_syscall {
	int nr;
	const char *name;
};

static
struct hook_syscall syscalls[] = {
	#include "hook_syscalls.inc"
};

static void **sys_call_table;
static void *orig_syscalls[NR_syscalls];

/* global references */
fn_hook_enter hook_enter = NULL;
fn_hook_exit  hook_exit	 = NULL;

/* NOTE. r10->rcx */
static
asmlinkage
long hooked_syscall(long rdi, long rsi, long rdx, long r10, long r8, long r9)
{
	long rax;
	long ret;

	/* new gcc > 5.0 put this asm after modifying $rax */
	asm volatile("movq %%rax, %0\n" :"=m"(rax));

	/* keep this code hierarchy */
	{
		asmlinkage long (*f)(long, long, long, long, long, long);
		long args[] = {rdi, rsi, rdx, r10, r8, r9};
		uint64_t sid;
		
		f = orig_syscalls[rax];

		if (hook_enter != NULL && hook_exit != NULL) {
			if (TRACE_COND()) {
				if (hook_enter(rax, args, &ret, &sid) == HOOK_CONT) {
					ret = f(rdi, rsi, rdx, r10, r8, r9);
				}
				hook_exit(rax, args, ret, sid);
				return ret;
			}
		}

		return f(rdi, rsi, rdx, r10, r8, r9);
	}

	/* un-reachable */
	return ret;
}

static
inline
int check_syscall(int nr)
{
	struct hook_syscall *sc;

	if (nr < 0 || nr >= sizeof(syscalls)/sizeof(syscalls[0])) {
		return 0;
	}

	sc = &syscalls[nr];

	/* not intercepted */
	if (!sc->name) {
		return 0;
	}

	/* intercepted */
	return 1;
}

static
int hook(void *data)
{
	size_t i;

	memcpy(orig_syscalls, sys_call_table, sizeof(orig_syscalls));
	for ( i = 0 ; i < NR_syscalls ; ++ i ) {
		if (check_syscall(i)) {
			struct ptregscall *c = find_ptregscall(i);
			if (c) {
				hook_ptregscall(c);
			} else	{
				sys_call_table[i] = hooked_syscall;
			}
			dbg(hook, "hooked:%s\n", syscalls[i].name);
		}
	}

	return 0;
}

static
int unhook(void *data)
{
	memcpy(sys_call_table, orig_syscalls, sizeof(orig_syscalls));
	unhook_ptregscalls();
	return 0;
}

static
int __init khook_init(void)
{
	sys_call_table = map_writable(NM_sys_call_table, sizeof(orig_syscalls));
	BUG_ON(!sys_call_table);

	if (hook_fs_init())
		return -EEXIST;

	stop_machine(hook, NULL, NULL);

	dbg(welcome, "%s:%s\n", __DATE__, __TIME__);

	return 0;
}

static
void __exit khook_exit(void)
{
	stop_machine(unhook, NULL, NULL);
	hook_fs_exit();
	unmap_writable(sys_call_table);

	msg(welcome, "=========\n");
}

/* inter modular overwrite routine */
int install_hooks(fn_hook_enter enter, fn_hook_exit exit)
{
	hook_enter = enter;
	hook_exit  = exit;

	dbg(info, "Hooked enter: %p\n", (void *) enter);
	dbg(info, "Hooked exit : %p\n", (void *) exit );

	return 0;
}

/* check hooking condition */
int hook_check_cond(void)
{
	return TRACE_COND(); 
}

module_init(khook_init);
module_exit(khook_exit);

MODULE_LICENSE("Dual MIT/GPL");
