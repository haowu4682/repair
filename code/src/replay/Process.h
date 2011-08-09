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
        Process(bool virt = false) : isVirtual(virt) {}
        Process(Command *comm, bool virt = false) : isVirtual(virt) { setCommand(comm); }
        virtual int exec();
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

        void addSubProcess(Process *process) { subProcessList.push_back(process); }
        bool isChild(Process *process);
        bool isOffSpring(Process *process);
        bool isParent(Process *process);
        bool isAncestor(Process *process);

        bool operator == (const Process &process) const;

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

