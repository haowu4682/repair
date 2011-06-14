
#ifndef _SYSCALL_HOOKS_H_
#define _SYSCALL_HOOKS_H_

//
// FIXME delete_module
// 
#define HOOKS( FN )                  \
    do {                             \
        FN( unlinkat              ); \
        FN( wait4                 ); \
        FN( close                 ); \
        FN( open                  ); \
        FN( read                  ); \
        FN( write                 ); \
        FN( writev                ); \
        FN( ftruncate             ); \
        FN( mmap                  ); \
        FN( creat                 ); \
        FN( truncate              ); \
        FN( rename                ); \
        FN( openat                ); \
        FN( exit                  ); \
        FN( exit_group            ); \
                                     \
        FN##_S( execve            ); \
        FN##_S( clone             ); \
        FN##_S( fork              ); \
        FN##_S( vfork             ); \
                                     \
        FN(accept4                ); \
        FN(access                 ); \
        FN(acct                   ); \
        FN(add_key                ); \
        FN(adjtimex               ); \
        FN(alarm                  ); \
        FN(bind                   ); \
        FN(brk                    ); \
        FN(capget                 ); \
        FN(capset                 ); \
        FN(chdir                  ); \
        FN(chmod                  ); \
        FN(chown                  ); \
        FN(chroot                 ); \
        FN(clock_getres           ); \
        FN(clock_gettime          ); \
        FN(clock_nanosleep        ); \
        FN(clock_settime          ); \
        FN(connect                ); \
        FN(dup                    ); \
        FN(dup2                   ); \
        FN(dup3                   ); \
        FN(epoll_create           ); \
        FN(epoll_create1          ); \
        FN(epoll_ctl              ); \
        FN(epoll_pwait            ); \
        FN(epoll_wait             ); \
        FN(eventfd                ); \
        FN(eventfd2               ); \
        FN(faccessat              ); \
        FN(fadvise64              ); \
        FN(fallocate              ); \
        FN(fchdir                 ); \
        FN(fchmod                 ); \
        FN(fchmodat               ); \
        FN(fchown                 ); \
        FN(fchownat               ); \
        FN(fcntl                  ); \
        FN(fdatasync              ); \
        FN(fgetxattr              ); \
        FN(flistxattr             ); \
        FN(flock                  ); \
        FN(fremovexattr           ); \
        FN(fsetxattr              ); \
        FN(fstat                  ); \
        FN(fstatfs                ); \
        FN(fsync                  ); \
        FN(futimesat              ); \
        FN(futex                  ); \
        FN(getcwd                 ); \
        FN(getdents               ); \
        FN(getdents64             ); \
        FN(getegid                ); \
        FN(geteuid                ); \
        FN(getgid                 ); \
        FN(getgroups              ); \
        FN(getitimer              ); \
        FN(get_mempolicy          ); \
        FN(getpeername            ); \
        FN(getpgid                ); \
        FN(getpgrp                ); \
        FN(getpid                 ); \
        FN(getppid                ); \
        FN(getpriority            ); \
        FN(getresgid              ); \
        FN(getresuid              ); \
        FN(getrlimit              ); \
        FN(get_robust_list        ); \
        FN(getrusage              ); \
        FN(getsid                 ); \
        FN(getsockname            ); \
        FN(getsockopt             ); \
        FN(gettid                 ); \
        FN(gettimeofday           ); \
        FN(getuid                 ); \
        FN(getxattr               ); \
        FN(init_module            ); \
        FN(inotify_add_watch      ); \
        FN(inotify_init           ); \
        FN(inotify_init1          ); \
        FN(inotify_rm_watch       ); \
        FN(io_cancel              ); \
        FN(ioctl                  ); \
        FN(io_destroy             ); \
        FN(io_getevents           ); \
        FN(ioprio_get             ); \
        FN(ioprio_set             ); \
        FN(io_setup               ); \
        FN(io_submit              ); \
        FN(kexec_load             ); \
        FN(keyctl                 ); \
        FN(kill                   ); \
        FN(lchown                 ); \
        FN(lgetxattr              ); \
        FN(link                   ); \
        FN(linkat                 ); \
        FN(listen                 ); \
        FN(listxattr              ); \
        FN(llistxattr             ); \
        FN(lookup_dcookie         ); \
        FN(lremovexattr           ); \
        FN(lseek                  ); \
        FN(lsetxattr              ); \
        FN(lstat                  ); \
        FN(madvise                ); \
        FN(mbind                  ); \
        FN(migrate_pages          ); \
        FN(mincore                ); \
        FN(mkdir                  ); \
        FN(mkdirat                ); \
        FN(mknod                  ); \
        FN(mknodat                ); \
        FN(mlock                  ); \
        FN(mlockall               ); \
        FN(mount                  ); \
        FN(move_pages             ); \
        FN(mprotect               ); \
        FN(mq_getsetattr          ); \
        FN(mq_notify              ); \
        FN(mq_open                ); \
        FN(mq_timedreceive        ); \
        FN(mq_timedsend           ); \
        FN(mq_unlink              ); \
        FN(mremap                 ); \
        FN(msgctl                 ); \
        FN(msgget                 ); \
        FN(msgrcv                 ); \
        FN(msgsnd                 ); \
        FN(msync                  ); \
        FN(munlock                ); \
        FN(munlockall             ); \
        FN(munmap                 ); \
        FN(nanosleep              ); \
        FN(newfstatat             ); \
        FN(nfsservctl             ); \
        FN(pause                  ); \
        FN(personality            ); \
        FN(pipe                   ); \
        FN(pipe2                  ); \
        FN(pivot_root             ); \
        FN(poll                   ); \
        FN(ppoll                  ); \
        FN(prctl                  ); \
        FN(pread64                ); \
        FN(preadv                 ); \
        FN(pselect6               ); \
        FN(ptrace                 ); \
        FN(pwrite64               ); \
        FN(pwritev                ); \
        FN(quotactl               ); \
        FN(readahead              ); \
        FN(readlink               ); \
        FN(readlinkat             ); \
        FN(readv                  ); \
        FN(reboot                 ); \
        FN(recvfrom               ); \
        FN(recvmsg                ); \
        FN(remap_file_pages       ); \
        FN(removexattr            ); \
        FN(renameat               ); \
        FN(request_key            ); \
        FN(restart_syscall        ); \
        FN(rmdir                  ); \
        FN(rt_sigaction           ); \
        FN(rt_sigpending          ); \
        FN(rt_sigprocmask         ); \
        FN(rt_sigqueueinfo        ); \
        FN(rt_sigsuspend          ); \
        FN(rt_sigtimedwait        ); \
        FN(rt_tgsigqueueinfo      ); \
        FN(sched_getaffinity      ); \
        FN(sched_getparam         ); \
        FN(sched_get_priority_max ); \
        FN(sched_get_priority_min ); \
        FN(sched_getscheduler     ); \
        FN(sched_rr_get_interval  ); \
        FN(sched_setaffinity      ); \
        FN(sched_setparam         ); \
        FN(sched_setscheduler     ); \
        FN(sched_yield            ); \
        FN(select                 ); \
        FN(semctl                 ); \
        FN(semget                 ); \
        FN(semop                  ); \
        FN(semtimedop             ); \
        FN(sendfile               ); \
        FN(sendmsg                ); \
        FN(sendto                 ); \
        FN(setdomainname          ); \
        FN(setfsgid               ); \
        FN(setfsuid               ); \
        FN(setgid                 ); \
        FN(setgroups              ); \
        FN(sethostname            ); \
        FN(setitimer              ); \
        FN(set_mempolicy          ); \
        FN(setpgid                ); \
        FN(setpriority            ); \
        FN(setregid               ); \
        FN(setresgid              ); \
        FN(setresuid              ); \
        FN(setreuid               ); \
        FN(setrlimit              ); \
        FN(set_robust_list        ); \
        FN(setsid                 ); \
        FN(setsockopt             ); \
        FN(set_tid_address        ); \
        FN(settimeofday           ); \
        FN(setuid                 ); \
        FN(setxattr               ); \
        FN(shmat                  ); \
        FN(shmctl                 ); \
        FN(shmdt                  ); \
        FN(shmget                 ); \
        FN(shutdown               ); \
        FN(signalfd               ); \
        FN(signalfd4              ); \
        FN(socket                 ); \
        FN(socketpair             ); \
        FN(splice                 ); \
        FN(stat                   ); \
        FN(statfs                 ); \
        FN(swapoff                ); \
        FN(swapon                 ); \
        FN(symlink                ); \
        FN(symlinkat              ); \
        FN(sync                   ); \
        FN(sync_file_range        ); \
        FN(sysfs                  ); \
        FN(sysinfo                ); \
        FN(syslog                 ); \
        FN(tee                    ); \
        FN(tgkill                 ); \
        FN(time                   ); \
        FN(timer_create           ); \
        FN(timer_delete           ); \
        FN(timerfd_create         ); \
        FN(timerfd_gettime        ); \
        FN(timerfd_settime        ); \
        FN(timer_getoverrun       ); \
        FN(timer_gettime          ); \
        FN(timer_settime          ); \
        FN(times                  ); \
        FN(tkill                  ); \
        FN(umask                  ); \
        FN(unlink                 ); \
        FN(unshare                ); \
        FN(uselib                 ); \
        FN(ustat                  ); \
        FN(utime                  ); \
        FN(utimensat              ); \
        FN(utimes                 ); \
        FN(vhangup                ); \
        FN(vmsplice               ); \
        FN(waitid                 ); \
    } while ( 0 )

#endif /* _SYSCALL_HOOKS_H_ */