#include <stdio.h>
#include <unistd.h>
#include "undocall.h"

void
undo_func_start(const char *undo_mgr,
		const char *funcname,
		const void *argbuf, int arglen)
{
    syscall(NR_undo_mask_start, undo_mgr, funcname, argbuf, arglen);
}

void
undo_func_end(const void *retbuf, int retlen)
{
    syscall(NR_undo_func_end, retbuf, retlen);
}

void
undo_mask_start(int fd)
{
    syscall(NR_undo_mask_start, fd);
}

void
undo_mask_end(int fd)
{
    syscall(NR_undo_mask_end, fd);
}

void
undo_depend(int fd, const char *subname, const char *mgr,
	    int proc_to_obj, int obj_to_proc)
{
    syscall(NR_undo_depend, fd, subname, mgr, proc_to_obj, obj_to_proc);
}
