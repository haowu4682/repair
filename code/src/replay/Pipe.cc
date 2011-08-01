// Author: Hao Wu <haowu@cs.utexas.edu>

#include <unistd.h>

#include <replay/Pipe.h>

Pipe::Pipe(const int *fileDes, long seqNum, FDManager *fdManager)
{
    setFileDes(fileDes);
    setSeqNum(seqNum);
    setFDManager(fdManager);
}

int Pipe::exec()
{
    int newFileDes[2];
    newFileDes[0] = fdManager->oldToNew(fileDes[0], seqNum);
    newFileDes[1] = fdManager->oldToNew(fileDes[1], seqNum);
    pipe(newFileDes);
    return 0;
}

