#!/usr/bin/python

from collections import namedtuple,defaultdict
from ctypes import *
from stat import *

import code

import os
import dbg
import sys
import bsddb
import re
import util
import socket

from struct import pack
from socket import AF_INET, htons, inet_ntop

# types
# these objects are considered ``immutable''
# they are only modified during deserialization

class _hashable(object):
  def __eq__(self, other):
    if type(self) != type(other):
      return False
    for k, v in self.__dict__.items():
      if not k.startswith("_") and v != other.__dict__.get(k, None):
        return False
    return True
  def __ne__(self, other):
    return not self.__eq__(other)
  def __hash__(self):
    h = 0
    for k, v in self.__dict__.items():
      if not k.startswith("_"):
        h = h ^ hash(v)
    return h

class _inode(_hashable):
  def __init__(self, mode):
    assert mode > 0
    self.mode = mode
  @property
  def prefix(self):
    """Returns the prefix of the string representation of this inode.
    e.g. 'dir', 'fifo', ...
    """
    if hasattr(self, "_prefix"):
      return self._prefix
    assert self.mode > 0
    d = { S_ISDIR:"dir",  S_ISCHR:"cdev",  S_ISBLK:"bdev",
          S_ISREG:"file", S_ISFIFO:"fifo", S_ISLNK:"link",
          S_ISSOCK:"socket" }
    p = None
    for k, v in d.items():
      if k(self.mode):
        p = v
        break
    if p == None:
      dbg.warn("%s should be one of modes" % self.mode)

    if p == "cdev":
      if hasattr(self, "pts"):
        assert self.rdev == 0x0502
        return "ptm"
      major = os.major(self.rdev)
      if major >= 136 and major < 144:
        return "pts"
    self._prefix = p
    return p
  def __repr__(self):
    assert self.mode > 0
    p = self.prefix
    if p == "ptm":
      attr = [self.pts]
    elif p == "pts":
      attr = [os.minor(self.rdev)]
    elif p == "rdev" or p == "bdev":
      attr = [os.major(self.rdev), os.minor(self.rdev)]
    else:
      attr = [self.dev, self.ino]
    attr.append(hasattr(self, "gen") and str(self.gen) or "")
    return "%s:%s" % (p, ":".join([str(x) for x in attr]))


class file(_hashable):
  def __init__(self, fd):
    self.fd = fd
    self.inode = None
  def __repr__(self):
    r = str(self.fd)
    if self.fd >= 0:
      r = r + "[" + repr(self.inode) + "]"
      if hasattr(self, "offset"):
        r += "@" + ("a" if self.offset == -1 else repr(self.offset))
    return r

class entry(_hashable):
  def __init__(self, parent, name):
    self.parent = parent
    self.name = name
  def __repr__(self):
    return '(%s, %s)' % (repr(self.parent), repr(self.name))

class dentry(entry):
  @property
  def inode(self):
    return self.parent

class namei(_hashable):
  def __init__(self, path):
    self.path = path
    self.cached_entries = None
    self.inode   = None
    self.pathid  = None
    self.root_in = None
    self.root_pn = None
  def __repr__(self):
    r = '"' + self.path + '"'
    if self.inode:
      r += "[" + repr(self.inode) + "]"
    if self.entries:
      r += "[" + ";".join(["/".join([repr(e.inode), e.name])
        for e in self.entries]) + "]"
    if self.root_in:
      r += "@%s" % self.root_in.ino
    return r
  @property
  def entries(self):
    if self.cached_entries is None:
      self.cached_entries = PathidTable.expand(self.pathid)
    return self.cached_entries

class process(_hashable):
  def __init__(self, pid, gen):
    self.pid = pid
    self.gen = gen
  def __repr__(self):
    return "%s.%s" % (self.pid, self.gen)
  def _bump(self):
    return process(self.pid, self.gen+1)

# structures used in syscalls

class record_struct(Structure): #ctypes.Structure
  def __repr__(self):
    return "{" + ",".join("%s" % c[0] for c in self._fields_) + "}"
  def __len__(self):
    return sizeof(self)

class usrbuf(record_struct):
  pass

class siginfo(record_struct):
  _fields_ = [
    ("si_signo", c_int),
    ("si_errno", c_int),
    ("si_code" , c_int) ]

class sockaddr(record_struct):
  _pack_   = True
  _fields_ = [
    ("sa_family"  , c_ushort),
    ("sa_data"    , c_ubyte*14)]

  def __repr__(self):
    family = ""
    for f in filter(lambda f:f.startswith("AF_"), dir(socket)):
      if getattr(socket, f) == int(self.sa_family):
        family = f
        break
    if family == "AF_INET":
      return "{AF_INET: %s@%s}" % (self.get_ip(), self.get_port())
    return "{family:%s, data:%s}" \
        % (family, "".join("%02X" % c for c in self.sa_data[:8]))

  def get_port(self):
    return socket.ntohs(util.unpack_ushort(util.btos(self.sa_data[:2])))

  def get_ip(self):
    return socket.inet_ntoa(util.btos(self.sa_data[2:6]))

class msghdr(record_struct):
  _fields_ = [
    ("msg_name"  , c_long),
    ("msg_namelen"   , c_long),
    ("msg_iov"   , c_long),
    ("msg_iovlen"    , c_long),
    ("msg_control"   , c_long),
    ("msg_controllen", c_long),
    ("msg_flags"   , c_long)]

class pollfd(record_struct):
  _fields_ = [
    ("fd"   , c_int  ),
    ("events" , c_short),
    ("revents", c_short)]

class timeval(record_struct):
  _fields_ = [
    ("tv_sec"  , c_long),
    ("tv_usec" , c_long)]

class fd_set(record_struct):
  _fields_ = [
    ("fds_bits", c_ulong * 16)]

  def __repr__(self):
    return "{%X}" % self.fds_bits[0]

# functions

def sysarg_void(f, arg = None): pass

sysarg_ignore = sysarg_void

def _varint(f, signed):
  c = f.read(1)
  if not c: raise EOFError
  b = ord(c)
  if signed and (b & 0x40): v = -1
  else: v = 0
  while True:
    v = (v << 7) | (b & 0x7f)
    if (b & 0x80) == 0:
      break
    b = ord(f.read(1))
  return v

def sysarg_sint(f, arg = None):
  return _varint(f, True)

def sysarg_uint(f, arg = None):
  return _varint(f, False)

def sysarg_sint32(f, arg = None):
  return sysarg_sint(f)

def sysarg_uint32(f, arg = None):
  return sysarg_uint(f)

sysarg_voidp    = sysarg_uint
sysarg_size_t   = sysarg_uint
sysarg_dev_t    = sysarg_uint
sysarg_uid_t    = sysarg_uint32
sysarg_gid_t    = sysarg_uint32
sysarg_ino_t    = sysarg_uint
sysarg_mode_t   = sysarg_uint32
sysarg_id_t     = sysarg_uint32
sysarg_idtype_t = sysarg_uint32
sysarg_off_t    = sysarg_sint
sysarg_clock_t  = sysarg_sint
sysarg_time_t   = sysarg_sint
sysarg_ssize_t  = sysarg_sint
sysarg_int      = sysarg_sint32

def sysarg_psize_t(f, arg = None):
  return sysarg_uint32(f)

def sysarg_pid_t(f, arg = None):
  return sysarg_sint32(f)

def sysarg_intp(f, arg = None):
  n = sysarg_size_t(f)
  return [sysarg_int(f) for i in range(n)]

def sysarg_buf(f, arg = None):
  n = sysarg_size_t(f)
  return f.read(n)

def sysarg_buf_det(f, arg = None):
  return sysarg_buf(f, arg)

def sysarg_sha1(f, arg = None):
  n = sysarg_size_t(f)
  return f.read(n)

def sysarg_pathid(f, arg = None):
  return f.read(20)

def sysarg_name(f, arg = None):
  s = sysarg_buf(f)
  if not isinstance(s, str): # python 3.x
    s = s.decode()
  return s

def sysarg_string(f, arg = None):
  return sysarg_buf(f, arg)

def sysarg_strings(f, arg = None):
  n = sysarg_size_t(f)
  return [sysarg_buf(f) for i in range(n)]

def sysarg_path_at(f, arg = None):
  path = sysarg_name(f)
  ni = namei(path)
  if len(path) == 0:
    return ni
  ni.root_in = sysarg_inode(f)
  ni.root_pn = sysarg_name(f)
  ni.pathid  = sysarg_pathid(f)
  ni.inode   = sysarg_inode(f)
  #code.interact(local=locals())
  return ni

def sysarg_rpath_at(f, arg = None):
  return sysarg_path_at(f)

def sysarg_path(f, arg = None):
  return sysarg_path_at(f)

def sysarg_rpath(f, arg = None):
  return sysarg_path_at(f)

def sysarg_fd(f, arg = None):
  filp = file(sysarg_int(f))
  if filp.fd >= 0:
    filp.inode = sysarg_inode(f, arg)
    if getattr(filp.inode, "rdev", 0) == 0x0502:
      setattr(filp.inode, "pts", sysarg_int(f))
    if arg and arg.aux:
      setattr(filp, "offset", sysarg_int(f))
    if S_ISSOCK(getattr(filp.inode, "mode", -1)):
      family = sysarg_uint(f)
      if family == AF_INET:
        sport = sysarg_uint(f)
        dport = sysarg_uint(f)
        dip   = sysarg_uint(f)

        # need to check family
        setattr(filp.inode, "sport", htons(sport))
        setattr(filp.inode, "dport", htons(dport))
        setattr(filp.inode, "dip"  , inet_ntop(AF_INET, pack("I", dip)))

  return filp

def sysarg_inode(f, arg = None):
  mode = sysarg_mode_t(f)
  if mode == 0:
    return None
  i = _inode(mode)
  i.dev = sysarg_dev_t(f)
  i.ino = sysarg_ino_t(f)
  if S_ISCHR(mode) or S_ISBLK(mode):
    i.rdev = sysarg_dev_t(f)
  else:
    i.rdev = 0
  return i

def sysarg_fd2(f, arg):
  return [sysarg_fd(f, arg) for i in range(2)]

def sysarg_dirfd(f, arg):
  return sysarg_fd(f, arg) # TODO

def sysarg_struct(f, arg):
  ty = arg.aux
  # dynamic size
  if ty == usrbuf:
    return sysarg_buf(f, arg)
  size = sysarg_size_t(f)
  # null pointer
  if size == 0:
    return None
  # XXX AF_INET6
  # assert size <= sizeof(ty)
  buf = create_string_buffer(f.read(size))
  return cast(buf, POINTER(ty)).contents

def sysarg_iovec(f, arg = None):
  n = sysarg_size_t(f)
  return [sysarg_sha1(f) for i in range(n)]

def sysarg_execve(f, arg = None):
  ret = sysarg_int(f)
  if ret != 0:
    return ret
  root = sysarg_inode(f)
  setattr(root, "_path", [sysarg_name(f)])
  pwd = sysarg_inode(f)
  setattr(root, "_path", [sysarg_name(f)])
  fds = []
  while True:
    fd = sysarg_fd(f)
    if fd.inode is None:
      break
    setattr(fd.inode, "offset", sysarg_int(f))
    setattr(fd.inode, "_flags", sysarg_int(f))
    fds.append(fd)
  return [ret, root, pwd, fds]

class PathidTable:
  pdt_entry = namedtuple("pdt_entry", "name inode parent")

  pdt = None

  @staticmethod
  def is_initialized():
    return bool(PathidTable.pdt)

  @staticmethod
  def initialize(dirp):
    PathidTable.pdt = PathidTable(dirp)

  @staticmethod
  def expand(pathid):
    """Traverse up the pathid->parent_pathid hierarchy and return a
    list of dentry's corresponding to the path.
    """
    if PathidTable.pdt is None:
      raise Exception("PathidTable not initialized")
    return PathidTable.pdt._expand(pathid)

#  def expand(self, pathid):
#    return self_expand(pathid)


  @staticmethod
  def getpathids(dev_xor_ino):
    if PathidTable.pdt is None:
      raise Exception("PathidTable not initialized")
    return PathidTable.pdt._getpathids(dev_xor_ino)

#  def getpathids(self, dev_xor_ino):
#    return self._getpathids(dev_xor_ino)

  # for debugging
  @staticmethod
  def dumptables():
    print 'pathid_map:'
    if PathidTable.pdt is None: raise Exception("PathidTable not initialized")
    for pathid, v in PathidTable.pdt.pathid_map.iteritems():
      print str_to_hex(pathid), v.name, v.inode, str_to_hex(v.parent)
    print
    print 'inode_map:'
    for xorkey, pathids in PathidTable.pdt.inode_map.iteritems():
      for pathid in pathids:
        print hex(xorkey), str_to_hex(pathid)

  def __init__(self, dirp):
    self.pathid_map = {}
    self.inode_map = defaultdict(set) # ino_xor_key -> pathid
    self.dirp = dirp
    files = os.listdir(dirp)
    syscallN = [os.path.join(dirp, x) for x in files if
                re.match("^syscall[0-9]$", x)]
    pathidtableN = [os.path.join(dirp, x) for x in files if
                    re.match("^pathidtable[0-9]$", x)]
    assert len(syscallN) == len(pathidtableN)

    parents = set()
    for fn in pathidtableN:
      db = bsddb.db.DB()
      db.open(fn, flags=bsddb.db.DB_RDONLY)
      for k,v in db.items():
        value = self._parse_dbvalue(v)
        parents.add(value.parent)
#XXX: assertion canceled by Hao here
#       assert not self.pathid_map.has_key(k)
        self.pathid_map[k] = value
    for k in set(self.pathid_map.keys()).difference(parents):
      pathids = [k]
      v = self.pathid_map[k]
      self.inode_map[self._make_xorkey(v)].update(pathids)
      while v.parent != 20*'\0':
        pathids.append(v.parent)
        v = self.pathid_map[v.parent]
        self.inode_map[self._make_xorkey(v)].update(pathids)

  def _make_xorkey(self, v):
    return v.inode.dev ^ (v.inode.ino & 0xffffffff)

  def _parse_dbvalue(self, dbvalue):
    """dbvalue -> (name, inode, parent).
    """
    assert dbvalue
    parent = dbvalue[:20]
    lf = util.ListFile(dbvalue[20:])
    inode = sysarg_inode(lf)
    name = dbvalue[20+lf.i:]
    return PathidTable.pdt_entry(name, inode, parent)

  def _expand(self, key):
    p = []
    while key in self.pathid_map:
      value = self.pathid_map[key]
      p.append(dentry(value.inode, value.name))
      key = value.parent
    return p
  def _getpathids(self, dev_xor_ino):
    dbg.pathid('pathids for %s: %s' %
               (str(dev_xor_ino),
               map(str_to_hex, self.inode_map.get(dev_xor_ino, []))))
    return self.inode_map.get(dev_xor_ino, None)

def str_to_hex(s):
  return ''.join([("%02x" % ord(byte)) for byte in s])

def sysarg_msghdr(f, arg = None):
  # parse msghdr
  msghdr = sysarg_struct(f, arg)
  # parse sockaddr
  class mock: pass
  arg = mock
  arg.aux = sockaddr
  msghdr.name = sysarg_struct(f, arg)
  msghdr.iov  = sysarg_iovec(f)
  return msghdr

