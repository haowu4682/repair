import os
import time
import errno
import hashlib
import collections

from glob import glob
from copy import deepcopy

import mgrapi
import mgrutil
import dbg
import nrdep
import fsmgr
import ptymgr
import sockmgr
import sysarg
import runopts
import syscall
import kutil
import rerun
import hmac
import record
import osloader
import networkmgr
import fifomgr
import cdevmgr

from sha1 import sha1, hexdigest

class Rerun:
	def __init__(self, arg, ret, uids=collections.defaultdict(lambda:0)):
		assert arg
		assert ret

		self.arg   = arg
		self.ret   = ret
		self.uids  = uids
		self.proc  = None
		self.pids  = {}
		self.seq   = 0
		
		self.last   = collections.defaultdict(lambda:(None, None))
		self.state  = "init"
		self.wait_r = None
		self.ts     = None
		self.actors = set()

		self.expanding = False

	def add(self, actor):
		self.actors.add(actor)

	def is_state(self, s):
		return self.state == s
	
	def detach(self, r, ts):
		self.state  = "detached"
		self.wait_r = r
		self.ts     = (ts[0] + 1,) + ts[1:]
		r.ts        = self.ts + (self.seq,)
		self.seq   += 1
		
	def resume(self):
		self.state = "resumed"
		self.wait_r = None
		self.expanding = True

	def activate(self):
		self.state = "active"
		
	def execve(self, pid):
		enter = self.arg
		
		dbg.infom("!", "#R<re-execute#>: %s" % str(enter))
		fn   = enter.args["filename"].path
		argv = enter.args["argv"]
		envp = enter.args["envp"]

		[ret, root, pwd, fds] = self.ret.aux

		# set sequence number as execve()'s sid
		self.seq = enter.sid & ((1<<48)-1) + 1

		# XXX
		# - setup fds		
		self.proc = rerun.Ptrace([fn] + argv[1:], envp, 
					 kutil.get(pwd),
					 kutil.get(root) if root.ino != 2 else None,
					 self.uids)
		
		# enter/exit should be execve() too
		(r, p) = self.proc.load(True , None)
		(r, p) = self.proc.load(False, None)

		assert r.usage == 1 and r.nr == syscall.NR_execve
		
		# virtualize pid
		self.pids[pid] = r.pid
		dbg.rerun("vpid:%s -> pid:%s (active)" % (pid, r.pid))

		# enable rerun module
		self.state = "active"
		
	def clone(self, vpid, child_pid):
		pid = self.pids[vpid]
		(r, p) = self.proc.load(False, pid.pid)
		assert r.usage == 1 and r.nr == syscall.NR_clone

		# virtualize child pid
		self.pids[child_pid] = r.ret
		dbg.rerun("vpid:%s -> pid:%s (active)" % (child_pid, r.ret))

		return (r, p)
		
	def wait4(self, vpid):
		pid  = self.pids[vpid]
		proc = self.proc.procs[pid.pid]

		# wait4 = 61
		assert proc.getreg("rax") == 61

		# backup syscall parameters
		regs = proc.getregs()
		rax  = proc.getregs().orig_rax

		# add NOHANG option
		rdx  = proc.getreg("rdx") | os.WNOHANG
		proc.setreg("rdx", rdx)

		# fire
		(r, p) = self.proc.load(False, pid.pid)
		self.__fix_pid(r, vpid)
		self.__fix_ts(r, False)

		# no child, immediate return
		if r.ret == 0:
			# modify rip (+regs) to execute wait4() next time
			proc.setregs(regs)
			proc.setreg("rax", rax)

		dbg.rerunm("!", "SKIP: [%s] %s" % (r.sid, str(r)))

		# keep r, p to communicate between ret/arg actors
		self.set_last(r, p)
		
		return (r, p)

	def exit_group(self, pid):
		(r, p) = self.next(False, pid)
		del self.last[pid]
		del self.pids[pid]
		
		if len(self.pids) == 0:
			self.stop()

		return (None, None)

	def stop(self):
		if self.is_active():
			self.proc.stop()
			self.proc = None

	def is_active(self):
		return self.proc != None and self.is_state("active")

	def is_expanding(self):
		return self.is_active() and self.expanding

	def is_following(self):
		return self.is_active() and not self.expanding
	
	def __fix_pid(self, r, vpid):
		# nversely map pid
		assert self.pids[vpid].pid == r.pid.pid
		# fix it
		r.pid = vpid

	def __fix_ts(self, r, entering):
		if self.ts == None:
			return
		r.ts = self.ts + (self.seq,)
		self.seq += 1
		
	def next(self, entering, vpid):
		# convert pid
		pid = self.pids[vpid]

		# execute one step, transform to pid integer for rerun module
		(r, p) = self.proc.load(entering, pid.pid)
		if r == None or p == None:
			return (r, p)

		self.__fix_pid(r, vpid)
		self.__fix_ts(r, entering)

		dbg.rerunm("!", "[%s] %s" % (r.sid & ((1<<48)-1), str(r)))

		# keep r, p to communicate between ret/arg actors
		self.set_last(r, p)
		
		return (r, p)

	def set_last(self, r, p):
		self.last[r.pid] = r
		
	def get_last(self, pid):
		return self.last[pid]

class ProcessActor(mgrapi.ActorNode):
	def __init__(self, name, pid):
		super(ProcessActor, self).__init__(name)
		self.pid   = pid
		self.rerun = None
		self.r_arg = None
		self.r_ret = None
		self.checkpoints.add(mgrutil.InitialState(self.name + ('ckpt',)))
		
	@staticmethod
	def get(pid, r=None):
		name = ('pid', pid.pid, pid.gen)
		n = mgrapi.RegisteredObject.by_name(name)
		if n is None:
			n = ProcessActor(name, pid)
		n.__update(r)
		return n

	def __update(self, r):
		if r:
			assert isinstance(r, record.SyscallRecord)
			if r.nr == syscall.NR_execve:
				if r.usage == 0:
					self.r_arg = r
				else:
					self.r_ret = r

	def get_execve(self):
		name   = ('pid', self.pid.pid, self.pid.gen-1)
		parent = mgrapi.RegisteredObject.by_name(name)
		return (parent.r_arg, self.r_ret)

	def rollback(self,c):
		assert isinstance(c, mgrutil.InitialState)

# compare arguments of two system calls
def check_args(r, a):
	for k in r.args:
		# ignore mmap/munmap: addr (ASLR, change every execution)
		if r.nr in [syscall.NR_mmap, syscall.NR_munmap] and k == "addr":
			continue
		
		# ignore ioctl buf (user address) argment, (XXX need to check its value)
		if r.nr in [syscall.NR_ioctl] and k == "buf":
			continue
		
		# compare inode instead of `fd' number
		if isinstance(a.args[k], sysarg.file):
			# like, fd != -1 & pts and socket
			if a.args[k].inode \
				    and a.args[k].inode.prefix in ["pts", "socket"]:
				continue
			if a.args[k].inode != r.args[k].inode:
				dbg.trace("1)", a.args[k])
				dbg.trace("2)", r.args[k])
				return False
			continue

		# check identical
		if a.args[k] != r.args[k]:
			# slow path: because some obj could be deep-copied, str -> repr
			if str(a.args[k]) == str(r.args[k]):
				continue
			dbg.trace("1)", a.args[k])
			dbg.trace("2)", r.args[k])
			return False
	return True

		
class ProcSysCall(mgrapi.Action):
	def __init__(self, actor, argsnode):
		self.argsnode = argsnode
		super(ProcSysCall, self).__init__(argsnode.name + ('pcall',), actor)
		
	def redo(self):
		rerun = self.actor.rerun
		
		# following history while rerunning
		if rerun and rerun.is_following():
			(r, p) = rerun.next(True, self.actor.pid)
			if r == None:
				return
			assert r.usage == 0

			# wait4() strategy
			#
			# why special?
			#  - this repair routine() should serialize all the execution
			#    of child/parent process whatever
			#  - but the history graph was created with multiple processes
			#  - then blocking in the wait4() (surely parent process) is
			#    acceptible but we can't do that (see rerun.wait4())
			#  - we want to propagate our repair as far as possible without
			#    blocking
			#
			# how emulate?
			#  - return from (first) wait4() by modifying the argument of wait4
			#    to non blocking
			#  - since tic of rest syscalls of child process should precede
			#    this wait()'s tac, the next (second) wait4() should retrieve
			#    child exit status
			#
			# wrong implementation?
			#  - tic/tac of wait4() unfortunately overlap in many cases
			#  - so I intentionally change tac -> tic to avoid this situation
			#
			
			# avoid wait4 squeezed tic/tac time
			if r.nr == syscall.NR_wait4 \
				    and self.argsnode.origdata.nr != syscall.NR_wait4:

				dbg.stop()

				# wait4() again, at this time we should be able
				# to fetch child exit status
				(r, p) = rerun.next(False, self.actor.pid)
				assert r.usage == 1 and r.nr == syscall.NR_wait4

				# this is a real system call we should execute
				# on this action node
				(r, p) = rerun.next(True, self.actor.pid)
				assert r.usage == 0

			# match up
			if self.argsnode.origdata.nr == r.nr and \
				    check_args(r, self.argsnode.origdata):

				# update arguments
				self.argsnode.data = deepcopy(self.argsnode.origdata)
				self.argsnode.data.args = r.args
				self.argsnode.rerun = rerun
				
			# failed: need to explore/create new nodes/edges
			else:
				dbg.infom("!", "#R<mismatched#> syscalls: %s vs %s" \
						  % (self.argsnode.origdata, r))

				# do nothing
				assert self.argsnode.data == None

				# pick time stamp
				ts  = 0
				for a in rerun.actors:
					t = max(a.actions)
					if ts < t.tac:
						ts = t.tac

				rerun.detach(r, ts)
				
				# really do create
				osloader.parse_record(r)
				
		# rerunning but creating new nodes/edges
		elif rerun and rerun.is_state("detached"):
			# if detached node
			if rerun.wait_r == self.argsnode.origdata:
				rerun.resume()
				rerun.activate()
				
		# dummy
		if rerun and rerun.is_expanding():
			self.argsnode.data = self.argsnode.origdata
			self.argsnode.rerun = rerun
		
	def register(self):
		self.outputset.add(self.argsnode)

class ProcSysRet(mgrapi.Action):
	def __init__(self, actor, retnode):
		self.retnode = retnode
		super(ProcSysRet, self).__init__(retnode.name + ('pret',), actor)
		
	def redo(self):
		if self.retnode.data is None: return

		# start shepherded re-execution
		if self.retnode.data.name == "execve" and not self.actor.rerun:
			# create rerun process
			(arg, ret) = self.actor.get_execve()
			rerun = Rerun(arg, ret, self.uids) if hasattr(self, "uids") else Rerun(arg, ret)
			rerun.execve(self.actor.pid)

			# process actor has a reference to the rerun module
			self.actor.rerun = rerun
			rerun.add(self.actor)
			
			return

		if self.actor.rerun \
			    and self.actor.rerun.is_expanding():
			
			pid = self.retnode.data.pid
			(r, p) = self.actor.rerun.next(True, pid)
			if r == None:
				return
			assert r.usage == 0

			# pass rerun object to the bumped process
			if r.pid != self.retnode.data.pid:
				assert r.pid == pid
				actor = ProcessActor.get(r.pid, r)
				actor.rerun = self.actor.rerun
				actor.rerun.add(actor)

			# create pcall action
			osloader.parse_record(r)

	def equiv(self):
		if self.retnode.data is None: return False
		if self.actor.rerun and self.actor.rerun.is_active(): return False
		if self.retnode.origdata.args == self.retnode.data.args \
		       and self.retnode.origdata.ret == self.retnode.data.ret :
			return True
		dbg.warn('#Y<XXX#> not quite right')
		return False
	
	def register(self):
		self.inputset.add(self.retnode)

class ISyscallAction(mgrapi.Action):
	def get_tic(self):
		if not hasattr(self, '_tic'): self.actor.load('actions')
		return self._tic
	def get_tac(self):
		if not hasattr(self, '_tac'): self.actor.load('actions')
		# we don't know about tac, when we see the entry record
		if not hasattr(self, '_tac'): return self.get_tic()
		return self._tac
	def set_tac(self, tac): self._tac = tac
	def set_tic(self, tic): self._tic = tic
	
	tic = property(get_tic, set_tic)
	tac = property(get_tic, set_tac)

class CloneSyscallAction(ISyscallAction):
	def __init__(self, name):
		self.argsnode       = None
		self.parent_retnode = None
		self.child_retnode  = None
		
		actor = mgrutil.StatelessActor(name + ('sysactor',))
		super(CloneSyscallAction, self).__init__(name, actor)

	@staticmethod
	def get(r):
		name = ('pid', r.pid.pid, 'syscall', r.sid)
		n = mgrapi.RegisteredObject.by_name(name)
		if n is None:
			n = CloneSyscallAction(name)
		return n

	def redo(self):
		if self.argsnode.data is None: return
		
		if hasattr(self.argsnode, "rerun"):
			if not self.argsnode.rerun.is_active():
				return
			
			rerun = self.argsnode.rerun
			pid   = self.argsnode.data.pid

			child_pid = self.parent_retnode.origdata.ret
			(r, p) = rerun.clone(pid, child_pid)
			
			# r is None when exit_group() like syscalls
			assert not r or (r and r.usage == 1)
			if r:
				self.child_retnode.data  = deepcopy(self.child_retnode.origdata)
				self.parent_retnode.data = deepcopy(self.parent_retnode.origdata)
				
			# find child actor and setup for rerun
			r = self.parent_retnode.data
			child = ProcessActor.get(r.ret, r)
			child.rerun = rerun
			rerun.add(child)
			
			return
	
	def register(self):
		if self.argsnode:       self.inputset.add(self.argsnode)
		if self.parent_retnode: self.outputset.add(self.parent_retnode)
		if self.child_retnode:  self.outputset.add(self.child_retnode)

		dbg.connect("connect(): (%s,%s,%s)" \
				  % (self.argsnode, self.parent_retnode, self.child_retnode))
		
class SyscallAction(ISyscallAction):
	def __init__(self, name):
		self.argsnode = None
		self.retnode  = None
		actor = mgrutil.StatelessActor(name + ('sysactor',))
		super(SyscallAction, self).__init__(name, actor)
		
	@staticmethod
	def get(r):
		name = ('pid', r.pid.pid, 'syscall', r.sid)
		n = mgrapi.RegisteredObject.by_name(name)
		if n is None:
			n = SyscallAction(name)
		if hasattr(r, "uids"):
			setattr(n, "uids", r.uids)
		return n
	
	def equiv(self):
		if self.argsnode is None or self.retnode is None:
			return False
		if self.argsnode.data is None or self.retnode.data is None:
			return False

		if hasattr(self.argsnode, "rerun") \
			    and self.argsnode.rerun.is_active():
			return False
		
		r = self.argsnode.data
		
		# case 1: read system call
		if r.nr == syscall.NR_read:
			fd = r.args["fd"]
			
			dev = fd.inode.dev
			ino = fd.inode.ino
			off = fd.offset
			cnt = self.retnode.data.ret
			sha = self.retnode.data.args["buf"]

			if not fd.inode.prefix in ["file"]:
				return False

			src_pn = kutil.get(fd.inode)
			with open(src_pn, 'r') as f:
				f.seek(off)
				h = sha1(f.read(cnt))
				dbg.sha1m("!", "%s vs %s = %s" \
						  % (hexdigest(h),
						     hexdigest(sha),
						     h == sha))
				return h == sha
			return False
		
		if self.argsnode.origdata == self.argsnode.data:
			return True
		#
		# add any cases to stop the propagations
		#
		dbg.warn('#Y<XXX#> not quite right')
		return False
	
	def redo(self):
		if self.argsnode.data is None: return

		# share expanding & following
		if hasattr(self.argsnode, "rerun") \
			    and self.argsnode.rerun.is_active():
			
			rerun = self.argsnode.rerun
			pid   = self.argsnode.data.pid

			# wait syscalls: make it non-blocking
			if self.argsnode.data.name == "wait4":
				(r, p) = rerun.wait4(pid)
			elif self.argsnode.data.name == "exit_group":
				(r, p) = rerun.exit_group(pid)

				# while expanding (after canceling all nodes),
				# you should create new node (pcall) to dynamically
				# create new nodes of parents
				if rerun.is_expanding() and len(rerun.pids) >= 1:
					for childpid in rerun.pids:
						dbg.info("reviewing: %s" % childpid)
						(childr, childp) = rerun.next(True, childpid)
						assert childr.usage == 0
						osloader.parse_record(childr)
						
			else:
				(r, p) = rerun.next(False, pid)

			# r is None when exit_group() like syscalls
			assert not r or (r and r.usage == 1)
			if r:
				# when we are following
				if self.retnode != None:
					self.retnode.data = deepcopy(self.retnode.origdata)
					self.retnode.data.ret = r.ret
					
				# explorering new nodes/edges
				else:
					osloader.parse_record(r)
			return

		r = self.argsnode.data
		func = getattr(self, "syscall_%s" % r.name, None)
		if func:
			# exit_group() has no retnode
			if self.retnode:
				self.retnode.data = func(self.argsnode, self.retnode)
			return

		# raise Exception('not impl')
		dbg.error("Not implemented:", r.name)

	def __search_checkpoints(self, n, x):
		for p in glob(os.path.join(kutil.fsget(x.dev), ".inodes", str(x.ino)) + "*"):
			dbg.info("found inodes: %s" % p)
			info = p.split(".")
			tic = (int(info[-2]) * 10**9 + int(info[-1]),0)
			n.checkpoints.add(fsmgr.FileCheckpoint(tic, tic, x.dev, x.ino, p))
		
	def register(self):
		if self.retnode:  self.outputset.add(self.retnode)
		if self.argsnode: self.inputset.add(self.argsnode)

		# register() should work even with only one retnode/argsnode
		dbg.connect("%s => connect(): (%s,%s)" % (self, self.argsnode, self.retnode))

		# merge a bit for compatibility
		r = None
		if self.argsnode and self.argsnode.data:
			r = self.argsnode.data
		else:
			if self.retnode and self.retnode.data:
				r = self.retnode.data
		if r and self.retnode and self.retnode.data:
			setattr(r, "ret", self.retnode.data.ret)

		if r is None:
			return
			
		(rs, ws) = nrdep.nrdep(r)
		for x in rs | ws:
			objs = []
			if type(x) == sysarg._inode:
				if x.prefix in ('file', 'link', 'dir'):
					n = fsmgr.FileDataNode.get(x.dev, x.ino)
					# add checkpoints of unlink syscall
					if r.name in ["unlinkat", "unlink", "rmdir"]:
						# find inodes in the .inodes
						self.__search_checkpoints(n, x)
					objs.append(n)
				elif x.prefix == 'socket':
					n = sockmgr.SocketNode.get(x)
					objs.append(n)
					if n.sport and n.dip and n.dport:
						objs.append(networkmgr.NetworkNode.get(x))
				# elif x.prefix in ['pts', 'ptm']:
				# 	objs.append(ptymgr.PtyNode.get(os.major(x.rdev), 0))
				# elif x.prefix == 'fifo':
				# 	objs.append(fifomgr.FifoNode.get(x))
				# elif x.prefix == 'cdev':
				# 	objs.append(cdevmgr.CdevNode.get(x))
				else:
					dbg.error('need data node for inode type', x.prefix)
			elif type(x) == sysarg.process:
				dbg.info('pid:%s -> pid:%s' % (r.pid, x))
			elif type(x) == sysarg.dentry:
				objs.append(fsmgr.DirentDataNode.get(x.inode.dev, x.inode.ino, x.name))
			else:
				dbg.error('need data node for', x, 'type', type(x))
				
			if len(objs) != 0:
				for n in objs:
					if x in rs: self.inputset.add(n)
					if x in ws: self.outputset.add(n)

	def syscall_write(self, arg, ret):
		r = arg.data
		assert r.nr == syscall.NR_write

		dbg.syscallm("!", "#R<write#>: %d with %s" % (r.args["fd"].inode.ino, str(r)))
		if runopts.dryrun:
			return ret.origdata

		fd = r.args["fd"]

		# XXX pts
		if not fd.inode.prefix in ["file"]:
			print "-" * 60
			print r.args["buf"]
			print "-" * 60
			return ret.origdata

		# regular write on files
		cur_pn = kutil.get(fd.inode)
		offset = fd.offset

		f = open(cur_pn, "r+")
		# append
		if offset == -1:
			f.seek(0, os.SEEK_END)
		else:
			f.seek(offset)
		f.write(r.args["buf"])
		f.close()

		# XXX
		# dynamically generated snapshots for fast lookup
		# dev = fd.inode.dev
		# ino = fd.inode.ino
		# tic = ret.origdata.ts
		# cp = fsmgr.FileContentCheckpoint(tic, tic, dev, ino)
		# fsmgr.FileDataNode.get(dev, ino).checkpoints.add(cp)
		
		# XXX update ret node
		return ret.origdata

	def syscall_open(self, arg, ret):
		r = arg.data
		assert r.nr == syscall.NR_open
		assert r.args["path"].inode is None or r.args["path"].inode == r.ret.inode

		inode = arg.data.ret.inode
		dbg.syscallm("!", "#R<open#>: %s" % (str(r)))
		if runopts.dryrun:
			return ret.origdata

		# given pathname
		pn = r.args['path'].path

		# if not absolute pathname, retrieve with iget
		if not pn.startswith('/'):
			dentry = r.args['path'].entries[0]
			assert type(dentry) == sysarg.dentry
			pn = os.path.join("%s/%s" % (kutil.get(dentry.parent), dentry.name))

		# flag == os.O_CREAT and only the case we really created an empty file
		if (r.args["path"].inode is None):
			src_pn = os.path.join(kutil.fsget(inode.dev), ".inodes", str(inode.ino))
			# to keep the inode
			os.rename(src_pn, pn)
			# make it empty when newly created
			with open(pn, "w") as f:
				pass
			dbg.syscallm("!", "#R<rename#>: %s -> %s" % (src_pn, pn))
			
		# if explicit truncate request
		if (r.args['flags'] & os.O_TRUNC):
			with open(pn, "w") as f:
				pass

		# XXX update ret node
		return ret.origdata

	def syscall_read(self, arg, ret):
		r = arg.data
		assert r.nr == syscall.NR_read

		fd = arg.data.args["fd"]
		inode = fd.inode
		dbg.syscallm("!", "#R<read#>: reading %s with %s" % (inode, r))

		# read & create mock object
		pn = kutil.get(inode)
		with open(pn, 'r') as f:
			f.seek(fd.offset)
			file_data = f.read(r.args["count"])

		sha = sha1(file_data)

		dbg.syscallm("!", "#R<read#>: sha1:%s (old:%s)" \
						% (hexdigest(sha), hexdigest(ret.origdata.args["buf"])))

		new_data = deepcopy(ret.origdata)
		new_data.args["buf"] = sha
		new_data.args["real_buf"] = file_data
		new_data.ret = len(file_data)

		return new_data

	# dummy exit_group()
	def syscall_exit_group(self, arg, ret):
		pass
