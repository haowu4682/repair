// Original Author: Hao Wu <haowu@cs.utexas.edu>

#include <sstream>

#include <common/common.h>
#include <syscall/SystemCall.h>
using namespace std;

SyscallType syscallTypeList[] =
{
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
    // The code here might be architecture-dependent.
    int code = regs.orig_rax;
    type = getSyscallType(code);
    args[0].setArg(regs.rdi, NULL);
    args[0].setArg(regs.rsi, NULL);
    args[0].setArg(regs.rdx, NULL);
    args[0].setArg(regs.r10, NULL);
    args[0].setArg(regs.r8, NULL);
    args[0].setArg(regs.r9, NULL);
    ret = regs.rax;
}

// XXX: This may not apply to some system calls. It needs to be reviewed.
int SystemCall::overwrite(user_regs_struct &regs)
{
    regs.rax = ret;
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

