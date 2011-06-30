// Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __REPLAY_FDMANAGER_H__
#define __REPLAY_FDMANAGER_H__

#include <map>

#include <common/common.h>

// The class is used to manage the relation between fd's and path's
class FDManager
{
    public:
        int addOld(int fd, String path);
        int removeOld(int fd);
        int addNew(int fd, String path);
        int removeNew(int fd);
        String searchOld(int fd);
        String searchNew(int fd);
        bool equals(int oldFd, int newFd);
        String toString();
    private:
        typedef std::pair<int, String> valueType;
        typedef std::map<int, String> mapType;
        mapType oldFDMap;
        mapType newFDMap;
};

#endif // __REPLAY_FDMANAGER_H__

