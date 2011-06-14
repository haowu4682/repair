#!/usr/bin/python
import undo

@undo.redoable
def xx_getpwnam(username):
    f = open('/etc/passwd')
    undo.mask_start(f)
    r = None
    for l in f.readlines():
	v = l.rstrip('\n').split(':')
	if v[0] == username:
	    undo.depend_from(f, username, 'pwmgr')
	    r = v
	    break
    undo.mask_end(f)
    f.close()
    return r

print xx_getpwnam('root')

