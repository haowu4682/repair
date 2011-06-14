// launcher daemon: bind & load services

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <openssl/conf.h>
#include "utility.h"

#define	MAX_SERVICES 256
static int fds[MAX_SERVICES], pids[MAX_SERVICES];
static char svcname[MAX_SERVICES][256];
static int nsvc = 0;

static int open_socket(short port);
static const char *get_nonnull_string(CONF *conf, const char *sec,
				      const char *key);
static int get_int(CONF *conf, const char *sec, const char *key);
static void launch_srv(CONF *conf, const char *secname);
static int service_parse_cb(const char *name, int len, void *arg);
static int args_parse_cb(const char *name, int len, void *arg);

struct args_parse {
    int argc;
    char *argv[32];
};

int
main(int argc, char **argv)
{
    // run as root
    if (geteuid()) {
	fprintf(stderr, "zookld must be started as root\n");
	exit(-1);
    }
    // accept
    if (argc != 2) {
	fprintf(stderr, "Usage: %s port\n", argv[0]);
	exit(-1);
    }
    short port = (short) atoi(argv[1]);

    int srvfd = open_socket(port);
    assert(srvfd >= 0);

    // load config:
    // http://linux.die.net/man/5/config
    // http://www.openssl.org/docs/apps/config.html
    CONF *conf = NCONF_new(NULL);
    char *filename = "zook.conf";
    long eline;
    if (0 == NCONF_load(conf, filename, &eline)) {
	fprintf(stderr, "error on line %ld of %s\n", eline, filename);
	exit(-1);
    }

    // start services
    const char *zookd_sec = get_nonnull_string(conf, "zookld", "dispatch");
    launch_srv(conf, zookd_sec);
    const char *http_svcs = NCONF_get_string(conf, zookd_sec, "http_svcs");
    if (http_svcs)
	CONF_parse_list(http_svcs, ',', 1, &service_parse_cb, conf);
    const char *extra_svcs = NCONF_get_string(conf, "zookld", "extra_svcs");
    if (extra_svcs)
	CONF_parse_list(extra_svcs, ',', 1, &service_parse_cb, conf);

    // send srvfd to zookd
    if (sendfd(fds[0], &nsvc, sizeof(nsvc), srvfd) < 0) {
	perror("send srvfd");
	exit(-1);
    }
    close(srvfd);

    // send url mapping to zookd
    for (int i = 1; i < nsvc; ++i) {
	const char *url = NCONF_get_string(conf, svcname[i], "url");
	if (!url)
	    url = "none";
	sendfd(fds[0], url, strlen(url) + 1, fds[i]);
	close(fds[i]);
    }
    close(fds[0]);

    NCONF_free(conf);

    // wait for zookd
    int status;
    waitpid(pids[0], &status, 0);
}

int
open_socket(short port)
{
    int srvfd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(srvfd, F_SETFD, FD_CLOEXEC);

    if (srvfd < 0) {
	perror("socket");
	exit(-1);
    }

    int on = 1;
    if (setsockopt(srvfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
	perror("setsockopt SO_REUSEADDR");
	exit(-1);
    }

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);
    if (bind(srvfd, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
	perror("bind");
	exit(-1);
    }

    if (listen(srvfd, 5) < 0) {
	perror("listen");
	exit(-1);
    }
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    fprintf(stderr, "listening on port %hd\n", port);
    return srvfd;
}

const char *
get_nonnull_string(CONF * conf, const char *sec, const char *key)
{
    const char *s = NCONF_get_string(conf, sec, key);
    if (NULL == s || 0 == strlen(s)) {
	fprintf(stderr, "%s::%s cannot be empty\n", sec, key);
	exit(-1);
    }
    return s;
}

int
get_int(CONF *conf, const char *sec, const char *key)
{
    long v;
    if (!NCONF_get_number_e(conf, sec, key, &v)) {
	fprintf(stderr, "%s::%s missing or invalid\n", sec, key);
	exit(-1);
    }
    return v;
}

void
launch_srv(CONF *conf, const char *secname)
{
    fprintf(stderr, "launching %s\n", secname);
    snprintf(&svcname[nsvc][0], sizeof(svcname[nsvc]), "%s", secname);

    int sockets[2] = { -1 };
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
	perror("socketpair");
	exit(-1);
    }

    pid_t pid = fork();
    if (pid < 0) {
	perror("fork");
	exit(-1);
    }

    if (pid > 0) {
	// parent
	close(sockets[1]);
	fds[nsvc] = sockets[0];
	pids[nsvc] = pid;
	nsvc++;
	return;
    }

    // child
    close(sockets[0]);

    const char *cmd = get_nonnull_string(conf, secname, "cmd");
    const char *dir = get_nonnull_string(conf, secname, "dir");
    int uid = get_int(conf, secname, "uid");
    int gid = get_int(conf, secname, "gid");

    struct args_parse ap = { 0 };
    ap.argv[0] = (char *) cmd;
    asprintf(&ap.argv[1], "%d", sockets[1]);
    ap.argc = 2;

    const char *args = NCONF_get_string(conf, secname, "args");
    if (args)
	CONF_parse_list(args, ' ', 1, &args_parse_cb, &ap);

    // chdir
    if (chdir(dir)) {
	perror("chdir");
	exit(-1);
    }

    // chroot
    if (chroot(".")) {
	perror("chroot");
	exit(-1);
    }

    if (setresgid(gid, gid, gid) < 0) {
	perror("setgid");
	exit(-1);
    }

    if (setresuid(uid, uid, uid) < 0) {
	perror("setuid");
	exit(-1);
    }

    execv(cmd, ap.argv);
    perror("execv");
    exit(-1);
}

int
service_parse_cb(const char *name, int len, void *arg)
{
    if (len == 0)
	return 1;

    char svcname[256];
    snprintf(&svcname[0], sizeof(svcname), "%s", name);
    svcname[len] = '\0';

    launch_srv((CONF *) arg, svcname);
    return 1;
}

int
args_parse_cb(const char *name, int len, void *arg)
{
    struct args_parse *ap = arg;
    ap->argv[ap->argc] = strndup(name, len);
    ap->argc++;
    return 1;
}
