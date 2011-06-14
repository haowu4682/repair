#include <linux/fs.h>
#include <linux/kprobes.h>
#include "syscall.h"
#include "util.h"

typedef int (*fn_hook_check_cond)(void);
fn_hook_check_cond kprobes_check_cond = NULL;

static void trace_drop_inode(struct inode *inode)
{
	if (kprobes_chan && kprobes_check_cond()) {
		char buf[64];
		size_t len;
		len = vartime(buf);
		len += varinode(inode, buf + len);
		relay_write(kprobes_chan, buf, len);
	}
	jprobe_return();
}

static struct jprobe drop_inode_jprobe = {
	.entry = trace_drop_inode,
	.kp = {
		.symbol_name ="generic_drop_inode"
	},
};

int kprobes_init(void)
{
	unsigned long (*lookup_symbol)(const char *name) \
		= NM_module_kallsyms_lookup_name;
	
	kprobes_check_cond = (fn_hook_check_cond) lookup_symbol("hook_check_cond");
	if (!kprobes_check_cond) {
		err("failed to find `hook_check_cond'");
		return -1;
	}

	printk(KERN_INFO "kprobes:%p\n", (void *)kprobes_check_cond);

	return register_jprobe(&drop_inode_jprobe);
}

void kprobes_exit(void)
{
	unregister_jprobe(&drop_inode_jprobe);
}
