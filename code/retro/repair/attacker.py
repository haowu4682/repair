#! /usr/bin/python

import os
import optparse

import code

import dbg
import mgrapi
import mgrutil
import osloader
import runopts
import multiprocessing

# return one of these values
PICK_STOP	= 1<<1
PICK_ATTACK	= 1<<2

def pick_execve(r):
	"""choose first successful execve"""

	if r.name == "execve" and		\
		    hasattr(r, "ret") and	\
		    r.ret == 0:
		dbg.info("pick:#R<%s#> %s" % (r.tac, r))
		return PICK_ATTACK|PICK_STOP

def pick_attack_execve(r):
	"""choose execve containing 'attacker' string"""

	if r.name == "execve" and str(r).find("attacker") != -1:
		dbg.info("#R<attack#>:%s" % r)
		return PICK_ATTACK|PICK_STOP

def pick_attack_write(r):
	"""choose write system call containing 'attack' string"""

	if r.name == "write" and		\
		    str(r).find("attack") != -1:
		dbg.info("pick:#R<%s#> %s" % (r.tac, r))
		return PICK_ATTACK|PICK_STOP

def pick_all(r):
	return PICK_ATTACK

def pick_evil(r):
	"""choose HTTP request with hint=attack in query"""
	if hasattr(r, 'query') and 'hint=attack' in r.query:
		return PICK_ATTACK

def find_picker(name):
	if name not in globals():
		raise Exception("please choose one of %s" % \
			     ", ".join([l for l in globals()
					if l.find("pick_") != -1]))
	return globals()[name]

def __find_attack_node(log, picker, pn, node):
	osloader.set_logdir(log)

	s = os.stat(pn)
	hint = ('ino', s.st_dev, s.st_ino) # ipopov: useful for reference
	mgrapi.RegisteredObject.by_name_load(hint)

	done = False
	for o in mgrapi.RegisteredObject.all():
		if done:
			break
		if isinstance(o, mgrutil.StatelessActor):
			for a in o.actions:
				cond = picker(a.argsnode.origdata)
				if cond is None:
					continue
				if cond & PICK_ATTACK:
					attack_node_name = o.name
				if cond & PICK_STOP:
					done = True

	if not done:
		raise "Failed to find attacker's binary `%s' with `%s'" % (pn, pick)

	node.value = str(attack_node_name)

def find_attack_node(log, picker, pn):
	picker = find_picker(picker)
	node = multiprocessing.Array("c", " "*100)
	#p = multiprocessing.Process(target=__find_attack_node, args=(log, picker, pn, node))
	#p.start()
	#p.join()
	__find_attack_node(log, picker, pn, node)
	#code.interact(local=locals())
	return eval(node.value)

if __name__ == "__main__":
	parser = optparse.OptionParser(usage="%prog [options] LOG-DIR PATH")
	parser.add_option("-a", "--attk"  , dest="attack",
			  metavar="NAME"  , default="pick_attack_execve",
			  help="algorithm to pick attacker's node")

	(opts, args) = parser.parse_args()

	if len(args) != 2:
		parser.print_usage()
		exit(1)

	print find_attack_node(args[0], opts.attack, args[1])

