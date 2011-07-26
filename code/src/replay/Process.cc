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

    LOG("%s %ld", command->argv[0].c_str(), command->argv.size());
    ProcessManager manager(&command->argv, syscallList, pidManager);
    manager.setPreActions(preActions);
    manager.setOldPid(command->pid);
    manager.getFDManager()->clone(fdManager);
    ret = pthread_create(&thread, NULL, replayProcess, &manager);

    // If pthread creation fails
    if (ret != 0)
    {
        LOG("pthread_create fails when trying to replay %s, errno=%d", command->argv[0].c_str(), ret);
        return -1;
    }
    // XXX: The join command must be moved to another place in the future.
    pthread_join(thread, NULL);
    return 0;
}

