dump : dump.c shared_xdr.h
	gcc -std=c99 -g dump.c record_xdr.c -o dump

test : dump
	rm record.log; ./dump ; hexdump -C record.log