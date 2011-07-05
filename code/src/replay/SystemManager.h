// Author: Hao Wu<haowu@cs.utexas.edu>

#ifndef __REPLAY_SYSTEMMANAGER_H__
#define __REPLAY_SYSTEMMANAGER_H__

#include <common/common.h>

// This class is used to manager a *system*, specifically, to manage different processes.
class SystemManager
{
    public:
        // Execute all the processes managed by the manager
        int execAll();
        // Set syscall list
        void setSyscallList(SystemCallList *syscallList) { this->syscallList = syscallList; }
        // add a command
        void addCommand(Vector<String>);
    private:
        // All the commands in the system manager
        Vector<Vector<String> > commands;
        // The system call list;
        SystemCallList *syscallList;
};

#endif //__REPLAY_SYSTEMMANAGER_H__
