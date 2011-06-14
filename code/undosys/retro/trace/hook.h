#pragma once

#include <linux/types.h>

#include "config.h"

/* hook for most syscalls */
typedef long (*fn_hook_enter)(long nr, long args[6], long *retp, uint64_t *idp);
typedef void (*fn_hook_exit )(long nr, long args[6], long  ret , uint64_t  id );

typedef unsigned long (*fn_lookup_symbol)(const char *name);
typedef int (*fn_install_hooks)(fn_hook_enter enter, fn_hook_exit exit);

/* use this for register hook_enter & hook_exit */
static
inline
int register_hooks(fn_hook_enter enter, fn_hook_exit exit)
{
	fn_lookup_symbol lookup_symbol;
	fn_install_hooks install_hooks;

	lookup_symbol = (fn_lookup_symbol) NM_module_kallsyms_lookup_name;
	install_hooks = (fn_install_hooks) lookup_symbol("install_hooks");

	if (!install_hooks) {
		printk(KERN_ERR "register_hooks(): insert hook.ko module first!\n");
		return -1;
	}

	install_hooks(enter, exit);

	return 0;
}

static
inline
void unregister_hooks(void)
{
	(void) register_hooks(NULL, NULL);
}