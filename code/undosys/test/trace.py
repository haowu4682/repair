#!/usr/bin/python
import undo
from funcs import *
from os import system

kv_set('/mnt/undofs/d/f.dat', 'alice', 'aaa')
kv_get('/mnt/undofs/d/f.dat', 'alice')
kv_set('/mnt/undofs/d/f.dat', 'bob', 'bbb')

f = open('/mnt/undofs/d/f.dat', 'r')
s = f.readlines()
f.close()

system('/mnt/undofs/d/attacker.py')

## XXX how to capture something like this without a clean append function?
kv_append('/mnt/undofs/d/f.dat', 'alice', '2')

kv_set('/mnt/undofs/d/f.dat', 'chuck', 'def')
kv_set('/mnt/undofs/d/f.dat', 'bob', 'xyz')
kv_set('/mnt/undofs/d/f.dat', 'bob', 'xxx')

