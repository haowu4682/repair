#!/usr/bin/python

from syscall import syscalls

# extract undo_* calls
def print_syscalls():
	for i, sc in enumerate(syscalls):
		if sc and sc.name.startswith( "undo_" ) :
			print("NR_%s=%d," % (sc.name, sc.nr))

if __name__ == "__main__":
	print_syscalls()
