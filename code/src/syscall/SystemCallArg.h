// Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __SYSCALL_SYSCALLARG_H__
#define __SYSCALL_SYSCALLARG_H__

// The enum shows the **type** of a certain syscall argument.
enum SystemCallArgumentType
{
    voidT,
    sintT,
    uintT,
    sint32T,
    uint32T,
    intpT,
    pid_tT,
    bufT,
    sha1T,
    stringT,
    stringsT,
    iovecT,
    fdT,
    fd2T,
    nameT, 
    pathT,
    path_atT,
    rpathT,
    rpath_atT,
    buf_detT, 
    structT,
    psize_tT,
    msghdrT
}

// The class declares a system call argument
class SystemCallArgument
{
    
};

#endif //__SYSCALL_SYSCALLARG_H__


