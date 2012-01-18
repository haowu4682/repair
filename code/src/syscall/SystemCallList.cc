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

long SystemCallList::searchMatch(SystemCall &match,
        const SystemCall &source, pid_t pid, size_t seq /* = 0 */)
{
    SyscallMapType::iterator it;
    if ((it = syscallMap.find(pid)) == syscallMap.end())
    {
        // No syscall list found
        LOG("No system call list found for %d", pid);
        return MATCH_NOT_FOUND;
    }

    SystemCallListItem &listItem = it->second;
    Vector<SystemCall> &syscalls = listItem.syscalls;

    //LOG("%s", source.toString().c_str());
    for (size_t pos = seq, end = syscalls.size(); pos < end; ++pos)
    {
        //LOG("%ld: %s", pos, syscalls[pos].toString().c_str());
        if (source.match(syscalls[pos]))
        {
            LOG("syscall found: %s", syscalls[pos].toString().c_str());
            if ((++pos) < syscalls.size())
            {
                match = syscalls[pos];
                if (match.getType() != source.getType())
                {
                    LOG("Syscall record pair not matched.");
                    --pos;
                    continue;
                }
                LOG("syscall match: %s", match.toString().c_str());
                return static_cast<int>(pos + 1);
            }
            else
            {
                LOG("Syscall record pair broken due to eof");
                break;
            }
        }
    }

    return MATCH_NOT_FOUND;
}

void SystemCallList::init(istream &in)
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
        Vector<SystemCall> &syscalls = syscallMap[oldPid].syscalls;
        if (syscalls.empty() || syscalls.back().getTimestamp() <= syscall.getTimestamp())
        {
            syscalls.push_back(syscall);
        }
        else
        {
            // TODO: change to binary search
            for (Vector<SystemCall>::iterator it = syscalls.begin(), e = syscalls.end();
                    it != e; ++it)
            {
                if (it->getTimestamp() > syscall.getTimestamp())
                {
                    syscalls.insert(it, syscall);
                    break;
                }
            }
        }

        if (syscall.isExec())
        {
            // This is the updated version
            // XXX: If the exec is executed by a `exec'-ed process, we shall not add it to the list here
            if (syscall.getUsage() & SYSARG_IFENTER) // && !pidManager->isForked(syscall.getPid()))
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
                preActionRecordEnabled[oldPid] = false;
            }
        }
        else if (syscall.isFork())
        {
            if (syscall.getUsage() & SYSARG_IFEXIT)
            {
                pid_t newPid = syscall.getReturn();
                Process *parent = root->searchProcess(oldPid);
                if (parent == NULL)
                {
                    parent = root;
                }
                // If there is already a parent other than this one, we should
                // remove that.
                Process *oldChild = root->searchProcess(newPid);
                if (oldChild != NULL)
                {
                    Process *oldParent = oldChild->getParent();
                    if (oldParent == parent)
                    {
                        continue;
                    }
                    oldParent->removeProcess(oldChild);
                }
                Process *child = parent->addSubProcess(newPid);
                processMap[newPid] = child;

                if (newPid != 0)
                {
                    pidManager->addForked(newPid);
                }
                preActionRecordEnabled[newPid] = true;
            }
        }
        else if (syscall.isPipe())
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
        }
        else
        {
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

