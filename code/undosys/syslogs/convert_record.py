#! /usr/bin/python

import os

fd = open( "../strace-4.5.19/record.h" )

record = []

# skip #include and comments
for l in fd :
    if l.startswith( "struct" ) :
        record.append( l.rstrip() )
        break

# copy struct declarations
for l in fd :
    if l.startswith( "/* the xdr functions */" ) :
        break
    record.append( l.rstrip() )

# copy function declarations
for l in fd :
    if l.startswith( "extern" ) :
        # strip extern
        record.append( " ".join( l.split()[1:] ) )
    elif l.startswith( "#else" ) :
        break
#
# $ cat record_xdr.c | grep " (\!" | cut -c 8- | awk '{print $1}' | sort |uniq
#
# xdr_array
# xdr_bool
# xdr_bytes
# xdr_int
# xdr_pointer
# xdr_quad_t
# xdr_string
#
# void xdr_init_encode(struct xdr_stream *xdr, struct xdr_buf *buf, __be32 *p);

print "#ifndef __RECORD_H__"
print "#define __RECORD_H__\n"

print "#include <linux/types.h>"
print "#include <linux/sunrpc/xdr.h>"

print open( "krn_xdr.h" ).read()

print "\n".join( record )

print "\n#endif"
