// Author: Hao Wu <haowu@cs.utexas.edu>

#include <fstream>
#include <iostream>
#include <pthread.h>
#include <sstream>

#include <common/util.h>
#include <replay/ProcessManager.h>
#include <replay/SystemManager.h>
using namespace std;

int SystemManager::execAll()
{
    Vector<Vector<String> >::iterator command_pt;
    for (command_pt = commands.begin(); command_pt < commands.end(); ++command_pt)
    {
        // TODO: Refrain from using fork here. Use pthread instead
        pthread_t thread;
        int ret;

        LOG1(command_pt[0][0].c_str());
        ProcessManager manager(&(*command_pt), syscallList);
        ret = pthread_create(&thread, NULL, replayProcess, &manager);
        // If the fork fails
        if (ret != 0)
        {
            LOG("pthread_create fails when trying to replay %s, errno=%d", (*command_pt)[0].c_str(), ret);
            return -1;
        }
    }
    return 0;
}

int SystemManager::addCommand(const SystemCall &syscall)
{
    // XXX: in x86_64, only `execve' can execute a command. So the code is harded-coded for this
    //   command. It does not support other `exec' commands.
    Vector<String> command;
    parseArgv(command, syscall.getArg(1).getValue());
    addCommand(command);
}

int SystemManager::addCommand(const Vector<String> &command)
{
    LOG1(command[0].c_str());
}

String SystemManager::toString()
{
    ostringstream os;
    Vector<Vector<String> >::iterator command_pt;
    for (command_pt = commands.begin(); command_pt < commands.end(); ++command_pt)
    {
        for (Vector<String>::iterator argv_pt = command_pt->begin(); argv_pt != command_pt->end();
                ++argv_pt)
        {
            os << *argv_pt << ' ';
        }
        os << endl;
    }
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
    list.init(fin, &fdManager);
    cout <<sysManager.toString();
    sysManager.execAll();
    return 0;
}

