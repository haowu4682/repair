import mgrapi, record, mgrutil, dbg
import procmgr, fsmgr
import sysarg, nrdep

from syscall import *

ld = None
loadset = set()
def set_logdir(pn):
	global ld
	ld = record.IndexLoader(pn)
	PathidTable.initialize(pn)

def parse_record(r):
	argsnode       = None
	retnode        = None
	retnode_child  = None
	retnode_parent = None

	is_clone = r.nr in [NR_clone, NR_fork, NR_vfork]

	# enter
	if r.usage == 0:
		actor_call = procmgr.ProcessActor.get(r.pid, r)
		argsname = actor_call.name + ('sysarg', r.sid)
		if mgrapi.RegisteredObject.by_name(argsname) is None:
			argsnode = mgrutil.BufferNode(argsname, r.ts + (2,), r)
			pc = procmgr.ProcSysCall(actor_call, argsnode)
			pc.tic = r.ts + (0 + r.pid.gen,)
			pc.tac = r.ts + (1 + r.pid.gen,)
			pc.connect()

	# exit
	if r.usage == 1:
		if not is_clone:
			actor_ret = procmgr.ProcessActor.get(r.pid, r)
			retname = actor_ret.name + ('sysret', r.sid)
			if mgrapi.RegisteredObject.by_name(retname) is None:
				retnode = mgrutil.BufferNode(retname, r.ts + (1,), r)
				pr = procmgr.ProcSysRet(actor_ret, retnode)
				pr.tic = r.ts + (2 + r.pid.gen,)
				pr.tac = r.ts + (3 + r.pid.gen,)
				if r.nr == NR_execve:
					assert hasattr(r, "uids")
					setattr(pr, "uids", r.uids)
				pr.connect()
		else:
			actor_child  = procmgr.ProcessActor.get(r.ret, r)
			actor_parent = procmgr.ProcessActor.get(r.pid, r)

			retname_parent = actor_parent.name + ('sysret', r.sid)
			retname_child  = actor_child.name  + ('sysret', r.sid)
			if mgrapi.RegisteredObject.by_name(retname_parent) is None:
				retnode_parent = mgrutil.BufferNode(retname_parent,
								    r.ts + (1,), r)
				pr = procmgr.ProcSysRet(actor_parent, retnode_parent)
				pr.tic = r.ts + (4 + r.pid.gen,)
				pr.tac = r.ts + (5 + r.pid.gen,)
				pr.connect()
			if mgrapi.RegisteredObject.by_name(retname_child) is None:
				retnode_child = mgrutil.BufferNode(retname_child,
								   r.ts + (1,), r)
				pr = procmgr.ProcSysRet(actor_child, retnode_child)
				pr.tic = r.ts + (2 + r.pid.gen,)
				pr.tac = r.ts + (3 + r.pid.gen,)
				pr.connect()

	if is_clone:
		sc = procmgr.CloneSyscallAction.get(r)
		if retnode_parent: sc.parent_retnode = retnode_parent
		if retnode_child:  sc.child_retnode  = retnode_child
	else:
		sc = procmgr.SyscallAction.get(r)
		if retnode: sc.retnode = retnode

	if argsnode: sc.argsnode = argsnode
	if r.usage == 0: sc.tic = r.ts + (3,)
	if r.usage == 1: sc.tac = r.ts + (0,)

	## Some system calls do not have return records
	## (or return value objects, but that's OK for now.)
	if r.nr in [NR_exit, NR_exit_group]: sc.tac = r.ts + (4,)

	sc.connect()

	return sc

def is_loaded(req, loadset):
	for (type, key, what) in loadset:
		if type == req[0] and key == req[1]:
			if req[2] == what:
				return True
			# we load alread everything
			if what == "all":
				return True
def load(n, what):
	global ld, loadset

	enable_incremental = True
	type = None
	key  = None

	if n is not None:
		if n[0] == 'pid':
			type = 'pid'
			if n[2] == 'syscall':
				key = (n[1], n[3])
			else:
				key = (n[1], None)
		elif n[0] == 'ino' or n[0] == 'dir':
			type = 'ino'
			key  = (n[1], n[2])
		elif n[0] == 'network':
			type = 'network'
			key  = (n[1], n[2])
		else:
			raise Exception('no incremental loading for', n)
		if not enable_incremental:
			return

	## normalize the kinds of "what" we support
	if what != 'writers': what = 'all'
	if key: key = key + (what,)

	keytrack = (type, key, what)
	if is_loaded(keytrack, loadset):
		dbg.load('skipping dup load request', keytrack)
		return
	loadset.add(keytrack)

	dbg.load('loading key', type, key)
	for r in ld.load(type, key):
		dbg.load('record', r.sid, r, what, type)

		# filter out readers when all we want is to load writers
		if what == 'writers' and type == 'ino':
			(read, write) = nrdep.check_rw(r)
			if not write: continue

		parse_record(r)

mgrapi.LoaderMap.register_loader('ino'    , load)
mgrapi.LoaderMap.register_loader('dir'    , load)
mgrapi.LoaderMap.register_loader('pid'    , load)
mgrapi.LoaderMap.register_loader('network', load)

