// Author: Hao Wu <haowu@cs.utexas.edu>

#include <climits>
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
        const SystemCall &source, pid_t pid, size_t seq,
        bool returnExit)
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

    for (size_t pos = seq, end = syscalls.size(); pos < end; ++pos)
    {
        if (source.match(syscalls[pos]))
        {
            if (returnExit)
            {
                if ((++pos) < syscalls.size())
                {
                    match = syscalls[pos];
                    if (match.getType() != source.getType())
                    {
                        LOG("Syscall record pair not matched.");
                        --pos;
                        continue;
                    }
                    return static_cast<int>(pos + 1);
                }
                else
                {
                    LOG("Syscall record pair broken due to eof");
                    break;
                }
            }
            else
            {
                match = syscalls[pos];
                return static_cast<int>(pos+1);
            }
        }
    }
    LOG("syscall not found due to EOF");

    return MATCH_NOT_FOUND;
}

// An aux function to calculate minimum timestamp
// and returns the index of the system call in the system call list.
// NULL is skipped
inline int minTs(Vector<SystemCall *> syscallList)
{
    long minValue = LONG_MAX;
    int minIndex = -1;

    for (int i = 0; i < syscallList.size(); ++i)
    {
        if (syscallList[i] == NULL)
            continue;
        long timestamp = syscallList[i]->getTimestamp();
        if (timestamp < minValue)
        {
            minValue = timestamp;
            minIndex = i;
        }
    }

    return minIndex;
}

void SystemCallList::init(Vector<istream *> &inList)
{
    // '\n' is used as a delimeter between syscalls
    Vector<SystemCall *> syscallFrontier(inList.size(), NULL);
    long current_ts = 0;
    int aliveFileCount = inList.size();
    Vector<bool> aliveFileList(inList.size(), true);
    String syscallString;

    int fileNum = 0;
    int stepCount = 0;

    while (aliveFileCount > 0)
    {
        // RoundRobin to find a next available file.
        if (++fileNum == inList.size())
        {
            fileNum = 0;
        }
        if (!aliveFileList[fileNum])
        {
            continue;
        }

        if (stepCount == aliveFileCount)
        {
            fileNum = minTs(syscallFrontier);
            assert(fileNum != -1);
            assert(syscallFrontier[fileNum] != NULL);
            LOG("StepCount reaches processor number. Timestamp is missing from %ld to %ld", current_ts,
                    syscallFrontier[fileNum]->getTimestamp());
            current_ts = syscallFrontier[fileNum]->getTimestamp();
        }

        istream &in = *inList[fileNum];

        if (syscallFrontier[fileNum] == NULL)
        {
            getline(in, syscallString);
            syscallFrontier[fileNum] = new SystemCall(syscallString, fdManager);
        }

        if (syscallFrontier[fileNum]->getTimestamp() == current_ts)
        {
            insertSyscall(*syscallFrontier[fileNum]);
            ++current_ts;
            stepCount = 0;

            getline(in, syscallString);
            syscallFrontier[fileNum] = new SystemCall(syscallString, fdManager);

            if (in.eof())
            {
                aliveFileList[fileNum] = false;
                --aliveFileCount;
                LOG("File num %d has been dead!", fileNum);
                continue;
            }
        }
        else
        {
            ++stepCount;
        }
    }
}

void SystemCallList::insertSyscall(SystemCall &syscall)
{
    string syscallString;
    SystemCall lastExecSyscall;
    Process *root = systemManager->getRoot();
    Map<pid_t, Process *> processMap;
    Map<pid_t, bool> preActionRecordEnabled;
    root->getCommand()->pid = ROOT_PID;
    processMap[ROOT_PID] = root;

    pid_t oldPid = syscall.getPid();
    Vector<SystemCall> &syscalls = syscallMap[oldPid].syscalls;
    if (syscalls.empty() || syscalls.back().getTimestamp() <= syscall.getTimestamp())
    {
        syscalls.push_back(syscall);
    }
    else
    {
        LOG("TIMESTAMP ERROR!");
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
                    goto out;
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

out:
    ;
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

