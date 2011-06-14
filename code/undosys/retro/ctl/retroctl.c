/*
 * NOTE: don't output anything like statistics to terminal in this
 * program.  otherwise the sshd process used to run the program would
 * write the same data to socket, which would be recorded by the
 * kernel and may cause more output.
 */

#include <sys/param.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <assert.h>
#include <db.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "zfile.h"

#define CANCEL_FD 1

// state
struct bufstate {
	size_t id;
	// file descriptors
	int syscall_in;
	int kprobes_in;
	int inode_in;
	int pid_in;
	int pathid_in;
	int pathidtable_in;
	int sock_in;
	zfile_t syscall_out;
	zfile_t kprobes_out;
	DB *inode_out;
	DB *pid_out;
	DB *pathid_out;
	DB *pathidtable_out;
	DB *sock_out;
#if CANCEL_FD
	int cancel_fd_read;
	int cancel_fd_write;
#endif
	// pthread
	pthread_t thread;
};

static void init(struct bufstate *);
static void *worker(void *);
static void filecopy(int infd, zfile_t outfd);
static void indexcopy(int infd, DB *dbp, size_t nbytes);
#define inodecopy(in, out)  indexcopy((in), (out), 4 )
#define pidcopy(in, out)    indexcopy((in), (out), 4 )
#define pathidcopy(in, out) indexcopy((in), (out), 20)
#define sockcopy(in, out)   indexcopy((in), (out), 8 )
static void pathidtablecopy(int infd, DB *dbp);

static void usage();
static int dir_exists(const char *);
static char *get_app_dir(const char *, char *);
static void toggle(const char *dir, const char *filename, size_t val);
static void wait_for_child();
static void wait_for_signal();

static char retro_dir[PATH_MAX];
static char khook_dir[PATH_MAX];

static char *out_dir = ".";
static char *cd = NULL;
static char *chrdir = NULL;

static int affinity = 0;
static int uid = 0;
static int profile = 0;

static char *profout = "/tmp/retroctl-profile";
static struct timeval beg_tv;
static struct timeval end_tv;

static void disable() { toggle(khook_dir, "enabled", 0); }

int main(int argc, char **argv)
{
	/* -p is default */
	int process_flag = 1;
	for (;;) {
		struct option longopts[] = {
			{"output-directory",  required_argument, 0, 'o'},
			{"affinity",	      no_argument,	 0, 'a'},
			{"process",	      no_argument,	 0, 'p'},
			{"step",	      no_argument,	 0, 's'},
			{"uid",		      optional_argument, 0, 'u'},
			{"dir",		      optional_argument, 0, 'd'},
			{"chroot",            optional_argument, 0, 'c'},
			{"profile",	      no_argument,	 0, 'r'},
			{"profile-directory", optional_argument, 0, 'x'},
			{"help",	      no_argument,	 0, 'h'},
			{0, 0, 0, 0}
		};
		int idx = 0;
		int c = getopt_long(argc, argv, "ho:psu:g:d:rx:ac:", longopts, &idx);
		if (c < 0)
			break;
		switch (c) {
		case 'h':
			usage();
			exit(0);
		case 'o':
			if (optarg) {
				out_dir = optarg;
				if (!dir_exists(out_dir))
					errx(1, "output directory not found");
			}
			break;
		case 's':
			process_flag |= 2;
		case 'p':
			process_flag |= 1;
			break;
		case 'u':
			if (optarg) {
				uid = atoi(optarg);
			}
			break;
		case 'd':
			if (optarg) {
				cd = optarg;
				if (!dir_exists(cd))
					errx(1, "directory not found");
			}
			break;
		case 'c':
			if (optarg) {
				chrdir = optarg;
				if (!dir_exists(chrdir))
					errx(1, "directory not found");
			}
		case 'r':
			profile = 1;
			break;
		case 'x':
			if (optarg) {
				profout = optarg;
			}
			break;
		case 'a':
			affinity = 1;
			break;
		case '?':
		default:
			exit(1);
		}
	}

	if (geteuid())
		errx(1, "this program must setuid root");

	if (process_flag) {
		// process trace
		if (optind >= argc)
			errx(1, "no program specified");
	} else {
		// whole-system trace
		if (optind < argc)
			errx(1, "unknown option");
	}

	get_app_dir("khook", khook_dir);
	get_app_dir("retro", retro_dir);

	atexit(disable);
	toggle(khook_dir, "enabled", 0);

	size_t ncpus = sysconf(_SC_NPROCESSORS_ONLN);
	struct bufstate bss[ncpus];
	memset(bss, 0, ncpus * sizeof(bss[0]));
	for (size_t i = 0; i < ncpus; ++i) {
		bss[i].id = i;
		init(&bss[i]);
		if (pthread_create(&bss[i].thread, 0, worker, &bss[i]) < 0)
			err(1, "pthread_create %zu", i);
	}

	// turn on stop watch
	if (profile && gettimeofday(&beg_tv, NULL)) {
		err(1, "failed to set profiler");
	}

	// reset channel
	toggle(retro_dir, "reset", 1);
	toggle(retro_dir, "trace", 0);

	// start
	toggle(khook_dir, "process", process_flag);
	toggle(khook_dir, "ctl", getpid());
	toggle(khook_dir, "enabled", 1);

	if (process_flag) {
		if (fork() == 0) {
			if (uid)
				setuid(uid);
			if (affinity) {
				cpu_set_t mask;
				CPU_ZERO(&mask);
				CPU_SET(0, &mask);
				if (sched_setaffinity(0, sizeof(mask), &mask))
					err(1, "sched_setaffinity");
			}
			if (cd) {
				if (chdir(cd))
					err(1, "failed to chdir:%s", cd);
			}
			if (chrdir) {
				if (chroot(chrdir)) {
					errx(1, "failed to chroot:%s", chrdir);
				}
			}
			ptrace(PTRACE_TRACEME, 0, NULL, NULL);
			execvp(argv[optind], argv+optind);
			exit(-1);
		}
		if (affinity) {
			cpu_set_t mask;
			CPU_ZERO(&mask);
			CPU_SET(1, &mask);
			if (sched_setaffinity(0, sizeof(mask), &mask))
				err(1, "sched_setaffinity");
		}
		wait_for_child();
	}
	else
		wait_for_signal();

	// stop
	toggle(khook_dir, "enabled", 0);

	// flush
	toggle(retro_dir, "flush", 0);

	for (size_t i = 0; i < ncpus; ++i) {
		void *dummy;
		struct bufstate *bs = &bss[i];
#if CANCEL_FD
		if (write(bs->cancel_fd_write, "x", 1) < 0)
			perror("write");
		close(bs->cancel_fd_write);
#else
		pthread_cancel(bs->thread);
#endif
		pthread_join(bs->thread, &dummy);
#if CANCEL_FD
		close(bs->cancel_fd_read);
#else
		filecopy(bs->syscall_in, bs->syscall_out);
		filecopy(bs->kprobes_in, bs->kprobes_out);
		inodecopy(bs->inode_in, bs->inode_out);
		pidcopy(bs->pid_in, bs->pid_out);
		pathidcopy(bs->pathid_in, bs->pathid_out);
		pathidtablecopy(bs->pathidtable_in, bs->pathidtable_out);
		sockcopy(bs->sock_in, bs->sock_out);
#endif
#define CLEANUP(x) \
		close(bs->x##_in); \
		zclose(bs->x##_out);
		CLEANUP(syscall);
		CLEANUP(kprobes);
#undef CLEANUP
		bs->inode_out->close(bs->inode_out, 0);
		bs->pid_out->close(bs->pid_out, 0);
		bs->pathid_out->close(bs->pathid_out, 0);
		bs->pathidtable_out->close(bs->pathidtable_out, 0);
		bs->sock_out->close(bs->sock_out, 0);
	}

	// turn off stop watch
	if (profile && gettimeofday(&end_tv, NULL)) {
		err(1, "failed to set profiler");
	}

	if (profile) {
		FILE * fd = fopen(profout, "a+");
		if (fd == NULL) {
			err(1, "failed to open: %s", profout);
		}

		char * t = ctime(&end_tv.tv_sec);
		t[strlen(t)-1] = '\0';
		fprintf(fd, "%s\t", t);

		const long sec	= end_tv.tv_sec	 - beg_tv.tv_sec;
		const long usec = end_tv.tv_usec - beg_tv.tv_usec;

		fprintf(fd, "%ld.%ld\n",
			sec - (usec < 0 ? 1 : 0),
			(usec < 0 ? (usec + 1000000) : usec));
		fclose(fd);
	}
}

static void usage()
{
	fprintf(stderr, "Usage: retroctl [OPTION]... [FILE]...\n"
		"  -o, --output-directory=DIR\tstore log files in DIR\n"
		"  -p, --process	     \ttrace a process and its descendants only\n"
		"  -s, --step		     \tstep-trace syscalls of a process\n"
		"  -r, --profile	     \trecord the execution time\n"
		"  -u, --uid		     \trun process as specified uid\n"
		"  -x, --profile-file	     \tstore profile output to the file\n"
		"  -d, --dir		     \tinvoke process in the specified directory\n"
	);
}

static int dir_exists(const char *path)
{
	struct stat st;
	return (stat(path, &st) == 0) && S_ISDIR(st.st_mode);
}

static char *get_app_dir(const char *app, char *path)
{
	char *ss[6];
	FILE *fp = fopen("/proc/mounts", "r");
	int found = 0;
	for (; !feof(fp) && !found; ) {
		int n = fscanf(fp, "%ms %ms %ms %ms %ms %ms", &ss[0], &ss[1],
		       &ss[2], &ss[3], &ss[4], &ss[5]);
		assert(n == 6);
		if (!strcmp(ss[2], "debugfs")) {
			found = 1;
			sprintf(path, "%s/%s", ss[1], app);
		}
		for (size_t i = 0; i < sizeof(ss)/sizeof(ss[0]); ++i)
			free(ss[i]);
	}
	fclose(fp);
	if (!found)
		errx(1, "debugfs not mounted");
	if (!dir_exists(path))
		errx(1, "module not loaded");
	return path;
}

static void toggle(const char *dir, const char *filename, size_t v)
{
	char path[PATH_MAX];
	char buf[64];
	snprintf(path, sizeof(path), "%s/%s", dir, filename);
	int fd = open(path, O_RDWR);
	if (fd < 0)
		err(1, "open %s", path);
	sprintf(buf, "%zu", v);
	if (write(fd, buf, strlen(buf)) < 0)
		err(1, "write %s", path);
	close(fd);
}

static void init(struct bufstate *bs)
{
#if CANCEL_FD
	int cancel_fds[2];
	if (pipe(cancel_fds) < 0)
		err(1, "pipe");
	for (int i = 0; i < 2; i++)
		if (fcntl(cancel_fds[i], F_SETFD, FD_CLOEXEC) < 0)
			err(1, "fcntl");
	bs->cancel_fd_read = cancel_fds[0];
	bs->cancel_fd_write = cancel_fds[1];
#endif

#if 0
	// affinity
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(bs->id, &mask);
	if (sched_setaffinity(0, sizeof(mask), &mask))
		err(1, "sched_setaffinity");
#endif

	// open files for read/write
	char path[PATH_MAX];
	DB *dbp = NULL;
	int ret;
#define CREATEIO(x) \
	snprintf(path, sizeof(path), "%s/"#x"%zu", retro_dir, bs->id); \
	bs->x##_in = open(path, O_RDONLY); \
	if (bs->x##_in < 0) \
		err(1, "open `%s' for read", path); \
	snprintf(path, sizeof(path), "%s/"#x"%zu", out_dir, bs->id);

	CREATEIO(syscall);
	bs->syscall_out = zcreat(path, 0644);

	CREATEIO(kprobes);
	bs->kprobes_out = zcreat(path, 0644);

#define CREATEIO_(x) \
	CREATEIO(x); \
	if ((ret = db_create(&dbp, NULL, 0))) \
		errx(1, "db_create: %s", db_strerror(ret)); \
	if ((ret = dbp->set_flags(dbp, DB_DUPSORT))) \
		errx(1, "db::set_flags: %s", db_strerror(ret)); \
	if ((ret = dbp->open(dbp, NULL, path, NULL, DB_BTREE, DB_CREATE | DB_TRUNCATE, 0644))) \
		errx(1, "db::open %s: %s", path, db_strerror(ret)); \
	bs->x##_out = dbp;

	CREATEIO_(inode);
	CREATEIO_(pid);
	CREATEIO_(pathid);
	CREATEIO_(sock);
	CREATEIO(pathidtable);

	if ((ret = db_create(&dbp, NULL, 0)))
		errx(1, "db_create: %s", db_strerror(ret));
	if ((ret = dbp->open(dbp, NULL, path, NULL, DB_HASH, DB_CREATE | DB_TRUNCATE, 0644)))
		errx(1, "db::open %s: %s", path, db_strerror(ret));
	bs->pathidtable_out = dbp;
}

static void *worker(void *data)
{
	struct bufstate *bs = data;
	// loop
	for (;;) {
		struct pollfd fds[] = {
			{ .fd = bs->syscall_in,     .events = POLLIN },
			{ .fd = bs->kprobes_in,     .events = POLLIN },
			{ .fd = bs->inode_in,       .events = POLLIN },
			{ .fd = bs->pid_in,         .events = POLLIN },
			{ .fd = bs->pathid_in,      .events = POLLIN },
			{ .fd = bs->pathidtable_in, .events = POLLIN },
			{ .fd = bs->sock_in,        .events = POLLIN },
#if CANCEL_FD
			{ .fd = bs->cancel_fd_read, .events = POLLIN },
#endif
		};
		int r = poll(fds, sizeof(fds)/sizeof(fds[0]), -1);
		if (r < 0) {
			if (errno != EINTR)
				err(1, "poll %zu", bs->id);
			warn("poll %zu", bs->id);
			continue;
		}
#if CANCEL_FD
#else
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
#endif
		if (fds[0].revents & POLLIN)
			filecopy(bs->syscall_in, bs->syscall_out);
		if (fds[1].revents & POLLIN)
			filecopy(bs->kprobes_in, bs->kprobes_out);
		if (fds[2].revents & POLLIN)
			inodecopy(bs->inode_in, bs->inode_out);
		if (fds[3].revents & POLLIN)
			pidcopy(bs->pid_in, bs->pid_out);
		if (fds[4].revents & POLLIN)
			pathidcopy(bs->pathid_in, bs->pathid_out);
		if (fds[5].revents & POLLIN)
			pathidtablecopy(bs->pathidtable_in, bs->pathidtable_out);
		if (fds[6].revents & POLLIN)
			sockcopy(bs->sock_in, bs->sock_out);
#if CANCEL_FD
		if ((fds[7].revents & POLLIN) && (r == 1))
			break;
#else
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
#endif
	}
	return 0;
}

static void filecopy(int infd, zfile_t outfd)
{
	char buf[64 * 1024];
	for (;;) {
		ssize_t len = read(infd, buf, sizeof(buf));
		if (len < 0)
			err(1, "read");
		if (len == 0)
			break;
		if (zwrite(outfd, buf, len) < len)
			err(1, "write");
	}
}

static void indexcopy(int infd, DB *dbp, size_t nbytes)
{
	for (;;) {
#pragma pack(push)
#pragma pack(1)
		struct {
			size_t offset;
			uint16_t nr;
			uint32_t sid;
			char data[nbytes];
		} rec[1024];
#pragma pack(pop)
		ssize_t len = read(infd, &rec[0], sizeof(rec));
		int nrec, i;
		if (len == 0)
			break;
		if ((len % sizeof(rec[0])) != 0)
			errx(1, "read@indexcopy %zd bytes", len);

		nrec = len / sizeof(rec[0]);
		for (i = 0; i < nrec; i++) {
			DBT key, data;
			memset(&key, 0, sizeof(DBT));
			memset(&data, 0, sizeof(DBT));
			key.data = &rec[i].data;
			key.size = nbytes;
			data.data = &rec[i].offset;
			data.size = sizeof(rec[i]) - nbytes;
			dbp->put(dbp, NULL, &key, &data, DB_NODUPDATA);
		}
	}
}

static void pathidtablecopy(int infd, DB *dbp) {
	for (;;) {
#pragma pack(push)
#pragma pack(1)
		struct {
			char pathid[20];
			char parent_pathid[20];
			char buf[128];
			size_t len;
		} rec[1024];
#pragma pack(pop)
		ssize_t len = read(infd, &rec[0], sizeof(rec));
		int nrec, i;
		if (len == 0)
			break;
		if ((len % sizeof(rec[0])) != 0)
			errx(1, "read@pathidtablecopy %zd bytes", len);

		nrec = len / sizeof(rec[0]);
		for (i = 0; i < nrec; i++) {
			DBT key, data;
			memset(&key, 0, sizeof(DBT));
			memset(&data, 0, sizeof(DBT));
			key.data = &rec[i].pathid;
			key.size = 20;
			data.data = &rec[i].parent_pathid;
			data.size = 20 + rec[i].len;
			dbp->put(dbp, NULL, &key, &data, 0);
		}
	}
}

static void wait_for_child()
{
	// ignore signals from keyboard
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	// first syscall
	int status;
	int pid = waitpid(-1, &status, __WALL);
	if (pid < 0)
		err(1, "waitpid 0");
	if (!WIFSTOPPED(status))
		errx(1, "invalid process");
	// first syscall should be execve
	assert(SIGTRAP == WSTOPSIG(status));
	assert(SYS_execve == ptrace(PTRACE_PEEKUSER, pid, ORIG_RAX * 8, 0));
	if (ptrace(PTRACE_SETOPTIONS, pid, NULL, PTRACE_O_TRACESYSGOOD
			| PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK
			| PTRACE_O_TRACEVFORK | PTRACE_O_TRACEEXEC))
		err(1, "PTRACE_SETOPTIONS");
	ptrace(PTRACE_CONT, pid, NULL, NULL);
	// loop
	for (;;) {
		pid = waitpid(-1, &status, __WALL);
		if (pid < 0)
			break;
		if (!WIFSTOPPED(status))
			continue;
		int signo = WSTOPSIG(status);
		if (signo == (SIGTRAP | 0x80))
			signo = 0;
		ptrace(PTRACE_CONT, pid, NULL, signo);
	}
}

static void wait_for_signal()
{
	sigset_t signals;
	sigemptyset(&signals);
	sigaddset(&signals, SIGINT);
	sigaddset(&signals, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &signals, NULL);
	int sig;
	while (sigwait(&signals, &sig) == 0) {
		if (sig == SIGINT || sig == SIGTERM)
			break;
	}
}
