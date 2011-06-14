#!/usr/bin/python
from funcs import *

kv_get('/mnt/undofs/d/f.dat', 'alice')
kv_get('/mnt/undofs/d/f.dat', 'bob')
kv_set('/mnt/undofs/d/f.dat', 'alice', 'attack')
kv_set('/mnt/undofs/d/f.dat', 'eve', 'bad')

