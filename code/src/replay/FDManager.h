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
        int removeOld(int fd, String path);
        int addNew(int fd, String path);
        int removeNew(int fd, String path);
        String searchOld(int fd);
        String searchNew(int fd);
        bool equals(int fd1, int fd2);
    private:
        std::map<int, String> oldFDMap;
        std::map<int, String> newFDMap;
};

#endif // __REPLAY_FDMANAGER_H__
