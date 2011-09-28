// Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __REPLAY_FDMANAGER_H__
#define __REPLAY_FDMANAGER_H__

#include <map>

#include <common/common.h>
#include <replay/File.h>

// The class is used to manage the relation between fd's and path's
// XXX: There might be a SHALLOW-COPY problem in this class
class FDManager
{
    public:
        FDManager() { init(); }
        // XXX: ASSUME item with lower seq number will always be inserted in the first place
        //      here.
        int addOldFile(File *file, long seqNum);
        //int removeOldFile(int fd);
        //int removeOldFile(File *file);
        int addNewFile(File *file, long seqNum = -1);
        //int removeNewFile(int fd);
        //int removeNewFile(File *file);

        File *searchOld(int fd, long seqNum);
        // Always return the most recent one
        File *searchNew(int fd);
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
        // Shall not use these things currently.
        FDManager(const FDManager &fdManager);
        FDManager& operator=(const FDManager &fdManger);

        /** The function initializes the fd manager with standard fds, namely
         *  stdin, stdout, stderr.
         */
        void init();

        struct FDItem
        {
            long seqNum;
            File *file;
            FDItem() {}
            FDItem(long seqNum, File *file) : seqNum(seqNum), file(file) {}
        };

        typedef std::pair<int, Vector<FDItem> > valueType;
        typedef std::map<int, Vector<FDItem> > mapType;
        mapType oldFDMap;
        mapType newFDMap;

        static File standardFiles[];
        static unsigned standardFilesSize;
};

#endif // __REPLAY_FDMANAGER_H__

