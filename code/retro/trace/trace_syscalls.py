#!/usr/bin/python

from syscall import syscalls
from ctypes import sizeof, Structure

# generate C code for _syscalls
def print_syscalls():
	for i, sc in enumerate(syscalls):
		if not sc:
			print("\t{ %s }," % i)
			continue
		assert(i == sc.nr)
		print('\t{ %s, "%s", %s, %s, {' % (sc.nr, sc.name, sc.ret.__name__, len(sc.args)))
		for arg in sc.args:
			ty = arg.ty.__name__
			aux = arg.aux
			if isinstance(aux, type):
				assert issubclass(aux, Structure)
				aux = sizeof(aux)
			print('\t\t{ "%s", %s, %s, %s },' % (arg.name, ty, arg.usage & 1, aux))
		print("\t} },")

if __name__ == "__main__":
	print_syscalls()
