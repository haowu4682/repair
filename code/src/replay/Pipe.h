// Author: Hao Wu

#ifndef __REPLAY_PIPE_H__
#define __REPLAY_PIPE_H__

#include <replay/Actor.h>
#include <replay/FDManager.h>

class Pipe : public Actor
{
    public:
        virtual int exec();
        Pipe(const int *fileDes, long seqNum, FDManager *fdManager);
        const int *getFileDes() const { return fileDes; }
        void setFileDes(const int *fileDes) { this->fileDes[0] = fileDes[0]; this->fileDes[1] = fileDes[1]; }
        FDManager *getFDManager() { return fdManager; }
        void setFDManager(FDManager *manager) { fdManager = manager; }
        long getSeqNum() { return seqNum; }
        void setSeqNum(long seqNum) { this->seqNum = seqNum; }
    private:
        int fileDes[2];
        FDManager *fdManager;
        long seqNum;
};

#endif //__REPLAY_PIPE_H__

