import os
import errno
import stat
import shutil

import mgrapi
import kutil
import dbg
import runopts

def search_snap(dev, ino):
	## XXX hack to find snapshots
	fs = kutil.fsget(dev)
	if fs in ['/'] or any(fs.startswith(d) for d in ['/proc', '/dev']):
		yield(mgrapi.MinusInfinity(), mgrapi.MinusInfinity(), fs)
		return
	if not fs.endswith('trunk'): return

	root = os.path.dirname(fs)

	for s in os.listdir(root):
		if s.startswith("snap"):
			t = (int(s.split("-")[-1]) * (10**9),)
			yield (t, t, os.path.join(root, s))
	mi = mgrapi.MinusInfinity()

	if os.path.exists(os.path.join(root, 'fresh')):
		yield (mi, mi, os.path.join(root, 'fresh'))

class FileCheckpoint(mgrapi.TimeInterval, mgrapi.TupleName):
	def __init__(self, tic, tac, dev, ino, snapdir_pn):
		self.name = ('File', dev, ino, snapdir_pn)
		self.dev  = dev
		self.ino  = ino
		self.d_pn = snapdir_pn
		
		super(FileCheckpoint, self).__init__(tic, tac)
		
	def pn(self):
		try:
			return kutil.iget(self.d_pn, self.ino)
		except kutil.MissingInode:
			return None

class FileContentCheckpoint(mgrapi.TimeInterval, mgrapi.TupleName):
	def __init__(self, tic, tac, dev, ino):
		fs  = kutil.fsget(dev)
		src = kutil.iget(fs, ino)
		dst = os.path.join(fs, ".inodes", str(ino) + "-" + str(tic[0]))

		shutil.copy(src, dst)

		self.name = ('File', dev, ino, dst)
		self.dev  = dev
		self.ino  = ino
		self.snap = dst
		
		super(FileContentCheckpoint, self).__init__(tic, tac)
		
	def pn(self):
		return self.snap

class FileDataNode(mgrapi.DataNode):
	def __init__(self, name, dev, ino):
		super(FileDataNode, self).__init__(name)
		self.dev = dev
		self.ino = ino
		for (tic, tac, snapdir_pn) in search_snap(dev, ino):
			cp = FileCheckpoint(tic, tac, self.dev, self.ino,
					    snapdir_pn)
			self.checkpoints.add(cp)
			
	@staticmethod
	def get(dev, ino):
		name = ('ino', dev, ino)
		n = mgrapi.RegisteredObject.by_name(name)
		if n is None:
			n = FileDataNode(name, dev, ino)
		return n
	
	def rollback(self, c):
		assert(isinstance(c, FileCheckpoint))

		# XXX. hack to ignore /proc and /dev entries
		if any(c.d_pn.startswith(d) for d in ["/proc", "/dev"]):
			return None
		
		cpn = c.pn()

		try:
			dev_pn = kutil.fsget(self.dev)
			ino_pn = kutil.iget(dev_pn, self.ino)
		except kutil.MissingInode:
			if cpn is None:
				return

		dbg.infom("!", "#R<rollback#> %s(%s) -> %s" % (self.ino, ino_pn, cpn))

		if runopts.dryrun:
			return

		cstat = cpn and os.lstat(cpn) or None
		istat = os.lstat(ino_pn)

		if cstat and \
		   (stat.S_IFMT(istat.st_mode) != stat.S_IFMT(cstat.st_mode)):
			raise Exception('inode changed types',
					cpn, ino_pn, cstat, istat)

		if stat.S_ISREG(istat.st_mode):
			if cpn:
				with open(cpn, 'r') as f1:
					f1data = f1.read()
			else:
				f1data = ''
			with open(ino_pn, 'r') as f2:
				f2data = f2.read()
			if f1data != f2data:
				with open(ino_pn, 'w') as f2:
					f2.write(f1data)
		elif stat.S_ISLNK(istat.st_mode):
			pass    ## no way to change existing symlink
		elif stat.S_ISDIR(istat.st_mode):
			# XXX.
			pass
		else:
			raise Exception('cannot handle inode type',
					stat.S_IFMT(istat.st_mode))

class DirentDataNode(mgrapi.DataNode):
	def __init__(self, name, dev, ino, dirname):
		super(DirentDataNode, self).__init__(name)
		self.dev = dev
		self.ino = ino
		self.dirname = dirname
		self.fnode = FileDataNode.get(dev, ino)
		self.checkpoints = self.fnode.checkpoints

	@staticmethod
	def get(dev, ino, dirname):
		name = ('dir', dev, ino, dirname)
		n = mgrapi.RegisteredObject.by_name(name)
		if n is None:
			n = DirentDataNode(name, dev, ino, dirname)
		return n

	def get_inode(self, dirpn, pn):
		try:
			ino = os.lstat(os.path.join(dirpn, pn)).st_ino
		except OSError, e:
			if e.errno != errno.ENOENT: raise
			ino = None
		return ino
		
	def rollback(self, c):
		assert(isinstance(c, FileCheckpoint))

		# XXX. hack to ignore /proc entries
		if any(c.d_pn.startswith(d) for d in ["/proc", "/dev"]):
			return

		cp_ino = None
		
		cpn = c.pn()
		if cpn:
			cp_ino = self.get_inode(cpn, self.dirname)

		dev_pn = kutil.fsget(self.dev)
		dir_pn = kutil.iget(dev_pn, self.ino)

		cur_ino = self.get_inode(dir_pn, self.dirname)

		if cp_ino == cur_ino:
			return

		if cur_ino is not None:
			src = os.path.join(dir_pn, self.dirname)
			dst = os.path.join(dev_pn, ".inodes", str(cur_ino))

			dbg.infom("!", "#R<rename#>: %s -> %s" % (src, dst))

			if not runopts.dryrun:
				os.rename(src, dst)
			
		if cp_ino is not None:
			src = kutil.iget(dev_pn, cp_ino)
			dst = os.path.join(dir_pn, self.dirname)

			dbg.infom("!", "#R<link#>: %s -> %s" % (src, dst))
			
			if not runopts.dryrun:
				os.link(src, dst)
			
		kutil.dcache_flush()

