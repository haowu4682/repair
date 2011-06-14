CFLAGS := -g -std=c99 -D_GNU_SOURCE=1 -Wall -Werror -I.. -fPIC

all = libdbwrap.so bdbtest
all : $(all)

libdbwrap.so : bdb.o
	$(CC) -shared -o $@ $^ -lresolv -ldl

bdbtest : test.o
	$(CC) -o $@ $^ -ldb

%.o : %.c ../undocall.h
	$(CC) -c $(CFLAGS) -o $@ $<

clean :
	rm -f *.o $(all)
