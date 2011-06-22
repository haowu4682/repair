// Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __SYSCALL_SYSCALLARG_H__
#define __SYSCALL_SYSCALLARG_H__

#include <common/common.h>

class SystemCallArgument;

// XXX: Here we use C-style definition, since the author (haowu) don't know how to implement them
// in C++-style.
// The type for a sysarg record
typedef void (*sysarg_type_t) (long argValue, SystemCallArgument &syscallArg);
#define SYSARG_(type) void type##_record(long argValue, SystemCallArgument &arg)

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

// _recordhe class declares a system call argument
class SystemCallArgument
{
    private:
        // The name
        String name;
        // The sysarg type
        sysarg_type_t type;
};

#endif //__SYSCALL_SYSCALLARG_H__


