CFLAGS	:= -Wall -std=c99 -D_GNU_SOURCE=1 -g
CPPFLAGS:= -Wall -D_GNU_SOURCE=1 -g
LDFLAGS	:= -g

all:	replay dump

replay:	replay.o record_xdr.o do_syscall.o sysarg.o
	g++ $(CPPFLAGS) -o $@ $^

dump: dump.o record_xdr.o

dump2: dump2.o record_xdr.o

replay.o: replay.cpp record.h sysarg.h
	g++ $(CPPFLAGS) -c -o $@ $<

sysarg.o: sysarg.c sysarg.h sysarg-types.h

do_syscall.o: do_syscall.S

record_xdr.o: record_xdr.c

record.h record_xdr.c: record.x
	rpcgen record.x
	@sed 's/register int32_t \*buf;/register int32_t \*buf; (void)buf;/g' -i -- record_xdr.c

LINKF	:= record.x sysarg.h sysarg-types.h sysarg.c
STRACE	:= ../strace-4.5.19

$(LINKF): %: $(STRACE)/%
	ln -s $< $@

clean:
	rm -f replay dump *.o record.h record_xdr.c $(LINKF)

