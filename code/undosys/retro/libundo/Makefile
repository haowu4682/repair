CFLAGS	:= -g -std=c99 -D_GNU_SOURCE=1 -fPIC -Wall -Werror

LDFLAGS := -shared

LDLIBS  := -lundo -ldl -L.

all: libundo.so undotest

undosyscalls.inc: syscalls.py ../repair/syscall.py
	PYTHONPATH=../repair ./$< > $@

libundo.so: undocall.o 
	$(CC) $(LDFLAGS) -o $@ $<

libundowrap.so: undowrap.o
	$(CC) $(LDFLAGS) -o $@ $< $(LDLIBS)

libcwrap.so: pwd.o
	$(CC) $(LDFLAGS) -o $@ $< $(LDLIBS)

undotest: undotest.o libundo.so
	$(CC) $< -o $@ -lundo -L. -Wl,-rpath $(shell pwd)

undocall.h: undosyscalls.inc

%.o: %.c undocall.h
	$(CC) $(CFLAGS) $< -c -o $@

clean:
	rm -f *.o *.so *.a undotest undosyscalls.inc

.PHONY: all clean
