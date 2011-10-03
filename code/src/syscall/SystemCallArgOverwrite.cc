// Author: Hao Wu <haowu@cs.utexas.edu>
// TODO: Implement everything

#include <common/util.h>
#include <syscall/SystemCallArg.h>

SYSARGOVERWRITE_(void)
{
    return 0;
}

SYSARGOVERWRITE_(sint)
{
    return 0;
}

SYSARGOVERWRITE_(uint)
{
    return 0;
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
    writeToProcess(str.c_str(), argVal, str.size(), pid);
    // Modify the length by modifying the return value.
    // This part should be done by SystemCall::overwrite. We do not execute it
    // here.
    return 0;
}

SYSARGOVERWRITE_(sha1)
{
    return buf_overwrite(sysarg, pid, argVal);
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
    return 0;
}

SYSARGOVERWRITE_(struct)
{
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

