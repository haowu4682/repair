#include <asm/unistd.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/unistd.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <asm/statfs.h>

#include "sysarg.h"
#include "undosysarg.h"

// FIXME 512 -> max syscall number anyway
struct sysarg_call sysarg_calls[512];
int nsysarg_calls[512] = {0,};

void init_syscalls( void ) {


sysarg_calls[__NR_mq_open] = 
   (struct sysarg_call)
     { "mq_open", {
        { sysarg_strnull,  }, /* const char __user * u_name */
        { sysarg_int,  }, /* int oflag */
        { sysarg_int,  }, /* mode_t mode */
        { sysarg_unknown,  }, /* struct mq_attr __user * u_attr */
        { sysarg_end } } };
nsysarg_calls[__NR_mq_open] = 4;

sysarg_calls[__NR_mq_unlink] = 
   (struct sysarg_call)
     { "mq_unlink", {
        { sysarg_strnull,  }, /* const char __user * u_name */
        { sysarg_end } } };
nsysarg_calls[__NR_mq_unlink] = 1;

sysarg_calls[__NR_mq_timedsend] = 
   (struct sysarg_call)
     { "mq_timedsend", {
        { sysarg_int,  }, /* mqd_t mqdes */
        { sysarg_unknown,  }, /* const char __user * u_msg_ptr */
        { sysarg_int,  }, /* size_t msg_len */
        { sysarg_int,  }, /* unsigned int msg_prio */
        { sysarg_buf_fixlen, sizeof(struct timespec) }, /* const struct timespec __user * u_abs_timeout */
        { sysarg_end } } };
nsysarg_calls[__NR_mq_timedsend] = 5;

sysarg_calls[__NR_mq_timedreceive] = 
   (struct sysarg_call)
     { "mq_timedreceive", {
        { sysarg_int,  }, /* mqd_t mqdes */
        { sysarg_unknown,  }, /* char __user * u_msg_ptr */
        { sysarg_int,  }, /* size_t msg_len */
        { sysarg_unknown,  }, /* unsigned int __user * u_msg_prio */
        { sysarg_buf_fixlen, sizeof(struct timespec) }, /* const struct timespec __user * u_abs_timeout */
        { sysarg_end } } };
nsysarg_calls[__NR_mq_timedreceive] = 5;

sysarg_calls[__NR_mq_notify] = 
   (struct sysarg_call)
     { "mq_notify", {
        { sysarg_int,  }, /* mqd_t mqdes */
        { sysarg_unknown,  }, /* const struct sigevent __user * u_notification */
        { sysarg_end } } };
nsysarg_calls[__NR_mq_notify] = 2;

sysarg_calls[__NR_mq_getsetattr] = 
   (struct sysarg_call)
     { "mq_getsetattr", {
        { sysarg_int,  }, /* mqd_t mqdes */
        { sysarg_unknown,  }, /* const struct mq_attr __user * u_mqstat */
        { sysarg_unknown,  }, /* struct mq_attr __user * u_omqstat */
        { sysarg_end } } };
nsysarg_calls[__NR_mq_getsetattr] = 3;

sysarg_calls[__NR_semget] = 
   (struct sysarg_call)
     { "semget", {
        { sysarg_int,  }, /* key_t key */
        { sysarg_int,  }, /* int nsems */
        { sysarg_int,  }, /* int semflg */
        { sysarg_end } } };
nsysarg_calls[__NR_semget] = 3;

sysarg_calls[__NR_semctl] = 
   (struct sysarg_call)
     { "semctl", {
        { sysarg_int,  }, /* int semid */
        { sysarg_int,  }, /* int semnum */
        { sysarg_int,  }, /* int cmd */
        { sysarg_unknown,  }, /* union semun arg */
        { sysarg_end } } };
nsysarg_calls[__NR_semctl] = 4;

sysarg_calls[__NR_semtimedop] = 
   (struct sysarg_call)
     { "semtimedop", {
        { sysarg_int,  }, /* int semid */
        { sysarg_unknown,  }, /* struct sembuf __user * tsops */
        { sysarg_int,  }, /* unsigned nsops */
        { sysarg_buf_fixlen, sizeof(struct timespec) }, /* const struct timespec __user * timeout */
        { sysarg_end } } };
nsysarg_calls[__NR_semtimedop] = 4;

sysarg_calls[__NR_semop] = 
   (struct sysarg_call)
     { "semop", {
        { sysarg_int,  }, /* int semid */
        { sysarg_unknown,  }, /* struct sembuf __user * tsops */
        { sysarg_int,  }, /* unsigned nsops */
        { sysarg_end } } };
nsysarg_calls[__NR_semop] = 3;

sysarg_calls[__NR_shmget] = 
   (struct sysarg_call)
     { "shmget", {
        { sysarg_int,  }, /* key_t key */
        { sysarg_int,  }, /* size_t size */
        { sysarg_int,  }, /* int shmflg */
        { sysarg_end } } };
nsysarg_calls[__NR_shmget] = 3;

sysarg_calls[__NR_shmctl] = 
   (struct sysarg_call)
     { "shmctl", {
        { sysarg_int,  }, /* int shmid */
        { sysarg_int,  }, /* int cmd */
        { sysarg_unknown,  }, /* struct shmid_ds __user * buf */
        { sysarg_end } } };
nsysarg_calls[__NR_shmctl] = 3;

sysarg_calls[__NR_shmat] = 
   (struct sysarg_call)
     { "shmat", {
        { sysarg_int,  }, /* int shmid */
        { sysarg_unknown,  }, /* char __user * shmaddr */
        { sysarg_int,  }, /* int shmflg */
        { sysarg_end } } };
nsysarg_calls[__NR_shmat] = 3;

sysarg_calls[__NR_shmdt] = 
   (struct sysarg_call)
     { "shmdt", {
        { sysarg_unknown,  }, /* char __user * shmaddr */
        { sysarg_end } } };
nsysarg_calls[__NR_shmdt] = 1;

sysarg_calls[__NR_msgget] = 
   (struct sysarg_call)
     { "msgget", {
        { sysarg_int,  }, /* key_t key */
        { sysarg_int,  }, /* int msgflg */
        { sysarg_end } } };
nsysarg_calls[__NR_msgget] = 2;

sysarg_calls[__NR_msgctl] = 
   (struct sysarg_call)
     { "msgctl", {
        { sysarg_int,  }, /* int msqid */
        { sysarg_int,  }, /* int cmd */
        { sysarg_unknown,  }, /* struct msqid_ds __user * buf */
        { sysarg_end } } };
nsysarg_calls[__NR_msgctl] = 3;

sysarg_calls[__NR_msgsnd] = 
   (struct sysarg_call)
     { "msgsnd", {
        { sysarg_int,  }, /* int msqid */
        { sysarg_unknown,  }, /* struct msgbuf __user * msgp */
        { sysarg_int,  }, /* size_t msgsz */
        { sysarg_int,  }, /* int msgflg */
        { sysarg_end } } };
nsysarg_calls[__NR_msgsnd] = 4;

sysarg_calls[__NR_msgrcv] = 
   (struct sysarg_call)
     { "msgrcv", {
        { sysarg_int,  }, /* int msqid */
        { sysarg_unknown,  }, /* struct msgbuf __user * msgp */
        { sysarg_int,  }, /* size_t msgsz */
        { sysarg_int,  }, /* long msgtyp */
        { sysarg_int,  }, /* int msgflg */
        { sysarg_end } } };
nsysarg_calls[__NR_msgrcv] = 5;

sysarg_calls[__NR_timer_create] = 
   (struct sysarg_call)
     { "timer_create", {
        { sysarg_int,  }, /* const clockid_t which_clock */
        { sysarg_unknown,  }, /* struct sigevent __user * timer_event_spec */
        { sysarg_unknown,  }, /* timer_t __user * created_timer_id */
        { sysarg_end } } };
nsysarg_calls[__NR_timer_create] = 3;

sysarg_calls[__NR_timer_gettime] = 
   (struct sysarg_call)
     { "timer_gettime", {
        { sysarg_int,  }, /* timer_t timer_id */
        { sysarg_buf_fixlen, sizeof(struct itimerspec) }, /* struct itimerspec __user * setting */
        { sysarg_end } } };
nsysarg_calls[__NR_timer_gettime] = 2;

sysarg_calls[__NR_timer_getoverrun] = 
   (struct sysarg_call)
     { "timer_getoverrun", {
        { sysarg_int,  }, /* timer_t timer_id */
        { sysarg_end } } };
nsysarg_calls[__NR_timer_getoverrun] = 1;

sysarg_calls[__NR_timer_settime] = 
   (struct sysarg_call)
     { "timer_settime", {
        { sysarg_int,  }, /* timer_t timer_id */
        { sysarg_int,  }, /* int flags */
        { sysarg_buf_fixlen, sizeof(struct itimerspec) }, /* const struct itimerspec __user * new_setting */
        { sysarg_buf_fixlen, sizeof(struct itimerspec) }, /* struct itimerspec __user * old_setting */
        { sysarg_end } } };
nsysarg_calls[__NR_timer_settime] = 4;

sysarg_calls[__NR_timer_delete] = 
   (struct sysarg_call)
     { "timer_delete", {
        { sysarg_int,  }, /* timer_t timer_id */
        { sysarg_end } } };
nsysarg_calls[__NR_timer_delete] = 1;

sysarg_calls[__NR_clock_settime] = 
   (struct sysarg_call)
     { "clock_settime", {
        { sysarg_int,  }, /* const clockid_t which_clock */
        { sysarg_buf_fixlen, sizeof(struct timespec) }, /* const struct timespec __user * tp */
        { sysarg_end } } };
nsysarg_calls[__NR_clock_settime] = 2;

sysarg_calls[__NR_clock_gettime] = 
   (struct sysarg_call)
     { "clock_gettime", {
        { sysarg_int,  }, /* const clockid_t which_clock */
        { sysarg_buf_fixlen, sizeof(struct timespec) }, /* struct timespec __user * tp */
        { sysarg_end } } };
nsysarg_calls[__NR_clock_gettime] = 2;

sysarg_calls[__NR_clock_getres] = 
   (struct sysarg_call)
     { "clock_getres", {
        { sysarg_int,  }, /* const clockid_t which_clock */
        { sysarg_buf_fixlen, sizeof(struct timespec) }, /* struct timespec __user * tp */
        { sysarg_end } } };
nsysarg_calls[__NR_clock_getres] = 2;

sysarg_calls[__NR_clock_nanosleep] = 
   (struct sysarg_call)
     { "clock_nanosleep", {
        { sysarg_int,  }, /* const clockid_t which_clock */
        { sysarg_int,  }, /* int flags */
        { sysarg_buf_fixlen, sizeof(struct timespec) }, /* const struct timespec __user * rqtp */
        { sysarg_buf_fixlen, sizeof(struct timespec) }, /* struct timespec __user * rmtp */
        { sysarg_end } } };
nsysarg_calls[__NR_clock_nanosleep] = 4;

sysarg_calls[__NR_alarm] = 
   (struct sysarg_call)
     { "alarm", {
        { sysarg_int,  }, /* unsigned int seconds */
        { sysarg_end } } };
nsysarg_calls[__NR_alarm] = 1;

sysarg_calls[__NR_getpid] = 
   (struct sysarg_call)
     { "getpid", {
        { sysarg_end } } };
nsysarg_calls[__NR_getpid] = 0;

sysarg_calls[__NR_getppid] = 
   (struct sysarg_call)
     { "getppid", {
        { sysarg_end } } };
nsysarg_calls[__NR_getppid] = 0;

sysarg_calls[__NR_getuid] = 
   (struct sysarg_call)
     { "getuid", {
        { sysarg_end } } };
nsysarg_calls[__NR_getuid] = 0;

sysarg_calls[__NR_geteuid] = 
   (struct sysarg_call)
     { "geteuid", {
        { sysarg_end } } };
nsysarg_calls[__NR_geteuid] = 0;

sysarg_calls[__NR_getgid] = 
   (struct sysarg_call)
     { "getgid", {
        { sysarg_end } } };
nsysarg_calls[__NR_getgid] = 0;

sysarg_calls[__NR_getegid] = 
   (struct sysarg_call)
     { "getegid", {
        { sysarg_end } } };
nsysarg_calls[__NR_getegid] = 0;

sysarg_calls[__NR_gettid] = 
   (struct sysarg_call)
     { "gettid", {
        { sysarg_end } } };
nsysarg_calls[__NR_gettid] = 0;

sysarg_calls[__NR_sysinfo] = 
   (struct sysarg_call)
     { "sysinfo", {
        { sysarg_unknown,  }, /* struct sysinfo __user * info */
        { sysarg_end } } };
nsysarg_calls[__NR_sysinfo] = 1;

sysarg_calls[__NR_kexec_load] = 
   (struct sysarg_call)
     { "kexec_load", {
        { sysarg_int,  }, /* unsigned long entry */
        { sysarg_int,  }, /* unsigned long nr_segments */
        { sysarg_unknown,  }, /* struct kexec_segment __user * segments */
        { sysarg_int,  }, /* unsigned long flags */
        { sysarg_end } } };
nsysarg_calls[__NR_kexec_load] = 4;

sysarg_calls[__NR_delete_module] = 
   (struct sysarg_call)
     { "delete_module", {
        { sysarg_strnull,  }, /* const char __user * name_user */
        { sysarg_int,  }, /* unsigned int flags */
        { sysarg_end } } };
nsysarg_calls[__NR_delete_module] = 2;

sysarg_calls[__NR_init_module] = 
   (struct sysarg_call)
     { "init_module", {
        { sysarg_unknown,  }, /* void __user * umod */
        { sysarg_int,  }, /* unsigned long len */
        { sysarg_unknown,  }, /* const char __user * uargs */
        { sysarg_end } } };
nsysarg_calls[__NR_init_module] = 3;

sysarg_calls[__NR_nanosleep] = 
   (struct sysarg_call)
     { "nanosleep", {
        { sysarg_buf_fixlen, sizeof(struct timespec) }, /* struct timespec __user * rqtp */
        { sysarg_buf_fixlen, sizeof(struct timespec) }, /* struct timespec __user * rmtp */
        { sysarg_end } } };
nsysarg_calls[__NR_nanosleep] = 2;

sysarg_calls[__NR_capget] = 
   (struct sysarg_call)
     { "capget", {
        { sysarg_unknown,  }, /* cap_user_header_t header */
        { sysarg_unknown,  }, /* cap_user_data_t dataptr */
        { sysarg_end } } };
nsysarg_calls[__NR_capget] = 2;

sysarg_calls[__NR_capset] = 
   (struct sysarg_call)
     { "capset", {
        { sysarg_unknown,  }, /* cap_user_header_t header */
        { sysarg_unknown,  }, /* const cap_user_data_t data */
        { sysarg_end } } };
nsysarg_calls[__NR_capset] = 2;

sysarg_calls[__NR_set_robust_list] = 
   (struct sysarg_call)
     { "set_robust_list", {
        { sysarg_buf_arglen, 1, 1 }, /* struct robust_list_head __user * head */
        { sysarg_int,  }, /* size_t len */
        { sysarg_end } } };
nsysarg_calls[__NR_set_robust_list] = 2;

sysarg_calls[__NR_get_robust_list] = 
   (struct sysarg_call)
     { "get_robust_list", {
        { sysarg_int,  }, /* int pid */
        { sysarg_buf_arglenp, 1, 2 }, /* struct robust_list_head __user * __user * head_ptr */
        { sysarg_buf_fixlen, sizeof(size_t) }, /* size_t __user * len_ptr */
        { sysarg_end } } };
nsysarg_calls[__NR_get_robust_list] = 3;

sysarg_calls[__NR_exit] = 
   (struct sysarg_call)
     { "exit", {
        { sysarg_int,  }, /* int error_code */
        { sysarg_end } } };
nsysarg_calls[__NR_exit] = 1;

sysarg_calls[__NR_exit_group] = 
   (struct sysarg_call)
     { "exit_group", {
        { sysarg_int,  }, /* int error_code */
        { sysarg_end } } };
nsysarg_calls[__NR_exit_group] = 1;

sysarg_calls[__NR_waitid] = 
   (struct sysarg_call)
     { "waitid", {
        { sysarg_int,  }, /* int which */
        { sysarg_int,  }, /* pid_t upid */
        { sysarg_unknown,  }, /* struct siginfo __user * infop */
        { sysarg_int,  }, /* int options */
        { sysarg_unknown,  }, /* struct rusage __user * ru */
        { sysarg_end } } };
nsysarg_calls[__NR_waitid] = 5;

sysarg_calls[__NR_wait4] = 
   (struct sysarg_call)
     { "wait4", {
        { sysarg_int,  }, /* pid_t upid */
        { sysarg_buf_fixlen, sizeof(int) }, /* int __user * stat_addr */
        { sysarg_int,  }, /* int options */
        { sysarg_buf_fixlen, sizeof(struct rusage) }, /* struct rusage __user * ru */
        { sysarg_end } } };
nsysarg_calls[__NR_wait4] = 4;

sysarg_calls[__NR_unshare] = 
   (struct sysarg_call)
     { "unshare", {
        { sysarg_int,  }, /* unsigned long unshare_flags */
        { sysarg_end } } };
nsysarg_calls[__NR_unshare] = 1;

sysarg_calls[__NR_ptrace] = 
   (struct sysarg_call)
     { "ptrace", {
        { sysarg_int,  }, /* long request */
        { sysarg_int,  }, /* long pid */
        { sysarg_int,  }, /* long addr */
        { sysarg_int,  }, /* long data */
        { sysarg_end } } };
nsysarg_calls[__NR_ptrace] = 4;

sysarg_calls[__NR_getitimer] = 
   (struct sysarg_call)
     { "getitimer", {
        { sysarg_int,  }, /* int which */
        { sysarg_buf_fixlen, sizeof(struct itimerval) }, /* struct itimerval __user * value */
        { sysarg_end } } };
nsysarg_calls[__NR_getitimer] = 2;

sysarg_calls[__NR_setitimer] = 
   (struct sysarg_call)
     { "setitimer", {
        { sysarg_int,  }, /* int which */
        { sysarg_buf_fixlen, sizeof(struct itimerval) }, /* struct itimerval __user * value */
        { sysarg_buf_fixlen, sizeof(struct itimerval) }, /* struct itimerval __user * ovalue */
        { sysarg_end } } };
nsysarg_calls[__NR_setitimer] = 3;

sysarg_calls[__NR_setpriority] = 
   (struct sysarg_call)
     { "setpriority", {
        { sysarg_int,  }, /* int which */
        { sysarg_int,  }, /* int who */
        { sysarg_int,  }, /* int niceval */
        { sysarg_end } } };
nsysarg_calls[__NR_setpriority] = 3;

sysarg_calls[__NR_getpriority] = 
   (struct sysarg_call)
     { "getpriority", {
        { sysarg_int,  }, /* int which */
        { sysarg_int,  }, /* int who */
        { sysarg_end } } };
nsysarg_calls[__NR_getpriority] = 2;

sysarg_calls[__NR_reboot] = 
   (struct sysarg_call)
     { "reboot", {
        { sysarg_int,  }, /* int magic1 */
        { sysarg_int,  }, /* int magic2 */
        { sysarg_int,  }, /* unsigned int cmd */
        { sysarg_unknown,  }, /* void __user * arg */
        { sysarg_end } } };
nsysarg_calls[__NR_reboot] = 4;

sysarg_calls[__NR_setregid] = 
   (struct sysarg_call)
     { "setregid", {
        { sysarg_int,  }, /* gid_t rgid */
        { sysarg_int,  }, /* gid_t egid */
        { sysarg_end } } };
nsysarg_calls[__NR_setregid] = 2;

sysarg_calls[__NR_setgid] = 
   (struct sysarg_call)
     { "setgid", {
        { sysarg_int,  }, /* gid_t gid */
        { sysarg_end } } };
nsysarg_calls[__NR_setgid] = 1;

sysarg_calls[__NR_setreuid] = 
   (struct sysarg_call)
     { "setreuid", {
        { sysarg_int,  }, /* uid_t ruid */
        { sysarg_int,  }, /* uid_t euid */
        { sysarg_end } } };
nsysarg_calls[__NR_setreuid] = 2;

sysarg_calls[__NR_setuid] = 
   (struct sysarg_call)
     { "setuid", {
        { sysarg_int,  }, /* uid_t uid */
        { sysarg_end } } };
nsysarg_calls[__NR_setuid] = 1;

sysarg_calls[__NR_setresuid] = 
   (struct sysarg_call)
     { "setresuid", {
        { sysarg_int,  }, /* uid_t ruid */
        { sysarg_int,  }, /* uid_t euid */
        { sysarg_int,  }, /* uid_t suid */
        { sysarg_end } } };
nsysarg_calls[__NR_setresuid] = 3;

sysarg_calls[__NR_getresuid] = 
   (struct sysarg_call)
     { "getresuid", {
        { sysarg_buf_fixlen, sizeof(uid_t) }, /* uid_t __user * ruid */
        { sysarg_buf_fixlen, sizeof(uid_t) }, /* uid_t __user * euid */
        { sysarg_buf_fixlen, sizeof(uid_t) }, /* uid_t __user * suid */
        { sysarg_end } } };
nsysarg_calls[__NR_getresuid] = 3;

sysarg_calls[__NR_setresgid] = 
   (struct sysarg_call)
     { "setresgid", {
        { sysarg_int,  }, /* gid_t rgid */
        { sysarg_int,  }, /* gid_t egid */
        { sysarg_int,  }, /* gid_t sgid */
        { sysarg_end } } };
nsysarg_calls[__NR_setresgid] = 3;

sysarg_calls[__NR_getresgid] = 
   (struct sysarg_call)
     { "getresgid", {
        { sysarg_buf_fixlen, sizeof(gid_t) }, /* gid_t __user * rgid */
        { sysarg_buf_fixlen, sizeof(gid_t) }, /* gid_t __user * egid */
        { sysarg_buf_fixlen, sizeof(gid_t) }, /* gid_t __user * sgid */
        { sysarg_end } } };
nsysarg_calls[__NR_getresgid] = 3;

sysarg_calls[__NR_setfsuid] = 
   (struct sysarg_call)
     { "setfsuid", {
        { sysarg_int,  }, /* uid_t uid */
        { sysarg_end } } };
nsysarg_calls[__NR_setfsuid] = 1;

sysarg_calls[__NR_setfsgid] = 
   (struct sysarg_call)
     { "setfsgid", {
        { sysarg_int,  }, /* gid_t gid */
        { sysarg_end } } };
nsysarg_calls[__NR_setfsgid] = 1;

sysarg_calls[__NR_times] = 
   (struct sysarg_call)
     { "times", {
        { sysarg_unknown,  }, /* struct tms __user * tbuf */
        { sysarg_end } } };
nsysarg_calls[__NR_times] = 1;

sysarg_calls[__NR_setpgid] = 
   (struct sysarg_call)
     { "setpgid", {
        { sysarg_int,  }, /* pid_t pid */
        { sysarg_int,  }, /* pid_t pgid */
        { sysarg_end } } };
nsysarg_calls[__NR_setpgid] = 2;

sysarg_calls[__NR_getpgid] = 
   (struct sysarg_call)
     { "getpgid", {
        { sysarg_int,  }, /* pid_t pid */
        { sysarg_end } } };
nsysarg_calls[__NR_getpgid] = 1;

sysarg_calls[__NR_getpgrp] = 
   (struct sysarg_call)
     { "getpgrp", {
        { sysarg_end } } };
nsysarg_calls[__NR_getpgrp] = 0;

sysarg_calls[__NR_getsid] = 
   (struct sysarg_call)
     { "getsid", {
        { sysarg_int,  }, /* pid_t pid */
        { sysarg_end } } };
nsysarg_calls[__NR_getsid] = 1;

sysarg_calls[__NR_setsid] = 
   (struct sysarg_call)
     { "setsid", {
        { sysarg_end } } };
nsysarg_calls[__NR_setsid] = 0;

sysarg_calls[__NR_sethostname] = 
   (struct sysarg_call)
     { "sethostname", {
        { sysarg_strnull,  }, /* char __user * name */
        { sysarg_int,  }, /* int len */
        { sysarg_end } } };
nsysarg_calls[__NR_sethostname] = 2;

sysarg_calls[__NR_setdomainname] = 
   (struct sysarg_call)
     { "setdomainname", {
        { sysarg_strnull,  }, /* char __user * name */
        { sysarg_int,  }, /* int len */
        { sysarg_end } } };
nsysarg_calls[__NR_setdomainname] = 2;

sysarg_calls[__NR_getrlimit] = 
   (struct sysarg_call)
     { "getrlimit", {
        { sysarg_int,  }, /* unsigned int resource */
        { sysarg_buf_fixlen, sizeof(struct rlimit) }, /* struct rlimit __user * rlim */
        { sysarg_end } } };
nsysarg_calls[__NR_getrlimit] = 2;

sysarg_calls[__NR_setrlimit] = 
   (struct sysarg_call)
     { "setrlimit", {
        { sysarg_int,  }, /* unsigned int resource */
        { sysarg_buf_fixlen, sizeof(struct rlimit) }, /* struct rlimit __user * rlim */
        { sysarg_end } } };
nsysarg_calls[__NR_setrlimit] = 2;

sysarg_calls[__NR_getrusage] = 
   (struct sysarg_call)
     { "getrusage", {
        { sysarg_int,  }, /* int who */
        { sysarg_buf_fixlen, sizeof(struct rusage) }, /* struct rusage __user * ru */
        { sysarg_end } } };
nsysarg_calls[__NR_getrusage] = 2;

sysarg_calls[__NR_umask] = 
   (struct sysarg_call)
     { "umask", {
        { sysarg_int,  }, /* int mask */
        { sysarg_end } } };
nsysarg_calls[__NR_umask] = 1;

sysarg_calls[__NR_prctl] = 
   (struct sysarg_call)
     { "prctl", {
        { sysarg_int,  }, /* int option */
        { sysarg_int,  }, /* unsigned long arg2 */
        { sysarg_int,  }, /* unsigned long arg3 */
        { sysarg_int,  }, /* unsigned long arg4 */
        { sysarg_int,  }, /* unsigned long arg5 */
        { sysarg_end } } };
nsysarg_calls[__NR_prctl] = 5;

sysarg_calls[__NR_personality] = 
   (struct sysarg_call)
     { "personality", {
        { sysarg_int,  }, /* u_long personality */
        { sysarg_end } } };
nsysarg_calls[__NR_personality] = 1;

sysarg_calls[__NR_sched_setscheduler] = 
   (struct sysarg_call)
     { "sched_setscheduler", {
        { sysarg_int,  }, /* pid_t pid */
        { sysarg_int,  }, /* int policy */
        { sysarg_unknown,  }, /* struct sched_param __user * param */
        { sysarg_end } } };
nsysarg_calls[__NR_sched_setscheduler] = 3;

sysarg_calls[__NR_sched_setparam] = 
   (struct sysarg_call)
     { "sched_setparam", {
        { sysarg_int,  }, /* pid_t pid */
        { sysarg_unknown,  }, /* struct sched_param __user * param */
        { sysarg_end } } };
nsysarg_calls[__NR_sched_setparam] = 2;

sysarg_calls[__NR_sched_getscheduler] = 
   (struct sysarg_call)
     { "sched_getscheduler", {
        { sysarg_int,  }, /* pid_t pid */
        { sysarg_end } } };
nsysarg_calls[__NR_sched_getscheduler] = 1;

sysarg_calls[__NR_sched_getparam] = 
   (struct sysarg_call)
     { "sched_getparam", {
        { sysarg_int,  }, /* pid_t pid */
        { sysarg_unknown,  }, /* struct sched_param __user * param */
        { sysarg_end } } };
nsysarg_calls[__NR_sched_getparam] = 2;

sysarg_calls[__NR_sched_setaffinity] = 
   (struct sysarg_call)
     { "sched_setaffinity", {
        { sysarg_int,  }, /* pid_t pid */
        { sysarg_int,  }, /* unsigned int len */
        { sysarg_unknown,  }, /* unsigned long __user * user_mask_ptr */
        { sysarg_end } } };
nsysarg_calls[__NR_sched_setaffinity] = 3;

sysarg_calls[__NR_sched_getaffinity] = 
   (struct sysarg_call)
     { "sched_getaffinity", {
        { sysarg_int,  }, /* pid_t pid */
        { sysarg_int,  }, /* unsigned int len */
        { sysarg_unknown,  }, /* unsigned long __user * user_mask_ptr */
        { sysarg_end } } };
nsysarg_calls[__NR_sched_getaffinity] = 3;

sysarg_calls[__NR_sched_yield] = 
   (struct sysarg_call)
     { "sched_yield", {
        { sysarg_end } } };
nsysarg_calls[__NR_sched_yield] = 0;

sysarg_calls[__NR_sched_get_priority_max] = 
   (struct sysarg_call)
     { "sched_get_priority_max", {
        { sysarg_int,  }, /* int policy */
        { sysarg_end } } };
nsysarg_calls[__NR_sched_get_priority_max] = 1;

sysarg_calls[__NR_sched_get_priority_min] = 
   (struct sysarg_call)
     { "sched_get_priority_min", {
        { sysarg_int,  }, /* int policy */
        { sysarg_end } } };
nsysarg_calls[__NR_sched_get_priority_min] = 1;

sysarg_calls[__NR_sched_rr_get_interval] = 
   (struct sysarg_call)
     { "sched_rr_get_interval", {
        { sysarg_int,  }, /* pid_t pid */
        { sysarg_buf_fixlen, sizeof(struct timespec) }, /* struct timespec __user * interval */
        { sysarg_end } } };
nsysarg_calls[__NR_sched_rr_get_interval] = 2;

sysarg_calls[__NR_acct] = 
   (struct sysarg_call)
     { "acct", {
        { sysarg_strnull,  }, /* const char __user * name */
        { sysarg_end } } };
nsysarg_calls[__NR_acct] = 1;

sysarg_calls[__NR_getgroups] = 
   (struct sysarg_call)
     { "getgroups", {
        { sysarg_int,  }, /* int gidsetsize */
        { sysarg_ignore,  }, /* gid_t __user * grouplist */
        { sysarg_end } } };
nsysarg_calls[__NR_getgroups] = 2;

sysarg_calls[__NR_setgroups] = 
   (struct sysarg_call)
     { "setgroups", {
        { sysarg_int,  }, /* int gidsetsize */
        { sysarg_ignore,  }, /* gid_t __user * grouplist */
        { sysarg_end } } };
nsysarg_calls[__NR_setgroups] = 2;

sysarg_calls[__NR_time] = 
   (struct sysarg_call)
     { "time", {
        { sysarg_unknown,  }, /* time_t __user * tloc */
        { sysarg_end } } };
nsysarg_calls[__NR_time] = 1;

sysarg_calls[__NR_gettimeofday] = 
   (struct sysarg_call)
     { "gettimeofday", {
        { sysarg_buf_fixlen, sizeof(struct timeval) }, /* struct timeval __user * tv */
        { sysarg_unknown,  }, /* struct timezone __user * tz */
        { sysarg_end } } };
nsysarg_calls[__NR_gettimeofday] = 2;

sysarg_calls[__NR_settimeofday] = 
   (struct sysarg_call)
     { "settimeofday", {
        { sysarg_buf_fixlen, sizeof(struct timeval) }, /* struct timeval __user * tv */
        { sysarg_unknown,  }, /* struct timezone __user * tz */
        { sysarg_end } } };
nsysarg_calls[__NR_settimeofday] = 2;

sysarg_calls[__NR_adjtimex] = 
   (struct sysarg_call)
     { "adjtimex", {
        { sysarg_unknown,  }, /* struct timex __user * txc_p */
        { sysarg_end } } };
nsysarg_calls[__NR_adjtimex] = 1;

sysarg_calls[__NR_syslog] = 
   (struct sysarg_call)
     { "syslog", {
        { sysarg_int,  }, /* int type */
        { sysarg_unknown,  }, /* char __user * buf */
        { sysarg_int,  }, /* int len */
        { sysarg_end } } };
nsysarg_calls[__NR_syslog] = 3;

sysarg_calls[__NR_restart_syscall] = 
   (struct sysarg_call)
     { "restart_syscall", {
        { sysarg_end } } };
nsysarg_calls[__NR_restart_syscall] = 0;

sysarg_calls[__NR_rt_sigprocmask] = 
   (struct sysarg_call)
     { "rt_sigprocmask", {
        { sysarg_int,  }, /* int how */
        { sysarg_buf_arglen, 1, 3 }, /* sigset_t __user * set */
        { sysarg_buf_arglen, 1, 3 }, /* sigset_t __user * oset */
        { sysarg_int,  }, /* size_t sigsetsize */
        { sysarg_end } } };
nsysarg_calls[__NR_rt_sigprocmask] = 4;

sysarg_calls[__NR_rt_sigpending] = 
   (struct sysarg_call)
     { "rt_sigpending", {
        { sysarg_unknown,  }, /* sigset_t __user * set */
        { sysarg_int,  }, /* size_t sigsetsize */
        { sysarg_end } } };
nsysarg_calls[__NR_rt_sigpending] = 2;

sysarg_calls[__NR_rt_sigtimedwait] = 
   (struct sysarg_call)
     { "rt_sigtimedwait", {
        { sysarg_unknown,  }, /* const sigset_t __user * uthese */
        { sysarg_unknown,  }, /* siginfo_t __user * uinfo */
        { sysarg_buf_fixlen, sizeof(struct timespec) }, /* const struct timespec __user * uts */
        { sysarg_int,  }, /* size_t sigsetsize */
        { sysarg_end } } };
nsysarg_calls[__NR_rt_sigtimedwait] = 4;

sysarg_calls[__NR_rt_sigreturn] = 
   (struct sysarg_call)
     { "rt_sigreturn", {
        { sysarg_int }, /* unsigned long unused */
        { sysarg_end } } };
nsysarg_calls[__NR_rt_sigreturn] = 1;

sysarg_calls[__NR_kill] = 
   (struct sysarg_call)
     { "kill", {
        { sysarg_int,  }, /* pid_t pid */
        { sysarg_int,  }, /* int sig */
        { sysarg_end } } };
nsysarg_calls[__NR_kill] = 2;

sysarg_calls[__NR_tgkill] = 
   (struct sysarg_call)
     { "tgkill", {
        { sysarg_int,  }, /* pid_t tgid */
        { sysarg_int,  }, /* pid_t pid */
        { sysarg_int,  }, /* int sig */
        { sysarg_end } } };
nsysarg_calls[__NR_tgkill] = 3;

sysarg_calls[__NR_tkill] = 
   (struct sysarg_call)
     { "tkill", {
        { sysarg_int,  }, /* pid_t pid */
        { sysarg_int,  }, /* int sig */
        { sysarg_end } } };
nsysarg_calls[__NR_tkill] = 2;

sysarg_calls[__NR_rt_sigqueueinfo] = 
   (struct sysarg_call)
     { "rt_sigqueueinfo", {
        { sysarg_int,  }, /* pid_t pid */
        { sysarg_int,  }, /* int sig */
        { sysarg_unknown,  }, /* siginfo_t __user * uinfo */
        { sysarg_end } } };
nsysarg_calls[__NR_rt_sigqueueinfo] = 3;

sysarg_calls[__NR_rt_tgsigqueueinfo] = 
   (struct sysarg_call)
     { "rt_tgsigqueueinfo", {
        { sysarg_int,  }, /* pid_t tgid */
        { sysarg_int,  }, /* pid_t pid */
        { sysarg_int,  }, /* int sig */
        { sysarg_unknown,  }, /* siginfo_t __user * uinfo */
        { sysarg_end } } };
nsysarg_calls[__NR_rt_tgsigqueueinfo] = 4;

sysarg_calls[__NR_rt_sigaction] = 
   (struct sysarg_call)
     { "rt_sigaction", {
        { sysarg_int,  }, /* int sig */
        { sysarg_buf_fixlen, sizeof(struct sigaction) }, /* const struct sigaction __user * act */
        { sysarg_buf_fixlen, sizeof(struct sigaction) }, /* struct sigaction __user * oact */
        { sysarg_int,  }, /* size_t sigsetsize */
        { sysarg_end } } };
nsysarg_calls[__NR_rt_sigaction] = 4;

sysarg_calls[__NR_pause] = 
   (struct sysarg_call)
     { "pause", {
        { sysarg_end } } };
nsysarg_calls[__NR_pause] = 0;

sysarg_calls[__NR_rt_sigsuspend] = 
   (struct sysarg_call)
     { "rt_sigsuspend", {
        { sysarg_unknown,  }, /* sigset_t __user * unewset */
        { sysarg_int,  }, /* size_t sigsetsize */
        { sysarg_end } } };
nsysarg_calls[__NR_rt_sigsuspend] = 2;

sysarg_calls[__NR_socket] = 
   (struct sysarg_call)
     { "socket", {
        { sysarg_int,  }, /* int family */
        { sysarg_int,  }, /* int type */
        { sysarg_int,  }, /* int protocol */
        { sysarg_end } } };
nsysarg_calls[__NR_socket] = 3;

sysarg_calls[__NR_socketpair] = 
   (struct sysarg_call)
     { "socketpair", {
        { sysarg_int,  }, /* int family */
        { sysarg_int,  }, /* int type */
        { sysarg_int,  }, /* int protocol */
        { sysarg_ignore,  }, /* int __user * usockvec */
        { sysarg_end } } };
nsysarg_calls[__NR_socketpair] = 4;

sysarg_calls[__NR_bind] = 
   (struct sysarg_call)
     { "bind", {
        { sysarg_int,  }, /* int fd */
        { sysarg_buf_arglen, 1, 2 }, /* struct sockaddr __user * umyaddr */
        { sysarg_int,  }, /* int addrlen */
        { sysarg_end } } };
nsysarg_calls[__NR_bind] = 3;

sysarg_calls[__NR_listen] = 
   (struct sysarg_call)
     { "listen", {
        { sysarg_int,  }, /* int fd */
        { sysarg_int,  }, /* int backlog */
        { sysarg_end } } };
nsysarg_calls[__NR_listen] = 2;

sysarg_calls[__NR_accept4] = 
   (struct sysarg_call)
     { "accept4", {
        { sysarg_int,  }, /* int fd */
        { sysarg_buf_arglenp, 1, 2 }, /* struct sockaddr __user * upeer_sockaddr */
        { sysarg_buf_fixlen, sizeof(int) }, /* int __user * upeer_addrlen */
        { sysarg_int,  }, /* int flags */
        { sysarg_end } } };
nsysarg_calls[__NR_accept4] = 4;

sysarg_calls[__NR_accept] = 
   (struct sysarg_call)
     { "accept", {
        { sysarg_int,  }, /* int fd */
        { sysarg_buf_arglenp, 1, 2 }, /* struct sockaddr __user * upeer_sockaddr */
        { sysarg_buf_fixlen, sizeof(int) }, /* int __user * upeer_addrlen */
        { sysarg_end } } };
nsysarg_calls[__NR_accept] = 3;

sysarg_calls[__NR_connect] = 
   (struct sysarg_call)
     { "connect", {
        { sysarg_int,  }, /* int fd */
        { sysarg_buf_arglen, 1, 2 }, /* struct sockaddr __user * uservaddr */
        { sysarg_int,  }, /* int addrlen */
        { sysarg_end } } };
nsysarg_calls[__NR_connect] = 3;

sysarg_calls[__NR_getsockname] = 
   (struct sysarg_call)
     { "getsockname", {
        { sysarg_int,  }, /* int fd */
        { sysarg_buf_arglenp, 1, 2 }, /* struct sockaddr __user * usockaddr */
        { sysarg_buf_fixlen, sizeof(int) }, /* int __user * usockaddr_len */
        { sysarg_end } } };
nsysarg_calls[__NR_getsockname] = 3;

sysarg_calls[__NR_getpeername] = 
   (struct sysarg_call)
     { "getpeername", {
        { sysarg_int,  }, /* int fd */
        { sysarg_buf_arglenp, 1, 2 }, /* struct sockaddr __user * usockaddr */
        { sysarg_buf_fixlen, sizeof(int) }, /* int __user * usockaddr_len */
        { sysarg_end } } };
nsysarg_calls[__NR_getpeername] = 3;

sysarg_calls[__NR_sendto] = 
   (struct sysarg_call)
     { "sendto", {
        { sysarg_int,  }, /* int fd */
        { sysarg_buf_arglen, 1, 2 }, /* void __user * buff */
        { sysarg_int,  }, /* size_t len */
        { sysarg_int,  }, /* unsigned flags */
        { sysarg_buf_arglen, 1, 5 }, /* struct sockaddr __user * addr */
        { sysarg_int,  }, /* int addr_len */
        { sysarg_end } } };
nsysarg_calls[__NR_sendto] = 6;

sysarg_calls[__NR_recvfrom] = 
   (struct sysarg_call)
     { "recvfrom", {
        { sysarg_int,  }, /* int fd */
        { sysarg_buf_arglen, 1, 2 }, /* void __user * ubuf */
        { sysarg_int,  }, /* size_t size */
        { sysarg_int,  }, /* unsigned flags */
        { sysarg_buf_arglenp, 1, 5 }, /* struct sockaddr __user * addr */
        { sysarg_buf_fixlen, sizeof(int) }, /* int __user * addr_len */
        { sysarg_end } } };
nsysarg_calls[__NR_recvfrom] = 6;

sysarg_calls[__NR_setsockopt] = 
   (struct sysarg_call)
     { "setsockopt", {
        { sysarg_int,  }, /* int fd */
        { sysarg_int,  }, /* int level */
        { sysarg_int,  }, /* int optname */
        { sysarg_buf_arglen, 1, 4 }, /* char __user * optval */
        { sysarg_int,  }, /* int optlen */
        { sysarg_end } } };
nsysarg_calls[__NR_setsockopt] = 5;

sysarg_calls[__NR_getsockopt] = 
   (struct sysarg_call)
     { "getsockopt", {
        { sysarg_int,  }, /* int fd */
        { sysarg_int,  }, /* int level */
        { sysarg_int,  }, /* int optname */
        { sysarg_buf_arglenp, 1, 4 }, /* char __user * optval */
        { sysarg_buf_fixlen, sizeof(int) }, /* int __user * optlen */
        { sysarg_end } } };
nsysarg_calls[__NR_getsockopt] = 5;

sysarg_calls[__NR_shutdown] = 
   (struct sysarg_call)
     { "shutdown", {
        { sysarg_int,  }, /* int fd */
        { sysarg_int,  }, /* int how */
        { sysarg_end } } };
nsysarg_calls[__NR_shutdown] = 2;

sysarg_calls[__NR_sendmsg] = 
   (struct sysarg_call)
     { "sendmsg", {
        { sysarg_int,  }, /* int fd */
        { sysarg_unknown,  }, /* struct msghdr __user * msg */
        { sysarg_int,  }, /* unsigned flags */
        { sysarg_end } } };
nsysarg_calls[__NR_sendmsg] = 3;

sysarg_calls[__NR_recvmsg] = 
   (struct sysarg_call)
     { "recvmsg", {
        { sysarg_int,  }, /* int fd */
        { sysarg_unknown,  }, /* struct msghdr __user * msg */
        { sysarg_int,  }, /* unsigned int flags */
        { sysarg_end } } };
nsysarg_calls[__NR_recvmsg] = 3;

sysarg_calls[__NR_add_key] = 
   (struct sysarg_call)
     { "add_key", {
        { sysarg_unknown,  }, /* const char __user * _type */
        { sysarg_unknown,  }, /* const char __user * _description */
        { sysarg_unknown,  }, /* const void __user * _payload */
        { sysarg_int,  }, /* size_t plen */
        { sysarg_int,  }, /* key_serial_t ringid */
        { sysarg_end } } };
nsysarg_calls[__NR_add_key] = 5;

sysarg_calls[__NR_request_key] = 
   (struct sysarg_call)
     { "request_key", {
        { sysarg_unknown,  }, /* const char __user * _type */
        { sysarg_unknown,  }, /* const char __user * _description */
        { sysarg_unknown,  }, /* const char __user * _callout_info */
        { sysarg_int,  }, /* key_serial_t destringid */
        { sysarg_end } } };
nsysarg_calls[__NR_request_key] = 4;

sysarg_calls[__NR_keyctl] = 
   (struct sysarg_call)
     { "keyctl", {
        { sysarg_int,  }, /* int option */
        { sysarg_int,  }, /* unsigned long arg2 */
        { sysarg_int,  }, /* unsigned long arg3 */
        { sysarg_int,  }, /* unsigned long arg4 */
        { sysarg_int,  }, /* unsigned long arg5 */
        { sysarg_end } } };
nsysarg_calls[__NR_keyctl] = 5;

sysarg_calls[__NR_epoll_create1] = 
   (struct sysarg_call)
     { "epoll_create1", {
        { sysarg_int,  }, /* int flags */
        { sysarg_end } } };
nsysarg_calls[__NR_epoll_create1] = 1;

sysarg_calls[__NR_epoll_create] = 
   (struct sysarg_call)
     { "epoll_create", {
        { sysarg_int,  }, /* int size */
        { sysarg_end } } };
nsysarg_calls[__NR_epoll_create] = 1;

sysarg_calls[__NR_epoll_ctl] = 
   (struct sysarg_call)
     { "epoll_ctl", {
        { sysarg_int,  }, /* int epfd */
        { sysarg_int,  }, /* int op */
        { sysarg_int,  }, /* int fd */
        { sysarg_unknown,  }, /* struct epoll_event __user * event */
        { sysarg_end } } };
nsysarg_calls[__NR_epoll_ctl] = 4;

sysarg_calls[__NR_epoll_wait] = 
   (struct sysarg_call)
     { "epoll_wait", {
        { sysarg_int,  }, /* int epfd */
        { sysarg_unknown,  }, /* struct epoll_event __user * events */
        { sysarg_int,  }, /* int maxevents */
        { sysarg_int,  }, /* int timeout */
        { sysarg_end } } };
nsysarg_calls[__NR_epoll_wait] = 4;

sysarg_calls[__NR_epoll_pwait] = 
   (struct sysarg_call)
     { "epoll_pwait", {
        { sysarg_int,  }, /* int epfd */
        { sysarg_unknown,  }, /* struct epoll_event __user * events */
        { sysarg_int,  }, /* int maxevents */
        { sysarg_int,  }, /* int timeout */
        { sysarg_unknown,  }, /* const sigset_t __user * sigmask */
        { sysarg_int,  }, /* size_t sigsetsize */
        { sysarg_end } } };
nsysarg_calls[__NR_epoll_pwait] = 6;

sysarg_calls[__NR_lookup_dcookie] = 
   (struct sysarg_call)
     { "lookup_dcookie", {
        { sysarg_int,  }, /* u64 cookie64 */
        { sysarg_unknown,  }, /* char __user * buf */
        { sysarg_int,  }, /* size_t len */
        { sysarg_end } } };
nsysarg_calls[__NR_lookup_dcookie] = 3;

sysarg_calls[__NR_flock] = 
   (struct sysarg_call)
     { "flock", {
        { sysarg_int,  }, /* unsigned int fd */
        { sysarg_int,  }, /* unsigned int cmd */
        { sysarg_end } } };
nsysarg_calls[__NR_flock] = 2;

sysarg_calls[__NR_getdents] = 
   (struct sysarg_call)
     { "getdents", {
        { sysarg_int,  }, /* unsigned int fd */
        { sysarg_buf_arglen, 1, 2 }, /* struct linux_dirent __user * dirent */
        { sysarg_int,  }, /* unsigned int count */
        { sysarg_end } } };
nsysarg_calls[__NR_getdents] = 3;

sysarg_calls[__NR_getdents64] = 
   (struct sysarg_call)
     { "getdents64", {
        { sysarg_int,  }, /* unsigned int fd */
        { sysarg_buf_arglen, 1, 2 }, /* struct linux_dirent64 __user * dirent */
        { sysarg_int,  }, /* unsigned int count */
        { sysarg_end } } };
nsysarg_calls[__NR_getdents64] = 3;

sysarg_calls[__NR_utime] = 
   (struct sysarg_call)
     { "utime", {
        { sysarg_strnull,  }, /* char __user * filename */
        { sysarg_ignore,  }, /* struct utimbuf __user * times */
        { sysarg_end } } };
nsysarg_calls[__NR_utime] = 2;

sysarg_calls[__NR_utimensat] = 
   (struct sysarg_call)
     { "utimensat", {
        { sysarg_int,  }, /* int dfd */
        { sysarg_strnull,  }, /* char __user * filename */
        { sysarg_ignore,  }, /* struct timespec __user * utimes */
        { sysarg_int,  }, /* int flags */
        { sysarg_end } } };
nsysarg_calls[__NR_utimensat] = 4;

sysarg_calls[__NR_futimesat] = 
   (struct sysarg_call)
     { "futimesat", {
        { sysarg_int,  }, /* int dfd */
        { sysarg_strnull,  }, /* char __user * filename */
        { sysarg_ignore,  }, /* struct timeval __user * utimes */
        { sysarg_end } } };
nsysarg_calls[__NR_futimesat] = 3;

sysarg_calls[__NR_utimes] = 
   (struct sysarg_call)
     { "utimes", {
        { sysarg_strnull,  }, /* char __user * filename */
        { sysarg_unknown,  }, /* struct timeval __user * utimes */
        { sysarg_end } } };
nsysarg_calls[__NR_utimes] = 2;

sysarg_calls[__NR_lseek] = 
   (struct sysarg_call)
     { "lseek", {
        { sysarg_int,  }, /* unsigned int fd */
        { sysarg_int,  }, /* off_t offset */
        { sysarg_int,  }, /* unsigned int origin */
        { sysarg_end } } };
nsysarg_calls[__NR_lseek] = 3;

sysarg_calls[__NR_read] = 
   (struct sysarg_call)
     { "read", {
        { sysarg_int,  }, /* unsigned int fd */
        { sysarg_ignore, 1, 2 }, /* char __user * buf */
        { sysarg_int,  }, /* size_t count */
        { sysarg_end } } };
nsysarg_calls[__NR_read] = 3;

sysarg_calls[__NR_write] = 
   (struct sysarg_call)
     { "write", {
        { sysarg_int,  }, /* unsigned int fd */
        { sysarg_buf_arglen_when_entering, 1, 2 }, /* const char __user * buf */
        { sysarg_int,  }, /* size_t count */
        { sysarg_end } } };
nsysarg_calls[__NR_write] = 3;

sysarg_calls[__NR_pread64] = 
   (struct sysarg_call)
     { "pread64", {
        { sysarg_int,  }, /* unsigned int fd */
        { sysarg_buf_arglen_when_exiting, 1, 2 }, /* char __user * buf */
        { sysarg_int,  }, /* size_t count */
        { sysarg_int,  }, /* loff_t pos */
        { sysarg_end } } };
nsysarg_calls[__NR_pread64] = 4;

sysarg_calls[__NR_pwrite64] = 
   (struct sysarg_call)
     { "pwrite64", {
        { sysarg_int,  }, /* unsigned int fd */
        { sysarg_buf_arglen_when_entering, 1, 2 }, /* const char __user * buf */
        { sysarg_int,  }, /* size_t count */
        { sysarg_int,  }, /* loff_t pos */
        { sysarg_end } } };
nsysarg_calls[__NR_pwrite64] = 4;

sysarg_calls[__NR_readv] = 
   (struct sysarg_call)
     { "readv", {
        { sysarg_int,  }, /* unsigned long fd */
        { sysarg_unknown,  }, /* const struct iovec __user * vec */
        { sysarg_int,  }, /* unsigned long vlen */
        { sysarg_end } } };
nsysarg_calls[__NR_readv] = 3;

sysarg_calls[__NR_writev] = 
   (struct sysarg_call)
     { "writev", {
        { sysarg_int,  }, /* unsigned long fd */
        { sysarg_unknown,  }, /* const struct iovec __user * vec */
        { sysarg_int,  }, /* unsigned long vlen */
        { sysarg_end } } };
nsysarg_calls[__NR_writev] = 3;

sysarg_calls[__NR_preadv] = 
   (struct sysarg_call)
     { "preadv", {
        { sysarg_int,  }, /* unsigned long fd */
        { sysarg_unknown,  }, /* const struct iovec __user * vec */
        { sysarg_int,  }, /* unsigned long vlen */
        { sysarg_int,  }, /* unsigned long pos_l */
        { sysarg_int,  }, /* unsigned long pos_h */
        { sysarg_end } } };
nsysarg_calls[__NR_preadv] = 5;

sysarg_calls[__NR_pwritev] = 
   (struct sysarg_call)
     { "pwritev", {
        { sysarg_int,  }, /* unsigned long fd */
        { sysarg_unknown,  }, /* const struct iovec __user * vec */
        { sysarg_int,  }, /* unsigned long vlen */
        { sysarg_int,  }, /* unsigned long pos_l */
        { sysarg_int,  }, /* unsigned long pos_h */
        { sysarg_end } } };
nsysarg_calls[__NR_pwritev] = 5;

sysarg_calls[__NR_sendfile] = 
   (struct sysarg_call)
     { "sendfile", {
        { sysarg_int,  }, /* int out_fd */
        { sysarg_int,  }, /* int in_fd */
        { sysarg_buf_fixlen, sizeof(off_t) }, /* off_t __user * offset */
        { sysarg_int,  }, /* size_t count */
        { sysarg_end } } };
nsysarg_calls[__NR_sendfile] = 4;

sysarg_calls[__NR_timerfd_create] = 
   (struct sysarg_call)
     { "timerfd_create", {
        { sysarg_int,  }, /* int clockid */
        { sysarg_int,  }, /* int flags */
        { sysarg_end } } };
nsysarg_calls[__NR_timerfd_create] = 2;

sysarg_calls[__NR_timerfd_settime] = 
   (struct sysarg_call)
     { "timerfd_settime", {
        { sysarg_int,  }, /* int ufd */
        { sysarg_int,  }, /* int flags */
        { sysarg_buf_fixlen, sizeof(struct itimerspec) }, /* const struct itimerspec __user * utmr */
        { sysarg_buf_fixlen, sizeof(struct itimerspec) }, /* struct itimerspec __user * otmr */
        { sysarg_end } } };
nsysarg_calls[__NR_timerfd_settime] = 4;

sysarg_calls[__NR_timerfd_gettime] = 
   (struct sysarg_call)
     { "timerfd_gettime", {
        { sysarg_int,  }, /* int ufd */
        { sysarg_buf_fixlen, sizeof(struct itimerspec) }, /* struct itimerspec __user * otmr */
        { sysarg_end } } };
nsysarg_calls[__NR_timerfd_gettime] = 2;

sysarg_calls[__NR_dup3] = 
   (struct sysarg_call)
     { "dup3", {
        { sysarg_int,  }, /* unsigned int oldfd */
        { sysarg_int,  }, /* unsigned int newfd */
        { sysarg_int,  }, /* int flags */
        { sysarg_end } } };
nsysarg_calls[__NR_dup3] = 3;

sysarg_calls[__NR_dup2] = 
   (struct sysarg_call)
     { "dup2", {
        { sysarg_int,  }, /* unsigned int oldfd */
        { sysarg_int,  }, /* unsigned int newfd */
        { sysarg_end } } };
nsysarg_calls[__NR_dup2] = 2;

sysarg_calls[__NR_dup] = 
   (struct sysarg_call)
     { "dup", {
        { sysarg_int,  }, /* unsigned int fildes */
        { sysarg_end } } };
nsysarg_calls[__NR_dup] = 1;

sysarg_calls[__NR_fcntl] = 
   (struct sysarg_call)
     { "fcntl", {
        { sysarg_int,  }, /* unsigned int fd */
        { sysarg_int,  }, /* unsigned int cmd */
        { sysarg_int,  }, /* unsigned long arg */
        { sysarg_end } } };
nsysarg_calls[__NR_fcntl] = 3;

sysarg_calls[__NR_sync] = 
   (struct sysarg_call)
     { "sync", {
        { sysarg_end } } };
nsysarg_calls[__NR_sync] = 0;

sysarg_calls[__NR_fsync] = 
   (struct sysarg_call)
     { "fsync", {
        { sysarg_int,  }, /* unsigned int fd */
        { sysarg_end } } };
nsysarg_calls[__NR_fsync] = 1;

sysarg_calls[__NR_fdatasync] = 
   (struct sysarg_call)
     { "fdatasync", {
        { sysarg_int,  }, /* unsigned int fd */
        { sysarg_end } } };
nsysarg_calls[__NR_fdatasync] = 1;

sysarg_calls[__NR_sync_file_range] = 
   (struct sysarg_call)
     { "sync_file_range", {
        { sysarg_int,  }, /* int fd */
        { sysarg_int,  }, /* loff_t offset */
        { sysarg_int,  }, /* loff_t nbytes */
        { sysarg_int,  }, /* unsigned int flags */
        { sysarg_end } } };
nsysarg_calls[__NR_sync_file_range] = 4;

sysarg_calls[__NR_sysfs] = 
   (struct sysarg_call)
     { "sysfs", {
        { sysarg_int,  }, /* int option */
        { sysarg_int,  }, /* unsigned long arg1 */
        { sysarg_int,  }, /* unsigned long arg2 */
        { sysarg_end } } };
nsysarg_calls[__NR_sysfs] = 3;

sysarg_calls[__NR_nfsservctl] = 
   (struct sysarg_call)
     { "nfsservctl", {
        { sysarg_int,  }, /* int cmd */
        { sysarg_unknown,  }, /* struct nfsctl_arg __user * arg */
        { sysarg_unknown,  }, /* void __user * res */
        { sysarg_end } } };
nsysarg_calls[__NR_nfsservctl] = 3;

sysarg_calls[__NR_ioctl] = 
   (struct sysarg_call)
     { "ioctl", {
        { sysarg_int,  }, /* unsigned int fd */
        { sysarg_int,  }, /* unsigned int cmd */
        { sysarg_int,  }, /* unsigned long arg */
        { sysarg_end } } };
nsysarg_calls[__NR_ioctl] = 3;

sysarg_calls[__NR_getcwd] = 
   (struct sysarg_call)
     { "getcwd", {
        { sysarg_buf_arglen, 1, 1 }, /* char __user * buf */
        { sysarg_int,  }, /* unsigned long size */
        { sysarg_end } } };
nsysarg_calls[__NR_getcwd] = 2;

sysarg_calls[__NR_stat] = 
   (struct sysarg_call)
     { "stat", {
        { sysarg_strnull,  }, /* char __user * filename */
        { sysarg_buf_fixlen, sizeof(struct __old_kernel_stat) }, /* struct __old_kernel_stat __user * statbuf */
        { sysarg_end } } };
nsysarg_calls[__NR_stat] = 2;

sysarg_calls[__NR_lstat] = 
   (struct sysarg_call)
     { "lstat", {
        { sysarg_strnull,  }, /* char __user * filename */
        { sysarg_buf_fixlen, sizeof(struct __old_kernel_stat) }, /* struct __old_kernel_stat __user * statbuf */
        { sysarg_end } } };
nsysarg_calls[__NR_lstat] = 2;

sysarg_calls[__NR_fstat] = 
   (struct sysarg_call)
     { "fstat", {
        { sysarg_int,  }, /* unsigned int fd */
        { sysarg_buf_fixlen, sizeof(struct __old_kernel_stat) }, /* struct __old_kernel_stat __user * statbuf */
        { sysarg_end } } };
nsysarg_calls[__NR_fstat] = 2;

sysarg_calls[__NR_newfstatat] = 
   (struct sysarg_call)
     { "newfstatat", {
        { sysarg_int,  }, /* int dfd */
        { sysarg_strnull,  }, /* char __user * filename */
        { sysarg_buf_fixlen, sizeof(struct stat) }, /* struct stat __user * statbuf */
        { sysarg_int,  }, /* int flag */
        { sysarg_end } } };
nsysarg_calls[__NR_newfstatat] = 4;

sysarg_calls[__NR_readlinkat] = 
   (struct sysarg_call)
     { "readlinkat", {
        { sysarg_int,  }, /* int dfd */
        { sysarg_strnull,  }, /* const char __user * pathname */
        { sysarg_buf_arglen, 1, 3 }, /* char __user * buf */
        { sysarg_int,  }, /* int bufsiz */
        { sysarg_end } } };
nsysarg_calls[__NR_readlinkat] = 4;

sysarg_calls[__NR_readlink] = 
   (struct sysarg_call)
     { "readlink", {
        { sysarg_strnull,  }, /* const char __user * path */
        { sysarg_buf_arglen, 1, 2 }, /* char __user * buf */
        { sysarg_int,  }, /* int bufsiz */
        { sysarg_end } } };
nsysarg_calls[__NR_readlink] = 3;

sysarg_calls[__NR_mount] = 
   (struct sysarg_call)
     { "mount", {
        { sysarg_strnull,  }, /* char __user * dev_name */
        { sysarg_strnull,  }, /* char __user * dir_name */
        { sysarg_unknown,  }, /* char __user * type */
        { sysarg_int,  }, /* unsigned long flags */
        { sysarg_unknown,  }, /* void __user * data */
        { sysarg_end } } };
nsysarg_calls[__NR_mount] = 5;

sysarg_calls[__NR_pivot_root] = 
   (struct sysarg_call)
     { "pivot_root", {
        { sysarg_unknown,  }, /* const char __user * new_root */
        { sysarg_unknown,  }, /* const char __user * put_old */
        { sysarg_end } } };
nsysarg_calls[__NR_pivot_root] = 2;

sysarg_calls[__NR_inotify_init1] = 
   (struct sysarg_call)
     { "inotify_init1", {
        { sysarg_int,  }, /* int flags */
        { sysarg_end } } };
nsysarg_calls[__NR_inotify_init1] = 1;

sysarg_calls[__NR_inotify_init] = 
   (struct sysarg_call)
     { "inotify_init", {
        { sysarg_end } } };
nsysarg_calls[__NR_inotify_init] = 0;

sysarg_calls[__NR_inotify_add_watch] = 
   (struct sysarg_call)
     { "inotify_add_watch", {
        { sysarg_int,  }, /* int fd */
        { sysarg_strnull,  }, /* const char __user * pathname */
        { sysarg_int,  }, /* u32 mask */
        { sysarg_end } } };
nsysarg_calls[__NR_inotify_add_watch] = 3;

sysarg_calls[__NR_inotify_rm_watch] = 
   (struct sysarg_call)
     { "inotify_rm_watch", {
        { sysarg_int,  }, /* int fd */
        { sysarg_int,  }, /* __s32 wd */
        { sysarg_end } } };
nsysarg_calls[__NR_inotify_rm_watch] = 2;

sysarg_calls[__NR_statfs] = 
   (struct sysarg_call)
     { "statfs", {
        { sysarg_strnull,  }, /* const char __user * pathname */
        { sysarg_buf_fixlen, sizeof(struct statfs) }, /* struct statfs __user * buf */
        { sysarg_end } } };
nsysarg_calls[__NR_statfs] = 2;

sysarg_calls[__NR_fstatfs] = 
   (struct sysarg_call)
     { "fstatfs", {
        { sysarg_int,  }, /* unsigned int fd */
        { sysarg_unknown,  }, /* struct statfs __user * buf */
        { sysarg_end } } };
nsysarg_calls[__NR_fstatfs] = 2;

sysarg_calls[__NR_truncate] = 
   (struct sysarg_call)
     { "truncate", {
        { sysarg_strnull,  }, /* const char __user * path */
        { sysarg_int,  }, /* long length */
        { sysarg_end } } };
nsysarg_calls[__NR_truncate] = 2;

sysarg_calls[__NR_ftruncate] = 
   (struct sysarg_call)
     { "ftruncate", {
        { sysarg_int,  }, /* unsigned int fd */
        { sysarg_int,  }, /* unsigned long length */
        { sysarg_end } } };
nsysarg_calls[__NR_ftruncate] = 2;

sysarg_calls[__NR_fallocate] = 
   (struct sysarg_call)
     { "fallocate", {
        { sysarg_int,  }, /* int fd */
        { sysarg_int,  }, /* int mode */
        { sysarg_int,  }, /* loff_t offset */
        { sysarg_int,  }, /* loff_t len */
        { sysarg_end } } };
nsysarg_calls[__NR_fallocate] = 4;

sysarg_calls[__NR_faccessat] = 
   (struct sysarg_call)
     { "faccessat", {
        { sysarg_int,  }, /* int dfd */
        { sysarg_strnull,  }, /* const char __user * filename */
        { sysarg_int,  }, /* int mode */
        { sysarg_end } } };
nsysarg_calls[__NR_faccessat] = 3;

sysarg_calls[__NR_access] = 
   (struct sysarg_call)
     { "access", {
        { sysarg_strnull,  }, /* const char __user * filename */
        { sysarg_int,  }, /* int mode */
        { sysarg_end } } };
nsysarg_calls[__NR_access] = 2;

sysarg_calls[__NR_chdir] = 
   (struct sysarg_call)
     { "chdir", {
        { sysarg_strnull,  }, /* const char __user * filename */
        { sysarg_end } } };
nsysarg_calls[__NR_chdir] = 1;

sysarg_calls[__NR_fchdir] = 
   (struct sysarg_call)
     { "fchdir", {
        { sysarg_int,  }, /* unsigned int fd */
        { sysarg_end } } };
nsysarg_calls[__NR_fchdir] = 1;

sysarg_calls[__NR_chroot] = 
   (struct sysarg_call)
     { "chroot", {
        { sysarg_strnull,  }, /* const char __user * filename */
        { sysarg_end } } };
nsysarg_calls[__NR_chroot] = 1;

sysarg_calls[__NR_fchmod] = 
   (struct sysarg_call)
     { "fchmod", {
        { sysarg_int,  }, /* unsigned int fd */
        { sysarg_int,  }, /* mode_t mode */
        { sysarg_end } } };
nsysarg_calls[__NR_fchmod] = 2;

sysarg_calls[__NR_fchmodat] = 
   (struct sysarg_call)
     { "fchmodat", {
        { sysarg_int,  }, /* int dfd */
        { sysarg_strnull,  }, /* const char __user * filename */
        { sysarg_int,  }, /* mode_t mode */
        { sysarg_end } } };
nsysarg_calls[__NR_fchmodat] = 3;

sysarg_calls[__NR_chmod] = 
   (struct sysarg_call)
     { "chmod", {
        { sysarg_strnull,  }, /* const char __user * filename */
        { sysarg_int,  }, /* mode_t mode */
        { sysarg_end } } };
nsysarg_calls[__NR_chmod] = 2;

sysarg_calls[__NR_chown] = 
   (struct sysarg_call)
     { "chown", {
        { sysarg_strnull,  }, /* const char __user * filename */
        { sysarg_int,  }, /* uid_t user */
        { sysarg_int,  }, /* gid_t group */
        { sysarg_end } } };
nsysarg_calls[__NR_chown] = 3;

sysarg_calls[__NR_fchownat] = 
   (struct sysarg_call)
     { "fchownat", {
        { sysarg_int,  }, /* int dfd */
        { sysarg_strnull,  }, /* const char __user * filename */
        { sysarg_int,  }, /* uid_t user */
        { sysarg_int,  }, /* gid_t group */
        { sysarg_int,  }, /* int flag */
        { sysarg_end } } };
nsysarg_calls[__NR_fchownat] = 5;

sysarg_calls[__NR_lchown] = 
   (struct sysarg_call)
     { "lchown", {
        { sysarg_strnull,  }, /* const char __user * filename */
        { sysarg_int,  }, /* uid_t user */
        { sysarg_int,  }, /* gid_t group */
        { sysarg_end } } };
nsysarg_calls[__NR_lchown] = 3;

sysarg_calls[__NR_fchown] = 
   (struct sysarg_call)
     { "fchown", {
        { sysarg_int,  }, /* unsigned int fd */
        { sysarg_int,  }, /* uid_t user */
        { sysarg_int,  }, /* gid_t group */
        { sysarg_end } } };
nsysarg_calls[__NR_fchown] = 3;

sysarg_calls[__NR_open] = 
   (struct sysarg_call)
     { "open", {
        { sysarg_strnull,  }, /* const char __user * filename */
        { sysarg_int,  }, /* int flags */
        { sysarg_int,  }, /* int mode */
        { sysarg_end } } };
nsysarg_calls[__NR_open] = 3;

sysarg_calls[__NR_openat] = 
   (struct sysarg_call)
     { "openat", {
        { sysarg_int,  }, /* int dfd */
        { sysarg_strnull,  }, /* const char __user * filename */
        { sysarg_int,  }, /* int flags */
        { sysarg_int,  }, /* int mode */
        { sysarg_end } } };
nsysarg_calls[__NR_openat] = 4;

sysarg_calls[__NR_creat] = 
   (struct sysarg_call)
     { "creat", {
        { sysarg_strnull,  }, /* const char __user * pathname */
        { sysarg_int,  }, /* int mode */
        { sysarg_end } } };
nsysarg_calls[__NR_creat] = 2;

sysarg_calls[__NR_close] = 
   (struct sysarg_call)
     { "close", {
        { sysarg_int,  }, /* unsigned int fd */
        { sysarg_end } } };
nsysarg_calls[__NR_close] = 1;

sysarg_calls[__NR_vhangup] = 
   (struct sysarg_call)
     { "vhangup", {
        { sysarg_end } } };
nsysarg_calls[__NR_vhangup] = 0;

sysarg_calls[__NR_mknodat] = 
   (struct sysarg_call)
     { "mknodat", {
        { sysarg_int,  }, /* int dfd */
        { sysarg_strnull,  }, /* const char __user * filename */
        { sysarg_int,  }, /* int mode */
        { sysarg_int,  }, /* unsigned dev */
        { sysarg_end } } };
nsysarg_calls[__NR_mknodat] = 4;

sysarg_calls[__NR_mknod] = 
   (struct sysarg_call)
     { "mknod", {
        { sysarg_strnull,  }, /* const char __user * filename */
        { sysarg_int,  }, /* int mode */
        { sysarg_int,  }, /* unsigned dev */
        { sysarg_end } } };
nsysarg_calls[__NR_mknod] = 3;

sysarg_calls[__NR_mkdirat] = 
   (struct sysarg_call)
     { "mkdirat", {
        { sysarg_int,  }, /* int dfd */
        { sysarg_strnull,  }, /* const char __user * pathname */
        { sysarg_int,  }, /* int mode */
        { sysarg_end } } };
nsysarg_calls[__NR_mkdirat] = 3;

sysarg_calls[__NR_mkdir] = 
   (struct sysarg_call)
     { "mkdir", {
        { sysarg_strnull,  }, /* const char __user * pathname */
        { sysarg_int,  }, /* int mode */
        { sysarg_end } } };
nsysarg_calls[__NR_mkdir] = 2;

sysarg_calls[__NR_rmdir] = 
   (struct sysarg_call)
     { "rmdir", {
        { sysarg_strnull,  }, /* const char __user * pathname */
        { sysarg_end } } };
nsysarg_calls[__NR_rmdir] = 1;

sysarg_calls[__NR_unlinkat] = 
   (struct sysarg_call)
     { "unlinkat", {
        { sysarg_int,  }, /* int dfd */
        { sysarg_strnull,  }, /* const char __user * pathname */
        { sysarg_int,  }, /* int flag */
        { sysarg_end } } };
nsysarg_calls[__NR_unlinkat] = 3;

sysarg_calls[__NR_unlink] = 
   (struct sysarg_call)
     { "unlink", {
        { sysarg_strnull,  }, /* const char __user * pathname */
        { sysarg_end } } };
nsysarg_calls[__NR_unlink] = 1;

sysarg_calls[__NR_symlinkat] = 
   (struct sysarg_call)
     { "symlinkat", {
        { sysarg_strnull,  }, /* const char __user * oldname */
        { sysarg_int,  }, /* int newdfd */
        { sysarg_strnull,  }, /* const char __user * newname */
        { sysarg_end } } };
nsysarg_calls[__NR_symlinkat] = 3;

sysarg_calls[__NR_symlink] = 
   (struct sysarg_call)
     { "symlink", {
        { sysarg_strnull,  }, /* const char __user * oldname */
        { sysarg_strnull,  }, /* const char __user * newname */
        { sysarg_end } } };
nsysarg_calls[__NR_symlink] = 2;

sysarg_calls[__NR_linkat] = 
   (struct sysarg_call)
     { "linkat", {
        { sysarg_int,  }, /* int olddfd */
        { sysarg_strnull,  }, /* const char __user * oldname */
        { sysarg_int,  }, /* int newdfd */
        { sysarg_strnull,  }, /* const char __user * newname */
        { sysarg_int,  }, /* int flags */
        { sysarg_end } } };
nsysarg_calls[__NR_linkat] = 5;

sysarg_calls[__NR_link] = 
   (struct sysarg_call)
     { "link", {
        { sysarg_strnull,  }, /* const char __user * oldname */
        { sysarg_strnull,  }, /* const char __user * newname */
        { sysarg_end } } };
nsysarg_calls[__NR_link] = 2;

sysarg_calls[__NR_renameat] = 
   (struct sysarg_call)
     { "renameat", {
        { sysarg_int,  }, /* int olddfd */
        { sysarg_strnull,  }, /* const char __user * oldname */
        { sysarg_int,  }, /* int newdfd */
        { sysarg_strnull,  }, /* const char __user * newname */
        { sysarg_end } } };
nsysarg_calls[__NR_renameat] = 4;

sysarg_calls[__NR_rename] = 
   (struct sysarg_call)
     { "rename", {
        { sysarg_strnull,  }, /* const char __user * oldname */
        { sysarg_strnull,  }, /* const char __user * newname */
        { sysarg_end } } };
nsysarg_calls[__NR_rename] = 2;

sysarg_calls[__NR_signalfd4] = 
   (struct sysarg_call)
     { "signalfd4", {
        { sysarg_int,  }, /* int ufd */
        { sysarg_unknown,  }, /* sigset_t __user * user_mask */
        { sysarg_int,  }, /* size_t sizemask */
        { sysarg_int,  }, /* int flags */
        { sysarg_end } } };
nsysarg_calls[__NR_signalfd4] = 4;

sysarg_calls[__NR_signalfd] = 
   (struct sysarg_call)
     { "signalfd", {
        { sysarg_int,  }, /* int ufd */
        { sysarg_unknown,  }, /* sigset_t __user * user_mask */
        { sysarg_int,  }, /* size_t sizemask */
        { sysarg_end } } };
nsysarg_calls[__NR_signalfd] = 3;

sysarg_calls[__NR_vmsplice] = 
   (struct sysarg_call)
     { "vmsplice", {
        { sysarg_int,  }, /* int fd */
        { sysarg_unknown,  }, /* const struct iovec __user * iov */
        { sysarg_int,  }, /* unsigned long nr_segs */
        { sysarg_int,  }, /* unsigned int flags */
        { sysarg_end } } };
nsysarg_calls[__NR_vmsplice] = 4;

sysarg_calls[__NR_splice] = 
   (struct sysarg_call)
     { "splice", {
        { sysarg_int,  }, /* int fd_in */
        { sysarg_unknown,  }, /* loff_t __user * off_in */
        { sysarg_int,  }, /* int fd_out */
        { sysarg_unknown,  }, /* loff_t __user * off_out */
        { sysarg_int,  }, /* size_t len */
        { sysarg_int,  }, /* unsigned int flags */
        { sysarg_end } } };
nsysarg_calls[__NR_splice] = 6;

sysarg_calls[__NR_tee] = 
   (struct sysarg_call)
     { "tee", {
        { sysarg_int,  }, /* int fdin */
        { sysarg_int,  }, /* int fdout */
        { sysarg_int,  }, /* size_t len */
        { sysarg_int,  }, /* unsigned int flags */
        { sysarg_end } } };
nsysarg_calls[__NR_tee] = 4;

sysarg_calls[__NR_quotactl] = 
   (struct sysarg_call)
     { "quotactl", {
        { sysarg_int,  }, /* unsigned int cmd */
        { sysarg_unknown,  }, /* const char __user * special */
        { sysarg_int,  }, /* qid_t id */
        { sysarg_unknown,  }, /* void __user * addr */
        { sysarg_end } } };
nsysarg_calls[__NR_quotactl] = 4;

sysarg_calls[__NR_setxattr] = 
   (struct sysarg_call)
     { "setxattr", {
        { sysarg_strnull,  }, /* const char __user * pathname */
        { sysarg_strnull,  }, /* const char __user * name */
        { sysarg_unknown,  }, /* const void __user * value */
        { sysarg_int,  }, /* size_t size */
        { sysarg_int,  }, /* int flags */
        { sysarg_end } } };
nsysarg_calls[__NR_setxattr] = 5;

sysarg_calls[__NR_lsetxattr] = 
   (struct sysarg_call)
     { "lsetxattr", {
        { sysarg_strnull,  }, /* const char __user * pathname */
        { sysarg_strnull,  }, /* const char __user * name */
        { sysarg_unknown,  }, /* const void __user * value */
        { sysarg_int,  }, /* size_t size */
        { sysarg_int,  }, /* int flags */
        { sysarg_end } } };
nsysarg_calls[__NR_lsetxattr] = 5;

sysarg_calls[__NR_fsetxattr] = 
   (struct sysarg_call)
     { "fsetxattr", {
        { sysarg_int,  }, /* int fd */
        { sysarg_strnull,  }, /* const char __user * name */
        { sysarg_unknown,  }, /* const void __user * value */
        { sysarg_int,  }, /* size_t size */
        { sysarg_int,  }, /* int flags */
        { sysarg_end } } };
nsysarg_calls[__NR_fsetxattr] = 5;

sysarg_calls[__NR_getxattr] = 
   (struct sysarg_call)
     { "getxattr", {
        { sysarg_strnull,  }, /* const char __user * pathname */
        { sysarg_strnull,  }, /* const char __user * name */
        { sysarg_buf_arglen, 1, 3 }, /* void __user * value */
        { sysarg_int,  }, /* size_t size */
        { sysarg_end } } };
nsysarg_calls[__NR_getxattr] = 4;

sysarg_calls[__NR_lgetxattr] = 
   (struct sysarg_call)
     { "lgetxattr", {
        { sysarg_strnull,  }, /* const char __user * pathname */
        { sysarg_strnull,  }, /* const char __user * name */
        { sysarg_buf_arglen, 1, 3 }, /* void __user * value */
        { sysarg_int,  }, /* size_t size */
        { sysarg_end } } };
nsysarg_calls[__NR_lgetxattr] = 4;

sysarg_calls[__NR_fgetxattr] = 
   (struct sysarg_call)
     { "fgetxattr", {
        { sysarg_int,  }, /* int fd */
        { sysarg_strnull,  }, /* const char __user * name */
        { sysarg_buf_arglen, 1, 3 }, /* void __user * value */
        { sysarg_int,  }, /* size_t size */
        { sysarg_end } } };
nsysarg_calls[__NR_fgetxattr] = 4;

sysarg_calls[__NR_listxattr] = 
   (struct sysarg_call)
     { "listxattr", {
        { sysarg_strnull,  }, /* const char __user * pathname */
        { sysarg_buf_arglen, 1, 2 }, /* char __user * list */
        { sysarg_int,  }, /* size_t size */
        { sysarg_end } } };
nsysarg_calls[__NR_listxattr] = 3;

sysarg_calls[__NR_llistxattr] = 
   (struct sysarg_call)
     { "llistxattr", {
        { sysarg_strnull,  }, /* const char __user * pathname */
        { sysarg_buf_arglen, 1, 2 }, /* char __user * list */
        { sysarg_int,  }, /* size_t size */
        { sysarg_end } } };
nsysarg_calls[__NR_llistxattr] = 3;

sysarg_calls[__NR_flistxattr] = 
   (struct sysarg_call)
     { "flistxattr", {
        { sysarg_int,  }, /* int fd */
        { sysarg_buf_arglen, 1, 2 }, /* char __user * list */
        { sysarg_int,  }, /* size_t size */
        { sysarg_end } } };
nsysarg_calls[__NR_flistxattr] = 3;

sysarg_calls[__NR_removexattr] = 
   (struct sysarg_call)
     { "removexattr", {
        { sysarg_strnull,  }, /* const char __user * pathname */
        { sysarg_strnull,  }, /* const char __user * name */
        { sysarg_end } } };
nsysarg_calls[__NR_removexattr] = 2;

sysarg_calls[__NR_lremovexattr] = 
   (struct sysarg_call)
     { "lremovexattr", {
        { sysarg_strnull,  }, /* const char __user * pathname */
        { sysarg_strnull,  }, /* const char __user * name */
        { sysarg_end } } };
nsysarg_calls[__NR_lremovexattr] = 2;

sysarg_calls[__NR_fremovexattr] = 
   (struct sysarg_call)
     { "fremovexattr", {
        { sysarg_int,  }, /* int fd */
        { sysarg_strnull,  }, /* const char __user * name */
        { sysarg_end } } };
nsysarg_calls[__NR_fremovexattr] = 2;

sysarg_calls[__NR_io_setup] = 
   (struct sysarg_call)
     { "io_setup", {
        { sysarg_int,  }, /* unsigned nr_events */
        { sysarg_unknown,  }, /* aio_context_t __user * ctxp */
        { sysarg_end } } };
nsysarg_calls[__NR_io_setup] = 2;

sysarg_calls[__NR_io_destroy] = 
   (struct sysarg_call)
     { "io_destroy", {
        { sysarg_int,  }, /* aio_context_t ctx */
        { sysarg_end } } };
nsysarg_calls[__NR_io_destroy] = 1;

sysarg_calls[__NR_io_submit] = 
   (struct sysarg_call)
     { "io_submit", {
        { sysarg_int,  }, /* aio_context_t ctx_id */
        { sysarg_int,  }, /* long nr */
        { sysarg_unknown,  }, /* struct iocb __user * __user * iocbpp */
        { sysarg_end } } };
nsysarg_calls[__NR_io_submit] = 3;

sysarg_calls[__NR_io_cancel] = 
   (struct sysarg_call)
     { "io_cancel", {
        { sysarg_int,  }, /* aio_context_t ctx_id */
        { sysarg_unknown,  }, /* struct iocb __user * iocb */
        { sysarg_unknown,  }, /* struct io_event __user * result */
        { sysarg_end } } };
nsysarg_calls[__NR_io_cancel] = 3;

sysarg_calls[__NR_io_getevents] = 
   (struct sysarg_call)
     { "io_getevents", {
        { sysarg_int,  }, /* aio_context_t ctx_id */
        { sysarg_int,  }, /* long min_nr */
        { sysarg_int,  }, /* long nr */
        { sysarg_unknown,  }, /* struct io_event __user * events */
        { sysarg_buf_fixlen, sizeof(struct timespec) }, /* struct timespec __user * timeout */
        { sysarg_end } } };
nsysarg_calls[__NR_io_getevents] = 5;

sysarg_calls[__NR_uselib] = 
   (struct sysarg_call)
     { "uselib", {
        { sysarg_unknown,  }, /* const char __user * library */
        { sysarg_end } } };
nsysarg_calls[__NR_uselib] = 1;

sysarg_calls[__NR_pipe2] = 
   (struct sysarg_call)
     { "pipe2", {
        { sysarg_unknown,  }, /* int __user * fildes */
        { sysarg_int,  }, /* int flags */
        { sysarg_end } } };
nsysarg_calls[__NR_pipe2] = 2;

sysarg_calls[__NR_pipe] = 
   (struct sysarg_call)
     { "pipe", {
        { sysarg_ignore,  }, /* int __user * fildes */
        { sysarg_end } } };
nsysarg_calls[__NR_pipe] = 1;

sysarg_calls[__NR_eventfd2] = 
   (struct sysarg_call)
     { "eventfd2", {
        { sysarg_int,  }, /* unsigned int count */
        { sysarg_int,  }, /* int flags */
        { sysarg_end } } };
nsysarg_calls[__NR_eventfd2] = 2;

sysarg_calls[__NR_eventfd] = 
   (struct sysarg_call)
     { "eventfd", {
        { sysarg_int,  }, /* unsigned int count */
        { sysarg_end } } };
nsysarg_calls[__NR_eventfd] = 1;

sysarg_calls[__NR_ustat] = 
   (struct sysarg_call)
     { "ustat", {
        { sysarg_int,  }, /* unsigned dev */
        { sysarg_unknown,  }, /* struct ustat __user * ubuf */
        { sysarg_end } } };
nsysarg_calls[__NR_ustat] = 2;

sysarg_calls[__NR_ioprio_set] = 
   (struct sysarg_call)
     { "ioprio_set", {
        { sysarg_int,  }, /* int which */
        { sysarg_int,  }, /* int who */
        { sysarg_int,  }, /* int ioprio */
        { sysarg_end } } };
nsysarg_calls[__NR_ioprio_set] = 3;

sysarg_calls[__NR_ioprio_get] = 
   (struct sysarg_call)
     { "ioprio_get", {
        { sysarg_int,  }, /* int which */
        { sysarg_int,  }, /* int who */
        { sysarg_end } } };
nsysarg_calls[__NR_ioprio_get] = 2;

sysarg_calls[__NR_select] = 
   (struct sysarg_call)
     { "select", {
        { sysarg_int,  }, /* int n */
        { sysarg_ignore,  }, /* fd_set __user * inp */
        { sysarg_ignore,  }, /* fd_set __user * outp */
        { sysarg_ignore,  }, /* fd_set __user * exp */
        { sysarg_buf_fixlen, sizeof(struct timeval) }, /* struct timeval __user * tvp */
        { sysarg_end } } };
nsysarg_calls[__NR_select] = 5;

sysarg_calls[__NR_pselect6] = 
   (struct sysarg_call)
     { "pselect6", {
        { sysarg_int,  }, /* int n */
        { sysarg_ignore,  }, /* fd_set __user * inp */
        { sysarg_ignore,  }, /* fd_set __user * outp */
        { sysarg_ignore,  }, /* fd_set __user * exp */
        { sysarg_buf_fixlen, sizeof(struct timespec) }, /* struct timespec __user * tsp */
        { sysarg_ignore,  }, /* void __user * sig */
        { sysarg_end } } };
nsysarg_calls[__NR_pselect6] = 6;

sysarg_calls[__NR_poll] = 
   (struct sysarg_call)
     { "poll", {
        { sysarg_ignore,  }, /* struct pollfd __user * ufds */
        { sysarg_int,  }, /* unsigned int nfds */
        { sysarg_int,  }, /* long timeout_msecs */
        { sysarg_end } } };
nsysarg_calls[__NR_poll] = 3;

sysarg_calls[__NR_ppoll] = 
   (struct sysarg_call)
     { "ppoll", {
        { sysarg_unknown,  }, /* struct pollfd __user * ufds */
        { sysarg_int,  }, /* unsigned int nfds */
        { sysarg_buf_fixlen, sizeof(struct timespec) }, /* struct timespec __user * tsp */
        { sysarg_unknown,  }, /* const sigset_t __user * sigmask */
        { sysarg_int,  }, /* size_t sigsetsize */
        { sysarg_end } } };
nsysarg_calls[__NR_ppoll] = 5;

sysarg_calls[__NR_mincore] = 
   (struct sysarg_call)
     { "mincore", {
        { sysarg_int,  }, /* unsigned long start */
        { sysarg_int,  }, /* size_t len */
        { sysarg_unknown,  }, /* unsigned char __user * vec */
        { sysarg_end } } };
nsysarg_calls[__NR_mincore] = 3;

sysarg_calls[__NR_brk] = 
   (struct sysarg_call)
     { "brk", {
        { sysarg_int,  }, /* unsigned long brk */
        { sysarg_end } } };
nsysarg_calls[__NR_brk] = 1;

sysarg_calls[__NR_munmap] = 
   (struct sysarg_call)
     { "munmap", {
        { sysarg_int,  }, /* unsigned long addr */
        { sysarg_int,  }, /* size_t len */
        { sysarg_end } } };
nsysarg_calls[__NR_munmap] = 2;

sysarg_calls[__NR_mremap] = 
   (struct sysarg_call)
     { "mremap", {
        { sysarg_int,  }, /* unsigned long addr */
        { sysarg_int,  }, /* unsigned long old_len */
        { sysarg_int,  }, /* unsigned long new_len */
        { sysarg_int,  }, /* unsigned long flags */
        { sysarg_int,  }, /* unsigned long new_addr */
        { sysarg_end } } };
nsysarg_calls[__NR_mremap] = 5;

sysarg_calls[__NR_mbind] = 
   (struct sysarg_call)
     { "mbind", {
        { sysarg_int,  }, /* unsigned long start */
        { sysarg_int,  }, /* unsigned long len */
        { sysarg_int,  }, /* unsigned long mode */
        { sysarg_unknown,  }, /* unsigned long __user * nmask */
        { sysarg_int,  }, /* unsigned long maxnode */
        { sysarg_int,  }, /* unsigned flags */
        { sysarg_end } } };
nsysarg_calls[__NR_mbind] = 6;

sysarg_calls[__NR_set_mempolicy] = 
   (struct sysarg_call)
     { "set_mempolicy", {
        { sysarg_int,  }, /* int mode */
        { sysarg_unknown,  }, /* unsigned long __user * nmask */
        { sysarg_int,  }, /* unsigned long maxnode */
        { sysarg_end } } };
nsysarg_calls[__NR_set_mempolicy] = 3;

sysarg_calls[__NR_migrate_pages] = 
   (struct sysarg_call)
     { "migrate_pages", {
        { sysarg_int,  }, /* pid_t pid */
        { sysarg_int,  }, /* unsigned long maxnode */
        { sysarg_unknown,  }, /* const unsigned long __user * old_nodes */
        { sysarg_unknown,  }, /* const unsigned long __user * new_nodes */
        { sysarg_end } } };
nsysarg_calls[__NR_migrate_pages] = 4;

sysarg_calls[__NR_get_mempolicy] = 
   (struct sysarg_call)
     { "get_mempolicy", {
        { sysarg_unknown,  }, /* int __user * policy */
        { sysarg_unknown,  }, /* unsigned long __user * nmask */
        { sysarg_int,  }, /* unsigned long maxnode */
        { sysarg_int,  }, /* unsigned long addr */
        { sysarg_int,  }, /* unsigned long flags */
        { sysarg_end } } };
nsysarg_calls[__NR_get_mempolicy] = 5;

sysarg_calls[__NR_readahead] = 
   (struct sysarg_call)
     { "readahead", {
        { sysarg_int,  }, /* int fd */
        { sysarg_int,  }, /* loff_t offset */
        { sysarg_int,  }, /* size_t count */
        { sysarg_end } } };
nsysarg_calls[__NR_readahead] = 3;

sysarg_calls[__NR_mlock] = 
   (struct sysarg_call)
     { "mlock", {
        { sysarg_int,  }, /* unsigned long start */
        { sysarg_int,  }, /* size_t len */
        { sysarg_end } } };
nsysarg_calls[__NR_mlock] = 2;

sysarg_calls[__NR_munlock] = 
   (struct sysarg_call)
     { "munlock", {
        { sysarg_int,  }, /* unsigned long start */
        { sysarg_int,  }, /* size_t len */
        { sysarg_end } } };
nsysarg_calls[__NR_munlock] = 2;

sysarg_calls[__NR_mlockall] = 
   (struct sysarg_call)
     { "mlockall", {
        { sysarg_int,  }, /* int flags */
        { sysarg_end } } };
nsysarg_calls[__NR_mlockall] = 1;

sysarg_calls[__NR_munlockall] = 
   (struct sysarg_call)
     { "munlockall", {
        { sysarg_end } } };
nsysarg_calls[__NR_munlockall] = 0;

sysarg_calls[__NR_fadvise64] = 
   (struct sysarg_call)
     { "fadvise64", {
        { sysarg_int,  }, /* int fd */
        { sysarg_int,  }, /* loff_t offset */
        { sysarg_int,  }, /* size_t len */
        { sysarg_int,  }, /* int advice */
        { sysarg_end } } };
nsysarg_calls[__NR_fadvise64] = 4;

sysarg_calls[__NR_msync] = 
   (struct sysarg_call)
     { "msync", {
        { sysarg_int,  }, /* unsigned long start */
        { sysarg_int,  }, /* size_t len */
        { sysarg_int,  }, /* int flags */
        { sysarg_end } } };
nsysarg_calls[__NR_msync] = 3;

sysarg_calls[__NR_brk] = 
   (struct sysarg_call)
     { "brk", {
        { sysarg_int,  }, /* unsigned long brk */
        { sysarg_end } } };
nsysarg_calls[__NR_brk] = 1;

sysarg_calls[__NR_munmap] = 
   (struct sysarg_call)
     { "munmap", {
        { sysarg_int,  }, /* unsigned long addr */
        { sysarg_int,  }, /* size_t len */
        { sysarg_end } } };
nsysarg_calls[__NR_munmap] = 2;

sysarg_calls[__NR_mremap] = 
   (struct sysarg_call)
     { "mremap", {
        { sysarg_int,  }, /* unsigned long addr */
        { sysarg_int,  }, /* unsigned long old_len */
        { sysarg_int,  }, /* unsigned long new_len */
        { sysarg_int,  }, /* unsigned long flags */
        { sysarg_int,  }, /* unsigned long new_addr */
        { sysarg_end } } };
nsysarg_calls[__NR_mremap] = 5;

sysarg_calls[__NR_move_pages] = 
   (struct sysarg_call)
     { "move_pages", {
        { sysarg_int,  }, /* pid_t pid */
        { sysarg_int,  }, /* unsigned long nr_pages */
        { sysarg_unknown,  }, /* const void __user * __user * pages */
        { sysarg_unknown,  }, /* const int __user * nodes */
        { sysarg_unknown,  }, /* int __user * status */
        { sysarg_int,  }, /* int flags */
        { sysarg_end } } };
nsysarg_calls[__NR_move_pages] = 6;

sysarg_calls[__NR_mprotect] = 
   (struct sysarg_call)
     { "mprotect", {
        { sysarg_int,  }, /* unsigned long start */
        { sysarg_int,  }, /* size_t len */
        { sysarg_int,  }, /* unsigned long prot */
        { sysarg_end } } };
nsysarg_calls[__NR_mprotect] = 3;

sysarg_calls[__NR_remap_file_pages] = 
   (struct sysarg_call)
     { "remap_file_pages", {
        { sysarg_int,  }, /* unsigned long start */
        { sysarg_int,  }, /* unsigned long size */
        { sysarg_int,  }, /* unsigned long prot */
        { sysarg_int,  }, /* unsigned long pgoff */
        { sysarg_int,  }, /* unsigned long flags */
        { sysarg_end } } };
nsysarg_calls[__NR_remap_file_pages] = 5;

sysarg_calls[__NR_madvise] = 
   (struct sysarg_call)
     { "madvise", {
        { sysarg_int,  }, /* unsigned long start */
        { sysarg_int,  }, /* size_t len_in */
        { sysarg_int,  }, /* int behavior */
        { sysarg_end } } };
nsysarg_calls[__NR_madvise] = 3;

sysarg_calls[__NR_swapoff] = 
   (struct sysarg_call)
     { "swapoff", {
        { sysarg_unknown,  }, /* const char __user * specialfile */
        { sysarg_end } } };
nsysarg_calls[__NR_swapoff] = 1;

sysarg_calls[__NR_swapon] = 
   (struct sysarg_call)
     { "swapon", {
        { sysarg_unknown,  }, /* const char __user * specialfile */
        { sysarg_int,  }, /* int swap_flags */
        { sysarg_end } } };
nsysarg_calls[__NR_swapon] = 2;

sysarg_calls[__NR_mmap] = 
   (struct sysarg_call)
     { "mmap", {
        { sysarg_int,  }, /* unsigned long addr */
        { sysarg_int,  }, /* unsigned long len */
        { sysarg_int,  }, /* unsigned long prot */
        { sysarg_int,  }, /* unsigned long flags */
        { sysarg_int,  }, /* unsigned long fd */
        { sysarg_int,  }, /* unsigned long off */
        { sysarg_end } } };
nsysarg_calls[__NR_mmap] = 6;

sysarg_calls[__NR_uname] = 
   (struct sysarg_call)
     { "uname", {
        { sysarg_ignore,  }, /* struct new_utsname __user * name */
        { sysarg_end } } };
nsysarg_calls[__NR_uname] = 1;

sysarg_calls[__NR_fork] = 
   (struct sysarg_call)
     { "fork", {
        { sysarg_end } } };
nsysarg_calls[__NR_fork] = 0;

sysarg_calls[__NR_vfork] = 
   (struct sysarg_call)
     { "vfork", {
        { sysarg_end } } };
nsysarg_calls[__NR_vfork] = 0;

sysarg_calls[__NR_clone] = 
   (struct sysarg_call)
     { "clone", {
        { sysarg_end } } };
nsysarg_calls[__NR_clone] = 0;

sysarg_calls[__NR_execve] = 
   (struct sysarg_call)
     { "execve", {
        { sysarg_strnull },
        { sysarg_argv_when_entering },
        { sysarg_argv_when_entering },
        { sysarg_end } } };
nsysarg_calls[__NR_execve] = 3;

sysarg_calls[__NR_arch_prctl] = 
   (struct sysarg_call)
     { "arch_prctl", {
        { sysarg_end } } };
nsysarg_calls[__NR_arch_prctl] = 0;

sysarg_calls[__NR_set_tid_address] = 
   (struct sysarg_call)
     { "set_tid_address", {
        { sysarg_ignore }, /* int __user * tidptr */
        { sysarg_end } } };
nsysarg_calls[__NR_set_tid_address] = 1;

sysarg_calls[__NR_futex] = 
   (struct sysarg_call)
     { "futex", {
        { sysarg_ignore }, /* u32 __user * uaddr */
        { sysarg_int,  }, /* int op */
        { sysarg_int,  }, /* u32 val */
        { sysarg_ignore }, /* struct timespec __user * utime, FIXME it depends on op, check sys_futex */
        { sysarg_ignore }, /* u32 __user * uaddr2 */
        { sysarg_int,  }, /* u32 val3 */
        { sysarg_end } } };
nsysarg_calls[__NR_futex] = 6;

sysarg_calls[__NR_undo_func_start] = 
   (struct sysarg_call)
     { "undo_func_start", {
        { sysarg_strnull },        /* const char *undo_mgr */
        { sysarg_strnull },        /* const char *funcname */
        { sysarg_int },            /* int arglen */
        { sysarg_buf_arglen, 1, 2 },    /* void *argbuf */
        { sysarg_end } } };
nsysarg_calls[__NR_undo_func_start] = 4;

sysarg_calls[__NR_undo_func_end] = 
   (struct sysarg_call)
     { "undo_func_end", {
        { sysarg_int },            /* int retlen */
        { sysarg_buf_arglen, 1, 0 },    /* void *retbuf */
        { sysarg_end } } };
nsysarg_calls[__NR_undo_func_end] = 2;

sysarg_calls[__NR_undo_mask_start] = 
   (struct sysarg_call)
     { "undo_mask_start", {
        { sysarg_int },            /* int fd */
        { sysarg_end } } };
nsysarg_calls[__NR_undo_mask_start] = 1;

sysarg_calls[__NR_undo_mask_end] = 
   (struct sysarg_call)
     { "undo_mask_end", {
        { sysarg_int },            /* int fd */
        { sysarg_end } } };
nsysarg_calls[__NR_undo_mask_end] = 1;

}
