// Author: Hao Wu <haowu@cs.utexas.edu>

#include <sys/stat.h>

#include <replay/File.h>

using namespace std;

File File::STDIN(0, device, "/dev/stdin");
File File::STDOUT(1, device, "/dev/stdout");
File File::STDERR(2, device, "/dev/stderr");

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
    return path == file.path;
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
    struct stat statBuf;
    int ret;
    mode_t mode;

    if ((ret = stat(path.c_str(), &statBuf)) < 0)
    {
        return unknown;
    }
    mode = statBuf.st_mode;

    if (S_ISBLK(mode) || S_ISCHR(mode))
    {
        return device;
    }
    else if (S_ISSOCK(mode))
    {
        return network;
    }
    else
    {
        return real;
    }
    return real;
}

