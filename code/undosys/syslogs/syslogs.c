#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/unistd.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/vmalloc.h>
#include <linux/fdtable.h>
#include <linux/file.h>
#include <linux/path.h>
#include <linux/fs_struct.h>
#include <linux/namei.h>
#include <linux/splice.h>
#include <linux/dcache.h>

#include <asm/syscalls.h>
#include <asm/cacheflush.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/uaccess.h>
#include <asm/mman.h>

#include <linux/syscalls.h>
#include <linux/smp_lock.h>

#include "syslogs_config.h"
#include "btrfs_ioctl.h"
#include "record.h"

#include "sysarg.h"
#include "undosysarg.h"

// ======================================================================
// configuration
// ----------------------------------------------------------------------

// /proc/undosys/record buffer size
#define RECORD_BUF_SIZE   (60000000)

// record buffer margin as safy factor
#define RECORD_BUF_MARGIN (30000000)

// record archive
#define RECORD_ARCHIVE "/tmp/record"

// reloadable #syscalls
// #define TEST_WITH_MINIMAL_SYSCALL

#define BEST_PERFORMANCE
// ======================================================================

// memory related workarounds
static int syslogs_set_memory_rw( unsigned long address );
static int syslogs_check_memory( unsigned long address );

// syscall args
extern struct sysarg_call sysarg_calls[512];
extern int nsysarg_calls[512];

extern void init_syscalls( void );

// dump buffer
static void dump( char * buf, int len );

// file related utils
static long vfs_ioctl( struct file * filp, unsigned int cmd, unsigned long arg );
static ssize_t copy_file_with_path( struct stat_info ** pp, int snap_id, char * src );
static ssize_t copy_file_with_fd( struct stat_info ** pp, int snap_id, int in_fd );
static ssize_t copy_file_with_file( struct stat_info ** pp, int snap_id, struct file * in_file );

static int record_snap_stat_with_path( struct stat_info ** pp, int snap_id, char * src );
static int record_snap_stat_with_fd( struct stat_info ** pp, int snap_id, int in_fd );

typedef int (*rw_verify_area_t)( int read_write,
                                 struct file *file,
                                 loff_t *ppos,
                                 size_t count );

static rw_verify_area_t _rw_verify_area = (rw_verify_area_t) __rw_verify_area;

typedef int (*sys_link_t)( const char * oldname, const char * newname );

static sys_link_t _sys_link = (sys_link_t)( __sys_link );

//
//
// we can find the sys_call_table by scanning bytes signatures but ..
//
static unsigned long * sys_call_table   = (unsigned long *) __sys_call_table;
static unsigned long * sys_call_table32 = (unsigned long *) __ia32_sys_call_table;

//
// syscall32 to restore
//
unsigned long orig_sys_call_table32[512];

#ifndef BEST_PERFORMANCE

 enum { namei_debug    = 0 };
 enum { stat_debug     = 0 };
 enum { xdr_debug      = 1 };
 enum { xdr_arg_debug  = 0 };
 enum { proc_debug     = 0 };
 enum { copy_debug     = 0 };
 enum { hook_debug     = 1 };
 enum { cache_debug    = 0 };
 enum { exec_debug     = 0 };
 enum { sched_debug    = 1 };
 enum { snap_debug     = 1 };
 enum { linkpath_debug = 0 };
 enum { dirents_debug  = 1 };

# define dbg( filter, msg, ... )                \
    do {                                        \
        if ( filter ) {                         \
            printk( KERN_INFO "%s(%d): " msg,   \
                    __FUNCTION__,               \
                    __LINE__,                   \
                    __VA_ARGS__ );              \
        }                                       \
    } while( 0 )

# define ifdbg( filter, statements )            \
    do {                                        \
        if ( filter ) {                         \
            (statements);                       \
        }                                       \
    } while ( 0 )

#else

# define dbg( filter, msg, ... )                \
    do {                                        \
    } while( 0 )

# define ifdbg( filter, statements )            \
    do {                                        \
    } while ( 0 )

#endif

#define err( msg, ... )                         \
    do {                                        \
        printk( KERN_INFO "[!] %s(%d): " msg,   \
                __FUNCTION__,                   \
                __LINE__,                       \
                __VA_ARGS__ );                  \
    } while( 0 )

// ======================================================================
// trace related

// task tracing flags
#define SYSLOG_PID_TRACING (0x80)

#define is_tracing() (current->flags & SYSLOG_PID_TRACING)

static inline
int trace( int pid )
{
    struct task_struct * task
        = pid_task( find_get_pid( pid ), PIDTYPE_PID );

    if ( task ) {
        task->flags |= SYSLOG_PID_TRACING;
    } else {
        err( "failed to trace pid: %d", task->pid );
        return 0;
    }

    return 1;
}

// ======================================================================
// cache (vmalloc substitute)

#include "cache.h"

DEF_CACHE( syscall_arg   , 8  );
DEF_CACHE( argv_str      , 150);
DEF_CACHE( namei_infos   , 1  );
DEF_CACHE( namei_info    , 15 );
DEF_CACHE( stat_info     , 1  );
DEF_CACHE( string        , 100);
DEF_CACHE( exec_info     , 1  );
DEF_CACHE( big_stat_info , 300);
DEF_CACHE( dirent_infos  , 1  );

// ======================================================================
// xdr recorder

static XDR xdr;
static struct xdr_buf xdr_buf;

static char * record_buf;
static int snap_count = 0;

DECLARE_MUTEX( record_lock );
spinlock_t record_buffer_lock;

#define record( fmt, ... )                                      \
    do {                                                        \
    } while( 0 )

static void init_xdr_buf( struct xdr_buf * buf )
{
    buf->head[0].iov_base = record_buf;
    buf->head[0].iov_len = 0;
    buf->tail[0].iov_len = 0;
    buf->page_len = 0;
    buf->flags = 0;
    buf->len = 0;
    buf->buflen = RECORD_BUF_SIZE;
}

static void init_xdr( void )
{
    spin_lock_init( &record_buffer_lock );

    record_buf = vmalloc( RECORD_BUF_SIZE * sizeof( record_buf[0] ) );

    memset( record_buf, 0, sizeof( record_buf[0] ) * RECORD_BUF_SIZE );

    init_xdr_buf( &xdr_buf );
    xdr_init_encode( &xdr, &xdr_buf, NULL );
}

static void exit_xdr( void )
{
    vfree( record_buf );
}

static
void write_record_to_file( char * dir )
{
    int len;

    struct file * fd;
    struct timespec ts;

    char record_file[512];

    mm_segment_t old_fs;

    struct path root;
    struct path old_root;

    // fetch root directory
    root.mnt    = current->fs->root.mnt;
    root.dentry = current->fs->root.dentry->d_sb->s_root;

    // change root directory (for chroot)
    write_lock( &current->fs->lock );
    old_root = current->fs->root;
    current->fs->root = root;
    path_get( &root );
    write_unlock( &current->fs->lock );

    ts = current_kernel_time();
    snprintf( record_file, 512, "%s/%d.%ld.log", dir, (int) ts.tv_sec, ts.tv_nsec );

    dbg( sched_debug, "write: %s", record_file );

    old_fs = get_fs();
    set_fs( KERNEL_DS );

    fd = filp_open( record_file, O_CREAT | O_RDWR, 0644 );
    len = (int)( (unsigned long) xdr.p - (unsigned long) record_buf );
    if ( !IS_ERR( fd ) ) {
        vfs_write( fd, record_buf, len, &fd->f_pos );
        xdr.p = (__be32 *)( (unsigned long) record_buf );
    } else {
        err( "error to open %s(len:%d, err:%d)", record_file, len, (int) fd );
    }

    set_fs( old_fs );

    // restore old root (for chroot)
    write_lock( &current->fs->lock );
    current->fs->root = old_root;
    write_unlock( &current->fs->lock );

    path_put( &root );
}


static
bool_t safe_xdr_syscall_record( XDR * xdrs, syscall_record * objp )
{
    unsigned long flags;
    int len;
    bool_t rtn;

    down( &record_lock );

    spin_lock_irqsave( &record_buffer_lock, flags );

    // record buffer size
    len = (int)((unsigned long) xdr.p - (unsigned long) record_buf);

    if ( len > RECORD_BUF_SIZE - RECORD_BUF_MARGIN ) {
        dbg( sched_debug, "record log size: %d", len );
        write_record_to_file( RECORD_ARCHIVE );
    }

    rtn = xdr_syscall_record( xdrs, objp );

    spin_unlock_irqrestore( &record_buffer_lock, flags );

    up( &record_lock );

    return rtn;
}

// ======================================================================
// => hooked_fn ex) hooked_sys_open, hooked_sys_close ..
//
// 1) add syscall functions to HOOKS (ex) FN(close)
// 2) define hooked_close()
//
// => clone, open, read, write, execve, wait4, unlinkat,

// FIXME change it to __NR_syscall_max + 1
static unsigned long map_syscalls[ 512 ] = {0,};

//
// add new syscalls here
//

#ifdef TEST_WITH_MINIMAL_SYSCALL
# include "syscall_hooks_min.h"
#else
# include "syscall_hooks.h"
#endif

//
// NOTE. special care on entry stub
//
// cat unistd_64.h | grep stub
//
// __SYSCALL(__NR_rt_sigreturn, stub_rt_sigreturn)
// __SYSCALL(__NR_clone, stub_clone)
// __SYSCALL(__NR_fork, stub_fork)
// __SYSCALL(__NR_vfork, stub_vfork)
// __SYSCALL(__NR_execve, stub_execve)
// __SYSCALL(__NR_sigaltstack, stub_sigaltstack)
// __SYSCALL(__NR_iopl, stub_iopl)
//

// template to hook normal syscalls
#define HOOK_SYSCALL( name )                    \
    do {                                        \
        unsigned long flags;                    \
        map_syscalls[ __NR_##name ]             \
            = sys_call_table[ __NR_##name ];    \
                                                \
        dbg( hook_debug, "Hooking:%s(%d)",      \
             #name, __NR_##name );              \
                                                \
        local_irq_save( flags );                \
        sys_call_table[ __NR_##name ]           \
            = (unsigned long) hooked_##name;    \
        local_irq_restore( flags );             \
                                                \
    } while ( 0 )

// template to hook stub syscalls
#define HOOK_SYSCALL_S( name ) \
    do {                       \
        install_##name();      \
    } while( 0 )

// template to unhook normal syscalls
#define UNHOOK_SYSCALL( name )                  \
    do {                                        \
        unsigned long flags;                    \
                                                \
        local_irq_save( flags );                \
        sys_call_table[ __NR_##name ]           \
            = map_syscalls[ __NR_##name ];      \
        local_irq_restore( flags );             \
                                                \
    } while ( 0 )

// template to unhook stub syscalls
#define UNHOOK_SYSCALL_S( name ) \
    do {                         \
        uninstall_##name();      \
    } while ( 0 )

// return from hooked_* functions
#define HOOK_RTN( name ) \
    (( sys_##name##_t )( map_syscalls[ __NR_##name ] ))

// generators with turning instrrupts off
#define HOOK_SYSCALLS()                         \
    do {                                        \
         HOOKS( HOOK_SYSCALL );                  \
     } while(0)

#define UNHOOK_SYSCALLS()                       \
    do {                                        \
        HOOKS( UNHOOK_SYSCALL );                \
    } while(0)

// ======================================================================
//
// hooking function
//
// 1) filter processes (/proc) <= pid hash
// 2) record to file? memory?
// 3) managing snap id
//

//
// NOTE. rcu_read_lock is unnecessary for real undo system
//       because we will prevent anyone from unloading module
//
#define DEF_HOOK_FN( name, ... )                                 \
    asmlinkage long hooked_##name( __VA_ARGS__ ) {               \
        typedef long (* sys_##name##_t)( __VA_ARGS__ );          \
        long rtn;                                                \
        int  post_hook = 0;                                      \
        void * args[7] = {};                                     \
        syscall_record rec                                       \
            = (syscall_record){ .scno = __NR_##name,             \
                                .scname = #name    ,             \
                              };                                 \
        const int _pid = current->pid;                           \
        do

#define END_DEF_HOOK                                             \
    while(0);                                                    \
    }

//
// hooking APIs, sorry for the many prepossors
//
#define IF_TRACKING_BEG(statments)                  \
    if ( is_tracing() ) {                           \
        statments;                                  \
        post_hook = 1;                              \
    } do {} while(0)

#define IF_TRACKING_END(statments)                  \
    if ( post_hook ) {                              \
        statments;                                  \
    }                                               \
    return rtn

#define IF_TRACKING_END_WITH_RTN()                  \
    IF_TRACKING_END( record( "%d\n", rtn ) )

#define IF_TRACKING_POST(statments)                 \
    if ( is_tracing() ) {                           \
        statments;                                  \
    } return rtn

#define POST_HOOK_RTN( name, ... )                  \
    do {                                            \
        rtn = HOOK_RTN( name )( __VA_ARGS__ );      \
    } while (0)

//
// FIXME
//   rec.ret
//   rec.err
//

//
// try to make it same as 'record_syscall' in syscall.c
//

static inline
char * strdup( const char * path )
{
    const int len = strlen( path );
    char * p = (char *) get_cache_string( len + 1 );
    strncpy( p, path, len );
    p[len] = '\0';
    return p;
}

static inline
int __count_path( const char * path )
{
    int np = 0;
    char * p = (char *) path;

    while ( p && (p = strchr( p, '/' )) ) {
        p ++;
        np ++;
    }

    return np;
}

static
int __lookup_path( struct path * root, char * lookup, namei_info * ni )
{
    struct nameidata nd;
    int err = vfs_path_lookup( root->dentry, root->mnt, lookup,
                               LOOKUP_FOLLOW, &nd );

    if ( !err ) {
        ni->dev  = (unsigned long) nd.path.dentry->d_inode->i_sb->s_dev;
        ni->ino  = (unsigned long) nd.path.dentry->d_inode->i_ino;
        ni->gen  = (unsigned long) nd.path.dentry->d_inode->i_generation;
        ni->name = strdup( lookup );
    }

    dbg( namei_debug, "err:%d, nd:%s", err, nd.path.dentry->d_name.name );
    dbg( namei_debug, "dev : %ld", (unsigned long) ni->dev );
    dbg( namei_debug, "ino : %ld", (unsigned long) ni->ino );
    dbg( namei_debug, "gen : %ld", (unsigned long) ni->gen );
    dbg( namei_debug, "name: %s" , ni->name );

    if ( !err ) {
        path_put( &nd.path );
    }

    return err;
}

static
void record_namei( syscall_record * rec, void ** args, int argn )
{
    int relpath = 0;
    int nsep;
    int end;
    int i;

    struct fs_struct * fs = current->fs;
    struct path root;

    const char * path = *(char **) args[argn];
    char p[512];

    // count the number of paths (including root path)
    nsep = __count_path( path ) + 1;

    // make proper path name
    snprintf( p, sizeof(p), "%s", path );

    // get pwd/root paths
    read_lock( &fs->lock );
    if ( path[0] == '/' ) {
        root = fs->root;
        path_get( &fs->root );
    } else {
        root = fs->pwd;
        path_get( &fs->pwd  );
    }
    read_unlock( &fs->lock );

    // init namei
    rec->args.args_val[argn].namei = get_cache_namei_infos(1);
    rec->args.args_val[argn].namei->ni.ni_len = 0;
    rec->args.args_val[argn].namei->ni.ni_val = 0;

    dbg( namei_debug, "path: %s", path );
    dbg( namei_debug, "root: %s", fs->root.dentry->d_name.name );
    dbg( namei_debug, "pwd : %s", fs->pwd.dentry->d_name.name );

    // NOTE. this is not the fastest way
    //
    // see, static int __link_path_walk( const char *name,
    //                                   struct nameidata *nd )
    //

    // NOTE.
    //
    // nsep : max namei_info
    //

    //
    // to record cwd (whether ./ or none)
    //
    if ( path[0] != '/' ) {
        nsep += 1;
    }

    // allocate namei memory of rec structure
    rec->args.args_val[argn].namei->ni.ni_len = nsep;
    rec->args.args_val[argn].namei->ni.ni_val = get_cache_namei_info( nsep );

    //
    // record start (root) path first
    //
    if ( path[0] != '/' ) {
        namei_info * ni = &rec->args.args_val[argn].namei->ni.ni_val[0];
        __lookup_path( &fs->root, (char *) fs->pwd.dentry->d_name.name, ni );
        dbg( namei_debug, "0: %s", fs->pwd.dentry->d_name.name );
        relpath = 1;
    }

    end = -1;
    for ( i = relpath ; i < nsep ; i ++ ) {
        char * lookup;

        namei_info * ni = &rec->args.args_val[argn].namei->ni.ni_val[i];

        ni->dev  = 0;
        ni->ino  = 0;
        ni->gen  = 0;
        ni->name = NULL;

        // restore '/'
        if ( end != -1 ) {
            p[end] = '/';
        }

        // delete rear parts
        do {
            end ++;
        } while ( p[end] != '/' && p[end] != '\0' );

        // next end
        p[end] = '\0';

        dbg( namei_debug, "%d: %s (end:%d)", i, p, end );

        //
        // record_namei(542): 0: /
        // record_namei(542): 1: /usr
        // record_namei(542): 2: /usr/share
        // record_namei(542): 3: /usr/share/locale
        // record_namei(542): 4: /usr/share/locale/locale.alias
        //

        //
        // in case ./usr/lib (for example)
        // record "."
        //
        lookup = p;
        if ( lookup[0] == '\0' ) {
            lookup = "/";
        }

        // remove unnecessary paths
        __lookup_path( &root, lookup, ni );
    }

    // release pwd/root paths
    if ( path[0] == '/' ) {
        path_put( &fs->root );
    } else {
        path_put( &fs->pwd );
    }
}

static inline
void __record_file_stat_nomem( struct stat_info * pp, struct file * fp )
{
    if ( !fp ) {
        return;
    }

    pp->dev  = (unsigned long) fp->f_path.dentry->d_inode->i_sb->s_dev;
    pp->ino  = (unsigned long) fp->f_path.dentry->d_inode->i_ino;
    pp->gen  = (unsigned long) fp->f_path.dentry->d_inode->i_generation;
    pp->mode = (unsigned long) fp->f_path.dentry->d_inode->i_mode;
    pp->rdev = (unsigned long) fp->f_path.dentry->d_inode->i_rdev;

    pp->fd_offset = fp->f_pos;
    pp->fd_flags  = fp->f_flags;

    dbg( stat_debug, "dev:%ld, ino:%ld, gen:%ld, mode:%ld",
         (unsigned long) pp->dev,
         (unsigned long) pp->ino,
         (unsigned long) pp->gen,
         (unsigned long) pp->mode );
}


static
void __record_file_stat( struct stat_info ** pp, struct file * fp )
{
    if ( !fp ) {
        return;
    }

    (*pp) = get_cache_stat_info(1);
    __record_file_stat_nomem( *pp, fp );
}

static inline
void __record_inode_stat_nomem( struct stat_info * pp, struct inode * inode )
{
    if ( !inode ) {
        return;
    }

    pp->dev  = (unsigned long) inode->i_sb->s_dev;
    pp->ino  = (unsigned long) inode->i_ino;
    pp->gen  = (unsigned long) inode->i_generation;
    pp->mode = (unsigned long) inode->i_mode;
    pp->rdev = (unsigned long) inode->i_rdev;

    pp->fd_offset = 0;
    pp->fd_flags  = 0;

    dbg( stat_debug, "dev:%ld, ino:%ld, gen:%ld, mode:%ld",
         (unsigned long) pp->dev,
         (unsigned long) pp->ino,
         (unsigned long) pp->gen,
         (unsigned long) pp->mode );
}


static
void __record_inode_stat( struct stat_info ** pp, struct inode * inode )
{
    if ( !inode ) {
        return;
    }

    //
    // TODO fetch gen/fd_offset/fd_flags for inode data
    //
    (*pp) = get_cache_stat_info(1);
    __record_inode_stat_nomem( *pp, inode );
}

static void
record_fd_stat( struct stat_info ** pp, int fdno )
{
    struct fdtable * fdt;

    if ( current->files ) {
        spin_lock( &current->files->file_lock );

        fdt = files_fdtable( current->files );

        if ( 0 <= fdno && fdno < fdt->max_fds ) {
            __record_file_stat( pp, rcu_dereference( fdt->fd[fdno] ) );
        }

        spin_unlock( &current->files->file_lock );
    }
}

static
void record_ret_fd_stat( syscall_record * rec )
{
    if ( rec->ret < 0) {
        return;
    }

    record_fd_stat( &rec->ret_fd_stat, rec->ret );
}

static
void record_arg_fd_stat( syscall_record * rec, void ** args, int argn )
{
    int fd = *(int *) args[argn];
    if ( fd == AT_FDCWD ) {
        __record_inode_stat( &rec->ret_fd_stat, current->fs->pwd.dentry->d_inode );
    } else {
        record_fd_stat( &rec->args.args_val[argn].fd_stat, fd );
    }
}

static
void record_exec( syscall_record * rec, void ** args )
{
    int i;
    int nfd = 0;

    struct file * fd;
    struct fdtable * fdt;

    rec->execi = get_cache_exec_info(1);

    // record cwd & root
    __record_inode_stat_nomem( &rec->execi->cwd , current->fs->pwd.dentry->d_inode  );
    __record_inode_stat_nomem( &rec->execi->root, current->fs->root.dentry->d_inode );

    spin_lock( &current->files->file_lock );

    fdt = files_fdtable( current->files );

    for ( i = 0 ; i < fdt->max_fds ; i ++ ) {
        fd = rcu_dereference(fdt->fd[i]);
        if ( fd && !( fd->f_flags & O_CLOEXEC ) ) {
            nfd = i;
        }
    }

    // include the last fd (as size)
    nfd ++;

    // dbg( exec_debug, "pid:%d, fds:%d, maxfd:%d\n", current->pid, nfd, fdt->max_fds );

    if ( nfd > CACHE_UNIT_big_stat_info ) {
        err( "overflow(%d)", nfd );
        nfd = CACHE_UNIT_big_stat_info;
    }

    rec->execi->fds.fds_len = nfd;
    rec->execi->fds.fds_val = get_cache_big_stat_info( nfd );

    for ( i = 0 ; i < nfd ; i ++ ) {
        fd = rcu_dereference(fdt->fd[i]);
        if ( fd && !( fd->f_flags & O_CLOEXEC ) ) {
            __record_file_stat_nomem( &rec->execi->fds.fds_val[i], fd );
        } else {
            rec->execi->fds.fds_val[i].mode = 0;
        }
    }

    spin_unlock( &current->files->file_lock );
}

static
void fill_namei_infos( struct dentry * d, struct namei_info * ni )
{
    ni->dev  = (unsigned long) d->d_inode->i_sb->s_dev;
    ni->ino  = (unsigned long) d->d_inode->i_ino;
    ni->gen  = (unsigned long) d->d_inode->i_generation;
    ni->name = strdup( d->d_name.name );
}

static
void record_snap_dirents( syscall_record * rec,
                          const char * path1,
                          const char * path2 )
{
    int error;

    dirent_infos * di;

    struct path old_path;

    rec->snap.dirents.dirents_len = 1;
    rec->snap.dirents.dirents_val = get_cache_dirent_infos(1);

    di = &rec->snap.dirents.dirents_val[0];

    di->ni.ni_len = 2;
    di->ni.ni_val = get_cache_namei_info( di->ni.ni_len );

    error = kern_path( path1, LOOKUP_FOLLOW, &old_path );
    if ( error ) {
        di->ni.ni_len --;
    } else {
        fill_namei_infos( old_path.dentry, &di->ni.ni_val[0] );
        path_put( &old_path );
    }

    if ( path2 ) {
        error = kern_path( path2, LOOKUP_FOLLOW, &old_path );
        if ( error ) {
            di->ni.ni_len --;
        } else {
            fill_namei_infos( old_path.dentry, &di->ni.ni_val[di->ni.ni_len-1] );
            path_put( &old_path );
        }
    } else {
        di->ni.ni_len --;
        if ( di->ni.ni_len == 0 ) {
            put_cache_namei_info( di->ni.ni_val );
        }
    }
}

static
void linkpath( const char * path )
{
    char lpath[512];

    struct file * fd;
    mm_segment_t old_fs;

    old_fs = get_fs();
    set_fs( KERNEL_DS );

    fd = filp_open( path, O_RDWR, 0 );
    if ( IS_ERR( fd ) ) {
        dbg( linkpath_debug, "error to open %s", path );
        goto out;
    }

    sprintf( lpath, "/mnt/undofs/snap/%ld_%ld_%ld",
             (unsigned long) fd->f_path.dentry->d_inode->i_sb->s_dev,
             (unsigned long) fd->f_path.dentry->d_inode->i_ino,
             (unsigned long) fd->f_path.dentry->d_inode->i_generation );

    _sys_link( path, lpath );

out:
    set_fs( old_fs );
}

static
char * makeupath( char * srcpath, int srcpath_len, int pid, int fd, char * relpath )
{
    if ( relpath[0] == '/' ) {
        return relpath;
    } else {
        if ( fd == AT_FDCWD ) {
            snprintf( srcpath, srcpath_len, "/proc/%d/cwd/%s", pid, relpath );
        } else {
            snprintf( srcpath, srcpath_len, "/proc/%d/fd/%d/%s", pid, fd, relpath );
        }
    }

    return srcpath;
}

static
void xdr_fill_sysarg( syscall_record * rec, void ** args )
{
    int i;

    unsigned long arg_8b[8];

    const struct sysarg_call * syscall;
    const int argc = nsysarg_calls[ rec->scno ];

    if ( !( syscall = &sysarg_calls[ rec->scno ] ) ) {
        err( "can't find syscall '%s' = %d", rec->scname, rec->scno );
        return;
    }

    // init record
    rec->ret_fd_stat    = NULL;
    rec->execi          = NULL;
    rec->args.args_len  = 0;
    rec->args.args_val  = 0;

    // init snap info
    rec->snap.file_stat = NULL;
    rec->snap.dirents.dirents_len = 0;
    rec->snap.dirents.dirents_val = NULL;

    dbg( xdr_debug, "%c%s(cn:%d, args:%d)",
         ( rec->enter ? '>' : '<' ), rec->scname, rec->scno, argc );

    // allocate arguments
    rec->args.args_len = argc;
    rec->args.args_val = get_cache_syscall_arg( argc );

    for ( i = 0 ; i < argc ; i ++ ) {
        char * arg_val = NULL;
        int    arg_len = 0;

        const struct sysarg_arg * sysargs = &syscall->args[i];

        rec->args.args_val[i].argv.argv_len = 0;
        rec->args.args_val[i].argv.argv_val = 0;
        rec->args.args_val[i].namei   = NULL;
        rec->args.args_val[i].fd_stat = NULL;

        switch ( sysargs->type ) {
        case sysarg_int:
            arg_8b[i] = *(int *) args[i];

            arg_val = (char *) &arg_8b[i];
            arg_len = 8;

            dbg( xdr_arg_debug, "arg_int:%d", *(int *)args[i] );
            break;

        case sysarg_strnull:
            arg_val = *(char **) args[i];
            arg_len = strlen( arg_val ) + 1;
            dbg( xdr_arg_debug, "arg_str:%s", arg_val );
            break;

        case sysarg_buf_arglen_when_entering:
        case sysarg_buf_arglen_when_exiting:
        case sysarg_buf_arglen:
            arg_val = 0;
            arg_len = 0;

            if ( (sysargs->type == sysarg_buf_arglen_when_entering && !rec->enter)
                 || (sysargs->type == sysarg_buf_arglen_when_exiting && rec->enter) ) {
                break;
            }

            if ( *(char **) args[i] == NULL ) {
                break;
            }

            arg_val = *(char **) args[i];
            arg_len = sysargs->lensz * (*(size_t *)args[ sysargs->lenarg ]);

            dbg( xdr_arg_debug, "buf_arglen:%p(%d)", (void *) arg_val, arg_len );
            break;

        case sysarg_argv_when_entering:
        case sysarg_argv_when_exiting:
        case sysarg_argv: {
            // sorry but args itself is char **
            char ** ptr = *(char ***) args[i];
            int idx;
            int len;

            arg_val = 0;
            arg_len = 0;

            dbg( xdr_arg_debug, "sysarg_argv:%s", *(char **)args[i] );

            if ( (sysargs->type == sysarg_argv_when_entering && !rec->enter )
                 || (sysargs->type == sysarg_argv_when_exiting && rec->enter) ) {
                break;
            }

            // counting #args
            for ( idx = 0 ; ; idx ++ ) {
                if ( ptr[idx] == NULL ) {
                    break;
                }
            }

            len = idx;

            dbg( xdr_arg_debug, "sysarg_argv:%d", len );

            // fill args
            rec->args.args_val[i].argv.argv_len = len;
            rec->args.args_val[i].argv.argv_val = get_cache_argv_str( len );

            // NOTE. copy pointer (NEVER call vfree)
            for ( idx = 0 ; idx < len ; idx ++ ) {
                rec->args.args_val[i].argv.argv_val[idx].s = ptr[idx];
            }

        } break;

        case sysarg_buf_arglenp:
            arg_len = sysargs->lensz * (**(unsigned long**) args[sysargs->lenarg]);
            arg_val = *(char **) args[i];

            if ( arg_val == NULL ) {
                arg_len = 0;
            }

            dbg( xdr_arg_debug, "sysarg_buf_arglenp: %p(%d,%d)",
                 (void *) arg_val, arg_len, sysargs->lensz );
            break;

        case sysarg_buf_fixlen:
            arg_val = *(char **) args[i];
            arg_len = sysargs->lensz;

            // ignore if nothing
            if ( arg_val == NULL ) {
                arg_len = 0;
            }

            dbg( xdr_arg_debug, "sysarg_buf_fixlen:%p(%d)", (void *) arg_val, arg_len );
            break;

        case sysarg_ignore:
            break;

        case sysarg_unknown:
            break;

        default:
            err( "UNIMPLEMENTED : (%s) %d", rec->scname, sysargs->type );
            continue;
        }

        rec->args.args_val[i].data.data_len = arg_len;
        rec->args.args_val[i].data.data_val = arg_val;
    }

    // per syscall specific arguments

    switch ( rec->scno ) {
    case __NR_open:
    case __NR_creat:
    case __NR_execve:
    case __NR_unlink:
    case __NR_mkdir:
    case __NR_rmdir:
        record_namei( rec, args, 0 );
        break;

    case __NR_rename:
        record_namei( rec, args, 0 );
        record_namei( rec, args, 1 );
    break;

    case __NR_utimensat:
    case __NR_futimesat:
    case __NR_newfstatat:
    case __NR_readlinkat:
    case __NR_faccessat:
    case __NR_fchmodat:
    case __NR_fchownat:
    case __NR_openat:
    case __NR_mknodat:
    case __NR_mkdirat:
    case __NR_unlinkat:
        if ( rec->enter ) {
            record_arg_fd_stat( rec, args, 0 );
            record_namei( rec, args, 1 );
        }
        break;

    case __NR_symlinkat:
        record_arg_fd_stat( rec, args, 1 );
        record_namei( rec, args, 2 );
        break;

    case __NR_linkat:
    case __NR_renameat:
        record_arg_fd_stat( rec, args, 0 );
        record_namei( rec, args, 1 );
        record_arg_fd_stat( rec, args, 2);
        record_namei( rec, args, 3 );
        break;

    case __NR_fstat:
    case __NR_read:
    case __NR_pread64:
    case __NR_readv:
    case __NR_write:
    case __NR_pwrite64:
    case __NR_writev:

    case __NR_undo_mask_start:
    case __NR_undo_mask_end:
    case __NR_undo_depend:

        record_arg_fd_stat( rec, args, 0 );
        break;
    }

    if ( !rec->enter ) {
        switch ( rec->scno ) {
        case __NR_open:
        case __NR_openat:
            record_ret_fd_stat( rec );
            break;
        }
    }

    if ( rec->scno == __NR_execve && rec->enter ) {
        record_exec( rec, args );
    }

    // skip unnecessary snapshot (I think we filter syscalls with the opposite way)
    if ( rec->enter &&
         !(rec->scno == __NR_rt_sigprocmask) &&
         !(rec->scno == __NR_rt_sigaction) &&
         !(rec->scno == __NR_rt_sigreturn) &&
         !(rec->scno == __NR_kill) &&
         !(rec->scno == __NR_fsync) &&
         !(rec->scno == __NR_msync) &&
         !(rec->scno == __NR_close) &&
         !(rec->scno == __NR_shutdown) &&
         !(rec->scno == __NR_read) &&
         !(rec->scno == __NR_write  && rec->args.args_val[0].fd_stat && !S_ISREG(rec->args.args_val[0].fd_stat->mode)) &&
         !(rec->scno == __NR_writev && rec->args.args_val[0].fd_stat && !S_ISREG(rec->args.args_val[0].fd_stat->mode)) &&
         !(rec->scno == __NR_open   && !(*(int *)args[1] & (O_CREAT | O_TRUNC))) &&
         !(rec->scno == __NR_openat && !(*(int *)args[2] & (O_CREAT | O_TRUNC))) &&
         !(rec->scno == __NR_fstat) &&
         !(rec->scno == __NR_newfstatat) &&
         !(rec->scno == __NR_lstat) &&
         !(rec->scno == __NR_statfs) &&
         !(rec->scno == __NR_chmod) &&
         !(rec->scno == __NR_chown) &&
         !(rec->scno == __NR_fchmod) &&
         !(rec->scno == __NR_fchmodat) &&
         !(rec->scno == __NR_fchown) &&
         !(rec->scno == __NR_lchown) &&
         !(rec->scno == __NR_utimensat) &&
         !(rec->scno == __NR_futimesat) &&
         !(rec->scno == __NR_utime) &&
         !(rec->scno == __NR_dup) &&
         !(rec->scno == __NR_dup2) &&
         !(rec->scno == __NR_readlink) &&
         !(rec->scno == __NR_getdents) &&
         !(rec->scno == __NR_arch_prctl) &&
         !(rec->scno == __NR_execve) &&
         !(rec->scno == __NR_clone) &&
         !(rec->scno == __NR_vfork) &&
         !(rec->scno == __NR_fork) &&
         !(rec->scno == __NR_exit_group) &&
         !(rec->scno == __NR_set_robust_list) &&
         !(rec->scno == __NR_set_tid_address) &&
         !(rec->scno == __NR_flock) &&
         !(rec->scno == __NR_fcntl) &&
         !(rec->scno == __NR_ioctl) &&
         !(rec->scno == __NR_futex) &&
         !(rec->scno == __NR_wait4) &&
         !(rec->scno == __NR_access) &&
         !(rec->scno == __NR_stat) &&
         !(rec->scno == __NR_munmap) &&
         !(rec->scno == __NR_brk) &&
         !(rec->scno == __NR_lseek) &&
         !(rec->scno == __NR_vhangup) &&
         !(rec->scno == __NR_nanosleep) &&

         !(rec->scno == __NR_select) &&
         !(rec->scno == __NR_pselect6) &&
         !(rec->scno == __NR_poll) &&
         !(rec->scno == __NR_epoll_create) &&
         !(rec->scno == __NR_epoll_ctl) &&
         !(rec->scno == __NR_epoll_wait) &&
         !(rec->scno == __NR_epoll_pwait) &&
         !(rec->scno == __NR_socket) &&
         !(rec->scno == __NR_connect) &&
         !(rec->scno == __NR_accept) &&
         !(rec->scno == __NR_listen) &&
         !(rec->scno == __NR_bind) &&
         !(rec->scno == __NR_getpeername) &&
         !(rec->scno == __NR_getsockname) &&
         !(rec->scno == __NR_recvmsg) &&
         !(rec->scno == __NR_sendmsg) &&
         !(rec->scno == __NR_recvfrom) &&
         !(rec->scno == __NR_sendto) &&
         !(rec->scno == __NR_getsockopt) &&
         !(rec->scno == __NR_setsockopt) &&
         !(rec->scno == __NR_pipe) &&
         !(rec->scno == __NR_socketpair) &&

         !(rec->scno == __NR_mkdir) &&
         !(rec->scno == __NR_rmdir) &&
         !(rec->scno == __NR_link) &&
         !(rec->scno == __NR_symlink) &&

         !(rec->scno == __NR_getuid) &&
         !(rec->scno == __NR_geteuid) &&
         !(rec->scno == __NR_getgid) &&
         !(rec->scno == __NR_getegid) &&
         !(rec->scno == __NR_getpgrp) &&
         !(rec->scno == __NR_getcwd) &&
         !(rec->scno == __NR_chdir) &&
         !(rec->scno == __NR_fchdir) &&
         !(rec->scno == __NR_getgroups) &&
         !(rec->scno == __NR_setgroups) &&
         !(rec->scno == __NR_getrusage) &&
         !(rec->scno == __NR_getrlimit) &&
         !(rec->scno == __NR_setrlimit) &&
         !(rec->scno == __NR_uname) &&
         !(rec->scno == __NR_umask) &&
         !(rec->scno == __NR_getpid) &&
         !(rec->scno == __NR_getppid) &&
         !(rec->scno == __NR_getpriority) &&
         !(rec->scno == __NR_setpriority) &&
         !(rec->scno == __NR_setuid) &&
         !(rec->scno == __NR_setgid) &&
         !(rec->scno == __NR_setpgid) &&
         !(rec->scno == __NR_setresuid) &&
         !(rec->scno == __NR_setresgid) &&
         !(rec->scno == __NR_setreuid) &&
         !(rec->scno == __NR_setregid) &&
         !(rec->scno == __NR_setsid) &&
         !(rec->scno == __NR_chroot) &&
         !(rec->scno == __NR_sendfile) &&
	 
         !(rec->scno == __NR_undo_mask_start) &&
         !(rec->scno == __NR_undo_mask_end) &&
         !(rec->scno == __NR_undo_func_start) &&
         !(rec->scno == __NR_undo_func_end) &&
         !(rec->scno == __NR_undo_depend && *(int *)args[3] == 0) &&

         !(rec->scno == __NR_mmap && !(*(int *)args[2] & PROT_WRITE)) &&
         !(rec->scno == __NR_mmap && ((*(int *)args[3] & MAP_TYPE) == MAP_PRIVATE)) &&
         !(rec->scno == __NR_mprotect && !(*(int *)args[2] & PROT_WRITE)) &&
         !(rec->scno == __NR_alarm) ) {

        char buf_srcpath[512];
        int srcfd = -1;
        char * srcpath = NULL;

        snap_count ++;
        rec->snap.id = (((uint64_t) 1) << 32) | snap_count;

        switch ( rec->scno ) {
        case __NR_write:
        case __NR_writev:
        case __NR_pwrite64:
        case __NR_undo_depend:
        case __NR_ftruncate:
            srcfd = *(int *) args[0];
            break;
        case __NR_mmap:
            srcfd = *(int *) args[4];
            break;
        case __NR_truncate:
            srcpath = *(char **) args[0];
            break;
        case __NR_open:
        case __NR_creat:
            srcpath = *(char **) args[0];
            record_snap_dirents( rec, srcpath, NULL );
            break;
        case __NR_openat:
            srcpath = makeupath( buf_srcpath, sizeof( buf_srcpath ), rec->pid,
                                *(int *) args[0], *(char **) args[1] );
            record_snap_dirents( rec, srcpath, NULL );
            break;
        case __NR_unlink:
            srcpath = *(char **) args[0];
            record_snap_dirents( rec, srcpath, NULL );
            linkpath( srcpath );
            break;
        case __NR_unlinkat: {
            srcpath = makeupath( buf_srcpath, sizeof( buf_srcpath ), rec->pid,
                                    *(int *) args[0], *(char **) args[1] );

            record_snap_dirents( rec, srcpath, NULL );
            linkpath( srcpath );
        } break;
        case __NR_rename:
            srcpath = *(char **) args[1];
            record_snap_dirents( rec, *(char **) args[0], srcpath );
            linkpath( srcpath );
            break;
        case __NR_renameat: {
            char * path;

            path = makeupath( buf_srcpath, sizeof( buf_srcpath ), rec->pid,
                              *(int *) args[0], *(char **) args[1] );

            srcpath = makeupath( buf_srcpath, sizeof( buf_srcpath ), rec->pid,
                                *(int *) args[2], *(char **) args[3] );

            record_snap_dirents( rec, path, srcpath );

            linkpath( srcpath );
        } break;
        default:
            err( "snapshot for unknown syscall %s(%d)", rec->scname, rec->scno );
            break;
        }

        //
        // record snap stat for write/open/rename
        //
        if ( rec->scno == __NR_write ) {
            record_snap_stat_with_fd( &rec->snap.file_stat, snap_count, srcfd );
        } else if ( rec->scno == __NR_open   ||
                    rec->scno == __NR_unlink ||
                    rec->scno == __NR_unlinkat ) {
            record_snap_stat_with_path( &rec->snap.file_stat, snap_count, srcpath );
        } else {

            if ( srcfd != -1 ) {
                copy_file_with_fd( &rec->snap.file_stat, snap_count, srcfd );
                dbg( snap_debug, "snapshot syscall %s(fd:%d) => %d",
                     rec->scname, srcfd, snap_count );
            }

            if ( srcpath ) {
                if ( copy_file_with_path( &rec->snap.file_stat, snap_count, srcpath ) < 0 ) {

#ifndef BEST_PERFORMANCE
                    // filter out when the file is really non existant
                    if ( !(rec->enter && ( rec->scno == __NR_open || rec->scno == __NR_openat ) ) ) {
                        err( "%s(%d):%d(%s)=%d",
                             rec->scname, rec->enter, rec->scno, srcpath, (int) rec->ret );
                    }
#endif
                }
                dbg( snap_debug, "snapshot syscall %s(%s) => %d",
                     rec->scname, srcpath, snap_count );
            }
        }
    } else {
        rec->snap.id = 0;
    }

    safe_xdr_syscall_record( &xdr, rec );
}

static void xdr_free_sysarg( syscall_record * rec, void ** args )
{
    int i, j;

    for ( i = 0 ; i < rec->args.args_len ; i ++ ) {

        /* we are using stacks for args
        if ( rec->args.args_val[i].data.data_val ) {
            free( rec->args.args_val[i].data.data_val );
        }
        */

        // please read args related routine
        if ( rec->args.args_val[i].argv.argv_val ) {
            /* we are using in user's space memory
            for ( j = 0; j < rec->args.args_val[i].argv.argv_len ; j ++ ) {
                vfree( rec->args.args_val[i].argv.argv_val[j].s );
            }
            */
            put_cache_argv_str( rec->args.args_val[i].argv.argv_val );
        }

        if ( rec->args.args_val[i].namei ) {
            for ( j = 0; j < rec->args.args_val[i].namei->ni.ni_len; j ++ ) {
                if ( rec->args.args_val[i].namei->ni.ni_val[j].name ) {
                    put_cache_string( (struct string *) rec->args.args_val[i].namei->ni.ni_val[j].name );
                }
            }

            put_cache_namei_info( rec->args.args_val[i].namei->ni.ni_val );
            put_cache_namei_infos( rec->args.args_val[i].namei );
        }

        if ( rec->args.args_val[i].fd_stat ) {
            put_cache_stat_info( rec->args.args_val[i].fd_stat );
        }
    }

    if ( rec->ret_fd_stat ) {
        put_cache_stat_info( rec->ret_fd_stat );
    }

    if ( rec->args.args_val ) {
        put_cache_syscall_arg( rec->args.args_val );
    }

    if ( rec->snap.file_stat ) {
        put_cache_stat_info( rec->snap.file_stat );
    }

    if ( rec->snap.dirents.dirents_len > 0 ) {
        for ( i = 0 ; i < rec->snap.dirents.dirents_len ; i ++ ) {
            put_cache_namei_info( rec->snap.dirents.dirents_val[i].ni.ni_val );
            for ( j = 0 ; j < rec->snap.dirents.dirents_val[i].ni.ni_len ; j ++ ) {
                put_cache_string( (struct string *) rec->snap.dirents.dirents_val[i].ni.ni_val[j].name );
            }
        }
        put_cache_dirent_infos( rec->snap.dirents.dirents_val );
    }

    if ( rec->execi ) {
        if ( rec->execi->fds.fds_val ) {
            put_cache_big_stat_info( rec->execi->fds.fds_val );
        }
        put_cache_exec_info( rec->execi );
    }
}

#include "hook.h"
#include "syscall_proto.h"

DEF_HOOK_FN( exit_group, int error_code )
{
    IF_TRACKING_BEG_XDR( args[0] = &error_code );

    if ( post_hook ) {
        dbg( exec_debug, "<exit_group: pid:%d", _pid );
    }

    POST_HOOK_RTN( exit_group, error_code );
    IF_TRACKING_END_XDR( write_record_to_file( RECORD_ARCHIVE ) );

} END_DEF_HOOK

//
// NOTE. wait4
//
// you might see NULL dereference when loading/unloading this module, but
// syslogs is designed to be not unloadable, so please use it as it is.
//
// the reason why NULL point dereference is that wait4 is yeild so long even
// after we unloaded this module (restored the hooked syscalls). It simply
// execute the unloaded memory - but we don't have to reboot to reload this
// module!
//

//
// NOTE. special care on execve
//
// when quit, we can't access the addressees that args are pointing
//
DEF_HOOK_FN( execve, char * filename, char ** argv, char ** envp,
             struct pt_regs * regs )
{

    IF_TRACKING_BEG_XDR( args[0] = &filename;
                         args[1] = &argv;
                         args[2] = &envp;
                         args[3] = &regs );

    if ( post_hook ) {
        dbg( exec_debug, "exec: filename:%s(%d)", filename, _pid );
    } else {
        dbg( exec_debug, "?exec: filename:%s(%d)", filename, _pid );
    }

    POST_HOOK_RTN( execve, filename, argv, envp, regs );

    if ( post_hook ) {
        rec.pid   = current->pid;
        rec.enter = 0;
        rec.ret   = rtn;
        rec.err   = 0;

        rec.ret_fd_stat   = NULL;
        rec.args.args_len = 0;
        rec.args.args_val = 0;

        // record to buffer
        safe_xdr_syscall_record( &xdr, &rec );
        // free
        xdr_free_sysarg( &rec, &args[0] );
    }

    return rtn;

} END_DEF_HOOK

//
// NOTE. tracking new child if on tracking
//
DEF_HOOK_FN( clone, unsigned long clone_flags, unsigned long newsp,
             void * parent_tid, void * child_tid,
             struct pt_regs * regs )
{
    IF_TRACKING_BEG_XDR( args[0] = &clone_flags;
                         args[1] = &newsp;
                         args[2] = &parent_tid;
                         args[3] = &child_tid;
                         args[4] = &regs );

    POST_HOOK_RTN( clone, clone_flags, newsp, parent_tid,
                   child_tid, regs );

    dbg( exec_debug, "%sclone: %d => %ld", post_hook ? "" : "?", _pid, rtn );

    IF_TRACKING_END_XDR( if ( rtn != -1 && !trace(rtn) ) { err( "failed to trace %ld", rtn ); } );

} END_DEF_HOOK

DEF_HOOK_FN( fork, struct pt_regs * regs )
{
    IF_TRACKING_BEG_XDR();
    POST_HOOK_RTN( fork, regs );

    dbg( exec_debug, "%sfork: %d => %ld", post_hook ? "" : "?", _pid, rtn );

    IF_TRACKING_END_XDR( if ( rtn != -1 && rtn != 0 && !trace(rtn) ) \
                             { err( "failed to trace %ld", rtn ); } );
} END_DEF_HOOK

DEF_HOOK_FN( vfork, struct pt_regs * regs )
{
    IF_TRACKING_BEG_XDR();
    POST_HOOK_RTN( vfork, regs );

    dbg( exec_debug, "%svfork: %d => %ld", post_hook ? "" : "?", _pid, rtn );

    IF_TRACKING_END_XDR( if ( rtn != -1 && rtn != 0 && !trace(rtn) ) \
                             { err( "failed to trace %ld", rtn ); } );
} END_DEF_HOOK

//
// =====================================================================
// stub system calls : two different sort of stub syscalls
//
//
// ex)
//
// stub_execve: 0xffffffff810124b0
//                                                                 ^
// 0xffffffff810124b0:     pop    %r11                             |
// 0xffffffff810124b2:     sub    $0x30,%rsp                       |
// ...                                                       fixed |
// 0xffffffff81012512:     mov    %rsp,%rcx                        |
// 0xffffffff81012515:     callq  0xffffffff810104f0 <sys_execve>  v
//                                  ^
//                                  +---------- hook here (4 bytes)
// 0xffffffff8101251a:     mov    0x98(%rsp),%r11
//
//
// for those stubs syscalls except (execve)
//
//	PTREGSCALL stub_clone, sys_clone, %r8
//	PTREGSCALL stub_fork, sys_fork, %rdi
//	PTREGSCALL stub_vfork, sys_vfork, %rdi
//	PTREGSCALL stub_sigaltstack, sys_sigaltstack, %rdx
//	PTREGSCALL stub_iopl, sys_iopl, %rsi
//
// ex)
// stub_clone: 0xffffffff810123d0
// sys_clone : 0xffffffff810104c0
//
// 0xffffffff810123d0:     sub    $0x30,%rsp                     ^
// 0xffffffff810123d4:     callq  0xffffffff81011ec0             |
// 0xffffffff810123d9:     lea    0x8(%rsp),%r8            fixed |
// 0xffffffff810123de:     callq  0xffffffff810104c0 <sys_clone> v
// 0xffffffff810123e3:     jmpq   0xffffffff81012470
//

static int install_stub_syscalls( unsigned long addr_stub,
                                  unsigned long addr_syscall,
                                  unsigned long diff_to_callq,
                                  unsigned int  nsyscall,
                                  unsigned long hooked_func )
{
    unsigned long flags;

    const unsigned char inst_callq = 0xe8;
    const unsigned long diff_callq = diff_to_callq;
    const unsigned long addr_callq = addr_stub + diff_callq;
    const unsigned char mref_callq = *(unsigned char *)( addr_callq );
    const unsigned int  mref_clone = *(unsigned int *)( addr_callq + 1 );

    const unsigned int calc_clon \
        = (unsigned int)( 0xffffffff - ( addr_callq + 5 - addr_syscall ) + 1 );

    const unsigned int hook \
        = (int)( 0xffffffff - ( addr_callq + 5 - hooked_func ) + 1 );

    // check santiy (callq == 0xe8)
    if ( mref_callq != inst_callq ) {
        err( "address: callq sys_clone (%p) != %02x",
             (void *) addr_callq,
             mref_callq );

        return 0;
    }

    dbg( hook_debug, "sys_clone: 0x%X (%p)", mref_clone, (void *) addr_syscall );

    // calculated relative address != real address
    if ( calc_clon != mref_clone ) {
        err( "failed to overwrite, 0x%X != 0x%X", calc_clon, mref_clone );
        return 0;
    }

    dbg( hook_debug, "overwrite %X with %X", mref_clone, hook );

    // make it rw
    syslogs_set_memory_rw( addr_stub );

    dbg( hook_debug, "map_syscalls: %p", (void *) map_syscalls );

    // record it in order to return to sys_clone in hooked function
    map_syscalls[ nsyscall ] = addr_syscall;

    local_irq_save( flags );
    *(unsigned int *)( addr_callq + 1 ) = hook;
    local_irq_restore( flags );

    return 1;
}

static int uninstall_stub_syscalls( unsigned long addr_stub,
                                    unsigned long addr_syscall,
                                    unsigned long diff_to_callq )
{
    // ref. install clone
    unsigned long flags;

    const unsigned long diff_callq = diff_to_callq;
    const unsigned long addr_callq = addr_stub + diff_callq;

    const unsigned int calc_clone \
        = (unsigned int)( 0xffffffff - ( addr_callq + 5 - addr_syscall ) + 1 );

    local_irq_save( flags );
    *(unsigned int *)( addr_callq + 1 ) = calc_clone;
    local_irq_restore( flags );

    return 1;
}

#define DEF_STUB_SYSCALL_HOOK( name, diff )                             \
    static int uninstall_##name( void )                                 \
    {                                                                   \
        return uninstall_stub_syscalls( __stub_##name,                  \
                                        __sys_##name,                   \
                                        diff );                         \
    }                                                                   \
                                                                        \
    static int install_##name( void )                                   \
    {                                                                   \
        return install_stub_syscalls( __stub_##name,                    \
                                      __sys_##name,                     \
                                      diff,                             \
                                      __NR_##name,                      \
                                      (unsigned long) hooked_##name );  \
    }

//
// add new stub syscall definition here
//
DEF_STUB_SYSCALL_HOOK( execve, 0xffffffff81012515 - 0xffffffff810124b0 )

DEF_STUB_SYSCALL_HOOK( clone , 0xffffffff810123de - 0xffffffff810123d0 )
DEF_STUB_SYSCALL_HOOK( fork  , 0xffffffff810123de - 0xffffffff810123d0 )
DEF_STUB_SYSCALL_HOOK( vfork , 0xffffffff810123de - 0xffffffff810123d0 )

#undef DEF_STUB_SYSCALL_HOOK

//
// make address protection bits as read/write
//
static int syslogs_set_memory_rw( unsigned long address )
{
    unsigned long start = address & PAGE_MASK;

    pte_t old_pte;
    pte_t * kpte;
    unsigned int level;

    dbg( hook_debug, "changed (%p) to read/write-able", (void *) start );

    kpte = lookup_address( start, &level );
    if ( !kpte ) {
        dbg( hook_debug, "failed to get pte(%lx)", start );
        return 0;
    }

    old_pte = *kpte;
    if ( !pte_val( old_pte ) ) {
        dbg( hook_debug, "invalid pte (%p)", kpte );
        return 0;
    }

    if ( level == PG_LEVEL_4K ) {
        pgprot_t new_prot = pte_pgprot(old_pte);
        unsigned long pfn = pte_pfn(old_pte);

        pgprot_val( new_prot ) |= pgprot_val(__pgprot(_PAGE_RW));

        dbg( hook_debug, "old : %X => new : %X",
             (unsigned int) pgprot_val( pte_pgprot( old_pte ) ),
             (unsigned int) pgprot_val( new_prot ) );

        set_pte_atomic( kpte, pfn_pte(pfn, new_prot) );

        clflush_cache_range( &address, sizeof( address ) );

        return 1;
    }

    return 0;
}

//
// check page entry protection bits
//
static int syslogs_check_memory( unsigned long address )
{
    unsigned long start = address & PAGE_MASK;

    pte_t old_pte;
    pte_t * kpte;

    unsigned int prot;
    unsigned int level;

    kpte = lookup_address( start, &level );
    if ( !kpte ) {
        err( "check memory address (%p)", (void *) start );
        return 0;
    }

    old_pte = *kpte;
    if ( !pte_val( old_pte ) ) {
        err( "invalid pte (%p)", kpte );
        return 0;
    }

    prot = pgprot_val( pte_pgprot( old_pte ) );

    printk( KERN_INFO "page table entry protection bits : 0x%x (RW:%d)",
            prot,
            (int)( prot & pgprot_val(__pgprot(_PAGE_RW)) ) );

    return 0;
}

//
// check sanity of kernel (like proper Sysmap)
//
static int check_sanity( void )
{
    printk( KERN_INFO "sys_call_table: %p", sys_call_table );

    if ( (void *) sys_call_table[ __NR_close ] != sys_close ) {

        printk( KERN_ERR "sys_call_table is not valid(%p vs %p)",
                (void *) sys_call_table[ __NR_close ],
                sys_close );

        return 0;
    }

    if ( sizeof( unsigned long ) != 8 ) {
        printk( KERN_ERR "tests only on 64-bit(%ld)", sizeof( unsigned long ) );
        return 0;
    }

    return 1;
}

// ======================================================================
// proc interfaces
//

//
// cat /proc/undosys/proc_stat : split out the buffer offset
//
static int proc_stat( char * page, char ** start, off_t off,
                      int count, int * eof, void * data )
{
    unsigned long flags;
    int len;

    spin_lock_irqsave( &record_buffer_lock, flags );

    len = sprintf( page, "%d",
                   (int)((unsigned long) xdr.p - (unsigned long) record_buf) );

    spin_unlock_irqrestore( &record_buffer_lock, flags );

    return len;
}

//
// cat /proc/undosys/record | less
//
// NOTE. not dealing with offset! (so, less /proc/ .. is not working)
//

static int proc_record( char * page, char ** start, off_t off,
                        int count, int * eof, void * data )
{
    unsigned long flags;
    int rtn;
    unsigned long len;

    spin_lock_irqsave( &record_buffer_lock, flags );

    len = (int)( (unsigned long) xdr.p - (unsigned long) record_buf );
    rtn = count >= len ? len : count;

    memcpy( page, record_buf, rtn );

    dbg( proc_debug, "off=%u/start=%ld/count=%d/pos=%ld",
         (unsigned int) off,
         *(unsigned long *) start,
         count,
         len );

    if ( count >= len ) {
        xdr.p = (__be32 *)( (unsigned long) record_buf );
        *eof = 1;
    } else {
        memcpy( record_buf, &record_buf[ rtn ], len - rtn );
        xdr.p = (__be32 *)( (unsigned long) xdr.p - (unsigned long) rtn );
    }

    *((unsigned long *) start) = (unsigned long)( rtn );

    spin_unlock_irqrestore( &record_buffer_lock, flags );

    return rtn;
}

// echo pid > /proc/undosys/tracing
static int proc_tracing( struct file * file, const char __user * buffer,
                          unsigned long count, void * data )
{
    int i;
    int pid = 0;

    // convert pid to integer
    for ( i = 0 ; i < count ; i ++ ) {
        // up until proper number
        if ( buffer[i] < '0' || buffer[i] > '9' ) {
            break;
        }

        pid *= 10;
        pid += buffer[i] - '0';
    }

    dbg( proc_debug, "pid : %d", pid );

    trace( pid );

    return count;
}

struct undocall {
    int num;
    char * name;
};

//
// NOTE. NO REENTERENCE!
//
// echo undo_func_start,undo_mgr,funcname,3,abc > /proc/undosys/mask
static int proc_mask( struct file * file, const char __user * buffer,
                      unsigned long count, void * data )
{
    static struct undocall snames[]
        = { { __NR_undo_func_start, "undo_func_start" },
            { __NR_undo_func_end  , "undo_func_end"   },
            { __NR_undo_mask_start, "undo_mask_start" },
            { __NR_undo_mask_end  , "undo_mask_end"   },
            { __NR_undo_depend    , "undo_depend"     } };

    int i;
    int intargs[7];
    void * ptrargs[7];
    void * args[7];

    struct syscall_record rec;

    static char kbuf[1024];

    if ( count >= sizeof( kbuf ) ) {
        return -1;
    }

    copy_from_user( kbuf, buffer, count );
    kbuf[ count ] = '\0';

    dbg( proc_debug, "%s", kbuf );

    //                          +-- &arg[1]     +-- &arg[3]
    //                          v               v
    // undo_func_start,undo_mgr,funcname,arglen,argbug
    //                 ^                 =>
    //                 +-- &arg[0]       intarg[2] <- arg[2]
    //
    for ( i = 0 ; i < sizeof( snames ) / sizeof( snames[0] ) ; i ++ ) {
        if ( strncmp( snames[i].name, kbuf, strlen( snames[i].name ) ) == 0 ) {
            int arg;

            const int snum  = snames[i].num;
            const int nargs = nsysarg_calls[ snum ];
            const char * sname = snames[i].name;

            char * p = kbuf;

            dbg( proc_debug, "found: %s(%d), arg:%d", sname, snum, nargs );

            // fill default info
            rec.scno   = snum;
            rec.scname = (char *) sname;
            rec.pid    = current->pid;
            rec.enter  = 1;
            rec.ret    = 0;
            rec.err    = 0;

            // fill arguments
            p = strchr( p, ',' );
            if ( !p ) {
                err( "illegal form of syscall input on %s(%d) call",
                     sname, nargs );
                goto exit;
            }

            p ++;

            for ( arg = 0 ; arg < nargs ; arg ++ ) {
                char * newp = p;

                // skip last argument, (protect arg buffer)
                if ( arg != nargs - 1 ) {

                    // find next argument
                    if ( !(p = strchr( p, ',' )) ) {
                        err( "illegal form of syscall input on %s(%d) call",
                             sname, nargs );
                        goto exit;
                    }

                    *p = '\0';
                    p ++;
                }

                dbg( proc_debug, "p:%s", newp );

                switch ( sysarg_calls[ snum ].args[ arg ].type ) {
                case sysarg_int:
                    sscanf( newp, "%d", &intargs[arg] );
                    args[arg] = &intargs[arg];
                    break;

                case sysarg_buf_arglen:
                case sysarg_strnull:
                    ptrargs[arg] = (void *) newp;
                    args[arg] = &ptrargs[arg];
                    break;
                }
            }

            xdr_fill_sysarg( &rec, &args[0] );
            xdr_free_sysarg( &rec, &args[0] );

            // end parsing other undo* syscalls
            break;
        }
    }

 exit:

    return count;
}


// proc root directory
struct proc_dir_entry * syslog_root = NULL;

static int install_proc( void )
{
    struct proc_dir_entry * res;

    syslog_root = proc_mkdir( "undosys", NULL );
    if ( !syslog_root ) {
        return 0;
    }

    //
    // TODO change permission (capability)
    //

    // read record
    res = create_proc_read_entry( "record", 0666, syslog_root, proc_record, NULL );
    if ( !res ) {
        return 0;
    }

    // read record offset
    res = create_proc_read_entry( "stat", 0666, syslog_root, proc_stat, NULL );
    if ( !res ) {
        return 0;
    }

    // start tracing
    res = create_proc_entry( "tracing", 0666, syslog_root );
    if ( !res ) {
        return 0;
    }

    res->write_proc = proc_tracing;
    res->data = NULL;

    // mask
    res = create_proc_entry( "mask", 0666, syslog_root );
    if ( !res ) {
        return 0;
    }

    res->write_proc = proc_mask;
    res->data = NULL;

    return 1;
}

static void uninstall_proc( void )
{
    remove_proc_entry( "tracing", syslog_root );
    remove_proc_entry( "record" , syslog_root );
    remove_proc_entry( "mask"   , syslog_root );
    remove_proc_entry( "stat"   , syslog_root );
    remove_proc_entry( "undosys", NULL );
}

// ======================================================================
// fast copy with splice
//

static int _direct_splice_actor(struct pipe_inode_info *pipe,
                   struct splice_desc *sd)
{
    struct file *file = sd->u.file;

    return do_splice_from(pipe, file, &sd->pos, sd->total_len, sd->flags);
}

long _do_splice_direct( struct file *in, loff_t *ppos, struct file *out,
                         size_t len, unsigned int flags)
{
    struct splice_desc sd = {
        .len		= len,
        .total_len	= len,
        .flags		= flags,
        .pos		= *ppos,
        .u.file		= out,
    };
    long ret;

    ret = splice_direct_to_actor(in, &sd, _direct_splice_actor);
    if (ret > 0)
        *ppos = sd.pos;

    return ret;
}

static ssize_t copy_file_with_file( struct stat_info ** pp, int snap_id, struct file * in_file )
{
    #define DST_DIR "/mnt/undofs/snap"

    struct path root;
    struct path old_root;

    const size_t copy_unit = 10000;

    struct file * out_file;
    struct inode * in_inode, * out_inode;
    loff_t pre_pos = 0;
    loff_t pos = 0;
    ssize_t retval = -EBADF;
    mm_segment_t old_fs;

    char dest[256];

    if ( !in_file || !(in_file->f_mode & FMODE_READ) ) {
        err( "file can't be read(%X/%X)", in_file->f_mode, FMODE_READ );
        return retval;
    }

    //
    // Get output file, and verify that it is ok..
    //
    snprintf( dest, sizeof( dest ), "%s/1.%d", DST_DIR, snap_id );

    // change memory to userspace
    old_fs = get_fs();
    set_fs( KERNEL_DS );

    // fetch root directory
    root.mnt    = current->fs->root.mnt;
    root.dentry = current->fs->root.dentry->d_sb->s_root;

    // change root directory (for chroot)
    write_lock( &current->fs->lock );
    old_root = current->fs->root;
    current->fs->root = root;
    path_get( &root );
    write_unlock( &current->fs->lock );

    // now open snapshot file
    out_file = filp_open( dest, O_CREAT | O_RDWR, 0600 );
    if ( IS_ERR( out_file ) ) {
        if ( (long) out_file != -ENOENT ) {
            err( "failed to open : %s(%ld)", dest, (long) out_file );
        }
        goto root_out;
    }

    in_inode  = in_file ->f_path.dentry->d_inode;
    out_inode = out_file->f_path.dentry->d_inode;

    do {
        pre_pos = out_file->f_pos = pos;

        retval = _rw_verify_area( WRITE, out_file, &out_file->f_pos, copy_unit );
        if ( retval < 0 ) {
            err( "failed to verify area(%d)", (int) retval );
            goto fput_out;
        }

        // copy!
        retval = _do_splice_direct( in_file, &pos, out_file, copy_unit, 0 );

        dbg( copy_debug, "%d(<=%s): ret: %d, pos:%u, copy_unit:%u",
             snap_id,
             in_file->f_path.dentry->d_name.name,
             (int) retval,
             (unsigned int) pos,
             (unsigned int) copy_unit );

    } while ( pos == (pre_pos + copy_unit) );

    __record_file_stat( pp, in_file );

fput_out:
    filp_close( out_file, current->files );

root_out:
    // restore old root (for chroot)
    write_lock( &current->fs->lock );
    current->fs->root = old_root;
    write_unlock( &current->fs->lock );

    path_put( &root );

    // restore address space
    set_fs( old_fs );

    return retval;

    #undef SRC_DIR
    #undef DST_DIR
}

static ssize_t copy_file_with_fd( struct stat_info ** pp, int snap_id, int in_fd )
{
    struct file * in_file;
    ssize_t retval;

    //
    // Get input file, and verify that it is ok..
    //
    retval = -EBADF;
    in_file = fget( in_fd );
    if ( !in_file ) {
        err( "open fd:%d error", in_fd );
        goto fput_in;
    }

    in_file->f_mode |= FMODE_READ;

    retval = copy_file_with_file( pp, snap_id, in_file );

fput_in:
    fput( in_file );

    return retval;
}

static ssize_t copy_file_with_path( struct stat_info ** pp, int snap_id, char * src )
{
    mm_segment_t old_fs;
    struct file * in_file;
    ssize_t retval = -EBADF;

    // change memory to userspace
    old_fs = get_fs();
    set_fs( KERNEL_DS );

    in_file = filp_open( src, O_RDONLY, 0 );
    if ( IS_ERR( in_file ) ) {
        // filtering out no such a file
        if ( (long) in_file != -ENOENT ) {
            err( "failed to open : %s = %ld", src, (long) in_file );
        }

        set_fs( old_fs );
        return retval;
    }

    retval = copy_file_with_file( pp, snap_id, in_file );

    filp_close( in_file, current->files );
    set_fs( old_fs );

    return retval;
}

static
int record_snap_stat_with_fd( struct stat_info ** pp, int snap_id, int in_fd )
{
    struct file * in_file;
    ssize_t retval;

    //
    // Get input file, and verify that it is ok..
    //
    retval = -EBADF;
    in_file = fget( in_fd );
    if ( !in_file ) {
        err( "open fd:%d error", in_fd );

        fput( in_file );
        return 0;
    }

    __record_file_stat( pp, in_file );

    fput( in_file );
    return 1;
}

static
int record_snap_stat_with_path( struct stat_info ** pp, int snap_id, char * src )
{
    mm_segment_t old_fs;
    struct file * in_file;

    // change memory to userspace
    old_fs = get_fs();
    set_fs( KERNEL_DS );

    in_file = filp_open( src, O_RDONLY, 0 );
    if ( IS_ERR( in_file ) ) {
        // filtering out no such a file
        if ( (long) in_file != -ENOENT ) {
            err( "failed to open : %s = %ld", src, (long) in_file );
        }

        set_fs( old_fs );
        return 0;
    }

    __record_file_stat( pp, in_file );

    filp_close( in_file, current->files );
    set_fs( old_fs );

    return 1;
}

// ======================================================================
// btrfs snapshot
//

//
// NOTE. fixed location
//
// /mnt/undofs/d => /mnt/undofs/snap/snapid
//

static long vfs_ioctl( struct file * filp, unsigned int cmd,
                       unsigned long arg )
{
    int error = -ENOTTY;

    if (!filp->f_op)
        goto out;

    if (filp->f_op->unlocked_ioctl) {
        error = filp->f_op->unlocked_ioctl(filp, cmd, arg);
        if (error == -ENOIOCTLCMD)
            error = -EINVAL;
        goto out;
    } else if (filp->f_op->ioctl) {
        lock_kernel();
        error = filp->f_op->ioctl(filp->f_path.dentry->d_inode,
                      filp, cmd, arg);
        unlock_kernel();
    }

 out:
    return error;
}

static void __put_unused_fd( struct files_struct * files, unsigned int fd )
{
    struct fdtable *fdt = files_fdtable( files );

    __FD_CLR( fd, fdt->open_fds );
    if ( fd < files->next_fd ) {
        files->next_fd = fd;
    }

    rcu_assign_pointer( fdt->fd[fd], NULL );
}

static void close_fd( unsigned int fd )
{
    struct files_struct *files = current->files;
    spin_lock(&files->file_lock);
    __put_unused_fd(files, fd);
    spin_unlock(&files->file_lock);
}

static int btrfs_snapshot( void )
{
    //
    // extract from btrfsctl.c (careful on file descriptors)
    //
    // fd = /mnt/undofs/d
    // snap_fd = /mnt/undofs/snap
    //
    // 1) sync
    //   ioctl( fd, BTRFS_IOC_SYNC, &args )
    //
    // 2) creating snapshot
    //   args.name = snapid
    //   args.fd = fd
    //   ioctl( snap_fd, BTRFS_IOC_SNAP_CREATE, &args )
    //

    #define SRC_DIR "/mnt/undofs/d"
    #define DST_DIR "/mnt/undofs/snap"

    static struct btrfs_ioctl_vol_args args;

    mm_segment_t old_fs;

    int nfd;

    struct file * snap_fd;
    struct file * fd;

    const unsigned long flags \
        = O_RDONLY | O_NONBLOCK | O_DIRECTORY | O_CLOEXEC;

    // change memory to userspace
    old_fs = get_fs();
    set_fs( KERNEL_DS );

    // open d directory
    fd = filp_open( SRC_DIR, flags, 0 );
    if ( !fd ) {
        err( "failed to open : %s", SRC_DIR );
        return -1;
    }

    // combine file descriptor to its fd number
    nfd = get_unused_fd();
    if ( nfd >= 0 ) {
        fd_install( nfd, fd );
    }

    // open snap directory
    snap_fd = filp_open( DST_DIR, flags, 0 );
    if ( !snap_fd ) {
        err( "failed to open : %s", DST_DIR );
        filp_close( fd, current->files );
        return -2;
    }

    args.fd = 0;
    args.name[0] = '\0';

    // sync, since btrfs only snapshots on-disk blocks
    // (see http://www.mail-archive.com/linux-btrfs@vger.kernel.org/msg04134.html)
    vfs_ioctl( fd, BTRFS_IOC_SYNC, (unsigned long) &args );

    // fill #fd of d directory & snapid
    args.fd = nfd;
    snprintf( args.name, BTRFS_PATH_NAME_MAX, "%d.%d",
              1,
              snap_count );

    // create snapshot
    vfs_ioctl( snap_fd, BTRFS_IOC_SNAP_CREATE, (unsigned long) &args );

    // remove #fd
    close_fd( nfd );

    filp_close( snap_fd, current->files );
    filp_close( fd, current->files );

    set_fs( old_fs );

    return 1;
}

//
// xdr testing
//

static void dump_char( char * buf, int i, int len )
{
    int j = 0;
    printk( " | " );
    for ( j = i ; j < i + len ; j ++ ) {
        if ( ('a' <= buf[j] && buf[j] <= 'z') ||
             ('0' <= buf[j] && buf[j] <= '9' ) ) {
            printk( "%c", buf[j] );
        } else {
            printk( "." );
        }
    }
}


static void __attribute__((unused)) dump( char * buf, int len )
{
    int i, j;
    for ( i = 0 ; i < len ; i ++ ) {
        if ( i % 16 == 0 ) {
            if ( i != 0 ) {
                dump_char( buf, i - 16, 16 );
            }
            printk( KERN_INFO "%08X  ", i );
        }

        printk( "%02X ", (unsigned char) buf[i] );
    }

    if ( len % 16 ) {
        for ( j = 0 ; j < 16 - len % 16 ; j ++ ) {
            printk( "   " );
        }
    }

    dump_char( buf, (len / 16) * 16, len % 16 );
}

#ifdef XDR_TEST

# define malloc vmalloc
# include "example/xdr/shared_xdr.h"
# undef malloc

void prepare_test_xdr( void )
{
    XDR xdr;
    struct xdr_buf buf;

    printk( KERN_INFO  "sizeof( quad_t )         : %ld\n", sizeof( quad_t         ) );
    printk( KERN_INFO  "sizeof( stat_info )      : %ld\n", sizeof( stat_info      ) );
    printk( KERN_INFO  "sizeof( namei_info )     : %ld\n", sizeof( namei_info     ) );
    printk( KERN_INFO  "sizeof( namei_infos )    : %ld\n", sizeof( namei_infos    ) );
    printk( KERN_INFO  "sizeof( argv_str )       : %ld\n", sizeof( argv_str       ) );
    printk( KERN_INFO  "sizeof( syscall_arg )    : %ld\n", sizeof( syscall_arg    ) );
    printk( KERN_INFO  "sizeof( syscall_record ) : %ld\n", sizeof( syscall_record ) );

    record_buf = vmalloc( RECORD_BUF_SIZE * sizeof( record_buf[0] ) );
    memset( record_buf, 0, sizeof( record_buf[0] ) * RECORD_BUF_SIZE );

    buf.head[0].iov_base = record_buf;
    buf.head[0].iov_len = 0;
    buf.tail[0].iov_len = 0;
    buf.page_len = 0;
    buf.flags = 0;
    buf.len = 0;
    buf.buflen = RECORD_BUF_SIZE;

    xdr_init_encode( &xdr, &buf, NULL );

    test_xdr( &xdr );
    dump( record_buf, (int)( (unsigned long) xdr.p - (unsigned long) record_buf ) );

    vfree( record_buf );
}
#endif

// ======================================================================
// init/exit functions
//

unsigned int syslogs_initialized = 0;

static int __mkdir( char * dir )
{
    int err;

    struct nameidata nd;
    struct dentry * dentry;

    err = path_lookup( dir, LOOKUP_PARENT, &nd );
    if ( err ) {
        err( "path_lookup:%s", dir );
        return err;
    }

    dentry = lookup_create( &nd, 1 );
    err = PTR_ERR( dentry );
    if ( !IS_ERR(dentry) ) {

        err = mnt_want_write( nd.path.mnt );
        if (err)
            goto out_dput;

        err = nd.path.dentry->d_inode->i_op->mkdir( nd.path.dentry->d_inode,
                                                    dentry,
                                                    S_IRWXU |
                                                    S_IRGRP |
                                                    S_IXGRP |
                                                    S_IROTH |
                                                    S_IWOTH |
                                                    S_IXOTH );

        dentry->d_inode->i_uid = 0;
        dentry->d_inode->i_gid = 0;

        // err = vfs_mkdir( nd.path.dentry->d_inode, dentry, mode );
        mnt_drop_write(nd.path.mnt);
        dput( dentry );
    }

    mutex_unlock( &nd.path.dentry->d_inode->i_mutex );

 out_dput:
    path_put( &nd.path );

    return err;
}

static int syslogs_init( void )
{
#ifdef XDR_TEST
    prepare_test_xdr();
    return 0;
#endif

    // mkdir the archive directory
    __mkdir( RECORD_ARCHIVE );

    // fill syscall's args table
    init_syscalls();

    syslogs_check_memory( __sys_call_table );
    if ( check_sanity() && syslogs_set_memory_rw( __sys_call_table ) ) {
        syslogs_check_memory( __sys_call_table );

        // init caches
        init_cache_syscall_arg();
        init_cache_argv_str();
        init_cache_namei_infos();
        init_cache_namei_info();
        init_cache_stat_info();
        init_cache_string();
        init_cache_exec_info();
        init_cache_big_stat_info();
        init_cache_dirent_infos();

        // init record buffer
        init_xdr();

        // overwrite system call table
        HOOK_SYSCALLS();

        // create proc interface
        install_proc();

        // mark initialized
        syslogs_initialized = 1;

        // mark as new log
        printk( KERN_INFO "==========================================" );
        printk( KERN_INFO "complete to load syslog" );
    } else {
        printk( KERN_INFO "failed to install syslog" );
        return -1;
    }

    return 0;
}

static void syslogs_exit( void )
{
    if ( syslogs_initialized ) {

        // free recording buffer
        exit_xdr();

        // uninstall proc interface
        uninstall_proc();

        // restore syscall call table
        UNHOOK_SYSCALLS();

        // optimistic grace time
        msleep(10);

        // vfree on cache
        exit_cache_syscall_arg();
        exit_cache_argv_str();
        exit_cache_namei_infos();
        exit_cache_namei_info();
        exit_cache_stat_info();
        exit_cache_string();
        exit_cache_exec_info();
        exit_cache_big_stat_info();
        exit_cache_dirent_infos();
    }

    printk( KERN_INFO "complete to unload syslog" );
    printk( KERN_INFO "==========================================" );
}

module_init( syslogs_init );
module_exit( syslogs_exit );

MODULE_LICENSE( "Dual BSD/GPL" );
