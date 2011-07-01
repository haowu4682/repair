// Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __REPLAY_PIDMANAGER_H__
#define __REPLAY_PIDMANAGER_H__

#include <map>

#include <common/common.h>

// The class is used to manage the relation between pid's
// TODO: add generation numbers
class PidManager
{
    public:
        int add(pid_t oldPid, pid_t newPid);
        bool equals(pid_t oldPid, pid_t newPid);
        pid_t getOld(pid_t newPid);
        pid_t getNew(pid_t oldPid);
        String toString();
    private:
        typedef std::pair<pid_t, pid_t> valueType;
        typedef std::map<pid_t, pid_t> mapType;
        mapType oldToNew;
        mapType newToOld;
};

#endif // __REPLAY_PIDMANAGER_H__

