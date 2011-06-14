#ifndef _SYSCALL_PROTO_H_
#define _SYSCALL_PROTO_H_

DEF_HOOK_FN_4(accept4, int, fd, struct sockaddr __user *, upeer_sockaddr,
		int __user *, upeer_addrlen, int, flags);
DEF_HOOK_FN_2(access, const char __user *, filename, int, mode);
DEF_HOOK_FN_1(acct, const char __user *, name);
DEF_HOOK_FN_5(add_key, const char __user *, _type,
		const char __user *, _description,
		const void __user *, _payload,
		size_t, plen,
		key_serial_t, ringid);
DEF_HOOK_FN_1(adjtimex, struct timex __user *, txc_p);
DEF_HOOK_FN_1(alarm, unsigned int, seconds);
DEF_HOOK_FN_3(bind, int, fd, struct sockaddr __user *, umyaddr, int, addrlen);
DEF_HOOK_FN_1(brk, unsigned long, brk);
DEF_HOOK_FN_2(capget, cap_user_header_t, header, cap_user_data_t, dataptr);
DEF_HOOK_FN_2(capset, cap_user_header_t, header, cap_user_data_t, data);
DEF_HOOK_FN_1(chdir, const char __user *, filename);
DEF_HOOK_FN_2(chmod, const char __user *, filename, mode_t, mode);
DEF_HOOK_FN_3(chown, const char __user *, filename, uid_t, user, gid_t, group);
DEF_HOOK_FN_1(chroot, const char __user *, filename);
DEF_HOOK_FN_2(clock_getres, clockid_t, which_clock,
		struct timespec __user *, tp);
DEF_HOOK_FN_2(clock_gettime, clockid_t, which_clock,
		struct timespec __user *,tp);
DEF_HOOK_FN_4(clock_nanosleep, clockid_t, which_clock, int, flags,
		const struct timespec __user *, rqtp,
		struct timespec __user *, rmtp);
DEF_HOOK_FN_2(clock_settime, clockid_t, which_clock,
		const struct timespec __user *, tp);
DEF_HOOK_FN_1(close, unsigned int, fd);
DEF_HOOK_FN_3(connect, int, fd, struct sockaddr __user *, uservaddr,
		int, addrlen);
DEF_HOOK_FN_2(creat, const char __user *, pathname, int, mode);
DEF_HOOK_FN_2(delete_module, const char __user *, name_user,
		unsigned int, flags);
DEF_HOOK_FN_1(dup, unsigned int, fildes);
DEF_HOOK_FN_2(dup2, unsigned int, oldfd, unsigned int, newfd);
DEF_HOOK_FN_3(dup3, unsigned int, oldfd, unsigned int, newfd, int, flags);
DEF_HOOK_FN_1(epoll_create, int, size);
DEF_HOOK_FN_1(epoll_create1, int, flags);
DEF_HOOK_FN_4(epoll_ctl, int, epfd, int, op, int, fd,
		struct epoll_event __user *, event);
DEF_HOOK_FN_6(epoll_pwait, int, epfd, struct epoll_event __user *, events,
		int, maxevents, int, timeout, const sigset_t __user *, sigmask,
		size_t, sigsetsize);
DEF_HOOK_FN_4(epoll_wait, int, epfd, struct epoll_event __user *, events,
		int, maxevents, int, timeout);
DEF_HOOK_FN_1(eventfd, unsigned int, count);
DEF_HOOK_FN_2(eventfd2, unsigned int, count, int, flags);
DEF_HOOK_FN_1(exit, int, error_code);
DEF_HOOK_FN_3(faccessat, int, dfd, const char __user *, filename, int, mode);
DEF_HOOK_FN_4(fadvise64, int, fd, loff_t, offset, size_t, len, int, advice);
DEF_HOOK_FN_4(fallocate, int, fd, int, mode, loff_t, offset, loff_t, len);
DEF_HOOK_FN_1(fchdir, unsigned int, fd);
DEF_HOOK_FN_2(fchmod, unsigned int, fd, mode_t, mode);
DEF_HOOK_FN_3(fchmodat, int, dfd, const char __user *, filename, mode_t, mode);
DEF_HOOK_FN_3(fchown, unsigned int, fd, uid_t, user, gid_t, group);
DEF_HOOK_FN_5(fchownat, int, dfd, const char __user *, filename, uid_t, user,
		gid_t, group, int, flag);
DEF_HOOK_FN_3(fcntl, unsigned int, fd, unsigned int, cmd, unsigned long, arg);
DEF_HOOK_FN_1(fdatasync, unsigned int, fd);
DEF_HOOK_FN_4(fgetxattr, int, fd, const char __user *, name,
		void __user *, value, size_t, size);
DEF_HOOK_FN_3(flistxattr, int, fd, char __user *, list, size_t, size);
DEF_HOOK_FN_2(flock, unsigned int, fd, unsigned int, cmd);
DEF_HOOK_FN_2(fremovexattr, int, fd, const char __user *, name);
DEF_HOOK_FN_5(fsetxattr, int, fd, const char __user *, name,
		const void __user *,value, size_t, size, int, flags);
DEF_HOOK_FN_2(fstat, unsigned int, fd, struct __old_kernel_stat __user *, statbuf);
DEF_HOOK_FN_2(fstatfs, unsigned int, fd, struct statfs __user *, buf);
DEF_HOOK_FN_1(fsync, unsigned int, fd);
DEF_HOOK_FN_2(ftruncate, unsigned int, fd, unsigned long, length);
DEF_HOOK_FN_6(futex, u32 __user *, uaddr, int, op, u32, val,
		struct timespec __user *, utime, u32 __user *, uaddr2,
		u32, val3);
DEF_HOOK_FN_3(futimesat, int, dfd, char __user *, filename,
		struct timeval __user *, utimes);
DEF_HOOK_FN_2(getcwd, char __user *, buf, unsigned long, size);
DEF_HOOK_FN_3(getdents, unsigned int, fd,
		struct linux_dirent __user *, dirent, unsigned int, count);
DEF_HOOK_FN_3(getdents64, unsigned int, fd,
		struct linux_dirent64 __user *, dirent, unsigned int, count);
DEF_HOOK_FN_0(getegid);
DEF_HOOK_FN_0(geteuid);
DEF_HOOK_FN_0(getgid);
DEF_HOOK_FN_2(getgroups, int, gidsetsize, gid_t __user *, grouplist);
DEF_HOOK_FN_2(getitimer, int, which, struct itimerval __user *, value);
DEF_HOOK_FN_5(get_mempolicy, int __user *, policy,
		unsigned long __user *, nmask, unsigned long, maxnode,
		unsigned long, addr, unsigned long, flags);
DEF_HOOK_FN_3(getpeername, int, fd, struct sockaddr __user *, usockaddr,
		int __user *, usockaddr_len);
DEF_HOOK_FN_1(getpgid, pid_t, pid);
DEF_HOOK_FN_0(getpgrp);
DEF_HOOK_FN_0(getpid);
DEF_HOOK_FN_0(getppid);
DEF_HOOK_FN_2(getpriority, int, which, int, who);
DEF_HOOK_FN_3(getresgid, gid_t __user *, rgid, gid_t __user *, egid, gid_t __user *, sgid);
DEF_HOOK_FN_3(getresuid, uid_t __user *, ruid, uid_t __user *, euid, uid_t __user *, suid);
DEF_HOOK_FN_2(getrlimit, unsigned int, resource, struct rlimit __user *, rlim);
DEF_HOOK_FN_3(get_robust_list, int, pid,
		struct robust_list_head __user * __user *, head_ptr,
		size_t __user *, len_ptr);
DEF_HOOK_FN_2(getrusage, int, who, struct rusage __user *, ru);
DEF_HOOK_FN_1(getsid, pid_t, pid);
DEF_HOOK_FN_3(getsockname, int, fd, struct sockaddr __user *, usockaddr,
		int __user *, usockaddr_len);
DEF_HOOK_FN_5(getsockopt, int, fd, int, level, int, optname,
		char __user *, optval, int __user *, optlen);
DEF_HOOK_FN_0(gettid);
DEF_HOOK_FN_2(gettimeofday, struct timeval __user *, tv,
		struct timezone __user *, tz);
DEF_HOOK_FN_0(getuid);
DEF_HOOK_FN_4(getxattr, const char __user *, pathname,
		const char __user *, name, void __user *, value, size_t, size);
DEF_HOOK_FN_3(init_module, void __user *, umod,
		unsigned long, len, const char __user *, uargs);
DEF_HOOK_FN_3(inotify_add_watch, int, fd, const char __user *, pathname,
		u32, mask);
DEF_HOOK_FN_0(inotify_init);
DEF_HOOK_FN_1(inotify_init1, int, flags);
DEF_HOOK_FN_2(inotify_rm_watch, int, fd, __s32, wd);
DEF_HOOK_FN_3(io_cancel, aio_context_t, ctx_id, struct iocb __user *, iocb,
		struct io_event __user *, result);
DEF_HOOK_FN_3(ioctl, unsigned int, fd, unsigned int, cmd, unsigned long, arg);
DEF_HOOK_FN_1(io_destroy, aio_context_t, ctx);
DEF_HOOK_FN_5(io_getevents, aio_context_t, ctx_id,
		long, min_nr,
		long, nr,
		struct io_event __user *, events,
		struct timespec __user *, timeout);
DEF_HOOK_FN_2(ioprio_get, int, which, int, who);
DEF_HOOK_FN_3(ioprio_set, int, which, int, who, int, ioprio);
DEF_HOOK_FN_2(io_setup, unsigned, nr_events, aio_context_t __user *, ctxp);
DEF_HOOK_FN_3(io_submit, aio_context_t, ctx_id, long, nr,
		struct iocb __user * __user *, iocbpp);
DEF_HOOK_FN_4(kexec_load, unsigned long, entry, unsigned long, nr_segments,
		struct kexec_segment __user *, segments, unsigned long, flags);
DEF_HOOK_FN_5(keyctl, int, option, unsigned long, arg2, unsigned long, arg3,
		unsigned long, arg4, unsigned long, arg5);
DEF_HOOK_FN_2(kill, pid_t, pid, int, sig);
DEF_HOOK_FN_3(lchown, const char __user *, filename, uid_t, user, gid_t, group);
DEF_HOOK_FN_4(lgetxattr, const char __user *, pathname,
		const char __user *, name, void __user *, value, size_t, size);
DEF_HOOK_FN_2(link, const char __user *, oldname, const char __user *, newname);
DEF_HOOK_FN_5(linkat, int, olddfd, const char __user *, oldname,
		int, newdfd, const char __user *, newname, int, flags);
DEF_HOOK_FN_2(listen, int, fd, int, backlog);
DEF_HOOK_FN_3(listxattr, const char __user *, pathname, char __user *, list,
		size_t, size);
DEF_HOOK_FN_3(llistxattr, const char __user *, pathname, char __user *, list,
		size_t, size);
DEF_HOOK_FN_0(lookup_dcookie);
DEF_HOOK_FN_2(lremovexattr, const char __user *, pathname,
		const char __user *, name);
DEF_HOOK_FN_3(lseek, unsigned int, fd, off_t, offset, unsigned int, origin);
DEF_HOOK_FN_5(lsetxattr, const char __user *, pathname,
		const char __user *, name, const void __user *, value,
		size_t, size, int, flags);
DEF_HOOK_FN_2(lstat, char __user *, filename, struct __old_kernel_stat __user *, statbuf);
DEF_HOOK_FN_3(madvise, unsigned long, start, size_t, len_in, int, behavior);
DEF_HOOK_FN_6(mbind, unsigned long, start, unsigned long, len,
		unsigned long, mode, unsigned long __user *, nmask,
		unsigned long, maxnode, unsigned, flags);
DEF_HOOK_FN_4(migrate_pages, pid_t, pid, unsigned long, maxnode,
		const unsigned long __user *, old_nodes,
		const unsigned long __user *, new_nodes);
DEF_HOOK_FN_3(mincore, unsigned long, start, size_t, len,
		unsigned char __user *, vec);
DEF_HOOK_FN_2(mkdir, const char __user *, pathname, int, mode);
DEF_HOOK_FN_3(mkdirat, int, dfd, const char __user *, pathname, int, mode);
DEF_HOOK_FN_3(mknod, const char __user *, filename, int, mode, unsigned, dev);
DEF_HOOK_FN_4(mknodat, int, dfd, const char __user *, filename, int, mode,
		unsigned, dev);
DEF_HOOK_FN_2(mlock, unsigned long, start, size_t, len);
DEF_HOOK_FN_1(mlockall, int, flags);
DEF_HOOK_FN_5(mount, char __user *, dev_name, char __user *, dir_name,
		char __user *, type, unsigned long, flags, void __user *, data);
DEF_HOOK_FN_6(move_pages, pid_t, pid, unsigned long, nr_pages,
		const void __user * __user *, pages,
		const int __user *, nodes,
		int __user *, status, int, flags);
DEF_HOOK_FN_3(mprotect, unsigned long, start, size_t, len,
		unsigned long, prot);
DEF_HOOK_FN_3(mq_getsetattr, mqd_t, mqdes,
		const struct mq_attr __user *, u_mqstat,
		struct mq_attr __user *, u_omqstat);
DEF_HOOK_FN_2(mq_notify, mqd_t, mqdes,
		const struct sigevent __user *, u_notification);
DEF_HOOK_FN_4(mq_open, const char __user *, u_name, int, oflag, mode_t, mode,
		struct mq_attr __user *, u_attr);
DEF_HOOK_FN_5(mq_timedreceive, mqd_t, mqdes, char __user *, u_msg_ptr,
		size_t, msg_len, unsigned int __user *, u_msg_prio,
		const struct timespec __user *, u_abs_timeout);
DEF_HOOK_FN_5(mq_timedsend, mqd_t, mqdes, const char __user *, u_msg_ptr,
		size_t, msg_len, unsigned int, msg_prio,
		const struct timespec __user *, u_abs_timeout);
DEF_HOOK_FN_1(mq_unlink, const char __user *, u_name);
DEF_HOOK_FN_5(mremap, unsigned long, addr, unsigned long, old_len,
		unsigned long, new_len, unsigned long, flags,
		unsigned long, new_addr);
DEF_HOOK_FN_3(msgctl, int, msqid, int, cmd, struct msqid_ds __user *, buf);
DEF_HOOK_FN_2(msgget, key_t, key, int, msgflg);
DEF_HOOK_FN_5(msgrcv, int, msqid, struct msgbuf __user *, msgp, size_t, msgsz,
		long, msgtyp, int, msgflg);
DEF_HOOK_FN_4(msgsnd, int, msqid, struct msgbuf __user *, msgp, size_t, msgsz,
		int, msgflg);
DEF_HOOK_FN_3(msync, unsigned long, start, size_t, len, int, flags);
DEF_HOOK_FN_2(munlock, unsigned long, start, size_t, len);
DEF_HOOK_FN_0(munlockall);
DEF_HOOK_FN_2(munmap, unsigned long, addr, size_t, len);
DEF_HOOK_FN_2(nanosleep, struct timespec __user *, rqtp,
		struct timespec __user *, rmtp);
DEF_HOOK_FN_4(newfstatat, int, dfd, char __user *, filename,
		struct stat __user *, statbuf, int, flag);
DEF_HOOK_FN_3(nfsservctl, int, cmd, struct nfsctl_arg __user *, arg,
		void __user *, res);
DEF_HOOK_FN_3(open, const char __user *, filename, int, flags, int, mode);
DEF_HOOK_FN_4(openat, int, dfd, const char __user *, filename, int, flags,
		int, mode);
DEF_HOOK_FN_0(pause);
DEF_HOOK_FN_1(personality, u_long, personality);
DEF_HOOK_FN_1(pipe, int __user *, fildes);
DEF_HOOK_FN_2(pipe2, int __user *, fildes, int, flags);
DEF_HOOK_FN_2(pivot_root, const char __user *, new_root,
		const char __user *, put_old);
DEF_HOOK_FN_3(poll, struct pollfd __user *, ufds, unsigned int, nfds,
		long, timeout_msecs);
DEF_HOOK_FN_5(ppoll, struct pollfd __user *, ufds, unsigned int, nfds,
		struct timespec __user *, tsp, const sigset_t __user *, sigmask,
		size_t, sigsetsize);
DEF_HOOK_FN_5(prctl, int, option, unsigned long, arg2, unsigned long, arg3,
		unsigned long, arg4, unsigned long, arg5);
DEF_HOOK_FN_0(pread64);
DEF_HOOK_FN_5(preadv, unsigned long, fd, const struct iovec __user *, vec,
		unsigned long, vlen, unsigned long, pos_l, unsigned long, pos_h);
DEF_HOOK_FN_6(pselect6, int, n, fd_set __user *, inp, fd_set __user *, outp,
		fd_set __user *, exp, struct timespec __user *, tsp,
		void __user *, sig);
DEF_HOOK_FN_4(ptrace, long, request, long, pid, long, addr, long, data);
DEF_HOOK_FN_0(pwrite64);
DEF_HOOK_FN_5(pwritev, unsigned long, fd, const struct iovec __user *, vec,
		unsigned long, vlen, unsigned long, pos_l, unsigned long, pos_h);
DEF_HOOK_FN_4(quotactl, unsigned int, cmd, const char __user *, special,
		qid_t, id, void __user *, addr);
DEF_HOOK_FN_3(read, unsigned int, fd, char __user *, buf, size_t, count);
DEF_HOOK_FN_0(readahead);
DEF_HOOK_FN_3(readlink, const char __user *, path, char __user *, buf,
		int, bufsiz);
DEF_HOOK_FN_4(readlinkat, int, dfd, const char __user *, pathname,
		char __user *, buf, int, bufsiz);
DEF_HOOK_FN_3(readv, unsigned long, fd, const struct iovec __user *, vec,
		unsigned long, vlen);
DEF_HOOK_FN_4(reboot, int, magic1, int, magic2, unsigned int, cmd,
		void __user *, arg);
DEF_HOOK_FN_6(recvfrom, int, fd, void __user *, ubuf, size_t, size,
		unsigned, flags, struct sockaddr __user *, addr,
		int __user *, addr_len);
DEF_HOOK_FN_3(recvmsg, int, fd, struct msghdr __user *, msg,
		unsigned int, flags);
DEF_HOOK_FN_5(remap_file_pages, unsigned long, start, unsigned long, size,
		unsigned long, prot, unsigned long, pgoff, unsigned long, flags);
DEF_HOOK_FN_2(removexattr, const char __user *, pathname,
		const char __user *, name);
DEF_HOOK_FN_2(rename, const char __user *, oldname, const char __user *, newname);
DEF_HOOK_FN_4(renameat, int, olddfd, const char __user *, oldname,
		int, newdfd, const char __user *, newname);
DEF_HOOK_FN_4(request_key, const char __user *, _type,
		const char __user *, _description,
		const char __user *, _callout_info,
		key_serial_t, destringid);
DEF_HOOK_FN_0(restart_syscall);
DEF_HOOK_FN_1(rmdir, const char __user *, pathname);
DEF_HOOK_FN_4(rt_sigaction, int, sig,
		const struct sigaction __user *, act,
		struct sigaction __user *, oact,
		size_t, sigsetsize);
DEF_HOOK_FN_2(rt_sigpending, sigset_t __user *, set, size_t, sigsetsize);
DEF_HOOK_FN_4(rt_sigprocmask, int, how, sigset_t __user *, set,
		sigset_t __user *, oset, size_t, sigsetsize);
DEF_HOOK_FN_3(rt_sigqueueinfo, pid_t, pid, int, sig,
		siginfo_t __user *, uinfo);
DEF_HOOK_FN_2(rt_sigsuspend, sigset_t __user *, unewset, size_t, sigsetsize);
DEF_HOOK_FN_4(rt_sigtimedwait, const sigset_t __user *, uthese,
		siginfo_t __user *, uinfo, const struct timespec __user *, uts,
		size_t, sigsetsize);
DEF_HOOK_FN_4(rt_tgsigqueueinfo, pid_t, tgid, pid_t, pid, int, sig,
		siginfo_t __user *, uinfo);
DEF_HOOK_FN_3(sched_getaffinity, pid_t, pid, unsigned int, len,
		unsigned long __user *, user_mask_ptr);
DEF_HOOK_FN_2(sched_getparam, pid_t, pid, struct sched_param __user *, param);
DEF_HOOK_FN_1(sched_get_priority_max, int, policy);
DEF_HOOK_FN_1(sched_get_priority_min, int, policy);
DEF_HOOK_FN_1(sched_getscheduler, pid_t, pid);
DEF_HOOK_FN_2(sched_rr_get_interval, pid_t, pid,
		struct timespec __user *, interval);
DEF_HOOK_FN_3(sched_setaffinity, pid_t, pid, unsigned int, len,
		unsigned long __user *, user_mask_ptr);
DEF_HOOK_FN_2(sched_setparam, pid_t, pid, struct sched_param __user *, param);
DEF_HOOK_FN_3(sched_setscheduler, pid_t, pid, int, policy,
		struct sched_param __user *, param);
DEF_HOOK_FN_0(sched_yield);
DEF_HOOK_FN_5(select, int, n, fd_set __user *, inp, fd_set __user *, outp,
		fd_set __user *, exp, struct timeval __user *, tvp);
DEF_HOOK_FN_0(semctl);
DEF_HOOK_FN_3(semget, key_t, key, int, nsems, int, semflg);
DEF_HOOK_FN_3(semop, int, semid, struct sembuf __user *, tsops,
		unsigned, nsops);
DEF_HOOK_FN_4(semtimedop, int, semid, struct sembuf __user *, tsops,
		unsigned, nsops, const struct timespec __user *, timeout);
DEF_HOOK_FN_4(sendfile, int, out_fd, int, in_fd, off_t __user *, offset, size_t, count);
DEF_HOOK_FN_3(sendmsg, int, fd, struct msghdr __user *, msg, unsigned, flags);
DEF_HOOK_FN_6(sendto, int, fd, void __user *, buff, size_t, len,
		unsigned, flags, struct sockaddr __user *, addr,
		int, addr_len);
DEF_HOOK_FN_2(setdomainname, char __user *, name, int, len);
DEF_HOOK_FN_1(setfsgid, gid_t, gid);
DEF_HOOK_FN_1(setfsuid, uid_t, uid);
DEF_HOOK_FN_1(setgid, gid_t, gid);
DEF_HOOK_FN_2(setgroups, int, gidsetsize, gid_t __user *, grouplist);
DEF_HOOK_FN_2(sethostname, char __user *, name, int, len);
DEF_HOOK_FN_3(setitimer, int, which, struct itimerval __user *, value,
		struct itimerval __user *, ovalue);
DEF_HOOK_FN_3(set_mempolicy, int, mode, unsigned long __user *, nmask,
		unsigned long, maxnode);
DEF_HOOK_FN_2(setpgid, pid_t, pid, pid_t, pgid);
DEF_HOOK_FN_3(setpriority, int, which, int, who, int, niceval);
DEF_HOOK_FN_2(setregid, gid_t, rgid, gid_t, egid);
DEF_HOOK_FN_3(setresgid, gid_t, rgid, gid_t, egid, gid_t, sgid);
DEF_HOOK_FN_3(setresuid, uid_t, ruid, uid_t, euid, uid_t, suid);
DEF_HOOK_FN_2(setreuid, uid_t, ruid, uid_t, euid);
DEF_HOOK_FN_2(setrlimit, unsigned int, resource, struct rlimit __user *, rlim);
DEF_HOOK_FN_2(set_robust_list, struct robust_list_head __user *, head,
		size_t, len);
DEF_HOOK_FN_0(setsid);
DEF_HOOK_FN_5(setsockopt, int, fd, int, level, int, optname,
		char __user *, optval, int, optlen);
DEF_HOOK_FN_1(set_tid_address, int __user *, tidptr);
DEF_HOOK_FN_2(settimeofday, struct timeval __user *, tv,
		struct timezone __user *, tz);
DEF_HOOK_FN_1(setuid, uid_t, uid);
DEF_HOOK_FN_5(setxattr, const char __user *, pathname,
		const char __user *, name, const void __user *, value,
		size_t, size, int, flags);
DEF_HOOK_FN_3(shmat, int, shmid, char __user *, shmaddr, int, shmflg);
DEF_HOOK_FN_3(shmctl, int, shmid, int, cmd, struct shmid_ds __user *, buf);
DEF_HOOK_FN_1(shmdt, char __user *, shmaddr);
DEF_HOOK_FN_3(shmget, key_t, key, size_t, size, int, shmflg);
DEF_HOOK_FN_2(shutdown, int, fd, int, how);
DEF_HOOK_FN_3(signalfd, int, ufd, sigset_t __user *, user_mask,
		size_t, sizemask);
DEF_HOOK_FN_4(signalfd4, int, ufd, sigset_t __user *, user_mask,
		size_t, sizemask, int, flags);
DEF_HOOK_FN_3(socket, int, family, int, type, int, protocol);
DEF_HOOK_FN_4(socketpair, int, family, int, type, int, protocol,
		int __user *, usockvec);
DEF_HOOK_FN_6(splice, int, fd_in, loff_t __user *, off_in,
		int, fd_out, loff_t __user *, off_out,
		size_t, len, unsigned int, flags);
DEF_HOOK_FN_2(stat, char __user *, filename, struct __old_kernel_stat __user *, statbuf);
DEF_HOOK_FN_2(statfs, const char __user *, pathname, struct statfs __user *, buf);
DEF_HOOK_FN_1(swapoff, const char __user *, specialfile);
DEF_HOOK_FN_2(swapon, const char __user *, specialfile, int, swap_flags);
DEF_HOOK_FN_2(symlink, const char __user *, oldname, const char __user *, newname);
DEF_HOOK_FN_3(symlinkat, const char __user *, oldname,
		int, newdfd, const char __user *, newname);
DEF_HOOK_FN_0(sync);
DEF_HOOK_FN_0(sync_file_range);
DEF_HOOK_FN_3(sysfs, int, option, unsigned long, arg1, unsigned long, arg2);
DEF_HOOK_FN_1(sysinfo, struct sysinfo __user *, info);
DEF_HOOK_FN_3(syslog, int, type, char __user *, buf, int, len);
DEF_HOOK_FN_4(tee, int, fdin, int, fdout, size_t, len, unsigned int, flags);
DEF_HOOK_FN_3(tgkill, pid_t, tgid, pid_t, pid, int, sig);
DEF_HOOK_FN_1(time, time_t __user *, tloc);
DEF_HOOK_FN_3(timer_create, clockid_t, which_clock,
		struct sigevent __user *, timer_event_spec,
		timer_t __user *, created_timer_id);
DEF_HOOK_FN_1(timer_delete, timer_t, timer_id);
DEF_HOOK_FN_2(timerfd_create, int, clockid, int, flags);
DEF_HOOK_FN_2(timerfd_gettime, int, ufd, struct itimerspec __user *, otmr);
DEF_HOOK_FN_4(timerfd_settime, int, ufd, int, flags,
		const struct itimerspec __user *, utmr,
		struct itimerspec __user *, otmr);
DEF_HOOK_FN_1(timer_getoverrun, timer_t, timer_id);
DEF_HOOK_FN_2(timer_gettime, timer_t, timer_id,
		struct itimerspec __user *, setting);
DEF_HOOK_FN_4(timer_settime, timer_t, timer_id, int, flags,
		const struct itimerspec __user *, new_setting,
		struct itimerspec __user *, old_setting);
DEF_HOOK_FN_1(times, struct tms __user *, tbuf);
DEF_HOOK_FN_2(tkill, pid_t, pid, int, sig);
DEF_HOOK_FN_2(truncate, const char __user *, path, unsigned long, length);
DEF_HOOK_FN_1(umask, int, mask);
DEF_HOOK_FN_1(unlink, const char __user *, pathname);
DEF_HOOK_FN_3(unlinkat, int, dfd, const char __user *, pathname, int, flag);
DEF_HOOK_FN_1(unshare, unsigned long, unshare_flags);
DEF_HOOK_FN_1(uselib, const char __user *, library);
DEF_HOOK_FN_2(ustat, unsigned, dev, struct ustat __user *, ubuf);
DEF_HOOK_FN_2(utime, char __user *, filename, struct utimbuf __user *, times);
DEF_HOOK_FN_4(utimensat, int, dfd, char __user *, filename,
		struct timespec __user *, utimes, int, flags);
DEF_HOOK_FN_2(utimes, char __user *, filename,
		struct timeval __user *, utimes);
DEF_HOOK_FN_0(vhangup);
DEF_HOOK_FN_4(vmsplice, int, fd, const struct iovec __user *, iov,
		unsigned long, nr_segs, unsigned int, flags);
DEF_HOOK_FN_4(wait4, pid_t, upid, int __user *, stat_addr,
		int, options, struct rusage __user *, ru);
DEF_HOOK_FN_5(waitid, int, which, pid_t, upid, struct siginfo __user *,
		infop, int, options, struct rusage __user *, ru);
DEF_HOOK_FN_3(write, unsigned int, fd, const char __user *, buf,
		size_t, count);
DEF_HOOK_FN_3(writev, unsigned long, fd, const struct iovec __user *, vec,
		unsigned long, vlen);
DEF_HOOK_FN_6( mmap, unsigned long, addr, unsigned long, len,
               unsigned long, prot, unsigned long, flags,
               unsigned long, fd, off_t, pgoff );
#endif /* _SYSCALL_PROTO_H_ */