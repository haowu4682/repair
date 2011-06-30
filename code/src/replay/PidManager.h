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
        int addPair(int oldPid, int newPid);
        bool equals(int oldPid, int newPid);
        String toString();
    private:
        typedef std::pair<int, int> valueType;
        typedef std::map<int, int> mapType;
        mapType pidMap;
};

#endif // __REPLAY_PIDMANAGER_H__

