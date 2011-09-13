// Author: Hao Wu <haowu@cs.utexas.edu>
// The class, File, is used to describe a file in the system. The file can actually be a real file
// on disk, a I/O device, or even a network connection. Anything represents by a Linux fd can
// be expressed by a file in our system.

#ifndef __REPLAY_FILE_H__
#define __REPLAY_FILE_H__

#include <common/common.h>

enum FileType
{
    real,
    device,
    network
};

class File
{
    public:
    private:
        int oldFd;
        int newFd;
        FileType type;
        String path;
};

#endif //__REPLAY_FILE_H__
