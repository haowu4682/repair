// Original Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __REPLAY_PROCESSMANAGER_H__
#define __REPLAY_PROCESSMANAGER_H__

#include <string>
#include <vector>

#include <common/common.h>
#include <replay/FDManager.h>
#include <syscall/SystemCall.h>
#include <syscall/SystemCallList.h>

// This class is used to manager the running or a process of class Process
// @author haowu
class ProcessManager
{
    public:
        // The constructor
        // @author haowu
        ProcessManager(Vector<String> *command, SystemCallList *list);
        ProcessManager(SystemCallList *list);

        // Start replaying the process
        // @author haowu
        int replay();

        // Trace the process without running ``exec''
        // The function is used when the process to be replayed has already started.
        int trace(pid_t);

        // Output all the infomation as a string.
        // The function is used for debugging and logging.
        // @author haowu
        String toString();

        // Get the fd manager
        FDManager *getFDManager() { return &fdManager; }
        // Get the pid manager
        PidManager *getPidManager() { return &pidManager; }
    private:
        // Start to execute and trace a process with Ptrace
        // @author haowu
        int startProcess();
        // Execute a process with Ptrace
        // @author haowu
        int executeProcess();
        // Trace a process with Ptrace
        // @author haowu
        // @param pid the pid of the process to be traced
        int traceProcess(pid_t pid);
        // Overwrite the return value of a system call
        // @author haowu
        // @param syscall The syscall to overwrite
        static int writeMatchedSyscall(SystemCall &syscall, pid_t pid);
        // Deal with fork (manage pid, add a new proc manager for it.
        int dealWithFork(SystemCall &syscall, pid_t oldPid);
        // The command line
        Vector<String> *commandList;
        // The system call list used when replaying
        SystemCallList *syscallList;
        // The fd manager
        FDManager fdManager;
        // The pid manager
        PidManager pidManager;
};

#endif //__REPLAY_PROCESSMANAGER_H__

