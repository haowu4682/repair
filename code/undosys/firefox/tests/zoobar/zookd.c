// demux

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include "utility.h"

enum { use_fork = 0 };

static char *
parse_req(int fd, char *reqpath, char *env, size_t * env_len)
{
    static char buf[8192];	/* static variables are not on the stack */

    if (read_line(&buf[0], sizeof(buf), fd) < 0)
	return "Socket IO error";

    /* Parse request like "GET /foo.html HTTP/1.0" */
    char *sp1 = strchr(&buf[0], ' ');
    if (!sp1)
	return "Cannot parse HTTP request (1)";
    *sp1 = '\0';
    sp1++;
    if (*sp1 != '/')
	return "Bad request path";

    char *sp2 = strchr(sp1, ' ');
    if (!sp2)
	return "Cannot parse HTTP request (2)";
    *sp2 = '\0';
    sp2++;

    /* We only support GET and POST requests */
    if (strcmp(&buf[0], "GET") && strcmp(&buf[0], "POST"))
	return "Unsupported request (not GET or POST)";

    char *envp = env;

    //setenv("REQUEST_METHOD", &buf[0], 1);
    envp += sprintf(envp, "REQUEST_METHOD=%s", buf) + 1;
    //setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);

    /* Decode URL escape sequences in the requested path into reqpath */
    url_decode(sp1, reqpath);

    /* Parse out query string, e.g. "foo.py?user=bob" */
    char *qp = strchr(reqpath, '?');
    if (qp) {
	*qp = '\0';
	//setenv("QUERY_STRING", qp+1, 1);
	envp += sprintf(envp, "QUERY_STRING=%s", qp + 1) + 1;
    }
    //setenv("SCRIPT_NAME", reqpath, 1);
    envp += sprintf(envp, "SCRIPT_NAME=%s", reqpath) + 1;

    *envp = 0;
    *env_len = envp - env + 1;
    return 0;
}

static void
process_client(int fd, int n, regex_t * urls, int *fds)
{
    char reqpath[256], env[9000];
    size_t env_len = 0;

    char *err = parse_req(fd, reqpath, env, &env_len);
    if (err) {
	http_err(fd, 500, "[zookd] error parsing request: %s", err);
	return;
    }

    int i;
    for (i = 0; i < n; ++i) {
	if (0 == regexec(&urls[i], reqpath, 0, 0, 0)) {
	    fprintf(stderr, "[zookd] forwarding %s to service %d\n",
		    reqpath, i);
	    break;
	}
    }

    if (i == n) {
	fprintf(stderr, "[zookd] not forwarding %s to any service\n", reqpath);
	http_err(fd, 500, "[zookd] error dispatching request");
	return;
    }

    if (sendfd(fds[i], env, env_len, fd) <= 0) {
	http_err(fd, 500, "[zookd] error forwarding request");
	return;
    }

    close(fd);
}

int
main(int argc, char *argv[])
{
    if (argc != 2) {
	printf("[zookd] must be called from zookld\n");
	exit(-1);
    }

    int socket = atoi(argv[1]), srvfd = -1;
    int n;
    ssize_t r = recvfd(socket, &n, sizeof(n), &srvfd);
    if (r <= 0 || srvfd < 0) {
	perror("[zookd] recv srvfd");
	exit(-1);
    }

    --n;
    printf("[zookd] started with %d service(s)\n", n);

    regex_t urls[n];
    int fds[n];
    for (int i = 0; i < n; ++i) {
	char buf[1024];
	ssize_t r = recvfd(socket, buf, sizeof(buf), &fds[i]);
	if (r <= 0 || 0 == memchr(buf, 0, sizeof(buf))) {
	    perror("[zookd] recv fd");
	    exit(-1);
	}

	char regexp[1024];
	snprintf(&regexp[0], sizeof(regexp), "^%s$", buf);
	if (regcomp(&urls[i], regexp, REG_EXTENDED | REG_NOSUB)) {
	    printf("[zookd] bad url: %s\n", buf);
	    exit(-1);
	}
	printf("[zookd] service %d: %s\n", i, regexp);
    }

    for (;;) {
	struct sockaddr_in client_addr;
	unsigned int addrlen = sizeof(client_addr);
	int cfd = accept(srvfd, (struct sockaddr *) &client_addr, &addrlen);
	if (cfd < 0) {
	    perror("[zookd] accept");
	    continue;
	}

	int pid = use_fork ? fork() : 0;
	if (pid < 0) {
	    perror("[zookd] fork");
	    close(cfd);
	    continue;
	}

	if (pid == 0) {
	    /* Child process. */
	    if (use_fork) {
		close(socket);
		close(srvfd);
	    }

	    process_client(cfd, n, urls, fds);

	    if (use_fork)
		exit(0);
	}

	if (pid > 0)
	    /* Parent process. */
	    close(cfd);
    }
}
