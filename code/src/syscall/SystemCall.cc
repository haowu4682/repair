// Original Author: Hao Wu <haowu@cs.utexas.edu>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <sys/ptrace.h>
#include <sys/select.h>

#include <common/common.h>
#include <common/util.h>
#include <replay/File.h>
#include <syscall/SystemCall.h>
using namespace std;

SyscallType syscallTypeList[] =
{
    #include <gen_include/trace_syscalls.inc>
};

String lastOpenFilePath;

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

SystemCall::SystemCall(const user_regs_struct &regs, pid_t pid, int usage, FDManager *fdManager,
        PidManager *pidManager)
{
    init(regs, pid, usage, fdManager, pidManager);
}

void SystemCall::init(const user_regs_struct &regs, pid_t pid, int usage, FDManager *fdManager,
        PidManager *pidManager)
{
    this->fdManager = fdManager;
    this->pidManager = pidManager;
    this->usage = usage;
    this->pid = pid;
    // XXX: The code here might be architecture-dependent for x86_64 only.
    int code = regs.orig_rax;
    type = getSyscallType(code);
    if (type == NULL)
    {
        valid = false;
        return;
    }
    else
    {
        valid = true;
    }
    long argsList[SYSCALL_MAX_ARGS];
    getRegsList(regs, argsList);
    int numArgs = type->numArgs;
    ret = getArgFromReg(regs, SYSCALL_MAX_ARGS);
    //LOG("name=%s", type->name.c_str());
    for (int i = 0; i < numArgs; i++)
    {
        const SyscallArgType *argType = &type->args[i];
        if ((argType->usage & usage) == 0)
        {
            //LOG("NOMATCH usage=%d, argtype=%s", usage, argType->name.c_str());
            args[i].setArg(argType);
        }
        else
        {
            //LOG("match usage=%d, argTypeUsage=%d, argtype=%s", usage, argType->usage, argType->name.c_str());
            SystemCallArgumentAuxilation aux = getAux(argsList, *type, i, ret, numArgs, pid, usage);
            args[i].setArg(argsList[i], &aux, argType);
        }
        //LOG("arg type pointer: %d %p", i, getArg(i).getType());
    }

    // Manager fd's
    if (fdManager != NULL)
    {
        // open
        if (type->nr == 2)
        {
            if (usage & SYSARG_IFEXIT)
            {
                if (ret >= 0)
                {
                    File *file = new File(ret, lastOpenFilePath);
//                    LOG("Add File %ld:%s", ret, lastOpenFilePath.c_str());
                    fdManager->addNewFile(file);
                }
            }
            else
            {
                lastOpenFilePath = args[0].getValue();
//                LOG("SYSCALL: %s", toString().c_str());
//                LOG("Record LastOpenFilePath = %s", lastOpenFilePath.c_str());
            }
        }
    }
}

// This is a utility function to get a args list from sets of regs
void SystemCall::getRegsList(const user_regs_struct &regs, long args[])
{
    // XXX: x86_64 specific code
    args[0] = regs.rdi;
    args[1] = regs.rsi;
    args[2] = regs.rdx;
    args[3] = regs.r10;
    args[4] = regs.r8;
    args[5] = regs.r9;
}

// This is a utility function to get a single arg from sets of regs
long SystemCall::getArgFromReg(const user_regs_struct &regs, int num)
{
    long retVal;
    switch (num)
    {
        case 0:
            retVal = regs.rdi;
            break;
        case 1:
            retVal = regs.rsi;
            break;
        case 2:
            retVal = regs.rdx;
            break;
        case 3:
            retVal = regs.r10;
            break;
        case 4:
            retVal = regs.r8;
            break;
        case 5:
            retVal = regs.r9;
            break;
        default:
            retVal = regs.rax;
            break;
    }
    return retVal;
}

// This is a utility function to get a single arg from sets of regs
void SystemCall::setArgToReg(user_regs_struct &regs, int num, long val)
{
    switch (num)
    {
        case 0:
            regs.rdi = val;
            break;
        case 1:
            regs.rsi = val;
            break;
        case 2:
            regs.rdx = val;
            break;
        case 3:
            regs.r10 = val;
            break;
        case 4:
            regs.r8 = val;
            break;
        case 5:
            regs.r9 = val;
            break;
        default:
            regs.rax = val;
            break;
    }
}

// This is an adpation of similar kernel-mode code in retro. But this one is in user-mode.
SystemCallArgumentAuxilation SystemCall::getAux(long args[], const SyscallType &syscallType, int i,
        long ret, int nargs, pid_t pid, int usage)
{
    SystemCallArgumentAuxilation a;
    a.pid = pid;
    a.usage = usage;
    const SyscallArgType &argType = syscallType.args[i];
    a.aux = argType.aux;

    // TODO: Implement the following arguments
    int used[SYSCALL_MAX_ARGS + 1] = {0};

    if (argType.record == iovec_record) {
        if (usage & SYSARG_IFEXIT) {
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
    if (argType.record == buf_record || argType.record == buf_det_record
            || argType.record == sha1_record) {
        if (usage & SYSARG_IFEXIT) {
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
    } else if (argType.record == struct_record) {
        if (usage & SYSARG_IFEXIT) {
            if (ret == 0) {
                //The following approach does not work now.
#if 0
                int size = 0;
                readFromProcess((void *)&size, args[i+1], sizeof(int), pid);
                a.aux = size;
#endif
            }
        } else {
            if (syscallType.args[i+1].record == uint_record)
            {
                used[i+1] = 1;
                a.aux = args[i+1];
            }
        }
    } else if (argType.record == path_at_record || argType.record == rpath_at_record) {
        /* test AT_SYMLINK_(NO)FOLLOW, always the last argument */
        /*
        if (a.aux && (a.aux & args[nargs - 1])) {
            if (argType.record == path_at_record)
                argType.record = rpath_at_record;
            else
                argType.record = path_at_record;
        }*/
        a.aux = args[i-1];
    }
    return a;
}

// Tell whether the syscall is a ``fork'' or ``vfork'' or ``clone''
bool SystemCall::isFork() const
{
    return valid && (type->nr == 56 // clone
                ||   type->nr == 57 // fork
                ||   type->nr == 58 // vfork
                );
}

bool SystemCall::isExec() const
{
    return valid && (type->nr == 59);
}

bool SystemCall::isPipe() const
{
    return valid && (type->nr == 22);
}

bool SystemCall::isSelect() const
{
    return valid && (type->nr == 23);
}

bool SystemCall::isPoll() const
{
    return valid && (type->nr == 7);
}

bool SystemCall::isInput() const
{
    return valid && (
            type->nr == 0 ||        // read
            type->nr == 17 ||       // pread
            type->nr == 19 ||       // readv
            type->nr == 45 ||       // recvfrom
            type->nr == 47 ||       // recvmsg
            type->nr == 23          // select
            );
}

bool SystemCall::isOutput() const
{
    //TODO: More
    return valid && (
            type->nr == 1 ||        // write
            type->nr == 18 ||       // pwrite
            type->nr == 20          // writev
            );
}

bool isFDUserInput(int fd, const FDManager *fdManager, bool isNew = true, long ts = 0)
{
    File *file;
    if (isNew)
    {
        file = fdManager->searchNew(fd);
    }
    else
    {
        file = fdManager->searchOld(fd, ts);
    }
    if (file == NULL)
    {
        // Unknown fd, we do not treat it as user input.
        LOG("Unknown fd: %d", fd);
        return false;
    }
    return file->isUserInput();
}

#if 0
bool SystemCall::isUserInput() const
{
    return isRegularUserInput() ||
           isUserSelect() ||
           isUserPoll();
}
#endif

bool SystemCall::isUserSelect(bool isNew) const
{
    if (!isSelect())
        return false;
    size_t numArgs = type->numArgs;
    // HARD CODE FOR X86_64
    for (size_t i = 1; i < 4; ++i)
    {
        const SyscallArgType *argType = &type->args[i];
        //LOG1(args[1].toString().c_str());
        if (argType->record != struct_record)
        {
            LOG("System call cannot be interpreted as select: %s", toString().c_str());
            break;
        }
        fd_set fds = fd_set_derecord(args[i].getValue().c_str());
        int nfds = atoi(args[0].getValue().c_str());
        for (int fd = 0; fd < nfds; ++fd)
        {
            if (FD_ISSET(fd, &fds))
            {
                if (isFDUserInput(fd, fdManager, isNew, ts))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

bool SystemCall::isUserPoll() const
{
    // TODO: implement
    return false;
}

bool SystemCall::isRegularUserInput() const
{
    bool ifUserInput = isInput();
    if (!ifUserInput)
        return false;
    size_t numArgs = type->numArgs;
    for (size_t i = 0; i < numArgs; ++i)
    {
        const SyscallArgType *argType = &type->args[i];
        if (argType->record == fd_record)
        {
            int fd = atoi(args[i].getValue().c_str());
            // XXX: Hard code for ``new syscall'' here.
            if (isFDUserInput(fd, fdManager, true))
            { 
                return true;
            }
        }
    }
    return false;
}

// Execute the syscall manually
int SystemCall::exec() 
{
    int ret;
    if (usage & SYSARG_IFENTER)
    {
        ret = type->exec(this);
    }
    else
    {
        ret = -1;
    }
    return ret;
}

// Merge two system calls
int SystemCall::merge(const SystemCall &src)
{
    if (isSelect() && src.isSelect())
    {
        if (ret < 0)
        {
            LOG("Select call has failed");
            return ret;
        }
        int nfds_dst = atoi(args[0].getValue().c_str());
        int nfds_src = atoi(src.args[0].getValue().c_str());
        int nfds_max = max(nfds_dst, nfds_src);
        fd_set fd_set_dst = fd_set_derecord(args[1].getValue());
        fd_set fd_set_src = fd_set_derecord(src.args[1].getValue());
        for (int oldFD = 0; oldFD < nfds_max; ++oldFD)
        {
            if (isFDUserInput(oldFD, fdManager, false, src.ts))
            {
                int newFD = fdManager->oldToNew(oldFD, src.ts);
                if (FD_ISSET(oldFD, &fd_set_src) && !FD_ISSET(newFD, &fd_set_dst))
                {
                    ++ret;
                    FD_SET(newFD, &fd_set_dst);
                }
                else if (!FD_ISSET(oldFD, &fd_set_src) && FD_ISSET(newFD, &fd_set_dst))
                {
                    --ret;
                    FD_CLR(newFD, &fd_set_dst);
                }
            }
        }
    }
    return ret;
}

// Overwrite the syscall result into a process
int SystemCall::overwrite(pid_t pid)
{
    int pret = 0;
    user_regs_struct regs;
    if (usage & SYSARG_IFEXIT)
    {
        // Achieve current regs list
        ptrace(PTRACE_GETREGS, pid, 0, &regs);
        LOG("regs=%s", regsToStr(regs).c_str());
        // Overwrite arguments
        for (int i = 0; i < type->numArgs; i++)
        {
            //long argVal = getArgFromReg(regs, i);
            args[i].overwrite(pid, regs, i);
        }
        // Overwrite return value
        ptrace(PTRACE_GETREGS, pid, 0, &regs);
        regs.orig_rax = type->nr;
        LOG("ret=%ld", ret);
        setArgToReg(regs, SYSCALL_MAX_ARGS, ret);
        // Write back the regs list
        ptrace(PTRACE_SETREGS, pid, 0, &regs);
    }
    else
    {
        return -1;
    }
    return pret;
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
    *value = str.substr(pos+1, str.length() - pos - 1);
}

// An aux function to parse a syscall arg.
size_t findPosForNextArg(String &str, int pos)
{
    size_t res;
    bool strMod = false;
    int arrayCount = 0;
    int groupCount = 0;
    for (res = pos; res < str.length(); ++res)
    {
        if (str[res] == '\"' && str[res-1] != '\\')
        {
            strMod = !strMod;
            continue;
        }
        if (!strMod && str[res] == '[' && str[res-1] != '\\')
        {
            arrayCount++;
            continue;
        }
        if (!strMod && str[res] == ']' && str[res-1] != '\\')
        {
            arrayCount--;
            continue;
        }
        if (!strMod && str[res] == '{' && str[res-1] != '\\')
        {
            groupCount++;
            continue;
        }
        if (!strMod && str[res] == '}' && str[res-1] != '\\')
        {
            groupCount--;
            continue;
        }
        if (!strMod && !arrayCount && !groupCount)
        {
            if (str[res] == ')')
                break;
            if (res != str.length() - 1 && str[res] == ',' && str[res+1] == ' ')
                break;
        }
    }
    return res;
}

int SystemCall::init(String record, FDManager *fdManager, PidManager *pidManager)
{
    this->fdManager = fdManager;
    this->pidManager = pidManager;
    // The format is:
    // ADDRESS NUMBER <|> timestamp PID syscallname(arg1, arg2, ..., argN) = ret
    istringstream is(record);
    String addr;
    // Equals '<' when it's **before** a syscall. Equals '>' when it's **after** a syscall.
    char statusChar;

    String auxStr;
    String syscallName;
    String sysargName;
    String sysargValue;
    String retStr;
    long seqNum;
    int i;

    // Read the first part of the record.
    is >> addr >> seqNum >> statusChar;
    if(statusChar != '<' && statusChar != '>')
    {
        valid = false;
        return -1;
    }
    is >> ts >> pid;
    usage = ((statusChar == '<') ? SYSARG_IFEXIT : SYSARG_IFENTER);
    // Now we are going to parse the args, we need some string operations here.
    is.get();
    getline(is, auxStr);
    size_t pos = 0;

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
            //LOG("%ld %ld", pos, auxStr.length());
            syscallName = auxStr.substr(0, index);
            const SyscallType *syscallType = getSyscallType(syscallName);
            if (syscallType == NULL)
            {
                LOG("The system call does not exist: %s", record.c_str());
                break;
            }
            type = syscallType;
            pos = index + 1;
        }
        size_t endPos = findPosForNextArg(auxStr, pos);
        if (pos > auxStr.length())
        {
            LOG("System call record is corrupted: %s", record.c_str());
        }
        String sysargStr = auxStr.substr(pos, endPos - pos);
        parseSyscallArg(sysargStr, &sysargName, &sysargValue);
        args[i].setName(sysargName);
        args[i].setArg(sysargValue, &type->args[i]);
        if (auxStr[endPos] == ')')
        {
            // The args part has finished
            if (usage & SYSARG_IFEXIT)
            {
                pos = endPos + 2;
                pos = pos + 2;
                if (pos < auxStr.length())
                {
                    retStr = auxStr.substr(pos);
                }
            }
            break;
        }
        pos = endPos + 2;
    }

    // Everything has been read. We now need to change the values in the SystemCall according
    // to the values here.
    valid = true;
    // type has been assigned
    // args has been assigned
    // ret has been assigned
    ret = atol(retStr.c_str());

    // Manager fd's
    if (fdManager != NULL)
    {
        // open
        if (type->nr == 2)
        {
            if (usage & SYSARG_IFEXIT)
            {
                File *file = new File(ret, lastOpenFilePath);
                fdManager->addOldFile(file, ts);
            }
            else
            {
                lastOpenFilePath = args[0].getValue();
            }
        }
    }
}

bool SystemCall::equals(const SystemCall &another) const
{
    if (!valid || !another.valid)
        return false;
    if (usage != another.usage)
        return false;
    if (type != another.type)
        return false;

    for (int i = 0; i < type->numArgs; i++)
    {
        if ((usage & type->args[i].usage) && (usage & another.type->args[i].usage))
        {
            // If it's a FD argument, use FDManager
            if (fdManager != NULL && type->args[i].record == fd_record)
            {
                int oldFD = atoi(another.args[i].getValue().c_str());
                int newFD = atoi(args[i].getValue().c_str());
                if (!fdManager->equals(oldFD, newFD, another.ts))
                {
                    LOG("OLDFD=%d, NEWFD=%d, NO EQUAL", oldFD, newFD);
                    return false;
                }
            }
            else if (!((args[i] < another.args[i]) || (another.args[i] < args[i])))
            {
                return false;
            }
        }
    }
    if (usage & SYSARG_IFEXIT)
    {
        if (ret != another.ret)
        {
            return false;
        }
    }
    return true;
}

bool SystemCall::match(const SystemCall &another) const
{
    if (!valid || !another.valid)
        return false;
    if (usage != another.usage)
        return false;
    if (type != another.type)
        return false;

    if (isRegularUserInput() || isOutput())
    {
        if (fdManager != NULL && type->args[0].record == fd_record)
        {
            int oldFD = atoi(another.args[0].getValue().c_str());
            int newFD = atoi(args[0].getValue().c_str());
            if (!fdManager->equals(oldFD, newFD, another.ts))
            {
                LOG("OLDFD=%d, NEWFD=%d, NO EQUAL", oldFD, newFD);
                return false;
            }
            else
            {
                return true;
            }
        }
        return args[0] == another.args[0];
    }
    else if (isSelect())
    {
        // We assume that only read fds matter here.
        // write fds do not matter because they will not be used as an input.
        fd_set read_fds1 = fd_set_derecord(args[1].getValue());
        fd_set read_fds2 = fd_set_derecord(another.args[1].getValue());
        int nfds = atoi(args[0].getValue().c_str());
        for (int newFD = 0; newFD < nfds; ++newFD)
        {
            int oldFD = fdManager->newToOld(newFD, another.ts);
            if (oldFD >= 0 && oldFD < nfds)
            {
                bool fdset1 = FD_ISSET(newFD, &read_fds1);
                bool fdset2 = FD_ISSET(oldFD, &read_fds2);
                if ((fdset1 && !fdset2) || (!fdset1 && fdset2))
                {
                    if (isFDUserInput(newFD, fdManager, true))
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    }
    else
    {
        return equals(another);
    }
}

String SystemCall::toString() const
{
    ostringstream ss;
    ss << "valid=" << valid << ", ";
    if (valid)
    {
        ss << "name=" << type->name << ", ";
        ss << "usage=" << usage << ", ";
        ss << "timestamp=" << ts << ", ";
        ss << "args=(";
        for (int i = 0; i < type->numArgs; i++)
        {
            ss << args[i].toString() << ", ";
        }
        ss << "), ";
        ss << "ret=" << ret;
    }
    return ss.str();
}

