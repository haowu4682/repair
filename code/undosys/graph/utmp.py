import ctypes, ctypes.util
import logging

_PATH_UTMP    = "/var/run/utmp"
_PATH_WTMP    = "/var/log/wtmp"
UTMP_FILE     = _PATH_UTMP
UTMP_FILENAME = _PATH_UTMP

UT_LINESIZE   = 32
UT_NAMESIZE   = 32
UT_HOSTSIZE   = 256

EMPTY         = 0
RUN_LVL       = 1
BOOT_TIME     = 2
NEW_TIME      = 3
OLD_TIME      = 4
INIT_PROCESS  = 5
LOGIN_PROCESS = 6
USER_PROCESS  = 7
DEAD_PROCESS  = 8
ACCOUNTING    = 9

UT_UNKNOWN    = EMPTY

class exit_status(ctypes.Structure):
    _fields_ = [("e_termination", ctypes.c_short),
                ("e_exit",        ctypes.c_short)]

class timeval(ctypes.Structure):
    _fields_ = [("tv_sec",  ctypes.c_int),
                ("tv_usec", ctypes.c_int)]

class utmp(ctypes.Structure):
    _fields_ = [("ut_type",    ctypes.c_short),
                ("ut_pid",     ctypes.c_int),
		("ut_line",    ctypes.c_char * UT_LINESIZE),
		("ut_id",      ctypes.c_char * 4),
		("ut_user",    ctypes.c_char * UT_NAMESIZE),
		("ut_host",    ctypes.c_char * UT_HOSTSIZE),
		("ut_exit",    exit_status),
		("ut_session", ctypes.c_int),
		("ut_tv",      timeval),
		("ut_addr_v6", ctypes.c_int * 4),
		("__unused",   ctypes.c_char * 20)]
    @staticmethod
    def typename(t):
	return ["EMPTY", "RUN_LVL", "BOOT_TIME", "NEW_TIME", "OLD_TIME", "INIT_PROCESS", "LOGIN_PROCESS", "USER_PROCESS", "DEAD_PROCESS", "ACCOUNTING"][t];
    def __str__(self):
	return "<" + ":".join([utmp.typename(self.ut_type), self.ut_id, self.ut_line]) + ">"
    def clone(self):
	r = type(self)()
	ctypes.pointer(r)[0] = self
	return r

_libc = ctypes.CDLL(ctypes.util.find_library("undowrap"))

utmpname = _libc.utmpname

_libc.setutent.restype = None
setutent = _libc.setutent

_libc.getutent.restype = ctypes.POINTER(utmp)
def getutent():
    return _libc.getutent().contents

_libc.endutent.restype = None
endutent = _libc.endutent

_libc.getutid.restype = ctypes.POINTER(utmp)
def getutid(ut):
    return _libc.getutid(ctypes.byref(ut)).contents

_libc.getutline.restype = ctypes.POINTER(utmp)
def getutline(ut):
    return _libc.getutline(ctypes.byref(ut)).contents

_libc.pututline.restype = ctypes.POINTER(utmp)
def pututline(ut):
    return _libc.pututline(ctypes.byref(ut)).contents

def delutline(filename, ut):
    logging.debug("remove " + str(ut) + " from " + filename)
    sz = ctypes.sizeof(utmp)
    needle = ctypes.string_at(ctypes.addressof(ut), sz)
    content = []
    f = open(filename, "rb")
    while True:
	s = f.read(sz)
	if len(s) < sz:
	   if len(s): logging.warning("truncate %s", filename)
	   break
	if s != needle:
	    content.append(s)
    f.close()
    # write back
    f = open(filename, "wb")
    f.writelines(content)
    f.close()

def delwtline(filename, ut):
    sz = ctypes.sizeof(utmp)
    content = []
    f = open(filename, "rb")
    while True:
	s = f.read(sz)
	if len(s) < sz:
	    if len(s): logging.warning("truncate %s", filename)
	    break
	e = ctypes.cast(ctypes.create_string_buffer(s), ctypes.POINTER(utmp)).contents
	if e.ut_pid != ut.ut_pid or e.ut_tv.tv_sec != ut.ut_tv.tv_sec or e.ut_tv.tv_usec != ut.ut_tv.tv_usec:
	    content.append(s)
    f.close()
    # write back
    f = open(filename, "wb")
    f.writelines(content)
    f.close()
