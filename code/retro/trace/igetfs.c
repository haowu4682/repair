#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/file.h>
#include <linux/magic.h>
#include <linux/kallsyms.h>
#include <linux/namei.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/dcache.h>
#include "fs.h"
#include "nm.h"
#include "util.h"

static struct dentry *d_iget;

/*
 * iget interface, from user-space:
 *
 *    dir = open("/", O_RDWR);
 *    ctl = open("/sys/kernel/debug/retro/iget", O_RDWR);
 *    ioctl(ctl, dir, inode_num);
 *
 *    char pathbuf[64];
 *    sprintf(&pathbuf[0], "/.ino.%d", inode_num);
 *    ifd = open(pathbuf, O_RDWR);
 *
 * works both for files and directories.
 * under memory pressure, dentry might get recycled, so retry ioctl.
 */

struct __our_btrfs_key {
	u64 objectid;
	u8 type;
	u64 offset;
} __attribute__ ((__packed__));

typedef unsigned long (*fn_module_kallsyms_lookup_name)(const char *name);
static fn_module_kallsyms_lookup_name __sym_module_kallsyms_lookup_name = \
    (fn_module_kallsyms_lookup_name) NM_module_kallsyms_lookup_name;

static long iget_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int fsfd = cmd;
	struct file *fsfile = 0;
	struct super_block *sb = 0;

	u64 ino = arg;
	struct inode *i = 0;
	struct dentry *dentry = 0;
	char fakename[64];
	struct qstr qs;

	int error = 0;

	fsfile = fget(fsfd);
	if (!fsfile) {
		error = -EBADF;
		goto out2;
	}

	sb = fsfile->f_dentry->d_sb;
	if (sb->s_magic == EXT4_SUPER_MAGIC) {
		struct inode *(*__sym_ext4_iget) (struct super_block *,
						  unsigned long) =
			NM_ext4_iget;

		if (!__sym_ext4_iget) {
			error = -EOPNOTSUPP;
			goto out2;
		}

		i = __sym_ext4_iget(sb, ino);
	}

	if (sb->s_magic == BTRFS_SUPER_MAGIC) {
		static struct inode *(*__sym_btrfs_iget) (struct super_block *,
							  struct __our_btrfs_key *,
							  void * /* btrfs_root */,
							  int *) =
			NM_btrfs_iget;
		struct __our_btrfs_key k;
		void *btrfs_root;

		if (!__sym_btrfs_iget ) {
			__sym_btrfs_iget =
				(void *) __sym_module_kallsyms_lookup_name("btrfs_iget");
			if (!__sym_btrfs_iget) {
				error = -EOPNOTSUPP;
				goto out2;
			}
		}

		k.objectid = ino;
		k.type = 1;
		k.offset = 0;
		btrfs_root = retro_inode_to_btrfs_root(fsfile->f_dentry->d_inode);

		i = __sym_btrfs_iget(sb, &k, btrfs_root, 0);
	}

	if (IS_ERR(i)) {
		error = -ENOENT;
		goto out2;
	}

	snprintf(&fakename[0], sizeof(fakename), ".ino.%lld", ino);
	qs.name = fakename;
	qs.len = strlen(fakename);
	qs.hash = full_name_hash(qs.name, qs.len);

	mutex_lock(&fsfile->f_dentry->d_inode->i_mutex);
	dentry = d_lookup(fsfile->f_dentry, &qs);
	if (!dentry) {
		dentry = d_alloc(fsfile->f_dentry, &qs);
		if (!dentry) {
			mutex_unlock(&fsfile->f_dentry->d_inode->i_mutex);
			error = -ENOMEM;
			goto out1;
		}
	}

	if (dentry->d_inode) {
		if (dentry->d_inode != i) {
			error = -EEXIST;
		} else {
			error = 0;
		}
	} else {
		struct dentry *new = d_splice_alias(i, dentry);
		if (new) {
			dput(dentry);
			dentry = new;
		}
		i = 0;
		error = 0;
	}
	mutex_unlock(&fsfile->f_dentry->d_inode->i_mutex);
 out1:
	if (dentry && !IS_ERR(dentry))
		dput(dentry);
	if (i)
		iput(i);

 out2:
	if (fsfile)
		fput(fsfile);
	return error;
}

static struct file_operations iget_file_operations = {
	.open		= nonseekable_open,
	.unlocked_ioctl	= iget_ioctl,
};

static int __init iget_init(void)
{
	d_iget = debugfs_create_file("iget", 0600, 0, 0, &iget_file_operations);
	if (!d_iget) {
		printk(KERN_ERR "'iget' file creation failed\n");
		return -EIO;
	}

	return 0;
}

static void __exit iget_exit(void)
{
	debugfs_remove(d_iget);
}

module_init(iget_init);
module_exit(iget_exit);

MODULE_LICENSE("Dual MIT/GPL");

