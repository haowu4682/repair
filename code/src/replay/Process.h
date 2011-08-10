// Author: Hao Wu

#ifndef __REPLAY_PROCESS_H__
#define __REPLAY_PROCESS_H__

#include <replay/Actor.h>
#include <replay/Command.h>
#include <replay/FDManager.h>
#include <replay/PidManager.h>
#include <syscall/SystemCallList.h>

class Process : public Actor
{
    public:
        Process(bool virt = false, Process *parent = NULL) : isVirtual(virt), parentProcess(parent) {}
        Process(Command *comm, bool virt = false, Process *parent = NULL) : isVirtual(virt),
            parentProcess(parent) { setCommand(comm); }
        ~Process();
        virtual int exec();
        void setCommand(SystemCall *syscall);
        void setCommand(Command *command) { this->command = command; }
        Command *getCommand() { return command;}
        void setFDManager(FDManager *fdManager) { this->fdManager = fdManager; }
        FDManager *getFDManager() { return fdManager; }
        void setPidManager(PidManager *pidManager) { this->pidManager = pidManager; }
        PidManager *getPidManager() { return pidManager; }
        void setSyscallList(SystemCallList *syscallList) { this->syscallList = syscallList; }
        SystemCallList *getSyscallList() { return syscallList; }
        void setPreActions(Vector<Action *> *preActions) { this->preActions = preActions; }
        Vector<Action *> *getPreActions() { return preActions; }
        void setVirtual(bool virt) { isVirtual = virt; }
        bool getVirtual() { return isVirtual; }

        //void addSubProcess(Process *process) { subProcessList.push_back(process); }
        Process *addSubProcess(pid_t pid);
        bool isChild(Process *process);
        bool isOffSpring(Process *process);
        bool isParent(Process *process);
        bool isAncestor(Process *process);
        Process *searchProcess(pid_t pid);

        bool operator == (const Process &process) const;
        bool operator == (pid_t pid) const;

    private:
        Command *command;
        FDManager *fdManager;
        PidManager *pidManager;
        SystemCallList *syscallList;
        Vector<Action *> *preActions;

        Process *parentProcess;
        Vector<Process *> subProcessList;
        bool isVirtual;
        int execReal();
        int execVirtual();
};

#endif //__REPLAY_PROCESS_H__

