#! /usr/bin/python

import os
import sys
import time
import glob
import math

ROOT=sys.path[0] + "/.."

if os.getuid() != 0:
	print "%s should be executed with root!" % sys.argv[0]
	exit(1)

def fmt_size(size):
	s = str(size)
	d = 0 if size == 0 else int(math.log(size, 1024))
	m = ["B", "K", "M", "G", "T"][d]
	return "%.2f%s" % (float(size)/(1024**d), m)

def du(files):
	total = 0
	for f in glob.glob(files):
		total += os.stat(f).st_size
	return fmt_size(total)

def time_diff(setup, cmd):
	os.system(setup)
	
	start = time.time()
	os.system(cmd)
	return time.time() - start

cmd = "ls -R / > /dev/null"
ctl = "%s/bin/retroctl -p -o /tmp -- " % ROOT

os.system("insmod ../trace/khook.ko 2> /dev/null")

# worm up
os.system(cmd)

print "w/o    retro:", time_diff("rmmod retro", cmd)
print "w/     retro:", time_diff("insmod %s/trace/retro.ko" % ROOT, ctl + cmd)
print "log    size :", du("/tmp/syscall*")
print "pid    size :", du("/tmp/pid*")
print "inode  size :", du("/tmp/inode*")
print "kprobe size :", du("/tmp/kprobes*")
print "pathid size :", du("/tmp/pathid[0-9]")
print "pathidtable size :", du("/tmp/pathidtable[0-9]")
