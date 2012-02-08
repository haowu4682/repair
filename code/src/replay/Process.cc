// Author: Hao Wu

#include <algorithm>
#include <pthread.h>

#include <common/common.h>
#include <common/util.h>
#include <replay/Process.h>
#include <replay/ProcessManager.h>
using namespace std;

int Process::execRoot()
{
    int ret;
    for (Vector<Process *>::iterator it = subProcessList.begin(); it != subProcessList.end(); ++it)
    {
        ret = (*it)->exec();
        if (ret < 0)
            return ret;
    }
}

int Process::exec()
{
    pthread_t thread;
    int ret;

    // If it is root, use execRoot instead.
    if (pid == ROOT_PID)
    {
        return execRoot();
    }

    // XXX: LEAK
    ProcessManager *manager = new ProcessManager(*this);
    ret = pthread_create(&thread, NULL, replayProcess, manager);

    // If pthread creation fails
    if (ret != 0)
    {
        LOG("pthread_create fails when trying to replay %d, errno=%d", pid, ret);
        return -1;
    }
    // TODO: Move the join function elsewhere to create parallelism
    pthread_join(thread, NULL);
    return 0;
}

bool Process::isParent(Process *process)
{
    if (process == NULL)
        return parentProcess == NULL;
    else if (parentProcess == NULL)
        return false;
    else
        return *parentProcess == *process;
}

bool Process::isChild(Process *process)
{
    if (process == NULL)
    {
        return false;
    }
    for (Vector<Process *>::iterator it = subProcessList.begin(); it != subProcessList.end(); ++it)
    {
        if ((**it) == (*process))
        {
            return true;
        }
    }
    return false;
}

bool Process::isAncestor(Process *process)
{
    // NULL is the ancestor of any process
    if (process == NULL)
        return true;
    else if (parentProcess == NULL)
        return false;
    else if (*process == *this)
        return true;
    else
        return parentProcess->isAncestor(process);
}

bool Process::isOffSpring(Process *process)
{
    if (process == NULL)
    {
        return false;
    }
    if (*this == *process)
    {
        return true;
    }
    for (Vector<Process *>::iterator it = subProcessList.begin(); it != subProcessList.end(); ++it)
    {
        if ((*it)->isOffSpring(process))
        {
            return true;
        }
    }
    return false;
}

Process *Process::getNextChild()
{
    if (childCount >= subProcessList.size())
        return NULL;
    return subProcessList[childCount++];
}

Process *Process::searchProcess(pid_t pid)
{
    Process *result = NULL;

    if (this->pid == pid)
    {
        result = this;
    }
    else
    {
        for (Vector<Process *>::iterator it = subProcessList.begin(); it != subProcessList.end(); ++it)
        {
            result = (*it)->searchProcess(pid);
            if (result != NULL)
            {
                break;
            }
        }
    }
    return result;
}

Process *Process::addSubProcess(pid_t pid)
{
    Process *proc = new Process(pid, this);
    proc->setSyscallList(syscallList);
    proc->setPidManager(pidManager);
    proc->setFDManager(fdManager);
    subProcessList.push_back(proc);
    return proc;
}

bool Process::operator ==(pid_t pid) const
{
    return this->pid == pid;
}

bool Process::operator ==(const Process &process) const
{
    return this->pid == process.pid;
}

void Process::removeProcess(Process *proc)
{
    Vector<Process *>::iterator toDeleteIt = find(subProcessList.begin(),
            subProcessList.end(), proc);
    if (toDeleteIt != subProcessList.end())
    {
        subProcessList.erase(toDeleteIt);
    }
}

