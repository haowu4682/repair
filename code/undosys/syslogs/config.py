#! /usr/bin/env python
#
# find syscall table
#

import os
import sys

# append necessary functions
hijacks = ["sys_call_table",
           "ia32_sys_call_table",
           
           "_stext",
           "__end_rodata",
           "sys_link",
           
           # to copy
           "rw_verify_area",

           # 32 stub
           "stub32_fork",
           "stub32_execve",
           "sys32_execve",
           "ia32_ptregs_common",
           
           # stubs
           "sys_fork",
           "stub_fork",
           "sys_vfork",
           "stub_vfork",
           "sys_clone",
           "stub_clone",
           "sys_execve",
           "stub_execve"]

def system( cmd ) :
    return os.popen( cmd ).read()[:-1]

kver = system("uname -r")
sys_map = "/boot/System.map-" + kver

if not os.path.exists( sys_map ) :
    print "could not find '%s' file" % sys_map
    exit(1)

def get_addr( fn ) :
    return (fn, system( "grep ' %s$' %s" % ( fn, sys_map ) ).split()[0])

addrs = [ get_addr( addr ) for addr in hijacks ]

if len( addrs ) != len( hijacks ) :
    print "could not find enough information to compile"
    exit(1)

config_fn = "syslogs_" + kver + "_config.h"
fd = open(config_fn, "w")

fd.write( """\
#ifndef _SYSLOGS_CONFIG_H_
#define _SYSLOGS_CONFIG_H_

// TARGET SYSTEM : %s

""" % sys_map )

for (fn,addr) in addrs :
    fd.write( "# define %-20s (0x%s)\n" % ( "__" + fn, addr ) )

fd.write( "\n#endif\n" )
fd.close()

if len( sys.argv ) == 1 :
    print ">> Generated %s for %s" % (config_fn, sys_map)

exit(0)
