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
    //TODO: not necessarily to be correct
    long value = atoi(sysarg->getValue().c_str());
    return writeInt(value, pid, regs, i);
}

SYSARGOVERWRITE_(uint)
{
    //TODO: not necessarily to be correct
    long value = atoi(sysarg->getValue().c_str());
    return writeInt(value, pid, regs, i);
}

SYSARGOVERWRITE_(sint32)
{
    //TODO: not necessarily to be correct
    long value = atoi(sysarg->getValue().c_str());
    return writeInt(value, pid, regs, i);
}

SYSARGOVERWRITE_(uint32)
{
    //TODO: not necessarily to be correct
    long value = atoi(sysarg->getValue().c_str());
    return writeInt(value, pid, regs, i);
}

SYSARGOVERWRITE_(intp)
{
    //TODO: not necessarily to be correct
    long value = atoi(sysarg->getValue().c_str());
    return writeInt(value, pid, regs, i);
}

SYSARGOVERWRITE_(pid_t)
{
    //TODO: not necessarily to be correct
    long value = atoi(sysarg->getValue().c_str());
    return writeInt(value, pid, regs, i);
}

SYSARGOVERWRITE_(buf)
{
    long pret;
    // Modify the buf value
    String str = sysarg->getValue();
    long argVal = SystemCall::getArgFromReg(regs, i);
    pret = writeToProcess(str.c_str(), argVal, str.size(), pid);
    // Modify the length by modifying the return value.
    // This part should be done by SystemCall::overwrite. We do not execute it
    // here.
    return pret;
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
        pret = 0;
    }
    else if (sysarg->getType()->aux == 128)
    {
        fd_set fds = fd_set_derecord(sysarg->getValue());
        long argVal = SystemCall::getArgFromReg(regs, i);
        pret = writeToProcess(&fds, argVal, sizeof(fds), pid);
    }
    return pret;
}

SYSARGOVERWRITE_(psize_t)
{
    return 0;
}

long writeMsgBack(msghdr hdr, const char *buf, long hdr_addr, pid_t pid)
{
    long pret;
    const char *name_buf, *iov_buf, *control_buf;

    // Step 0 Initialize
    name_buf = buf;
    iov_buf = buf + hdr.msg_namelen;
    control_buf = buf + hdr.msg_namelen + hdr.msg_iovlen;

    // Step 1 write back data
    if (hdr.msg_name != NULL)
    {
        pret = writeToProcess(name_buf, long(hdr.msg_name), hdr.msg_namelen, pid);
        TESTERROR(pret, "msghdr overwrite failed, unable to overwrite msg_name at"
                " %p with length %d.", hdr.msg_name, hdr.msg_namelen);
    }
    if (hdr.msg_iov != NULL)
    {
        pret = writeToProcess(iov_buf, long(hdr.msg_iov), hdr.msg_iovlen, pid);
        TESTERROR(pret, "msghdr overwrite failed, unable to overwrite msg_iov at"
                " %p with length %ld.", hdr.msg_iov, hdr.msg_iovlen);
    }
    if (hdr.msg_control != NULL)
    {
        pret = writeToProcess(control_buf, long(hdr.msg_control), hdr.msg_controllen, pid);
        TESTERROR(pret, "msghdr overwrite failed, unable to overwrite msg_control at"
                " %p with length %ld.", hdr.msg_control, hdr.msg_controllen);
    }

    // Step 2 write back msghdr
    pret = writeToProcess(&hdr, hdr_addr, sizeof(msghdr), pid);
    TESTERROR(pret, "msghdr overwrite failed, unable to overwrite msghdr at"
            " %p with length %ld.", (void *)hdr_addr, sizeof(msghdr));
    return 0;
}

SYSARGOVERWRITE_(msghdr)
{
    long pret;
    msghdr hdr;
    long hdr_addr;
    String value;
    size_t total_len;
    char *hdr_buf;

    // Step 1: Get original msghdr and data
    value = sysarg->getValue();
    hdr_addr = SystemCall::getArgFromReg(regs, i);
    pret = readFromProcess(&hdr, hdr_addr, sizeof(struct msghdr), pid);
    if (pret < 0)
    {
        LOG("read msghdr fails from addr %ld", hdr_addr);
        return pret;
    }

    // Step 2: Create buf for specified msghdr
    total_len = hdr.msg_namelen + hdr.msg_iovlen + hdr.msg_controllen;
    LOG("total len = %ld", total_len);
    hdr_buf = new char[total_len];

    // Step 3: derecord the record to fill in the buf
    msghdr_derecord(value, &hdr, hdr_buf);

    // Step 4: write buf back according to msghdr
    writeMsgBack(hdr, hdr_buf, hdr_addr, pid);

    // Step 5: clean things up
    LOG("before deleting buf");
    delete[] hdr_buf;
    LOG("after deleting buf");
    return pret;
}

SYSARGOVERWRITE_(execve)
{
    return 0;
}

