// Original Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __SYSCALL_SYSCALL_H__
#define __SYSCALL_SYSCALL_H__

#include <sys/user.h>

#include <common/common.h>
#include <replay/Action.h>
#include <replay/FDManager.h>
#include <replay/PidManager.h>
#include <syscall/SystemCallArg.h>

// The max number of system call arguments in a system call
#define SYSCALL_MAX_ARGS 6
class SystemCall;
typedef int (*syscall_exec_t) (SystemCall *syscall);

// The **type** of a syscall describes which syscall it is
// e.g. "read", "fork", etc.
// It does not contain the **value** of the syscall
struct SyscallType
{
    int nr;
    String name;
    size_t numArgs;
    SyscallArgType args[6];
    bool operator ==(SyscallType &another) { return nr == another.nr && name == another.name; }
    syscall_exec_t exec;
};

#define SYSCALL_(type) int type##_exec(SystemCall *syscall)
#include <gen_include/trace_syscalls_exec.inc>

extern SyscallType syscallTypeList[];
#define syscallTypeListSize (sizeof(syscallTypeList) / sizeof(SyscallType))

// The class is used to represent the **record** of a system call
// @author haowu
class SystemCall : public Action
{
    public:
        SystemCall() : valid(false), fdManager(NULL) {}
        // Construct the system call from registers
        SystemCall(const user_regs_struct &regs, pid_t pid, bool usage, FDManager *fdManager = NULL,
                PidManager *pidManager = NULL);
        SystemCall(String record, FDManager *fdManager = NULL, PidManager *pidManager = NULL)
        { this->init(record, fdManager, pidManager); }

        // Whether two syscalls are equal. Two syscalls are equal if their usages are equal, and
        // each available arguments and return value(if usage==true) are equal
        bool operator == (SystemCall &);

        // Whether the system call is valid.
        // A valid system call is a system with its code and args provided.
        // @author haowu
        bool isValid() { return valid;}

        // Given the current registers, overwrite the return value in the registers
        // with current return values of the system call.
        // @author haowu
        // @param regs The current registers, will be modified.
        int overwrite(user_regs_struct &regs);

        // Init a system call from a record
        // @param record The record
        int init(String record, FDManager *fdManager = NULL, PidManager *pidManager = NULL);
        void init(const user_regs_struct &regs, pid_t pid, bool usage, FDManager *fdManager = NULL,
                PidManager *pidManager = NULL);

        // Tell whether the system call is a ``fork'' or ``vfork''
        bool isFork() const;

        // Tell whether the system call is an ``exec'' type syscall
        bool isExec() const;

        // Get return value
        long getReturn() const { return ret; }

        // Get pid which owns the syscall
        pid_t getPid() const { return pid; }

        bool getUsage() const { return usage;}

        // Get arguments;
        const SystemCallArgument *getArgs() const { return args;}

        // Get an argument
        const SystemCallArgument &getArg(int i) const { return args[i]; }

        // Get syscall args list from regs
        // Warning: The size of the given list must be at least SYSCALL_MAX_ARGS!
        // We do not check it here.
        static void getRegsList(const user_regs_struct &regs, long args[]);

        // to string
        String toString() const;

        // execute the systemcall
        virtual int exec();
    private:
        // Get an aux value for determing an argument
        static SystemCallArgumentAuxilation getAux(long args[], SyscallArgType &argType, int i,
                long ret, int nargs, pid_t pid, bool usage);
        // If the system call is valid
        bool valid;
        // When the syscall is taken *before* at a syscall entry, it is false. Else it is true
        bool usage;
        // The system call type
        const SyscallType *type;
        // The system call args
        // `6' is not hard-coded here now.
        SystemCallArgument args[SYSCALL_MAX_ARGS];
        // The return value of the system call
        long ret;
        // The old pid which owns the syscall
        pid_t pid;
        // The fd manager
        FDManager *fdManager;
        // The pid manager
        PidManager *pidManager;
};

#endif //__SYSCALL_SYSCALL_H__

