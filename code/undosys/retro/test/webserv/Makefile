CFLAGS	:= -m32 -g -std=c99 -fno-stack-protector -Wall -Werror -D_GNU_SOURCE=1
LDFLAGS	:= -m32
PROGS	:= httpd

all:	$(PROGS)

httpd:	httpd.o
	gcc $< -o $@ $(LDFLAGS)

%.o:	%.c
	gcc $< -c -o $@ $(CFLAGS)

%.o:	%.S
	gcc $< -c -o $@ $(CFLAGS)

clean:
	rm -f *.o *.pyc *.bin httpd $(PROGS)

.PRECIOUS: %.o
