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
#import osloader
import networkmgr
import fifomgr
import cdevmgr

from sha1 import sha1, hexdigest

import code

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

    self.expanding = False # ipopov: ???? What does this variable mean?
                           #         It probably means that this
                           #         object is creating new nodes

  def add(self, actor):
    self.actors.add(actor)

  def is_state(self, s):
    return self.state == s

  def detach(self, r, ts):
    dbg.rerun('expanding... rerun instance %s detaching' % self)
    self.state  = "detached"
    self.wait_r = r
    self.ts     = (ts[0] + 1,) + ts[1:]
    r.ts        = self.ts + (self.seq,)
    self.seq   += 1

  def resume(self):
    dbg.rerun('expanding... rerun instance %s resuming' % self)
    #self.state = "resumed"
    self.wait_r = None
    self.expanding = True
    self.state = "done"
    self.release_all()

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

    # Virtualize child pid. It's possible that this clone syscall takes place
    # during expansion, in which case the child pid cannot be matched up to a
    # vpid from the original execution. In that case, let vpid=pid.
    if not child_pid:
      child_pid = r.ret
    self.pids[child_pid] = r.ret
    dbg.rerun("vpid:%s -> pid:%s (active)" % (child_pid, r.ret))

    self.__fix_pid(r, vpid)
    self.__fix_ts(r, False)
    self.set_last(r, p)
    return (r, p)

  @dbg.util.trace
  def wait4(self, vpid):
    """Do not use this function unless the relevant process is halfway through
    the syscall (i.e. unless process.syscall() has been called exactly once.
    """
    pid  = self.pids[vpid]
    proc = self.proc.procs[pid.pid]

    assert proc.getreg("rax") == syscall.NR_wait4

    orig_nohang = proc.getreg("rdx") & os.WNOHANG

    orig_rdx  = proc.getreg("rdx")
    proc.setreg("rdx", orig_rdx | os.WNOHANG)

    # fire
    (r, p) = self.proc.load(False, pid.pid)
    self.__fix_pid(r, vpid)
    self.__fix_ts(r, False)

    if r.ret == 0 and not orig_nohang:
      # A child exists, but it has not changed state yet.  Modify rip (+regs)
      # to execute wait4() next time.
      # TODO: pull this code into rerun.py
      proc.setreg("rax", proc.getreg("orig_rax"))
      proc.setreg("rip", proc.getreg("rip") - 2)
      dbg.rerun('messing with rerun registers outside of rerun.py')
      return None

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
    dbg.rerun('proc.load returned %s' % str((r, p)))
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

  def kill(self, vpid):
    pid = self.pids[vpid]
    dbg.rerun('killing rerun process with vpid %s pid %s' % (pid, vpid))
    self.proc.kill(pid.pid)
    del self.last[vpid]
    del self.pids[vpid]

  def release_all(self):
    """To be called from self.resume(). By this point, we are only covering
    "expanded" nodes, so we need do no tracking via Retro.
    """
    self.proc.release_all()


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
      if (a.args[k].inode and
          a.args[k].inode.prefix in ["pts", "socket"]):
        continue
      if a.args[k].inode != r.args[k].inode:
        dbg.trace("1)", a.args[k])
        dbg.trace("2)", r.args[k])
        return False
      continue

    # check identical
    if a.args[k] != r.args[k]:
      if r.nr == syscall.NR_open:
        # TODO(ipopov). Check this. It seems that the argument to open() should
        # not include the inode opened. Surely that belongs in the return value
        # of the syscall. Handling this here is a hack, and it needs to be
        # moved into a more general place.
        if (a.args[k].path == r.args[k].path and
            a.args[k].root_in == r.args[k].root_in and
            a.args[k].root_pn == r.args[k].root_pn and
            a.args[k].pathid == a.args[k].pathid):
           dbg.rerun('short circuiting. returning True. would have returned %s.' % (str(a.args[k]) == str(r.args[k])))
           continue
      # slow path: because some obj could be deep-copied, str -> repr
      if str(a.args[k]) == str(r.args[k]):
        continue
      # but sometimes, this str representation gets messed up for whatever
      # reason, so handle this special case here:
      # (needless to say, TODO: get some more sane argument checking here)
      if isinstance(a.args[k], list):
        if str(set(a.args[k])) == str(set(r.args[k])):
          dbg.trace('hi ivo!')
          continue
      dbg.trace("1)", a.args[k])
      dbg.trace("2)", r.args[k])
      return False
  return True


class ProcSysCall(mgrapi.Action):
  """An syscall action belonging to a ProcessActor."""
  def __init__(self, actor, argsnode, osloader):
    """argsnode, counterintuitively, does not represent the arguments
    _to_ this node, but rather the argument node that this node
    populates.
    """
    self.argsnode = argsnode
    super(ProcSysCall, self).__init__(argsnode.name + ('pcall',),
                                      actor, osloader)

  def redo(self):
    """Execute another action of this action's actor. If the greedy
    heuristic matches the current action to the action just executed,
    we're fine. Otherwise, call parse_record() to create a new node in
    the action history graph.
    """

    dbg.syscall("redoing")
    rerun = self.actor.rerun

    if not rerun:
      dbg.syscall("no rerun instance bound to ProcSyscall %s;"
                  "returning prematurely" % self)
      return

    # following history while rerunning
    if rerun.is_following():
      (r, p) = rerun.next(True, self.actor.pid)
      if r == None:
        dbg.syscall('Aborting syscall; rerun returned no record')
        return
      assert r.usage == 0

      # special treatment of wait4
      # (see note in wait4.doc)
      # The following if statement seems pretty impenetrable. What I think it
      # does is the following. If a process under rerun tries to execute a
      # wait4, but that doesn't match up with the expected syscall from the
      # previous execution, call code.interact() and handle it manually.
      if (r.nr == syscall.NR_wait4 and
          self.argsnode.origdata.nr != syscall.NR_wait4):
        dbg.rerun('entering interactive')
        code.interact(local=locals())

        # wait4() again, at this time we should be able
        # to fetch child exit status
        (r, p) = rerun.next(False, self.actor.pid)
        assert r.usage == 1 and r.nr == syscall.NR_wait4

        # this is a real system call we should execute
        # on this action node
        (r, p) = rerun.next(True, self.actor.pid)
        assert r.usage == 0

      # match up
      if (self.argsnode.origdata.nr == r.nr and
          check_args(r, self.argsnode.origdata)):
        # update arguments
        self.argsnode.data = deepcopy(self.argsnode.origdata)
        self.argsnode.data.args = r.args
        self.argsnode.rerun = rerun

      # failed: need to explore/create new nodes/edges
      else:
        dbg.infom("!", "#R<mismatched#> syscalls: %s vs %s" %
                         (self.argsnode.origdata, r))
        assert self.argsnode.data == None
        ts  = 0
        for a in rerun.actors:
          t = max(a.actions)
          if ts < t.tac:
            ts = t.tac

        rerun.detach(r, ts)

        # Actually create a new node to correspond to what was just
        # executed.
        self.osloader.parse_record(r)

    # rerunning but creating new nodes/edges
    elif rerun.is_state("detached"):
      # if detached node
      if rerun.wait_r == self.argsnode.origdata:
        rerun.resume()
        #rerun.activate()

    # ipopov: ????
    if rerun.is_expanding():
      self.argsnode.data = self.argsnode.origdata
      self.argsnode.rerun = rerun

  def register(self):
    self.outputset.add(self.argsnode)

class ProcSysRet(mgrapi.Action):
  """The action belonging to a ProcessActor that corresponds to sysret."""
  def __init__(self, actor, retnode, osloader):
    self.retnode = retnode
    super(ProcSysRet, self).__init__(retnode.name + ('pret',), actor,
                                     osloader)

  def redo(self):
    dbg.syscall("redoing ProcSysRet")

    if not self.actor.rerun and getattr(self.retnode, 'rerun', None):
      self.actor.rerun = self.retnode.rerun

    if self.retnode.data is None:
      # This probably means that the retnode has been rolled back to
      # its (-Inf, -Inf, None) checkpoint and has not been updated
      # (rolled forward), probably because the syscall that was to
      # fill it never got executed.
      dbg.syscall("no argsnode data for %s, returning prematurely" % self)
      return

    # start shepherded re-execution
    if self.retnode.data.name == "execve" and not self.actor.rerun:
      dbg.rerun('actor %s (%s) has no rerun object' % (self.actor, id(self.actor)))
      # create rerun process
      (arg, ret) = self.actor.get_execve()
      if hasattr(self, "uids"):
        rerun = Rerun(arg, ret, self.uids)
      else:
        rerun = Rerun(arg, ret)
      rerun.execve(self.actor.pid)

      # process actor has a reference to the rerun module
      self.actor.rerun = rerun
      rerun.add(self.actor)
      return

    if self.actor.rerun and self.actor.rerun.is_expanding():
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
      self.osloader.parse_record(r)
      return
    # It's not undesirable that we fall through to here. ProcSysRets should
    # only be reexecuted if there is some information to be handed back to the
    # process (which there never should be, because in such a circumstance, the
    # rerun module would already be active, and receiving "sysret nodes"
    # directly from the kernel, as it were).
    dbg.syscall('ignoring Procsysret redo request')

  def equiv(self):
    # TODO
    # This function should consider only those parts of an argsnode
    # that are visible to the calling process.
    if self.retnode.data is None:
      return False
    if self.actor.rerun and self.actor.rerun.is_active():
      return False
    if (self.retnode.origdata.args == self.retnode.data.args and
        self.retnode.origdata.ret == self.retnode.data.ret):
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
  def __init__(self, name, osloader):
    self.argsnode       = None
    self.parent_retnode = None
    self.child_retnode  = None

    actor = mgrutil.StatelessActor(name + ('sysactor',))
    super(CloneSyscallAction, self).__init__(name, actor, osloader)

  @staticmethod
  def get(r, osloader):
    name = ('pid', r.pid.pid, 'syscall', r.sid)
    n = mgrapi.RegisteredObject.by_name(name)
    if n is None:
      n = CloneSyscallAction(name, osloader)
    return n

  def redo(self):
    if self.argsnode.data is None:
      dbg.syscall("no argsnode data for %s, returning prematurely" % self)
      return

    if (hasattr(self.argsnode, "rerun") and
        self.argsnode.rerun.is_active()):
      rerun = self.argsnode.rerun
      pid = self.argsnode.data.pid
      expanding = not hasattr(self, 'parent_retnode') or not self.parent_retnode
      if expanding:
        child_pid = None
      else:
        child_pid = self.parent_retnode.origdata.ret
      (r, p) = rerun.clone(pid, child_pid)
      r.sid = self.argsnode.data.sid  # TODO(ipopov): refactor into __fix_sid()

      if expanding:
        # This creates parent_retnode and child_retnode
        self.osloader.parse_record(r)
      else:
        self.child_retnode.data  = deepcopy(self.child_retnode.origdata)
        self.parent_retnode.data = deepcopy(self.parent_retnode.origdata)
        # TODO(ipopov): should we change the retvalue here? (probably
        # not)

      # find child actor and setup for rerun
      r = self.parent_retnode.data
      child = ProcessActor.get(r.ret, r)
      child.rerun = rerun
      rerun.add(child)
    else:
      # This should not happen.
      dbg.error('no redo performed for clonesyscall action!')

  def register(self):
    if self.argsnode:       self.inputset.add(self.argsnode)
    if self.parent_retnode: self.outputset.add(self.parent_retnode)
    if self.child_retnode:  self.outputset.add(self.child_retnode)

    dbg.connect("connect(): (%s,%s,%s)" %
                (self.argsnode, self.parent_retnode, self.child_retnode))

class SyscallAction(ISyscallAction):
  def __init__(self, name, osloader):
    self.argsnode = None
    self.retnode  = None
    actor = mgrutil.StatelessActor(name + ('sysactor',))
    super(SyscallAction, self).__init__(name, actor,
                                        osloader)

  @staticmethod
  def get(r, osloader):
    name = ('pid', r.pid.pid, 'syscall', r.sid)
    n = mgrapi.RegisteredObject.by_name(name)
    if n is None:
      n = SyscallAction(name, osloader)
    if hasattr(r, "uids"):
      setattr(n, "uids", r.uids)
    return n

  def equiv(self):
    # ipopov: conjecture: the following four lines are probably to
    # guard against equiv() going ahead with incomplete data about
    # argument values and return values and such.
    if self.argsnode is None or self.retnode is None:
      return False
    if self.argsnode.data is None or self.retnode.data is None:
      return False

    if (hasattr(self.argsnode, "rerun") and
        self.argsnode.rerun.is_active()):
      return False

    r = self.argsnode.data

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
    elif r.nr == syscall.NR_open:
      path = r.args['path']
      arg_name = path.path
      dir_dev = path.entries[0].parent.dev
      dir_ino = path.entries[0].parent.ino

      ret_dev = self.retnode.data.ret.inode.dev
      ret_ino = self.retnode.data.ret.inode.ino

      dirent = fsmgr.DirentDataNode.get(dir_dev, dir_ino, arg_name)
      (cur_dev, cur_ino) = dirent.get_file_dev_ino()

      #code.interact(local=locals())

      dbg.info('open on %s yields (dev, ino) (%s, %s) origdata was'
               '(%s %s)' % (path, cur_dev, cur_ino, ret_dev, ret_ino))

      if (cur_dev, cur_ino) == (ret_dev, ret_ino):
        # XXXX what if the file got _created_ by the open syscall? i.e.
        # if r.args['path'] contains no inode?
        return True
      else:
        return False

    if self.argsnode.origdata == self.argsnode.data:
      return True
    #
    # add any cases to stop the propagations
    #
    dbg.warn('#Y<XXX#> not quite right')
    return False

  def redo(self):
    dbg.syscall("redoing")
    if self.argsnode.data is None:
      dbg.syscall("no argsnode data for %s, returning prematurely" % self)
      return

    # share expanding & following
    if (hasattr(self.argsnode, "rerun") and
        self.argsnode.rerun.is_active()):
      rerun = self.argsnode.rerun
      pid   = self.argsnode.data.pid

      # wait syscalls: make it non-blocking
      if self.argsnode.data.name == "wait4":
        result = rerun.wait4(pid)
        if not result:
          self.flatten()
          raise mgrapi.ReschedulePendingException
        else:
          (r, p) = result
      elif self.argsnode.data.name == "exit_group":
        (r, p) = rerun.exit_group(pid)
        # from Taesoo:
        # while expanding (after canceling all nodes), you should
        # create new node (pcall) to dynamically create new nodes of
        # parents
        #
        # The current process is exiting. But, perhaps its reexecution
        # has spawned new children that must persist.
        if rerun.is_expanding() and len(rerun.pids) >= 1:
          for childpid in rerun.pids:
            dbg.info("reviewing: %s" % childpid)
            (childr, childp) = rerun.next(True, childpid)
            assert childr.usage == 0
            self.osloader.parse_record(childr)
        return
      else:
        (r, p) = rerun.next(False, pid)

      r.sid = self.argsnode.data.sid

      # Is this actually true?:
      # Vestigial, from Taesoo: r is None with exit_group()-like
      # syscalls
      assert not r or (r and r.usage == 1)
      if r:
        # self.retnode exists iff the exit record has already been
        # parsed for the syscall in question, i.e. iff this syscall
        # has already been carried out, i.e. iff we are retracing it.
        if self.retnode: # following
          self.retnode.data = deepcopy(self.retnode.origdata)
          self.retnode.data.ret = r.ret
        else: # exploring new nodes/edges
          self.osloader.parse_record(r)
      self.retnode.rerun = rerun
      return
    else:
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

    # WTF???!!! This can (and does) put the return value of the system
    # call into the node for the argument node! :(
    # XXXX
    # Why is this necessary???
    # XXXX
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
        #   objs.append(ptymgr.PtyNode.get(os.major(x.rdev), 0))
        # elif x.prefix == 'fifo':
        #   objs.append(fifomgr.FifoNode.get(x))
        # elif x.prefix == 'cdev':
        #   objs.append(cdevmgr.CdevNode.get(x))
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
    assert ((r.args["path"].inode is None) or
            (r.args["path"].inode == r.ret.inode))

    inode = r.ret.inode
    dbg.syscallm("!", "#R<open#>: %s" % (str(r)))
    if runopts.dryrun:
      return ret.origdata

    pn = r.args['path'].path  # pathname to file

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

    # If the syscall opened a _different_ inode than the file
    # points to now.
    path = r.args['path']
    arg_name = path.path
    dir_dev = path.entries[0].parent.dev
    dir_ino = path.entries[0].parent.ino

    dirent = fsmgr.DirentDataNode.get(dir_dev, dir_ino, arg_name)
    (cur_dev, cur_ino) = dirent.get_file_dev_ino()
    ret.data = deepcopy(ret.origdata)
    ret.data.ret.inode.ino, ret.data.ret.inode.dev = (cur_ino, cur_dev)

    return ret.data

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

  def cancel_hook(self):
    if not self.argsnode.data:
      dbg.warn('cancel called on %s, but no argsnode' % self)
      return
    if (hasattr(self.argsnode, "rerun") and
        self.argsnode.rerun.is_active()):
      self.argsnode.rerun.kill(self.argsnode.data.pid)
    else:
      dbg.warn('rerun instance present, but not killing process, '
               'because rerun is inactive')

  # dummy exit_group()
  def syscall_exit_group(self, arg, ret):
    pass

  @dbg.util.trace
  def flatten(self):
    """Set tic[0] to tac[0].

    Used for wait4 rescheduling."""
    assert self.retnode
    assert len(self.retnode.readers) == 1
    procSysRet = list(self.retnode.readers)[0]
    newtic = procSysRet.tic
    newtac = procSysRet.tac
    dbg.syscall('setting tic,tac %s %s -> %s %s' %
                (self.tic, self.tac, newtic, newtac))
    self.tic, self.tac = newtic, newtac
