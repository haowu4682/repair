// Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __SYSCALL_SYSCALLARG_H__
#define __SYSCALL_SYSCALLARG_H__

#include <common/common.h>

class SystemCallArgument;

// This struct declares some helpful information in recording a system call argument.
struct SystemCallArgumentAuxilation
{
    pid_t pid;
    bool usage;
    long aux;
    long ret;
};

typedef String (*sysarg_type_t) (long argValue, SystemCallArgumentAuxilation *argAux);
// The type for a sysarg record
struct SyscallArgType
{
    String name;
    sysarg_type_t record;
    bool usage;
    long aux;
    bool operator == (SyscallArgType &another) { return name == another.name; }
};

#define SYSARG_(type) String type##_record(long argValue, SystemCallArgumentAuxilation *argAux)

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

// This class declares a system call argument.
// Currently we use a **string** to represent the argument.
class SystemCallArgument
{
    public:
        SystemCallArgument() : type(NULL) { }
        SystemCallArgument(const SyscallArgType *syscallType) : type(syscallType) { }
        // Create the argument with a given register value
        void setArg(long argValue, SystemCallArgumentAuxilation *aux,
                const SyscallArgType *syscallType = NULL);
        // Create the argument from a syscall arg record
        void setArg(String record, const SyscallArgType *syscallType = NULL);
        // Set the argument to "None".
        void setArg();
        // Set up the sysarg name
        void setName(String newName) { name = newName; }
        // compare if two system call arguments are equal
        bool operator ==(SystemCallArgument &);
        bool operator !=(SystemCallArgument &);
        bool operator <(SystemCallArgument &);
        // to string
        String toString() const;
        // get value
        String getValue() const;
    private:
        // The name
        String name;
        // The type
        const SyscallArgType *type;
        // The value
        String value;
};

#endif //__SYSCALL_SYSCALLARG_H__

