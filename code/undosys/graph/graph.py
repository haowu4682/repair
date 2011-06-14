#:!/bin/sh
magic='--calling-python-from-/bin/sh--'
"""exec" python -E "$0" "$@" """#$magic"
if __name__ == '__main__':
  import sys
  if sys.argv[-1] == '#%s' % magic:
    del sys.argv[-1]
del magic


import sys
import re
import optparse
import xdrlib
import record
import struct
import Queue
import os
import pickle
import stat
import shutil
import itertools
import patch
import rerun
import utmp
import logging, logging2

#
# trigger pdb when error
#
def install_pdb() :
    def info(type, value, tb):
        if hasattr(sys, 'ps1') or not sys.stderr.isatty():
            # You are in interactive mode or don't have a tty-like
            # device, so call the default hook
            sys.__execthook__(type, value, tb)
        else:
            import traceback, pdb
            # You are not in interactive mode; print the exception
            traceback.print_exception(type, value, tb)
            print
            # ... then star the debugger in post-mortem mode
            pdb.pm()

    sys.excepthook = info

install_pdb()

## Workarounds that should be addressed more cleanly later on.
ugly_hacks = True

## statistics
statistics = {
    'proc_redo':set(),
    'fs_redo':set(),
    'fs_undo':set(),
    'fs_redo':set(),
    'dir_redo':set(),
    'dir_undo':set(),
    'off_redo':set(),
    'off_undo':set(),
    'pts_redo':set(),
    'pts_undo':set(),
    'pwd_redo':set(),
    'edge_same':set(),
    'edge_considered':set(),
    'edge_repaired':set(),
    'node_considered':set(),
    'node_repaired':set(),
}


parser = optparse.OptionParser()
parser.add_option("-e", "--exclude", action="store_true", default=False,
                  dest="exclude", help="exclude /usr etc.")
parser.add_option("-b", "--backtrack", dest="backtrack", type="int",
		  metavar="LINE", help="line of suspect action for backtracking")
parser.add_option("-f", "--forward", dest="forward", type="int",
                  metavar="LINE", help="deprecated, use -a instead")
parser.add_option("-q", "--quiet", action="store_const", const=logging.FATAL,
                  dest="level", help="suppress normal logging")
parser.add_option("-v", "--verbose", action="store_const", const=logging.DEBUG,
                  dest="level", help="verbose logging", default=logging.INFO)
parser.add_option("-V", "--very-verbose", action="store_true", default=False,
                  dest="vverbose", help="verbose node names")
parser.add_option("-a", "--attacker", dest="attacker", type="string",
                  metavar="PID", help="attacker's pid")
parser.add_option("-r", "--repair", action="store_true", default=False,
                  dest="repair", help="repair")
parser.add_option("-d", "--dry-run", action="store_true", default=False,
		  dest="dryrun", help="dry run for repair")
parser.add_option("-o", "--output", dest="graphout", type="string",
		  metavar="DOTFILE", help="output graph dot file")
parser.add_option("-l", "--log", dest="logfile", type="string",
		  metavar="RECORDLOG", help="input record.log file")
opt, args = parser.parse_args() #getoionetopt(sys.argv[1:], 'rbef:')
if len(args) != 0:
    parser.error("error; try -h to see options")
if not opt.logfile:
    parser.error("record log file required")
opt.verbose = (opt.vverbose or opt.level <= logging.DEBUG)

logging2.coloredConfig(
    level=opt.level,
    format="[%(funcName)s] %(message)s",
    color=True
)

# processes should be named by pid plus gen #
# files should be named by inode #, gen #
# name should be named ?

history = []
maskset = []		## for a given history line, currently masked objects
actors  = []		## for a given history line, stack of delegated actors
pidgen  = []		## for a given history line, the gen# of the PID

exclude_list = ['/bin', '/lib', '/usr', '/proc', '/dev', '/etc']

def q(s):
    return '"' + s + '"'

def dump_label(n, a):
    s = '"' + n + ' (' + a['_undomgr'] + ')'
    for i in a:
	if not i.startswith('_'):
	    s += '\\n' + i
    s += '"'
    return s

def exclude_name(pname):
    if opt.exclude == False:
        return False
    for i in exclude_list:
        if pname.startswith(i):
            return True
    return False

class Edge:
    def __init__(self, s, d, t):
        self.s = s
        self.d = d
        self.t = t
        self.a = {}
    def __str__(self):
	return self.tostr()
    def src(self):
        return self.s
    def dst(self):
        return self.d
    def time(self):
        return self.t
    def add_attr(self, attr):
        self.a[attr] = None
    def get_attr(self):
        return self.a
    def tostr(self):
	if opt.vverbose:
	    return self.s + " --> " + self.d + " (" + str(self.a) + ") @ " + str(self.t)
	else:
	    return self.s + "-->" + self.d + "@" + str(self.t)
    def display(self):
        print self.tostr()
    def dump(self, f):
	if '_repair_state' in self.a:
	    if self.a['_repair_state'] == 'undone':
		a = ',color=red'
	    elif self.a['_repair_state'] == 'redone':
		a = ',color=yellow'
	    else:
		print 'XXX unknown edge repair state', self.a['_repair_state']
        elif 'attacker' in self.a:
            a = ',color=green,fontcolor=green'
        else:
            a = ''
        f.write(q(self.s) + ' -> ' + q(self.d) + ' [label='+str(self.t)+a+'];\n')
        
class Node:
    def __init__(self, t, n):
        self.t = t
        self.n = n
        self.a = {}
    def __str__(self):
	return self.tostr()
    def label(self):
        return self.n
    def add_attr(self, attr):
        if attr in self.a:
            return
        self.a[attr] = None
    def get_attr(self):
        return self.a
    def tostr(self):
	if opt.vverbose:
	    return "[type: " + self.t + " label: " + self.n + " attr: " + str(self.a) + "]"
	else:
	    return self.n
    def display(self):
        print self.tostr()
    def skip(self):
        if opt.exclude == True and 'exclude' in self.get_attr():
            return True
        else:
            return False
    def dump(self, f):
        if self.skip():
            return
	if '_repair_state' in self.a:
	    if self.a['_repair_state'] == 'dead':
		extra = ', style = filled, color=red,'
	    elif self.a['_repair_state'] == 'reexecuted':
		extra = ', style = filled, color=yellow,'
	    else:
		print 'XXX unknown repair state', self.a['_repair_state']
	elif '_repair_loop_last_state' in self.a:
	    if self.a['_repair_loop_last_state'] == 'undone':
		extra = ', style = filled, color=red,'
	    elif self.a['_repair_loop_last_state'] == 'redone':
		extra = ', style = filled, color=yellow,'
	    else:
		print 'XXX unknown repair state', self.a['_repair_loop_last_state']
        elif 'attacker' in self.a:
            extra = ', style = filled, color=green,'
        else:
            extra = ','
        if 'attacker' in self.a: del self.a['attacker']
        if self.t == 'process' or self.t == 'call':
            f.write(q(self.n) + '[shape=box' + extra + 'label=' + dump_label(self.n, self.a)+'];\n')
        elif self.t == 'file' or self.t == 'mgr':
            f.write(q(self.n) + '[shape=ellipse' + extra + 'label=' + dump_label(self.n, self.a)+'];\n')
        else:
            print "dump: unknown type"
            exit(-1)
    def undomgr(self):
	mgrname = self.a['_undomgr']
	return undo_managers[mgrname]

class Graph:
    def __init__(self):
        self.g = {}
    def present(self, l):
        return self.g.has_key(l)
    def labels(self):
        return self.g.keys()
    def node(self, label):
        return self.g[label][0]
    def forward(self, label):
        return self.g[label][1]
    def backward(self, label):
         return self.g[label][2]
    def add_node(self, v):
        self.g[v.label()] = [v, [],[]]
    def add_biedge(self, e):
        self.g[e.src()][1].append(e)
        self.g[e.dst()][2].append(e)
    def add_uedge(self, e):
        self.g[e.src()][1].append(e)
    def display(self):
        for l in self.labels():
            n = self.node(l)
            n.display()
            for e in self.forward(l):
                e.display()
    def dump(self, fname):
        f = open(fname, 'w')
        f.write('digraph G {\n')

	## Exclude unconnected nodes (e.g. parents of delegated files)
	for l in self.labels():
	    if len(self.forward(l)) == 0 and len(self.backward(l)) == 0:
		n = self.node(l)
		n.add_attr('exclude')

	## Bring back any excluded files with dependencies to them
	progress = True
	while progress:
	    progress = False
	    for l in self.labels():
		for e in self.forward(l):
		    ns = self.node(e.src())
		    nd = self.node(e.dst())
		    if not ns.skip() and 'exclude' in nd.get_attr():
			del nd.get_attr()['exclude']
			progress = True

        for l in self.labels():
            n = self.node(l)
            n.dump(f)
        for l in self.labels():
            for e in self.forward(l):
                n = self.node(e.src())
                if not n.skip():
                    e.dump(f)
        f.write('}\n')
        f.close()
    def markforward(self, l, time, a, visited):
        if l in visited:
            return
        visited.append(l)
        n = self.node(l)
        n.add_attr(a)
        for e in self.forward(l):
            if e.time() >= time:
                e.add_attr(a)
                self.markforward(e.dst(), time, a, visited)

## g:    original graph
## bg:   new backtracked graph
## l:    target node to backtrack to
## time: version of target node (i.e. line number) that's relevant
def backtrack(g, bg, l, time):
    if bg.present(l):
        return bg
    bg.add_node(g.node(l))
    for e in g.backward(l):
        if e.time() <= time:
            bg = backtrack(g, bg, e.src(), e.time())
            ## XXX why the reverse?
            ##bg.add_uedge(Edge(l, e.src(), e.time()))
            bg.add_uedge(e)
    return bg

def process_obj(g, pid, gen):
    pname = "%d.%d" % (pid, gen)
    if g.present(pname):
        p = g.node(pname)
    else:
        p = Node('process', pname)
	p.a['_pid'] = pid
	p.a['_pidgen'] = gen
	p.a['_undomgr'] = 'procmgr'
	g.add_node(p)
    return p

def call_obj(g, h, pid):
    cname = "%d/%d" % (pid, h)
    if g.present(cname):
	c = g.node(cname)
    else:
	e = history[h]
	undo_mgr = e.args[0].data.rstrip('\x00')
	funcname = e.args[1].data.rstrip('\x00')
	funcargs = e.args[3].data

	c = Node('call', cname)
	c.a['_pid'] = pid
	c.a['_undomgr'] = undo_mgr
	c.a['_funcname'] = funcname
	c.a['_funcargs'] = funcargs
	c.add_attr(undo_mgr + '/' + funcname)
	g.add_node(c)
    return c

def mgr_obj(g, fstat, subname, mgrname):
    mname = "%s:%s" % (fileid(fstat), subname)
    if g.present(mname):
	m = g.node(mname)
    else:
	m = Node('mgr', mname)
	g.add_node(m)
	m.a['_ino'] = fstat.ino
	m.a['_dev'] = fstat.dev
	m.a['_gen'] = fstat.gen
	m.a['_subname'] = subname
	m.a['_undomgr'] = mgrname

	f = file_obj(g, fstat)
	if not f.a.has_key('_sub_objects'): f.a['_sub_objects'] = set()
	f.a['_sub_objects'].add(mname)
	m.a['_parent_object'] = f.label()

	# copy filenames from file obj
	for k, v in f.a.iteritems():
	    if not k.startswith('_') and not k in m.a:
		m.a[k] = v
    return m

def file_obj(g, fstat):
    fid = fileid(fstat)
    if g.present(fid):
        f = g.node(fid)
    else:
        f = Node('file', fid)
	g.add_node(f)
	f.a['_ino'] = fstat.ino
	f.a['_dev'] = fstat.dev
	f.a['_gen'] = fstat.gen
	if 'mode' in dir(fstat): f.a['_mode'] = fstat.mode
	if 'rdev' in dir(fstat): f.a['_rdev'] = fstat.rdev
	if 'ptsid' in dir(fstat): f.a['_ptsid'] = fstat.ptsid
	f.a['_undomgr'] = 'fsmgr'
    return f

def update_file_attr(n, pname):
    n.add_attr(pname)
    if exclude_name(pname) == True:
        n.add_attr('exclude')

    ## guess undo manager for file based on pathname
    if pname == '/var/log/lastlog':
	n.a['_guess_mgr'] = 'offmgr'
	n.a['_block_length'] = 292
    elif pname in ['/var/run/utmp', '/var/log/wtmp']:
	n.a['_guess_mgr'] = 'offmgr'
	n.a['_block_length'] = 384
    elif pname.startswith('/dev/pts/') or n.n.startswith('cdev:128:'):
	n.a['_guess_mgr'] = 'ptsmgr'
	n.a['_pts_name'] = pname

def name_obj(g, dirstat, fname, pname):
    n = mgr_obj(g, dirstat, fname, 'dirmgr')
    n.add_attr(pname)
    if exclude_name(pname):
        n.add_attr('exclude')
    return n

def fileid(statinfo):
    if 'mode' in dir(statinfo):
	if stat.S_ISCHR(statinfo.mode):
	    ## /dev/ptmx special case
	    if statinfo.rdev == 0x0502 or os.major(statinfo.rdev) == 136:
		## master-side pty's are really major 128;
		## another option is to name master & slave ends the same,
		## causing dependency propagation (slaves are major 136).
		## we will handle pty output specially, though, so we don't
		## need this dependency propagation.
		return "cdev:128:%d" % (statinfo.ptsid)
	    else:
		return "cdev:%d:%d" % (os.major(statinfo.rdev), os.minor(statinfo.rdev))
    return "%d:%d:%d" % (statinfo.dev, statinfo.ino, statinfo.gen)

# identify objects involved in this event, and perhaps return dependency
#
# XXX it would be good to return Edge objects instead of src->dst pairs,
# so that we can stick interesting info into Edge attributes.
def objs(g, h):
    e = history[h]
    r = []
    p = actors[h][-1]

    ## For future reference: how to deal with 32-bit or 64-bit int in syscall arg data:
    ##	    struct.unpack("i", e.args[0].data[0:4])[0]
    ##	    struct.unpack("q", e.args[0].data[0:8])[0]

    # Find any namei information used by this system call
    if e.enter == True:
	for a in e.args:
	    if a.namei:
		for n in a.namei.ni:
		    if n.name != '':
			nobj = name_obj(g, n, n.name, a.data.rstrip('\x00'))
			r.append([nobj, p])

    if e.scname in ['write', 'pwrite64'] and e.enter == False:
	if e.args[0].fd_stat is None:
	    return []
	f = file_obj(g, e.args[0].fd_stat)
	offset = e.args[0].fd_stat.fd_offset
	if e.scname.startswith('p'):
	    offset = struct.unpack("q", e.args[3].data[0:8])[0]
	if f.a.get('_guess_mgr', None) == 'offmgr' and \
	   ((e.args[0].fd_stat.fd_flags & os.O_APPEND) == 0) and \
	   (offset % f.a['_block_length']) == 0 and \
	   (e.ret % f.a['_block_length']) == 0:
	    foff = mgr_obj(g, e.args[0].fd_stat,
			   str(offset - f.a['_block_length']),
			   f.a['_guess_mgr'])
	    r.append([p, foff])
	elif f.a.get('_guess_mgr', None) == 'ptsmgr':
	    fmgr = mgr_obj(g, e.args[0].fd_stat, 'write', 'ptsmgr')
	    r.append([p, fmgr])
	else:
	    r.append([p, f])
    elif e.scname in ['read', 'pread64'] and e.enter == False:
	if e.args[0].fd_stat != None:
	    f = file_obj(g, e.args[0].fd_stat)
	    offset = e.args[0].fd_stat.fd_offset
	    if e.scname.startswith('p'):
		offset = struct.unpack("q", e.args[3].data[0:8])[0]
	    if f.a.get('_guess_mgr', None) == 'offmgr' and \
	       (offset % f.a['_block_length']) == 0 and \
	       (e.ret % f.a['_block_length']) == 0:
		foff = mgr_obj(g, e.args[0].fd_stat,
			       str(offset - f.a['_block_length']),
			       f.a['_guess_mgr'])
		r.append([foff, p])
	    elif f.a.get('_guess_mgr', None) == 'ptsmgr':
		fmgr = mgr_obj(g, e.args[0].fd_stat, 'read', 'ptsmgr')
		r.append([fmgr, p])
	    else:
		r.append([f, p])
#     elif e.scname == 'stat' and e.enter == False:
#         XXX old:
#         [fd, fid] = fileids1(e[5])
#         f = file_obj(g, fid)
#         r.append([f, p])
    elif e.scname == 'clone' and e.enter == False and e.ret >= 0:
        p2 = process_obj(g, e.ret, 0)
        # in the case that logs are trucated, better to name it as anonymous
        p2.a['_current_exec'] = p.a.get('_current_exec', "anonymous")
	p2.add_attr(p2.a['_current_exec'])
        r.append([p, p2])
    elif e.scname == 'open' and e.enter == False and e.ret >= 0:
	pn = e.args[0].data.rstrip('\x00')
        ## update the file name for a new file
        f = file_obj(g, e.ret_fd_stat)
        update_file_attr(f, pn)

	## to figure out if O_CREAT happened, need to look at enter record
	e2 = history[h-1]
	for hid in range(h-1,-1,-1):
	    e2 = history[hid]
	    if (e2.pid == e.pid): break

        ## in the case that logs are trucated, possible not to find the entity
        if e2.pid != e.pid :
          logging.debug("e2.pid != e.pid")
          return r

	## add a dependency p->fname with O_CREAT when file didn't exist
	flags = struct.unpack("i", e2.args[1].data[0:4])[0]
	if flags & os.O_CREAT and e2.args[0].namei.ni[-1].name != '':
	    nobj = name_obj(g, e2.args[0].namei.ni[-1], e2.args[0].namei.ni[-1].name, pn)
	    r.append([p, nobj])
	if flags & os.O_TRUNC:
	    r.append([p, f])

    elif e.scname == 'execve' and e.enter == True:
	f = file_obj(g, e.args[0].namei.ni[-1])
	p2 = process_obj(g, p.a['_pid'], p.a['_pidgen'] + 1)

	r.append([f, p2])
	update_file_attr(f, e.args[0].data.rstrip('\x00'))

	r.append([p, p2])
        p2.add_attr(e.args[0].data.rstrip('\x00'))
        p2.a['_current_exec'] = e.args[0].data.rstrip('\x00')
    elif e.scname == 'wait4' and e.enter == False and e.ret >= 0:
	for h2 in range(h-1,-1,-1):
	    e2 = history[h2]
	    if e2.pid == e.ret:
		p2 = actors[h2][0]
		r.append([p2, p])
		break
    elif e.scname in ['link', 'linkat'] and e.enter == True:
	if e.scname.endswith('at'):
	    a = e.args[2]
	else:
	    a = e.args[1]
	idx = -1
	if a.namei.ni[idx].name == '': idx = -2
	n = name_obj(g, a.namei.ni[idx], a.namei.ni[idx].name, a.data.rstrip('\x00'))
	r.append([p, n])
    elif e.scname in ['unlink', 'unlinkat', 'mkdir', 'mkdirat', 'rmdir', \
                      'rename', 'renameat'] and e.enter == True:
	if e.scname.endswith('at'):
	    a = e.args[1]
	else:
	    a = e.args[0]
	idx = -1
	if a.namei.ni[idx].name == '': idx = -2
	n = name_obj(g, a.namei.ni[idx], a.namei.ni[idx].name, a.data.rstrip('\x00'))
	r.append([p, n])
	# XXX: add dependency for newname?
	if e.scname.startswith('rename'):
	    if e.scname.endswith('at'):
		a = e.args[3]
	    else:
		a = e.args[1]
	    idx = -1
	    if a.namei.ni[idx].name == '': idx = -2
	    n = name_obj(g, a.namei.ni[idx], a.namei.ni[idx].name, a.data.rstrip('\x00'))
	    r.append([p, n])

    elif e.scname == 'undo_func_start' and e.enter == True:
	c = call_obj(g, h, e.pid)
	r.append([p, c])
    elif e.scname == 'undo_func_end' and e.enter == True:
	p2 = actors[h][-2]
	r.append([p, p2])
    elif e.scname == 'undo_depend' and e.enter == True:
	mgr = e.args[2].data.rstrip('\x00')
	n = mgr_obj(g, e.args[0].fd_stat, e.args[1].data.rstrip('\x00'), mgr)
	if struct.unpack("i", e.args[3].data[0:4])[0] != 0:
	    r.append([p, n])
	if struct.unpack("i", e.args[4].data[0:4])[0] != 0:
	    r.append([n, p])

    ## XXX handle rename

    for mobj in maskset[h]:
	r2 = []
	for dp in r:
	    if dp[0] == mobj or dp[1] == mobj:
		pass
	    else:
		r2.append(dp)
	r = r2

    ## Fill in dependencies on sub-objects
    r2 = []
    for dp in r:
	dp0subs = [g.node(x) for x in list(dp[0].a.get('_sub_objects', []))]
	dp0subs.append(dp[0])
	dp1subs = [g.node(x) for x in list(dp[1].a.get('_sub_objects', []))]
	dp1subs.append(dp[1])

	for a in dp0subs:
	    for b in dp1subs:
		r2.append([a,b])
    r = r2

    return r

# read in events into history
infile = open(opt.logfile, "r")
buf = infile.read()
infile.close()

unpacker = xdrlib.Unpacker(buf)
while True:
    try:
        l = record.syscall_record.unpack(unpacker)
        history.append(l)
    except EOFError:
        break

#print "History contains", len(history), "events"

# complete graph
g = Graph()

# process delegations and other generated info
mask_set    = {}
actor_stack = {}
pid_gen     = {}

for h in range(0, len(history)-1):
    e = history[h]
    pid = e.pid

    if not mask_set.has_key(pid): mask_set[pid] = []
    if not pid_gen.has_key(pid): pid_gen[pid] = 0
    if not actor_stack.has_key(pid): actor_stack[pid] = [process_obj(g, pid, pid_gen[pid])]

    maskset.append(set(mask_set[pid]))
    actors.append(list(actor_stack[pid]))
    pidgen.append(pid_gen[pid])

    if e.enter == True:
	if e.scname == 'undo_mask_start':
	    f = file_obj(g, e.args[0].fd_stat)
	    mask_set[pid].append(f)
	elif e.scname == 'undo_mask_end':
	    f = file_obj(g, e.args[0].fd_stat)
	    mask_set[pid].remove(f)
	elif e.scname == 'undo_func_start':
	    c = call_obj(g, h, pid)
	    actor_stack[pid].append(c)
	elif e.scname == 'undo_func_end':
	    actor_stack[pid].pop()
	elif e.scname == 'execve':
	    pid_gen[pid] = pid_gen[pid] + 1
	    actor_stack[pid] = [process_obj(g, pid, pid_gen[pid])]

# add all events to graph
for h in range(0, len(history)-1):
    l = objs(g, h)
    for e in l:
        [s, t] = e
        if s == None or t == None:
            continue
        if g.present(t.label()) == False:
            g.add_node(t)
        if g.present(s.label()) == False:
            g.add_node(s)
        g.add_biedge(Edge(s.label(), t.label(), h))

def find_exec(g, pid):
    r = None
    for h, e in enumerate(history):
        if e.scname == 'execve' and e.enter == True and e.pid == pid:
            r = h
        if e.scname == 'execve' and e.enter == False and e.pid == pid:
            assert(r != None)
            #print(r, history[r])
            return r
    raise ValueError('cannot find exec for pid ' + str(pid))

def forward(g, line):
    l = objs(g, line)
    for e in l:
        [s,t] = e
        g.markforward(t.label(), line, 'attacker', [])

class UndoManager:
    def __init__(self, g):
	self.g = g
	self.really_repair = not opt.dryrun

    def undo(self, e, n):
	raise Exception("not implemented")

    def redo(self, e, n):
	raise Exception("not implemented")

    ## Optional for both actors and objects.  Return False if not sure.
    def edge_same(self, e, ns, nd):
	return False

    ## Update attributes for all direct actor descendants as having
    ## been re-executed, and all direct edges as having been repaired.
    ## XXX maybe need a way to avoid re-running children during re-execution?
    ## XXX what if rerun didn't produce some edges, or produced new edges?
    ##     need to nullify edges that didn't come up during rerun, and add
    ##     new edges to the graph.
    ## XXX also need to keep a mapping between equivalent files (inodes)
    ##     in original execution and in re-execution.
    def mark_actor_children(self, n, ts, exclude):
	n.a['_repair_state'] = 'reexecuted'
	logging.debug("mark %s @%d as %s, exclude %s", n, ts,
	              n.a['_repair_state'], [str(x) for x in exclude])
	for e in self.g.forward(n.label()):
	    if e.time() < ts: continue

	    h = history[e.time()]
	    if h.scname != 'wait4' and h.scname != 'undo_func_end':
		e.a['_repair_state'] = 'redone'
		logging.debug("mark %s as %s", e, e.a['_repair_state'])

	    dst = self.g.node(e.dst())
	    if dst.label() in exclude: continue

	    if (dst.t == 'process' and h.scname == 'clone') or \
	       (dst.t == 'process' and h.scname == 'execve') or \
	       (dst.t == 'call' and h.scname == 'undo_func_start'):
		self.mark_actor_children(dst, e.time(), exclude)

class FsUndoManager(UndoManager):
  
    #
    # generate snapshot file at cur time & return its name if it exists
    # 
    def create_snapshot(self, snap_id, cur, nattr) :
        snap_root = "/mnt/undofs/snap/"

        # 
        # this snapshot file should be re-generated by executing
        # subsequent system calls in record log
        # 
        snapfile = snap_root + self.snap_id_to_str(snap_id)
        
        print "[!] request : %s(cur:%d)" % (snapfile, cur)
        
        # 
        # but we are only supporting write/open
        #
        # non-existance :
        #  1) by our snapshot optimization      : have to generate
        #  2) really non exist when open/unlink : simply return
        # 
        if os.path.exists( snapfile ) :
            return snapfile

        # 
        # move right before the snap_id is taken
        # 
        parent = ""
        for t in range(cur-1,-1,-1):
            # 
            # NOTE.
            #  - inode was changed, but path should be same at this moment)
            #  - search root shapshot directory, /mnt/undofs/snap/d
            # 
            if t == 0 :
                # lookup inode in /mnt/undofs/d
                d = self.find_ino( "/mnt/undofs/d", nattr['_ino'], nattr['_gen'] )
                
                # not exist
                if d == None :
                  return None

                print "[!] found, %s(%d) in the directory" % (d, nattr['_ino'])
                parent = "/mnt/undofs/snap/d" + d[len("/mnt/undofs/d"):]

                # 
                # NOTE.
                #  open -> create this inode, but we can't match it
                #  since we are taking a snapshot right before executing the syscalls
                #
                
                # create empty (it must be open syscall)
                if not os.path.exists(parent):
                    open(snapfile, "w").close()
                    print "[!] creating %s (<=%s) : empty" % (snapfile, parent)
                    return snapfile

                # if exist copy from the first snapshot!
		try :
		    shutil.copyfile(parent, snapfile)
                    print "[!] copied %s => %s" % (parent, snapfile)
		except IOError :
		    # ok, it might be due to the open syscall on empty file
		    pass
                
                return d

            h = history[t]
            if h.snap.file_stat is not None and \
                   (h.snap.file_stat.dev == nattr['_dev'] and \
                    h.snap.file_stat.ino == nattr['_ino'] and \
                    h.snap.file_stat.gen == nattr['_gen']):

		parent = snap_root + self.snap_id_to_str(h.snap.id)
		if not os.path.exists(parent) :
		    self.create_snapshot(h.snap.id, t, nattr)

		# copy parent file
		try :
		    shutil.copyfile(parent, snapfile)
		except IOError :
		    # ok, it might be due to the open syscall on empty file
		    pass

                def dump_h(h) :
                  if h.scname == "write" :
                      return h.args[1].data[0:4].replace("\n","") + ".."
                  elif history[t].scname == "open" :
                      return h.args[0].data
                  elif history[t].scname == "unlink" :
                      return h.args[0].data
                  elif history[t].scname == "unlinkat" :
                      return h.args[1].data
                  return ""
                
		# subsequent system call ex) write/read/lseek ...
		print "[!] creating %s (<=%s) by %s(%s) (cur:%d)" \
		    % (snapfile, parent, history[t].scname, dump_h(history[t]), t)

		# simulate the equivalent operations
		#
		# open/write/unlink read +)seek
		# 

		if h.scname == "open" :
		    # if creat/truncate -> make it empty file
		    flags = struct.unpack("i", h.args[1].data[0:4])[0]
		    if flags & os.O_TRUNC :
                        print "[!] trucating file %s (flags:%d)" % (snapfile, flags)
                        open( snapfile, "w" ).close()
                      
                elif h.scname == "write" :
                    # 
                    # look up exiting entry of write
                    # 
                    for write_h in history[t+1:] :
                        if write_h.pid == h.pid and \
                              write_h.scname == "write" and \
                              not write_h.enter and \
                              write_h.args[0].fd_stat.ino == h.snap.file_stat.ino and \
                              write_h.args[0].fd_stat.dev == h.snap.file_stat.dev and \
                              write_h.args[0].fd_stat.gen == h.snap.file_stat.gen :

                          #
                          # XXX need to deal with flags
                          #
                          # since this is the context after calling write syscall
                          # 
                          fd = open( snapfile, "a" )
                          fd.seek( write_h.args[0].fd_stat.fd_offset - write_h.ret )
                          fd.write( h.args[1].data )
                          fd.close()
                          break
                        
                elif h.scname == "unlink" or h.scname == "unlinkat" :
                    # removing copied file
                    open( snapfile, "w" ).close()

                return snapfile
    
    def snap_id_to_str(self, snap_id):
	snap_pid = (snap_id >> 32)
	snap_ctr = snap_id & ((1 << 32) - 1)
	return "%d.%d" % (snap_pid, snap_ctr)

    def find_ino(self, d, ino, gen):
	for (dirpath, dirnames, filenames) in os.walk(d):
	    for n in set(dirnames).union(filenames).union(['.']):
	    	pn = dirpath
		if n != '.': pn = pn + '/' + n
		try:
		    s = os.stat(pn)
		    if s.st_ino == ino:		## XXX check gen# later?
			return pn
		except:
		    pass
	return None

    def find_ino_cur(self, nattr):
	return self.find_ino("/mnt/undofs/d", nattr['_ino'], nattr['_gen'])

    def find_stat_cur(self, ni):
	return self.find_ino("/mnt/undofs/d", ni.ino, ni.gen)

    def find_ino_ever(self, nattr):
	pn = self.find_ino_cur(nattr)
	if pn is None:
	    pn = self.find_ino("/mnt/undofs/snap", nattr['_ino'], nattr['_gen'])
	return pn

    def find_snap(self, e):
	h = history[e.time()]
	if h.snap.id != 0: return (h,e.time())
	if not h.enter:
	    for t in range(e.time()-1,-1,-1):
                h2 = history[t]
		if h2.pid == h.pid:
		    if h2.snap.id != 0: return (h2,t)
		    break
	raise Exception('no snapshot for edge ' + e.tostr())

    def find_ino_snap(self, nattr, e):
	try:
	    (h,t) = self.find_snap(e)
	except:
	    return None

	if h.snap.file_stat is None or \
	   (h.snap.file_stat.dev == nattr['_dev'] and \
	    h.snap.file_stat.ino == nattr['_ino'] and \
	    h.snap.file_stat.gen == nattr['_gen']):
            return self.create_snapshot(h.snap.id, t, nattr)
        
	raise Exception('mismatched snapshot at', e.time(),
			nattr, h.snap.file_stat)

    def find_ino_snap_after(self, nattr, e):
        for t in range(e.time()+1,len(history)):
            h = history[t]
	    if h.snap.file_stat is not None and \
	       (h.snap.file_stat.dev == nattr['_dev'] and \
	        h.snap.file_stat.ino == nattr['_ino'] and \
	        h.snap.file_stat.gen == nattr['_gen']):
              return self.create_snapshot(h.snap.id, t, nattr)

	return self.find_ino_cur(nattr)

    def find_dir_snap(self, nattr, e):
	h = history[e.time()]
	for di in h.snap.dirents:
	    if di.dev == nattr['_dev'] and \
	       di.ino == nattr['_ino'] and \
	       di.gen == nattr['_gen']:
		return di
	## XXX search in history
	return None #assert(0)

    def fs_ignore_node(self, n):
	## Ignore some dependencies, at least for now.
	if n.a['_undomgr'] != 'fsmgr': return False
	if n.a['_dev'] == 3: return True	## procfs
	return False

    def edge_same(self, e, ns, nd):
	if ugly_hacks:
	    if ns.a['_undomgr'] == 'fsmgr' and '/root/.bash_history' in ns.a:
		return True
	    if ('/etc/passwd' in ns.a or '/etc/group' in ns.a) and '/bin/tar' in nd.a:
		logging.warn("hack, %e, %s, %s", e, ns, nd)
		return True
	return self.fs_ignore_node(ns) or self.fs_ignore_node(nd)

    def undo(self, e, n):
	## We may need a better plan to keep track of what the current
	## version of this node (file) really contains.  Right now,
	## if we undo by rolling back and re-executing later ops,
	## we keep track of the version to which we rolled back, so
	## undoing subsequent writes is a no-op (but, need to make sure
	## those subsequent writes are not passed to redo!).

	attr = n.get_attr()
	if '_cur_ver' in attr and attr['_cur_ver'] <= e.time():
	    ## File is already rolled back to before this time
	    logging.debug("skip: file %s already rolled back to earlier @%s", n, attr['_cur_ver'])
	    return

	if n.a.has_key('_mode') and stat.S_ISDIR(n.a['_mode']):
	    raise Exception('direct writes to directory not supported')

	## Don't do anything for pipes and sockets, for now.
	## May need compensation actions for AF_INET sockets.
	## If re-executed, the entire pipeline will be re-executed.
	if n.a.has_key('_mode') and not stat.S_ISREG(n.a['_mode']):
	    return

	cur_pn = self.find_ino_cur(n.a)
	snap_pn = self.find_ino_snap(n.a, e)

	logging.info("roll back file %s to @%s", n, e.time())

	if cur_pn is None:
	    cur_pn = ""
	if snap_pn is None:
	    if self.really_repair and os.path.exists(cur_pn):
		# so just delete it
		logging.info("removing current file %s", n)
		st = os.stat(cur_pn)
		if not cur_pn.startswith('/mnt/undofs/d/proc/'):
		    os.rename(cur_pn, "/mnt/undofs/snap/%d_%d_%d" % (st.st_dev, st.st_ino, 0))
		#os.unlink(cur_pn)
		cur_pn = ""
	    snap_pn = ""

	## Find all subsequent writes to the same object
	reruns = filter(lambda e2: e2.time() > e.time(),
			self.g.backward(n.label()))
	'''
	## Try clever things, if the alternative is re-execution
	## XXX maybe this code should be in a TextFileUndoManager,
	##     and we should explicitly mark files as text files.
	h = history[e.time()]
	if h.scname == "write" and len(reruns) > 0:
	    ## Try a text diff
	    snap_after_pn = self.find_ino_snap_after(n.a, e)
	    if snap_after_pn != None:
	        plen = len("/mnt/undofs/snap/")
	        logging.debug("applying (%s - %s) to %s",
		        snap_pn[plen:], snap_after_pn[plen:], n)
	        if "/var/log/wtmp" in n.a:
		    p = patch.RecordPatch(snap_after_pn, snap_pn, utmp.utmp)
		else:
		    p = patch.TextPatch(snap_after_pn, snap_pn)
		if p.dry_run(cur_pn):
		    ## XXX patching changes the inode of a file
		    if self.really_repair: p.run(cur_pn)
		    logging.info("file %s patched", n)
		    return
	'''

	# the file content is restored only if the file exists both now and past
	# * if it exists now but not in the past, we just deleted it
	# * if it exists in the past but not now, DirUndoMgr should recover it
	if self.really_repair and os.path.exists(snap_pn) and os.path.exists(cur_pn):
	    shutil.copyfile(snap_pn, cur_pn)
	    logging.info("file %s restored", n)
	    statistics['fs_undo'].add((e, n))
	attr['_cur_ver'] = e.time()
	return reruns

    def redo(self, e, n):
	if n.a.has_key('_mode') and stat.S_ISDIR(n.a['_mode']):
	    raise Exception('direct writes to directory not supported')
	if n.a.has_key('_mode') and not stat.S_ISREG(n.a['_mode']):
	    return
	
	cur_pn = self.find_ino_cur(n.a)
	if cur_pn is None:
	    logging.error("cannot find file %s", n)
	    return

	logging.info("re-writing to file %s", n)

	h = history[e.time()]
	if h.scname == 'write':
	    if self.really_repair:
		f = open(cur_pn, 'r+')
		if (h.args[0].fd_stat.fd_flags & os.O_APPEND):
		    f.seek(0, os.SEEK_END)
		else:
		    f.seek(h.args[0].fd_stat.fd_offset - h.ret)
		f.write(h.args[1].data[0:h.ret])
		f.close()
		statistics['fs_redo'].add((e, n))
	else:
	    logging.error("XXX unknown syscall %s", h.scname)

class ProcessUndoManager(UndoManager):
    def exec_edge(self, n, ts):
	for e in self.g.backward(n.label()):
	    if e.time() > ts: continue
	    h = history[e.time()]
	    if h.scname == 'execve':
		return e
	    if h.scname == 'clone':
		return self.exec_edge(self.g.node(e.src()), ts)
	logging.error('XXX none found for %s @%s', n, ts)
	assert(False)

    def undo(self, e, n):
	if n.a.get('_repair_state', None) == 'dead':
	    return

	src = self.g.node(e.src())

	h = history[e.time()]
	if h.scname == 'clone' or h.scname == 'execve':
	    n.a['_repair_state'] = 'dead'
	    for e2 in self.g.forward(n.label()):
		if e2.time() <= e.time(): continue
		e2.a['_repair'] = 'nullify'
	elif h.scname == 'wait4' and src.a.get('_repair_state', None) == 'dead':
	    pass
	else:
	    ## can't undo arbitrary edges; return a start edge for redo
	    return [self.exec_edge(n, e.time())]

    def sub_exec_edges(self, n, ts):
	r = []
	for e in self.g.forward(n.label()):
	    if e.time() < ts: continue
	    h = history[e.time()]
	    if h.scname == 'clone':
		r.extend(self.sub_exec_edges(self.g.node(e.dst()), ts))
	    elif h.scname == 'execve':
		r.append(e)
		r.extend(self.sub_exec_edges(self.g.node(e.dst()), ts))
	return r

    def sub_edges(self, n, ts):
	r = []
	for e in self.g.forward(n.label()):
	    if e.time() < ts: continue
	    h = history[e.time()]
	    if h.scname in ['clone', 'execve']:
		r.extend(self.sub_edges(self.g.node(e.dst()), ts))
		r.append(e)
	return r

    @staticmethod
    def dupfd(si, fd2):
	if not stat.S_ISREG(si.mode):
	    return

	pn = undo_managers['fsmgr'].find_stat_cur(si)
	if pn is None:
	    return

	fd = os.open(pn, si.fd_flags)
	os.lseek(fd, si.fd_offset, 0)
	# ugly fix: flags like O_CREAT and O_TRUNC are missing
	if (si.fd_flags & os.O_WRONLY) and (not si.fd_flags & os.O_APPEND):
	    os.ftruncate(fd, si.fd_offset)
	os.dup2(fd, fd2)
	os.close(fd)

    @staticmethod
    def redoable(e):
	h = history[e.time()]
	if h.scname != 'execve':
	    return False
	cmd = h.args[0].data.strip('\x00')
	# interactive shells
	if cmd in ['/bin/login']:
	    return False
	if re.match('/bin/[a-z]*sh', cmd) and len(h.args[1].argv) <= 1:
	    logging.debug ("skip %s (%s), not redoable", e, cmd)
	    return False
	return True

    def redo(self, e, n):
	if n.a.get('_repair_state', None) is not None:
	    ## 'reexecuted' or 'dead'
	    ## XXX keep track of versions of inputs, and maybe need to
	    ## re-execute or do something if inputs changed a second time?
	    return

	if not self.redoable(e):
	    ## can't undo arbitrary edges; return a start edge for redo
	    return [self.exec_edge(n, e.time())]

	exec_h = e.time()
	argv = []
	for s in history[exec_h].args[1].argv:
	    argv.append(s.s)

	envp = {}
	for s in history[exec_h].args[2].argv:
	    [k, blah, v] = s.s.partition('=')
	    envp[k] = v

	logging.info("re-executing %s @%d, argv %s", n, exec_h, argv)
	#print "envp", str(envp)

	## The wait status goes to the last exec'ed proc object with same PID
	last_proc_obj = process_obj(self.g, n.a['_pid'], pid_gen[n.a['_pid']])

	## XXX fold pts handling support into strace-based re-execution
	## otherwise we will have a deadlock (see below -- reading from
	## the pty master and driving strace at the same time)

	## See if there were any pts writes by this process, and if so,
	## create a new pts for this process to use.  Really, this should
	## be recorded in the exec record (what FDs were passed on), but
	## this is a workaround for now.
	pts_fd = None
	for e2 in self.g.forward(n.label()):
	    d = self.g.node(e2.dst())
	    if d.a['_undomgr'] != 'ptsmgr': continue
	    h = history[e2.time()]
	    if h.scname != 'write': continue

	    pts_fd = struct.unpack('i', h.args[0].data[0:4])[0]
	    pts_e  = e2

	    ## just one pts FD for now, although in general we should
	    ## check if there are multiple ones..
	    break

	ni = history[exec_h].args[0].namei.ni[-1]
	exec_pn = undo_managers['fsmgr'].find_stat_cur(ni)
	if exec_pn and len(ni.name):
	    exec_pn = exec_pn + '/' + ni.name

	execi = history[exec_h].execi
	exec_cwd = undo_managers['fsmgr'].find_stat_cur(execi.cwd)
	exec_root = undo_managers['fsmgr'].find_stat_cur(execi.root)

	## if we ran binaries outside of our sandbox (e.g. /bin/cp)..
	if exec_pn is None:
	    exec_pn = history[exec_h].args[0].data.rstrip('\x00')

	if exec_root is None:
	    st = os.stat('/')
	    if st.st_dev == execi.root.dev and st.st_ino == execi.root.ino:
		exec_root = '/'

	if exec_pn is None:
	    logging.error('cannot find executable')
	if exec_cwd is None:
	    logging.error('cannot find cwd')
	if exec_root is None:
	    logging.error('cannot find root')

	if exec_root == '/mnt/undofs/d/dev/pts/ptmx':
	    exec_root = '/mnt/undofs/d'
	if exec_root != '/':
	    exec_pn = '/' + os.path.relpath(exec_pn, exec_root)

	if exec_pn == '/usr/sbin/useradd':
	    self.update_argv_useradd(argv, e)

	not_redone_children = []
	if self.really_repair:
	    if pts_fd:
		(nptm_fd, npts_fd) = os.openpty()

	    ## wrap execution in strace
	    argv = ['exec-ptrace', exec_root, exec_pn] + argv
	    exec_pn = os.getcwd() + '/exec-ptrace'
	    statistics['proc_redo'].add((e, n))

	    pid = os.fork()
	    if pid == 0:
	        logging.debug("change cwd to %s", exec_cwd)
		os.chdir(exec_cwd)
		for fdnum, si in enumerate(history[exec_h].execi.fds):
		    self.dupfd(si, fdnum)

		if pts_fd:
		    os.dup2(npts_fd, pts_fd)
		    os.close(npts_fd)
		    os.close(nptm_fd)

		logging.debug("execve %s with %s", exec_pn, argv)
		os.execve(exec_pn, argv, envp)
		assert(0)
		os._exit(-1)

	    if pts_fd:
		os.close(npts_fd)

		pts_e.a['_pts_write_data'] = ''
		while True:
		    try:
			s = os.read(nptm_fd, 1024)
		    except:
			break
		    if len(s) == 0: break
		    pts_e.a['_pts_write_data'] += s
		os.close(nptm_fd)

	    p = rerun.Process(pid)
	    first_exec = False
	    runmap = {}
	    outs = set()
	    descendants = self.sub_exec_edges(n, e.time())
	    while True:
		h = p.get_sysrec()
		if h == None: break

		if h.scname == 'execve' and first_exec == False:
		    first_exec = True
		    p.writecmd('c')
		    continue

		if h.scname == 'execve' and h.enter:
		    ## Find a likely candidate from our history
		    ## Really, our goal is to re-compute output edges
		    ## from this exec'ed process, and if we already
		    ## have an exec'ed process from history, with the
		    ## same input arguments, and the same file inputs
		    ## (at the "same" point in time), then we can just
		    ## use the outputs from history too.
		    for e2 in descendants:
		        h2 = history[e2.time()]
			if h2.args[0].data != h.args[0].data: continue
			if len(h2.args[1].argv) != len(h.args[1].argv): continue
			for i in range(0, len(h.args[1].argv)):
			    if h2.args[1].argv[i].s != h.args[1].argv[i].s: continue
			## XXX: check envp
			#break
			logging.info('matching exec at %s', e2)
			not_redone_children.append(e2.dst())
			runmap[h.pid] = e2.dst()
			p.writecmd('x')
			break

		if h.scname == 'wait4' and not h.enter:
		    logging.info('wait4 by %d, child %d', h.pid, h.ret)
		    if runmap.has_key(h.ret):
			n2 = runmap[h.ret]
			for e2 in self.g.forward(n2):
			    h2 = history[e2.time()]
			    if h2.scname != 'wait4': continue
			    logging.info('found wait4 from history %s', e2)
			    p.writearg(1, h2.args[1].data)

		if h.pid == pid:
		    self.update_outs(outs, h)

		p.writecmd('c')

	    [pid2, wstat] = os.waitpid(pid, 0)
	    logging.info("%s re-executed, wstat %x", n, wstat)
	    last_proc_obj.a['_repair_waitstat'] = wstat
	else:
	    last_proc_obj.a['_repair_waitstat'] = 0

	self.mark_actor_children(n, e.time(), not_redone_children)

	## mark_actor_children marked all edges as repaired already,
	## but we need to redo the pts write edge, if there is one.
	if pts_fd: del pts_e.a['_repair_state']

	sub_edges = []
	for n2 in not_redone_children:
	    sub_edges.extend(self.sub_edges(self.g.node(n2), e.time()))
	return self.mark_outs(n, e.time(), not_redone_children, sub_edges, outs)

    def edge_same(self, e, ns, nd):
	h = history[e.time()]
	if h.scname == 'wait4':
	    oldstat = struct.unpack("i", h.args[1].data[0:4])[0]
	    newstat = ns.a.get('_repair_waitstat', None)
	    logging.debug("%s, old stat %s, new stat %s, _repair_state %s",
	                  e, oldstat, newstat, ns.a.get('_repair_state', None))
	    return (oldstat == newstat) or \
		   (ns.a.get('_repair_state', None) == 'dead')
	return False

    def update_outs(self, outs, h):
	if h.enter == False and h.ret >= 0:
	    if h.scname in ['write', 'pwrite64']:
		outs.add(file_obj(self.g, h.args[0].fd_stat))
	    if h.scname in ['open', 'openat', 'creat']:
		outs.add(file_obj(self.g, h.ret_fd_stat))

    def mark_outs(self, n, ts, procs, sub_edges, outs, rerun = []):
	if '_mark_outs' in n.a:
	    return
	n.a['_mark_outs'] = ''
	if n.label() in procs:
	    n.a['_rerun_matched'] = ''
	    for e in sub_edges:
		for e2 in self.g.forward(e.dst()):
		    self.update_outs(outs, history[e2.time()])
	    for e in self.g.forward(n.label()):
		self.update_outs(outs, history[e.time()])
	for e in self.g.forward(n.label()):
	    if e.time() < ts: continue
	    dst = self.g.node(e.dst())
	    h = history[e.time()]

	    if (dst.t == 'process' and h.scname == 'clone') or \
	       (dst.t == 'process' and h.scname == 'execve') or \
	       (dst.t == 'call' and h.scname == 'undo_func_start'):
		self.mark_outs(dst, e.time(), procs, sub_edges, outs)

	    if '_rerun_matched' in dst.a:
		n.a['_rerun_matched'] = dst.a['_rerun_matched']

	    if (dst.t == 'file' and dst not in outs) or \
	       (dst.t == 'process' and '_rerun_matched' not in dst.a and e not in sub_edges and h.scname not in ['wait4', 'undo_func_end']):
		logging.debug("mark undo %s", e)
		e.a['_repair'] = 'nullify'
		if '_repair_state' in e.a:
		    del e.a['_repair_state']
		    logging.debug("mark %s as undo", e)
		rerun.append(e)
	return rerun

    def update_argv_useradd(self, argv, e):
	if '-u' in argv or '--uid' in argv:
	    return
	h0 = history[e.time()]
	assert h0.scname == 'execve'
	for t in range(len(history)-1, e.time(), -1):
	    h = history[t]
	    if h.pid != h0.pid: continue
	    if h.scname != 'rename' or not h.enter: continue
	    old_pn = None
	    new_pn = None
	    for ni in h.snap.dirents[0].ni:
		nattr = {'_dev':ni.dev, '_ino':ni.ino, '_gen':ni.gen}
		if ni.name == 'passwd+':
		    new_pn = undo_managers['fsmgr'].find_ino_ever(nattr)
		elif ni.name == 'passwd':
		    old_pn = undo_managers['fsmgr'].find_ino_ever(nattr)
	    if old_pn and new_pn:
		with open(old_pn, 'r') as f: old_lines = f.readlines()
		with open(new_pn, 'r') as f: new_lines = f.readlines()
		if len(old_lines) + 1 == len(new_lines):
		    line = new_lines[-1].split(':')
		    assert(argv[-1] == line[0]) # name
		    uid = line[2]
		    argv.insert(1, uid)
		    argv.insert(1, '-u')

class DirUndoManager(UndoManager):
    def undo(self, e, n):
	attr = n.get_attr()
	if '_cur_ver' in attr and attr['_cur_ver'] <= e.time():
	    ## Name is already rolled back to before this time
	    logging.debug("skip: name %s already rolled back to earlier @%s", n, attr['_cur_ver'])
	    return


	logging.info("roll back name %s to @%s", n, e.time())

	parent = self.g.node(attr['_parent_object'])
	cur_d = parent.undomgr().find_ino_cur(parent.a)
	snap_d = parent.undomgr().find_dir_snap(parent.a, e)

	if cur_d is None:
	    cur_pn = ""
	else:
	    cur_pn = cur_d + "/" + attr['_subname']
	if snap_d is None:
	    snap_ni = None
	else:
	    snap_ni = next((ni for ni in snap_d.ni if ni.name == attr['_subname']), None)

	## might need to be a little careful with things like rename
	if snap_ni == None:
	    if os.path.exists(cur_pn):
		logging.info("removing name %s (no snapshot found)", n)
		if self.really_repair:
		#    if stat.S_ISDIR(os.stat(cur_pn).st_mode):
		# 	os.rmdir(cur_pn)
		#    else:
		#	os.unlink(cur_pn)
		    st = os.stat(cur_pn)
		    os.rename(cur_pn, "/mnt/undofs/snap/%d_%d_%d" % (st.st_dev, st.st_ino, 0))
	else:
	    if os.path.exists(cur_pn):
		## Check if the name points to the same inode
		cur_st = os.stat(cur_pn)
		if cur_st.st_ino == snap_ni.ino: return	## XXX check gen
		logging.info("replacing name %s (unlink first)", n)
		#if stat.S_ISDIR(os.stat(cur_pn).st_mode):
		#    os.rmdir(cur_pn)
		#else:
		#    os.unlink(cur_pn)
		os.rename(cur_pn, "/mnt/undofs/snap/%d_%d_%d" % (cur_st.st_dev, cur_st.st_ino, 0))

	    logging.info("re-creating name %s", n);
	    statistics['dir_undo'].add((e, n));

	    ## Try to find the same file in the file system
	    cur_pn_link = parent.undomgr().find_ino_ever({'_ino': snap_ni.ino, '_gen': -1})
	    if cur_pn_link and self.really_repair:

                # alreay exist
                try :
                    os.link(cur_pn_link, cur_pn)
                except OSError :
                    pass
                  
		# test if when we should remove current file
		# don't remove snapshots
		if cur_pn_link.startswith("/mnt/undofs/d/"):
		    bn = os.path.basename(cur_pn_link)
		    if next((True for ni in snap_d.ni if ni.name == bn), None) == None:
			logging.info("removing name %s", bn)
			os.unlink(cur_pn_link)

	attr['_cur_ver'] = e.time()
	return filter(lambda e2: e2.time() > e.time(),
		      self.g.backward(n.label()))

    def redo(self, e, n):
	## As with KvUndoManager::redo, it's hard to know exactly what the
	## actor was doing.  We can guess some simple cases..
	attr = n.get_attr()
	h = history[e.time()]
	logging.info("re-%s dir %s", h.scname, n)

	parent = self.g.node(attr['_parent_object'])

	if h.scname == 'unlinkat':
	    cur_d = parent.undomgr().find_ino_cur(parent.a)

	    if cur_d is None:
		logging.warn("cannot find dir", n)
		return
	    idx = -1
	    if h.args[1].namei.ni[idx].name == '': idx = -2
	    if self.really_repair:
		#os.unlink(cur_d + '/' + e.args[1].namei.ni[idx].name)
		pn = cur_d + '/' + e.args[1].namei.ni[idx].name
		if os.path.exists(pn):
		    st = os.stat(pn)
		    os.rename(pn, "/mnt/undofs/snap/%d_%d_%d" % (st.st_dev, st.st_ino, 0))
		    statistics['dir_redo'].add((e, n))
	else:
	    logging.error("unknown op %s for directory %s", h.scname, e)

class PyUndoManager(UndoManager):
    def undo(self, e, n):
	h = history[e.time()]
	if h.scname == 'undo_func_start':
	    n.a['_repair_state'] = 'dead'
	    for e2 in self.g.forward(n.label()):
		if e2.time() <= e.time(): continue
		e2.a['_repair'] = 'nullify'
	elif h.scname == 'undo_func_end':
	    pass
	else:
	    print "XXX undoing non-start edge"

    def redo(self, e, n):
	if n.a.get('_repair_state', None) is not None:
	    return

	## XXX where to get the code from?
	## perhaps use inspect.getfile() or inspect.getsourcefile()
	path = sys.path
	sys.path.append('/mnt/undofs/d')
	mod = __import__('funcs')
	sys.path = path
	func = getattr(mod, n.a['_funcname'])
	args = pickle.loads(n.a['_funcargs'])

	ret = None
	print "redo: re-running python function", n.tostr()
	if self.really_repair:
	    ret = func(*args)

	self.mark_actor_children(n, e.time(), [])
	n.a['_repair_retval'] = pickle.dumps(ret)

    def edge_same(self, e, ns, nd):
	h = history[e.time()]
	if h.scname == 'undo_func_end':
	    oldret = h.args[1].data
	    newret = ns.a.get('_repair_retval', None)
	    return (oldret == newret)
	return False

class KvUndoManager(UndoManager):
    def undo(self, e, n):
	attr = n.get_attr()
	if '_cur_ver' in attr and attr['_cur_ver'] <= e.time():
	    return

	print "undo: roll back undomgr object", n.tostr(), "to time", e.time()
	parent = self.g.node(attr['_parent_object'])
	cur_pn = parent.undomgr().find_ino_cur(parent.a)
	snap_pn = parent.undomgr().find_ino_snap(parent.a, e)

	if cur_pn is None or snap_pn is None:
	    print "undo: cannot find file", n.tostr()
	    return

	if self.really_repair:
	    path = sys.path
	    sys.path.append('../test')
	    sys.path.append('../libundo')
	    kvmod = __import__('funcs')
	    sys.path = path

	    v = kvmod.kv_get(snap_pn, attr['_subname'])
	    kvmod.kv_set(cur_pn, attr['_subname'], v)

	attr['_cur_ver'] = e.time()
	return filter(lambda e2: e2.time() > e.time(),
		      self.g.backward(n.label()))

    def redo(self, e, n):
	## XXX this is a bit messy; we don't really know what the edge
	##     operation was, so here we assume it was an entire object
	##     write (so, we pick up the value that ended up being stored
	##     for this key from the next snapshot, and use it).  the risk
	##     is that the operation was a partial object write, and we
	##     inadvertently propagate some of attacker's data (which was
	##     in the next snapshot's value) in this redo operation.
	##
	##     really, it seems like this redo should be done by the actor,
	##     or we should have redo managers associated with edges?

	parent = self.g.node(n.a['_parent_object'])
	cur_pn = parent.undomgr().find_ino_cur(parent.a)
	snap2_pn = parent.undomgr().find_ino_snap_after(parent.a, e)

	if cur_pn is None or snap2_pn is None:
	    print "redo: cannot find file", n.tostr()
	    return

	if self.really_repair:
	    path = sys.path
	    sys.path.append('../test')
	    sys.path.append('../libundo')
	    kvmod = __import__('funcs')
	    sys.path = path

	    v = kvmod.kv_get(snap2_pn, attr['_subname'])
	    kvmod.kv_set(cur_pn, attr['_subname'], v)

class FileOffsetUndoManager(UndoManager):
    def undo(self, e, n):
	if '_cur_ver' in n.a and n.a['_cur_ver'] <= e.time():
	    return

	print "undo: roll back offset", n.tostr(), "to time", e.time()

	parent = self.g.node(n.a['_parent_object'])
	off = int(n.a['_subname'])
	blen = parent.a['_block_length']

	cur_pn = parent.undomgr().find_ino_cur(parent.a)
	snap_pn = parent.undomgr().find_ino_snap(parent.a, e)

	if cur_pn is None or snap_pn is None:
	    print "undo: cannot find files for", n.tostr()
	    return

	if self.really_repair:
	    fsrc = open(snap_pn, 'r')
	    fdst = open(cur_pn,  'r+')
	    fsrc.seek(off)
	    fdst.seek(off)
	    data = fsrc.read(blen)
	    if len(data) == 0: # macilious append
		data = '\x00' * blen
	    fdst.write(data)
	    fsrc.close()
	    fdst.close()
	    statistics['off_undo'].add((e, n))
	n.a['_cur_ver'] = e.time()
	return filter(lambda e2: e2.time() > e.time(),
		      self.g.backward(n.label()))

    def redo(self, e, n):
	h = history[e.time()]
	if h.scname == 'write':
	    parent = self.g.node(n.a['_parent_object'])
	    cur_pn = parent.undomgr().find_ino_cur(parent.a)
	    if cur_pn is None:
		print "redo: cannot find file", n.tostr()
		return

	    if self.really_repair:
		f = open(cur_pn, 'r+')
		f.seek(h.args[0].fd_stat.fd_offset - h.ret)
		f.write(h.args[1].data[0:h.ret])
		f.close()
		statistics['off_redo'].add((e, n))
	else:
	    print "XXX redo: unknown syscall", h.scname

    def edge_same(self, e, ns, nd):
	if ugly_hacks:
	    if ns.a['_undomgr'] == 'offmgr':
		parent = self.g.node(ns.a['_parent_object'])
		if '/var/log/lastlog' in parent.a: return True
		if '/var/run/utmp' in parent.a: return True
		if '/var/log/wtmp' in parent.a: return True

## XXX: bdb may store data in a database file and a separate log file 
class BdbUndoManager(UndoManager):
    def undo(self, e, n):
	attr = n.get_attr()
	if '_cur_ver' in attr and attr['_cur_ver'] <= e.time():
	    return

	print "undo: roll back undomgr object", n.tostr(), "to time", e.time()
	parent = self.g.node(attr['_parent_object'])
	cur_pn = parent.undomgr().find_ino_cur(parent.a)
	snap_pn = parent.undomgr().find_ino_snap(parent.a, e)

	if cur_pn is None or snap_pn is None:
	    print "undo: cannot find file", n.tostr()
	    return

	if self.really_repair:
	    import bsddb
	    dbsrc = bsddb.btopen(snap_pn, 'r')
	    dbdst = bsddb.btopen(cur_pn)
	    k = attr['_subname'].decode("base64")
	    v = dbsrc.get(k, None)
	    dbdst[k] = v
	    #print "[bdb] undo " + k + " => " + v + " in " + cur_pn
	    dbsrc.close()
	    dbdst.close()

	attr['_cur_ver'] = e.time()
	return filter(lambda e2: e2.time() > e.time(),
		      self.g.backward(n.label()))

    def redo(self, e, n):
	attr = n.get_attr()
	parent = self.g.node(attr['_parent_object'])
	cur_pn = parent.undomgr().find_ino_cur(parent.a)
	snap2_pn = parent.undomgr().find_ino_snap_after(parent.a, e)

	if cur_pn is None or snap2_pn is None:
	    print "redo: cannot find file", n.tostr()
	    return

	if self.really_repair:
	    import bsddb, base64
	    dbsrc = bsddb.btopen(snap2_pn, 'r')
	    dbdst = bsddb.btopen(cur_pn)
	    k = base64.b64decode(attr['_subname'])
	    v = dbsrc.get(k, None)
	    dbdst[k] = v
	    #print "[bdb] redo " + k + " => " + v + " in " + cur_pn
	    dbsrc.close()
	    dbdst.close()

pts_undo_buf = {}
class PtsUndoManager(UndoManager):
    ## XXX later: connect pts master objects with pts slave objects,
    ##     and have edge_same() return True for pts master reads,
    ##     because we have a compensation plan for pts slave writes.

    def ptsq(self, n):
	global pts_undo_buf
	if not pts_undo_buf.has_key(n.label()):
	    pts_undo_buf[n.label()] = (Queue.PriorityQueue(), Queue.PriorityQueue())
	return pts_undo_buf[n.label()]

    def undo(self, e, n):
	if history[e.time()].scname == 'write':
	    self.ptsq(n)
	    statistics['pts_undo'].add((e, n))
	    return self.g.backward(n.label())

    def redo(self, e, n):
	h = history[e.time()]
	if h.scname != 'write':
	    return

	old_data = h.args[1].data
	new_data = h.args[1].data
	if e.a.has_key('_pts_write_data'):
	    new_data = e.a['_pts_write_data']

	qs = self.ptsq(n)
	qs[0].put((e.time(), old_data))
	qs[1].put((e.time(), new_data))
	statistics['pts_redo'].add((e, n))
	return self.g.backward(n.label())

class PwdUndoManager(UndoManager):
    def undo(self, e, n):
	pass

    def redo(self, e, n):
	if n.a.get('_repair_state', None) is not None:
	    return
	fn = n.a['_funcname']
	arg = n.a['_funcargs']
	logging.info('re-executing %s("%s")', fn, arg)
	ret = {
	    'getpwnam'  :self._getpwnam,
	    'getpwnam_r':self._getpwnam,
	    'getpwuid'  :self._getpwuid,
	    'getpwuid_r':self._getpwuid,
	    'getspnam'  :self._getspnam,
	    'getspnam_r':self._getspnam,
	    'getgrnam'  :self._getgrnam,
	    'getgrnam_r':self._getgrnam,
	    'getgrgid'  :self._getgrgid,
	    'getgrgid_r':self._getgrgid,
	    'grantpt'   :self._pass,
	    'initgroups':self._initgroups,
	    #'getgrouplist':self._pass,
	    #'getgroups' :self._pass,
	    #'setgroups' :self._pass,
	} [fn](arg)
	self.mark_actor_children(n, e.time(), [])
	n.a['_repair_retval'] = ret
        statistics['pwd_redo'].add((e, n))

    def edge_same(self, e, ns, nd):
	h = history[e.time()]
	if h.scname == 'undo_func_end':
	    # it's ok if redo has not been called
	    if '_repair_retval' not in ns.a:
		return True
	    oldret = h.args[1].data
	    newret = ns.a['_repair_retval']
	    r = (oldret == newret)
	    fn = ns.a['_funcname']
	    arg = ns.a['_funcargs']

            # 
            # NOTE. SHOULD BE REMOVED
            # ugly_hacks
            if arg == "eve" or (arg == struct.pack("i",1001) and not newret) :
                return True
            
	    if r: logging.info('no change for %s("%s"), %s', fn, arg, e)
	    else: logging.info('%s("%s") changed, %s, old "%s", new "%s"', fn, arg, e, oldret, newret)
	    return r
	return False

    def _find(self, i, fn):
	if isinstance(i, str):
	    idx = 0 # name
	else:
	    assert(isinstance(i, int))
	    idx = 2 # id
	try:
	    fn = '/mnt/undofs/d' + fn
	    with open(fn, 'r') as f:
		for line in f:
		    if line.split(':')[idx] == str(i):
			return line.rstrip('\n')
	except:
	    pass
	return ''

    def _getpwnam(self, name):
	return self._find(name, '/etc/passwd')
    def _getpwuid(self, arg):
	uid = struct.unpack('i',arg)[0]
	return self._find(uid, '/etc/passwd')
    def _getspnam(self, name):
	return self._find(name, '/etc/shadow')
    def _getgrnam(self, name):
	return self._find(name, '/etc/group')
    def _getgrgid(self, arg):
	gid = struct.unpack('i', arg)[0]
	return self._find(gid, '/etc/group')
    def _initgroups(self, arg):
	return struct.pack('i', 0)
    def _pass(self, arg): return struct.pack('i', 0)

## Repair first_edge in graph g, and any descendants as appropriate
def repair_loop(g, attack_edges):
    q = Queue.PriorityQueue()
    s = set()
    for e in attack_edges:
	q.put((e.time(), e))
	s.add(e)

    while not q.empty():
	(t, e) = q.get()
        
	statistics['edge_considered'].add(e)
        
	src = g.node(e.src())
	dst = g.node(e.dst())

        statistics['node_considered'].add(e.src())
        statistics['node_considered'].add(e.dst())
        
	src_same = src.undomgr().edge_same(e, src, dst)
	dst_same = dst.undomgr().edge_same(e, src, dst)

	if (e.a.get('_repair_force_rerun', False)) or \
	   (e.a.get('_repair', False) == 'nullify' and not dst_same) or \
	   (not src_same and not dst_same):
	    r = repair_edge(g, e)
	else:
	    r = []
	    if src_same: statistics['edge_same'].add((e, src))
	    if dst_same: statistics['edge_same'].add((e, dst))
	s.remove(e)

	for new_edge in r:
	    if new_edge in s:
		continue
	    if new_edge.time() < e.time():
		#raise Exception("new repair edge " + new_edge.tostr() + " goes back in time")
		## XXX actually the check should be whether we're forming a
		##     dependency loop in time.  it's fine to re-execute
		##     things out of order, as long as they doesn't come
		##     to depend on each other's output somehow.
		pass
	    q.put((new_edge.time(), new_edge))
	    s.add(new_edge)

def repair_edge(g, e):
    statistics['edge_repaired'].add(e)
    statistics['node_repaired'].add(e.src())
    statistics['node_repaired'].add(e.dst())
    
    ## Node to repair
    n = g.node(e.dst())
    mgr = n.undomgr()

    if e.a.get('_repair_state', None) is not None:
	if not n.a['_undomgr'] == 'ptsmgr':
	    logging.debug('edge %s already %s', e, e.a['_repair_state'])
	r = None
    elif e.a.get('_repair') == 'nullify':
	if not n.a['_undomgr'] == 'ptsmgr':
	    logging.debug('undoing edge %s with %s', e, mgr.__class__.__name__)
	r = mgr.undo(e, n)
	e.a['_repair_state'] = 'undone'
	n.a['_repair_loop_last_state'] = 'undone'
    else:
	if not n.a['_undomgr'] == 'ptsmgr':
	    logging.debug('redoing edge %s with %s', e, mgr.__class__.__name__)
	r = mgr.redo(e, n)
	e.a['_repair_state'] = 'redone'
	n.a['_repair_loop_last_state'] = 'redone'
    if r is None: r = []

    if len(r) > 0 and not n.a['_undomgr'] == 'ptsmgr':
	logging.debug("todo edges")
	[logging.debug("\t%s", e2) for e2 in r]

    for e2 in r:
	e2.a['_repair_force_rerun'] = True

    r = r + filter(lambda e2: e2.time() > e.time(), g.forward(n.label()))
    for e in r:
	if e.a.get('_repair_state', None) != 'redone': return r
    return []

def tidy_block(fn, blen):
    if not os.path.exists(fn):
	return
    contents = []
    with open(fn, 'r') as f:
	while True:
	    r = f.read(blen)
	    if len(r) == 0: break
	    if r != '\x00' * blen:
		contents.append(r)
    with open(fn, 'w') as f:
	f.writelines(contents)

undo_managers = {
    'procmgr':  ProcessUndoManager(g),
    'fsmgr':    FsUndoManager(g),
    'dirmgr':   DirUndoManager(g),
    'pymgr':    PyUndoManager(g),
    'kvmgr':    KvUndoManager(g),
    'offmgr':   FileOffsetUndoManager(g),
    'bdb_mgr':  BdbUndoManager(g),
    'ptsmgr':   PtsUndoManager(g),
    'pwdmgr':   PwdUndoManager(g),
}

if opt.backtrack:
    l = objs(g, opt.backtrack)
    for e in l:
	[s, t] = e
	target = t.label()  # which one should it be?
    bg = backtrack(g, Graph(), target, opt.backtrack)
    if not opt.graphout:
	print "Error: must specify output file"
    else:
	bg.dump(opt.graphout)
    exit(0)
if opt.attacker:
    opt.forward = []
    for a in [int(x) for x in opt.attacker.split(',')]:
	opt.forward.append(find_exec(g, a))
if opt.forward:
    for a in opt.forward:
	forward(g, a)
if opt.repair:
    if opt.attacker:
	atts = []
	for y in [int(x) for x in opt.attacker.split(',')]:
	    a = process_obj(g, y, 1)
	    attackedge = g.backward(a.label())[0]	## initial exec
	    attackedge.a['_repair'] = 'nullify'
	    print "attack edge", attackedge.time()
	    atts.append(attackedge)
	repair_loop(g, atts)
	# ugly hack
	tidy_block('/mnt/undofs/d/var/run/utmp', 384)
	tidy_block('/mnt/undofs/d/var/log/wtmp', 384)

        # delete non attackers nodes
        statistics['node_repaired']   = [n for n in statistics['node_repaired'  ] if 'attacker' in g.g[n][0].a]
        statistics['node_considered'] = [n for n in statistics['node_considered'] if 'attacker' in g.g[n][0].a]

	for k in sorted(statistics):
	    print k, '\t', len(statistics[k])

        def diff(stat,lhs,rhs):
            ldiff = stat[lhs] - stat[rhs]
            rdiff = stat[rhs] - stat[lhs]
            
            if len(ldiff) > 0 :
                print "!%s: %s" % (lhs, ",".join([str(e) for e in ldiff]))
            if len(rdiff) > 0 :
                print "!%s: %s" % (rhs, ",".join([str(e) for e in rdiff]))
           
        diff( statistics, "edge_considered", "edge_repaired" )
            
	for pts in pts_undo_buf:
	    qs = pts_undo_buf[pts]
	    os = ['', '']
	    for idx in [0, 1]:
		while not qs[idx].empty(): os[idx] = os[idx] + qs[idx].get()[1]
	    ls = ['', '']
	    for idx in [0, 1]:
		ls[idx] = [l + '\n' for l in os[idx].split('\n')]
	    if os[0] == os[1]: continue

	    diff = difflib.context_diff(ls[0], ls[1],
					'original output on ' + pts,
					'new output on ' + pts)
	    print reduce(lambda x,y: x+y, diff),
    else:
	print "Specify attacker for repair"
if opt.graphout:
    g.dump(opt.graphout)

