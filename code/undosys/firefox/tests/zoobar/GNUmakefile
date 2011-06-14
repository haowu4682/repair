CC	:= gcc
LD	:= gcc
CFLAGS	:= -g -std=c99 -Werror
LDFLAGS	:= 
LDLIBS	:= -lcrypto

PROGS	:= zookld zookd zookfs zooksvc

all: install

install: $(PROGS)
	./chroot-setup.sh

zookld = zookld.c utility.c
zookld : $(patsubst %.c,%.o,$(zookld))

zookd = zookd.c utility.c
zookd : $(patsubst %.c,%.o,$(zookd))

zookfs = zookfs.c utility.c
zookfs : $(patsubst %.c,%.o,$(zookfs))

zooksvc = zooksvc.c
zooksvc : $(patsubst %.c,%.o,$(zooksvc))

-include *.d

lab%-handin.tar.gz: clean
	tar cf - `find . -type f | grep -v '^\.*$$' | grep -v '/CVS/' | grep -v '/\.svn/' | grep -v '/\.git/' | grep -v 'lab[0-9].*\.tar\.gz'` | gzip > $@

handin: lab2-handin.tar.gz
	@echo "Please visit http://pdos.csail.mit.edu/cgi-bin/893handin"
	@echo "and upload $<.  Thanks!"

.PHONY : clean install handin
clean :
	rm -rf *.o *.d $(PROGS)

