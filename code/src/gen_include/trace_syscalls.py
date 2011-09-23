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
        print('\t{ %s, "%s", %s, {' % (sc.nr, sc.name, len(sc.args)))
        for arg in sc.args:
            ty = arg.ty.__name__
            # haowu: some hack here to adapt to our system
            new_ty = ty[7:] + "_record"
            new_overwrite = ty[7:] + "_overwrite"
            aux = arg.aux
            if isinstance(aux, type):
                assert issubclass(aux, Structure)
                aux = sizeof(aux)
            print('\t\t{ "%s", %s, %s, %s, %s },' % (arg.name, new_ty, new_overwrite, arg.usage & 1, aux))
        print("\t}, %s_exec }," % sc.name)

if __name__ == "__main__":
    print_syscalls()

