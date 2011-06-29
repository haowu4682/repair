#!/usr/bin/python

import sys
import record
import osloader
import util

def strace(dirp):
	osloader.set_logdir(dirp)
	for x in record.load(dirp):
		print ('0x%04x' % x.sid), x

if __name__ == "__main__":
	util.install_pdb()
	if len(sys.argv) != 2:
		sys.stderr.write("usage: strace.py LOG-DIRECTORY\n")
		sys.exit(0)
	strace(sys.argv[1])
