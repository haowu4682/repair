// Author: Hao Wu <haowu@cs.utexas.edu>

#include <sstream>

#include <replay/FDManager.h>
#include <replay/PidManager.h>
#include <replay/SystemManager.h>
#include <syscall/SystemCallList.h>
using namespace std;

SystemCall SystemCallList::search(SystemCall &syscall)
{
    SystemCall result;
    pid_t newPid = syscall.getPid();
    if (pidManager != NULL)
    {
        pid_t oldPid = pidManager->getOld(newPid);
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
    Process *root = systemManager->getRoot();
    Map<pid_t, Process *> processMap;
    Map<pid_t, bool> preActionRecordEnabled;
    root->getCommand()->pid = ROOT_PID;
    processMap[ROOT_PID] = root;

    while (!getline(in, syscallString).eof())
    {
        SystemCall syscall(syscallString, fdManager);
        pid_t oldPid = syscall.getPid();
        syscallMap[oldPid].syscalls.push_back(syscall);

        if (syscall.isExec())
        {
            // This is the updated version
            // XXX: If the exec is executed by a `exec'-ed process, we shall not add it to the list here
            if (!syscall.getUsage()) // && !pidManager->isForked(syscall.getPid()))
            {
                Map<pid_t, Process *>::iterator it = processMap.find(oldPid);
                if (it == processMap.end())
                {
                    Process *proc = root->addSubProcess(oldPid);
                    processMap[oldPid] = proc;
                    proc->setCommand(&syscall);
                }
                else
                {
                    it->second->setCommand(&syscall);
                }
                //systemManager->togglePreActionsOff(oldPid);
                preActionRecordEnabled[oldPid] = false;
            }
        }
        else if (syscall.isFork())
        {
            if (syscall.getUsage())
            {
                pid_t newPid = syscall.getReturn();
                Process *parent = root->searchProcess(oldPid);
                if (parent == NULL)
                {
                    parent = root;
                }
                Process *child = parent->addSubProcess(newPid);
                processMap[newPid] = child;

                if (newPid != 0)
                {
                    pidManager->addForked(newPid);
                }
                //systemManager->togglePreActionsOn(newPid);
                preActionRecordEnabled[newPid] = true;
            }
        }
        else if (syscall.isPipe())
        {
            // XXX: memory leak
            SystemCall *newSyscall = new SystemCall(syscall);
            Process *proc;
            Map<pid_t, Process *>::iterator it = processMap.find(oldPid);
            if (it == processMap.end())
            {
                proc = root;
            }
            else
            {
                proc = it->second;
            }
            proc->addPreAction(newSyscall);
            //systemManager->togglePreActionsOn(oldPid);
            //systemManager->recordPreAction(oldPid, newSyscall);
            //systemManager->togglePreActionsOff(oldPid);
        }
        else
        {
            // XXX: memory leak
            Map<pid_t, bool>::iterator jt = preActionRecordEnabled.find(oldPid);
            if (jt != preActionRecordEnabled.end() && jt->second)
            {
                SystemCall *newSyscall = new SystemCall(syscall);
                Process *proc;
                Map<pid_t, Process *>::iterator it = processMap.find(oldPid);
                if (it == processMap.end())
                {
                    proc = root;
                }
                else
                {
                    proc = it->second;
                }
                proc->addPreAction(newSyscall);
                //systemManager->recordPreAction(oldPid, newSyscall);
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

