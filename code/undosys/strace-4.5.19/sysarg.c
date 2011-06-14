#include "sysarg.h"
#include "sysarg-types.h"

struct sysarg_call sysarg_calls[] = {
    { "pciconfig_read", {
      { sysarg_int,  }, /* unsigned long bus */
      { sysarg_int,  }, /* unsigned long dfn */
      { sysarg_int,  }, /* unsigned long off */
      { sysarg_int,  }, /* unsigned long len */
      { sysarg_unknown,  }, /* void __user * buf */
      { sysarg_end } } },
    { "pciconfig_write", {
      { sysarg_int,  }, /* unsigned long bus */
      { sysarg_int,  }, /* unsigned long dfn */
      { sysarg_int,  }, /* unsigned long off */
      { sysarg_int,  }, /* unsigned long len */
      { sysarg_unknown,  }, /* void __user * buf */
      { sysarg_end } } },
    { "mq_open", {
      { sysarg_strnull,  }, /* const char __user * u_name */
      { sysarg_int,  }, /* int oflag */
      { sysarg_int,  }, /* mode_t mode */
      { sysarg_unknown,  }, /* struct mq_attr __user * u_attr */
      { sysarg_end } } },
    { "mq_unlink", {
      { sysarg_strnull,  }, /* const char __user * u_name */
      { sysarg_end } } },
    { "mq_timedsend", {
      { sysarg_int,  }, /* mqd_t mqdes */
      { sysarg_unknown,  }, /* const char __user * u_msg_ptr */
      { sysarg_int,  }, /* size_t msg_len */
      { sysarg_int,  }, /* unsigned int msg_prio */
      { sysarg_buf_fixlen, sizeof(struct timespec) }, /* const struct timespec __user * u_abs_timeout */
      { sysarg_end } } },
    { "mq_timedreceive", {
      { sysarg_int,  }, /* mqd_t mqdes */
      { sysarg_unknown,  }, /* char __user * u_msg_ptr */
      { sysarg_int,  }, /* size_t msg_len */
      { sysarg_unknown,  }, /* unsigned int __user * u_msg_prio */
      { sysarg_buf_fixlen, sizeof(struct timespec) }, /* const struct timespec __user * u_abs_timeout */
      { sysarg_end } } },
    { "mq_notify", {
      { sysarg_int,  }, /* mqd_t mqdes */
      { sysarg_unknown,  }, /* const struct sigevent __user * u_notification */
      { sysarg_end } } },
    { "mq_getsetattr", {
      { sysarg_int,  }, /* mqd_t mqdes */
      { sysarg_unknown,  }, /* const struct mq_attr __user * u_mqstat */
      { sysarg_unknown,  }, /* struct mq_attr __user * u_omqstat */
      { sysarg_end } } },
    { "semget", {
      { sysarg_int,  }, /* key_t key */
      { sysarg_int,  }, /* int nsems */
      { sysarg_int,  }, /* int semflg */
      { sysarg_end } } },
    { "semctl", {
      { sysarg_int,  }, /* int semid */
      { sysarg_int,  }, /* int semnum */
      { sysarg_int,  }, /* int cmd */
      { sysarg_unknown,  }, /* union semun arg */
      { sysarg_end } } },
    { "semtimedop", {
      { sysarg_int,  }, /* int semid */
      { sysarg_unknown,  }, /* struct sembuf __user * tsops */
      { sysarg_int,  }, /* unsigned nsops */
      { sysarg_buf_fixlen, sizeof(struct timespec) }, /* const struct timespec __user * timeout */
      { sysarg_end } } },
    { "semop", {
      { sysarg_int,  }, /* int semid */
      { sysarg_unknown,  }, /* struct sembuf __user * tsops */
      { sysarg_int,  }, /* unsigned nsops */
      { sysarg_end } } },
    { "shmget", {
      { sysarg_int,  }, /* key_t key */
      { sysarg_int,  }, /* size_t size */
      { sysarg_int,  }, /* int shmflg */
      { sysarg_end } } },
    { "shmctl", {
      { sysarg_int,  }, /* int shmid */
      { sysarg_int,  }, /* int cmd */
      { sysarg_unknown,  }, /* struct shmid_ds __user * buf */
      { sysarg_end } } },
    { "shmat", {
      { sysarg_int,  }, /* int shmid */
      { sysarg_unknown,  }, /* char __user * shmaddr */
      { sysarg_int,  }, /* int shmflg */
      { sysarg_end } } },
    { "shmdt", {
      { sysarg_unknown,  }, /* char __user * shmaddr */
      { sysarg_end } } },
    { "msgget", {
      { sysarg_int,  }, /* key_t key */
      { sysarg_int,  }, /* int msgflg */
      { sysarg_end } } },
    { "msgctl", {
      { sysarg_int,  }, /* int msqid */
      { sysarg_int,  }, /* int cmd */
      { sysarg_unknown,  }, /* struct msqid_ds __user * buf */
      { sysarg_end } } },
    { "msgsnd", {
      { sysarg_int,  }, /* int msqid */
      { sysarg_unknown,  }, /* struct msgbuf __user * msgp */
      { sysarg_int,  }, /* size_t msgsz */
      { sysarg_int,  }, /* int msgflg */
      { sysarg_end } } },
    { "msgrcv", {
      { sysarg_int,  }, /* int msqid */
      { sysarg_unknown,  }, /* struct msgbuf __user * msgp */
      { sysarg_int,  }, /* size_t msgsz */
      { sysarg_int,  }, /* long msgtyp */
      { sysarg_int,  }, /* int msgflg */
      { sysarg_end } } },
    { "timer_create", {
      { sysarg_int,  }, /* const clockid_t which_clock */
      { sysarg_unknown,  }, /* struct sigevent __user * timer_event_spec */
      { sysarg_unknown,  }, /* timer_t __user * created_timer_id */
      { sysarg_end } } },
    { "timer_gettime", {
      { sysarg_int,  }, /* timer_t timer_id */
      { sysarg_buf_fixlen, sizeof(struct itimerspec) }, /* struct itimerspec __user * setting */
      { sysarg_end } } },
    { "timer_getoverrun", {
      { sysarg_int,  }, /* timer_t timer_id */
      { sysarg_end } } },
    { "timer_settime", {
      { sysarg_int,  }, /* timer_t timer_id */
      { sysarg_int,  }, /* int flags */
      { sysarg_buf_fixlen, sizeof(struct itimerspec) }, /* const struct itimerspec __user * new_setting */
      { sysarg_buf_fixlen, sizeof(struct itimerspec) }, /* struct itimerspec __user * old_setting */
      { sysarg_end } } },
    { "timer_delete", {
      { sysarg_int,  }, /* timer_t timer_id */
      { sysarg_end } } },
    { "clock_settime", {
      { sysarg_int,  }, /* const clockid_t which_clock */
      { sysarg_buf_fixlen, sizeof(struct timespec) }, /* const struct timespec __user * tp */
      { sysarg_end } } },
    { "clock_gettime", {
      { sysarg_int,  }, /* const clockid_t which_clock */
      { sysarg_buf_fixlen, sizeof(struct timespec) }, /* struct timespec __user * tp */
      { sysarg_end } } },
    { "clock_getres", {
      { sysarg_int,  }, /* const clockid_t which_clock */
      { sysarg_buf_fixlen, sizeof(struct timespec) }, /* struct timespec __user * tp */
      { sysarg_end } } },
    { "clock_nanosleep", {
      { sysarg_int,  }, /* const clockid_t which_clock */
      { sysarg_int,  }, /* int flags */
      { sysarg_buf_fixlen, sizeof(struct timespec) }, /* const struct timespec __user * rqtp */
      { sysarg_buf_fixlen, sizeof(struct timespec) }, /* struct timespec __user * rmtp */
      { sysarg_end } } },
    { "alarm", {
      { sysarg_int,  }, /* unsigned int seconds */
      { sysarg_end } } },
    { "getpid", {
      { sysarg_end } } },
    { "getppid", {
      { sysarg_end } } },
    { "getuid", {
      { sysarg_end } } },
    { "geteuid", {
      { sysarg_end } } },
    { "getgid", {
      { sysarg_end } } },
    { "getegid", {
      { sysarg_end } } },
    { "gettid", {
      { sysarg_end } } },
    { "sysinfo", {
      { sysarg_unknown,  }, /* struct sysinfo __user * info */
      { sysarg_end } } },
    { "kexec_load", {
      { sysarg_int,  }, /* unsigned long entry */
      { sysarg_int,  }, /* unsigned long nr_segments */
      { sysarg_unknown,  }, /* struct kexec_segment __user * segments */
      { sysarg_int,  }, /* unsigned long flags */
      { sysarg_end } } },
    { "delete_module", {
      { sysarg_strnull,  }, /* const char __user * name_user */
      { sysarg_int,  }, /* unsigned int flags */
      { sysarg_end } } },
    { "init_module", {
      { sysarg_unknown,  }, /* void __user * umod */
      { sysarg_int,  }, /* unsigned long len */
      { sysarg_unknown,  }, /* const char __user * uargs */
      { sysarg_end } } },
    { "nanosleep", {
      { sysarg_buf_fixlen, sizeof(struct timespec) }, /* struct timespec __user * rqtp */
      { sysarg_buf_fixlen, sizeof(struct timespec) }, /* struct timespec __user * rmtp */
      { sysarg_end } } },
    { "capget", {
      { sysarg_unknown,  }, /* cap_user_header_t header */
      { sysarg_unknown,  }, /* cap_user_data_t dataptr */
      { sysarg_end } } },
    { "capset", {
      { sysarg_unknown,  }, /* cap_user_header_t header */
      { sysarg_unknown,  }, /* const cap_user_data_t data */
      { sysarg_end } } },
    { "sysctl", {
      { sysarg_unknown,  }, /* struct __sysctl_args __user * args */
      { sysarg_end } } },
    { "sysctl", {
      { sysarg_unknown,  }, /* struct __sysctl_args __user * args */
      { sysarg_end } } },
    { "set_robust_list", {
      { sysarg_buf_arglen, 1, 1 }, /* struct robust_list_head __user * head */
      { sysarg_int,  }, /* size_t len */
      { sysarg_end } } },
    { "get_robust_list", {
      { sysarg_int,  }, /* int pid */
      { sysarg_buf_arglenp, 1, 2 }, /* struct robust_list_head __user * __user * head_ptr */
      { sysarg_buf_fixlen, sizeof(size_t) }, /* size_t __user * len_ptr */
      { sysarg_end } } },
    { "chown16", {
      { sysarg_strnull,  }, /* const char __user * filename */
      { sysarg_int,  }, /* old_uid_t user */
      { sysarg_int,  }, /* old_gid_t group */
      { sysarg_end } } },
    { "lchown16", {
      { sysarg_strnull,  }, /* const char __user * filename */
      { sysarg_int,  }, /* old_uid_t user */
      { sysarg_int,  }, /* old_gid_t group */
      { sysarg_end } } },
    { "fchown16", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_int,  }, /* old_uid_t user */
      { sysarg_int,  }, /* old_gid_t group */
      { sysarg_end } } },
    { "setregid16", {
      { sysarg_int,  }, /* old_gid_t rgid */
      { sysarg_int,  }, /* old_gid_t egid */
      { sysarg_end } } },
    { "setgid16", {
      { sysarg_int,  }, /* old_gid_t gid */
      { sysarg_end } } },
    { "setreuid16", {
      { sysarg_int,  }, /* old_uid_t ruid */
      { sysarg_int,  }, /* old_uid_t euid */
      { sysarg_end } } },
    { "setuid16", {
      { sysarg_int,  }, /* old_uid_t uid */
      { sysarg_end } } },
    { "setresuid16", {
      { sysarg_int,  }, /* old_uid_t ruid */
      { sysarg_int,  }, /* old_uid_t euid */
      { sysarg_int,  }, /* old_uid_t suid */
      { sysarg_end } } },
    { "getresuid16", {
      { sysarg_buf_fixlen, sizeof(old_uid_t) }, /* old_uid_t __user * ruid */
      { sysarg_buf_fixlen, sizeof(old_uid_t) }, /* old_uid_t __user * euid */
      { sysarg_buf_fixlen, sizeof(old_uid_t) }, /* old_uid_t __user * suid */
      { sysarg_end } } },
    { "setresgid16", {
      { sysarg_int,  }, /* old_gid_t rgid */
      { sysarg_int,  }, /* old_gid_t egid */
      { sysarg_int,  }, /* old_gid_t sgid */
      { sysarg_end } } },
    { "getresgid16", {
      { sysarg_buf_fixlen, sizeof(old_gid_t) }, /* old_gid_t __user * rgid */
      { sysarg_buf_fixlen, sizeof(old_gid_t) }, /* old_gid_t __user * egid */
      { sysarg_buf_fixlen, sizeof(old_gid_t) }, /* old_gid_t __user * sgid */
      { sysarg_end } } },
    { "setfsuid16", {
      { sysarg_int,  }, /* old_uid_t uid */
      { sysarg_end } } },
    { "setfsgid16", {
      { sysarg_int,  }, /* old_gid_t gid */
      { sysarg_end } } },
    { "getgroups16", {
      { sysarg_int,  }, /* int gidsetsize */
      { sysarg_ignore,  }, /* old_gid_t __user * grouplist */
      { sysarg_end } } },
    { "setgroups16", {
      { sysarg_int,  }, /* int gidsetsize */
      { sysarg_ignore,  }, /* old_gid_t __user * grouplist */
      { sysarg_end } } },
    { "getuid16", {
      { sysarg_end } } },
    { "geteuid16", {
      { sysarg_end } } },
    { "getgid16", {
      { sysarg_end } } },
    { "getegid16", {
      { sysarg_end } } },
    { "_exit", {
      { sysarg_int,  }, /* int error_code */
      { sysarg_end } } },
    { "exit", {
      { sysarg_int,  }, /* int error_code */
      { sysarg_end } } },
    { "exit_group", {
      { sysarg_int,  }, /* int error_code */
      { sysarg_end } } },
    { "waitid", {
      { sysarg_int,  }, /* int which */
      { sysarg_int,  }, /* pid_t upid */
      { sysarg_unknown,  }, /* struct siginfo __user * infop */
      { sysarg_int,  }, /* int options */
      { sysarg_unknown,  }, /* struct rusage __user * ru */
      { sysarg_end } } },
    { "wait4", {
      { sysarg_int,  }, /* pid_t upid */
      { sysarg_buf_fixlen, sizeof(int) }, /* int __user * stat_addr */
      { sysarg_int,  }, /* int options */
      { sysarg_buf_fixlen, sizeof(struct rusage) }, /* struct rusage __user * ru */
      { sysarg_end } } },
    { "waitpid", {
      { sysarg_int,  }, /* pid_t pid */
      { sysarg_unknown,  }, /* int __user * stat_addr */
      { sysarg_int,  }, /* int options */
      { sysarg_end } } },
    { "unshare", {
      { sysarg_int,  }, /* unsigned long unshare_flags */
      { sysarg_end } } },
    { "perf_event_open", {
      { sysarg_unknown,  }, /* struct perf_event_attr __user * attr_uptr */
      { sysarg_int,  }, /* pid_t pid */
      { sysarg_int,  }, /* int cpu */
      { sysarg_int,  }, /* int group_fd */
      { sysarg_int,  }, /* unsigned long flags */
      { sysarg_end } } },
    { "ptrace", {
      { sysarg_int,  }, /* long request */
      { sysarg_int,  }, /* long pid */
      { sysarg_int,  }, /* long addr */
      { sysarg_int,  }, /* long data */
      { sysarg_end } } },
    { "getitimer", {
      { sysarg_int,  }, /* int which */
      { sysarg_buf_fixlen, sizeof(struct itimerval) }, /* struct itimerval __user * value */
      { sysarg_end } } },
    { "setitimer", {
      { sysarg_int,  }, /* int which */
      { sysarg_buf_fixlen, sizeof(struct itimerval) }, /* struct itimerval __user * value */
      { sysarg_buf_fixlen, sizeof(struct itimerval) }, /* struct itimerval __user * ovalue */
      { sysarg_end } } },
    { "setpriority", {
      { sysarg_int,  }, /* int which */
      { sysarg_int,  }, /* int who */
      { sysarg_int,  }, /* int niceval */
      { sysarg_end } } },
    { "getpriority", {
      { sysarg_int,  }, /* int which */
      { sysarg_int,  }, /* int who */
      { sysarg_end } } },
    { "reboot", {
      { sysarg_int,  }, /* int magic1 */
      { sysarg_int,  }, /* int magic2 */
      { sysarg_int,  }, /* unsigned int cmd */
      { sysarg_unknown,  }, /* void __user * arg */
      { sysarg_end } } },
    { "setregid", {
      { sysarg_int,  }, /* gid_t rgid */
      { sysarg_int,  }, /* gid_t egid */
      { sysarg_end } } },
    { "setgid", {
      { sysarg_int,  }, /* gid_t gid */
      { sysarg_end } } },
    { "setreuid", {
      { sysarg_int,  }, /* uid_t ruid */
      { sysarg_int,  }, /* uid_t euid */
      { sysarg_end } } },
    { "setuid", {
      { sysarg_int,  }, /* uid_t uid */
      { sysarg_end } } },
    { "setresuid", {
      { sysarg_int,  }, /* uid_t ruid */
      { sysarg_int,  }, /* uid_t euid */
      { sysarg_int,  }, /* uid_t suid */
      { sysarg_end } } },
    { "getresuid", {
      { sysarg_buf_fixlen, sizeof(uid_t) }, /* uid_t __user * ruid */
      { sysarg_buf_fixlen, sizeof(uid_t) }, /* uid_t __user * euid */
      { sysarg_buf_fixlen, sizeof(uid_t) }, /* uid_t __user * suid */
      { sysarg_end } } },
    { "setresgid", {
      { sysarg_int,  }, /* gid_t rgid */
      { sysarg_int,  }, /* gid_t egid */
      { sysarg_int,  }, /* gid_t sgid */
      { sysarg_end } } },
    { "getresgid", {
      { sysarg_buf_fixlen, sizeof(gid_t) }, /* gid_t __user * rgid */
      { sysarg_buf_fixlen, sizeof(gid_t) }, /* gid_t __user * egid */
      { sysarg_buf_fixlen, sizeof(gid_t) }, /* gid_t __user * sgid */
      { sysarg_end } } },
    { "setfsuid", {
      { sysarg_int,  }, /* uid_t uid */
      { sysarg_end } } },
    { "setfsgid", {
      { sysarg_int,  }, /* gid_t gid */
      { sysarg_end } } },
    { "times", {
      { sysarg_unknown,  }, /* struct tms __user * tbuf */
      { sysarg_end } } },
    { "setpgid", {
      { sysarg_int,  }, /* pid_t pid */
      { sysarg_int,  }, /* pid_t pgid */
      { sysarg_end } } },
    { "getpgid", {
      { sysarg_int,  }, /* pid_t pid */
      { sysarg_end } } },
    { "getpgrp", {
      { sysarg_end } } },
    { "getsid", {
      { sysarg_int,  }, /* pid_t pid */
      { sysarg_end } } },
    { "setsid", {
      { sysarg_end } } },
    { "newuname", {
      { sysarg_ignore,  }, /* struct new_utsname __user * name */
      { sysarg_end } } },
    { "sethostname", {
      { sysarg_strnull,  }, /* char __user * name */
      { sysarg_int,  }, /* int len */
      { sysarg_end } } },
    { "gethostname", {
      { sysarg_strnull,  }, /* char __user * name */
      { sysarg_int,  }, /* int len */
      { sysarg_end } } },
    { "setdomainname", {
      { sysarg_strnull,  }, /* char __user * name */
      { sysarg_int,  }, /* int len */
      { sysarg_end } } },
    { "getrlimit", {
      { sysarg_int,  }, /* unsigned int resource */
      { sysarg_buf_fixlen, sizeof(struct rlimit) }, /* struct rlimit __user * rlim */
      { sysarg_end } } },
    { "old_getrlimit", {
      { sysarg_int,  }, /* unsigned int resource */
      { sysarg_buf_fixlen, sizeof(struct rlimit) }, /* struct rlimit __user * rlim */
      { sysarg_end } } },
    { "setrlimit", {
      { sysarg_int,  }, /* unsigned int resource */
      { sysarg_buf_fixlen, sizeof(struct rlimit) }, /* struct rlimit __user * rlim */
      { sysarg_end } } },
    { "getrusage", {
      { sysarg_int,  }, /* int who */
      { sysarg_buf_fixlen, sizeof(struct rusage) }, /* struct rusage __user * ru */
      { sysarg_end } } },
    { "umask", {
      { sysarg_int,  }, /* int mask */
      { sysarg_end } } },
    { "prctl", {
      { sysarg_int,  }, /* int option */
      { sysarg_int,  }, /* unsigned long arg2 */
      { sysarg_int,  }, /* unsigned long arg3 */
      { sysarg_int,  }, /* unsigned long arg4 */
      { sysarg_int,  }, /* unsigned long arg5 */
      { sysarg_end } } },
    { "getcpu", {
      { sysarg_unknown,  }, /* unsigned __user * cpup */
      { sysarg_unknown,  }, /* unsigned __user * nodep */
      { sysarg_unknown,  }, /* struct getcpu_cache __user * unused */
      { sysarg_end } } },
    { "personality", {
      { sysarg_int,  }, /* u_long personality */
      { sysarg_end } } },
    { "nice", {
      { sysarg_int,  }, /* int increment */
      { sysarg_end } } },
    { "sched_setscheduler", {
      { sysarg_int,  }, /* pid_t pid */
      { sysarg_int,  }, /* int policy */
      { sysarg_unknown,  }, /* struct sched_param __user * param */
      { sysarg_end } } },
    { "sched_setparam", {
      { sysarg_int,  }, /* pid_t pid */
      { sysarg_unknown,  }, /* struct sched_param __user * param */
      { sysarg_end } } },
    { "sched_getscheduler", {
      { sysarg_int,  }, /* pid_t pid */
      { sysarg_end } } },
    { "sched_getparam", {
      { sysarg_int,  }, /* pid_t pid */
      { sysarg_unknown,  }, /* struct sched_param __user * param */
      { sysarg_end } } },
    { "sched_setaffinity", {
      { sysarg_int,  }, /* pid_t pid */
      { sysarg_int,  }, /* unsigned int len */
      { sysarg_unknown,  }, /* unsigned long __user * user_mask_ptr */
      { sysarg_end } } },
    { "sched_getaffinity", {
      { sysarg_int,  }, /* pid_t pid */
      { sysarg_int,  }, /* unsigned int len */
      { sysarg_unknown,  }, /* unsigned long __user * user_mask_ptr */
      { sysarg_end } } },
    { "sched_yield", {
      { sysarg_end } } },
    { "sched_get_priority_max", {
      { sysarg_int,  }, /* int policy */
      { sysarg_end } } },
    { "sched_get_priority_min", {
      { sysarg_int,  }, /* int policy */
      { sysarg_end } } },
    { "sched_rr_get_interval", {
      { sysarg_int,  }, /* pid_t pid */
      { sysarg_buf_fixlen, sizeof(struct timespec) }, /* struct timespec __user * interval */
      { sysarg_end } } },
    { "acct", {
      { sysarg_strnull,  }, /* const char __user * name */
      { sysarg_end } } },
    { "getgroups", {
      { sysarg_int,  }, /* int gidsetsize */
      { sysarg_ignore,  }, /* gid_t __user * grouplist */
      { sysarg_end } } },
    { "setgroups", {
      { sysarg_int,  }, /* int gidsetsize */
      { sysarg_ignore,  }, /* gid_t __user * grouplist */
      { sysarg_end } } },
    { "time", {
      { sysarg_unknown,  }, /* time_t __user * tloc */
      { sysarg_end } } },
    { "stime", {
      { sysarg_unknown,  }, /* time_t __user * tptr */
      { sysarg_end } } },
    { "gettimeofday", {
      { sysarg_buf_fixlen, sizeof(struct timeval) }, /* struct timeval __user * tv */
      { sysarg_unknown,  }, /* struct timezone __user * tz */
      { sysarg_end } } },
    { "settimeofday", {
      { sysarg_buf_fixlen, sizeof(struct timeval) }, /* struct timeval __user * tv */
      { sysarg_unknown,  }, /* struct timezone __user * tz */
      { sysarg_end } } },
    { "adjtimex", {
      { sysarg_unknown,  }, /* struct timex __user * txc_p */
      { sysarg_end } } },
    { "syslog", {
      { sysarg_int,  }, /* int type */
      { sysarg_unknown,  }, /* char __user * buf */
      { sysarg_int,  }, /* int len */
      { sysarg_end } } },
    { "restart_syscall", {
      { sysarg_end } } },
    { "rt_sigprocmask", {
      { sysarg_int,  }, /* int how */
      { sysarg_buf_arglen, 1, 3 }, /* sigset_t __user * set */
      { sysarg_buf_arglen, 1, 3 }, /* sigset_t __user * oset */
      { sysarg_int,  }, /* size_t sigsetsize */
      { sysarg_end } } },
    { "rt_sigpending", {
      { sysarg_unknown,  }, /* sigset_t __user * set */
      { sysarg_int,  }, /* size_t sigsetsize */
      { sysarg_end } } },
    { "rt_sigtimedwait", {
      { sysarg_unknown,  }, /* const sigset_t __user * uthese */
      { sysarg_unknown,  }, /* siginfo_t __user * uinfo */
      { sysarg_buf_fixlen, sizeof(struct timespec) }, /* const struct timespec __user * uts */
      { sysarg_int,  }, /* size_t sigsetsize */
      { sysarg_end } } },
    { "rt_sigreturn", {
      { sysarg_int }, /* unsigned long unused */
      { sysarg_end } } },
    { "kill", {
      { sysarg_int,  }, /* pid_t pid */
      { sysarg_int,  }, /* int sig */
      { sysarg_end } } },
    { "tgkill", {
      { sysarg_int,  }, /* pid_t tgid */
      { sysarg_int,  }, /* pid_t pid */
      { sysarg_int,  }, /* int sig */
      { sysarg_end } } },
    { "tkill", {
      { sysarg_int,  }, /* pid_t pid */
      { sysarg_int,  }, /* int sig */
      { sysarg_end } } },
    { "rt_sigqueueinfo", {
      { sysarg_int,  }, /* pid_t pid */
      { sysarg_int,  }, /* int sig */
      { sysarg_unknown,  }, /* siginfo_t __user * uinfo */
      { sysarg_end } } },
    { "rt_tgsigqueueinfo", {
      { sysarg_int,  }, /* pid_t tgid */
      { sysarg_int,  }, /* pid_t pid */
      { sysarg_int,  }, /* int sig */
      { sysarg_unknown,  }, /* siginfo_t __user * uinfo */
      { sysarg_end } } },
    { "sigpending", {
      { sysarg_unknown,  }, /* old_sigset_t __user * set */
      { sysarg_end } } },
    { "sigprocmask", {
      { sysarg_int,  }, /* int how */
      { sysarg_unknown,  }, /* old_sigset_t __user * set */
      { sysarg_unknown,  }, /* old_sigset_t __user * oset */
      { sysarg_end } } },
    { "rt_sigaction", {
      { sysarg_int,  }, /* int sig */
      { sysarg_buf_fixlen, sizeof(struct sigaction) }, /* const struct sigaction __user * act */
      { sysarg_buf_fixlen, sizeof(struct sigaction) }, /* struct sigaction __user * oact */
      { sysarg_int,  }, /* size_t sigsetsize */
      { sysarg_end } } },
    { "sgetmask", {
      { sysarg_end } } },
    { "ssetmask", {
      { sysarg_int,  }, /* int newmask */
      { sysarg_end } } },
    { "signal", {
      { sysarg_int,  }, /* int sig */
      { sysarg_unknown,  }, /* __sighandler_t handler */
      { sysarg_end } } },
    { "pause", {
      { sysarg_end } } },
    { "rt_sigsuspend", {
      { sysarg_unknown,  }, /* sigset_t __user * unewset */
      { sysarg_int,  }, /* size_t sigsetsize */
      { sysarg_end } } },
    { "socket", {
      { sysarg_int,  }, /* int family */
      { sysarg_int,  }, /* int type */
      { sysarg_int,  }, /* int protocol */
      { sysarg_end } } },
    { "socketpair", {
      { sysarg_int,  }, /* int family */
      { sysarg_int,  }, /* int type */
      { sysarg_int,  }, /* int protocol */
      { sysarg_ignore,  }, /* int __user * usockvec */
      { sysarg_end } } },
    { "bind", {
      { sysarg_int,  }, /* int fd */
      { sysarg_buf_arglen, 1, 2 }, /* struct sockaddr __user * umyaddr */
      { sysarg_int,  }, /* int addrlen */
      { sysarg_end } } },
    { "listen", {
      { sysarg_int,  }, /* int fd */
      { sysarg_int,  }, /* int backlog */
      { sysarg_end } } },
    { "accept4", {
      { sysarg_int,  }, /* int fd */
      { sysarg_buf_arglenp, 1, 2 }, /* struct sockaddr __user * upeer_sockaddr */
      { sysarg_buf_fixlen, sizeof(int) }, /* int __user * upeer_addrlen */
      { sysarg_int,  }, /* int flags */
      { sysarg_end } } },
    { "accept", {
      { sysarg_int,  }, /* int fd */
      { sysarg_buf_arglenp, 1, 2 }, /* struct sockaddr __user * upeer_sockaddr */
      { sysarg_buf_fixlen, sizeof(int) }, /* int __user * upeer_addrlen */
      { sysarg_end } } },
    { "connect", {
      { sysarg_int,  }, /* int fd */
      { sysarg_buf_arglen, 1, 2 }, /* struct sockaddr __user * uservaddr */
      { sysarg_int,  }, /* int addrlen */
      { sysarg_end } } },
    { "getsockname", {
      { sysarg_int,  }, /* int fd */
      { sysarg_buf_arglenp, 1, 2 }, /* struct sockaddr __user * usockaddr */
      { sysarg_buf_fixlen, sizeof(int) }, /* int __user * usockaddr_len */
      { sysarg_end } } },
    { "getpeername", {
      { sysarg_int,  }, /* int fd */
      { sysarg_buf_arglenp, 1, 2 }, /* struct sockaddr __user * usockaddr */
      { sysarg_buf_fixlen, sizeof(int) }, /* int __user * usockaddr_len */
      { sysarg_end } } },
    { "sendto", {
      { sysarg_int,  }, /* int fd */
      { sysarg_buf_arglen, 1, 2 }, /* void __user * buff */
      { sysarg_int,  }, /* size_t len */
      { sysarg_int,  }, /* unsigned flags */
      { sysarg_buf_arglen, 1, 5 }, /* struct sockaddr __user * addr */
      { sysarg_int,  }, /* int addr_len */
      { sysarg_end } } },
    { "send", {
      { sysarg_int,  }, /* int fd */
      { sysarg_buf_arglen, 1, 2 }, /* void __user * buff */
      { sysarg_int,  }, /* size_t len */
      { sysarg_int,  }, /* unsigned flags */
      { sysarg_end } } },
    { "recvfrom", {
      { sysarg_int,  }, /* int fd */
      { sysarg_buf_arglen, 1, 2 }, /* void __user * ubuf */
      { sysarg_int,  }, /* size_t size */
      { sysarg_int,  }, /* unsigned flags */
      { sysarg_buf_arglenp, 1, 5 }, /* struct sockaddr __user * addr */
      { sysarg_buf_fixlen, sizeof(int) }, /* int __user * addr_len */
      { sysarg_end } } },
    { "setsockopt", {
      { sysarg_int,  }, /* int fd */
      { sysarg_int,  }, /* int level */
      { sysarg_int,  }, /* int optname */
      { sysarg_buf_arglen, 1, 4 }, /* char __user * optval */
      { sysarg_int,  }, /* int optlen */
      { sysarg_end } } },
    { "getsockopt", {
      { sysarg_int,  }, /* int fd */
      { sysarg_int,  }, /* int level */
      { sysarg_int,  }, /* int optname */
      { sysarg_buf_arglenp, 1, 4 }, /* char __user * optval */
      { sysarg_buf_fixlen, sizeof(int) }, /* int __user * optlen */
      { sysarg_end } } },
    { "shutdown", {
      { sysarg_int,  }, /* int fd */
      { sysarg_int,  }, /* int how */
      { sysarg_end } } },
    { "sendmsg", {
      { sysarg_int,  }, /* int fd */
      { sysarg_unknown,  }, /* struct msghdr __user * msg */
      { sysarg_int,  }, /* unsigned flags */
      { sysarg_end } } },
    { "recvmsg", {
      { sysarg_int,  }, /* int fd */
      { sysarg_unknown,  }, /* struct msghdr __user * msg */
      { sysarg_int,  }, /* unsigned int flags */
      { sysarg_end } } },
    { "socketcall", {
      { sysarg_int,  }, /* int call */
      { sysarg_unknown,  }, /* unsigned long __user * args */
      { sysarg_end } } },
    { "add_key", {
      { sysarg_unknown,  }, /* const char __user * _type */
      { sysarg_unknown,  }, /* const char __user * _description */
      { sysarg_unknown,  }, /* const void __user * _payload */
      { sysarg_int,  }, /* size_t plen */
      { sysarg_int,  }, /* key_serial_t ringid */
      { sysarg_end } } },
    { "request_key", {
      { sysarg_unknown,  }, /* const char __user * _type */
      { sysarg_unknown,  }, /* const char __user * _description */
      { sysarg_unknown,  }, /* const char __user * _callout_info */
      { sysarg_int,  }, /* key_serial_t destringid */
      { sysarg_end } } },
    { "keyctl", {
      { sysarg_int,  }, /* int option */
      { sysarg_int,  }, /* unsigned long arg2 */
      { sysarg_int,  }, /* unsigned long arg3 */
      { sysarg_int,  }, /* unsigned long arg4 */
      { sysarg_int,  }, /* unsigned long arg5 */
      { sysarg_end } } },
    { "epoll_create1", {
      { sysarg_int,  }, /* int flags */
      { sysarg_end } } },
    { "epoll_create", {
      { sysarg_int,  }, /* int size */
      { sysarg_end } } },
    { "epoll_ctl", {
      { sysarg_int,  }, /* int epfd */
      { sysarg_int,  }, /* int op */
      { sysarg_int,  }, /* int fd */
      { sysarg_unknown,  }, /* struct epoll_event __user * event */
      { sysarg_end } } },
    { "epoll_wait", {
      { sysarg_int,  }, /* int epfd */
      { sysarg_unknown,  }, /* struct epoll_event __user * events */
      { sysarg_int,  }, /* int maxevents */
      { sysarg_int,  }, /* int timeout */
      { sysarg_end } } },
    { "epoll_pwait", {
      { sysarg_int,  }, /* int epfd */
      { sysarg_unknown,  }, /* struct epoll_event __user * events */
      { sysarg_int,  }, /* int maxevents */
      { sysarg_int,  }, /* int timeout */
      { sysarg_unknown,  }, /* const sigset_t __user * sigmask */
      { sysarg_int,  }, /* size_t sigsetsize */
      { sysarg_end } } },
    { "lookup_dcookie", {
      { sysarg_int,  }, /* u64 cookie64 */
      { sysarg_unknown,  }, /* char __user * buf */
      { sysarg_int,  }, /* size_t len */
      { sysarg_end } } },
    { "flock", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_int,  }, /* unsigned int cmd */
      { sysarg_end } } },
    { "old_readdir", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_unknown,  }, /* struct old_linux_dirent __user * dirent */
      { sysarg_int,  }, /* unsigned int count */
      { sysarg_end } } },
    { "getdents", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_buf_arglen, 1, 2 }, /* struct linux_dirent __user * dirent */
      { sysarg_int,  }, /* unsigned int count */
      { sysarg_end } } },
    { "getdents64", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_buf_arglen, 1, 2 }, /* struct linux_dirent64 __user * dirent */
      { sysarg_int,  }, /* unsigned int count */
      { sysarg_end } } },
    { "utime", {
      { sysarg_strnull,  }, /* char __user * filename */
      { sysarg_ignore,  }, /* struct utimbuf __user * times */
      { sysarg_end } } },
    { "utimensat", {
      { sysarg_int,  }, /* int dfd */
      { sysarg_strnull,  }, /* char __user * filename */
      { sysarg_ignore,  }, /* struct timespec __user * utimes */
      { sysarg_int,  }, /* int flags */
      { sysarg_end } } },
    { "futimesat", {
      { sysarg_int,  }, /* int dfd */
      { sysarg_strnull,  }, /* char __user * filename */
      { sysarg_ignore,  }, /* struct timeval __user * utimes */
      { sysarg_end } } },
    { "utimes", {
      { sysarg_strnull,  }, /* char __user * filename */
      { sysarg_unknown,  }, /* struct timeval __user * utimes */
      { sysarg_end } } },
    { "lseek", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_int,  }, /* off_t offset */
      { sysarg_int,  }, /* unsigned int origin */
      { sysarg_end } } },
    { "llseek", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_int,  }, /* unsigned long offset_high */
      { sysarg_int,  }, /* unsigned long offset_low */
      { sysarg_unknown,  }, /* loff_t __user * result */
      { sysarg_int,  }, /* unsigned int origin */
      { sysarg_end } } },
    { "read", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_buf_arglen_when_exiting, 1, 2 }, /* char __user * buf */
      { sysarg_int,  }, /* size_t count */
      { sysarg_end } } },
    { "write", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_buf_arglen_when_entering, 1, 2 }, /* const char __user * buf */
      { sysarg_int,  }, /* size_t count */
      { sysarg_end } } },
    { "pread64", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_buf_arglen_when_exiting, 1, 2 }, /* char __user * buf */
      { sysarg_int,  }, /* size_t count */
      { sysarg_int,  }, /* loff_t pos */
      { sysarg_end } } },
    { "pwrite64", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_buf_arglen_when_entering, 1, 2 }, /* const char __user * buf */
      { sysarg_int,  }, /* size_t count */
      { sysarg_int,  }, /* loff_t pos */
      { sysarg_end } } },
    { "readv", {
      { sysarg_int,  }, /* unsigned long fd */
      { sysarg_unknown,  }, /* const struct iovec __user * vec */
      { sysarg_int,  }, /* unsigned long vlen */
      { sysarg_end } } },
    { "writev", {
      { sysarg_int,  }, /* unsigned long fd */
      { sysarg_unknown,  }, /* const struct iovec __user * vec */
      { sysarg_int,  }, /* unsigned long vlen */
      { sysarg_end } } },
    { "preadv", {
      { sysarg_int,  }, /* unsigned long fd */
      { sysarg_unknown,  }, /* const struct iovec __user * vec */
      { sysarg_int,  }, /* unsigned long vlen */
      { sysarg_int,  }, /* unsigned long pos_l */
      { sysarg_int,  }, /* unsigned long pos_h */
      { sysarg_end } } },
    { "pwritev", {
      { sysarg_int,  }, /* unsigned long fd */
      { sysarg_unknown,  }, /* const struct iovec __user * vec */
      { sysarg_int,  }, /* unsigned long vlen */
      { sysarg_int,  }, /* unsigned long pos_l */
      { sysarg_int,  }, /* unsigned long pos_h */
      { sysarg_end } } },
    { "sendfile", {
      { sysarg_int,  }, /* int out_fd */
      { sysarg_int,  }, /* int in_fd */
      { sysarg_buf_fixlen, sizeof(off_t) }, /* off_t __user * offset */
      { sysarg_int,  }, /* size_t count */
      { sysarg_end } } },
    { "sendfile64", {
      { sysarg_int,  }, /* int out_fd */
      { sysarg_int,  }, /* int in_fd */
      { sysarg_buf_fixlen, sizeof(off_t) }, /* loff_t __user * offset */
      { sysarg_int,  }, /* size_t count */
      { sysarg_end } } },
    { "timerfd_create", {
      { sysarg_int,  }, /* int clockid */
      { sysarg_int,  }, /* int flags */
      { sysarg_end } } },
    { "timerfd_settime", {
      { sysarg_int,  }, /* int ufd */
      { sysarg_int,  }, /* int flags */
      { sysarg_buf_fixlen, sizeof(struct itimerspec) }, /* const struct itimerspec __user * utmr */
      { sysarg_buf_fixlen, sizeof(struct itimerspec) }, /* struct itimerspec __user * otmr */
      { sysarg_end } } },
    { "timerfd_gettime", {
      { sysarg_int,  }, /* int ufd */
      { sysarg_buf_fixlen, sizeof(struct itimerspec) }, /* struct itimerspec __user * otmr */
      { sysarg_end } } },
    { "dup3", {
      { sysarg_int,  }, /* unsigned int oldfd */
      { sysarg_int,  }, /* unsigned int newfd */
      { sysarg_int,  }, /* int flags */
      { sysarg_end } } },
    { "dup2", {
      { sysarg_int,  }, /* unsigned int oldfd */
      { sysarg_int,  }, /* unsigned int newfd */
      { sysarg_end } } },
    { "dup", {
      { sysarg_int,  }, /* unsigned int fildes */
      { sysarg_end } } },
    { "fcntl", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_int,  }, /* unsigned int cmd */
      { sysarg_int,  }, /* unsigned long arg */
      { sysarg_end } } },
    { "fcntl64", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_int,  }, /* unsigned int cmd */
      { sysarg_int,  }, /* unsigned long arg */
      { sysarg_end } } },
    { "sync", {
      { sysarg_end } } },
    { "fsync", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_end } } },
    { "fdatasync", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_end } } },
    { "sync_file_range", {
      { sysarg_int,  }, /* int fd */
      { sysarg_int,  }, /* loff_t offset */
      { sysarg_int,  }, /* loff_t nbytes */
      { sysarg_int,  }, /* unsigned int flags */
      { sysarg_end } } },
    { "sync_file_range2", {
      { sysarg_int,  }, /* int fd */
      { sysarg_int,  }, /* unsigned int flags */
      { sysarg_int,  }, /* loff_t offset */
      { sysarg_int,  }, /* loff_t nbytes */
      { sysarg_end } } },
    { "sysfs", {
      { sysarg_int,  }, /* int option */
      { sysarg_int,  }, /* unsigned long arg1 */
      { sysarg_int,  }, /* unsigned long arg2 */
      { sysarg_end } } },
    { "nfsservctl", {
      { sysarg_int,  }, /* int cmd */
      { sysarg_unknown,  }, /* struct nfsctl_arg __user * arg */
      { sysarg_unknown,  }, /* void __user * res */
      { sysarg_end } } },
    { "bdflush", {
      { sysarg_int,  }, /* int func */
      { sysarg_int,  }, /* long data */
      { sysarg_end } } },
    { "ioctl", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_int,  }, /* unsigned int cmd */
      { sysarg_int,  }, /* unsigned long arg */
      { sysarg_end } } },
    { "getcwd", {
      { sysarg_buf_arglen, 1, 1 }, /* char __user * buf */
      { sysarg_int,  }, /* unsigned long size */
      { sysarg_end } } },
    { "stat", {
      { sysarg_strnull,  }, /* char __user * filename */
      { sysarg_buf_fixlen, sizeof(struct __old_kernel_stat) }, /* struct __old_kernel_stat __user * statbuf */
      { sysarg_end } } },
    { "lstat", {
      { sysarg_strnull,  }, /* char __user * filename */
      { sysarg_buf_fixlen, sizeof(struct __old_kernel_stat) }, /* struct __old_kernel_stat __user * statbuf */
      { sysarg_end } } },
    { "fstat", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_buf_fixlen, sizeof(struct __old_kernel_stat) }, /* struct __old_kernel_stat __user * statbuf */
      { sysarg_end } } },
    { "newstat", {
      { sysarg_strnull,  }, /* char __user * filename */
      { sysarg_buf_fixlen, sizeof(struct stat) }, /* struct stat __user * statbuf */
      { sysarg_end } } },
    { "newlstat", {
      { sysarg_strnull,  }, /* char __user * filename */
      { sysarg_buf_fixlen, sizeof(struct stat) }, /* struct stat __user * statbuf */
      { sysarg_end } } },
    { "newfstatat", {
      { sysarg_int,  }, /* int dfd */
      { sysarg_strnull,  }, /* char __user * filename */
      { sysarg_buf_fixlen, sizeof(struct stat) }, /* struct stat __user * statbuf */
      { sysarg_int,  }, /* int flag */
      { sysarg_end } } },
    { "newfstat", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_buf_fixlen, sizeof(struct stat) }, /* struct stat __user * statbuf */
      { sysarg_end } } },
    { "readlinkat", {
      { sysarg_int,  }, /* int dfd */
      { sysarg_strnull,  }, /* const char __user * pathname */
      { sysarg_buf_arglen, 1, 3 }, /* char __user * buf */
      { sysarg_int,  }, /* int bufsiz */
      { sysarg_end } } },
    { "readlink", {
      { sysarg_strnull,  }, /* const char __user * path */
      { sysarg_buf_arglen, 1, 2 }, /* char __user * buf */
      { sysarg_int,  }, /* int bufsiz */
      { sysarg_end } } },
    { "stat64", {
      { sysarg_strnull,  }, /* char __user * filename */
      { sysarg_buf_fixlen, sizeof(struct stat64) }, /* struct stat64 __user * statbuf */
      { sysarg_end } } },
    { "lstat64", {
      { sysarg_strnull,  }, /* char __user * filename */
      { sysarg_buf_fixlen, sizeof(struct stat64) }, /* struct stat64 __user * statbuf */
      { sysarg_end } } },
    { "fstat64", {
      { sysarg_int,  }, /* unsigned long fd */
      { sysarg_buf_fixlen, sizeof(struct stat64) }, /* struct stat64 __user * statbuf */
      { sysarg_end } } },
    { "fstatat64", {
      { sysarg_int,  }, /* int dfd */
      { sysarg_strnull,  }, /* char __user * filename */
      { sysarg_buf_fixlen, sizeof(struct stat64) }, /* struct stat64 __user * statbuf */
      { sysarg_int,  }, /* int flag */
      { sysarg_end } } },
    { "umount", {
      { sysarg_strnull,  }, /* char __user * name */
      { sysarg_int,  }, /* int flags */
      { sysarg_end } } },
    { "oldumount", {
      { sysarg_strnull,  }, /* char __user * name */
      { sysarg_end } } },
    { "mount", {
      { sysarg_strnull,  }, /* char __user * dev_name */
      { sysarg_strnull,  }, /* char __user * dir_name */
      { sysarg_unknown,  }, /* char __user * type */
      { sysarg_int,  }, /* unsigned long flags */
      { sysarg_unknown,  }, /* void __user * data */
      { sysarg_end } } },
    { "pivot_root", {
      { sysarg_unknown,  }, /* const char __user * new_root */
      { sysarg_unknown,  }, /* const char __user * put_old */
      { sysarg_end } } },
    { "inotify_init1", {
      { sysarg_int,  }, /* int flags */
      { sysarg_end } } },
    { "inotify_init", {
      { sysarg_end } } },
    { "inotify_add_watch", {
      { sysarg_int,  }, /* int fd */
      { sysarg_strnull,  }, /* const char __user * pathname */
      { sysarg_int,  }, /* u32 mask */
      { sysarg_end } } },
    { "inotify_rm_watch", {
      { sysarg_int,  }, /* int fd */
      { sysarg_int,  }, /* __s32 wd */
      { sysarg_end } } },
    { "statfs", {
      { sysarg_strnull,  }, /* const char __user * pathname */
      { sysarg_buf_fixlen, sizeof(struct statfs) }, /* struct statfs __user * buf */
      { sysarg_end } } },
    { "statfs64", {
      { sysarg_strnull,  }, /* const char __user * pathname */
      { sysarg_int,  }, /* size_t sz */
      { sysarg_unknown,  }, /* struct statfs64 __user * buf */
      { sysarg_end } } },
    { "fstatfs", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_unknown,  }, /* struct statfs __user * buf */
      { sysarg_end } } },
    { "fstatfs64", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_int,  }, /* size_t sz */
      { sysarg_unknown,  }, /* struct statfs64 __user * buf */
      { sysarg_end } } },
    { "truncate", {
      { sysarg_strnull,  }, /* const char __user * path */
      { sysarg_int,  }, /* long length */
      { sysarg_end } } },
    { "ftruncate", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_int,  }, /* unsigned long length */
      { sysarg_end } } },
    { "truncate64", {
      { sysarg_strnull,  }, /* const char __user * path */
      { sysarg_int,  }, /* loff_t length */
      { sysarg_end } } },
    { "ftruncate64", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_int,  }, /* loff_t length */
      { sysarg_end } } },
    { "fallocate", {
      { sysarg_int,  }, /* int fd */
      { sysarg_int,  }, /* int mode */
      { sysarg_int,  }, /* loff_t offset */
      { sysarg_int,  }, /* loff_t len */
      { sysarg_end } } },
    { "faccessat", {
      { sysarg_int,  }, /* int dfd */
      { sysarg_strnull,  }, /* const char __user * filename */
      { sysarg_int,  }, /* int mode */
      { sysarg_end } } },
    { "access", {
      { sysarg_strnull,  }, /* const char __user * filename */
      { sysarg_int,  }, /* int mode */
      { sysarg_end } } },
    { "chdir", {
      { sysarg_strnull,  }, /* const char __user * filename */
      { sysarg_end } } },
    { "fchdir", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_end } } },
    { "chroot", {
      { sysarg_strnull,  }, /* const char __user * filename */
      { sysarg_end } } },
    { "fchmod", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_int,  }, /* mode_t mode */
      { sysarg_end } } },
    { "fchmodat", {
      { sysarg_int,  }, /* int dfd */
      { sysarg_strnull,  }, /* const char __user * filename */
      { sysarg_int,  }, /* mode_t mode */
      { sysarg_end } } },
    { "chmod", {
      { sysarg_strnull,  }, /* const char __user * filename */
      { sysarg_int,  }, /* mode_t mode */
      { sysarg_end } } },
    { "chown", {
      { sysarg_strnull,  }, /* const char __user * filename */
      { sysarg_int,  }, /* uid_t user */
      { sysarg_int,  }, /* gid_t group */
      { sysarg_end } } },
    { "fchownat", {
      { sysarg_int,  }, /* int dfd */
      { sysarg_strnull,  }, /* const char __user * filename */
      { sysarg_int,  }, /* uid_t user */
      { sysarg_int,  }, /* gid_t group */
      { sysarg_int,  }, /* int flag */
      { sysarg_end } } },
    { "lchown", {
      { sysarg_strnull,  }, /* const char __user * filename */
      { sysarg_int,  }, /* uid_t user */
      { sysarg_int,  }, /* gid_t group */
      { sysarg_end } } },
    { "fchown", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_int,  }, /* uid_t user */
      { sysarg_int,  }, /* gid_t group */
      { sysarg_end } } },
    { "open", {
      { sysarg_strnull,  }, /* const char __user * filename */
      { sysarg_int,  }, /* int flags */
      { sysarg_int,  }, /* int mode */
      { sysarg_end } } },
    { "openat", {
      { sysarg_int,  }, /* int dfd */
      { sysarg_strnull,  }, /* const char __user * filename */
      { sysarg_int,  }, /* int flags */
      { sysarg_int,  }, /* int mode */
      { sysarg_end } } },
    { "creat", {
      { sysarg_strnull,  }, /* const char __user * pathname */
      { sysarg_int,  }, /* int mode */
      { sysarg_end } } },
    { "close", {
      { sysarg_int,  }, /* unsigned int fd */
      { sysarg_end } } },
    { "vhangup", {
      { sysarg_end } } },
    { "mknodat", {
      { sysarg_int,  }, /* int dfd */
      { sysarg_strnull,  }, /* const char __user * filename */
      { sysarg_int,  }, /* int mode */
      { sysarg_int,  }, /* unsigned dev */
      { sysarg_end } } },
    { "mknod", {
      { sysarg_strnull,  }, /* const char __user * filename */
      { sysarg_int,  }, /* int mode */
      { sysarg_int,  }, /* unsigned dev */
      { sysarg_end } } },
    { "mkdirat", {
      { sysarg_int,  }, /* int dfd */
      { sysarg_strnull,  }, /* const char __user * pathname */
      { sysarg_int,  }, /* int mode */
      { sysarg_end } } },
    { "mkdir", {
      { sysarg_strnull,  }, /* const char __user * pathname */
      { sysarg_int,  }, /* int mode */
      { sysarg_end } } },
    { "rmdir", {
      { sysarg_strnull,  }, /* const char __user * pathname */
      { sysarg_end } } },
    { "unlinkat", {
      { sysarg_int,  }, /* int dfd */
      { sysarg_strnull,  }, /* const char __user * pathname */
      { sysarg_int,  }, /* int flag */
      { sysarg_end } } },
    { "unlink", {
      { sysarg_strnull,  }, /* const char __user * pathname */
      { sysarg_end } } },
    { "symlinkat", {
      { sysarg_strnull,  }, /* const char __user * oldname */
      { sysarg_int,  }, /* int newdfd */
      { sysarg_strnull,  }, /* const char __user * newname */
      { sysarg_end } } },
    { "symlink", {
      { sysarg_strnull,  }, /* const char __user * oldname */
      { sysarg_strnull,  }, /* const char __user * newname */
      { sysarg_end } } },
    { "linkat", {
      { sysarg_int,  }, /* int olddfd */
      { sysarg_strnull,  }, /* const char __user * oldname */
      { sysarg_int,  }, /* int newdfd */
      { sysarg_strnull,  }, /* const char __user * newname */
      { sysarg_int,  }, /* int flags */
      { sysarg_end } } },
    { "link", {
      { sysarg_strnull,  }, /* const char __user * oldname */
      { sysarg_strnull,  }, /* const char __user * newname */
      { sysarg_end } } },
    { "renameat", {
      { sysarg_int,  }, /* int olddfd */
      { sysarg_strnull,  }, /* const char __user * oldname */
      { sysarg_int,  }, /* int newdfd */
      { sysarg_strnull,  }, /* const char __user * newname */
      { sysarg_end } } },
    { "rename", {
      { sysarg_strnull,  }, /* const char __user * oldname */
      { sysarg_strnull,  }, /* const char __user * newname */
      { sysarg_end } } },
    { "signalfd4", {
      { sysarg_int,  }, /* int ufd */
      { sysarg_unknown,  }, /* sigset_t __user * user_mask */
      { sysarg_int,  }, /* size_t sizemask */
      { sysarg_int,  }, /* int flags */
      { sysarg_end } } },
    { "signalfd", {
      { sysarg_int,  }, /* int ufd */
      { sysarg_unknown,  }, /* sigset_t __user * user_mask */
      { sysarg_int,  }, /* size_t sizemask */
      { sysarg_end } } },
    { "vmsplice", {
      { sysarg_int,  }, /* int fd */
      { sysarg_unknown,  }, /* const struct iovec __user * iov */
      { sysarg_int,  }, /* unsigned long nr_segs */
      { sysarg_int,  }, /* unsigned int flags */
      { sysarg_end } } },
    { "splice", {
      { sysarg_int,  }, /* int fd_in */
      { sysarg_unknown,  }, /* loff_t __user * off_in */
      { sysarg_int,  }, /* int fd_out */
      { sysarg_unknown,  }, /* loff_t __user * off_out */
      { sysarg_int,  }, /* size_t len */
      { sysarg_int,  }, /* unsigned int flags */
      { sysarg_end } } },
    { "tee", {
      { sysarg_int,  }, /* int fdin */
      { sysarg_int,  }, /* int fdout */
      { sysarg_int,  }, /* size_t len */
      { sysarg_int,  }, /* unsigned int flags */
      { sysarg_end } } },
    { "quotactl", {
      { sysarg_int,  }, /* unsigned int cmd */
      { sysarg_unknown,  }, /* const char __user * special */
      { sysarg_int,  }, /* qid_t id */
      { sysarg_unknown,  }, /* void __user * addr */
      { sysarg_end } } },
    { "setxattr", {
      { sysarg_strnull,  }, /* const char __user * pathname */
      { sysarg_strnull,  }, /* const char __user * name */
      { sysarg_unknown,  }, /* const void __user * value */
      { sysarg_int,  }, /* size_t size */
      { sysarg_int,  }, /* int flags */
      { sysarg_end } } },
    { "lsetxattr", {
      { sysarg_strnull,  }, /* const char __user * pathname */
      { sysarg_strnull,  }, /* const char __user * name */
      { sysarg_unknown,  }, /* const void __user * value */
      { sysarg_int,  }, /* size_t size */
      { sysarg_int,  }, /* int flags */
      { sysarg_end } } },
    { "fsetxattr", {
      { sysarg_int,  }, /* int fd */
      { sysarg_strnull,  }, /* const char __user * name */
      { sysarg_unknown,  }, /* const void __user * value */
      { sysarg_int,  }, /* size_t size */
      { sysarg_int,  }, /* int flags */
      { sysarg_end } } },
    { "getxattr", {
      { sysarg_strnull,  }, /* const char __user * pathname */
      { sysarg_strnull,  }, /* const char __user * name */
      { sysarg_buf_arglen, 1, 3 }, /* void __user * value */
      { sysarg_int,  }, /* size_t size */
      { sysarg_end } } },
    { "lgetxattr", {
      { sysarg_strnull,  }, /* const char __user * pathname */
      { sysarg_strnull,  }, /* const char __user * name */
      { sysarg_buf_arglen, 1, 3 }, /* void __user * value */
      { sysarg_int,  }, /* size_t size */
      { sysarg_end } } },
    { "fgetxattr", {
      { sysarg_int,  }, /* int fd */
      { sysarg_strnull,  }, /* const char __user * name */
      { sysarg_buf_arglen, 1, 3 }, /* void __user * value */
      { sysarg_int,  }, /* size_t size */
      { sysarg_end } } },
    { "listxattr", {
      { sysarg_strnull,  }, /* const char __user * pathname */
      { sysarg_buf_arglen, 1, 2 }, /* char __user * list */
      { sysarg_int,  }, /* size_t size */
      { sysarg_end } } },
    { "llistxattr", {
      { sysarg_strnull,  }, /* const char __user * pathname */
      { sysarg_buf_arglen, 1, 2 }, /* char __user * list */
      { sysarg_int,  }, /* size_t size */
      { sysarg_end } } },
    { "flistxattr", {
      { sysarg_int,  }, /* int fd */
      { sysarg_buf_arglen, 1, 2 }, /* char __user * list */
      { sysarg_int,  }, /* size_t size */
      { sysarg_end } } },
    { "removexattr", {
      { sysarg_strnull,  }, /* const char __user * pathname */
      { sysarg_strnull,  }, /* const char __user * name */
      { sysarg_end } } },
    { "lremovexattr", {
      { sysarg_strnull,  }, /* const char __user * pathname */
      { sysarg_strnull,  }, /* const char __user * name */
      { sysarg_end } } },
    { "fremovexattr", {
      { sysarg_int,  }, /* int fd */
      { sysarg_strnull,  }, /* const char __user * name */
      { sysarg_end } } },
    { "io_setup", {
      { sysarg_int,  }, /* unsigned nr_events */
      { sysarg_unknown,  }, /* aio_context_t __user * ctxp */
      { sysarg_end } } },
    { "io_destroy", {
      { sysarg_int,  }, /* aio_context_t ctx */
      { sysarg_end } } },
    { "io_submit", {
      { sysarg_int,  }, /* aio_context_t ctx_id */
      { sysarg_int,  }, /* long nr */
      { sysarg_unknown,  }, /* struct iocb __user * __user * iocbpp */
      { sysarg_end } } },
    { "io_cancel", {
      { sysarg_int,  }, /* aio_context_t ctx_id */
      { sysarg_unknown,  }, /* struct iocb __user * iocb */
      { sysarg_unknown,  }, /* struct io_event __user * result */
      { sysarg_end } } },
    { "io_getevents", {
      { sysarg_int,  }, /* aio_context_t ctx_id */
      { sysarg_int,  }, /* long min_nr */
      { sysarg_int,  }, /* long nr */
      { sysarg_unknown,  }, /* struct io_event __user * events */
      { sysarg_buf_fixlen, sizeof(struct timespec) }, /* struct timespec __user * timeout */
      { sysarg_end } } },
    { "uselib", {
      { sysarg_unknown,  }, /* const char __user * library */
      { sysarg_end } } },
    { "pipe2", {
      { sysarg_unknown,  }, /* int __user * fildes */
      { sysarg_int,  }, /* int flags */
      { sysarg_end } } },
    { "pipe", {
      { sysarg_ignore,  }, /* int __user * fildes */
      { sysarg_end } } },
    { "eventfd2", {
      { sysarg_int,  }, /* unsigned int count */
      { sysarg_int,  }, /* int flags */
      { sysarg_end } } },
    { "eventfd", {
      { sysarg_int,  }, /* unsigned int count */
      { sysarg_end } } },
    { "ustat", {
      { sysarg_int,  }, /* unsigned dev */
      { sysarg_unknown,  }, /* struct ustat __user * ubuf */
      { sysarg_end } } },
    { "ioprio_set", {
      { sysarg_int,  }, /* int which */
      { sysarg_int,  }, /* int who */
      { sysarg_int,  }, /* int ioprio */
      { sysarg_end } } },
    { "ioprio_get", {
      { sysarg_int,  }, /* int which */
      { sysarg_int,  }, /* int who */
      { sysarg_end } } },
    { "select", {
      { sysarg_int,  }, /* int n */
      { sysarg_ignore,  }, /* fd_set __user * inp */
      { sysarg_ignore,  }, /* fd_set __user * outp */
      { sysarg_ignore,  }, /* fd_set __user * exp */
      { sysarg_buf_fixlen, sizeof(struct timeval) }, /* struct timeval __user * tvp */
      { sysarg_end } } },
    { "pselect6", {
      { sysarg_int,  }, /* int n */
      { sysarg_ignore,  }, /* fd_set __user * inp */
      { sysarg_ignore,  }, /* fd_set __user * outp */
      { sysarg_ignore,  }, /* fd_set __user * exp */
      { sysarg_buf_fixlen, sizeof(struct timespec) }, /* struct timespec __user * tsp */
      { sysarg_ignore,  }, /* void __user * sig */
      { sysarg_end } } },
    { "poll", {
      { sysarg_ignore,  }, /* struct pollfd __user * ufds */
      { sysarg_int,  }, /* unsigned int nfds */
      { sysarg_int,  }, /* long timeout_msecs */
      { sysarg_end } } },
    { "ppoll", {
      { sysarg_unknown,  }, /* struct pollfd __user * ufds */
      { sysarg_int,  }, /* unsigned int nfds */
      { sysarg_buf_fixlen, sizeof(struct timespec) }, /* struct timespec __user * tsp */
      { sysarg_unknown,  }, /* const sigset_t __user * sigmask */
      { sysarg_int,  }, /* size_t sigsetsize */
      { sysarg_end } } },
    { "mincore", {
      { sysarg_int,  }, /* unsigned long start */
      { sysarg_int,  }, /* size_t len */
      { sysarg_unknown,  }, /* unsigned char __user * vec */
      { sysarg_end } } },
    { "brk", {
      { sysarg_int,  }, /* unsigned long brk */
      { sysarg_end } } },
    { "munmap", {
      { sysarg_int,  }, /* unsigned long addr */
      { sysarg_int,  }, /* size_t len */
      { sysarg_end } } },
    { "mremap", {
      { sysarg_int,  }, /* unsigned long addr */
      { sysarg_int,  }, /* unsigned long old_len */
      { sysarg_int,  }, /* unsigned long new_len */
      { sysarg_int,  }, /* unsigned long flags */
      { sysarg_int,  }, /* unsigned long new_addr */
      { sysarg_end } } },
    { "mbind", {
      { sysarg_int,  }, /* unsigned long start */
      { sysarg_int,  }, /* unsigned long len */
      { sysarg_int,  }, /* unsigned long mode */
      { sysarg_unknown,  }, /* unsigned long __user * nmask */
      { sysarg_int,  }, /* unsigned long maxnode */
      { sysarg_int,  }, /* unsigned flags */
      { sysarg_end } } },
    { "set_mempolicy", {
      { sysarg_int,  }, /* int mode */
      { sysarg_unknown,  }, /* unsigned long __user * nmask */
      { sysarg_int,  }, /* unsigned long maxnode */
      { sysarg_end } } },
    { "migrate_pages", {
      { sysarg_int,  }, /* pid_t pid */
      { sysarg_int,  }, /* unsigned long maxnode */
      { sysarg_unknown,  }, /* const unsigned long __user * old_nodes */
      { sysarg_unknown,  }, /* const unsigned long __user * new_nodes */
      { sysarg_end } } },
    { "get_mempolicy", {
      { sysarg_unknown,  }, /* int __user * policy */
      { sysarg_unknown,  }, /* unsigned long __user * nmask */
      { sysarg_int,  }, /* unsigned long maxnode */
      { sysarg_int,  }, /* unsigned long addr */
      { sysarg_int,  }, /* unsigned long flags */
      { sysarg_end } } },
    { "readahead", {
      { sysarg_int,  }, /* int fd */
      { sysarg_int,  }, /* loff_t offset */
      { sysarg_int,  }, /* size_t count */
      { sysarg_end } } },
    { "mlock", {
      { sysarg_int,  }, /* unsigned long start */
      { sysarg_int,  }, /* size_t len */
      { sysarg_end } } },
    { "munlock", {
      { sysarg_int,  }, /* unsigned long start */
      { sysarg_int,  }, /* size_t len */
      { sysarg_end } } },
    { "mlockall", {
      { sysarg_int,  }, /* int flags */
      { sysarg_end } } },
    { "munlockall", {
      { sysarg_end } } },
    { "fadvise64_64", {
      { sysarg_int,  }, /* int fd */
      { sysarg_int,  }, /* loff_t offset */
      { sysarg_int,  }, /* loff_t len */
      { sysarg_int,  }, /* int advice */
      { sysarg_end } } },
    { "fadvise64", {
      { sysarg_int,  }, /* int fd */
      { sysarg_int,  }, /* loff_t offset */
      { sysarg_int,  }, /* size_t len */
      { sysarg_int,  }, /* int advice */
      { sysarg_end } } },
    { "msync", {
      { sysarg_int,  }, /* unsigned long start */
      { sysarg_int,  }, /* size_t len */
      { sysarg_int,  }, /* int flags */
      { sysarg_end } } },
    { "brk", {
      { sysarg_int,  }, /* unsigned long brk */
      { sysarg_end } } },
    { "munmap", {
      { sysarg_int,  }, /* unsigned long addr */
      { sysarg_int,  }, /* size_t len */
      { sysarg_end } } },
    { "mremap", {
      { sysarg_int,  }, /* unsigned long addr */
      { sysarg_int,  }, /* unsigned long old_len */
      { sysarg_int,  }, /* unsigned long new_len */
      { sysarg_int,  }, /* unsigned long flags */
      { sysarg_int,  }, /* unsigned long new_addr */
      { sysarg_end } } },
    { "move_pages", {
      { sysarg_int,  }, /* pid_t pid */
      { sysarg_int,  }, /* unsigned long nr_pages */
      { sysarg_unknown,  }, /* const void __user * __user * pages */
      { sysarg_unknown,  }, /* const int __user * nodes */
      { sysarg_unknown,  }, /* int __user * status */
      { sysarg_int,  }, /* int flags */
      { sysarg_end } } },
    { "mprotect", {
      { sysarg_int,  }, /* unsigned long start */
      { sysarg_int,  }, /* size_t len */
      { sysarg_int,  }, /* unsigned long prot */
      { sysarg_end } } },
    { "remap_file_pages", {
      { sysarg_int,  }, /* unsigned long start */
      { sysarg_int,  }, /* unsigned long size */
      { sysarg_int,  }, /* unsigned long prot */
      { sysarg_int,  }, /* unsigned long pgoff */
      { sysarg_int,  }, /* unsigned long flags */
      { sysarg_end } } },
    { "madvise", {
      { sysarg_int,  }, /* unsigned long start */
      { sysarg_int,  }, /* size_t len_in */
      { sysarg_int,  }, /* int behavior */
      { sysarg_end } } },
    { "swapoff", {
      { sysarg_unknown,  }, /* const char __user * specialfile */
      { sysarg_end } } },
    { "swapon", {
      { sysarg_unknown,  }, /* const char __user * specialfile */
      { sysarg_int,  }, /* int swap_flags */
      { sysarg_end } } },
    { "mmap", {
      { sysarg_int,  }, /* unsigned long addr */
      { sysarg_int,  }, /* unsigned long len */
      { sysarg_int,  }, /* unsigned long prot */
      { sysarg_int,  }, /* unsigned long flags */
      { sysarg_int,  }, /* unsigned long fd */
      { sysarg_int,  }, /* unsigned long off */
      { sysarg_end } } },
    { "uname", {
      { sysarg_ignore,  }, /* struct new_utsname __user * name */
      { sysarg_end } } },

    /* These are going to be special anyway.. */
    { "fork", {
      { sysarg_end } } },
    { "vfork", {
      { sysarg_end } } },
    { "clone", {
      { sysarg_end } } },
    { "execve", {
      { sysarg_strnull },
      { sysarg_argv_when_entering },
      { sysarg_argv_when_entering },
      { sysarg_end } } },

    /* These are ~useless for replay / dependency analysis */
    { "arch_prctl", {
      { sysarg_end } } },
    { "set_tid_address", {
      { sysarg_ignore }, /* int __user * tidptr */
      { sysarg_end } } },
    { "futex", {
      { sysarg_ignore }, /* u32 __user * uaddr */
      { sysarg_int,  }, /* int op */
      { sysarg_int,  }, /* u32 val */
      { sysarg_ignore }, /* struct timespec __user * utime, FIXME it depends on op, check sys_futex */
      { sysarg_ignore }, /* u32 __user * uaddr2 */
      { sysarg_int,  }, /* u32 val3 */
      { sysarg_end } } },

    /* Special undo system calls */
    { "undo_func_start", {
      { sysarg_strnull },		/* const char *undo_mgr */
      { sysarg_strnull },		/* const char *funcname */
      { sysarg_int },			/* int arglen */
      { sysarg_buf_arglen, 1, 2 },	/* void *argbuf */
      { sysarg_end } } },

    { "undo_func_end", {
      { sysarg_int },			/* int retlen */
      { sysarg_buf_arglen, 1, 0 },	/* void *retbuf */
      { sysarg_end } } },

    { "undo_mask_start", {
      { sysarg_int },			/* int fd */
      { sysarg_end } } },

    { "undo_mask_end", {
      { sysarg_int },			/* int fd */
      { sysarg_end } } },

    { "undo_depend", {
      { sysarg_int },			/* int fd */
      { sysarg_strnull },		/* const char *subname */
      { sysarg_strnull },		/* const char *mgr */
      { sysarg_int },			/* int proc_to_obj */
      { sysarg_int },			/* int obj_to_proc */
      { sysarg_end } } },
      
    { 0 },
};
