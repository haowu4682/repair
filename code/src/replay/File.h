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
    network,
    fifo,
    unknown,
    MAX_SIZE
};

class File
{
    public:
        File(int fd, String path);
        File(int fd, FileType type, String path);
        bool equals(const File &file) const;
        bool operator == (const File &file) const { return equals(file); }
        bool operator != (const File &file) const { return !equals(file); }

        int getFD() { return fd; }
        FileType getType() { return type; }
        String getPath() { return path; }

        bool isUserInput()
        {
            // XXX: Do we interact both device and network?
            LOG("type is %d", type);
            return ((type == device) || (type == network));
        }

        friend std::ostream& operator <<(std::ostream&, File&);
        friend std::ostream& operator <<(std::ostream&, File*);

        static FileType pathToType(const String &path);

        static File STDIN;
        static File STDOUT;
        static File STDERR;

    private:
        int fd;
        FileType type;
        String path;
};

#endif //__REPLAY_FILE_H__

