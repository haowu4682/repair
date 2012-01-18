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
        const static int MATCH_NOT_FOUND = -1;

        // The constructor, requires a pid manager
        SystemCallList(PidManager *pidManager, SystemManager *systemManager,
                FDManager *fdManager) : pidManager(pidManager), 
                                        systemManager(systemManager),
                                        fdManager(fdManager) {}

        // Search for a system call **same** or **similar** with the given system call.
        // @param syscall the given system call
        // @ret the same or similar system call. If no such a system call is found, an **invalid**
        //     system call will be returned.
        SystemCall search(SystemCall &syscall);

        // Search for a matching system call.
        // @param match the matched system call
        // @param source the original system call
        // @param pid the old pid of the system call
        // @param seq the sequence number of the first system call in the list
        //        to search. Default value is 0.
        // @ret the sequence number of the next system call in the list.
        //      MATCH_NOT_FOUND if no matched system call is found.
        long searchMatch(SystemCall &match, const SystemCall &source,
                pid_t pid, size_t seq = 0);

        // Search for a matching select system call. Parameters are similar with
        // searchMatchInput.
        //long searchMatchSelect(SystemCall &match, const SystemCall &source,
        //        pid_t pid, size_t seq = 0);

        // Init the system call list from an input stream.
        // @param inList the list of all the input streams
        void init(Vector<std::istream *> &inList);

        // to string
        String toString();

    private:
        typedef std::map<pid_t, SystemCallListItem> SyscallMapType;

        // A map from old pid to a syscall list
        SyscallMapType syscallMap;

        // A pid manager use to manage processes mapping
        PidManager *pidManager;

        // An FD Manager to manager file descriptors
        FDManager *fdManager;

        // A system manager to store all the process to be directed `exec'-ed in replaying
        SystemManager *systemManager;
};

#endif //__SYSCALL_SYSCALLLIST_H__

