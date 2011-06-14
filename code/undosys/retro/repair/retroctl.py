import os
import sys
import util
import os.path as path

# This should be kept consistent with ../trace/ktrace.h
TRACE_RECORD = 0
TRACE_RECORD_ENTRY = 1
TRACE_RECORD_EXIT = 2

#@util.memorize
def _get_appdir(root):
  for l in open("/proc/mounts"):
    if l.find("debugfs") != -1:
      return path.join(l.split()[1], root)

def toggle(root, pn, v):
  """Issue sysctl _v_ on debugfs entry _pn_."""
  fd = open(path.join(_get_appdir(root), pn), "w")
  if fd == -1:
    raise Exception("failed to open:%s" % pn)
  fd.write(str(v))
  fd.close()

def disable():
  toggle("khook", "enabled", 0)

def enable(ctl):
  toggle("retro", "trace"   , TRACE_RECORD)
  toggle("khook", "process" , 1)
  toggle("khook", "ctl"   , ctl)
  toggle("khook", "enabled" , 1)

def enable_record_single():
  """Enable syscall execution, write out both entry and exit records."""
  toggle("retro", "trace", TRACE_RECORD)

def enable_record_enter():
  """Prevent syscalls from executing, but write out their entry records."""
  toggle("retro", "trace", TRACE_RECORD_ENTRY)

def enable_record_exit():
  """Execute syscalls, but write out only their exit records."""
  toggle("retro", "trace", TRACE_RECORD_EXIT)

def flush_record():
  toggle("retro", "flush", 0);
