// Author: Hao Wu <haowu@cs.utexas.edu>
// TODO: Implement everything

#include <cstdlib>
#include <cstring>
#include <sys/ptrace.h>

#include <common/util.h>
#include <syscall/SystemCall.h>
#include <syscall/SystemCallArg.h>

SYSARGOVERWRITE_(void)
{
    return 0;
}

inline long writeInt(long value, pid_t pid, user_regs_struct &regs, int i)
{
    ptrace(PTRACE_GETREGS, pid, 0, &regs);
    SystemCall::setArgToReg(regs, i, value);
    return ptrace(PTRACE_SETREGS, pid, 0, &regs);
}

SYSARGOVERWRITE_(sint)
{
    return 0;
}

SYSARGOVERWRITE_(uint)
{
    //TODO: not necessarily to be correct
    long value = atoi(sysarg->getValue().c_str());
    return writeInt(value, pid, regs, i);
}

SYSARGOVERWRITE_(sint32)
{
    return 0;
}

SYSARGOVERWRITE_(uint32)
{
    return 0;
}

SYSARGOVERWRITE_(intp)
{
    return 0;
}

SYSARGOVERWRITE_(pid_t)
{
    return 0;
}

SYSARGOVERWRITE_(buf)
{
    long pret;
    // Modify the buf value
    String str = sysarg->getValue();
    long argVal = SystemCall::getArgFromReg(regs, i);
    if (str[0] == '\"')
    {
        writeToProcess(str.c_str()+1, argVal, str.size()-2, pid);
    }
    else
    {
        writeToProcess(str.c_str(), argVal, str.size(), pid);
    }
    // Modify the length by modifying the return value.
    // This part should be done by SystemCall::overwrite. We do not execute it
    // here.
    return 0;
}

SYSARGOVERWRITE_(sha1)
{
    return buf_overwrite(sysarg, pid, regs, i);
}

SYSARGOVERWRITE_(string)
{
    return 0;
}

SYSARGOVERWRITE_(strings)
{
    return 0;
}

SYSARGOVERWRITE_(iovec)
{
    return 0;
}

SYSARGOVERWRITE_(fd)
{
    return 0;
}

SYSARGOVERWRITE_(fd2)
{
    return 0;
}

SYSARGOVERWRITE_(name)
{
    return 0;
}

SYSARGOVERWRITE_(path)
{
    return 0;
}

SYSARGOVERWRITE_(path_at)
{
    return 0;
}

SYSARGOVERWRITE_(rpath)
{
    return 0;
}

SYSARGOVERWRITE_(rpath_at)
{
    return 0;
}

SYSARGOVERWRITE_(buf_det)
{
    buf_overwrite(sysarg, pid, regs, i);
    return 0;
}

SYSARGOVERWRITE_(struct)
{
    // XXX: ad-hoc
    long pret;
    if (sysarg->getValue() == "None")
    {
        SystemCallArgument fakeArg;
        fakeArg.setArg("0", NULL);
        uint_overwrite(sysarg, pid, regs, i);
    }
    else if (sysarg->getType()->aux == 128)
    {
        fd_set fds = fd_set_derecord(sysarg->getValue());
        long argVal = SystemCall::getArgFromReg(regs, i);
        pret = writeToProcess(&fds, argVal, sizeof(fds), pid);
        return pret;
    }
    return 0;
}

SYSARGOVERWRITE_(psize_t)
{
    return 0;
}

SYSARGOVERWRITE_(msghdr)
{
    return 0;
}

SYSARGOVERWRITE_(execve)
{
    return 0;
}

