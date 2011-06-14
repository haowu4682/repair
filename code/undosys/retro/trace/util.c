#include <asm/cacheflush.h>
#include <asm/uaccess.h>
#include <linux/fdtable.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include "util.h"
#include "nm.h"

typedef unsigned long (*fn_module_kallsyms_lookup_name)(const char *name);
static fn_module_kallsyms_lookup_name __sym_module_kallsyms_lookup_name = \
    (fn_module_kallsyms_lookup_name) NM_module_kallsyms_lookup_name;

/*
 * from ksplice.c
 *
 * map_writable creates a shadow page mapping of the range
 * [addr, addr + len) so that we can write to code mapped read-only.
 *
 * It is similar to a generalized version of x86's text_poke.  But
 * because one cannot use vmalloc/vfree() inside stop_machine, we use
 * map_writable to map the pages before stop_machine, then use the
 * mapping inside stop_machine, and unmap the pages afterwards.
 */
void *map_writable(void *addr, size_t len)
{
	void *vaddr;
	int nr_pages = DIV_ROUND_UP(offset_in_page(addr) + len, PAGE_SIZE);
	struct page **pages = kmalloc(nr_pages * sizeof(*pages), GFP_KERNEL);
	void *page_addr = (void *)((unsigned long)addr & PAGE_MASK);
	int i;

	if (pages == NULL)
		return NULL;

	for (i = 0; i < nr_pages; i++) {
		if (__module_address((unsigned long)page_addr) == NULL) {
			pages[i] = virt_to_page(page_addr);
			WARN_ON(!PageReserved(pages[i]));
		} else {
			pages[i] = vmalloc_to_page(addr);
		}
		if (pages[i] == NULL) {
			kfree(pages);
			return NULL;
		}
		page_addr += PAGE_SIZE;
	}
	vaddr = vmap(pages, nr_pages, VM_MAP, PAGE_KERNEL);
	kfree(pages);
	if (vaddr == NULL)
		return NULL;
	return vaddr + offset_in_page(addr);
}

void unmap_writable(void *vaddr)
{
	vunmap((void *)((unsigned long)vaddr & PAGE_MASK));
}

/*
 * from fs/file_table.c
 *
 * reproduced here because it is not exported by the kernel.
 */
struct file *fget_light(unsigned int fd, int *fput_needed)
{
	struct file *file;
	struct files_struct *files = current->files;

	*fput_needed = 0;
	if (likely((atomic_read(&files->count) == 1))) {
		file = fcheck_files(files, fd);
	} else {
		rcu_read_lock();
		file = fcheck_files(files, fd);
		if (file) {
			if (atomic_long_inc_not_zero(&file->f_count))
				*fput_needed = 1;
			else
				/* Didn't get the reference, someone's freed */
				file = NULL;
		}
		rcu_read_unlock();
	}

	return file;
}

/*
 * from fs/exec.c
 *
 * count the number of strings in array ARGV.
 */
int count_argv(char __user * __user * argv, int max)
{
	int i = 0;
	if (argv != NULL) {
		for (;;) {
			char __user * p;
			if (get_user(p, argv))
				return -EFAULT;
			if (!p)
				break;
			argv++;
			if (i++ >= max)
				return -E2BIG;
			cond_resched();
		}
	}
	return i;
}

void *
retro_inode_to_btrfs_root(struct inode *i)
{
	static struct kmem_cache **__sym_btrfs_inode_cachep =
	    NM_btrfs_inode_cachep;

	void *btrfs_ino;
	void *btrfs_root;

	if (!__sym_btrfs_inode_cachep) {
		__sym_btrfs_inode_cachep = (void *)
			__sym_module_kallsyms_lookup_name("btrfs_inode_cachep");
		if (!__sym_btrfs_inode_cachep) {
			printk(KERN_ERR "cannot find btrfs_inode_cachep\n");
			return 0;
		}
	}

	/* root pointer is the first ptr in the btrfs_inode */
	btrfs_ino = i;
	btrfs_ino += sizeof(struct inode);
	btrfs_ino -= (*__sym_btrfs_inode_cachep)->objsize;
		     /* i.e., sizeof(struct btrfs_inode) */
	btrfs_root = *((void**) btrfs_ino);

	return btrfs_root;
}

static DEFINE_PER_CPU_SHARED_ALIGNED(struct timespec, ts_last);

size_t
vartime(char *buf)
{
	struct timespec *last = &__get_cpu_var(ts_last);
	struct timespec ts;
	size_t len;
	//ktime_get_ts(&ts);
	getnstimeofday(&ts);

	if (ts.tv_sec < last->tv_sec ||
	    (ts.tv_sec == last->tv_sec &&
	     ts.tv_nsec <= last->tv_nsec))
	{
	    ts.tv_sec  = last->tv_sec;
	    ts.tv_nsec = last->tv_nsec + 1;
	    if (ts.tv_nsec == 1000000000) {
		ts.tv_sec += 1;
		ts.tv_nsec = 0;
	    }
	}
	*last = ts;

	len = uvarint(ts.tv_sec, buf);
	len += uvarint(ts.tv_nsec, buf + len);
	return len;
}
