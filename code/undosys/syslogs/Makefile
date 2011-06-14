proj  = undosys

sysl  = syslogs
bins  = example/0.child/child example/loader/loader
xdr   = record
args  = sysarg

obj-m  = $(proj).o
$(proj)-objs := $(sysl).o  $(xdr)_xdr.o $(args).o

KVERSION = $(shell uname -r)

all: $(proj).ko $(bins)

$(proj).ko: $(sysl)_$(KVERSION)_config.h $(sysl).c $(xdr)_xdr.c $(xdr).h \
	$(args).c cache.h syscall_hooks.h syscall_proto.h ia32_syscall_def.h ia32_sys_table.h
	echo "#include \"$(sysl)_$(KVERSION)_config.h\"" > $(sysl)_config.h
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules

$(xdr)_xdr.c: convert_xdr.py ../strace-4.5.19/record_xdr.c
	./convert_xdr.py > $@

$(xdr).h: convert_record.py krn_xdr.h ../strace-4.5.19/record.h
	./convert_record.py > $@

#
# now it is manually modifed
#
# $(args).c: convert_sysarg.py ../strace-4.5.19/sysarg.c
# 	./convert_sysarg.py >$@ 2>sysarg.log

$(sysl)_$(KVERSION)_config.h: config.py /boot/System.map-$(KVERSION)
	./config.py

clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
	rm -f $(sysl)_config.h $(sysl)_*_config.h $(bins) $(xdr).h

insmod:
	sudo insmod $(proj).ko

rmmod:
	sudo rmmod $(proj)

test: all
	(make rmmod; true) ; make insmod ; dmesg | tail -30

dep:
	gcc -MM *.c >> Makefile

# DEPEDNECY
syslogs.o: syslogs.c syslogs_config.h syslogs_2.6.31-20-generic_config.h \
  btrfs_ioctl.h

