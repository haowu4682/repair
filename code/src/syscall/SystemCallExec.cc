// Author: Hao Wu <haowu@cs.utexas.edu>
// This file contians how to execute a syscall.
// TODO: implement everything

#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include <common/util.h>
#include <syscall/SystemCall.h>
#include <syscall/SystemCallArg.h>
using namespace std;

SYSCALL_(read)
{
    return 0;
}

SYSCALL_(write)
{
    return 0;
}

SYSCALL_(open)
{
    String path = syscall->getArg(0).getValue();
    int flag = atoi(syscall->getArg(1).getValue().c_str());
    mode_t mode = atoi(syscall->getArg(2).getValue().c_str());
    int newFd = open(path.c_str(), flag, mode);
    LOG("path=%s, newFd=%d", path.c_str(), newFd);
    FDManager *fdManager = syscall->getFDManager();
    File *file = new File(newFd, path);
    fdManager->addNewFile(file);
    return 0;
}

SYSCALL_(close)
{
    int originalFd = atoi(syscall->getArg(0).getValue().c_str());
    FDManager *fdManager = syscall->getFDManager();
    long ts = syscall->getTimestamp();
    int currentFd = fdManager->oldToNew(originalFd, ts);
    close(currentFd);
    return 0;
}

SYSCALL_(poll)
{
    return 0;
}

SYSCALL_(mmap)
{
    return 0;
}

SYSCALL_(munmap)
{
    return 0;
}

SYSCALL_(ioctl)
{
    return 0;
}

SYSCALL_(pread)
{
    return 0;
}

SYSCALL_(pwrite)
{
    return 0;
}

SYSCALL_(readv)
{
    return 0;
}

SYSCALL_(writev)
{
    return 0;
}

SYSCALL_(access)
{
    String path = syscall->getArg(0).getValue();
    mode_t mode = atoi(syscall->getArg(1).getValue().c_str());
    access(path.c_str(), mode);
    return 0;
}

SYSCALL_(pipe)
{
    Pair<int, int> fd2 = fd2_derecord(syscall->getArg(0).getValue());
    int originalOldFd = fd2.first;
    int originalNewFd = fd2.second;
    long ts = syscall->getTimestamp();
    FDManager *fdManager = syscall->getFDManager();
    int fd[2];
    fd[0] = fdManager->oldToNew(originalOldFd, ts);
    fd[1] = fdManager->oldToNew(originalNewFd, ts);
    pipe(fd);
    return 0;
}

SYSCALL_(select)
{
    return 0;
}

SYSCALL_(dup)
{
    return 0;
}

SYSCALL_(dup2)
{
    int originalOldFd = atoi(syscall->getArg(0).getValue().c_str());
    int originalNewFd = atoi(syscall->getArg(1).getValue().c_str());
    long ts = syscall->getTimestamp();
    FDManager *fdManager = syscall->getFDManager();
    int currentOldFd = fdManager->oldToNew(originalOldFd, ts);
    int currentNewFd = fdManager->oldToNew(originalNewFd, ts);
    LOG("newFd=(%d, %d)", currentOldFd, currentNewFd);
    dup2(currentOldFd, currentNewFd);
    return 0;
}

SYSCALL_(socket)
{
    return 0;
}

SYSCALL_(connect)
{
    return 0;
}

SYSCALL_(accept)
{
    return 0;
}

SYSCALL_(sendto)
{
    return 0;
}

SYSCALL_(recvfrom)
{
    return 0;
}

SYSCALL_(sendmsg)
{
    return 0;
}

SYSCALL_(recvmsg)
{
    return 0;
}

SYSCALL_(shutdown)
{
    return 0;
}

SYSCALL_(bind)
{
    return 0;
}

SYSCALL_(listen)
{
    return 0;
}

SYSCALL_(getsockname)
{
    return 0;
}

SYSCALL_(getpeername)
{
    return 0;
}

SYSCALL_(socketpair)
{
    return 0;
}

SYSCALL_(setsockopt)
{
    return 0;
}

SYSCALL_(getsockopt)
{
    return 0;
}

SYSCALL_(clone)
{
    return 0;
}

SYSCALL_(fork)
{
    fork();
    return 0;
}

SYSCALL_(vfork)
{
    vfork();
    return 0;
}

SYSCALL_(execve)
{
    String path = syscall->getArg(0).getValue();
    Vector<String> argv;
    // TODO: envp
    int ret = parseArgv(argv, syscall->getArg(1).getValue());
    if (ret < 0)
    {
        LOG("system call information corrupted, execution aborted: %s.",
                syscall->toString().c_str());
        return ret;
    }

    char **args = new char *[argv.size()+1];
    for (int i = 0; i < argv.size(); i++)
    {
        // XXX: This is not very clean. But we have to copy `argv' into a new char array since
        // `char *' is required in execvp, but the type of `argv' is `const char *'.
        const char *arg = argv[i].c_str();
        args[i] = new char[strlen(arg)];
        strcpy(args[i], arg);
    }
    args[argv.size()] = NULL;

    execv(path.c_str(), args);
    return 0;
}

SYSCALL_(exit)
{
    return 0;
}

SYSCALL_(wait4)
{
    return 0;
}

SYSCALL_(fcntl)
{
    return 0;
}

SYSCALL_(truncate)
{
    return 0;
}

SYSCALL_(ftruncate)
{
    return 0;
}

SYSCALL_(chdir)
{
    return 0;
}

SYSCALL_(fchdir)
{
    return 0;
}

SYSCALL_(rename)
{
    return 0;
}

SYSCALL_(mkdir)
{
    return 0;
}

SYSCALL_(rmdir)
{
    return 0;
}

SYSCALL_(creat)
{
    return 0;
}

SYSCALL_(link)
{
    return 0;
}

SYSCALL_(unlink)
{
    return 0;
}

SYSCALL_(symlink)
{
    return 0;
}

SYSCALL_(readlink)
{
    return 0;
}

SYSCALL_(mknod)
{
    return 0;
}

SYSCALL_(chroot)
{
    return 0;
}

SYSCALL_(undo_depend)
{
    return 0;
}

SYSCALL_(snapshot)
{
    return 0;
}

SYSCALL_(undo_func_start)
{
    return 0;
}

SYSCALL_(undo_func_end)
{
    return 0;
}

SYSCALL_(undo_mask_start)
{
    return 0;
}

SYSCALL_(undo_mask_end)
{
    return 0;
}

SYSCALL_(exit_group)
{
    return 0;
}

SYSCALL_(waitid)
{
    return 0;
}

SYSCALL_(openat)
{
    return 0;
}

SYSCALL_(mkdirat)
{
    return 0;
}

SYSCALL_(mknodat)
{
    return 0;
}

SYSCALL_(unlinkat)
{
    return 0;
}

SYSCALL_(renameat)
{
    return 0;
}

SYSCALL_(linkat)
{
    return 0;
}

SYSCALL_(symlinkat)
{
    return 0;
}

SYSCALL_(readlinkat)
{
    return 0;
}

SYSCALL_(faccessat)
{
    return 0;
}

SYSCALL_(dup3)
{
    return 0;
}

SYSCALL_(pipe2)
{
    return 0;
}

