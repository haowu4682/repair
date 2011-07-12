// Author: Hao Wu <haowu@cs.utexas.edu>

#include <sstream>

#include <replay/PidManager.h>
using namespace std;

int PidManager::add(int oldPid, int newPid)
{
    oldToNew[oldPid] = newPid;
    newToOld[newPid] = oldPid;
    return 0;
}

bool PidManager::equals(int oldPid, int newPid)
{
    // This is equivalent to newToOld[newPid] == oldPid
    return newPid == oldToNew[oldPid];
}

pid_t PidManager::getNew(pid_t oldPid)
{
    pid_t res = 0;
    mapType::iterator it = oldToNew.find(oldPid);
    if (it != oldToNew.end())
    {
        res = it->second;
    }
    return res;
}

pid_t PidManager::getOld(pid_t newPid)
{
    pid_t res = 0;
    LOG("%d", newToOld.size())
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
    for (mapType::iterator it = oldToNew.begin(); it != oldToNew.end(); ++it)
    {
        sout << it->first << "\t" << it->second << endl;
    }
    return sout.str();
}

