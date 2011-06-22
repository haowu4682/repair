// Original Author: Hao Wu <haowu@cs.utexas.edu>

#include <syscall/SystemCall.h>
using namespace std;

SystemCall::SystemCall(const user_regs_struct &regs)
{
    // The code here might be architecture-dependent.
    code = regs.orig_rax;
    args[0] = regs.rdi;
    args[1] = regs.rsi;
    args[2] = regs.rdx;
    args[3] = regs.r10;
    args[4] = regs.r8;
    args[5] = regs.r9;
    ret = regs.rax;
}

// XXX: This may not apply to some system calls. It needs to be reviewed.
int SystemCall::overwrite(user_regs_struct &regs)
{
    regs.rax = ret;
}

// TODO: Implement
int SystemCall::init(String record)
{
    // The format is:
    // ADDRESS number syscall(arg1, arg2, ...)
}
