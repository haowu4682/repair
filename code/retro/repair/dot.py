#!/usr/bin/python

import sys
import optparse
import mgrapi
import procmgr
import sockmgr
import mgrutil
import runopts
import dbg
import cStringIO

from collections import defaultdict

def dot_repr(o, xrefs = {}, omit = []):
	orepr = ""
	if isinstance(o, procmgr.ISyscallAction):
		orepr = repr(o.argsnode.data)
	else :
		orepr = repr(o)
	rtn = xrefs[orepr] if orepr in xrefs else orepr
	return rtn if rtn in omit else orepr

def has_color(o):
        return hasattr(o, 'color') and o.color is not None

class Scope:
	def __init__(self, beg, end):
		self.beg = (eval(beg),) if isinstance(beg, str) else beg
		self.end = (eval(end),) if isinstance(end, str) else end

	def valid(self, node):
		# display all
		if self.beg == None and self.end ==None:
			return True
		if self.end == None:
			return self.beg < node.tic
		return self.beg < node.tic and node.tac < self.end

class Dot:
	def __init__(self):
		self.subs = set()
		self.xref = {}
		self.opts = {}

        def load_graph(self, loader, path):
		m = __import__(loader)
		m.set_logdir(path)
		m.load(None, None)

	def load_xref(self):
		for o in mgrapi.RegisteredObject.all():
			if isinstance(o, mgrapi.DataNode):
				if isinstance(o, mgrutil.BufferNode):
					continue

				name = "cluster data_%s" % dot_repr(o)
				for x in sorted(o.checkpoints):
					self.xref[dot_repr(x)] = name
                                self.subs.add(name)

			if isinstance(o, mgrapi.ActorNode) and o.actions:
				if isinstance(o, mgrutil.StatelessActor):
					continue

				name = "cluster actor_%s" % dot_repr(o)
				for x in sorted(o.actions):
					self.xref[dot_repr(x)] = name
                                self.subs.add(name)

	def collapse_subgraph(self, f, omit_sub = [], *args, **kwds):
		if len(self.xref) == 0:
			self.load_xref()

		# special case, subgraphs == [] => collapse all subgraphs
		if len(omit_sub) == 0:
			omit_sub = self.subs
		return self.dump(f, omit_sub = omit_sub, *args, **kwds)

	# GUI display options
	def getopts(self):
		default_omit = set(["lib", "etc", "LC", "mmap", "unmap", "access", "close", "cache", "wait"])
		default_hl   = set(["write"])

		return {"omit"     : list(default_omit|set(self.opts.get("omit", []))),
			"highlight": list(default_hl  |set(self.opts.get("highlight", []))),
			"subgraph" : self.subs,
			"sysnode"  : True}

	# GUI glue
	def dump_opts(self, f, opts):
		return self.dump(f,
				 omit      = opts["omit"],
				 highlight = opts["highlight"],
				 omit_sub  = opts["subgraph" ],
				 omit_sys  = opts["sysnode"  ])

	def dump(self, f, beg = None, end = None, omit = [], highlight = [], omit_sub = [], omit_sys = False):
		scope = Scope(beg, end)

		# update options
		self.opts["omit"     ] = omit
		self.opts["highlight"] = highlight
		self.opts["subgraph" ] = omit_sub
		self.opts["sysnode"  ] = omit_sys

		if len(self.xref) == 0:
			self.load_xref()

		in_edges  = {}
		out_edges = {}

		omitted = set()

		f.write("digraph G {\n")

		fbuf = []
		fact = []
		for o in mgrapi.RegisteredObject.all():
			if isinstance(o, mgrapi.Action):
				if not scope.valid(o):
					continue

				o_repr = dot_repr(o, self.xref, omit_sub)
                                if has_color(o):
                                        fbuf.append(' "%s" [%s];\n' % (o_repr, o.color))

				# general omit
				if any(o_repr.find(s) != -1 for s in omit):
					continue

				# filter highlight
				if isinstance(o, procmgr.ISyscallAction) and \
				       any(o_repr.find(s) != -1 for s in highlight):
					fbuf.append('  "%s" [style=filled, color=green];\n' % o_repr)

				# draw inputs/outputs edges
				for n in o.inputs:
					# omit sysarg/sysret
					if isinstance(n, mgrutil.BufferNode) and omit_sys:
						assert len(n.writers) == 1
						n = list(n.writers)[0]

					n_repr = dot_repr(n, self.xref, omit_sub)

					if n_repr in out_edges and o_repr in out_edges[n_repr]:
						continue

					# general omit
					if any(n_repr.find(s) != -1 for s in omit):
						continue

					fbuf.append('  "%s" -> "%s" [color=green];\n' % (n_repr, o_repr))

					if not n_repr in out_edges:
						out_edges[n_repr] = set()
					if not o_repr in in_edges:
						in_edges[o_repr] = set()

					out_edges[n_repr].add(o_repr)
					in_edges[o_repr].add(n_repr)

				for n in o.outputs:
					# omit sysarg/sysret
					if isinstance(n, mgrutil.BufferNode) and omit_sys:
						assert len(n.readers) == 1
						n = list(n.readers)[0]

					n_repr = dot_repr(n, self.xref, omit_sub)

					if n_repr in in_edges and o_repr in in_edges[n_repr]:
						continue

					# general omit
					if any(n_repr.find(s) != -1 for s in omit):
						continue

					fbuf.append('  "%s" -> "%s" [color=green];\n' % (o_repr, n_repr))

					if not n_repr in in_edges:
						in_edges[n_repr] = set()
					if not o_repr in out_edges:
						out_edges[o_repr] = set()

					in_edges[n_repr].add(o_repr)
					out_edges[o_repr].add(n_repr)

		# omit meaningless actions
		for o in mgrapi.RegisteredObject.all():
			if isinstance(o, mgrapi.DataNode):
				if isinstance(o, mgrutil.BufferNode):
					if not omit_sys:
						o_repr = dot_repr(o)
						fbuf.append('  "%s" [shape=box %s];\n' %
                                                        (o_repr, o.color if has_color(o) else ''))
					continue

				name = "cluster data_%s" % dot_repr(o, self.xref)
				if name in omit_sub:
					continue

				# general omit
				if any(name.find(s) != -1 for s in omit):
					continue

				ckps = sorted(o.checkpoints)
				if len(ckps) == 0:
					continue

				# draw omit_sub of checkpoints
				out = cStringIO.StringIO()
				out.write('  subgraph "%s" {\n' % name)
                                if has_color(o):
                                        out.write('	 %s' % ('; '.join(o.color.split())))
				out.write('	 ')
				out.write(' -> '.join('"%s"' % dot_repr(x) for x in ckps))
				out.write(' %s "%s"' % ("->" if len(ckps) > 0 else "", dot_repr(o)))
				out.write(';\n')
				out.write('    label="%s";\n' % dot_repr(o, self.xref))
				out.write('  }\n')

				fbuf.append(out.getvalue())

			if isinstance(o, mgrapi.ActorNode) and o.actions:
				if isinstance(o, mgrutil.StatelessActor):
					continue

				name = "cluster actor_%s" % dot_repr(o, self.xref)
				if name in omit_sub:
					# store for later use
					omitted.add(name)
					continue

				# general omit
				if any(name.find(s) != -1 for s in omit):
					continue

				# collect edges having a reference
				objs = []
				for x in sorted(o.actions):
					x_repr = dot_repr(x)
					if x_repr in in_edges or x_repr in out_edges:
						objs.append('"%s"' % x_repr)

				out = cStringIO.StringIO()

				# draw omit_sub of actions
				if len(objs) == 0:
					continue

				out.write('  subgraph "%s" {\n' % name)
                                if has_color(o):
                                        out.write('	 %s' % ('; '.join(o.color.split())))
				out.write('	 ')
				out.write(' -> '.join(objs))
				out.write(';\n')
				out.write('    label="%s";\n' % dot_repr(o, self.xref))
				out.write('  }\n')

				fact.append(out.getvalue())

		# print actor
		for l in fact:
			f.write(l)

		for o in omitted:
			fbuf.append('  "%s" [style=filled shape=box color=green];\n' % o)

		# flush sorted fbuf to f
		fbuf.sort()
		for l in fbuf:
			f.write(l)

		f.write("}\n")

if __name__ == "__main__":
	parser = optparse.OptionParser(usage="%prog [options] LOG-DIRECTORY")
	parser.add_option("-o", "--dot" , dest="dot",
			  metavar="FILE", default="-",
			  help="write dot into FILE")
	parser.add_option('-m', '--module', dest='module',
			  metavar='MODULE', default='osloader',
			  help='loader module (osloader, zoobarmgr, ..)')
	parser.add_option("-s", "--statistics", default=False, action="store_true",
			  dest="statistics", help="report statistics at the end")
	parser.add_option("-b", "--beg" , dest="beg",
			  metavar="TIME", default=None,
			  help="begin time to parse")
	parser.add_option("-e", "--end" , dest="end",
			  metavar="TIME", default=None,
			  help="end time to parse")

	(opts, args) = parser.parse_args()

	if len(args) != 1:
		parser.error("wrong argument")
		exit(1)

	if opts.statistics:
		runopts.set_statistics()

	out = (opts.dot == "-") and sys.stdout or open(opts.dot, "w")
	d = Dot()
        d.load_graph(opts.module, args[0])

	d.dump(out, beg = opts.beg, end = opts.end, highlight = ["sock", "network"], omit_sys = False)


