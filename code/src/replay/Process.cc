// Author: Hao Wu

#include <pthread.h>

#include <common/common.h>
#include <replay/Process.h>
#include <replay/ProcessManager.h>
using namespace std;

int Process::exec()
{
    pthread_t thread;
    int ret;

    LOG1(command->argv[0].c_str());
    ProcessManager manager(&command->argv, syscallList, pidManager);
    manager.setOldPid(command->pid);
    manager.getFDManager()->clone(fdManager);
    ret = pthread_create(&thread, NULL, replayProcess, &manager);

    // If pthread creation fails
    if (ret != 0)
    {
        LOG("pthread_create fails when trying to replay %s, errno=%d", command->argv[0].c_str(), ret);
        return -1;
    }
    return 0;
}

