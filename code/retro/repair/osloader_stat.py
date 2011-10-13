import mgrapi, record, mgrutil, dbg
import procmgr, fsmgr
import nrdep

from syscall import *
import sysarg

ld = None
def set_logdir(pn):
	global ld
	ld = record.IndexLoader(pn)

def procname(pid):
	return ('pid', pid.pid, pid.gen)

storage = {}

def get_obj(name):
	global storage
	if name in storage:
		return storage[name]
	return None

def set_obj(cat, name):
	global storage
	storage[name] = cat

#
# XXX merge with osloader
#
# make retro not to spend memory by auditing the code
#
def load(n):
	cnt = 0
	for r in ld.load(None, None):
		cnt += 1

		argsnode       = None
		retnode        = None
		retnode_child  = None
		retnode_parent = None
		is_clone = r.nr in [NR_clone, NR_fork, NR_vfork]

		if r.usage & ENTER:
			actor_call = procname(r.pid)
			argsname = actor_call + ('sysarg', r.sid)
			set_obj("ProcessActor", actor_call)
			if get_obj(argsname) is None:
				set_obj("ProcSysCall", argsname + ('pcall',))
				set_obj("BufferNode" , argsname)

		if r.usage & EXIT:
			if not is_clone or (is_clone and r.ret < 0):
				actor_ret = procname(r.pid)
				retname = actor_ret + ('sysret', r.sid)
				set_obj("ProcessActor", actor_ret)

				if get_obj(retname) is None:
					set_obj("ProcSysRet" , retname + ('pret',))
					set_obj("BufferNode" , retname)

			else:
				actor_child  = procname(r.ret)
				actor_parent = procname(r.pid)

				set_obj("ProcessActor", actor_child)
				set_obj("ProcessActor", actor_parent)

				retname_parent = actor_parent + ('sysret', r.sid)
				retname_child  = actor_child  + ('sysret', r.sid)

				if get_obj(retname_parent) is None:
					set_obj("ProcSysRet" , actor_parent + ('pret',))
					set_obj("BufferNode" , retname_parent)

				if get_obj(retname_child) is None:
					set_obj("ProcSysRet" , actor_child + ('pret',))
					set_obj("BufferNode" , retname_child)

		if is_clone:
			name = ('pid', r.pid.pid, 'syscall', r.sid)
			set_obj("CloneSyscallAction", name)
			set_obj("StatelessActor", name + ('syscator',))
		else:
			name = ('pid', r.pid.pid, 'syscall', r.sid)
			set_obj("SyscallAction", name)
			set_obj("StatelessActor", name + ('sysactor',))

			if r.usage & ENTER:
				(rs, ws) = nrdep.nrdep(r)

				for x in rs | ws:
					n = None
					if type(x) == sysarg._inode:
						if x.prefix in ('file', 'link', 'dir'):
							name = ('ino', x.dev, x.ino)
							set_obj("FileDataNode", name)
						elif x.prefix == 'pts':
							name = ('pts', os.major(x.rdev), x.gen)
							set_obj("PtyNode", name)
						else:
							# dbg.error('need data node for inode type', x.prefix)
							pass
					elif type(x) == sysarg.process:
						# dbg.info(r, ': pid:%s -> pid:%s' % (self.argsnode.data.pid, x))
						pass
					elif type(x) == sysarg.dentry:
						name = ('dir', x.inode.dev, x.inode.ino, x.name)
						set_obj("DirentDataNode", name)
					else:
						# dbg.error('need data node for', x, 'type', type(x))
						pass

		if cnt % 1000 == 0:
			print "Processing ... %s syscalls\r" % cnt,
		del r

mgrapi.LoaderMap.register_loader('ino', load)
mgrapi.LoaderMap.register_loader('dir', load)
mgrapi.LoaderMap.register_loader('pid', load)
