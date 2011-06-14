#!/usr/bin/python

import sys
import runopts

storage = {}

def collect(cat, data):
	if cat not in storage:
		storage[cat] = set()

	storage[cat].add(data)

def report_str():
	cats = storage.keys()
	s = ("%30s\t#items\n" % "category")
	for k in sorted(cats):
		s += ("%30s\t%d\n" % (k, len(storage[k])))
	return s

def report():
	print report_str()

def calc_statistics():
	import mgrapi
	import statistics
	import fsmgr
	import procmgr
	import ptymgr
	import mgrutil

	for o in mgrapi.RegisteredObject.all():
		cat = type(o).__name__
		statistics.collect(cat, o)

	sys.stderr.write(report_str())

if __name__ == "__main__":
	runopts.set_profile(["warn"])

	if len(sys.argv) != 2:
		print "usage: %s dir" % sys.argv[0]
		exit(1)

	try:
		m = __import__("osloader_stat")
		m.set_logdir(sys.argv[1])
		m.load(None)
	except Exception, e:
		print "Warning: %s" % str(e)
	except KeyboardInterrupt:
		pass

	for f in m.ld.sfs:
		print "%s:%s" % (f, f.tell())

	for (obj,cat) in m.storage.iteritems():
		i = storage.get(cat, 0)
		storage[cat] = i + 1

	print ("%30s\t#items\n" % "category")
	cats = storage.keys()
	for k in sorted(cats):
		print ("%30s\t%d" % (k, storage[k]))
	print "\n"
