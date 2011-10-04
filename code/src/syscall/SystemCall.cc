// Original Author: Hao Wu <haowu@cs.utexas.edu>

#include <cstdlib>
#include <cstring>
#include <sstream>
#include <sys/ptrace.h>

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

SystemCall::SystemCall(const user_regs_struct &regs, pid_t pid, bool usage, FDManager *fdManager,
        PidManager *pidManager)
{
    init(regs, pid, usage, fdManager, pidManager);
}

void SystemCall::init(const user_regs_struct &regs, pid_t pid, bool usage, FDManager *fdManager,
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
    //ret = regs.rax;
    ret = getArgFromReg(regs, SYSCALL_MAX_ARGS);
    for (int i = 0; i < numArgs; i++)
    {
        const SyscallArgType *argType = &type->args[i];
        //LOG("arg type pointer original: %d %s %p", i, type->name.c_str(), argType);
        if (argType->usage != usage)
        {
            args[i].setArg(argType);
        }
        else
        {
            SystemCallArgumentAuxilation aux = getAux(argsList, *argType, i, ret, numArgs, pid, usage);
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
            if (usage)
            {
                File *file = new File(ret, lastOpenFilePath);
                fdManager->addNewFile(file);
            }
            else
            {
                lastOpenFilePath = args[0].getValue();
            }
        }
        // close
        if (type->nr == 3)
        {
            if (!usage)
            {
                //fdManager->removeNew(atoi(args[0].getValue().c_str()));
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
SystemCallArgumentAuxilation SystemCall::getAux(long args[], const SyscallArgType &type, int i,
        long ret, int nargs, pid_t pid, bool usage)
{
    SystemCallArgumentAuxilation a;
    a.pid = pid;
    a.usage = usage;
    // TODO: Implement the following arguments
    int used[SYSCALL_MAX_ARGS + 1] = {0};

    if (type.record == iovec_record) {
        if (usage) {
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
    if (type.record == buf_record || type.record == buf_det_record
            || type.record == sha1_record) {
        if (usage) {
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
    } else if (type.record == struct_record) {
        if (usage) {
            a.aux = 0;
            if (ret == 0) {
                int size = 0;
                readFromProcess((void *)size, args[i+1], sizeof(int), pid);
                a.aux = size;
            }
        } else {
            used[i+1] = 1;
            a.aux = args[i+1];
        }
    } else if (type.record == path_at_record || type.record == rpath_at_record) {
        /* test AT_SYMLINK_(NO)FOLLOW, always the last argument */
        /*
        if (a.aux && (a.aux & args[nargs - 1])) {
            if (type.record == path_at_record)
                type.record = rpath_at_record;
            else
                type.record = path_at_record;
        }*/
        a.aux = args[i-1];
    }
    return a;
}

// Tell whether the syscall is a ``fork'' or ``vfork'' or ``clone''
bool SystemCall::isFork() const
{
    // XXX: Hard code the syscall number for x86_64 here now.
    return valid && (type->nr == 56 // clone
                ||   type->nr == 57 // fork
                ||   type->nr == 58 // vfork
                );
}

bool SystemCall::isExec() const
{
    // XXX: Hard code the syscall number for x86_64 here now.
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
            type->nr == 45 ||       // recvfrom
            type->nr == 47 ||       // recvmsg
            type->nr == 23          // select
            );
}

bool SystemCall::isOutput() const
{
    //TODO: Implement
    return false;
}

bool SystemCall::isUserInput() const
{
    return isRegularUserInput() ||
           isUserSelect() ||
           isUserPoll();
}

bool SystemCall::isUserSelect() const
{
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
            File *file = fdManager->searchNew(fd);
            if (file == NULL)
            {
                // Unknown fd, we do not treat it as user input.
                // XXX: might need to assert(file != NULL)
                LOG("Unknown fd: %d", fd);
                continue;
            }
            FileType fileTy = file->getType();
            // XXX: Do we interact both device and network?
            if (fileTy == device || fileTy == network)
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
    if (!usage)
    {
        ret = type->exec(this);
    }
    else
    {
        ret = -1;
    }
    return ret;
}

// Overwrite the syscall result into a process
int SystemCall::overwrite(pid_t pid)
{
    int ret;
    user_regs_struct regs;
    if (usage)
    {
        // Achieve current regs list
        ptrace(PTRACE_GETREGS, pid, 0, &regs);
        // Overwrite arguments
        for (int i = 0; i < type->numArgs; i++)
        {
            long argVal = getArgFromReg(regs, i);
            args[i].overwrite(pid, argVal);
        }
        // Overwrite return value
        setArgToReg(regs, SYSCALL_MAX_ARGS, ret);
        // Write back the regs list
        ptrace(PTRACE_SETREGS, pid, 0, &regs);
    }
    else
    {
        return -1;
    }
    return ret;
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
    // ADDRESS NUMBER <|> PID syscallname(arg1, arg2, ..., argN) = ret
    istringstream is(record);
    String addr;
    // Equals '<' when it's **before** a syscall. Equals '>' when it's **after** a syscall.
    char statusChar;

    String auxStr;
    String syscallName;
    String sysargName;
    String sysargValue;
    String retStr;
    int i;

    // Read the first part of the record.
    is >> addr >> seqNum >> statusChar >> pid;
    usage = ((statusChar == '<') ? true : false);
    // Now we are going to parse the args, we need some string operations here.
    is.get();
    getline(is, auxStr);
    size_t pos = 0;
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
            //LOG("%ld %ld", pos, auxStr.length());
            syscallName = auxStr.substr(0, index);
            const SyscallType *syscallType = getSyscallType(syscallName);
            if (syscallType == NULL)
            {
                LOG("The system call dose not exist: %s", record.c_str());
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
            if (usage)
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
            if (usage)
            {
                File *file = new File(ret, lastOpenFilePath);
                fdManager->addOldFile(file, seqNum);
            }
            else
            {
                lastOpenFilePath = args[0].getValue();
            }
        }
        // close
        if (type->nr == 3)
        {
            /*
            if (!usage)
            {
                fdManager->removeOld(atoi(args[0].getValue().c_str()));
            }
            */
        }
    }
}

bool SystemCall::operator ==(const SystemCall &another) const
{
    if (!valid || !another.valid)
        return false;
    if (usage != another.usage)
        return false;
    if (type != another.type)
        return false;
    for (int i = 0; i < type->numArgs; i++)
    {
        if (usage == type->args[i].usage && usage == another.type->args[i].usage)
        {
            // If it's a FD argument, use FDManager
            if (fdManager != NULL && type->args[i].record == fd_record)
            {
                int oldFD = atoi(another.args[i].getValue().c_str());
                int newFD = atoi(args[i].getValue().c_str());
                if (!fdManager->equals(oldFD, newFD, another.seqNum))
                {
                    return false;
                }
            }
            else if (!((args[i] < another.args[i]) || (another.args[i] < args[i])))
            {
                return false;
            }
        }
    }
    return true;
}

String SystemCall::toString() const
{
    ostringstream ss;
    ss << "valid=" << valid << ", ";
    if (valid)
    {
        ss << "name=" << type->name << ", ";
        ss << "usage=" << usage << ", ";
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

