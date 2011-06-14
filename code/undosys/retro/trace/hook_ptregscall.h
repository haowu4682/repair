#pragma once

#include <linux/syscalls.h>

struct ptregscall {
	int nr;      /* syscall number */
	void *stub;  /* stub_* address */
	void *sys;   /* sys_* address */
	void *trace; /* our trace_* address */
	void *rel32; /* `CALL rel32' address */
};

struct ptregscall * find_ptregscall(int nr);
void hook_ptregscall(struct ptregscall *);
void unhook_ptregscalls(void);
