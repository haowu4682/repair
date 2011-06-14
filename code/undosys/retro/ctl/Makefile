CFLAGS += -g -O2 -Wall -Werror -std=c99 -D_GNU_SOURCE
LDLIBS += -ldb -lpthread -lrt

#CFLAGS += -DHAVE_ZLIB_H
#LDLIBS += -lz

#CFLAGS += -DHAVE_BZLIB_H
#LDLIBS += -lbz2

all := ctl_setroot retroctl retrosnap iget
all: $(all)

ctl_setroot: retroctl
	sudo chown root.root retroctl
	sudo chmod +s retroctl
retroctl: retroctl.o
retrosnap: retrosnap.o
iget: iget.o

clean:
	rm -f $(all) *.o

.PHONY: all clean
