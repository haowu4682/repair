import bisect
import os
import sys
import struct
import threading

# byte (int) list to concatinated string
def btos(s):
    return "".join(chr(c) for c in s)

# unpack
def unpack_ushort(s):
    return struct.unpack("H", s)[0]

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

#
# memorize the return value of a function
# 
def memorize(func):
    def __new__(*args, **kws):
	if not hasattr(__new__, "cache"):
	    __new__.cache = {}
	if __new__.cache.has_key(args):
	    return __new__.cache[args]
	__new__.cache[args] = func(*args, **kws)
	return __new__.cache[args]
    return __new__

#
# add relpath to python load path
#
def append_path(relpath):
    d = os.path.dirname(sys.argv[0])
    pn = os.path.abspath(os.path.join(d, relpath))
    sys.path.append(pn)

# compared to queue.PriorityQueue
# 1. no locking
# 2. unique elements in queue
class PriorityQueue(object):
    def __init__(self):
	self._list = []
        self._hash = {}
    def __nonzero__(self):
	return self._list
    def empty(self):
	return len(self._list) == 0
    def get(self):
	rtn = self._list.pop(0)
	del(self._hash[rtn])
	return rtn
    def put(self, item):
	if self._hash.has_key(item):
	    return item
	i = bisect.bisect_left(self._list, item)
	if len(self._list) > i and self._list[i] == item:
	    return item
	self._list.insert(i, item)
	self._hash[item] = 1
	return item

class WriterThread(threading.Thread):
    def __init__(self, f, data):
	super(WriterThread, self).__init__()
	self.f = f
	self.data = data
    def run(self):
	self.f.write(self.data)
	self.f.close()

class ReaderThread(threading.Thread):
    def __init__(self, f):
	super(ReaderThread, self).__init__()
	self.f = f
	self.data = None
    def run(self):
	self.data = self.f.read()

def between(t0, t1):
    assert(t0 < t1)
    for i in range(0, max(len(t0), len(t1))):
	if len(t0) <= i: t0 = t0 + (t1[i] - 2,)
	t = t0[0:i] + (t0[i] + 1,)
	if t0 < t < t1: return t
    t = t0 + (0,)
    assert(t0 < t < t1)
    return t

## Warning: this Relation uses 'is' to compare objects, not '=='.
class Relation(object):
    def __init__(self):
	self.tuples = set()
    def insert(self, a, b):
	self.tuples.add((a, b))
    def remove(self, a, b):
	self.tuples.remove((a, b))
    def atobs(self, the_a):
	for (a, b) in self.tuples:
	    if a is the_a:
		yield b
    def btoas(self, the_b):
	for (a, b) in self.tuples:
	    if b is the_b:
		yield a

class ListFile:
    def __init__(self, l):
	self.l = l
	self.i = 0
	self.le = len(l)
    def read(self, amt):
	if self.i + amt < self.le:
	    self.i += amt
	    return self.l[self.i-amt:self.i]
	if self.i < self.le:
	    i, self.i = self.i, self.le
	    return self.l[i:]
	return None

## file utils
def unlink(pn):
    try: os.unlink(pn)
    except OSError: pass

def file_write(fn, data):
    f = open(fn, 'w')
    f.write(data)
    f.close()

def file_read(fn):
    with open(fn) as f:
        return f.read()

def mkfifo(pn):
    unlink(pn)
    os.mkfifo(pn)

