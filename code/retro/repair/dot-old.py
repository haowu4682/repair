#!/usr/bin/python

import sys
import record
import graph
import mgrs
import optparse
import sqlite3
import attacker
import zoobarmgr

from util import PriorityQueue

def dot(f, db, dirp, pick, excludes):
	graph.builddb(db, record.load(dirp))
	attack_edges = attacker.choose_attack(db, pick)

	#graph.builddb(db, zoobarmgr.load(dirp))
	#attack_edges = attacker.choose_attack(db, attacker.pick_evil)

	class dotfilter:
		def __init__(self, f):
			self.filtered = False
			self.f = f
			self.close = f.close
			
		def write(self, out, objs=[]):
			for ob in objs:
				if any(str(ob.ob).find(e)!=-1 for e in excludes):
					self.filtered = True
					return
			self.f.write(out)
			self.filtered = False

	f = dotfilter(f)
	f.write("digraph G {\n")
	q = PriorityQueue()
	for e in attack_edges:
		setattr(q.put(graph.node_by_name(e, db)), "_repair", True)
	_dot(f, db, q)
	for e in attack_edges:
		f.write('"%s" [shape=box, style=filled, color=red];\n' % e)
	f.write("}\n")

def _dot(f, db, q):
	vers = {}
	while not q.empty():
		a = q.get()
		t = a.ticket
		# todo edges, no need to repair
		if not getattr(a, "_repair", None):
			f.write('"%s";\n' % a, [a])
			for n in a.outputs:
				f.write('"%s" -> "%s@%s";\n' % (a, n, t), [a,n])
			continue
		# repair edges
		f.write('"%s" [shape=box, style=filled, color=green];\n' % a, [a])
		srcs = getattr(a, "_srcs", set())
		for n in a.inputs:
			v = vers.setdefault(n.name, [t-1])[-1]
			# draw a green line if from a tainted input
			if n in srcs:
				color = " [color=green]"
			else:
				color = ""
			f.write('"%s@%s" -> "%s"%s;\n' % (n, v, a, color), [a,n])
		for n in a.outputs:
			f.write('"%s" -> "%s@%s" [color=green];\n' % (a, n, t), [a,n])
			if not f.filtered:
				f.write('"%s@%s" [style=filled, color=green];\n' % (n, t), [n])
				h = vers.setdefault(n.name, [t])
				if h[-1] != t:
					h.append(t)
			for x in n.writers:
				if x > a:
					q.put(x)
			for x in n.readers:
				if x > a:
					y = q.put(x)
					# mark as repair, not only todo
					setattr(y, "_repair", True)
					# mark tainted inputs
					srcs = getattr(y, "_srcs", set())
					srcs.add(n)
					setattr(y, "_srcs", srcs)
	count = 0
	for name, h in vers.items():
		if len(h) <= 1:
			continue
		f.write("subgraph cluster%s {\n" % count)
		f.write(" -> ".join(['"%s@%s"' % (name, x) for x in h]) + ";\n")
		#for x in h:
		#	f.write('"%s@%s";\n' % (name, x))
		f.write("}\n")
		count = count + 1

if __name__ == "__main__":
	exclude_list = ["/usr", "/lib", "/proc", "/dev", "/etc", "/sbin"]

	parser = optparse.OptionParser(usage="%prog [options] LOG-DIRECTORY")
	parser.add_option("-e", "--excl", dest="exclude",
			  metavar="LIST", default=repr(exclude_list),
			  help="%s" % exclude_list)
	parser.add_option("-o", "--dot" , dest="dot",
			  metavar="FILE", default="-",
			  help="write dot into FILE")
	parser.add_option("-f", "--db"  , dest="db", 
			  metavar="FILE", default=":memory:",
			  help="write db to FILE")
	parser.add_option("-a", "--attk", dest="attack" ,
			  metavar="NAME", default="pick_execve",
			  help="algorithm to pick attacker's node")
	(opts, args) = parser.parse_args()

	if len(args) != 1:
		parser.error("wrong argument")

	if opts.attack not in dir(attacker):
		parser.error("please choose one of %s" % \
			     ", ".join([l for l in dir(attacker)
					if l.find("pick_") != -1]))

	opts.attack = getattr(attacker, opts.attack)
	
	out = (opts.dot == "-") and sys.stdout or open(opts.dot, "w")
	dot(out, sqlite3.connect(opts.db), args[0], opts.attack, eval(opts.exclude))
