#ifndef _UNDOCALL_H
#define _UNDOCALL_H

void	undo_func_start(const char *undo_mgr, const char *funcname,
			const void *argbuf, int arglen);
void	undo_func_end(const void *retbuf,int retlen);
void	undo_mask_start(int fd);
void	undo_mask_end(int fd);
// FIX: put undo_depend before calling the original function
//      otherwise the repair part may pick up a wrong snapshot
void	undo_depend(int fd, const char *subname, const char *mgr,
		    int proc_to_obj, int obj_to_proc);

enum {
    #include "undosyscalls.inc"
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
