// Author: Hao Wu

#include <pthread.h>

#include <common/common.h>
#include <common/util.h>
#include <replay/Process.h>
#include <replay/ProcessManager.h>
using namespace std;

int Process::exec()
{
    LOG("Executing process %ld", subProcessList.size());
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
    for (Vector<Action *>::iterator it = preActions->begin(); it != preActions->end(); ++it)
    {
        (*it)->exec();
    }
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
        LOG("pthread_create fails when trying to replay %s, errno=%d", command->toString().c_str(), ret);
        return -1;
    }
    LOG("Executing %s", command->toString().c_str());
    // XXX: The join command must be moved to another place in the future.
    pthread_join(thread, NULL);
    return 0;
}

void Process::setCommand(SystemCall *syscall)
{
    command->pid = syscall->getPid();
    parseArgv(command->argv, syscall->getArg(1).getValue());
    LOG1(command->argv[0].c_str());
    setVirtual(false);
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

Process *Process::searchProcess(pid_t pid)
{
    Process *result = NULL;
    if (command->pid == pid)
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
    Command *comm = new Command;
    comm->pid = pid;
    Vector<Action *> *preActions = new Vector<Action *>();
    Process *proc = new Process(comm, true, this);
    proc->setSyscallList(syscallList);
    proc->setPidManager(pidManager);
    proc->setFDManager(fdManager);
    proc->preActions = preActions;
    subProcessList.push_back(proc);
    return proc;
}

bool Process::operator ==(pid_t pid) const
{
    return command->pid == pid;
}

bool Process::operator ==(const Process &process) const
{
    return command->pid == process.command->pid;
}

Process::~Process()
{
    /*
    delete command;
    delete preActions;
    for (Vector<Process *>::iterator it = subProcessList.begin(); it != subProcessList.end(); ++it)
    {
        delete *it;
    }
    */
}

