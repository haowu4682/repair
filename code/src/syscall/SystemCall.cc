// Original Author: Hao Wu <haowu@cs.utexas.edu>

#include <sstream>

#include <common/common.h>
#include <syscall/SystemCall.h>
using namespace std;

SyscallType syscallTypeList[] =
{
    #include "trace_syscalls.inc"
};

#define syscallTypeListSize (sizeof(syscallTypeList) / sizeof(SyscallType))

// Search for the syscall type in syscallTypeList
const SyscallType *getSyscallType(int nr)
{
    if ((nr < 0) || (nr >= syscallTypeListSize))
    {
        return NULL;
    }
    const SyscallType *st = &syscallTypeList[nr];
    if (st->name == "")
    {
        return NULL;
    }
    return st;
}

// Search for the syscall type given its **name**
const SyscallType *getSyscallType(String name)
{
    for (int i = 0; i < syscallTypeListSize; i++)
    {
        if (syscallTypeList[i].name == name)
        {
            return &syscallTypeList[i];
        }
    }
    return NULL;
}

SystemCall::SystemCall(const user_regs_struct &regs)
{
    // XXX: The code here might be architecture-dependent for x86_64 only.
    int code = regs.orig_rax;
    type = getSyscallType(code);
    args[0].setArg(regs.rdi, NULL, &type->args[0]);
    args[1].setArg(regs.rsi, NULL, &type->args[1]);
    args[2].setArg(regs.rdx, NULL, &type->args[2]);
    args[3].setArg(regs.r10, NULL, &type->args[3]);
    args[4].setArg(regs.r8, NULL, &type->args[4]);
    args[5].setArg(regs.r9, NULL, &type->args[5]);
    ret = regs.rax;
}

long getAux(long args[])
{
    struct sysarg a = *arg;
    if (a.usage != usage || used[i])
        continue;
    if (a.ty == sysarg_iovec) {
        if (a.usage) {
            if (ret > 0) {
                a.aux = args[i+1];
                a.ret = ret;
            }
        } else {
            /* very vulnerable from user inputs */
            used[i+1] = 1;
            a.aux = args[i+1];
            a.ret = UIO_MAXIOV*PAGE_SIZE;
        }
    }
    if (a.ty == sysarg_buf || a.ty == sysarg_buf_det) {
        if (a.usage) {
            // Length of the buffer depends on the kernel.
            if (ret > 0) {
                used[6] = 1;
                a.aux = ret;
            } else
                a.aux = 0;
        } else {
            // Length of the buffer is passed in by the user.
            used[i+1] = 1;
            a.aux = args[i+1];
        }
    } else if (a.ty == sysarg_struct) {
        if (a.usage) {
            a.aux = 0;
            if (ret == 0) {
                if (sc->args[i+1].ty == sysarg_psize_t) {
                    int size = 0;
                    get_user(size, (int *)args[i+1]);
                    a.aux = size;
                }
            }
        } else {
            if (sc->args[i+1].ty == sysarg_uint) {
                used[i+1] = 1;
                a.aux = args[i+1];
            }
        }
    } else if (a.ty == sysarg_sha1) {
        // Akin to the case of sysarg_buf.
        if (a.usage) {
            a.aux = ret;
        } else {
            a.aux = args[i+1];
        }
    } else if (a.ty == sysarg_path_at || a.ty == sysarg_rpath_at) {
        /* test AT_SYMLINK_(NO)FOLLOW, always the last argument */
        if (a.aux && (a.aux & args[sc->nargs - 1])) {
            /* toggle */
            if (a.ty == sysarg_path_at)
                a.ty = sysarg_rpath_at;
            else
                a.ty = sysarg_path_at;
        }
        a.aux = args[i-1];
    }
}

// XXX: This may not apply to some system calls. It needs to be reviewed.
int SystemCall::overwrite(user_regs_struct &regs)
{
    // TODO: Do the overwrite stuff
    // regs.rax = ret;
    return 0;
}

// Tell whether the syscall is a ``fork'' or ``vfork''
bool SystemCall::isFork()
{
    // XXX: Hard code the syscall number for x86_64 here now.
    return type->nr == 57 || type->nr == 58;
}

// An aux function to parse a syscall arg.
void parseSyscallArg(String str, String *name, String *value)
{
    size_t pos = str.find_first_of('=');
    if (pos == String::npos)
    {
        LOG("Syscall arg is corrupted: %s", str.c_str());
        return;
    }
    *name = str.substr(0, pos);
    // remove the trailing ','
    *value = str.substr(pos+1, str.length() - 1);
}

// TODO: Combine the state **BEFORE** a syscall and the state **AFTER** a syscall.
int SystemCall::init(String record)
{
    // The format is:
    // ADDRESS NUMBER <|> PID syscallname(arg1, arg2, ..., argN) = ret
    istringstream is(record);
    String addr;
    long number;
    // Equals '<' when it's **before** a syscall. Equals '>' when it's **after** a syscall.
    char statusChar;
    pid_t pid;

    String auxStr;
    String syscallName;
    String sysargName;
    String sysargValue;
    int i;

    // Read the first part of the record.
    is >> addr >> number >> statusChar >> pid;
    // Now we are going to parse the args, we need some string operations here.
    is >> auxStr;
    // `6' is not hard-coded here now. The same as the max number of args hard-coded in `SystemCall.h'.
    for (i = 0; i < SYSCALL_MAX_ARGS; i++)
    {
        // The record of args hasn't finished here.
        if (i == 0)
        {
            // Extract the syscall name
            size_t index = auxStr.find_first_of('(');
            if (index == String::npos)
            {
                LOG("System call record is corrupted: %s", record.c_str());
                break;
            }
            syscallName = auxStr.substr(0, index);
            const SyscallType *syscallType = getSyscallType(syscallName);
            if (syscallType == NULL)
            {
                LOG("The system call dose not exist: %s", record.c_str());
                break;
            }
            type = syscallType;
            String sysargStr = auxStr.substr(index+1);
            parseSyscallArg(sysargStr, &sysargName, &sysargValue);
            args[i].setName(sysargName);
            args[i].setArg(sysargValue, &type->args[i]);
        }
        else
        {
            parseSyscallArg(auxStr, &sysargName, &sysargValue);
            args[i].setName(sysargName);
            args[i].setArg(sysargValue, &type->args[i]);
        }
        is >> auxStr;
        if (auxStr == "=")
        {
            // The args part has finished
            break;
        }
    }

    is >> ret;

    // Everything has been read. We now need to change the values in the SystemCall according
    // to the values here.
    valid = true;
    // type has been assigned
    // args has been assigned
    // ret has been assigned
}

