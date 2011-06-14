#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/syscall.h>

extern "C" {
  # include "record.h"
  # include "do_syscall.h"
  # include "sysarg.h"
}

#include <sstream>
#include <iostream>
#include <map>

using namespace std;

// set debug level (0 means nothing)
const int debug = 0;

#define dbg( level, args )                            \
    do {                                              \
        if ( level < debug ) {                        \
            cout << " [" << level << "] " << __func__ \
                 << "-" << __LINE__                   \
                 << ": " << args << endl;             \
        }                                             \
    } while ( false )

#define err( args )                                   \
    do {                                              \
        cout << " [!] " << __func__ << "-" <<__LINE__ \
             << ": " << args << endl;                 \
    } while ( false )

//
// replay_fds[ pid ][ fd ] = fd (real)
//

typedef map< int, map< int, int > > replay_fds_t;

replay_fds_t replay_fds;

static int
fd_lookup(int orig_pid, int orig_fd)
{
    int fd = replay_fds[ orig_pid ][ orig_fd ];
    if ( fd == 0 ) {
        dbg( 4, orig_pid << " can't find " << orig_fd );
        return -1;
    }

    return fd;
}

static void
fd_close(int orig_pid, int orig_fd)
{
    int fd = replay_fds[ orig_pid ][ orig_fd ];
    if ( fd == 0 ) {
        err( orig_pid << " can't find " << orig_fd );
        return;
    }

    dbg( 4, orig_pid << " closed " << orig_fd );

    map<int,int> & proc = replay_fds[ orig_pid ];
    proc.erase( proc.find( orig_fd ) );

    close( fd );
}

static void
fd_put(int orig_pid, int orig_fd, int replay_fd)
{
    int fd = replay_fds[ orig_pid ][ orig_fd ];
    if ( fd > 0 ) {
        dbg( 3, orig_pid << "/" << orig_fd << " already mapped to " << fd );
        fd_close( orig_pid, orig_fd );
    }

    replay_fds[ orig_pid ][ orig_fd ] = replay_fd;
}

static void
fd_close_proc(int orig_pid)
{
    map<int,int> proc = replay_fds[ orig_pid ];

    map<int,int>::iterator iter;
    for ( iter  = proc.begin() ;
          iter != proc.end()   ;
          iter ++ ) {

        dbg( 4, orig_pid << " closed " << iter->first );

        close( iter->second );
    }

    replay_fds.erase( replay_fds.find( orig_pid ) );
}

static void
fd_clone(int parent_pid, int child_pid)
{
    map<int,int> proc = replay_fds[ parent_pid ];
    map<int,int>::iterator iter;
    for ( iter  = proc.begin() ;
          iter != proc.end() ;
          iter ++ ) {
        fd_put( child_pid, iter->first, dup( iter->second ) );
    }
}

static void
fd_dup(int pid, int old_fd, int new_fd)
{
    int fd = dup( fd_lookup( pid, old_fd ) );
    if ( fd < 0 ) {
        err( pid << " failed to dup " << new_fd << " from " << old_fd );
    }

    fd_put( pid, new_fd, fd );
}

//
// create stdin and stdout of the initial process
//
static void
fd_init(int pid)
{
    //
    // TODO we might lose these data?
    //
    int stdin  = open( "replay.in", O_CREAT | O_RDWR, 0644 );
    int stdout = open( "replay.out", O_CREAT | O_RDWR, 0644 );

    if ( stdin == -1 || stdout == -1 ) {
        err( "can't initialize in/out fd" );
        exit(-1);
    }

    fd_put( pid, 0, stdin );
    fd_put( pid, 1, stdout );
}

static long
issue_syscall(syscall_record *r)
{
    long args[6];
    long scno = r->scno;

    const struct sysarg_call *sa = &sysarg_calls[0];
    while (sa->name) {
        if (!strcmp(r->scname, sa->name))
            break;
        sa++;
    }
    assert(sa->name);

    memset(&args[0], 0, sizeof(args));
    for (int i = 0; sa->args[i].type != sysarg_end; i++) {
        switch (sa->args[i].type) {
        case sysarg_int:
            args[i] = *(long*)r->args.args_val[i].data.data_val;
            break;

        case sysarg_strnull:
        case sysarg_buf_arglen:
        case sysarg_buf_arglenp:
        case sysarg_buf_fixlen:
            args[i] = (long)r->args.args_val[i].data.data_val;
            break;

        default:
            assert(0);
        }
    }

    long ret = do_syscall(args[0], args[1], args[2],
                          args[3], args[4], args[5], scno);
    return ret;
}


static string
syscall_to_string(syscall_record *r)
{
    stringstream ss;

    const struct sysarg_call *sa = &sysarg_calls[0];
    while (sa->name) {
        if (!strcmp(r->scname, sa->name))
            break;
        sa++;
    }
    assert(sa->name);

    ss << sa->name << "(";

    for ( int i = 0 ; sa->args[i].type != sysarg_end ; i ++ ) {
        if ( i != 0 ) {
            ss << ",";
        }

        switch (sa->args[i].type) {
        case sysarg_int:
            ss << *(long*)r->args.args_val[i].data.data_val;
            break;

        case sysarg_strnull:
        case sysarg_buf_arglen:
        case sysarg_buf_arglenp:
        case sysarg_buf_fixlen:
            ss << (long)r->args.args_val[i].data.data_val;
            break;

        // default:
        //     assert(0);
        }
    }

    ss << ") = " << (int)(r->ret);
    
    return ss.str();
}

static void
translate_fd(syscall_record *r, int argnum)
{
    int *fdp = (int*)r->args.args_val[argnum].data.data_val;
    int fd_orig = *fdp;

    if (fd_orig == AT_FDCWD &&
        !strcmp(r->scname + strlen(r->scname) - 2, "at"))
    {
        /* leave it at AT_FDCWD; need to separately track cwd anyway */
    } else {
        *fdp = fd_lookup(r->pid, *fdp);
        if (*fdp < 0)
            fprintf(stderr, "[%d] fd %d not found for syscall %s\n",
                    r->pid, fd_orig, r->scname);
    }
}

static void
analyze_syscall(syscall_record *r)
{
    dbg( 8, syscall_to_string( r ) );

    if (r->enter) {
        switch (r->scno) {
        case SYS_close: {
            int fd = *(int*)r->args.args_val[0].data.data_val;
            fd_close(r->pid, fd);
            break;
        }

        case SYS_exit:
        case SYS_exit_group:
            fd_close_proc(r->pid);
            break;

        case SYS_write:
        case SYS_unlinkat:
            translate_fd(r, 0);
            issue_syscall(r);
            break;

        case SYS_unlink:
            issue_syscall(r);
            break;
        }
    } else if (r->ret >= 0) {
        switch (r->scno) {
        case SYS_open: {
            int fd = issue_syscall(r);
            if (fd < 0)
                fprintf(stderr, "open replay mismatch\n");

            fd_put(r->pid, r->ret, fd);
            break;
        }

        case SYS_fcntl: {
            int old_fd = *(int*)r->args.args_val[0].data.data_val;
            int arg1   = *(int*)r->args.args_val[1].data.data_val;

            if ( (arg1 == F_DUPFD) && r->ret >= 0 ) {
                fd_dup( r->pid, old_fd, r->ret );
            }
        }

        case SYS_dup2: {
            int old_fd = *(int*)r->args.args_val[0].data.data_val;
            int new_fd = *(int*)r->args.args_val[1].data.data_val;

            fd_dup( r->pid, old_fd, new_fd );

            break;
        }

        case SYS_clone: {
            int new_pid = r->ret;
            fd_clone(r->pid, new_pid);
            break;
        }
        }
    }
}

int
main(int ac, char **av)
{
    int fd = open("/tmp/record.log", O_RDONLY);
    if (fd < 0) {
        perror("open /tmp/record.log");
        exit(-1);
    }

    FILE *f = fdopen(fd, "r");
    if (!f) {
        perror("fdopen");
        exit(-1);
    }

    XDR xdrs;
    xdrstdio_create(&xdrs, f, XDR_DECODE);

    if (chdir("../test") < 0) {
        perror("chdir");
        exit(-1);
    }

    for ( int turn = 0 ;; turn ++ ) {
        syscall_record r;
        memset(&r, 0, sizeof(r));

        if (!xdr_syscall_record(&xdrs, &r))
            break;

        // initailize stdin/stdout
        if ( turn == 0 ) {
            fd_init( r.pid );
        }

        analyze_syscall(&r);

        xdr_free((xdrproc_t) &xdr_syscall_record, (char *) &r);
    }

    return 0;
}
