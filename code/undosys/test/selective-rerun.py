#!/usr/bin/python

import sys
sys.path.append('../graph')

import rerun
import os
import struct

pid = os.fork()
if pid == 0:
    os.execve('../graph/exec-ptrace', ['exec-ptrace',
				       './selective-rerun-script.sh',
				       './selective-rerun-script.sh'], {})
    os.exit(1)

p = rerun.Process(pid)

childpid = None

while True:
    h = p.get_sysrec()
    if h == None:
	break

    if h.scname == 'execve' and h.enter:
	pn = h.args[0].data.rstrip('\x00')
	print 'exec of', pn
	if pn == '/bin/ls':
	    childpid = h.pid
	    p.writecmd('x')
	    # fall through to 'c'

    if h.scname == 'wait4' and not h.enter and childpid == h.ret:
	print 'wait4 ret', h.ret
	print 'wait4 pid', struct.unpack('i', h.args[0].data[0:4])[0]
	print 'wait4 stat', struct.unpack('i', h.args[1].data[0:4])[0]
	p.writearg(1, struct.pack('q', 0))

    if h.enter:
	print 'entering', h.scname
    else:
	print 'exiting ', h.scname, 'retval', h.ret
    p.writecmd('c')

