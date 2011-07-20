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
        Process() {}
        Process(Command *comm) { setCommand(comm); }
        virtual int exec();
        void setCommand(Command *command) { this->command = command; }
        Command *getCommand() { return command;}
    private:
        Command *command;
        FDManager *fdManager;
        PidManager *pidManager;
        SystemCallList *syscallList;
};

#endif //__REPLAY_PROCESS_H__

