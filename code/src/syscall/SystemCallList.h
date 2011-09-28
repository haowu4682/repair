// Original Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __SYSCALL_SYSCALLLIST_H__
#define __SYSCALL_SYSCALLLIST_H__

#include <istream>

#include <common/common.h>
#include <syscall/SystemCall.h>

// a vector of system calls which belong to a single process
struct SystemCallListItem
{
    size_t currentPos;
    Vector<SystemCall> syscalls;
    SystemCallListItem() : currentPos(0) { }
};

// The class is used to represent the **record** of a system call list
//     as well as simple operations like matching.
class SystemCallList
{
    public:
        // The constructor, requires a pid manager
        SystemCallList(PidManager *pidManager, SystemManager *systemManager)
            { this->pidManager = pidManager; this->systemManager = systemManager;}

        // Search for a system call **same** or **similar** with the given system call.
        // @param syscall the given system call
        // @ret the same or similar system call. If no such a system call is found, an **invalid**
        //     system call will be returned.
        SystemCall search(SystemCall &syscall);

        // Search for a matching user input system call.
        // @param match the matched system call
        // @param source the original system call
        // @param pid the old pid of the system call
        // @param seq the sequence number of the first system call in the list
        //        to search. Default value is 0.
        // @ret the sequence number of the next system call in the list.
        //      -1 if no matched system call is found
        size_t searchMatchInput(SystemCall &match, const SystemCall &source,
                pid_t pid, size_t seq);

        // Init the system call list from an input stream.
        // @param in the input stream
        void init(std::istream &in, FDManager *fdManager = NULL);

        // to string
        String toString();
    private:
        typedef std::map<pid_t, SystemCallListItem> SyscallMapType;

        // A map from old pid to a syscall list
        SyscallMapType syscallMap;
        
        // A pid manager use to manage processes mapping
        PidManager *pidManager;

        // A system manager to store all the process to be directed `exec'-ed in replaying
        SystemManager *systemManager;
};

#endif //__SYSCALL_SYSCALLLIST_H__

