// Author: Hao Wu <haowu@cs.utexas.edu>

#include <sstream>

#include <syscall/SystemCall.h>
#include <syscall/SystemCallList.h>
using namespace std;

SystemCall SystemCallList::search(SystemCall &syscall)
{
    SystemCall result;
    pid_t newPid = syscall.getPid();
    if (pidManager != NULL)
    {
        pid_t oldPid = pidManager->getOld(newPid);
        //LOG("%d", oldPid);
        SyscallMapType::iterator it = syscallMap.find(oldPid);
        if (it != syscallMap.end())
        {
            SystemCallListItem *list = &it->second;
            // TODO: use a more heuristic way to find a matched syscall
            size_t pos;
            for (pos = list->currentPos; pos < list->syscalls.size(); ++pos)
            {
                if (syscall == list->syscalls[pos])
                {
                    result = list->syscalls[pos];
                    list->currentPos = pos;
                    break;
                }
            }
        }
    }
    return result;
}

void SystemCallList::init(istream &in, FDManager *fdManager)
{
    // '\n' is used as a delimeter between syscalls
    string syscallString;
    SystemCall lastExecSyscall;
    while (!getline(in, syscallString).eof())
    {
        SystemCall syscall(syscallString, fdManager);
        pid_t oldPid = syscall.getPid();
        syscallMap[oldPid].syscalls.push_back(syscall);

        if (syscall.isExec())
        {
#if 0
            // The following code does not reflect the behavior of ``exec''
            if (syscall.getUsage())
            {
                systemManager->addCommand(lastExecSyscall);
            }
            else
            {
                lastExecSyscall = syscall;
            }
#endif
            // This is the updated version
            // If the exec is executed by a `fork'-ed process, we shall not add it to the list here.
            if (!syscall.getUsage() && !pidManager->isForked(syscall.getPid()))
            {
                systemManager->addCommand(syscall);
            }
        }
        else if (syscall.isFork())
        {
            if (syscall.getUsage())
            {
                pid_t newPid = syscall.getReturn();
                if (newPid != 0)
                {
                    pidManager->addForked(newPid);
                }
            }
        }
    }
}

String SystemCallList::toString()
{
    ostringstream ss;
    ss << "SystemCallList:" << endl;
    SyscallMapType::iterator it;
    Vector<SystemCall>::iterator jt;
    for (it = syscallMap.begin(); it != syscallMap.end(); ++it)
    {
        ss << "pid = " << it->first << ":" << endl;
        ss << "--------------------------------------" << endl;
        for (jt = it->second.syscalls.begin(); jt != it->second.syscalls.end(); ++jt)
        {
            ss << jt->toString() << endl;
        }
    }
    return ss.str();
}

