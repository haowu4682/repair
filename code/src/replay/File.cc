// Author: Hao Wu <haowu@cs.utexas.edu>

#include <replay/File.h>

using namespace std;

File::File(int fd, FileType type, String path)
{
    this->fd = fd;
    this->type = type;
    this->path = path;
}

File::File(int fd, String path)
{
    this->fd = fd;
    this->type = pathToType(path);
    this->path = path;
}

bool File::equals(const File &file) const
{
    return (type == file.type) &&
           (path == file.path);
}

ostream &operator <<(ostream &os, File *file)
{
    if (file != NULL)
    {
        os << "[" << file->fd << ", ";
        os << file->type << ", ";
        os << file->path << "]";
    }
    return os;
}

ostream &operator <<(ostream &os, File &file)
{
    os << &file;
    return os;
}

FileType File::pathToType(const String &path)
{
    //TODO: implement
    return real;
}

