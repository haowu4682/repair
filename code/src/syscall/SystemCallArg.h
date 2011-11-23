// Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __SYSCALL_SYSCALLARG_H__
#define __SYSCALL_SYSCALLARG_H__

#include <sys/select.h>

#include <common/common.h>

class SystemCallArgument;

#define SYSARG_IFENTER 0x1
#define SYSARG_IFEXIT 0x2

// This struct declares some helpful information in recording a system call argument.
struct SystemCallArgumentAuxilation
{
    pid_t pid;
    int usage;
    long aux;
    long ret;
};

typedef String (*sysarg_type_t) (long argValue, SystemCallArgumentAuxilation *argAux);
typedef int (*sysarg_overwrite_t) (const SystemCallArgument *sysarg, pid_t pid, user_regs_struct &regs, int i);
// The type for a sysarg record
struct SyscallArgType
{
    String name;
    sysarg_type_t record;
    sysarg_overwrite_t overwrite;
    int usage;
    long aux;
    bool operator == (const SyscallArgType &another) const
        { return name == another.name; }
};

#define SYSARG_(type) String type##_record(long argValue, SystemCallArgumentAuxilation *argAux)
#define SYSARGOVERWRITE_(type) int type##_overwrite(const SystemCallArgument *sysarg, pid_t pid, user_regs_struct &regs, int i)

SYSARG_(void);
SYSARG_(sint);
SYSARG_(uint);
SYSARG_(sint32);
SYSARG_(uint32);
SYSARG_(intp);
SYSARG_(pid_t);
SYSARG_(buf);
SYSARG_(sha1);
SYSARG_(string);
SYSARG_(strings);
SYSARG_(iovec);
SYSARG_(fd);
SYSARG_(fd2);
SYSARG_(name);
SYSARG_(path);
SYSARG_(path_at);
SYSARG_(rpath);
SYSARG_(rpath_at);
SYSARG_(buf_det);
SYSARG_(struct);
SYSARG_(psize_t);
SYSARG_(msghdr);
#define dirfd_record    fd_record
SYSARG_(execve);

extern sysarg_type_t sysarg_type_list[];

Pair<int, int> fd2_derecord(String value);
fd_set fd_set_derecord(String value);

SYSARGOVERWRITE_(void);
SYSARGOVERWRITE_(sint);
SYSARGOVERWRITE_(uint);
SYSARGOVERWRITE_(sint32);
SYSARGOVERWRITE_(uint32);
SYSARGOVERWRITE_(intp);
SYSARGOVERWRITE_(pid_t);
SYSARGOVERWRITE_(buf);
SYSARGOVERWRITE_(sha1);
SYSARGOVERWRITE_(string);
SYSARGOVERWRITE_(strings);
SYSARGOVERWRITE_(iovec);
SYSARGOVERWRITE_(fd);
SYSARGOVERWRITE_(fd2);
SYSARGOVERWRITE_(name);
SYSARGOVERWRITE_(path);
SYSARGOVERWRITE_(path_at);
SYSARGOVERWRITE_(rpath);
SYSARGOVERWRITE_(rpath_at);
SYSARGOVERWRITE_(buf_det);
SYSARGOVERWRITE_(struct);
SYSARGOVERWRITE_(psize_t);
SYSARGOVERWRITE_(msghdr);
#define dirfd_overwrite    fd_overwrite
SYSARGOVERWRITE_(execve);

// This class declares a system call argument.
// Currently we use a **string** to represent the argument.
class SystemCallArgument
{
    public:
        SystemCallArgument() : type(NULL) { }
        SystemCallArgument(const SyscallArgType *syscallType) : type(syscallType) { }
        // Create the argument with a given register value
        void setArg(long argValue, SystemCallArgumentAuxilation *aux,
                const SyscallArgType *syscallType);
        // Create the argument from a syscall arg record
        void setArg(String record, const SyscallArgType *syscallType);
        // Set the argument to "None".
        void setArg(const SyscallArgType *syscallType);
        // Set up the sysarg name
        void setName(String newName) { name = newName; }
        // compare if two system call arguments are equal
        bool operator ==(const SystemCallArgument &) const;
        bool operator !=(const SystemCallArgument &) const;
        bool operator <(const SystemCallArgument &) const;
        // to string
        String toString() const;
        // get value
        String getValue() const;
        // set value
        void setValue(String newValue) { value = newValue; }
        // get type
        const SyscallArgType *getType() const { return type; }

        // overwrite syscall argument
        int overwrite(pid_t, user_regs_struct &regs, int i) const;
    private:
        // The name
        String name;
        // The type
        const SyscallArgType *type;
        // The value
        String value;
};

#endif //__SYSCALL_SYSCALLARG_H__

