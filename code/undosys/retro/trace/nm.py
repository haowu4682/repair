#!/usr/bin/python

import os.path
import sys

prefix = "/boot/System.map"
suffixes = ["26", "-"+sys.argv[1]]
paths = [prefix+s for s in suffixes]
paths.append(sys.argv[2])
filename = [p for p in paths if os.path.exists(p)][0]
m = {}
#print("file name is %s\n" % filename)
for line in open(filename):
	[addr, dummy, symbol] = line.split()
	m[symbol] = addr

print("#pragma once\n")

ptregcalls = ["clone", "execve", "fork", "iopl", "rt_sigreturn", "sigaltstack", "vfork"]
symbols = ["sys_call_table"] \
	+ ["stub_"+x for x in ptregcalls] \
	+ ["sys_"+x for x in ptregcalls] \
	+ ["__d_lookup", "lookup_mnt"] \
	+ ["kallsyms_lookup_name", "module_kallsyms_lookup_name"] \
	+ ["ext4_iget", "sys_ioctl", "sys_unlinkat", "do_path_lookup"] \
	+ ["ptrace_notify", "__lookup_hash"] \
	+ ["user_path_parent", "socket_file_ops"]
symbols_optional = ["btrfs_iget", "btrfs_inode_cachep", "dummy_symbol"]

for s in symbols:
	print("#define NM_%-30s ((void *)0x%s)" % (s, m[s]))
for s in symbols_optional:
	print("#define NM_%-30s ((void *)0x%s)" % (s, m.get(s, 0)))

