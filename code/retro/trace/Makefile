## For stock installed kernel
KVER := $(shell uname -r)
#KVER := 2.6.39
KDIR := /lib/modules/$(KVER)/build

## For kernel build tree
#KDIR := /home/nickolai/refsrc/linux-git

PWD  := `pwd`

COMM_KHDRS  := trace_syscalls.inc hook_syscalls.inc nm.h dbg.h \
		    config.h

RETRO_KHDRS := $(COMM_KHDRS) syscall.h btrfs-ioctl.h namei.c
RETRO_KSRCS := ktrace.c sysarg.c syscall.c fs.c kprobes.c util.c \
	       	unlink.c btrfs.c sha1.c pathid.c
RETRO_KOBJS := $(RETRO_KSRCS:.c=.o)

IGET_KSRCS := igetfs.c util.c
IGET_KOBJS := $(IGET_KSRCS:.c=.o)

HOOKS_KHDRS := $(COMM_KHDRS) hook_fs.h
HOOKS_KSRCS := hook.c hook_fs.c hook_ptregscall.c util.c
HOOKS_KOBJS := $(HOOKS_KSRCS:.c=.o)

obj-m += retro.o iget.o khook.o
retro-objs := $(RETRO_KOBJS)
iget-objs  := $(IGET_KOBJS)
khook-objs := $(HOOKS_KOBJS)

all := khook.ko retro.ko iget.ko 
all: $(all)

iget.ko retro.ko: $(RETRO_KSRCS) $(IGET_KSRCS) $(RETRO_KHDRS)
	make -C $(KDIR) M=$(PWD) modules

khook.ko: $(HOOKS_KSRCS) $(HOOKS_KHDRS)
	make -C $(KDIR) M=$(PWD) modules

#trace_syscalls.inc: trace_syscalls.py ../repair/syscall.py
#	PYTHONPATH=../repair ./$< > $@

hook_syscalls.inc: hook_syscalls.py ../repair/syscall.py
	PYTHONPATH=../repair ./$< > $@

nm.h: nm.py
	./$< "$(KVER)" $(KDIR)/System.map > $@

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -f $(all) *.o *.inc nm.h

.PHONY: all clean
