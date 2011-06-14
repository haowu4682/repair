from syscall import *
import copy
import os

_mappers = {}

def _mapper(old_nr, new_nr):
	def wrap(f):
		def new(c):
			r = copy.deepcopy(c)
			r.nr = new_nr
			f(r)
			return r
		_mappers[old_nr] = new
	return wrap

def nrmap(c):
	f = _mappers.get(c.nr)
	return f and f(c) or c

@_mapper(NR_creat, NR_open)
def _map_creat(r):
	r.args["flags"] = os.O_CREAT | os.O_WRONLY | os.O_TRUNC

@_mapper(NR_pipe, NR_pipe2)
def _map_pipe(r):
	r.args["flags"] = 0
