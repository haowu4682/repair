#include "syscall.h"
#include "util.h"
#include "nm.h"
#include "dbg.h"

#include <linux/namei.h>
#include <linux/mount.h>
#include <linux/fs_struct.h>
#include <linux/slab.h>

/*
 * An alternative design may be to pre-emptively link all inodes into
 * a separate directory for safekeeping, and also intercept the system
 * call return.  Then, if unlink/rmdir wasn't successful, remove our
 * extra inode reference.
 */

typedef
long (*fn_unlinkat)( int dfd, const char __user * pathname, int flag);

fn_unlinkat sys_unlinkat = (fn_unlinkat)(NM_sys_unlinkat);

typedef
int (*fn_do_path_lookup)(int dfd, const char *name,
			 unsigned int flags, struct nameidata *nd);

static
fn_do_path_lookup do_path_lookup = (fn_do_path_lookup) NM_do_path_lookup;

typedef
int (*fn_user_path_parent)(int dfd, const char __user *path,
			   struct nameidata *nd, char **name);

static
fn_user_path_parent user_path_parent = (fn_user_path_parent) NM_user_path_parent;

static struct dentry *(*fn___lookup_hash) (struct qstr *name,
		       struct dentry *base, struct nameidata *nd)
	= NM___lookup_hash;

static struct dentry *
fn_lookup_hash(struct nameidata *nd) {
    return fn___lookup_hash(&nd->last, nd->path.dentry, nd);
}

/*
 * dirty version of sys_linkat call, the newname parameter should be in
 * the kernel memory page.
 */

extern int mnt_want_write(struct vfsmount *mnt);
extern void mnt_drop_write(struct vfsmount *mnt);

static
long sys_linkat(int olddfd, const char __user * oldname, const char * newname)
{
	struct dentry *new_dentry;
	struct nameidata nd;
	struct path old_path;
	int error;

	error = user_path_at(olddfd, oldname, 0, &old_path);
	if (error)
		return error;

	error = do_path_lookup(AT_FDCWD, newname, LOOKUP_PARENT, &nd);
	if (error)
		goto out;

	error = -EXDEV;
	if (old_path.mnt != nd.path.mnt)
		goto out_release;

	new_dentry = lookup_create(&nd, 0);
	error = PTR_ERR(new_dentry);
	if (IS_ERR(new_dentry))
		goto out_unlock;

	error = mnt_want_write(nd.path.mnt);
	if (error)
		goto out_dput;

	error = vfs_link(old_path.dentry, nd.path.dentry->d_inode, new_dentry);

	mnt_drop_write(nd.path.mnt);

out_dput:
	dput(new_dentry);

out_unlock:
	mutex_unlock(&nd.path.dentry->d_inode->i_mutex);

out_release:
	path_put(&nd.path);

out:
	path_put(&old_path);

	return error;
}

static
long sys_renameat(int olddfd, const char __user * oldname,
		  const char * to)
{
	struct dentry *old_dir, *new_dir;
	struct dentry *old_dentry, *new_dentry;
	struct dentry *trap;
	struct nameidata oldnd, newnd;
	char *from;
	int error;

	error = user_path_parent(olddfd, oldname, &oldnd, &from);
	if (error)
		goto exit;

	error = do_path_lookup(AT_FDCWD, to, LOOKUP_PARENT, &newnd);
	if (error)
		goto exit1;

	error = -EXDEV;
	if (oldnd.path.mnt != newnd.path.mnt)
		goto exit2;

	old_dir = oldnd.path.dentry;
	error = -EBUSY;
	if (oldnd.last_type != LAST_NORM)
		goto exit2;

	new_dir = newnd.path.dentry;
	if (newnd.last_type != LAST_NORM)
		goto exit2;

	oldnd.flags &= ~LOOKUP_PARENT;
	newnd.flags &= ~LOOKUP_PARENT;
	newnd.flags |= LOOKUP_RENAME_TARGET;

	trap = lock_rename(new_dir, old_dir);

	old_dentry = fn_lookup_hash(&oldnd);
	error = PTR_ERR(old_dentry);
	if (IS_ERR(old_dentry))
		goto exit3;

	/* source must exist */
	error = -ENOENT;
	if (!old_dentry->d_inode)
		goto exit4;

	/* unless the source is a directory trailing slashes give -ENOTDIR */
	if (!S_ISDIR(old_dentry->d_inode->i_mode)) {
		error = -ENOTDIR;
		if (oldnd.last.name[oldnd.last.len])
			goto exit4;
		if (newnd.last.name[newnd.last.len])
			goto exit4;
	}

	/* source should not be ancestor of target */
	error = -EINVAL;
	if (old_dentry == trap)
		goto exit4;

	new_dentry = fn_lookup_hash(&newnd);
	error = PTR_ERR(new_dentry);
	if (IS_ERR(new_dentry))
		goto exit4;

	/* target should not be an ancestor of source */
	error = -ENOTEMPTY;
	if (new_dentry == trap)
		goto exit5;

	error = mnt_want_write(oldnd.path.mnt);
	if (error)
		goto exit5;
	/*
	error = security_path_rename(&oldnd.path, old_dentry,
				     &newnd.path, new_dentry);
	*/

	if (error)
		goto exit6;
	error = vfs_rename(old_dir->d_inode, old_dentry,
				   new_dir->d_inode, new_dentry);
exit6:
	mnt_drop_write(oldnd.path.mnt);
exit5:
	dput(new_dentry);
exit4:
	dput(old_dentry);
exit3:
	unlock_rename(new_dir, old_dir);
exit2:
	path_put(&newnd.path);
exit1:
	path_put(&oldnd.path);
	putname(from);
exit:
	return error;
}

static
int exists(const char * path)
{
	struct nameidata nd;
	if (do_path_lookup(AT_FDCWD, path, 0, &nd)) {
		return 0;
	}
	path_put(&nd.path);
	return 1;
}

static
long get_ino(int dfd, const char * name)
{
	int error = 0;
	long ino = 0;
	struct dentry *dentry;
	struct nameidata nd;

	error = do_path_lookup(dfd, name, LOOKUP_PARENT, &nd);
	if (error)
		return error;

	mutex_lock_nested(&nd.path.dentry->d_inode->i_mutex, I_MUTEX_PARENT);
	dentry = fn_lookup_hash(&nd);
	error = PTR_ERR(dentry);
	if (!IS_ERR(dentry) && dentry->d_inode) {
		ino = dentry->d_inode->i_ino;
		dput(dentry);
	}
	mutex_unlock(&nd.path.dentry->d_inode->i_mutex);
	path_put(&nd.path);

	return ino;
}

static
int choose_archive(int dfd, const char __user *src, char *dest , int size)
{
	char mnt[256];
	char * name;
	char * iter;
	int len;
	long ino;

	name = getname(src);
	if (IS_ERR(name))
		return -1;

	ino = get_ino(dfd,name);
	len = strlen(name);

	// we know somehow unlink won't be successful, but be quiet
	if ( ino <= 0 ) {
		goto end;
	}

	// copy src path
	dbg(unlink, "%s/%s",
	    current->fs->root.dentry->d_name.name,
	    current->fs->pwd.dentry->d_name.name);

	{
		char tmp[256];
		struct dentry * d = current->fs->pwd.dentry;
		while (d && d != d->d_parent) {
			dbg(unlink, ">%s", d->d_name.name);
			strncpy(mnt, tmp, sizeof(mnt));
			snprintf(tmp, sizeof(tmp), "%s/%s", d->d_name.name, mnt);
			d = d->d_parent;
		}
		
		d = current->fs->pwd.mnt->mnt_mountpoint;
		while (d && d != d->d_parent) {
			dbg(unlink, ">%s", d->d_name.name);
			strncpy(mnt, tmp, sizeof(mnt));
			snprintf(tmp, sizeof(tmp), "%s/%s", d->d_name.name, mnt);
			d = d->d_parent;
		}
		snprintf(mnt, sizeof(mnt), "/%s", tmp);
	}

	// let's walk from the end
	iter = mnt + strlen(mnt);
	while (iter >= mnt) {
		if (*iter == '/') {
			snprintf(iter+1, sizeof(mnt) - len, ".inodes");
			dbg(unlink, "searching: %s", mnt);
			if (exists(mnt)) {
				break;
			}

			*iter = '\0';
		}
		iter --;
	}

	// default mnt, /tmp
	if ( *iter == '\0' ) {
		snprintf(mnt, sizeof(mnt), "/tmp");
	}

	// record with time for checkpoints
	{
		struct timespec ts;
		getnstimeofday(&ts);
		snprintf(dest, size, "%s/%ld.%ld.%ld", mnt, ino, (long) ts.tv_sec, (long) ts.tv_nsec); 
	}

	dbg(unlink, "%s => inode:%ld, %s", name, ino, dest);

 end:
	putname(name);

	return ino;
}

int handle_unlinkat(int dfd, const char __user *pathname)
{
	char dest[256];
	if ( choose_archive(dfd, pathname, dest, sizeof(dest)) > 0 ) {
		long error = sys_linkat(dfd, pathname, dest);
		if (error) {
			dbg(unlink, "failed to link:%ld", error);
			return error;
		}
	}

	return -1;
}

int handle_rmdirat(int dfd, const char __user *pathname)
{
	char dest[256];
	if ( choose_archive(dfd, pathname, dest, sizeof(dest)) > 0 ) {
		long error = sys_renameat(dfd, pathname, dest);
		if ( error ) {
			dbg(unlink, "failed to rename:%ld", error);
			return error;
		}
	}


	return -1;
}
