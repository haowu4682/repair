#!/usr/bin/python
import undo

@undo.redoable
def kv_set(fn, k, v):
    f = open(fn, 'r+')
    undo.mask_start(f)
    kvs = {}
    for l in f.readlines():
	x = l.rstrip('\n').split(':')
	kvs[x[0]] = x[1]
    kvs[k] = v

    undo.depend_to(f, k, 'kvmgr')
    f.seek(0)
    f.truncate()
    for k in kvs:
	if kvs[k] != None:
	    f.write(k + ":" + kvs[k] + "\n")

    f.flush()
    undo.mask_end(f)
    f.close()

@undo.redoable
def kv_get(fn, k):
    f = open(fn, 'r')
    undo.mask_start(f)
    kvs = {}
    for l in f.readlines():
	x = l.rstrip('\n').split(':')
	kvs[x[0]] = x[1]
    v = kvs.get(k, None)

    undo.depend_from(f, k, 'kvmgr')
    undo.mask_end(f)
    f.close()
    return v

@undo.redoable
def kv_append(fn, k, suffix):
    kv_set(fn, k, kv_get(fn, k) + suffix)

