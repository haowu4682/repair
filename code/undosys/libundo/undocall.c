#include <stdio.h>
#include <unistd.h>
#include "undocall.h"

void
undo_func_start(const char *undo_mgr,
		const char *funcname,
		int arglen, const void *argbuf)
{
    syscall(SYS_undo_func_start, undo_mgr, funcname, arglen, argbuf);
}

void
undo_func_end(int retlen, const void *retbuf)
{
    syscall(SYS_undo_func_end, retlen, retbuf);
}

void
undo_mask_start(int fd)
{
    syscall(SYS_undo_mask_start, fd);
}

void
undo_mask_end(int fd)
{
    syscall(SYS_undo_mask_end, fd);
}

void
undo_depend(int fd, const char *subname, const char *mgr,
	    int proc_to_obj, int obj_to_proc)
{
    syscall(SYS_undo_depend, fd, subname, mgr, proc_to_obj, obj_to_proc);
}
