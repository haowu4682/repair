/*
 *Copied selectively from linux kernel source:
 * fs/namei.c
 * 
 * copy static and other necessary functions here.
 *
 * modified by adding calls to ksysarg_*, as follows.
 * - do_lookup          for normal lookup
 * - follow_dotdot      for ".."
 * - __do_follow_link   for symbolic links
 */

#include <linux/mount.h>
#include <linux/namei.h>
#include <linux/version.h>
#include "nm.h"
#include "pathid.h"

struct vfsmount *lookup_mnt(struct path *path)
{
	static struct vfsmount *(*f)(struct path *) = NM_lookup_mnt;
	return f(path);
}

/*
 * This does basic POSIX ACL permission checking
 */
static int acl_permission_check(struct inode *inode, int mask,
		int (*check_acl)(struct inode *inode, int mask, unsigned flag))
{
	umode_t			mode = inode->i_mode;

	mask &= MAY_READ | MAY_WRITE | MAY_EXEC;

	if (current_fsuid() == inode->i_uid)
		mode >>= 6;
	else {
		if (IS_POSIXACL(inode) && (mode & S_IRWXG) && check_acl) {
			int error = check_acl(inode, mask, 0);
			if (error != -EAGAIN)
				return error;
		}

		if (in_group_p(inode->i_gid))
			mode >>= 3;
	}

	/*
	 * If the DACs are ok we don't need any capability check.
	 */
	if ((mask & ~mode) == 0)
		return 0;
	return -EACCES;
}

static inline struct dentry *
do_revalidate(struct dentry *dentry, struct nameidata *nd)
{
	int status = dentry->d_op->d_revalidate(dentry, nd);
	if (unlikely(status <= 0)) {
		/*
		 * The dentry failed validation.
		 * If d_revalidate returned 0 attempt to invalidate
		 * the dentry otherwise d_revalidate is asking us
		 * to return a fail status.
		 */
		if (!status) {
			if (!d_invalidate(dentry)) {
				dput(dentry);
				dentry = NULL;
			}
		} else {
			dput(dentry);
			dentry = ERR_PTR(status);
		}
	}
	return dentry;
}

/*
 * force_reval_path - force revalidation of a dentry
 *
 * In some situations the path walking code will trust dentries without
 * revalidating them. This causes problems for filesystems that depend on
 * d_revalidate to handle file opens (e.g. NFSv4). When FS_REVAL_DOT is set
 * (which indicates that it's possible for the dentry to go stale), force
 * a d_revalidate call before proceeding.
 *
 * Returns 0 if the revalidation was successful. If the revalidation fails,
 * either return the error returned by d_revalidate or -ESTALE if the
 * revalidation it just returned 0. If d_revalidate returns 0, we attempt to
 * invalidate the dentry. It's up to the caller to handle putting references
 * to the path if necessary.
 */
static int
force_reval_path(struct path *path, struct nameidata *nd)
{
	int status;
	struct dentry *dentry = path->dentry;

	/*
	 * only check on filesystems where it's possible for the dentry to
	 * become stale. It's assumed that if this flag is set then the
	 * d_revalidate op will also be defined.
	 */
	if (!(dentry->d_sb->s_type->fs_flags & FS_REVAL_DOT))
		return 0;

	status = dentry->d_op->d_revalidate(dentry, nd);
	if (status > 0)
		return 0;

	if (!status) {
		d_invalidate(dentry);
		status = -ESTALE;
	}
	return status;
}

/*
 * Short-cut version of permission(), for calling on directories
 * during pathname resolution.  Combines parts of permission()
 * and generic_permission(), and tests ONLY for MAY_EXEC permission.
 *
 * If appropriate, check DAC only.  If not appropriate, or
 * short-cut DAC fails, then call ->permission() to do more
 * complete permission check.
 */
static int exec_permission(struct inode *inode)
{
	int ret;

	if (inode->i_op->permission) {
        // XXX: The interface here has changed
		ret = inode->i_op->permission(inode, MAY_EXEC, 0);
		if (!ret)
			goto ok;
		return ret;
	}
	ret = acl_permission_check(inode, MAY_EXEC, inode->i_op->check_acl);
	if (!ret)
		goto ok;

	if (capable(CAP_DAC_OVERRIDE) || capable(CAP_DAC_READ_SEARCH))
		goto ok;

	return ret;
ok:
	return 0;
/*
	return security_inode_permission(inode, MAY_EXEC);
*/
}

static __always_inline void set_root(struct nameidata *nd)
{
	if (!nd->root.mnt) {
		struct fs_struct *fs = current->fs;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
		spin_lock(&fs->lock);
#else
		read_lock(&fs->lock);
#endif
		nd->root = fs->root;
		path_get(&nd->root);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
		spin_unlock(&fs->lock);
#else
		read_unlock(&fs->lock);
#endif
	}
}

static int link_path_walk(const char *, struct nameidata *);

static __always_inline int __vfs_follow_link(struct nameidata *nd, const char *link)
{
	if (IS_ERR(link))
		goto fail;

	if (*link == '/') {
		set_root(nd);
		path_put(&nd->path);
		nd->path = nd->root;
		path_get(&nd->root);
	}

	return link_path_walk(link, nd);
fail:
	path_put(&nd->path);
	return PTR_ERR(link);
}

static void path_put_conditional(struct path *path, struct nameidata *nd)
{
	dput(path->dentry);
	if (path->mnt != nd->path.mnt)
		mntput(path->mnt);
}

static inline void path_to_nameidata(struct path *path, struct nameidata *nd)
{
	dput(nd->path.dentry);
	if (nd->path.mnt != path->mnt) {
		mntput(nd->path.mnt);
		nd->path.mnt = path->mnt;
	}
	nd->path.dentry = path->dentry;
}

static __always_inline int
__do_follow_link(struct path *path, struct nameidata *nd, void **p)
{
	int error;
	struct dentry *dentry = path->dentry;

	touch_atime(path->mnt, dentry);
	nd_set_link(nd, NULL);

	if (path->mnt != nd->path.mnt) {
		path_to_nameidata(path, nd);
		dget(dentry);
	}
	mntget(path->mnt);
	nd->last_type = LAST_BIND;
	*p = dentry->d_inode->i_op->follow_link(dentry, nd);
	error = PTR_ERR(*p);
	if (!IS_ERR(*p)) {
		char *s = nd_get_link(nd);
		error = 0;
		if (s) {
			store_pathid(s, strlen(s), path->dentry->d_inode);
			error = __vfs_follow_link(nd, s);
		}
		else if (nd->last_type == LAST_BIND) {
			error = force_reval_path(&nd->path, nd);
			if (error)
				path_put(&nd->path);
		}
	}
	return error;
}

/*
 * This limits recursive symlink follows to 8, while
 * limiting consecutive symlinks to 40.
 *
 * Without that kind of total limit, nasty chains of consecutive
 * symlinks can cause almost arbitrarily long lookups. 
 */
static inline int do_follow_link(struct path *path, struct nameidata *nd)
{
	void *cookie;
	int err = -ELOOP;
	if (current->link_count >= MAX_NESTED_LINKS)
		goto loop;
	if (current->total_link_count >= 40)
		goto loop;
	BUG_ON(nd->depth >= MAX_NESTED_LINKS);
	cond_resched();
/*
	err = security_inode_follow_link(path->dentry, nd);
	if (err)
		goto loop;
*/
	current->link_count++;
	current->total_link_count++;
	nd->depth++;
	err = __do_follow_link(path, nd, &cookie);
	if (!IS_ERR(cookie) && path->dentry->d_inode->i_op->put_link)
		path->dentry->d_inode->i_op->put_link(path->dentry, nd, cookie);
	path_put(path);
	current->link_count--;
	nd->depth--;
	return err;
loop:
	path_put_conditional(path, nd);
	path_put(&nd->path);
	return err;
}

/* no need for dcache_lock, as serialization is taken care in
 * namespace.c
 */
static int __follow_mount(struct path *path)
{
	int res = 0;
	while (d_mountpoint(path->dentry)) {
		struct vfsmount *mounted = lookup_mnt(path);
		if (!mounted)
			break;
		dput(path->dentry);
		if (res)
			mntput(path->mnt);
		path->mnt = mounted;
		path->dentry = dget(mounted->mnt_root);
		res = 1;
	}
	return res;
}

static void follow_mount(struct path *path)
{
	while (d_mountpoint(path->dentry)) {
		struct vfsmount *mounted = lookup_mnt(path);
		if (!mounted)
			break;
		dput(path->dentry);
		mntput(path->mnt);
		path->mnt = mounted;
		path->dentry = dget(mounted->mnt_root);
	}
}

static __always_inline void follow_dotdot(struct nameidata *nd)
{
	store_pathid("..", 2, nd->path.dentry->d_inode);

	set_root(nd);

	while(1) {
		struct dentry *old = nd->path.dentry;

		if (nd->path.dentry == nd->root.dentry &&
		    nd->path.mnt == nd->root.mnt) {
			break;
		}
		if (nd->path.dentry != nd->path.mnt->mnt_root) {
			/* rare case of legitimate dget_parent()... */
			nd->path.dentry = dget_parent(nd->path.dentry);
			dput(old);
			break;
		}
		if (!follow_up(&nd->path))
			break;
	}
	follow_mount(&nd->path);
}

/*
 *  It's more convoluted than I'd like it to be, but... it's still fairly
 *  small and for now I'd prefer to have fast path as straight as possible.
 *  It _is_ time-critical.
 */
static int do_lookup(struct nameidata *nd, struct qstr *name,
		     struct path *path)
{
	struct vfsmount *mnt = nd->path.mnt;
	struct dentry *dentry, *parent;
	struct inode *dir;
	/*
	 * See if the low-level filesystem might want
	 * to use its own hash..
	 */
	if (nd->path.dentry->d_op && nd->path.dentry->d_op->d_hash) {
		int err = nd->path.dentry->d_op->d_hash(nd->path.dentry, nd->path.dentry->d_inode, name);
		if (err < 0)
			return err;
	}

	dentry = __d_lookup(nd->path.dentry, name);
	if (!dentry)
		goto need_lookup;
	if (dentry->d_op && dentry->d_op->d_revalidate)
		goto need_revalidate;
done:
	store_pathid(name->name, name->len, nd->path.dentry->d_inode);
	path->mnt = mnt;
	path->dentry = dentry;
	__follow_mount(path);
	return 0;

need_lookup:
	parent = nd->path.dentry;
	dir = parent->d_inode;

	mutex_lock(&dir->i_mutex);
	/*
	 * First re-do the cached lookup just in case it was created
	 * while we waited for the directory semaphore..
	 *
	 * FIXME! This could use version numbering or similar to
	 * avoid unnecessary cache lookups.
	 *
	 * The "dcache_lock" is purely to protect the RCU list walker
	 * from concurrent renames at this point (we mustn't get false
	 * negatives from the RCU list walk here, unlike the optimistic
	 * fast walk).
	 *
	 * so doing d_lookup() (with seqlock), instead of lockfree __d_lookup
	 */
	dentry = d_lookup(parent, name);
	if (!dentry) {
		struct dentry *new;

		/* Don't create child dentry for a dead directory. */
		dentry = ERR_PTR(-ENOENT);
		if (IS_DEADDIR(dir))
			goto out_unlock;

		new = d_alloc(parent, name);
		dentry = ERR_PTR(-ENOMEM);
		if (new) {
			dentry = dir->i_op->lookup(dir, new, nd);
			if (dentry)
				dput(new);
			else
				dentry = new;
		}
out_unlock:
		mutex_unlock(&dir->i_mutex);
		if (IS_ERR(dentry))
			goto fail;
		goto done;
	}

	/*
	 * Uhhuh! Nasty case: the cache was re-populated while
	 * we waited on the semaphore. Need to revalidate.
	 */
	mutex_unlock(&dir->i_mutex);
	if (dentry->d_op && dentry->d_op->d_revalidate) {
		dentry = do_revalidate(dentry, nd);
		if (!dentry)
			dentry = ERR_PTR(-ENOENT);
	}
	if (IS_ERR(dentry))
		goto fail;
	goto done;

need_revalidate:
	dentry = do_revalidate(dentry, nd);
	if (!dentry)
		goto need_lookup;
	if (IS_ERR(dentry))
		goto fail;
	goto done;

fail:
	return PTR_ERR(dentry);
}

/*
 * This is a temporary kludge to deal with "automount" symlinks; proper
 * solution is to trigger them on follow_mount(), so that do_lookup()
 * would DTRT.  To be killed before 2.6.34-final.
 */
static inline int follow_on_final(struct inode *inode, unsigned lookup_flags)
{
	return inode && unlikely(inode->i_op->follow_link) &&
		((lookup_flags & LOOKUP_FOLLOW) || S_ISDIR(inode->i_mode));
}

/*
 * Name resolution.
 * This is the basic name resolution function, turning a pathname into
 * the final dentry. We expect 'base' to be positive and a directory.
 *
 * Returns 0 and nd will have valid dentry and mnt on success.
 * Returns error and drops reference to input namei data on failure.
 */
static int link_path_walk(const char *name, struct nameidata *nd)
{
	struct path next;
	struct inode *inode;
	int err;
	unsigned int lookup_flags = nd->flags;
	
	while (*name=='/')
		name++;
	if (!*name)
		goto return_reval;

	inode = nd->path.dentry->d_inode;
	if (nd->depth)
		lookup_flags = LOOKUP_FOLLOW | (nd->flags & LOOKUP_CONTINUE);

	/* At this point we know we have a real path component. */
	for(;;) {
		unsigned long hash;
		struct qstr this;
		unsigned int c;

		nd->flags |= LOOKUP_CONTINUE;
		err = exec_permission(inode);
 		if (err)
			break;

		this.name = name;
		c = *(const unsigned char *)name;

		hash = init_name_hash();
		do {
			name++;
			hash = partial_name_hash(c, hash);
			c = *(const unsigned char *)name;
		} while (c && (c != '/'));
		this.len = name - (const char *) this.name;
		this.hash = end_name_hash(hash);

		/* remove trailing slashes? */
		if (!c)
			goto last_component;
		while (*++name == '/');
		if (!*name)
			goto last_with_slashes;

		/*
		 * "." and ".." are special - ".." especially so because it has
		 * to be able to know about the current root directory and
		 * parent relationships.
		 */
		if (this.name[0] == '.') switch (this.len) {
			default:
				break;
			case 2:	
				if (this.name[1] != '.')
					break;
				{
					int cpu = smp_processor_id();
					int rosave = syscall_chan_info[cpu].ro;
					syscall_chan_info[cpu].ro = 1;
					follow_dotdot(nd);
					syscall_chan_info[cpu].ro = rosave;
				}
				inode = nd->path.dentry->d_inode;
				/* fallthrough */
			case 1:
				continue;
		}
		/* This does the actual lookups.. */
		{
			int cpu = smp_processor_id();
			int rosave = syscall_chan_info[cpu].ro;
			syscall_chan_info[cpu].ro = 1;
			err = do_lookup(nd, &this, &next);
			syscall_chan_info[cpu].ro = rosave;
		}
		if (err)
			break;

		err = -ENOENT;
		inode = next.dentry->d_inode;
		if (!inode)
			goto out_dput;

		if (inode->i_op->follow_link) {
			int cpu = smp_processor_id();
			int rosave = syscall_chan_info[cpu].ro;
			syscall_chan_info[cpu].ro = 1;
			err = do_follow_link(&next, nd);
			syscall_chan_info[cpu].ro = rosave;

			if (err)
				goto return_err;
			err = -ENOENT;
			inode = nd->path.dentry->d_inode;
			if (!inode)
				break;
		} else
			path_to_nameidata(&next, nd);
		err = -ENOTDIR; 
		if (!inode->i_op->lookup)
			break;
		continue;
		/* here ends the main loop */

last_with_slashes:
		lookup_flags |= LOOKUP_FOLLOW | LOOKUP_DIRECTORY;
last_component:
		/* Clear LOOKUP_CONTINUE iff it was previously unset */
		nd->flags &= lookup_flags | ~LOOKUP_CONTINUE;
		if (lookup_flags & LOOKUP_PARENT)
			goto lookup_parent;
		if (this.name[0] == '.') switch (this.len) {
			default:
				break;
			case 2:	
				if (this.name[1] != '.')
					break;
				follow_dotdot(nd);
				inode = nd->path.dentry->d_inode;
				/* fallthrough */
			case 1:
				goto return_reval;
		}
		err = do_lookup(nd, &this, &next);
		if (err)
			break;

		inode = next.dentry->d_inode;
		if (follow_on_final(inode, lookup_flags)) {
			err = do_follow_link(&next, nd);
			if (err)
				goto return_err;
			inode = nd->path.dentry->d_inode;
		} else
			path_to_nameidata(&next, nd);
		err = -ENOENT;
		if (!inode)
			break;
		if (lookup_flags & LOOKUP_DIRECTORY) {
			err = -ENOTDIR; 
			if (!inode->i_op->lookup)
				break;
		}
		goto return_base;
lookup_parent:
		nd->last = this;
		nd->last_type = LAST_NORM;
		if (this.name[0] != '.')
			goto return_base;
		if (this.len == 1)
			nd->last_type = LAST_DOT;
		else if (this.len == 2 && this.name[1] == '.')
			nd->last_type = LAST_DOTDOT;
		else
			goto return_base;
return_reval:
		/*
		 * We bypassed the ordinary revalidation routines.
		 * We may need to check the cached dentry for staleness.
		 */
		if (nd->path.dentry && nd->path.dentry->d_sb &&
		    (nd->path.dentry->d_sb->s_type->fs_flags & FS_REVAL_DOT)) {
			err = -ESTALE;
			/* Note: we do not d_invalidate() */
			if (!nd->path.dentry->d_op->d_revalidate(
					nd->path.dentry, nd))
				break;
		}
return_base:
		return 0;
out_dput:
		path_put_conditional(&next, nd);
		break;
	}
	path_put(&nd->path);
return_err:
	return err;
}

static int path_walk(const char *name, struct nameidata *nd)
{
	struct path save = nd->path;
	int result;

	current->total_link_count = 0;

	/* make sure the stuff we saved doesn't go away */
	path_get(&save);

	result = link_path_walk(name, nd);
	if (result == -ESTALE) {
		/* nd->path had been dropped */
		current->total_link_count = 0;
		nd->path = save;
		path_get(&nd->path);
		nd->flags |= LOOKUP_REVAL;
		result = link_path_walk(name, nd);
	}

	path_put(&save);

	return result;
}

static int path_init(int dfd, const char *name, unsigned int flags, struct nameidata *nd)
{
	int retval = 0;
	int fput_needed;
	struct file *file;

	nd->last_type = LAST_ROOT; /* if there are only slashes... */
	nd->flags = flags;
	nd->depth = 0;
	nd->root.mnt = NULL;

	if (*name=='/') {
		set_root(nd);
		nd->path = nd->root;
		path_get(&nd->root);
	} else if (dfd == AT_FDCWD) {
		struct fs_struct *fs = current->fs;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
		spin_lock(&fs->lock);
#else
		read_lock(&fs->lock);
#endif
		nd->path = fs->pwd;
		path_get(&fs->pwd);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
		spin_unlock(&fs->lock);
#else
		read_unlock(&fs->lock);
#endif
	} else {
		struct dentry *dentry;

		file = fget_light(dfd, &fput_needed);
		retval = -EBADF;
		if (!file)
			goto out_fail;

		dentry = file->f_path.dentry;

		retval = -ENOTDIR;
		if (!S_ISDIR(dentry->d_inode->i_mode))
			goto fput_fail;

		retval = file_permission(file, MAY_EXEC);
		if (retval)
			goto fput_fail;

		nd->path = file->f_path;
		path_get(&file->f_path);

		fput_light(file, fput_needed);
	}
	return 0;

fput_fail:
	fput_light(file, fput_needed);
out_fail:
	return retval;
}

/* Returns 0 and nd will be valid on success; Retuns error, otherwise. */
static int do_path_lookup(int dfd, const char *name,
			  unsigned int flags, struct nameidata *nd)
{
	int retval = path_init(dfd, name, flags, nd);
	if (!retval)
		retval = path_walk(name, nd);
/*
	if (unlikely(!retval && !audit_dummy_context() && nd->path.dentry &&
				nd->path.dentry->d_inode))
		audit_inode(name, nd->path.dentry);
*/
	if (nd->root.mnt) {
		path_put(&nd->root);
		nd->root.mnt = NULL;
	}
	return retval;
}

struct dentry * __d_lookup(struct dentry * parent, struct qstr * name)
{
	struct dentry * (*f)(struct dentry *, struct qstr *) = NM___d_lookup;
	return f(parent, name);
}
