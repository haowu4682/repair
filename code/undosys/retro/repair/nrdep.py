import os
import stat

import util

from sysarg import *
from syscall import *

def nrsucc(r):
	if not hasattr(r, "ret"):
		return False
	v = r.ret
	if isinstance(v, int):
		return v >= 0
	if isinstance(v, file):
		assert (v.fd < 0) == (v.inode is None)
		return v.inode is not None
	if isinstance(v, process):
		return v.pid > 0
	assert False

# optimistic (faster) decision to find writers
# input record could be entry/exit record
def check_rw(r):
	rs = False
	ws = False
	
	for i, a in enumerate(syscalls[r.nr].args):
		if not r.args.has_key(a.name): continue
		v = r.args[a.name]
		if isinstance(v, namei):
			return (True, True)
		if isinstance(v, file) and v.inode is not None:
			if a.usage & READ:
				rs = True
			if a.usage & WRITE:
				ws = True
			if a.usage & TRUNCATE:
				ws = True
			if a.usage & REMOVE:
				ws = True
				
	if hasattr(r, "ret") and nrsucc(r):
		if isinstance(r.ret, file):
			if r.nr in [NR_open, NR_openat]:
				if r.usage == 1:
					ws = True
				else :
					inode = r.args["path"].inode
					if r.args["flags"] & os.O_TRUNC:
						ws = True
					if inode is None:
						ws = True
	return (rs, ws)
	
# input record should be merged with entry/exit records
# @util.memorize
def nrdep(r):
	rs = set()
	ws = set()
	sc = syscalls[r.nr]
	
	for i, a in enumerate(sc.args):
		if not r.args.has_key(a.name): continue
		v = r.args[a.name]
		if isinstance(v, namei):
			for e in v.entries:
				if stat.S_ISLNK(e.inode.mode):
					rs.add(e.inode)
				else:
					rs.add(e)
			if v.inode is not None:
				rs.add(v.inode)
			if nrsucc(r) and len(v.entries) > 0:
				# create/remove
				# the file didn't exist before creation
				if a.usage & CREATE and v.inode is None:
					#rs.remove(v.entries[-1])
					ws.add(v.entries[-1])
				if a.usage & REMOVE:
					assert v.inode is not None
					ws.add(v.entries[-1])
		if (isinstance(v, file) or isinstance(v, namei)) and v.inode is not None:
			if a.usage & READ:
				rs.add(v.inode)
				# TODO: off, len
			if a.usage & WRITE:
				ws.add(v.inode)
				# TODO: off, len
			if a.usage & TRUNCATE:
				ws.add(v.inode)
				# TODO: len
			if a.usage & REMOVE:
				ws.add(v.inode)
				
	if hasattr(r, "ret") and nrsucc(r):
		if isinstance(r.ret, process):
			assert r.ret.pid > 0
			if r.nr in [NR_wait4]: rs.add(r.ret)
		if isinstance(r.ret, file):
			assert r.ret.inode is not None
			if r.nr in [NR_open, NR_openat]:
				if r.args.has_key('path') and r.args.has_key('flags'):
					inode = r.args["path"].inode
					# file exists and truncated
					if r.args["flags"] & os.O_TRUNC and inode is not None:
						assert inode == r.ret.inode
						ws.add(inode)
					if inode is None:
						ws.add(r.ret.inode)
			elif r.nr in [NR_socket, NR_accept]:
				ws.add(r.ret.inode)
			# elif r.nr in [NR_dup, NR_dup2, NR_dup3]:
			# 	ws.add(r.ret.inode)
	return (rs, ws)

