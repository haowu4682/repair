#! /usr/bin/python

import os
import sys

from os import path, system
from glob import glob

ROOT = sys.path[0] + "/.."

sys.path.append(ROOT + "/repair")

import retroctl

if os.getuid() != 0:
    print "execute %s as root" % sys.argv[0]
    sys.exit(-1)

def insmod(module, reload=False):
    if reload or system("lsmod | grep -q %s" % module) != 0:
        if reload:
            system("rmmod %s" % module)
        system("insmod %s/trace/%s.ko" % (ROOT, module))

insmod("iget" , False)
insmod("khook", False)
insmod("retro", True)

# clean up btrfs
system("%s/test/clean-btrfs.sh /mnt/retro" % ROOT)

# mount
system("make mountnochroot > /dev/null")

# disable retro
retroctl.disable()

# flush each logs
root = "/sys/kernel/debug/retro/"
for log in glob(root + "syscall*") + glob(root + "kprobes*"):
    with open(log) as fd:
        while True:
            dump = fd.read(4096)
            if len(dump) == 0:
                break

