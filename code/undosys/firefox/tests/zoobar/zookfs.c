#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include "utility.h"

static void
process_client(int fd)
{
    char *err = parse_http_header(fd);
    if (err) {
	http_err(fd, 500, "[zookfs] error parsing header: %s", err);
	return;
    }

    char *pn = getenv("SCRIPT_NAME");
    struct stat st;
    if (stat(pn, &st) < 0) {
	http_err(fd, 404, "File not found or not accessible: %s", pn);
	return;
    }

    if (S_ISDIR(st.st_mode)) {
	/* For directories, use index.html in that directory */
	if (pn[strlen(pn) - 1] != '/')
	    strcat(pn, "/");
	strcat(pn, "index.html");
	if (stat(pn, &st) < 0) {
	    http_err(fd, 404, "File not found or not accessible: %s", pn);
	    return;
	}
    }


    if (S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR)) {
	fdprintf(fd, "HTTP/1.0 200 OK\r\n");
	/* executable bits -- run as CGI script */
	dup2(fd, 0);
	dup2(fd, 1);
	close(fd);
	execl(pn, pn, 0);
	perror("execl");
	http_err(1, 500, "Cannot execute %s: %s", pn, strerror(errno));
	exit(-1);
    }

    /* Non-executable: serve contents */
    int filefd = open(pn, O_RDONLY);
    if (filefd < 0) {
	http_err(fd, 500, "Cannot open %s: %s", pn, strerror(errno));
	return;
    }

    const char *ext = strrchr(pn, '.');
    const char *mimetype = "text/html";
    if (ext && !strcmp(ext, ".css"))
	mimetype = "text/css";

    fdprintf(fd, "HTTP/1.0 200 OK\r\n");
    fdprintf(fd, "Content-Type: %s\r\n", mimetype);
    fdprintf(fd, "\r\n");

    for (;;) {
	char readbuf[1024];
	int cc = read(filefd, &readbuf[0], sizeof(readbuf));
	if (cc <= 0)
	    break;
	write(fd, &readbuf[0], cc);
    }

    close(filefd);
    close(fd);
}

int
main(int argc, char *argv[])
{
    if (argc != 2) {
	printf("zookfs must be called from zookld\n");
	exit(-1);
    }

    int socket = atoi(argv[1]);
    for (;;) {
	char buf[9000];
	int cfd = -1;
	ssize_t r = recvfd(socket, buf, sizeof(buf), &cfd);
	if (r <= 0 || 0 == memchr(buf, 0, sizeof(buf))) {
	    printf("[zookfs] recv fd: socket %d\n", socket);
	    exit(-1);
	    //continue;
	}
	signal(SIGCHLD, SIG_DFL);
	int pid = fork();
	if (pid < 0) {
	    http_err(cfd, 500, "Cannot fork: %s", strerror(errno));
	} else if (pid > 0) {
	    close(cfd);
	    int status;
	    waitpid(pid, &status, 0);
	} else {
	    // child
	    //cleanenv();
	    deserialize_env(buf, sizeof(buf));
	    process_client(cfd);
	}
    }
}
