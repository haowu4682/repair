#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <sys/syscall.h>

#include "record.h"
#include "do_syscall.h"
#include "sysarg.h"

static void
analyze_syscall(syscall_record *r)
{
    static int seq = 0;
    
    printf("%d, %d, %s%s(%d)=%d, snap=%d",
           seq ++,
           r->pid, r->enter ? ">" : "<", r->scname, r->scno, (int) r->ret, r->snap.id );

    for (int i = 0; i < r->args.args_len; i++) {
	long v = 0;
	void *ptr = r->args.args_val[i].data.data_val;
	int p_len = r->args.args_val[i].data.data_len;
	memcpy(&v, ptr, MIN(p_len, sizeof(v)));
	printf(", 0x%lx", v);

	printf("[");
	for (int j = 0; j < MIN(p_len, sizeof(v)); j++)
	    printf("%c", isprint(((char*)ptr)[j]) ? ((char*)ptr)[j] : '.');
	printf("]");

	if (r->args.args_val[i].fd_stat)
	    printf("[stat:%lx:%lx:%lx]",
		   r->args.args_val[i].fd_stat->dev,
		   r->args.args_val[i].fd_stat->ino,
		   r->args.args_val[i].fd_stat->gen);
    }

    if (r->enter) {
	switch (r->scno) {
	case SYS_execve:
	case SYS_open:
	    printf(", %s", r->args.args_val[0].data.data_val);

	    if (!r->args.args_val[0].namei)
		fprintf(stderr, "Warning: no namei info for %s\n", r->scname);
	    else
		for (int i = 0; i < r->args.args_val[0].namei->ni.ni_len; i++)
		    printf(", %lx:%lx:%lx:%s",
			   r->args.args_val[0].namei->ni.ni_val[i].dev,
			   r->args.args_val[0].namei->ni.ni_val[i].ino,
			   r->args.args_val[0].namei->ni.ni_val[i].gen,
			   r->args.args_val[0].namei->ni.ni_val[i].name);

	    break;
	}
    }

    if (!r->enter) {
	switch (r->scno) {
	case SYS_wait4:
	    assert(r->args.args_val[1].data.data_len == sizeof(int));
	    printf(", %x", *(int*)r->args.args_val[1].data.data_val);
	case SYS_clone:
	    printf(", %lx", r->ret);
	    break;

	case SYS_open:
	    printf(", %lx:%lx:%lx:%lx",
		   r->ret,
		   r->ret_fd_stat ? r->ret_fd_stat->dev : 0,
		   r->ret_fd_stat ? r->ret_fd_stat->ino : 0,
		   r->ret_fd_stat ? r->ret_fd_stat->gen : 0);
	    break;
	}
    }

    printf("\n");
}

int
main(int ac, char **av)
{
	char *filename;
	switch (ac) {
	case 2:
		filename = av[1];
		break;
	default:
		fprintf(stderr, "Usage: %s record.log\n", av[0]);
		exit(-1);
    }

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
	perror("open");
	exit(-1);
    }

    FILE *f = fdopen(fd, "r");
    if (!f) {
	perror("fdopen");
	exit(-1);
    }

    XDR xdrs;
    xdrstdio_create(&xdrs, f, XDR_DECODE);

    for (;;) {
	syscall_record r;
	memset(&r, 0, sizeof(r));

	if (!xdr_syscall_record(&xdrs, &r))
	    break;

	analyze_syscall(&r);

	xdr_free((xdrproc_t) &xdr_syscall_record, (char *) &r);
    }

    return 0;
}
