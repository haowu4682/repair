/*
 * filesystem interface for khook
 *
 */

#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/relay.h>
#include <linux/file.h>
#include <linux/magic.h>
#include <linux/kallsyms.h>
#include <linux/namei.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/stop_machine.h>

#include "nm.h"
#include "util.h"
#include "hook_fs.h"

int hook_enabled_flag = 0;
int hook_process_flag = 0;
pid_t hook_ctl_pid = 0;

static struct dentry *dir;
static struct dentry *d_enabled;
static struct dentry *d_process;
static struct dentry *d_ctl_pid;

int hook_fs_init(void)
{
	dir = debugfs_create_dir("khook", NULL);
	if (!dir) {
		printk(KERN_ERR "'khook' directory creation failed\n");
		goto fail;
	}

	d_enabled = debugfs_create_u32("enabled", 0600, dir, &hook_enabled_flag);
	if (!d_enabled) {
		printk(KERN_ERR "'enabled' file creation failed\n");
		goto fail;
	}

	d_process = debugfs_create_u32("process", 0600, dir, &hook_process_flag);
	if (!d_process) {
		printk(KERN_ERR "'process' file creation failed\n");
		goto fail;
	}

	d_ctl_pid = debugfs_create_u32("ctl", 0600, dir, &hook_ctl_pid);
	if (!d_ctl_pid) {
		printk(KERN_ERR "'ctl' file creation failed\n");
		goto fail;
	}

	return 0;

fail:
	hook_fs_exit();
	return -1;
}

void hook_fs_exit(void)
{
	if (d_ctl_pid)
		debugfs_remove(d_ctl_pid);
	if (d_process)
		debugfs_remove(d_process);
	if (d_enabled)
		debugfs_remove(d_enabled);
	if (dir) {
		debugfs_remove(dir);
		dir = NULL;
	}
}
