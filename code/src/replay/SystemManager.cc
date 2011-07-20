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
        (*actor_pt)->exec();
        /*
        pthread_t thread;
        int ret;

        LOG1(actor_pt->argv[0].c_str());
        ProcessManager manager(&actor_pt->argv, syscallList, pidManager);
        manager.setOldPid(actor_pt->pid);
        processManagerList.push_back(manager);
        manager.getFDManager()->clone(fdManager);
        ret = pthread_create(&thread, NULL, replayProcess, &processManagerList.back());
        // If pthread creation fails
        if (ret != 0)
        {
            LOG("pthread_create fails when trying to replay %s, errno=%d", actor_pt->argv[0].c_str(), ret);
            return -1;
        }
        threads.push_back(thread);
        */
    }
    /*
    for (Vector<pthread_t>::iterator it = threads.begin(); it != threads.end(); ++it)
        pthread_join(*it, NULL);
        */
    return 0;
}

int SystemManager::addCommand(const SystemCall &syscall)
{
    // In x86_64, only `execve' can execute a actor. So the code is harded-coded for this
    // actor. It does not support other `exec' actors.
    Command actor;
    parseArgv(actor.argv, syscall.getArg(1).getValue());
    actor.pid = syscall.getPid();
    LOG1(syscall.toString().c_str());
    return addCommand(actor);
}

int SystemManager::addCommand(Command &actor)
{
    Process process(&actor);
    return addActor(process);
}

int SystemManager::addActor(Actor &actor)
{
    actors.push_back(&actor);
    return 0;
}

String SystemManager::toString()
{
    ostringstream os;
    // TODO: implement
    //LOG("%ld", actors.size());
    /*
    Vector<Actor>::iterator actor_pt;
    for (actor_pt = actors.begin(); actor_pt < actors.end(); ++actor_pt)
    {
        for (Vector<String>::iterator argv_pt = actor_pt->argv.begin(); argv_pt !=
                actor_pt->argv.end(); ++argv_pt)
        {
            os << *argv_pt << ", ";
        }
        os << actor_pt->pid << endl;
    }*/
    return os.str();
}

int main(int argc, char **argv)
{
    ifstream fin("/home/haowu/repair_data/dumb.txt");
    PidManager pidManager;
    SystemManager sysManager;
    FDManager fdManager;
    SystemCallList list(&pidManager, &sysManager);
    sysManager.setSyscallList(&list);
    sysManager.setFDManager(&fdManager);
    sysManager.setPidManager(&pidManager);
    list.init(fin, &fdManager);
    LOG("init finished");
    LOG1(sysManager.toString().c_str());
    sysManager.execAll();
    return 0;
}

