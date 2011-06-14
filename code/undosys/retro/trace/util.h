#pragma once

#include <linux/fs.h>
#include <linux/ktime.h>
#include <linux/magic.h>
#include <linux/version.h>
#include <linux/fs_struct.h>

void *retro_inode_to_btrfs_root(struct inode *i);

void *map_writable(void *, size_t len);
void unmap_writable(void *);

int count_argv(char __user * __user * argv, int max);
size_t vartime(char *buf);

/* This function and the next serialize a variable-length
   (long) integer ABCDEFGHIJ....XYZ as
   (observe the 1's and 0's)
   1ABCDEFG ... 1MNOPQRS 0TUVWXYZ

 */
static inline size_t svarint(long v, char *buf)
{
	size_t nbits = 1 + ((v && v != -1)? (64 - __builtin_clzll((v < 0)? ~v: v)): 0);
	size_t nbytes = (nbits / 7) + ((nbits % 7)? 1: 0);
	size_t n = nbytes - 1;
	char *p = buf + n;
	*p = v & 0x7f;
	for (; n; --n) {
		v >>= 7;
		--p;
		*p = (v & 0x7f) | 0x80;
	}
	return nbytes;
}

/* See comment above */
static inline size_t uvarint(unsigned long v, char *buf)
{
	size_t nbits = (v)? (64 - __builtin_clzll(v)): 1;
	size_t nbytes = (nbits / 7) + ((nbits % 7)? 1: 0);
	size_t n = nbytes - 1;
	char *p = buf + n;
	*p = v & 0x7f;
	for (; n; --n) {
		v >>= 7;
		--p;
		*p = (v & 0x7f) | 0x80;
	}
	return nbytes;
}

static inline size_t varinode(struct inode *inode, char *buf)
{
	size_t len;
	dev_t dev;
	BUG_ON(inode->i_mode == 0);
	len = uvarint(inode->i_mode, buf);
	if (inode->i_sb->s_magic == BTRFS_SUPER_MAGIC &&
	    inode->i_op && inode->i_op->getattr) {
		struct dentry dfake;
		struct kstat stat;

		dfake.d_inode = inode;
		inode->i_op->getattr(0, &dfake, &stat);
		dev = stat.dev;
	} else {
		dev = inode->i_sb->s_dev;
	}
	len += uvarint(new_encode_dev(dev), buf + len);
	len += uvarint(inode->i_ino, buf + len);
	if (S_ISCHR(inode->i_mode) || S_ISBLK(inode->i_mode))
		len += uvarint(new_encode_dev(inode->i_rdev), buf + len);
	return len;
}

static inline void fs_lock(struct fs_struct * fs) 
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
	spin_lock(&fs->lock);
#else
	read_lock(&fs->lock);
#endif
}


static inline void fs_unlock(struct fs_struct * fs) 
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
	spin_unlock(&fs->lock);
#else
	read_unlock(&fs->lock);
#endif
}
