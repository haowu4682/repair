import sys
import statistics

global dryrun
dryrun = False

def set_dryrun(t):
	global dryrun
	dryrun = t

def set_quiet(dbg_enable=[]):
	import time
	import ctrl
	import dbg

	def ignore(*key, **kwds):
		pass

	ctrl.prettify = ignore

	for k in set(dbg.settings.keys()) - set(dbg_enable):
		setattr(dbg, k, ignore)
		setattr(dbg, k+"m", ignore)

	start = time.time()
	def done():
		print "Executed: %ss" % (time.time() - start)


def set_profile(dbg_enable=["error", "warn"]):
	import atexit

	set_quiet(dbg_enable)

	start = time.time()
	def done():
		print "Executed: %ss" % (time.time() - start)

	atexit.register(done)

def set_statistics():
	import atexit
	atexit.register(statistics.calc_statistics)