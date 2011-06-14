#if defined(__linux) || defined(linux) || defined(DARWIN) || defined(darwin)
#define HAVE_BACKTRACE 1
#endif

#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/uio.h>
#ifdef HAVE_BACKTRACE
#include <execinfo.h>
#endif
#include "utility.h"

void
die(const char *s)
{
	perror(s);
#ifdef HAVE_BACKTRACE
	void *stack[16];
	int n = backtrace(stack, sizeof(stack)/sizeof(stack[0]));
	backtrace_symbols_fd(stack, n, fileno(stderr));
#endif
	exit(-1);
}

ssize_t
sendfd(int socket, const void *buffer, size_t length, int fd)
{
	struct iovec iov = {(void *)buffer, length};
	char buf[CMSG_LEN(sizeof(int))];
	struct cmsghdr *cmsg = (struct cmsghdr *)buf;
	cmsg->cmsg_len = sizeof(buf);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	*((int *)CMSG_DATA(cmsg)) = fd;
	struct msghdr msg = {0};
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = cmsg;
	msg.msg_controllen = cmsg->cmsg_len;
	ssize_t r = sendmsg(socket, &msg, 0);
	if (r <= 0)
		perror("sendmsg");
	return r;
}

ssize_t
recvfd(int socket, void *buffer, size_t length, int *fd)
{
	struct iovec iov = {buffer, length};
	char buf[CMSG_LEN(sizeof(int))];
	struct cmsghdr *cmsg = (struct cmsghdr *)buf;
	cmsg->cmsg_len = sizeof(buf);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	struct msghdr msg = {0};
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = cmsg;
	msg.msg_controllen = cmsg->cmsg_len;
	ssize_t r = recvmsg(socket, &msg, 0);
	if (r <= 0)
		perror("recvmsg");
	else
		*fd = *((int*)CMSG_DATA(cmsg));
	return r;
}

int
read_line(char *buf, int size, int fd)
{
    int i = 0;

    for (;;) {
	int cc = read(fd, &buf[i], 1);
	if (cc <= 0)
	    break;

	if (buf[i] == '\r') {
	    buf[i] = '\0';	/* skip */
	    continue;
	}

	if (buf[i] == '\n') {
	    buf[i] = '\0';
	    return 0;
	}

	if (i >= size - 1) {
	    buf[i] = '\0';
	    return 0;
	}

	i++;
    }

    return -1;
}

void
url_decode(char *src, char *dst)
{
    for (;;) {
	if (src[0] == '%' && src[1] && src[2]) {
	    char hexbuf[3];
	    hexbuf[0] = src[1];
	    hexbuf[1] = src[2];
	    hexbuf[2] = '\0';

	    *dst = strtol(&hexbuf[0], 0, 16);
	    src += 3;
	} else {
	    *dst = *src;
	    src++;

	    if (*dst == '\0')
		break;
	}

	dst++;
    }
}

void
fdprintf(int fd, char *fmt, ...)
{
    char *s = 0;

    va_list ap;
    va_start(ap, fmt);
    vasprintf(&s, fmt, ap);
    va_end(ap);

    write(fd, s, strlen(s));
    free(s);
}

void
http_err(int fd, int code, char *fmt, ...)
{
    fdprintf(fd, "HTTP/1.0 %d Error\r\n", code);
    fdprintf(fd, "Content-Type: text/html\r\n");
    fdprintf(fd, "\r\n");
    fdprintf(fd, "<H1>An error occurred</H1>\r\n");

    char *msg = 0;
    va_list ap;
    va_start(ap, fmt);
    vasprintf(&msg, fmt, ap);
    va_end(ap);

    fdprintf(fd, "%s", msg);
    free(msg);

    close(fd);
}

void
deserialize_env(const char *env, size_t len)
{
	for (;;)
	{
		char *p = strchr(env, '=');
		if (p == 0 || p - env > len)
			break;
		*p++ = 0;
		setenv(env, p, 1);
		//fprintf(stderr, "%s=%s\n", env, p);
		p += strlen(p)+1;
		len -= (p - env);
		env = p;
	}
	setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);
	setenv("SCRIPT_FILENAME", getenv("SCRIPT_NAME"), 1);
}

char *
parse_http_header(int fd)
{
    static char buf[8192];	/* static variables are not on the stack */

    /* Now parse HTTP headers */
    for (;;) {
	if (read_line(&buf[0], sizeof(buf), fd) < 0)
	    return "Socket IO error";

	if (buf[0] == '\0')	/* end of headers */
	    break;

	/* Parse things like "Cookie: foo bar" */
	char *sp = strchr(&buf[0], ' ');
	if (!sp)
	    return "Header parse error (1)";
	*sp = '\0';
	sp++;

	/* Strip off the colon, making sure it's there */
	if (strlen(buf) == 0)
	    return "Header parse error (2)";

	char *colon = &buf[strlen(buf) - 1];
	if (*colon != ':')
	    return "Header parse error (3)";
	*colon = '\0';

	/* Set the header name to uppercase */
	for (int i = 0; i < strlen(buf); i++)
	    buf[i] = toupper(buf[i]);

	/* Decode URL escape sequences in the value */
	char value[8192];
	url_decode(sp, &value[0]);

	/* Store header in env. variable for application code */
	char envvar[8192];
	sprintf(&envvar[0], "HTTP_%s", buf);
	setenv(envvar, value, 1);

	/* Some special headers go into env. vars of their own */
	if (!strcasecmp(buf, "Content-Type"))
	    setenv("CONTENT_TYPE", value, 1);
	if (!strcasecmp(buf, "Content-Length"))
	    setenv("CONTENT_LENGTH", value, 1);
    }

    return 0;
}
