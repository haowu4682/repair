import os
import sys
import util
import os.path as path

@util.memorize
def _get_appdir(root):
	for l in open("/proc/mounts"):
		if l.find("debugfs") != -1:
			return path.join(l.split()[1], root)

def toggle(root, pn, v):
	fd = open(path.join(_get_appdir(root), pn), "w")
	if fd == -1:
		raise Exception("failed to open:%s" % pn)
	fd.write(str(v))
	fd.close()

def disable():
	toggle("khook", "enabled", 0)

def enable(ctl):
	toggle("retro", "trace"   , 0)
	toggle("khook", "process" , 1)
	toggle("khook", "ctl"	  , ctl)
	toggle("khook", "enabled" , 1)

def enable_record_single():
	toggle("retro", "trace", 0)
	
def enable_record_enter():
	toggle("retro", "trace", 1)

def enable_record_exit():
	toggle("retro", "trace", 2)

def flush_record():
	toggle("retro", "flush", 0);