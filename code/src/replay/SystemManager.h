// Author: Hao Wu<haowu@cs.utexas.edu>

#ifndef __REPLAY_SYSTEMMANAGER_H__
#define __REPLAY_SYSTEMMANAGER_H__

#include <common/common.h>
#include <syscall/SystemCall.h>
#include <syscall/SystemCallList.h>

// Not a good coding style here.
class SystemCallList;

// This class is used to manager a *system*, specifically, to manage different processes.
class SystemManager
{
    public:
        // Execute all the processes managed by the manager
        int execAll();
        // Set syscall list
        void setSyscallList(SystemCallList *syscallList) { this->syscallList = syscallList; }
        // add a command using command string
        int addCommand(const Vector<String> &command);
        // add a command using values from a syscall
        int addCommand(const SystemCall &syscall);
        // to string
        String toString();
    private:
        // All the commands in the system manager
        Vector<Vector<String> > commands;
        // The system call list;
        SystemCallList *syscallList;
};

#endif //__REPLAY_SYSTEMMANAGER_H__
