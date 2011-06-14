import os, os.path
import subprocess
import record
import xdrlib
import fcntl
import select
import struct

class Process:
    def __init__(self, pid):
	self.pid = pid
	args = [os.path.dirname(os.path.realpath(__file__)) + '/../strace-4.5.19/strace',
		'-f',
		'-o', '-',
		'-R',
		'-p', str(pid)]
	self.stracep = subprocess.Popen(args,
					stdin=subprocess.PIPE,
					stdout=subprocess.PIPE,
					stderr=2,
					close_fds=True)
	self.si = self.stracep.stdin
	self.so = self.stracep.stdout

	fcntl.fcntl(self.so.fileno(), fcntl.F_SETFL,
	            fcntl.fcntl(self.so.fileno(), fcntl.F_GETFL) | os.O_NONBLOCK)
	self.inbuf = ''

	while True:
	    h = self.get_sysrec()
	    if h is None: break
	    if h.scno == 0xc0ffee and not h.enter:
		self.writecmd('r' + struct.pack('q', 0) + 'c')
		break
	    self.writecmd('c')

    def get_sysrec(self):
	while True:
	    try:
		unpacker = xdrlib.Unpacker(self.inbuf)
		h = record.syscall_record.unpack(unpacker)
		self.inbuf = self.inbuf[unpacker.get_position():]
		return h
	    except EOFError:
		select.select([self.so], [], [])
		try:
		    d = self.so.read(1024)
		    if len(d) == 0:
			return None
		    self.inbuf = self.inbuf + d
		except EOFError:
		    return None

    def writeaddr(self, addr, buf):
	self.writecmd("".join(["w" + struct.pack('q', addr + i) + buf[i]
	    for i in range(0, len(buf))]))

    def writearg(self, argnum, buf):
	self.writecmd("".join(["W" + struct.pack('q', argnum)
	    + struct.pack('q', i) + buf[i] for i in range(0, len(buf))]))

    def writecmd(self, buf):
	self.si.write(buf)

