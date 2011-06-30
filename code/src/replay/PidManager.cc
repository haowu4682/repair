// Author: Hao Wu <haowu@cs.utexas.edu>

#include <sstream>

#include <replay/PidManager.h>
using namespace std;

int PidManager::add(int oldPid, int newPid)
{
    pidMap.insert(valueType(oldPid, newPid));
    return 0;
}

bool PidManager::equals(int oldPid, int newPid)
{
    return newPid == pidMap[oldPid];
}

String PidManager::toString()
{
    ostringstream sout;
    sout << "Pid Mapping:" << endl;
    for (mapType::iterator it = pidMap.begin(); it != pidMap.end(); ++it)
    {
        sout << it->first << "\t" << it->second << endl;
    }
    return sout.str();
}

