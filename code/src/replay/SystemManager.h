// Author: Hao Wu<haowu@cs.utexas.edu>

#ifndef __REPLAY_SYSTEMMANAGER_H__
#define __REPLAY_SYSTEMMANAGER_H__

#include <pthread.h>

#include <common/common.h>
#include <replay/FDManager.h>
#include <syscall/SystemCallList.h>
#include <replay/ProcessManager.h>
#include <syscall/SystemCall.h>

// Not a good coding style here.
class SystemCallList;
class ProcessManager;

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
        // set fd manager
        void setFDManager(FDManager *fdManager) { this->fdManager = fdManager; }
        // get fd manager
        FDManager *getFDManager() { return fdManager; }
    private:
        // All the commands in the system manager
        Vector<Vector<String> > commands;
        // The system call list;
        SystemCallList *syscallList;
        // The original copy of fd manager
        FDManager *fdManager;
        // All the Process Managers
        Vector<ProcessManager> processManagerList;
        // All the threads to be executed
        Vector<pthread_t> threads;
};

#endif //__REPLAY_SYSTEMMANAGER_H__

