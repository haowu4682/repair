#!/usr/bin/python

from syscall import syscalls
from ctypes import sizeof, Structure

# generate C definitions for syscall exec's
def print_syscalls_exec():
    for i, sc in enumerate(syscalls):
        if not sc:
            continue
        assert(i == sc.nr)
        print 'SYSCALL_(%s);' % sc.name

if __name__ == "__main__":
    print_syscalls_exec()

