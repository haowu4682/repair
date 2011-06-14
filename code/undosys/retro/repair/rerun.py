#!/usr/bin/python

import os
import re
import sys
import dbg
import select
import errno
import os.path as path
import retroctl
import collections
import atexit

import code

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
  """Get a string representation of a syscall."""
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
  "Register self for tracing and exec child."""
  try:
    ptrace_traceme()
  except PtraceError, err:
    raise Exception(str(err))

  # Close all files except 0, 1, 2 and errpipe_write
  # for fd in xrange(3, MAXFD):
  #   if fd == errpipe_write:
  #     continue
  #   try:
  #     os.close(fd)
  #   except OSError:
  #     pass
  _execChild(arguments, no_stdout, env)
  exit(255)

class Ptrace(Application):
  """ipopov (conjecture): This class is used to trace a process
  *during* re-execution. Retroctl is activated (i.e. Retro is turned
  on) for the pid in question, and the process is allowed to run until
  it traps (executes a sysenter instruction)."""
  def __init__(self, args=[], envs=[], cwd=None,
               root=None, uids=collections.defaultdict(lambda:0)):

    class dummyopt(object):
      pass

    self.options            = dummyopt()
    self.options.fork       = False
    self.options.trace_exec = True
    self.options.pid        = None
    self.options.no_stdout  = False

    self.cwd      = cwd
    self.root     = root
    self.args     = args
    self.procs    = {}  # Processes under the control of this Ptrace instance
    self.signals  = collections.defaultdict(lambda:None)
    self.debugger = None
    self.pidd     = PidDict()
    self.psyscalls= {}
    self.signum   = 0
    self.uids     = uids
    self.execve_records = collections.defaultdict(lambda:Queue())

    # kq
    files = os.listdir(get_appdir("retro"))
    kprobesN = [os.path.join(get_appdir("retro"), x)
                for x in files if re.match("kprobes[0-9]+", x)]
    self.kq = KprobesQueue([zopen(fn) for fn in kprobesN])

    # convert list of env to dict
    self.envs = {}
    if type(envs) is list:
      for e in envs:
        e = e.split("=", 1)
        self.envs[e[0]] = e[1]
    else:
      assert type(envs) is dict
      self.envs = envs

    self.__initialized = False

  # copied from ptrace/debugger/child.py to change cwd of child process
  def createChild(self, arguments, no_stdout, env=None):
    """
    Create a child process:
     - arguments: list of string where (eg. ['ls', '-la'])
     - no_stdout: if True, use null device for stdout/stderr
     - env: environment variables dictionary

    Use:
     - env={} to start with an empty environment
     - env=None (default) to copy the environment
    """
    errpipe_read, errpipe_write = os.pipe()
    _set_cloexec_flag(errpipe_write)

    # Fork process
    pid = os.fork()
    if pid:  # parent
      os.close(errpipe_write)
      _createParent(pid, errpipe_read)
      return pid
    else:  # child
      os.close(errpipe_read)
      #### begin addition to ptrace/debugger/child.py
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
      #### end addition to ptrace/debugger/child.py
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

  def initialize(self):
    retroctl.disable()
    self.q = ISyscallQueue()

    self.debugger = PtraceDebugger()
    self.setupDebugger()
    self.debugger.options |= PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK

    def quitDebugger():
      self.debugger.quit()
    atexit.register(quitDebugger)

    retroctl.enable(os.getpid())

    p = self.createProcess()
    if not p:
      return False

    self.procs[p.pid] = p

    # Handle the execve that gets executed before tracing begins. python-ptrace
    # execs the child before attaching to it in earnest
    execve = []
    r = self.q.pop()
    while r:
      dbg.ptrace( "skipping: %s" % r)
      if r.name == "execve": execve.append(r)
      r = self.q.pop()

    assert len(execve) == 2
    for r in execve:
      self.execve_records[p.pid].put(r)

    return True

  def is_active(self):
    return len(self.procs) > 0

  def stop(self):
    self.procs = {}
    self.debugger.quit()
    retroctl.disable()

  def release_all(self):
    self.procs = {}
    for process in self.debugger:
      process.detach()
    self.stop()

  @dbg.util.trace
  def next(self, entering, pid=None):
    """Returns (None, None) if it fails to trace any syscalls.
    Returns (record, process) otherwise.
    """
    #code.interact(local=locals())
    if not self.__initialized:
      if not self.initialize():
        raise Exception("Failed to execute:%s" % self.args)
      self.__initialized = True

    execve_pid = pid
    if not execve_pid:
      pids = [k for k in self.execve_records.keys()
              if not self.execve_records[k].empty()]
      if pids:
        execve_pid = max(pids)  # I'm not sure why max()
                         # TODO: figure this out

    while (execve_pid and
           not self.execve_records[execve_pid].empty()):
      r = self.execve_records[execve_pid].get()
      if ((entering and r.usage == 0) or
          (not entering and r.usage == 1)):
        return (r, self.procs[execve_pid])
      else:
        dbg.warn('dropping a record: %s' % str((r, p)))
        continue

    (r, p) = self._next(entering, pid)
    while p and not r:
      if entering:
        # Execute a syscall and discard its exit record, so
        # we can get to the next one. In effect, this skips
        # over syscalls that Retro is not configured to trace.
        self._next(not entering, pid)
      (r, p) = self._next(entering, pid)
    if (r, p) == (None, None):
      return (None, None)

    if pid: assert p.pid == pid

    assert r
    if entering:
      assert r.usage == 0
    else:
      assert r.usage == 1

    return (r, p)

  @dbg.util.trace
  def load(self, entering, pid=None):
    """Return the (record, process) for the next syscall we're
    interested in tracing."""

    (r, p) = self.next(entering, pid)
    if (r, p) == (None, None):
      return (None, None)

    self.pidd.fix(r)
    self.kq.fix(r)

    self.psyscalls.setdefault(r.pid, [])
    if r.usage == 0:
      # an enter record
      if r.nr not in [NR_exit, NR_exit_group]:
        pass
        #self.psyscalls[r.pid].append(r)
    else:
      # an exit record
      if r.name == "execve":
        r.pid.gen -= 1

      #top = self.psyscalls[r.pid].pop()

      #assert r.nr == top.nr
      # fix sid
      #r.sid = top.sid

    fix_r(r)
    return (r, p)

  @dbg.util.trace
  def _next(self, entering, pid=None):
    """Execute a syscall and return its corresponding records if they were
    captured by Retro.

    Returns (entry_record, process) or (exit_record, process).

    Returns (None, None) when something exceptional happens... like
    the process being traced quitting.

    Returns (None, process) when the process is continuing but the
    system call just executed is not one of those that Retro is set up to trace
    (e.g. getpid).

    This just executes the process under ptrace, expecting the side effect of
    stuff showing up inside the ISysCallQueue. Then it takes stuff out of there
    and returns it.
    """
    if not self.is_active():
      return (None, None)

    assert not self.q.pop()

    # pick a process, child first
    if pid is None:
      pid = self.procs.keys()[-1]
    if pid not in self.procs:
      dbg.error('Ptrace.next called with pid %s; not found in'
                'self.procs %s' % (str(pid), str(self.procs)))
      return (None, None)

    process = self.procs[pid]

    if entering:
      retroctl.enable_record_enter()
    else:
      retroctl.enable_record_exit()

    try:
      self.syscall_step(process, entering)
      self.syscall_step(process, entering)
      #orig_regs = process.getregs()
    except (ProcessExit, PtraceError):
      return (None, None)

    # Set up syscall to be reexecuted if we only captured its entry
    # record on this invocation.
    if entering:
      #process.setregs(orig_regs)
      process.setreg('rax', process.getregs().orig_rax)
      # x86 SYSCALL/SYSENTER are each two bytes long
      process.setreg('rip', process.getreg('rip') - 2)

    return (self.q.pop(), process)

  def syscall_step(self, process, entering):
    """entering is purely for debugging purposes."""
    event = None
    while not event:
      signum = self.signals[process.pid]
      del self.signals[process.pid]
      try:
        if signum:
          process.syscall(signum)
        else:
          process.syscall()
      except PtraceError, e:
        # no such a process, done
        #if e.errno == errno.ESRCH:
          #return (None, None)
        raise

      try:
        event = self.debugger.waitSyscall(process)
        assert event.process is process
        dbg.info('syscall nr %s' % event.process.getregs().orig_rax)
      except ProcessExit as e:
        p = e.process
        del self.procs[p.pid]
        dbg.info("exit process:", p.pid)
        dbg.info("processes:", self.procs)
        raise
      except ProcessSignal as e:
        dbg.info("signal:%s" % (e.signum))
        assert e.process is process
        self.signals[process.pid] = e.signum
      except NewProcessEvent as e:
        # clone(): trace child now
        child = e.process
        # the next two are lines are vestigial, from taesoo....
        #child.parent.syscall()
        #self.debugger.waitSyscall()
        dbg.info("new process: %s" % child.pid)
        self.procs[child.pid] = child
        #self.syscall_step(process)
      except ProcessExecution as e:
        assert not entering
        assert e.process is process
        # let it finish up
        #self.syscall_step(process, entering)
        # ...
        # Evidently, this is not necessary, and indeed it messes things up. It
        # appears that ProcessExecution is delivered as an exception even
        # though it is the natural consequence of completing the execve
        # syscall.
      except:
        raise

  def kill(self, pid):
    """Terminate the process with pid pid.

    E.g. use in order to halt execution of a ptraced process whose initial
    execve gets canceled. Otherwise the process will never exit, and wait4's on
    that pid will always block."""
    proc = self.procs[pid]
    # This will be painful.
    rip = proc.getreg('rip')
    proc.setreg('rax', NR_exit_group)
    proc.setreg('rbx', 0)
    proc.writeBytes(rip, "\x0f\x05") # x86 SYSCALL

    sig = None
    retroctl.disable()
    while True:
      print 'blah'
      try:
        if sig:
          proc.syscall(sig)
          sig = None
        else:
          proc.syscall()
        proc.waitSyscall()
      except ProcessSignal as e:
        sig = e.signum
        print 'signal', sig
      except ProcessExit:
        print 'exit'
        break
      except:
        raise
    retroctl.enable(os.getpid())


class Spool():
  def __init__(self, name):
    self.spool = SpooledTemporaryFile(max_size=1024*1024, mode="w+")
    self.name  = name
    self.cur   = 0

  def write(self, blob):
    """Append a blob."""
    self.spool.seek(0, os.SEEK_END)
    return self.spool.write(blob)

  def read(self, size):
    """Read the next blob."""
    self.spool.seek(self.cur, os.SEEK_SET)
    blob = self.spool.read(size)
    self.cur += len(blob)
    return blob

class ISyscallQueue(SyscallQueue):
  """Interactive??? SyscallQueue?

  Like a SyscallQueue, but on a temporary spooled file, with a backing
  file, presumably being populated in realtime by Retro.

  You can push records onto it, but retrieval semantics are as for a
  queue, not a stack. When you try to pop a record, if nothing that
  has been pushed remains, the backing files are read into the
  temporary spools, and records are read out of them.
  """

  def __init__(self):
    self.last = None
    self.pri = Queue()
    self.fds = []

    src = get_appdir("retro")
    for fn in [x for x in os.listdir(src) if re.match("syscall[0-9]+", x)]:
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

def get_appdir(suffix):
  # XXXXXXXXXX hack
  return ('/sys/kernel/debug/' + suffix)

#
# usage like below
#
if __name__ == "__main__":
  #hack
  PathidTable.initialize('/tmp/retro')
  ptrace = Ptrace(["/bin/ls"])
  # ptrace = Ptrace(["/bin/bash", "-c", "./test.sh"])
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
