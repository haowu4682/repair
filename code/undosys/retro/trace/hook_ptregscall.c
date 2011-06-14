#include <linux/sched.h>

#include "nm.h"
#include "util.h"
#include "dbg.h"

#include "hook.h"
#include "hook_fs.h"
#include "hook_ptregscall.h"

/* from hook module */
extern fn_hook_enter hook_enter;
extern fn_hook_exit  hook_exit;

#define PTREGSCALL(x, R, name, ...)                                     \
    typedef R (*sys_##name##_t)(__SC_DECL##x(__VA_ARGS__));             \
    static R trace_##name(__SC_DECL##x(__VA_ARGS__));                   \
    static struct ptregscall ptregs_##name = {                          \
        __NR_##name, NM_stub_##name, NM_sys_##name, trace_##name        \
    };                                                                  \
    static R trace_##name(__SC_DECL##x(__VA_ARGS__))                    \
    {                                                                   \
        sys_##name##_t orig = (sys_##name##_t)(ptregs_##name.sys);      \
        long args[] = {regs->di, regs->si, regs->dx,                    \
                       regs->r10, regs->r8, regs->r9};                  \
        R ret = 0;                                                      \
        uint64_t sid;                                                   \
        if (hook_enter != NULL && hook_exit != NULL) {                  \
            if (TRACE_COND()) {                                         \
                if (hook_enter(ptregs_##name.nr, args, 0, &sid)         \
                        == HOOK_CONT) {                                 \
                    ret = orig(__SC_CAST##x(__VA_ARGS__));              \
                }                                                       \
                hook_exit(ptregs_##name.nr, args, ret, sid);            \
                return ret;                                             \
            }                                                           \
        }                                                               \
        return orig(__SC_CAST##x(__VA_ARGS__));                         \
    }

/* seven ptregscalls in x86-64 */
/* probably only clone and execve are necessary */

PTREGSCALL(5, long, clone, unsigned long, clone_flags, unsigned long, newsp,
    void __user *, parent_tid, void __user *, child_tid, struct pt_regs *, regs);
PTREGSCALL(4, long, execve, char *, name, char **, argv, char **, envp,
    struct pt_regs *, regs);
PTREGSCALL(1, int, fork, struct pt_regs *, regs)
PTREGSCALL(2, long, iopl, int, level, struct pt_regs *, regs);
PTREGSCALL(1, long, rt_sigreturn, struct pt_regs *, regs);
PTREGSCALL(3, long, sigaltstack, const stack_t __user *, uss,
    stack_t __user *, uoss, struct pt_regs *, regs);
PTREGSCALL(1, int, vfork, struct pt_regs *, regs);

static struct ptregscall *ptregscalls[] = {
    &ptregs_clone,
    &ptregs_execve,
    &ptregs_fork,
    &ptregs_iopl,
    &ptregs_rt_sigreturn,
    &ptregs_sigaltstack,
    &ptregs_vfork,
};

struct ptregscall * find_ptregscall(int nr)
{
    size_t i;
    for (i = 0; i < sizeof(ptregscalls)/sizeof(*ptregscalls); ++i)
        if (ptregscalls[i]->nr == nr)
            return ptregscalls[i];
    return NULL;
}

void hook_ptregscall(struct ptregscall *call)
{
    void *p = (void *)call->stub;
    int offset;
    int32_t *q;
    for (;;) {
        // call rel32
        p = memchr(p, 0xe8, 1024);
        BUG_ON(!p);
        ++p;
        offset = *(int32_t *)p;
        if (offset == call->sys - (p + 4))
            break;
    }
    call->rel32 = p;
    q = map_writable(p, 4);
    *q = call->trace - (p + 4);
    unmap_writable(q);
}

void unhook_ptregscalls(void)
{
    size_t i;
    for (i = 0; i < sizeof(ptregscalls)/sizeof(*ptregscalls); ++i) {
        struct ptregscall *c = ptregscalls[i];
        void *p = c->rel32;
        if (p) {
            int32_t *q = map_writable(p, 4);
            *q = c->sys - (p + 4);
            unmap_writable(q);
        }
    }
}
