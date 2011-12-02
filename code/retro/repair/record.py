from syscall import *
from sysarg import PathidTable
import os, os.path
import re
#import gzip, bz2
import nrdep
import copy, struct
import bsddb
import dbg
import sys

import code

from socket import AF_INET, inet_pton, ntohs

def _q(v):
  #code.interact(local=locals())
  if isinstance(v, list):
    s = "[" + ", ".join([_q(e) for e in v]) + "]"
  elif isinstance(v, bytes) or isinstance(v, str):
    s = '"' + repr(v).lstrip("b")[1:-1].replace('"', '\\"') + '"'
  else:
    s = repr(v)
  return s

class Record(object):
  def __cmp__(self, other):
    if self.ts < other.ts:
      return -1
    if self.ts > other.ts:
      return 1
    return 0
  def __lt__(self, other):
    return self.ts < other.ts
  def __eq__(self, other):
    return other and self.ts == other.ts

class SyscallRecord(Record):
  def __init__(self):
    self.ticket = None
  @property
  def name(self):
    return syscalls[self.nr].name
  def __repr__(self):
    t  = [self.ticket] if self.ticket else self.ts
    ts = ".".join([str(x) for x in t])
    return "sys:" + self.name + "@" + ts
  def __str__(self):
    args = []
    for a in syscalls[self.nr].args:
      x = a.name + "="
      v = self.args.get(a.name)
      if v is None:
        s = "None"
      elif a.ty in (sysarg_buf, sysarg_buf_det) and len(v) > 10:
        s = _q(v[0:11]) + "..."
      elif a.ty == sysarg_sha1:
        s = "sha1:" + "".join("%02X" % ord(c) for c in v)[:10]
      elif a.name in ["envp"]:
        s = "[/* %s vars */]" % (len(v))
      elif a.ty == sysarg_iovec:
        s = "%d:%s" % (len(v), ",".join(["%s@%d" % (_q(i[:5]), len(i)) for i in v]))
      else:
        s = _q(v)
      args.append(x + s)
    if hasattr(self, "usage") and self.usage in range(BOTH):
      prefix = ["", ">", "<", "<>"][self.usage] + " "
    else:
      prefix = ""
    s = prefix + " ".join([str(self.pid), self.name]) \
      + "(" + ", ".join(args) + ")"
    if hasattr(self, "ret"):
      s = s + " = " + str(self.ret)
    return s
  def nrdep(self):
    return nrdep.nrdep(self)

class KprobesRecord(Record):
  def __repr__(self):
    ts = ".".join([str(x) for x in self.ts])
    return str(self) + "@" + ts
  def __str__(self):
    return "%s:%s" % (self.inode.dev, self.inode.ino)

def _read_time(f):
  cpuid = int(re.search("[0-9]+$", f.name).group(0))
  sec = sysarg_uint(f)
  nsec = sysarg_uint(f)
  return (sec * (10**9) + nsec, cpuid)

def _read_sid(f):
  return struct.unpack('Q', f.read(8))[0]

def _fix_f(f):
  # find 0x84 0xE4 (time stamp) to fix up
  skip = 0
  dump = []
  while True:
    skip += 1
    c = f.read(1)
    dump.append(c)
    if ord(c) == 0x84:
      c = f.read(1)
      if ord(c) & 0xE0 == 0xE0:
        f.seek(f.tell() - 2)
        break
  dbg.warn("Skip:%d [%s]" % (skip, "".join("%02x" % ord(c) for c in dump)))

def read_syscall(f):
  # This is appalling.
  try:
    return _read_syscall(f)
  except EOFError:
    raise
  except TypeError:
    raise
  except AssertionError:
    raise
  except:
    # second chance
    # ORLY?
    _fix_f(f)
    return _read_syscall(f)

def _read_syscall(f):
  r = SyscallRecord()
  r.ts = _read_time(f)
  r.pid = sysarg_uint(f)
  r.usage = sysarg_uint(f)
#  XXX: do not check the usage currently since the record contains some invalid
#  usage data. It is a bug, but we do not need to fix it now since it does not
#  prevent the system from running.
  #assert r.usage in range(BOTH)
  r.nr = sysarg_uint(f)
  r.sid = _read_sid(f)
  print r.ts, r.pid, r.usage, r.nr, r.sid
  r.args = {}
  args = []

  sc = syscalls[r.nr]

  # UIDs are logged for execve
  if (r.usage & 2 != 0) and r.nr == NR_execve:
    r.uids = {}
    for uid in ["uid", "gid", "euid", "egid"]:
      r.uids[uid] = sysarg_uint(f)

  for i, a in enumerate(sc.args):
    if (a.usage & r.usage == 0):
      assert i == len(args)
      args.append(None)
      continue
    if i < len(args):
      v = args[i] # already set
    else:
      assert i == len(args)
      v = a.ty(f, a)
      args.append(v)
    r.args[a.name] = v
    if a.ty in [sysarg_buf, sysarg_buf_det]:
      if (a.usage & EXIT):
        assert sc.ret == sysarg_ssize_t
        if len(v) > 0:
          setattr(r, "ret", len(v))
      else:
        assert sc.args[i+1].ty == sysarg_size_t
        args.append(len(v))
    elif a.ty == sysarg_struct:
      if (a.usage & ENTER):
        if len(sc.args) > i+1 \
              and sc.args[i+1].ty == sysarg_size_t:
          args.append(0 if v is None else len(v))
    elif a.ty == sysarg_iovec:
      if (a.usage & ENTER):
        sc.args[i+1] == sysarg_int
        args.append(len(v))

  if (r.usage & EXIT) and not hasattr(r, "ret"):
    v = sc.ret(f)
    assert v is not None
    if r.name == "execve":
      r.aux = v
    if isinstance(v, list):
      assert not hasattr(r, "snapshot")
      r.snapshot = v[1:]
      v = v[0]
    r.ret = v
  return r

def read_kprobes(f):
  r = KprobesRecord()
  setattr(r, "ts", _read_time(f))
  setattr(r, "inode", sysarg_inode(f))
  assert r.inode is not None
  return r

# open a group of files and pop the earliest record
class _RecordQueue(object):
  def __init__(self, fs, read):
    # all file descriptors
    self._fs = fs[:]
    self._read = read
    # the earliest record of each file
    self._tops = [None] * len(self._fs)
  def top(self):
    for i, x in enumerate(self._tops):
      if x is None and self._fs[i] is not None:
        try:
          self._tops[i] = self._read(self._fs[i])
        except EOFError:
          self._tops[i] = None
          self._fs[i] = None
    self._fs = [f for f in self._fs if f is not None]
    self._tops = [x for x in self._tops if x is not None]
    assert len(self._fs) == len(self._tops)
    if len(self._tops) == 0:
      return None
    return min(self._tops)
  def pop(self):
    r = self.top()
    if r is not None:
      idx = self._tops.index(r)
      self._tops[idx] = None
    return r

class SyscallQueue(_RecordQueue):
  def __init__(self, fs):
    super(SyscallQueue, self).__init__(fs, read_syscall)

class KprobesQueue(_RecordQueue):
  def __init__(self, fs):
    self.gens = {}
    super(KprobesQueue, self).__init__(fs, read_kprobes)
  # fix inode gen number
  def fix(self, r):
    ts = r.ts
    for a in r.args.values():
      self._fix(a, ts)
    if hasattr(r, "ret"):
      self._fix(r.ret, ts)
  def _fix(self, ob, ts):
    if isinstance(ob, list):
      for e in ob:
        self._fix(e, ts)
    elif isinstance(ob, file):
      self._fix_inode(ob.inode, ts)
    elif isinstance(ob, namei):
      self._fix_inode(ob.inode, ts)
      for e in ob.entries:
        self._fix_inode(e.inode, ts)
  def _fix_inode(self, i, ts):
    # check type
    if i is None or i.mode == 0:
      return
    # update
    while True:
      r = self.top()
      if r is None or r.ts > ts:
        break
      self.pop()
      k = (r.inode.dev, r.inode.ino)
      gen = self.gens.get(k, 0) + 1
      self.gens[k] = gen
    # /dev/ptmx
    if hasattr(i, "pts"):
      k = (os.stat("/dev/pts").st_dev, 3 + i.pts)
    else:
      k = (i.dev, i.ino)
    gen = self.gens.get(k, 0)
    setattr(i, "gen", gen)

def zopen(fn):
  fmts = [] #[gzip.GzipFile, bz2.BZ2File]
  for fmt in fmts:
    try:
      f = fmt(fn, "rb")
      f.read(1)
      f.close()
      return fmt(fn, "rb")
    except:
      pass
  return open(fn, "rb")

def fix_r(r):
  """Syscall specific clean up."""
  if (r.usage & ENTER):
    if r.nr == NR_open and not r.args["flags"] & os.O_CREAT:
      r.args["mode"] = 0

def load(dirp):
  """This is a generator which avoids loading all files into memory.

  ipopov: This function is not used by regular repair. It is used
  by strace.py."""

  files = os.listdir(dirp)
  syscallN = [os.path.join(dirp, x) for x in files if re.match("syscall[0-9]+", x)]
  kprobesN = [os.path.join(dirp, x) for x in files if re.match("kprobes[0-9]+", x)]
  assert len(syscallN) > 0
  assert len(syscallN) == len(kprobesN)
  del files

  sfs = [zopen(fn) for fn in syscallN]
  kfs = [zopen(fn) for fn in kprobesN]
  sq = SyscallQueue(sfs)
  kq = KprobesQueue(kfs)
  pidd = PidDict()
  psyscalls = {}
  count = 1
  while True:
    r = sq.pop()
    if r is None:
      break
    setattr(r, 'tac', r.ts)
    # fix inode gen
    kq.fix(r)
    # fix pid gen
    pidd.fix(r)
    if r.pid not in psyscalls:
      psyscalls[r.pid] = []
    if (r.usage & ENTER):
      # an enter record
      if r.nr not in [NR_exit, NR_exit_group]:
        psyscalls[r.pid].append(r)
        continue
      # syscalls that never return
      # no exit record
      top = r
    else:
      # an exit record
      try:
        top = psyscalls[r.pid].pop()
      except:
        # lookup old process (prev gen)
        old = r.pid
        old.gen = r.pid.gen - 1
        top = psyscalls[r.pid].pop()

      assert r.nr == top.nr
      # merge args
      for a in syscalls[r.nr].args:
        if a.name not in r.args:
          r.args[a.name] = top.args[a.name]

    fix_r(r)

    # merge enter time
    setattr(r, "tic", top.tac)
    r.ticket = count
    count = count + 1
    delattr(r, "usage")
    del top
    # merge done
    yield r

  # clean up
  for f in sfs + kfs:
    f.close()
  for tops in psyscalls.values():
    assert len(tops) == 0

# to prevent pid reuse with generation number
class PidDict():
  """Maps a numeric pid to a process() object."""
  def __init__(self):
    self._dict = {}
  # fix pid gen number
  def fix(self, r):
    sc = syscalls[r.nr]

    # ipopov: conjecture:
    # This next line is nasty. We get passed in a record r, with a pid
    # attribute that is an integer. We then change the pid attribute
    # to be a process(pid, 0)... or something like that.
    #
    # Actually, it may be kind of clever...

    r.pid = self._get(r.pid)
    assert isinstance(r.pid, process)

    for i, a in enumerate(sc.args):
      if a == sysarg_pid_t:
        r[a.name] = self._get(r[a.name])
    if hasattr(r, "ret"):
      if sc.ret == sysarg_pid_t:
        r.ret = self._get(r.ret)
      # XXX
      # ipopov: The execution of a new process up until the execve has
      # pid:gen; thereafter, pid:(gen+1).
      if r.nr == NR_execve and r.ret == 0:
        r.pid = r.pid._bump()
        self._dict[r.pid.pid] = r.pid
      if r.nr in [NR_clone, NR_fork, NR_vfork] and r.ret in self._dict:
        p = self._dict[r.ret]
        self._dict[r.ret] = p._bump()
      if r.nr in [NR_exit, NR_exit_group]:
        p = r.pid
        self._dict[p.pid] = p._bump()
  def _get(self, pid):
    # ipopov:
    # What does this comparison do? Verify that pid is not of type
    # process? Why not "not instanceof(process)"?
    # My guess is that this works only by good fortune.
    if pid <= 0:
      return pid
    return self._dict.setdefault(pid, process(pid, 0))

class IndexLoader:
  @staticmethod
  def _make_cursor(name):
    db = bsddb.db.DB()
    db.open(name, flags=bsddb.db.DB_RDONLY)
    return db.cursor()

  @staticmethod
  def _dbget(idx, k):
    #code.interact(local=locals())
    dbg.load('key to dbget: ', repr(k))
    for i, c in enumerate(idx):
      dbg.load('i, c ', i, c)
      pair = c.get(k, bsddb.db.DB_SET)
      dbg.load('pair ', pair)
      while pair is not None:
        v = struct.unpack("=QHI", pair[1])
            #'=QHI': [
            #     unsigned long long (8), unsigned short (2),
            #     unsigned int(4)]
            #
            # This corresponds to this struct from //retro/ctl/retroctl.c:
            # struct {
            #       size_t offset;
            #       uint16_t nr;
            #       uint32_t sid;
            #       ...
            # }

        offset = v[0]
        nr = v[1]
        sid = v[2]
        yield (i, offset, nr, sid)
        pair = c.next_dup()

  def __init__(self, dirp):
    ## XXX because sorted() does not do numeric sort, we only support 10 cores.
    self.dirp = dirp
    files = os.listdir(dirp)

    syscallN = [os.path.join(dirp, x) for x in files if re.match("^syscall[0-9]$", x)]
    kprobesN = [os.path.join(dirp, x) for x in files if re.match("^kprobes[0-9]$", x)]
    pidN     = [os.path.join(dirp, x) for x in files if re.match("^pid[0-9]$"    , x)]
    inodeN   = [os.path.join(dirp, x) for x in files if re.match("^inode[0-9]$"  , x)]
    pathidN  = [os.path.join(dirp, x) for x in files if re.match("^pathid[0-9]$" , x)]
    sockN    = [os.path.join(dirp, x) for x in files if re.match("^sock[0-9]$"   , x)]

    assert len(syscallN) > 0
    assert len(syscallN) == len(kprobesN)
    assert len(syscallN) == len(pidN)
    assert len(syscallN) == len(inodeN)
    assert len(syscallN) == len(pathidN)
    assert len(syscallN) == len(sockN)
    del files

    #code.interact(local=locals())

    self.sfs     = [zopen(fn) for fn in sorted(syscallN)]
    self.kfs     = [zopen(fn) for fn in sorted(kprobesN)]
    self.inodes  = [self._make_cursor(fn) for fn in sorted(inodeN) ]
    self.pids    = [self._make_cursor(fn) for fn in sorted(pidN)   ]
    self.pathids = [self._make_cursor(fn) for fn in sorted(pathidN)]
    self.socks   = [self._make_cursor(fn) for fn in sorted(sockN)  ]

  def _read_records(self, offlist):
    records = []
    for (fidx, offset) in sorted(offlist):
      f = self.sfs[fidx]
      f.seek(0, 2)
      flen = f.tell()
      if offset >= flen:
        dbg.error('XXX system call offset out of range', offset, flen, f.name)
        continue
      f.seek(offset)
      try:
        r = read_syscall(f)
        records.append(r)
      except EOFError:
        ## Unfortunate, but ...
        pass
    return records

  def _get_records(self, keytype, key):
    key_sid = None
    pathids = None
    # ipopov: What is this absurd-looking arithmetic here?
    #         Answer: this is what inodes are indexed by. Go figure.
    if keytype == "ino":
      indices = self.inodes
      xorkey = key[0] ^ (key[1] & 0xffffffff)
      pathids = PathidTable.getpathids(xorkey)
      k = struct.pack("I", xorkey)
      #code.interact()
    elif keytype == "pid":
      indices = self.pids
      k = struct.pack("I", key[0])
      key_sid = key[1]
    elif keytype == "network":
      indices = self.socks
      ip = inet_pton(AF_INET, key[0])
      port = struct.pack("I", ntohs(key[1]))
      k = ip + port
    else:
      raise "Unknown keytype: " + keytype
    #print indices, k

    offsets = set()
    for (fidx, offset, nr, sid) in self._dbget(indices, k):
      if key[2] == 'writers':
        # The following syscalls are strictly readers. But not
        # NR_open, for it is a writer when truncating/creating files
        if nr in [NR_read, NR_access, NR_readlink, NR_dup2, NR_close,
                  NR_chdir, NR_munmap, NR_wait4]:
          continue
        # XXX
        # XXX indeed, Taesoo. Thank you.
        if nr == NR_execve and offset > 100000:
          continue
      if key_sid and (key_sid & 0xffffffff) != sid: continue
      p = (fidx, offset)
      offsets.add(p)

    if keytype == 'ino' and key[2] != 'writers':
      xorkey = xorkey ^ 0x80000000 # ro inode index keys have their
                                   # highest-order bit set
      k = struct.pack("I", xorkey)
      for (fidx, offset, nr, sid) in self._dbget(indices, k):
        p = (fidx, offset)
        offsets.add(p)

    if (pathids is not None and
        len(pathids) != 0 and
        key[2] != 'writers'):
      indices = self.pathids
      while len(pathids) != 0:
        k = pathids.pop()
        for (fidx, offset, nr, sid) in self._dbget(indices, k):
          p = (fidx, offset)
          offsets.add(p)

    records = self._read_records(offsets)

    # ??? ipopov ???
    # Now, also load records of pids impacted by the ones already
    # loaded (i.e. corresponding to clone actions)
    offsets2 = set()
    if keytype != 'pid' or key_sid:
      for pid in [r.pid for r in records]:
        pidk = struct.pack('i', pid)
        for (fidx, offset, nr, sid) in self._dbget(self.pids, pidk):
          if nr in [NR_clone, NR_fork, NR_vfork, NR_execve]:
            p = (fidx, offset)
            if p not in offsets:
              offsets2.add(p)
      records.extend(self._read_records(offsets2))

    return sorted(records)

  def load(self, keytype, key):
    dbg.load('loading key in load', keytype, key)
    if keytype is None:
      for f in self.sfs: f.seek(0)
      sq = SyscallQueue(self.sfs)
      records = iter(lambda: sq.pop(), None)
    else:
      records = self._get_records(keytype, key)
      #if len(records) > 1000: raise Exception('too many records')
    dbg.load('loaded records in load', records)
    # bingo!

    for kf in self.kfs: kf.seek(0)
    kq = KprobesQueue(self.kfs)
    pidd = PidDict()

    for r in records:
      try:
        kq.fix(r)       # fix inode gen
        pidd.fix(r)     # fix pid gen
        fix_r(r)  # fix args
      except EOFError:
        continue
      except:
        try:
          # fixup fs
          for f in self.sfs:
            _fix_f(f)
          continue
        except:
          raise StopIteration
      yield r
