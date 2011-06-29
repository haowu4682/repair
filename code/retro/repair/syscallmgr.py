import graph
import record
import kutil
import dbg
import snapshot

from fsmgr import FileDataNode
from syscall import *

class SyscallActionNode(graph.ActionNode):
	pass

class OpenSyscallActionNode(SyscallActionNode):
	def redo(self, dryrun):
		cur_pn = kutil.iget("/mnt/retro/trunk", self.ob.ret.inode.ino)

		if not dryrun:
			if self.ob.args["flags"] & os.O_TRUNC:
				dbg.syscall("%d: %s" % (self.ob.ticket, self.ob))
				dbg.syscall("creating empty file, %s" % cur_pn)

				open(cur_pn, "w").close()
	def equiv(self):
		return True

class ReadSyscallActionNode(SyscallActionNode):
	def equiv(self):
		# TODO
		# find snapshot
		# fetching buf argument from the snapshot
		# compare
		pass

class WriteSyscallActionNode(SyscallActionNode):
	def redo(self, dryrun):
		cur_pn = kutil.iget("/mnt/retro/trunk", self.ob.args["fd"].inode.ino)

		dbg.syscall("%d: %s" % (self.ob.ticket, self.ob))
		dbg.syscall("re-writing to fd:%s" % cur_pn)

		if not dryrun:
			offset = self.ob.args["fd"].offset

			f = open(cur_pn, "r+")
			# append
			if offset == -1:
				f.seek(0, os.SEEK_END)
			else:
				f.seek(offset)
			f.write(self.ob.args["buf"])
			f.close()

	def equiv(self):
		return False

class Wait4SyscallActionNode(SyscallActionNode):
	def equiv(self):
		# FIXME check old & new state
		return True

class UnlinkSyscallActionNode(SyscallActionNode):
	def redo(self, dryrun):
		dbg.syscall("%d: %s" % (self.ob.ticket, self.ob))

		if not dryrun:
			if self.ob.ret != -1:
				p = self.ob.args["path"].path
				dbg.syscall("unlinking fd:%s" % p)
				os.unlink(p)

	def equiv(self):
		return False

#
# procmgr in previous implementation
#
class ExecveSyscallActionNode(SyscallActionNode):
	def redo(self, dryrun):
		dbg.syscall("re-executing %s", self.ob)
		if not dryrun:
			print self.ob.ret
			[ret, root, pwd, fds] = self.ob.ret
		pass

	def equiv(self):
		return False

def syscall_factory(name, conn, ob):
	sc = syscalls[ob.nr]

	# default
	cls = SyscallActionNode

	if sc.name == "open" \
		    and ob.args["path"].path.startswith("/dev/pts/"):
		# TODO
		# cls = PtsMgr
		pass

	if cls == SyscallActionNode:
		try:
			cls = eval("%sSyscallActionNode" % sc.name.capitalize())
		except:
			pass

	return cls(name, conn, ob)

graph.register_mgr(record.SyscallRecord, syscall_factory)

