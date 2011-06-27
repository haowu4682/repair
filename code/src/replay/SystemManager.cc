// Author: Hao Wu <haowu@cs.utexas.edu>

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
            ProcessManager manager(*command_pt, syscallList);
            manager.replay();
        }
    }
    return 0;
}

