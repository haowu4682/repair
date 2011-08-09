// Author: Hao Wu

#include <pthread.h>

#include <common/common.h>
#include <replay/Process.h>
#include <replay/ProcessManager.h>
using namespace std;

int Process::exec()
{
    if (isVirtual)
    {
        execVirtual();
    }
    else
    {
        execReal();
    }
}

int Process::execVirtual()
{
    int ret;
    for (Vector<Process *>::iterator it = subProcessList.begin(); it != subProcessList.end(); ++it)
    {
        ret = (*it)->exec();
        if (ret < 0)
            return ret;
    }
    return 0;
}

int Process::execReal()
{
    pthread_t thread;
    int ret;

    ProcessManager manager(*this);
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

bool Process::isParent(Process *process)
{
}

bool Process::isChild(Process *process)
{
}

bool Process::isAncestor(Process *process)
{
}

bool Process::isOffSpring(Process *process)
{
}

bool Process::operator ==(const Process &process) const
{
}

