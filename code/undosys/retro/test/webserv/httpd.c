#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>

enum { use_fork = 0 };

static int cur_fd;	/* connection being currently handled */

static int retro_rerun = 0;
static char* retro_fifo_in  = 0;
static char* retro_fifo_out = 0;

static char* retro_inf = "/tmp/retro/note";

static void
check_retro_rerun() {
    
    if (getenv("RETRO_RERUN")) {
        retro_rerun = 1;
	asprintf(&retro_fifo_in, "%s.in", getenv("RETRO_RERUN"));
	asprintf(&retro_fifo_out, "%s.out", getenv("RETRO_RERUN"));
	if (access(retro_fifo_in, R_OK) != 0 ||
	    access(retro_fifo_out, W_OK) != 0) {
	    perror("access");
	    exit(-1);
	}
    }
}

static int
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

static void
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

static char *
parse_req(char *reqpath)
{
    static char buf[8192];	/* static variables are not on the stack */
    clearenv();

    if (read_line(&buf[0], sizeof(buf), cur_fd) < 0)
	return "Socket IO error";

    /* Parse request like "GET /foo.html HTTP/1.0" */
    char *sp1 = strchr(&buf[0], ' ');
    if (!sp1)
	return "Cannot parse HTTP request (1)";
    *sp1 = '\0';
    sp1++;

    char *sp2 = strchr(sp1, ' ');
    if (!sp2)
	return "Cannot parse HTTP request (2)";
    *sp2 = '\0';
    sp2++;

    /* We only support GET and POST requests */
    if (strcmp(&buf[0], "GET") && strcmp(&buf[0], "POST"))
	return "Unsupported request (not GET or POST)";

    setenv("REQUEST_METHOD", &buf[0], 1);
    setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);
    setenv("REDIRECT_STATUS", "200", 1);

    /* Decode URL escape sequences in the requested path into reqpath */
    url_decode(sp1, reqpath);

    /* Parse out query string, e.g. "foo.py?user=bob" */
    char *qp = strchr(reqpath, '?');
    if (qp) {
	*qp = '\0';
	setenv("QUERY_STRING", qp+1, 1);
    }

    setenv("SCRIPT_NAME", reqpath, 1);

    /* Now parse HTTP headers */
    for (;;) {
	if (read_line(&buf[0], sizeof(buf), cur_fd) < 0)
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
	char value[512];
	url_decode(sp, &value[0]);

	/* Store header in env. variable for application code */
	char envvar[256];
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

static void
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

static void
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

static void
log_record(pid_t pid, const char *type, const char *subtype, const char *data)
{
    char * outfile = "/tmp/retro/httpd.log";
    if (retro_rerun)
        outfile = retro_fifo_out;
	
    FILE *f = fopen(outfile, "a");
    if (!f)
	return;

    struct timeval tv;
    gettimeofday(&tv, 0);
    uint64_t ts = ((uint64_t)tv.tv_sec) * 1000000 + tv.tv_usec;

    fprintf(f, "%d %lld %s %s ", pid, ts, type, subtype);
    for (int i = 0; i < (data ? strlen(data) : 0); i++) {
	if (isalnum(data[i]))
	    fprintf(f, "%c", data[i]);
	else
	    fprintf(f, "%%%02x", ((unsigned)data[i]));
    }
    fprintf(f, "\n");

    fclose(f);
}

static void
log_cwd(pid_t pid)
{
    char *cwd = get_current_dir_name();
    log_record(pid, "httpreq_start", "x", "x");
    log_record(pid, "httpreq_cwd", "x", cwd);
    free(cwd);    
}

static void
log_env(pid_t pid)
{
    for (char **p = environ; *p; p++) {
	char buf[1024];
	strncpy(&buf[0], *p, sizeof(buf));
	buf[sizeof(buf)-1] = '\0';
	char *eq = strchr(buf, '=');
	if (!eq) continue;
	*eq = 0;
	log_record(pid, "httpreq_env", buf, getenv(buf));
    }    
}

static void
process_post_data(pid_t pid, int sock, int phpin)
{
    if (!strcmp(getenv("REQUEST_METHOD"), "POST") && 
	getenv("CONTENT_LENGTH")) {
	
	int postlen = strtol(getenv("CONTENT_LENGTH"), 0, 10);
	char * postbuf = malloc(postlen+1);
	read(sock, postbuf, postlen);
	if (!retro_rerun)
	    write(phpin, postbuf, postlen);
	postbuf[postlen] = '\0';
	log_record(pid, "httpreq_post", "x", postbuf);
	free(postbuf);
    }
    log_record(pid, "httpreq_end", "x", "x");
}

static void
process_response(pid_t pid, int sock, int phpout)
{
    int resplen = 1024;
    char *respbuf = malloc(resplen);
    int respcc = 0;

    for (;;) {
	if (respcc >= resplen) {
	    resplen = resplen * 2;
	    respbuf = realloc(respbuf, resplen);
	}

	int cc = read(phpout, &respbuf[respcc], resplen - respcc);
	if (cc <= 0)
	    break;

	write(sock, &respbuf[respcc], cc);
	respcc += cc;
    }

    if (!retro_rerun)
	log_record(pid, "httpresp", "x", respbuf);
	
    free(respbuf);
}

static char prev_pageid[1024];
/* XXX: snapshot periodically instead of after every page load */
static void
snapshot_db(const char *dir) {
    char *pageid = getenv("HTTP_X_PAGE_ID");
    if (pageid && strncmp(prev_pageid, pageid, 1024)) {
	char cmd[4096];
	snprintf(cmd, 4096, "%s/db-checkpoint.py %s/db/zoobar", dir, dir);
	system(cmd);
	strncpy(prev_pageid, pageid, 1024);
    }
}

static void
process_client(const char *dir, int fd)
{
    char reqpath[256];
    cur_fd = fd;

    char *err = parse_req(&reqpath[0]);
    if (err) {
	http_err(fd, 500, "Error parsing request: %s", err);
	return;
    }

    char pn[1024];
    sprintf(&pn[0], "%s/%s", dir, reqpath);
    struct stat st;
    if (stat(pn, &st) < 0) {
	http_err(fd, 404, "File not found or not accessible: %s", pn);
	return;
    }

    if (S_ISDIR(st.st_mode)) {
	/* For directories, use index.html in that directory */
	strcat(pn, "/index.html");
	if (stat(pn, &st) < 0) {
	    strcpy(pn + strlen(pn) - 4, "php");
	    if (stat(pn, &st) < 0) {
		pn[strlen(pn) - 9] = '\0';
		http_err(fd, 404, "File not found or not accessible: %s", pn);
		return;
	    }
	}
    }

    const char *ext = strrchr(pn, '.');
    fdprintf(fd, "HTTP/1.0 200 OK\r\n");

    if (S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR)) {
	/* executable bits -- run as CGI script */
	setenv("SCRIPT_FILENAME", pn, 1);

	if (!retro_rerun) {
	    int p_out[2];
	    if (pipe(p_out) < 0) {
		http_err(fd, 500, "Cannot pipe: %s", strerror(errno));
		return;
	    }
    
	    int p_in[2];
	    if (pipe(p_in) < 0) {
		http_err(fd, 500, "Cannot pipe: %s", strerror(errno));
		return;
	    }
    
	    signal(SIGCHLD, SIG_DFL);
	    int pid = fork();
	    if (pid < 0) {
		http_err(fd, 500, "Cannot fork: %s", strerror(errno));
		return;
	    }
    
	    if (pid > 0) {
		close(p_out[1]);
		close(p_in[0]);
    
		snapshot_db(dir);
		log_cwd(pid);
		log_env(pid);
		process_post_data(pid, fd, p_in[1]);
		process_response(pid, fd, p_out[0]);
		    
		close(p_in[1]);
		close(p_out[0]);
	    }
    
	    if (pid == 0) {
		/* Child process */
		close(p_out[0]);
		close(p_in[1]);
    
		dup2(p_in[0], 0);
		dup2(p_out[1], 1);
		close(fd);
    
		execl(pn, pn, 0);
		perror("execl");
		exit(-1);
	    }
    
	    int status;
	    waitpid(pid, &status, 0);
	} else {
	    int pid = getpid();
	    log_cwd(pid);
	    log_env(pid);
	    process_post_data(pid, fd, 0);
	    
	    int fifo_in = open(retro_fifo_in, O_RDONLY);
	    process_response(pid, fd, fifo_in);
	    close(fifo_in);
	}
	
	shutdown(fd, SHUT_RDWR);
	close(fd);
    } else {
	/* Non-executable: serve contents */
	int filefd = open(pn, O_RDONLY);
	if (filefd < 0) {
	    http_err(fd, 500, "Cannot open %s: %s", pn, strerror(errno));
	    return;
	}

	const char *mimetype = "text/html";
	if (ext && !strcmp(ext, ".css"))
	    mimetype = "text/css";
	if (ext && !strcmp(ext, ".jpg"))
	    mimetype = "image/jpeg";

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
}

int
main(int ac, const char **av)
{
    if (ac != 3) {
	fprintf(stderr, "Usage: %s port dir\n", av[0]);
	exit(-1);
    }

    int port = atoi(av[1]);
    const char *dir = av[2];

    if (!port) {
	fprintf(stderr, "Bad port number: %s\n", av[1]);
	exit(-1);
    }

    /* record logs from httpd */
    {
	    FILE *f = fopen(retro_inf, "w");
	    if (f) {
		    fprintf(f, "httpd");
		    fclose(f);
	    }
    }    
        
    check_retro_rerun();
    
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    int srvfd = socket(AF_INET, SOCK_STREAM, 0);
    if (srvfd < 0) {
	perror("socket");
	exit(-1);
    }

    int on = 1;
    if (setsockopt(srvfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
	perror("setsockopt SO_REUSEADDR");
	exit(-1);
    }

    if (bind(srvfd, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
	perror("bind");
	exit(-1);
    }

    listen(srvfd, 5);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    for (;;) {
	struct sockaddr_in client_addr;
	unsigned int addrlen = sizeof(client_addr);

	int cfd = accept(srvfd, (struct sockaddr *) &client_addr, &addrlen);
	if (cfd < 0) {
	    perror("accept");
	    continue;
	}

	int pid = use_fork ? fork() : 0;
	if (pid < 0) {
	    perror("fork");
	    close(cfd);
	    continue;
	}

	if (pid == 0) {
	    /* Child process. */
	    if (use_fork)
		close(srvfd);

	    process_client(dir, cfd);

	    if (use_fork)
		exit(0);
	}

	if (pid > 0) {
	    /* Parent process. */
	    close(cfd);
	}
    }
}
