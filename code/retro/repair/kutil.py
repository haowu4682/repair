import os
import dbg
import errno
import fcntl
import sysarg
import util

class MissingInode(Exception):
  pass

#@util.memorize
def fsget(dev):
  """Get the mount point of the device dev.
  """
  with open('/proc/mounts') as f:
    mtpts = [l.split(' ')[1] for l in f.readlines()]
  btrfsvols = [(x + '/trunk') for x in mtpts]
  for mtpt in mtpts + btrfsvols:
    try:
      s = os.stat(mtpt)
      if s.st_dev == dev:
        return mtpt
    except OSError:
      pass

  for mtpt in btrfsvols:
    probe_pn = mtpt + '/.btrfs_link_probe'
    try:
      os.symlink('probe', probe_pn)
      s = os.lstat(probe_pn)
      try:
        os.unlink(probe_pn)
      except OSError:
        pass
      if s.st_dev == dev:
        return mtpt
    except OSError:
      try:
        os.unlink(probe_pn)
      except OSError:
        pass
      pass

  raise Exception('cannot find file system', dev)

# XXX. it's incorrect cache but it might work in most test case
#
#      What were you thinking?????? Why was this paper
#      published????
wrong_cache = {}
def slow_iget(fs, ino):
  if (fs, ino) in wrong_cache:
    return wrong_cache[(fs, ino)]

  fs_stat = os.lstat(fs)
  if fs_stat.st_ino == ino:
    dbg.iget("  FOUND %s, %s == %s" % (fs, ino, fs))
    wrong_cache[(fs, ino)] = fs
    return fs

  for root, dirs, files in os.walk(fs):
    if root.find("/proc/") != -1:
      continue
    for f in files + dirs:
      pn = os.path.join(root,f)
      try:
        s = os.lstat(pn)
        if s.st_ino == ino and s.st_dev == fs_stat.st_dev:
          dbg.iget("  FOUND %s, %s == %s" % (fs, ino, pn))
          wrong_cache[(fs, ino)] = pn
          return pn
      except:
        pass

  raise MissingInode(fs, ino)

def fast_iget(fs, ino):
  """Search for inode with iget.ko module. Returns the file that
  contains the inode, under the "fs" mountpoint.
  """
  if fs in ["/proc"]:
    return slow_iget(fs, ino)

  dir = os.open(fs, os.O_RDONLY)
  ctl = os.open('/sys/kernel/debug/iget', os.O_RDONLY)

  try:
    r = fcntl.ioctl(ctl, dir, ino)
  except IOError, e:
    r = -e.errno

  os.close(dir)
  os.close(ctl)

  if r < 0 and r != -errno.EEXIST:
    raise Exception('cannot iget', fs, ino, r)

  f = os.path.join(fs, ".ino.%d" % ino)
  try:
    os.lstat(f)
  except OSError:
    raise MissingInode(fs, ino)
  return f

# slow_iget: scratching all files in the system
# fast_iget: search inode with iget.ko module
iget = fast_iget

# flushing dcache: use in case that you want to delete .ino* dcaches
def dcache_flush():
  global wrong_cache
  wrong_cache = {}
  with open('/proc/sys/vm/drop_caches', 'w') as f:
    f.write('2')

# search inode and return the pathename
def get(inode):
  assert type(inode) == sysarg._inode
  return iget(fsget(inode.dev), inode.ino)

# get the pathname of a saved inode which was deleted by rm syscall
def get_saved_inode(inode):
  return os.path.join(fsget(inode.dev), ".inodes", str(inode.ino))
