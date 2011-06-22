#!/usr/bin/python

from syscall import syscalls
from ctypes import sizeof, Structure

# generate C code for _syscalls
def print_syscalls():
	for i, sc in enumerate(syscalls):
		if not sc:
			print("\t{ %s, NULL }," % i)
			continue
		assert(i == sc.nr)
		print('\t{ %s, "%s"},' % (sc.nr, sc.name))

if __name__ == "__main__":
	print_syscalls()
