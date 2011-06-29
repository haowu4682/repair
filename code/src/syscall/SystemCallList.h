// Original Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __SYSCALL_SYSCALLLIST_H__
#define __SYSCALL_SYSCALLLIST_H__

#include <istream>

#include <common/common.h>
#include <syscall/SystemCall.h>

// The class is used to represent the **record** of a system call list
//     as well as simple operations like matching.
// @author haowu
class SystemCallList
{
    public:
        // Search for a system call **same** or **similar** with the given system call.
        // @param syscall the given system call
        // @ret the same or similar system call. If no such a system call is found, an **invalid**
        //     system call will be returned.
        SystemCall search(SystemCall &syscall);
        // Init the system call list from an input stream.
        // @param in the input stream
        void init(std::istream &in);
        // to string
        String toString();
    private:
        // A vector which stores all the system call list
        Vector<SystemCall> syscallVector;
};

#endif //__SYSCALL_SYSCALLLIST_H__
