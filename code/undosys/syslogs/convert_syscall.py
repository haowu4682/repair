#! /usr/bin/python

# 1. lookup TAG line
# 2. lookup SOURCE code
# 3. extract line
# 4. convert to our hooking format

import sys

KERNEL = "/home/taesoo/Working/kernel/linux-2.6.31"

TAGS = open( "%s/TAGS" % KERNEL ).readlines()

def find( name ) :
    src = ""
    for i in range( len(TAGS) ) :
        l = TAGS[i]
        if l != "" and ord(l[0]) == 12 :
            src = TAGS[i+1]
        if (l.find( "(" + name + ")" ) >= 0 or l.find( "(" + name + "," ) >= 0) \
               and l.find( "SYSCALL" ) >= 0 :
            return (src.split(",")[0], int(l.split(",")[-1]))

def find_func( src, pos ) :
    content = open( "%s/%s" % ( KERNEL, src ) ).read()

    i = pos
    while i < len( content ) :
        if content[i] == ")" :
            break
        i += 1

    return content[pos:i+1].replace( "SYSCALL_DEFINE", "DEF_HOOK_FN_" )

proto = open( "syscall_proto.h", "w" )
hooks = open( "syscall_hooks.h", "w" )

proto.write( """
#ifndef _SYSCALL_PROTO_H_
#define _SYSCALL_PROTO_H_
""" )

hooks.write( """
#ifndef _SYSCALL_HOOKS_H_
#define _SYSCALL_HOOKS_H_

#define HOOKS( FN )                   \
    do {                              \
""" )

while True :
    try :
        func = raw_input()
    except EOFError :
        break
    
    try :
        proto.write( find_func( *find( func ) ) + ";\n" )
        hooks.write( "FN(%-20s);\\\n" % func )
    except :
        print "Ignore: %s" % func

proto.write( "#endif /* _SYSCALL_PROTO_H_ */" )
proto.close()

hooks.write( "    } while ( 0 )\n\n#endif /* _SYSCALL_HOOKS_H_ */" )
hooks.close()

#
# check manually
# 
# Ignore: arch_prctl
# Ignore: clone
# Ignore: execve
# Ignore: fork
# Ignore: mmap
# Ignore: rt_sigreturn
# Ignore: sysarg.h
# Ignore: undo_func_end
# Ignore: undo_func_start
# Ignore: undo_mask_end
# Ignore: undo_mask_start
# Ignore: undosysarg.h
# Ignore: vfork

#
# initial candidates
# 
# FN( unlinkat               ); \
# FN( wait4                  ); \
# FN( close                  ); \
# FN( open                   ); \
# FN( read                   ); \
# FN( write                  ); \
# FN( writev                 ); \
# FN( ftruncate              ); \
# FN( mmap                   ); \
# FN( creat                  ); \
# FN( truncate               ); \
# FN( rename                 ); \
# FN( openat                 ); \
# FN( exit                   ); \
# FN( exit_group             ); \
#                               \
# FN##_S( execve             ); \
# FN##_S( clone              ); \
