#!/usr/bin/python

import time, sys, os

ts = int(time.time() * 1000 * 1000)
db = sys.argv[1]
snap = db + '/.snap.' + str(ts)
os.mkdir(snap)

for f in os.listdir(db):
    if f.startswith('.'): continue
    with open(db + '/' + f, 'r') as fsrc:
	with open(snap + '/' + f, 'w') as fdst:
	    fdst.write(fsrc.read())

