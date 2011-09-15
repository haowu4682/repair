// Author: Hao Wu <haowu@cs.utexas.edu>

#include <replay/File.h>

File::File(int fd, FileType type, String path)
{
    this->fd = fd;
    this->type = type;
    this->path = path;
}

bool File::equals(const File &file) const
{
    return (type == file.type) &&
           (path == file.path);
}

