#include <linux/fs.h>
#include <linux/fdtable.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include <asm/uaccess.h>

#include "dbg.h"
#include "btrfs-ioctl.h"

extern void lock_kernel(void);
extern void unlock_kernel(void);
extern int get_unused_fd(void);
extern void fd_install(unsigned int fd, struct file *file);

static long vfs_ioctl(struct file * filp, unsigned int cmd,
                      unsigned long arg)
{
	int error = -ENOTTY;

	if (!filp->f_op)
		goto out;

	if (filp->f_op->unlocked_ioctl) {
		error = filp->f_op->unlocked_ioctl(filp, cmd, arg);
		if (error == -ENOIOCTLCMD)
			error = -EINVAL;
		goto out;
	}

 out:
	return error;
}

static void __put_unused_fd( struct files_struct * files, unsigned int fd )
{
	struct fdtable *fdt = files_fdtable(files);

	__FD_CLR(fd, fdt->open_fds);
	if ( fd < files->next_fd ) {
		files->next_fd = fd;
	}

	rcu_assign_pointer(fdt->fd[fd], NULL);
}

static void close_fd( unsigned int fd )
{
	struct files_struct *files = current->files;
	spin_lock(&files->file_lock);
	__put_unused_fd(files, fd);
	spin_unlock(&files->file_lock);
}

/*
 * Problem: what if we have a concurrent write to a file that's
 * blocked for some reason (e.g. reading a block from disk)?
 * Our checkpoint could be either before or after that write
 * completed.  Even stop_machine() would not solve that problem.
 * So, in the FS repair manager, we will need to check that there
 * are no file writes that overlap with the snapshot ioctl.
 *
 * Alternatively, we can either retry snapshots until we get
 * a snapshot with no concurrent writes, or prevent new system
 * calls from being made (e.g. block in trace_enter), wait for
 * all concurrent FS system calls to complete, and then snapshot.
 */

int take_snapshot(const char __user * trunk,
		  const char __user * parent,
		  const char __user * name )
{
	//
	// extract from btrfsctl.c (careful on file descriptors)
	//
	// fd = trunk's
	// snap_fd = parent's
	//
	// 1) sync
	//   ioctl( fd, BTRFS_IOC_SYNC, &args )
	//
	// 2) creating snapshot
	//   args.name = name
	//   args.fd = fd
	//   ioctl( snap_fd, BTRFS_IOC_SNAP_CREATE, &args )
	//

	static struct btrfs_ioctl_vol_args args;

	mm_segment_t old_fs;

	int nfd;

	struct file * snap_fd;
	struct file * fd;

	char * ktrunk;
	char * kparent;
	char * kname;

	const unsigned long flags \
		= O_RDONLY | O_NONBLOCK | O_DIRECTORY | O_CLOEXEC;

	ktrunk = getname(trunk);
	if (IS_ERR(ktrunk))
		goto end1;

	kparent = getname(parent);
	if (IS_ERR(kparent))
		goto end2;

	kname = getname(name);
	if (IS_ERR(kname))
		goto end3;

	// change memory to userspace
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	// open d directory
	fd = filp_open(ktrunk, flags, 0);
	if (!fd) {
		err("failed to open : %s", ktrunk);
		return -1;
	}

	// combine file descriptor to its fd number
	nfd = get_unused_fd();
	if (nfd >= 0) {
		fd_install(nfd, fd);
	}

	// open snap directory
	snap_fd = filp_open(kparent, flags, 0);
	if (!snap_fd) {
		err("failed to open : %s", kparent);
		filp_close( fd, current->files );
		goto end4;
	}

	args.fd = 0;
	args.name[0] = '\0';

	// sync, since btrfs only snapshots on-disk blocks
	// (see http://www.mail-archive.com/linux-btrfs@vger.kernel.org/msg04134.html)
	vfs_ioctl(fd, BTRFS_IOC_SYNC, (unsigned long) &args);

	// fill #fd of d directory & snapid
	args.fd = nfd;
	snprintf(args.name, BTRFS_PATH_NAME_MAX, "%s", kname);

	// create snapshot
	vfs_ioctl(snap_fd, BTRFS_IOC_SNAP_CREATE, (unsigned long) &args);

	// remove #fd
	close_fd(nfd);

	filp_close(snap_fd, current->files);
	filp_close(fd, current->files);

	set_fs(old_fs);

 end4:	
	putname(kname);
 end3:
	putname(kparent);
 end2:
	putname(ktrunk);
 end1:

	return 1;
}
