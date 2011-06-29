#!/usr/bin/python

import sys
import record
import graph
import optparse
import sqlite3
import mgrs
import dbg
from util import PriorityQueue
from statistics import statistics

rollbacks = None
queued_actions = None

class TimeLoopException(Exception):
	def __init__(self, n):
		self.n = n

def add_queue(queue, eles):
	global queued_actions
	for e in eles:
		queued_actions.append(e)
		# XXX what's the right order: tic or tac?  tickets seem weird.
		queue.put(e)

def rollback_nodes(nodes, tic):
	global rollbacks
	r = set()

	for n in nodes:
		if hasattr(n, 'rollback_ts') and n.rollback_ts <= tic:
			continue

		cps = n.checkpts()
		if cps is None or len(cps) == 0:
			raise Exception('no checkpoints for', n)
		cp_tacs = [(cp.tac, cp) for cp in cps if cp.tac <= tic]
		(max_tac, max_cp) = max(cp_tacs)

		dbg.ctrl("rollback %s to %s" % (n, max_cp))
		n.rollback(max_cp)
		rollbacks.append((n, tic))
		setattr(n, 'rollback_ts', max_cp.tac)
		r.update([x for x in set(n.readers)|set(n.writers) if x.tac > max_cp.tic])

	for e in r:
		if getattr(e, 'redone', False):
			raise TimeLoopException(e)
		if set(e.outputs).intersection(set(nodes)):
			e.todo = True

	return r

def rollback_action(a):
	return rollback_nodes(a.outputs, a.tic)

def repair_loop(db, q, attack_edges, dryrun):
	for e in attack_edges:
		n = graph.node_by_name(e, db)
		n.new_action = "nop"
		add_queue(q, rollback_action(n))

	while not q.empty():
		a = q.get()

		statistics("action_node_considered", a)
		assert(not getattr(a, 'redone', False))

		if (not getattr(a, 'todo', False)) and a.equiv():
			continue

		ne = rollback_action(a)
		if ne:
			add_queue(q, ne)
			continue

		dbg.ctrl("redoing %s" % a)
		a.redo(dryrun)
		a.redone = True

def repair(db, attack_edges, dryrun):
	global rollbacks
	global queued_actions

	rollbacks = []
	q = PriorityQueue()

	while True:
		try:
			queued_actions = []
			repair_loop(db, q, attack_edges, dryrun)
			return
		except TimeLoopException as tle:
			dbg.ctrl("time loop: %s" % tle.n)
			dbg.info("node list: %s" % str(rollbacks))
			for a in queued_actions:
				if hasattr(a, 'todo'): delattr(a, 'todo')
				if hasattr(a, 'redone'): delattr(a, 'redone')
				if hasattr(a, 'new_action'): delattr(a, 'new_action')
			rbsave = rollbacks
			rollbacks = []
			q = PriorityQueue()
			for (n, tic) in rbsave:
				if hasattr(n, 'rollback_ts'): delattr(n, 'rollback_ts')
			for (n, tic) in rbsave:
				ne = rollback_nodes([n], tic)
				add_queue(q, ne)

