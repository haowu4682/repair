import os
import subprocess
import ctypes, difflib, tempfile, shutil

cmd_base = [ 'patch', '--context', '--no-backup-if-mismatch',
	     '--binary', '--force', '--forward', '--reject-file=-',
	     '--quiet' ]

def run_proc(args, input):
    p = subprocess.Popen(args, bufsize=len(input), stdin=subprocess.PIPE,
			 stdout=subprocess.PIPE, stderr=subprocess.PIPE,
			 close_fds=True)
    stdout, stderr = p.communicate(input)
    status = p.wait()
    return stdout, stderr, status

def patch_try(pn, diff):
    stdout, stderr, status = run_proc(cmd_base + [ '--dry-run', pn ], diff)
    return (status == 0)

def patch_apply(pn, diff):
    stdout, stderr, status = run_proc(cmd_base + [ pn ], diff)
    if status != 0:
	raise SystemError(status, stderr)

def readlines(fn):
    with open(fn) as f:
	return f.readlines()

class TextPatch:
    def __init__(self, f0, f1):
	a = readlines(f0)
	b = readlines(f1)
	d = difflib.context_diff(a, b)
	self.diff = reduce(lambda x, y: x+y, d)
    def dry_run(self, pn):
    	return patch_try(pn, self.diff)
    def run(self, pn):
	# don't patch directly
	# otherwise a new file is created instead by patch
	# with different stat info
	with tempfile.NamedTemporaryFile(delete = False) as f:
	    pass
	shutil.copyfile(pn, f.name)
	patch_apply(f.name, self.diff)
	shutil.copyfile(f.name, pn)
	os.unlink(f.name)

def readrecords(fn, size):
    r = []
    with open(fn) as f:
	while True:
	    s = f.read(size)
	    if len(s) < size:
		break
	    r.append(s.encode("hex") + "\n")
    return r

# convert to text files for diff
class RecordPatch:
    def __init__(self, f0, f1, sz):
	if isinstance(sz, int):
	    self.size = sz
	elif issubclass(sz, ctypes.Structure):
	    self.size = ctypes.sizeof(sz)
	else:
	    raise ValueError("size must be integer or ctypes.Structure subclass")
	a = readrecords(f0, self.size)
	b = readrecords(f1, self.size)
	d = difflib.context_diff(a, b)
	self.diff = reduce(lambda x, y: x+y, d)
    def dry_run(self, pn):
	with tempfile.NamedTemporaryFile(delete = False) as f:
	    f.writelines(readrecords(pn, self.size))
	r = patch_try(f.name, self.diff)
	os.unlink(f.name)
	return r
    def run(self, pn):
	with tempfile.NamedTemporaryFile(delete = False) as f:
	    f.writelines(readrecords(pn, self.size))
	patch_apply(f.name, self.diff)
	lines = map(lambda s: s[0:-1].decode("hex"), readlines(f.name))
	os.unlink(f.name)
	with open(pn, "wb") as out:
	    out.writelines(lines)

