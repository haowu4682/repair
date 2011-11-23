// Original Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __REPLAY_PROCESSMANAGER_H__
#define __REPLAY_PROCESSMANAGER_H__

#include <string>
#include <vector>

#include <common/common.h>
#include <replay/FDManager.h>
#include <replay/Process.h>
#include <syscall/SystemCall.h>

// This class is used to manager the running or a process of class Process
// @author haowu
class ProcessManager
{
    public:
        // The constructor
        ProcessManager(const Process &proc) : process(proc) {}
        ProcessManager(const Process *proc) : process(*proc) {}

        // Start replaying the process
        int replay();

        // Trace the process without running ``exec''
        // The function is used when the process to be replayed has already started.
        int trace(pid_t);

        // Output all the infomation as a string.
        // The function is used for debugging and logging.
        String toString();

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
        // @param pid
        static int writeMatchedSyscall(SystemCall &syscall, pid_t pid);
        // Skip executing a syscall
        // @param pid
        static int skipSyscall(pid_t pid);
        // Deal with fork (manage pid, add a new proc manager for it.
        int dealWithFork(SystemCall &syscall, pid_t oldPid);
        // The process to be replayed
        Process process;
};

// Replay a process. The function is used as an API for pthread
// @param manager the process manager used to replay the process
void *replayProcess(void *manager);

struct ManagedProcess
{
    ProcessManager *manager;
    pid_t pid;
};

// Trace a process. The function is used as an API for pthread.
// @param process the process to be traced.
void *traceProcess(void *process);

#endif //__REPLAY_PROCESSMANAGER_H__

