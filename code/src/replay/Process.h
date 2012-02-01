// Author: Hao Wu

#ifndef __REPLAY_PROCESS_H__
#define __REPLAY_PROCESS_H__

#include <replay/Actor.h>
#include <replay/FDManager.h>
#include <replay/PidManager.h>
#include <syscall/SystemCallList.h>

class Process : public Actor
{
    public:
        Process(pid_t pid, Process *parent = NULL) : pid(pid), parentProcess(parent) {}
        virtual int exec();
        pid_t getPid() { return pid; }
        void setPid(pid_t pid) { this->pid = pid; }
        void setFDManager(FDManager *fdManager) { this->fdManager = fdManager; }
        FDManager *getFDManager() { return fdManager; }
        void setPidManager(PidManager *pidManager) { this->pidManager = pidManager; }
        PidManager *getPidManager() { return pidManager; }
        void setSyscallList(SystemCallList *syscallList) { this->syscallList = syscallList; }
        SystemCallList *getSyscallList() { return syscallList; }

        Process *addSubProcess(pid_t pid);
        bool isChild(Process *process);
        bool isOffSpring(Process *process);
        bool isParent(Process *process);
        bool isAncestor(Process *process);
        Process *searchProcess(pid_t pid);
        void removeProcess(Process *);
        Process *getParent() { return parentProcess; }

        bool operator == (const Process &process) const;
        bool operator == (pid_t pid) const;

    private:
        pid_t pid;
        FDManager *fdManager;
        PidManager *pidManager;
        SystemCallList *syscallList;

        Process *parentProcess;
        Vector<Process *> subProcessList;

        int execRoot();
};

#endif //__REPLAY_PROCESS_H__

