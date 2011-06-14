import pickle
import ctypes

__all__ = [ "mask_start", "mask_end", "func_start", "func_end",
	    "depend", "depend_to", "depend_from", "depend_both" ]

__undo_lib = None

def __undo_init():
    global __undo_lib
    if __undo_lib is None:
	for p in ['libundo.so', './libundo.so', '../libundo/libundo.so']:
	    try:
		__undo_lib = ctypes.CDLL(p)
	    except:
		pass
    if __undo_lib is None:
	raise Exception('cannot find libundo.so')

def mask_start(f):
    __undo_init()
    __undo_lib.undo_mask_start(f.fileno())

def mask_end(f):
    __undo_init()
    __undo_lib.undo_mask_end(f.fileno())

def func_start(undomgr, fname, args):
    d = pickle.dumps(args)
    __undo_init()
    __undo_lib.undo_func_start(undomgr, fname, len(d), d)

def func_end(retval):
    d = pickle.dumps(retval)
    __undo_init()
    __undo_lib.undo_func_end(len(d), d)

def depend(f, subname, mgr, proc_to_obj, obj_to_proc):
    p2o = 0
    o2p = 0
    if proc_to_obj: p2o = 1
    if obj_to_proc: o2p = 1
    __undo_init()
    __undo_lib.undo_depend(f.fileno(), subname, mgr, p2o, o2p)

def depend_to(f, subname, mgr):
    depend(f, subname, mgr, True, False)

def depend_from(f, subname, mgr):
    depend(f, subname, mgr, False, True)

def depend_both(f, subname, mgr):
    depend(f, subname, mgr, True, True)

def redoable(f):
    def newf(*fargs):
	func_start('pymgr', f.__name__, fargs)
	v = f(*fargs)
	func_end(v)
	return v
    return newf

