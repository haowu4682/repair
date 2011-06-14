#!/usr/bin/python

#
# test/rerun0: simplest rerun testing (need to re-execute ./append.sh)
# test/rerun1: ./append.sh needs to invoke the new child, /bin/cat, which
#              isn't in the graph
# test/rerun2: after re-executing ./append.sh, we need to repair ./last.sh
#              without re-executing it - to show logical timestamp works
#

import os
import re
import sys
import dbg
import select
import errno
import os.path as path
import retroctl
import collections

from Queue    import Queue
from tempfile import SpooledTemporaryFile
from record   import SyscallQueue, PidDict, KprobesQueue, zopen, fix_r
from syscall  import *

from subprocess import MAXFD

from ptrace.binding        import ptrace_traceme
from ptrace.binding.func   import PTRACE_O_TRACECLONE, PTRACE_O_TRACEFORK
from ptrace.debugger       import PtraceDebugger, Application, ProcessExit
from ptrace.debugger       import ProcessSignal, NewProcessEvent, ProcessExecution
from ptrace.debugger.child import _createParent, _set_cloexec_flag, _execChild
from ptrace.func_call      import FunctionCallOptions
from ptrace.error          import PtraceError

def _parse_syscall(process):
	syscall_options = FunctionCallOptions(
		write_types        = False,
		write_argname      = True,
		string_max_length  = 300,
		replace_socketcall = False,
		write_address      = True,
		max_array_count    = 20
		)
	
	syscall_options.instr_pointer = True
	
	syscall = process.syscall_state.event(syscall_options)
	
	return syscall.format()

def _createChild(arguments, no_stdout, env, errpipe_write):
	# Child code
	try:
		ptrace_traceme()
	except PtraceError, err:
		raise Exception(str(err))

	# Close all files except 0, 1, 2 and errpipe_write
	# for fd in xrange(3, MAXFD):
	# 	if fd == errpipe_write:
	# 		continue
	# 	try:
	# 		os.close(fd)
	# 	except OSError:
	# 		pass
	_execChild(arguments, no_stdout, env)
	exit(255)

class Ptrace(Application):
	def __init__(self, args=[], envs=[], cwd=None, root=None, uids=collections.defaultdict(lambda:0)):

		class dummyopt(object):
			pass

		self.options            = dummyopt()
		self.options.fork       = False
		self.options.trace_exec = True
		self.options.pid        = None
		self.options.no_stdout	= False

		self.cwd      = cwd
		self.root     = root
		self.args     = args
		self.procs    = {}
		self.signals  = collections.defaultdict(lambda:None)
		self.debugger = None
		self.pidd     = PidDict()
		self.psyscalls= {}
		self.signum   = 0
		self.uids     = uids

		# kq
		files = os.listdir(retroctl._get_appdir("retro"))
		kprobesN = [os.path.join(retroctl._get_appdir("retro"), x) \
				    for x in files if re.match("kprobes[0-9]+", x)]
		self.kq = KprobesQueue([zopen(fn) for fn in kprobesN])
		
		# convert list of env to dict
		self.envs = {}
		if type(envs) is list:
			for e in envs:
				e = e.split("=")
				self.envs[e[0]] = e[1]
		else:
			assert type(envs) is dict
			self.envs = envs
		
		self.__initialized = False

	# copied from ptrace/debugger/child.py to change cwd of child process
	def createChild(self, arguments, no_stdout, env=None):
		errpipe_read, errpipe_write = os.pipe()
		_set_cloexec_flag(errpipe_write)

		# Fork process
		pid = os.fork()
		if pid:
			os.close(errpipe_write)
			_createParent(pid, errpipe_read)
			return pid
		else:
			os.close(errpipe_read)
			if self.cwd:
				os.chdir(self.cwd)
				dbg.info("cwd: %s" % self.cwd)
			if self.root:
				os.chroot(self.root)
				dbg.info("root: %s (pwd:%s)" % (self.root, os.getcwd()))
			try:
				dbg.info("setting uids: " + str(self.uids))
				os.setregid(self.uids["gid"], self.uids["egid"])
				os.setreuid(self.uids["uid"], self.uids["euid"])
			except OSError:
				raise Exception("Failed to set uids for: %s" % (arguments[0]))
			_createChild(arguments, no_stdout, env, errpipe_write)
		
	def createProcess(self):
		#
		# XXX adjust fd (ptrace/debugger/child.py)
		#
		pid = self.createChild(self.args, self.options.no_stdout, self.envs)

		dbg.info("create: pid:%d -> process %s (%s)" % (os.getpid(), pid, self.args))
		
		try:
			return self.debugger.addProcess(pid, is_attached=True)
		except (ProcessExit, PtraceError), err:
			if isinstance(err, PtraceError) and err.errno == EPERM:
				raise Exception("Not enough permission %s" % pid)
			else:
				raise Exception("Process cannot be attached %s" % err)
		return None

	def run(self):
		retroctl.disable()
		self.q = ISyscallQueue()

		self.debugger = PtraceDebugger()
		self.setupDebugger()
		self.debugger.options |= PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK

		retroctl.enable(os.getpid())

		p = self.createProcess()
		if not p:
			return False
		
		self.procs[p.pid] = p

		# ready? poping upto execve
		execve = []
		while True:
			r = self.q.pop()
			if r is None:
				break
			dbg.ptrace( "skipping: %s" % r)
			if r.name == "execve":
				execve.append(r)

		# imitating next instruction is execve
		for r in execve:
			self.q.push(r)

		return True

	def is_active(self):
		return len(self.procs) > 0
	
	def stop(self):
		self.procs = {}
		self.debugger.quit()
		retroctl.disable()
		
	def next(self, entering, pid=None):
		if not self.__initialized:
			if not self.run():
				raise Exception("Failed to execute:%s" % self.args)
			self.__initialized = True
		while True:
			# ignore syscalls that we are not tracing
			(r, p) = self._next(entering, pid)
			# flipping to propagate
			entering = not entering
			if p is None:
				return (r, p)
			if r is None:
				continue
			return (r, p)

	# fix record like what a loader is doing
	def load(self, entering, pid=None):
		while True:
			# ignore syscalls that we are not tracing
			(r, p) = self.next(entering, pid)
			if r is None:
				if p is None:
					return (r, p)
				continue
			
			# fix pid/inode gen
			self.pidd.fix(r)
			self.kq.fix(r)
			
			if r.pid not in self.psyscalls:
				self.psyscalls[r.pid] = []
			if r.usage == 0:
				# an enter record
				if r.nr not in [NR_exit, NR_exit_group]:
					self.psyscalls[r.pid].append(r)
			else:
				# an exit record
				if r.name == "execve":
					r.pid.gen -= 1
					
				top = self.psyscalls[r.pid].pop()
				
				assert r.nr == top.nr
				# fix sid
				r.sid = top.sid
				# merge args
				for a in syscalls[r.nr].args:
					if a.name in r.args:
						if a.name in top.args:
							dbg.info("check %s is already assigned: %s <- %s" \
									 % (a.name, r.args[a.name], top.args[a.name]))
					else:
						r.args[a.name] = top.args[a.name]

			fix_r(r)
			
			return (r, p)

	def _next(self, entering, pid=None):
		# if not initiated or terminated
		if not self.is_active():
			return None, None

		# pick a process, child first
		if pid is None:
			pid = self.procs.keys()[-1]

		if pid not in self.procs:
			return (None, None)
		
		process = self.procs[pid]

		# check queue because python-ptrace did sth dirty
		r = self.q.pop()
		if r:
			return (r, process)

		# dbg.ptrace("%s" % ">" if entering else "<")

		# change ptrace state
		if entering:
			retroctl.enable_record_enter()
		else:
			retroctl.enable_record_exit()

		# 
		# run two times
		#  1. record entry and not execute syscall
		#  2. execute syscall and record exit
		#

		signum = self.signals[pid]
		del self.signals[pid]
		
		for i in range(2):
			try:
				if signum:
					process.syscall(signum)
				else:
					process.syscall()
			except PtraceError, e:
				# no such a process, done
				if e.errno == errno.ESRCH:
					return (None, None)
				raise

			# wait until next syscall enter
			try:
				event = self.debugger.waitSyscall(process)

				self.procs[pid] = process = event.process

				# dbg.ptrace(process.getregs().rip,
				# 	   process.getregs().orig_rax)

				if entering:
					regs     = process.getregs()
					orig_rax = process.getregs().orig_rax

			except ProcessExit, event:
				p = event.process
				dbg.info("exit process:", p.pid)
				del self.procs[p.pid]
				dbg.info("processes:", self.procs)
				return (None, None)
			except ProcessSignal, event:
				dbg.info("signal:%s" % (event.signum))
				# dbg.stop()
				self.signals[pid] = event.signum
				process.syscall(event.signum)
				self.debugger.waitSyscall()
			except NewProcessEvent, event:
				# clone(): trace child now
				child = event.process
				dbg.info("new process: %s" % child.pid)
				# let parent finish up the clone()
				child.parent.syscall()
				self.debugger.waitSyscall()
				# now let child execute
				# process = child
				self.procs[child.pid] = child
				assert entering == False and i == 1
			except ProcessExecution, event:
				assert entering == False
				process = event.process

				# let it finish (execve())
				process.syscall()
				self.debugger.waitSyscall()
			except:
				raise

		if entering:
			# fake like calling first time
			process.setregs(regs)

			# skip syscall instruction
			rip = process.getreg("rip")
			process.setreg("rip", rip - 2)

			# retore system call number
			process.setreg("rax", orig_rax)

		# (record, process)
		return (self.q.pop(), process)

class Spool():
	def __init__(self, name):
		self.spool = SpooledTemporaryFile(max_size=1024*1024, mode="w+")
		self.name  = name
		self.cur   = 0

	def write(self, blob):
		# append it
		self.spool.seek(0, os.SEEK_END)
		return self.spool.write(blob)

	def read(self, size):
		# move to the next pos
		self.spool.seek(self.cur, os.SEEK_SET)
		blob = self.spool.read(size)
		self.cur += len(blob)
		return blob

class ISyscallQueue(SyscallQueue):
	def __init__(self):
		self.last = None
		self.pri = Queue()
		self.fds = []

		src = retroctl._get_appdir("retro")
		for fn in [x for x in os.listdir(src)
			   if re.match("syscall[0-9]+", x)]:
			self.fds.append([open(path.join(src, fn), 'r'), Spool(fn)])
			
		self.reset()
		
	def reset(self):
		super(ISyscallQueue, self).__init__([s for (r,s) in self.fds])

	def push(self, r):
		self.pri.put(r)

	def pop(self):
		#
		# update from src to dst
		#
		
		if not self.pri.empty():
			return self.pri.get()

		if self.last is None:
			self.reset()
			
		more = True
		while more:
			more = False
			for (fd, spool) in self.fds:
				blob = fd.read(4096)
				if len(blob) == 0:
					continue

				spool.write(blob)
				more = True

		self.last = super(ISyscallQueue, self).pop()

		return self.last

#
# usage like below
#
if __name__ == "__main__":

	# ptrace = Ptrace(["/bin/ls"])
	ptrace = Ptrace(["/bin/bash", "-c", "./test.sh"])
	entering = True
	while True:
		(r,p) = ptrace.next(entering)
		if r:
			# print p.pid, p.getregs().rip, _parse_syscall(p)
			# print _parse_syscall(p)
			dbg.ptrace(r, p)
			dbg.ptrace("!")
		else:
			break
		
		entering = not entering
 
	ptrace.stop()
