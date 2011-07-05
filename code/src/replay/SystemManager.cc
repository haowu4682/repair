// Author: Hao Wu <haowu@cs.utexas.edu>

#include <common/util.h>
#include <replay/ProcessManager.h>
#include <replay/SystemManager.h>
using namespace std;

int SystemManager::execAll()
{
    Vector<Vector<String> >::iterator command_pt;
    for (command_pt = commands.begin(); command_pt < commands.end(); ++command_pt)
    {
        // Fork a process for the process
        pid_t pid = fork();
        // If the fork fails
        if (pid == -1)
        {
            LOG("fork fails when trying to run %s", (*command_pt)[0].c_str());
            return -1;
        }

        // If it is the child process
        if (pid == 0)
        {
            ProcessManager manager(&(*command_pt), syscallList);
            manager.replay();
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
    commands.push_back(command);
}

