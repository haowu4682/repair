// Author: Hao Wu <haowu@cs.utexas.edu>

#include <cstdlib>
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
    rootProcess->exec();
    return 0;
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

void usage()
{
    char usageStr[] = "Usage: SystemReplay record1 [record2] [...] [recordN]";
    cerr << usageStr << endl;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        usage();
        exit(0);
    }
    PidManager pidManager;
    SystemManager sysManager;
    FDManager fdManager;
    // root process has pid ROOT_PID
    Process rootProcess(ROOT_PID);
    SystemCallList list(&pidManager, &sysManager, &fdManager);

    sysManager.setSyscallList(&list);
    sysManager.setFDManager(&fdManager);
    sysManager.setPidManager(&pidManager);
    sysManager.setRoot(&rootProcess);

    Vector<istream *> inList;
    for (int i = 1; i < argc; ++i)
    {
        inList.push_back(new ifstream(argv[i]));
    }
    list.init(inList);
    for (Vector<istream *>::iterator it = inList.begin(), e = inList.end();
            it != e; ++it)
    {
        delete *it;
    }
    LOG("init finished");
    sysManager.execAll();
    LOG("execution finished");
    return 0;
}

