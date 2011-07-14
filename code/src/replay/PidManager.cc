// Author: Hao Wu <haowu@cs.utexas.edu>

#include <sstream>

#include <replay/PidManager.h>
using namespace std;

int PidManager::addForked(pid_t pid)
{
    forkedPid.insert(pid);
    return 0;
}

bool PidManager::isForked(pid_t pid)
{
    return forkedPid.find(pid) != forkedPid.end();
}

int PidManager::add(pid_t oldPid, pid_t newPid)
{
    oldToNew[oldPid] = newPid;
    newToOld[newPid] = oldPid;
    return 0;
}

bool PidManager::equals(pid_t oldPid, pid_t newPid)
{
    // This is equivalent to newToOld[newPid] == oldPid
    return newPid == oldToNew[oldPid];
}

pid_t PidManager::getNew(pid_t oldPid)
{
    pid_t res = -1;
    mapType::iterator it = oldToNew.find(oldPid);
    if (it != oldToNew.end())
    {
        res = it->second;
    }
    return res;
}

pid_t PidManager::getOld(pid_t newPid)
{
    pid_t res = -1;
    //LOG("%lu %d", newToOld.size(), newPid)
    mapType::iterator it = newToOld.find(newPid);
    if (it != newToOld.end())
    {
        res = it->second;
    }
    return res;
}

String PidManager::toString()
{
    ostringstream sout;
    sout << "Pid Mapping:" << endl;
    sout << "OldToNew:" << endl;
    for (mapType::iterator it = oldToNew.begin(); it != oldToNew.end(); ++it)
    {
        sout << it->first << "\t" << it->second << endl;
    }

    sout << "NewToOld:" << endl;
    for (mapType::iterator it = newToOld.begin(); it != newToOld.end(); ++it)
    {
        sout << it->first << "\t" << it->second << endl;
    }
    sout << "ForkedPid:" << endl;
    for (setType::iterator it = forkedPid.begin(); it != forkedPid.end(); ++it)
    {
        sout << *it << endl;
    }
    return sout.str();
}

