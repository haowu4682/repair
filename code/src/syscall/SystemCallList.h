// Original Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __SYSCALL_SYSCALLLIST_H__
#define __SYSCALL_SYSCALLLIST_H__

#include <istream>

#include <common/common.h>
#include <replay/FDManager.h>
#include <replay/PidManager.h>
#include <syscall/SystemCall.h>

// The class is used to represent the **record** of a system call list
//     as well as simple operations like matching.
// @author haowu
class SystemCallList
{
    public:
        // The constructor, requires a pid manager
        SystemCallList(PidManager *pidManager) { this->pidManager = pidManager; }
        // Search for a system call **same** or **similar** with the given system call.
        // @param syscall the given system call
        // @ret the same or similar system call. If no such a system call is found, an **invalid**
        //     system call will be returned.
        SystemCall search(SystemCall &syscall);
        // Init the system call list from an input stream.
        // @param in the input stream
        void init(std::istream &in, FDManager *fdManager = NULL);
        // to string
        String toString();
    private:
        // A vector which stores all the system call list
        Vector<SystemCall> syscallVector;
        // A pid manager use to manage processes mapping
        PidManager *pidManager;
};

#endif //__SYSCALL_SYSCALLLIST_H__
