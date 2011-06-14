#ifndef _UNDOCALL_H
#define _UNDOCALL_H

void	undo_func_start(const char *undo_mgr, const char *funcname,
			int arglen, const void *argbuf);
void	undo_func_end(int retlen, const void *retbuf);
void	undo_mask_start(int fd);
void	undo_mask_end(int fd);
// FIX: put undo_depend before calling the original function
//      otherwise the repair part may pick up a wrong snapshot
void	undo_depend(int fd, const char *subname, const char *mgr,
		    int proc_to_obj, int obj_to_proc);

enum {
    SYS_undo_base = 0xd0d0d0,
    SYS_undo_func_start = SYS_undo_base,
    SYS_undo_func_end,
    SYS_undo_mask_start,
    SYS_undo_mask_end,
    SYS_undo_depend,
    SYS_undo_max
};

#define undo_depend_to(fd, s, mgr)   undo_depend(fd, s, mgr, 1, 0)
#define undo_depend_from(fd, s, mgr) undo_depend(fd, s, mgr, 0, 1)
#define undo_depend_both(fd, s, mgr) undo_depend(fd, s, mgr, 1, 1)

#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>

#define UNDO_ORIG(F) get_##F()
#define UNDO_WRAP(F, R, P) \
	typedef R (*proto_##F) P; \
	proto_##F lower_##F; \
	static proto_##F get_##F() \
	{ \
		if (!lower_##F) { \
			lower_##F = dlsym(RTLD_NEXT, #F); \
			if (!lower_##F) { \
				fprintf(stderr, "unable to resolve symbol %s\n", #F); \
				assert(0); \
			} \
			/*fprintf(stderr, "%s done\n", #F);*/ \
		} \
		return lower_##F; \
	} \
	R F P

#endif
