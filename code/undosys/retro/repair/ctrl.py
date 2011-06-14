import sys, dbg, mgrapi, collections, util, os, atexit

# dirty heuristic to debug
def prettify(n):
        if 'syscall' in str(n) \
		    and hasattr(n, 'retnode'):
		sid = n.argsnode.origdata.sid & ((1<<48)-1)
		r   = n.argsnode.origdata
		return "@%s  [%s]%s" % (sid, str(r.ts), str(r))
	
	if isinstance(n, tuple):
		return "/".join(str(c) for c in n)
	
	tok = act = pid = ext = tic = ""
	n = str(n)
	if n.find("ckpt") != -1:
		return n.split(":")[-1]
	if n.startswith("pid:"):
		tok = n.split(":")
		pid = tok[1]
		if len(tok) >= 3:
			act = tok[2]
		if len(tok) >= 4:
			tic = tok[3]
		if len(tok) >= 5:
			ext = tok[4]
		return "[%s] %s%s%s" % (pid, act,
					("(%s)" % ext if ext else ""),
					("@" + str(tic)[-8:] if tic else ""))
	return n

class RepairCtrl(object):
        ## repair_ckpts is a dict of repair nodes to the ckpts they need
        ## to be rolled back to 
        def __init__(self, repair_ckpts):
                pass
        def has_next(self):
                raise Exception('has_next missing for ' + type(self).__name__)
        def next(self):
                raise Exception('next missing for ' + type(self).__name__)
        def repair(self):
                raise Exception('repair missing for ' + type(self).__name__)

class BasicRepairCtrl(RepairCtrl):
        colors = { 'current'  : 'deepskyblue',
                   'affected' : 'orange',
                   'dirty'    : 'saddlebrown',
                   'skipped'  : 'greenyellow',
                   'cancelled': 'red3',
                   'redone'   : 'green4' }

        def set_repair_flag(self):
                with open("/tmp/retro/retro_rerun", "w") as f: f.write("")
                atexit.register(util.unlink, "/tmp/retro/retro_rerun")
                
        def __init__(self, repair_ckpts):
                self.set_repair_flag()
                pi = mgrapi.PlusInfinity()
                self.action = None
                self.cur_state = collections.defaultdict(lambda: mgrapi.TimeInterval(pi, pi))
                self.mod_state = collections.defaultdict(lambda: mgrapi.TimeInterval(pi, pi))
                self.dirty = collections.defaultdict(lambda: False)
                for n in repair_ckpts:
                        self.rollback(n, repair_ckpts[n])
                self.next = self.pick_action

        def set_color(self, n, c):
                n.color = "style=filled color=%s" % c
                
        def set_cs(self, n, s):
                self.cur_state[n] = s
                self.set_color(n, self.colors['affected'])

        def set_dirty(self, n, v):
                self.dirty[n] = v
                self.set_color(n, self.colors['dirty' if v else 'affected'])
                
        def rollback(self, n, cp):
                dbg.info('#G<rollback#> %s -> %s' % (prettify(n), (prettify(cp))))
                n.rollback(cp)
                self.set_cs(n, cp)
                self.mod_state[n] = cp
                self.set_dirty(n, False)
                self.set_color(cp, self.colors['redone'])
        
        def prepare_redo(self):
                r = True
                if self.cur_state[self.action.actor] > self.action:
                        cp = max([c for c in self.action.actor.checkpts if c <= self.action])
                        self.rollback(self.action.actor, cp)
                        r = False
                for n in [n for n in self.action.outputs | self.action.inputs
                                  if self.mod_state[n] > self.action]:
                        cp = max([c for c in n.checkpts if c <= self.action])
                        self.rollback(n, cp)
                        r = False
                return r

        def actions(self, n):
                if isinstance(n, mgrapi.DataNode):
                        if self.dirty[n]:
                                return n.readers | n.writers
                        return n.writers
                return n.actions

        def save_ckpts(self, nodes, end):
                for n in nodes:
                        if n not in self.mod_state:
                                continue
                        for c in n.checkpts:
                                if c >= self.mod_state[n] and c < end:
                                        n.save_checkpoint(c)
                        
        def pick_action(self):
                self.action = None
                candidates = set()
                for n in self.cur_state:
                        nacts = [x for x in self.actions(n) if x > self.cur_state[n]]
                        if nacts: candidates.add(min(nacts))
                if candidates:
                        self.action = min(candidates)
                        self.set_color(self.action, self.colors['current'])

                self.next = self.done if self.action is None else self.process_action

        def process_action(self):
                self.next = self.pick_action
                dbg.infom('*', '%s:%s' % (prettify(self.action),
					  "S" if self.action.equiv() else "#R<D#>"))
		
                # dbg.infom('*', '[%s]%s:%s' % (prettify(self.action.tic),
		# 			      prettify(self.action),
		# 			      "S" if self.action.equiv() else "#R<D#>"))
		
		if (not self.action.cancel) and self.action.equiv() and \
		   all([self.cur_state[n] >= self.action for n in self.action.outputs |
							set([self.action.actor])]):
			dbg.info('skipping', self.action)
			for n in self.action.inputs: self.set_cs(n, max(self.cur_state[n], self.action))
                        self.set_color(self.action, self.colors['skipped'])
		elif not self.action.connect():
			dbg.debug('retrying after connect')
		elif not self.prepare_redo():
			dbg.debug('retrying after prepare')
		elif self.action.cancel:
			dbg.info('cancelled action', self.action)
                        self.next = self.update_state
                        self.set_color(self.action, self.colors['cancelled'])
		else:
			dbg.info('#G<redoing#> ', prettify(self.action))
                        self.save_ckpts(self.action.outputs, self.action)
			self.action.redo()
                        self.next = self.update_state
                        self.set_color(self.action, self.colors['redone'])

        def update_state(self):
		for n in self.action.inputs:
                        self.set_cs(n, self.action)
		for n in self.action.outputs | set([self.action.actor]):
                        self.set_cs(n, self.action)
			self.mod_state[n] = self.action
                        self.set_dirty(n, True)
                self.next = self.pick_action

        def done(self):
                pass

        def has_next(self):
                return self.next != self.done

        def repair(self):
                while(self.has_next()):
                        self.next()

                pi = mgrapi.PlusInfinity()
                self.save_ckpts(self.mod_state.keys(), mgrapi.TimeInterval(pi, pi))
                dbg.info('repair done')

        def repair_next_action(self):
                if self.next == self.done: return False
                
                while self.next != self.process_action and self.next != self.done:
                        self.next()
                if self.next == self.done: return False

                self.next() ## invoke process_action
                
                while self.next != self.process_action and self.next != self.done:
                        self.next()
                return True

class InteractiveRepairCtrl(BasicRepairCtrl):
        def __init__(self, repair_ckpts):
                super(InteractiveRepairCtrl, self).__init__(repair_ckpts)
                
        def repair(self):
                while (self.has_next()):
                        self.next()
                        print "Press ENTER to run next step..."
                        sys.stdin.readline()

util.append_path("../../xdot")
import xdotviewer, gtk, dot, StringIO
class GuiRepairCtrl(BasicRepairCtrl):
        def __init__(self, repair_ckpts):
                super(GuiRepairCtrl, self).__init__(repair_ckpts)
                self.dot = dot.Dot()

        def update_dotcode(self):
                s = StringIO.StringIO()
                self.dot.dump(s)
		(px, py) = self.win.widget.get_current_pos()
		pz = self.win.widget.zoom_ratio
                self.win.set_dotcode(s.getvalue())
		self.win.widget.zoom_ratio = pz
		self.win.widget.set_current_pos(px, py)
                
        def on_next(self):
                self.next()
                self.update_dotcode()
                ## XXX: disable buttons if repair complete

        def on_continue(self):
                while self.has_next():
                        self.next()
                self.update_dotcode()
                ## XXX: disable buttons if repair complete

        def repair(self):
                self.win = xdotviewer.DotWindow(self.on_next, self.on_continue,
                                                self.colors)
                self.win.connect('destroy', gtk.main_quit)
                self.win.set_filter('dot')
                self.update_dotcode()
                gtk.main()

# repair for multiple attack nodes
def repair2(repair_ckpts, gui=False):
        dbg.info('repairing with', repair_ckpts)
        if gui:
                ctrl = GuiRepairCtrl(repair_ckpts)
        else:
                ctrl = BasicRepairCtrl(repair_ckpts)
        ctrl.repair()

def repair(repair_node, repair_cp, gui=False):
        ckpts = {repair_node: repair_cp}
        repair2(ckpts, gui)
