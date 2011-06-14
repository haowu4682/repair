#! /usr/bin/python

#
# make it efficent to lookup arguments
#

import os
import re
import sys

#
# construct only valid syscall
#
undocalls = [ l.split()[1].replace( "__NR_", "" )
              for l in open( "undosysarg.h" )
              if l.startswith( "#define __NR" ) ]

syscalls = [ l.split()[1].replace( "__NR_", "" ) \
             for l in open( "unistd64.list" ) ]

syscalls.extend( undocalls )

#
# now convert sysarg.c
# 

fd = open( "../strace-4.5.19/sysarg.c" )

# HEADER
print """\
#include <asm/unistd.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/unistd.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <asm/statfs.h>

#include "sysarg.h"
#include "undosysarg.h"

// FIXME 512 -> max syscall number anyway
struct sysarg_call sysarg_calls[512];
int nsysarg_calls[512] = {0,};

void init_syscalls( void ) {

"""

# BODY
args = []

# skip #include and comments
for l in fd :
    if l.startswith( "struct" ) :
        break

scname = ""    
for l in fd :
    if l.startswith( "    { 0 }," ) :
        break

    m = re.search( '\s*"([^"]+)",', l )
    if m :
        # flush syscall
        if args :
            if scname in syscalls :
                print "sysarg_calls[__NR_%s] = " % scname
                print "   (struct sysarg_call)"
                print "     %s\n" % args[0].lstrip().rstrip(),
                print "       ",
                print "        ".join( args[1:] ),

                print "nsysarg_calls[__NR_%s] = %d;\n" % ( scname, len(args) - 2 )
                
            else :
                sys.stderr.write( "Can't find '%s' system call\n" % scname )

        scname = m.groups()[0]
        args = []

    l = l.replace( "} },", "} };" ).lstrip().replace( "\t", "    " )
    if len(l) != 0 and l.find("{") >= 0 :
        args.append( l )

print "}"
