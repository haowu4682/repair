// Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __SYSCALL_SYSCALLARG_H__
#define __SYSCALL_SYSCALLARG_H__

#include <common/common.h>

class SystemCallArgument;

// This struct declares some helpful information in recording a system call argument.
struct SystemCallArgumentAuxilation
{
    pid_t pid;
    long aux;
};

// XXX: Here we use C-style definition, since the author (haowu) don't know how to implement them
// in C++-style.
// The type for a sysarg record
class SysargType
{
    public:
        // from regValue to string
        String toString(long argValue);
};
typedef String (*sysarg_type_t) (long argValue, SystemCallArgumentAuxilation *argAux);
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
#define sysarg_dirfd    sysarg_fd
SYSARG_(execve);

sysarg_type_t sysarg_type_list[] =
{
    void_record,
    sint_record,
    uint_record,
    sint32_record,
    uint32_record,
    intp_record,
    pid_t_record,
    buf_record,
    sha1_record,
    string_record,
    strings_record,
    iovec_record,
    fd_record,
    fd2_record,
    name_record, 
    path_record,
    path_at_record,
    rpath_record,
    rpath_at_record,
    buf_det_record, 
    struct_record,
    psize_t_record,
    msghdr_record
};

// This class declares a system call argument.
// Currently we use a **string** to represent the argument.
class SystemCallArgument
{
    public:
        SystemCallArgument(sysarg_type_t syscallType) : type(syscallType) { }
        // Create the argument with a given register value
        void setArg(long argValue, SystemCallArgumentAuxilation *aux,
                sysarg_type_t syscallType = NULL);
        // Create the argument from a syscall arg record
        void setArg(String record, sysarg_type_t syscallType = NULL);
        // compare if two system call arguments are equal
        bool operator ==(SystemCallArgument &);
    private:
        // The name
        String name;
        // The type
        sysarg_type_t type;
        // The value
        String value;
};

#endif //__SYSCALL_SYSCALLARG_H__


