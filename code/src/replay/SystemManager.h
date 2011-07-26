// Author: Hao Wu<haowu@cs.utexas.edu>

#ifndef __REPLAY_SYSTEMMANAGER_H__
#define __REPLAY_SYSTEMMANAGER_H__

#include <pthread.h>

#include <common/common.h>
#include <replay/Actor.h>
#include <replay/Command.h>
#include <replay/FDManager.h>
#include <replay/ProcessManager.h>
#include <syscall/SystemCall.h>
#include <syscall/SystemCallList.h>

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
        int addCommand(Command &command);
        // add a command using values from a syscall
        int addCommand(const SystemCall &syscall);
        // add an actor
        int addActor(Actor *actor);
        // to string
        String toString();
        // set fd manager
        void setFDManager(FDManager *fdManager) { this->fdManager = fdManager; }
        // set pid manager
        void setPidManager(PidManager *pidManager) { this->pidManager = pidManager; }
        // get fd manager
        FDManager *getFDManager() { return fdManager; }
        // get pid manager
        PidManager *getPidManager() { return pidManager; }

        // Set on unset recording pre-actions
        void togglePreActionsOn(pid_t);
        void togglePreActionsOff(pid_t);
        // Record a pre-action if preActionsEnabled is enabled
        int recordPreAction(Action *action);

    private:
        // All the actors in the system manager
        Vector<Actor*> actors;
        // The system call list;
        SystemCallList *syscallList;
        // The original copy of fd manager
        FDManager *fdManager;
        // The original pid manager
        PidManager *pidManager;
        // All the Process Managers
        Vector<ProcessManager> processManagerList;
        // All the threads to be executed
        Vector<pthread_t> threads;
        // Enable recording pre-actions
        Vector<pid_t> preActionsEnabled;
        // Recorded pre-actions
        typedef Map<pid_t, Vector<Action *> > PreActionsRecordType;
        PreActionsRecordType preActionsMap;
};

#endif //__REPLAY_SYSTEMMANAGER_H__

