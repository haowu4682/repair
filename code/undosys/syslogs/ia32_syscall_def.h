#define __NR32_restart_syscall         (0)
#define __NR32_exit                    (1)
#define __NR32_read                    (3)
#define __NR32_write                   (4)
#define __NR32_close                   (6)
#define __NR32_creat                   (8)
#define __NR32_link                    (9)
#define __NR32_unlink                  (10)
#define __NR32_execve                  (11)
#define __NR32_chdir                   (12)
#define __NR32_mknod                   (14)
#define __NR32_chmod                   (15)
#define __NR32_stat                    (18)
#define __NR32_getpid                  (20)
#define __NR32_alarm                   (27)
#define __NR32_fstat                   (28)
#define __NR32_pause                   (29)
#define __NR32_access                  (33)
#define __NR32_sync                    (36)
#define __NR32_rename                  (38)
#define __NR32_mkdir                   (39)
#define __NR32_rmdir                   (40)
#define __NR32_dup                     (41)
#define __NR32_brk                     (45)
#define __NR32_acct                    (51)
#define __NR32_setpgid                 (57)
#define __NR32_umask                   (60)
#define __NR32_chroot                  (61)
#define __NR32_dup2                    (63)
#define __NR32_getppid                 (64)
#define __NR32_getpgrp                 (65)
#define __NR32_setsid                  (66)
#define __NR32_sethostname             (74)
#define __NR32_symlink                 (83)
#define __NR32_lstat                   (84)
#define __NR32_readlink                (85)
#define __NR32_uselib                  (86)
#define __NR32_swapon                  (87)
#define __NR32_reboot                  (88)
#define __NR32_munmap                  (91)
#define __NR32_truncate                (92)
#define __NR32_ftruncate               (93)
#define __NR32_fchmod                  (94)
#define __NR32_getpriority             (96)
#define __NR32_setpriority             (97)
#define __NR32_syslog                  (103)
#define __NR32_vhangup                 (111)
#define __NR32_swapoff                 (115)
#define __NR32_fsync                   (118)
#define __NR32_setdomainname           (121)
#define __NR32_init_module             (128)
#define __NR32_delete_module           (129)
#define __NR32_getpgid                 (132)
#define __NR32_fchdir                  (133)
#define __NR32_sysfs                   (135)
#define __NR32_personality             (136)
#define __NR32_flock                   (143)
#define __NR32_msync                   (144)
#define __NR32_getsid                  (147)
#define __NR32_fdatasync               (148)
#define __NR32_mlock                   (150)
#define __NR32_munlock                 (151)
#define __NR32_mlockall                (152)
#define __NR32_munlockall              (153)
#define __NR32_sched_setparam          (154)
#define __NR32_sched_getparam          (155)
#define __NR32_sched_setscheduler      (156)
#define __NR32_sched_getscheduler      (157)
#define __NR32_sched_yield             (158)
#define __NR32_sched_get_priority_max  (159)
#define __NR32_sched_get_priority_min  (160)
#define __NR32_mremap                  (163)
#define __NR32_poll                    (168)
#define __NR32_prctl                   (172)
#define __NR32_rt_sigsuspend           (179)
#define __NR32_getcwd                  (183)
#define __NR32_capget                  (184)
#define __NR32_capset                  (185)
#define __NR32_lchown                  (198)
#define __NR32_getuid                  (199)
#define __NR32_getgid                  (200)
#define __NR32_geteuid                 (201)
#define __NR32_getegid                 (202)
#define __NR32_setreuid                (203)
#define __NR32_setregid                (204)
#define __NR32_getgroups               (205)
#define __NR32_setgroups               (206)
#define __NR32_fchown                  (207)
#define __NR32_setresuid               (208)
#define __NR32_getresuid               (209)
#define __NR32_setresgid               (210)
#define __NR32_getresgid               (211)
#define __NR32_chown                   (212)
#define __NR32_setuid                  (213)
#define __NR32_setgid                  (214)
#define __NR32_setfsuid                (215)
#define __NR32_setfsgid                (216)
#define __NR32_pivot_root              (217)
#define __NR32_mincore                 (218)
#define __NR32_madvise                 (219)
#define __NR32_gettid                  (224)
#define __NR32_setxattr                (226)
#define __NR32_lsetxattr               (227)
#define __NR32_fsetxattr               (228)
#define __NR32_getxattr                (229)
#define __NR32_lgetxattr               (230)
#define __NR32_fgetxattr               (231)
#define __NR32_listxattr               (232)
#define __NR32_llistxattr              (233)
#define __NR32_flistxattr              (234)
#define __NR32_removexattr             (235)
#define __NR32_lremovexattr            (236)
#define __NR32_fremovexattr            (237)
#define __NR32_tkill                   (238)
#define __NR32_io_destroy              (246)
#define __NR32_io_cancel               (249)
#define __NR32_exit_group              (252)
#define __NR32_epoll_create            (254)
#define __NR32_epoll_ctl               (255)
#define __NR32_epoll_wait              (256)
#define __NR32_remap_file_pages        (257)
#define __NR32_set_tid_address         (258)
#define __NR32_timer_getoverrun        (262)
#define __NR32_timer_delete            (263)
#define __NR32_tgkill                  (270)
#define __NR32_mbind                   (274)
#define __NR32_set_mempolicy           (276)
#define __NR32_mq_unlink               (278)
#define __NR32_add_key                 (286)
#define __NR32_request_key             (287)
#define __NR32_keyctl                  (288)
#define __NR32_ioprio_set              (289)
#define __NR32_ioprio_get              (290)
#define __NR32_inotify_init            (291)
#define __NR32_inotify_add_watch       (292)
#define __NR32_inotify_rm_watch        (293)
#define __NR32_migrate_pages           (294)
#define __NR32_mkdirat                 (296)
#define __NR32_mknodat                 (297)
#define __NR32_fchownat                (298)
#define __NR32_unlinkat                (301)
#define __NR32_renameat                (302)
#define __NR32_linkat                  (303)
#define __NR32_symlinkat               (304)
#define __NR32_readlinkat              (305)
#define __NR32_fchmodat                (306)
#define __NR32_faccessat               (307)
#define __NR32_unshare                 (310)
#define __NR32_splice                  (313)
#define __NR32_tee                     (315)
#define __NR32_epoll_pwait             (319)
#define __NR32_timerfd_create          (322)
#define __NR32_eventfd                 (323)
#define __NR32_eventfd2                (328)
#define __NR32_epoll_create1           (329)
#define __NR32_dup3                    (330)
#define __NR32_pipe2                   (331)
#define __NR32_inotify_init1           (332)
