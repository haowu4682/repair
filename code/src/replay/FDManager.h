// Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __REPLAY_FDMANAGER_H__
#define __REPLAY_FDMANAGER_H__

#include <map>

#include <common/common.h>

// The class is used to manage the relation between fd's and path's
// XXX: There might be a SHALLOW-COPY problem in this class
class FDManager
{
    public:
        // XXX: ASSUME item with lower seq number will always be inserted in the first place
        //      here.
        int addOld(int fd, String path, long seqNum);
        int removeOld(int fd);
        int addNew(int fd, String path, long seqNum = -1);
        int removeNew(int fd);

        String searchOld(int fd, long seqNum);
        // Always return the most recent one
        String searchNew(int fd);
        // If we can find a fd mapping, return the mapped fd. Otherwise return the old fd.
        int oldToNew(int oldFd, long seqNum);
        // If we can find a fd mapping, return the mapped fd. Otherwise return the new fd.
        // XXX: temporarily invalid
        // int newToOld(int newFd, long seqNum);
        // The seqNum is used for old-fd, the new fd does not need a seqNum
        bool equals(int oldFd, int newFd, long seqNum);
        String toString();

        // Clone the FDMap from another FDManager
        void clone(FDManager *another);

    private:
        struct FDItem
        {
            long seqNum;
            String path;
            FDItem() {}
            FDItem(long seqNum, String path) : seqNum(seqNum), path(path) {}
        };

        typedef std::pair<int, Vector<FDItem> > valueType;
        typedef std::map<int, Vector<FDItem> > mapType;
        mapType oldFDMap;
        mapType newFDMap;
};

#endif // __REPLAY_FDMANAGER_H__

