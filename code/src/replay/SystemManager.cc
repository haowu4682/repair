// Author: Hao Wu <haowu@cs.utexas.edu>

#include <fstream>
#include <iostream>
#include <pthread.h>
#include <sstream>

#include <common/util.h>
#include <replay/Actor.h>
#include <replay/Process.h>
#include <replay/ProcessManager.h>
#include <replay/SystemManager.h>
using namespace std;

int SystemManager::execAll()
{
    Vector<Actor*>::iterator actor_pt;
    for (actor_pt = actors.begin(); actor_pt != actors.end(); ++actor_pt)
    {
        // Refrain from using fork here. Use pthread instead
        // XXX: Remove the following when pipes are introduced
        Process *actor = (Process *)(*actor_pt);
        (*actor_pt)->exec();
    }
    rootProcess->exec();
    return 0;
}

int SystemManager::addCommand(const SystemCall &syscall)
{
    // In x86_64, only `execve' can execute a actor. So the code is harded-coded for this
    // actor. It does not support other `exec' actors.
    // XXX Caution: Memory leak!
    Command *command = new Command();
    parseArgv(command->argv, syscall.getArg(1).getValue());
    command->pid = syscall.getPid();
    return addCommand(*command);
}

int SystemManager::addCommand(Command &command)
{
    // XXX Caution: Memory Leak! Must be optimized when the system grows.
    Process *process = new Process(&command);
    process->setFDManager(fdManager);
    process->setPidManager(pidManager);
    process->setSyscallList(syscallList);
    process->setPreActions(&preActionsMap[command.pid]);
    return addActor(process);
}

int SystemManager::addActor(Actor *actor)
{
    actors.push_back(actor);
    return 0;
}

void SystemManager::setRoot(Process *root)
{
    root->setFDManager(fdManager);
    root->setPidManager(pidManager);
    root->setSyscallList(syscallList);
    rootProcess = root;
}

void SystemManager::togglePreActionsOn(pid_t pid)
{
    preActionsEnabled[pid] = true;
}

void SystemManager::togglePreActionsOff(pid_t pid)
{
    preActionsEnabled[pid] = false;
}

int SystemManager::recordPreAction(pid_t pid, Action *action)
{
    PreActionsEnabledType::iterator it = preActionsEnabled.find(pid);
    if (it != preActionsEnabled.end())
    {
        bool enabled = it->second;
        if (enabled)
        {
            preActionsMap[pid].push_back(action);
            return 0;
        }
    }
    delete action;
    return 0;
}

String SystemManager::toString()
{
    ostringstream os;
    // TODO: implement
    return os.str();
}

int main(int argc, char **argv)
{
    ifstream fin("/home/haowu/repair_data/dumb.txt");
    PidManager pidManager;
    SystemManager sysManager;
    FDManager fdManager;
    Process rootProcess(true, NULL);
    SystemCallList list(&pidManager, &sysManager);

    sysManager.setSyscallList(&list);
    sysManager.setFDManager(&fdManager);
    sysManager.setPidManager(&pidManager);
    sysManager.setRoot(&rootProcess);

    list.init(fin, &fdManager);
    LOG("init finished");
    sysManager.execAll();
    LOG("execution finished");
    return 0;
}

