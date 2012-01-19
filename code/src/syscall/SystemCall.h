// Original Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __SYSCALL_SYSCALL_H__
#define __SYSCALL_SYSCALL_H__

#include <sys/user.h>

#include <common/common.h>
#include <replay/Action.h>
#include <replay/FDManager.h>
#include <replay/PidManager.h>
#include <syscall/SystemCallArg.h>
//#include <syscall/Timestamp.h>

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
    bool operator ==(const SyscallType &another) const
        { return nr == another.nr && name == another.name; }
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
        SystemCall(const user_regs_struct &regs, pid_t pid, int usage, FDManager *fdManager = NULL,
                PidManager *pidManager = NULL);
        SystemCall(String record, FDManager *fdManager = NULL, PidManager *pidManager = NULL)
        { this->init(record, fdManager, pidManager); }

        // Whether two syscalls are equal. Two syscalls are equal if their usages are equal, and
        // each available arguments and return value(if usage==true) are equal
        bool operator == (const SystemCall &syscall) const
        {
            return equals(syscall);
        }
        bool equals(const SystemCall &) const;
        // Whether two syscalls are equalivant as a user input or an output.
        // If the syscall is not a user input or output, it just returns to regular
        // equivalence.
        bool match(const SystemCall &) const;

        // Whether the system call is valid.
        // A valid system call is a system with its code and args provided.
        // @author haowu
        bool isValid() const { return valid; }

        // Init a system call from a record
        // @param record The record
        int init(String record, FDManager *fdManager = NULL, PidManager *pidManager = NULL);
        void init(const user_regs_struct &regs, pid_t pid, int usage, FDManager *fdManager = NULL,
                PidManager *pidManager = NULL);

        // Tell whether the system call is a ``fork'' or ``vfork''
        bool isFork() const;

        // Tell whether the system call is an ``exec'' type syscall
        bool isExec() const;

        bool isPipe() const;

        // Tell whether the system call is a user input
        // A user input is an input whose source is from user.
        //bool isUserInput() const;

        // Tell whether the system call is a input
        bool isInput() const;

        // Tell whether the system call is a output
        bool isOutput() const;

        // Tell whether the system call is a ``select''
        bool isSelect() const;

        // Tell whether the system call is a ``poll''
        bool isPoll() const;

        // This function returns if a user input is a regular user input(i.e.
        // read or recvfrom)
        bool isRegularUserInput() const;

        // This function returns if a user input is a `select' for a user input.
        bool isUserSelect(bool isNew) const;

        // This function returns if a user input is a `poll' for a user input.
        bool isUserPoll() const;

        // Get return value
        long getReturn() const { return ret; }

        // Get pid which owns the syscall
        pid_t getPid() const { return pid; }

        int getUsage() const { return usage;}

        // Get arguments;
        const SystemCallArgument *getArgs() const { return args;}

        // Get an argument
        const SystemCallArgument &getArg(int i) const { return args[i]; }

        // Get the sequence number
        long getSeqNum() const { return seqNum; }

        // Get the type
        const SyscallType *getType() const { return type; }

        // Get syscall args list from regs
        // Warning: The size of the given list must be at least SYSCALL_MAX_ARGS!
        // We do not check it here.
        static void getRegsList(const user_regs_struct &regs, long args[]);

        // Get the value of certain syscall arg. It returns the relative
        // value in the register, not the actual argument. For example, if an
        // argument is buf = "Hello World", the return value should be &buf.
        // @param regs the current register frame
        // @param num the sequence number of the argument. If it's out of
        // range(0...5), it in fact returns the ``return value''. This is
        // something to be aware of.
        static long getArgFromReg(const user_regs_struct &regs, int num);

        // Set the value of certain syscall arg to a register frame. The
        // function is similar with getArgFromReg.
        // @param regs the current register frame
        // @param num the sequence number of the argument. If it's out of
        // range(0...5), it in fact means the ``return value''. This is
        // something to be aware of.
        static void setArgToReg(user_regs_struct &regs, int num, long value);

        // Get fd manager
        FDManager *getFDManager() const { return fdManager; }

        // Get pid manager
        PidManager *getPidManager() const { return pidManager; }

        // Get Timestamp
        //Timestamp getTimestamp() const { return ts; }
        long getTimestamp() const { return ts; }

        // to string
        String toString() const;
        //friend std::ostream &operator <<(std::ostream &os, SystemCall &syscall) 
        //{ syscall.toString(os); }

        // execute the systemcall
        virtual int exec();

        // Overwrite return value of the system call into a process
        int overwrite(pid_t pid);

        // The function is used to merge the result of two different syscalls. Currently
        // it is only used for dealing with select, poll, and epoll.
        // @param src The source system call. We will merge the result of user input fds
        // in src into dst
        int merge(const SystemCall &src);

    private:
        // Get an aux value for determing an argument
        static SystemCallArgumentAuxilation getAux(long args[], const SyscallType &syscallType, int i,
                long ret, int nargs, pid_t pid, int usage);

        // If the system call is valid
        bool valid;
        // When the syscall is taken *before* at a syscall entry, it is false. Else it is true
        int usage;
        // The system call type
        const SyscallType *type;
        // The system call args
        SystemCallArgument args[SYSCALL_MAX_ARGS];
        // The return value of the system call
        long ret;
        // The seq number of the system call
        long seqNum;
        // The old pid which owns the syscall
        pid_t pid;
        // The fd manager
        FDManager *fdManager;
        // The pid manager
        PidManager *pidManager;
        // Timestamp
        //Timestamp ts;
        long ts;
};

#endif //__SYSCALL_SYSCALL_H__

