import sys
import dbg
import collections
import util
import os
import atexit

import mgrapi

import code

from sysarg import PathidTable # for PathidTable.dumptables

# dirty heuristic to debug
def prettify(n):
  if 'syscall' in str(n) and hasattr(n, 'argsnode'):
    sid = n.argsnode.origdata.sid & ((1<<48)-1)
    r   = n.argsnode.origdata
    return "@%s %s [%s]%s" % (sid, str(n), str(r.ts), str(r))

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
    return "[%s] %s%s%s %s" % (pid, act,
                               ("(%s)" % ext if ext else ""),
                               ("@" + str(tic)[-8:] if tic else ""),
                               n)
  return n

class RepairCtrl(object):
  """Virtual. Main repair loop of Retro.

  This class is implemented as a state machine, with self.next
  referring to the next function to execute.
  """
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
  """Main repair loop of Retro.

  This class is implemented as a state machine, with self.next
  referring to the next function to execute.
  """
  colors = { 'current'  : 'deepskyblue',
       'affected' : 'orange',
       'dirty'  : 'saddlebrown',
       'skipped'  : 'greenyellow',
       'cancelled': 'red3',
       'redone'   : 'green4' }

  def dump_graph(self):
    """Print, through dbg.infom(), the nodes and actions that this
    instance contains. (ipopov)"""
    for o in self.cur_state:
      dbg.infom('%s' % (o.__class__.__name__))
      for a in self.actions(o):
        dbg.infom(' -> %s' % (a.__class__.__name__))
        dbg.infom('    -> actor %s' % (a.actor.__class__.__name__))

  def set_repair_flag(self):
    with open("/tmp/retro/retro_rerun", "w") as f: f.write("")
    atexit.register(util.unlink, "/tmp/retro/retro_rerun")

  def __init__(self, repair_ckpts):
    self.set_repair_flag()
    pi = mgrapi.PlusInfinity()
    self.action = None
    # ipopov: What is the difference between cur_state and mod_state???
    self.cur_state = collections.defaultdict(lambda: mgrapi.TimeInterval(pi, pi))
    self.mod_state = collections.defaultdict(lambda: mgrapi.TimeInterval(pi, pi))
    self.dirty = collections.defaultdict(lambda: False)
    for node, checkpoint in repair_ckpts.iteritems():
      self.rollback(node, checkpoint)
    self.next = self.pick_action

  def set_color(self, n, c):
    n.color = "style=filled color=%s" % c

  def set_cs(self, n, s):
    """Set the current state of node n to s."""
    self.cur_state[n] = s
    self.set_color(n, self.colors['affected'])

  def set_dirty(self, n, v):
    self.dirty[n] = v
    self.set_color(n, self.colors['dirty' if v else 'affected'])

  def rollback(self, n, cp):
    #code.interact(local=locals())
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
      if nacts:
        candidates.add(min(nacts))
    if candidates:
      self.action = min(candidates)
      self.set_color(self.action, self.colors['current'])

    self.next = self.done if self.action is None else self.process_action

  def process_action(self):
    self.next = self.pick_action
    dbg.infom('*', '%s:%s' % (prettify(self.action),
            "same" if self.action.equiv() else "#R<changed#>"))

    # dbg.infom('*', '[%s]%s:%s' % (prettify(self.action.tic),
    #         prettify(self.action),
    #         "S" if self.action.equiv() else "#R<D#>"))

    # ipopov: conjecture:
    # The purpose of this if/elif/else chain is to prevent the redo
    # from continuing if any of the statements in the elif's return to
    # the effect that other nodes have been updated during this pass
    # through process_action(). Why do we care? Because this class is
    # implemented as a state machine, and processing maintains the
    # invariant that the action being redone now is the earliest one.
    # But connect() and prepare_redo() may affect the invariant.

    if (not self.action.cancel and
        self.action.equiv() and
        all([self.cur_state[n] >= self.action for n in
             self.action.outputs | set([self.action.actor])])):
      dbg.info('skipping', self.action)
      for n in self.action.inputs:
        self.set_cs(n, max(self.cur_state[n], self.action))
      self.set_color(self.action, self.colors['skipped'])
    elif not self.action.connect():
      dbg.debug('retrying after connect')
    elif not self.prepare_redo():
      dbg.debug('retrying after prepare')
    elif self.action.cancel:
      if hasattr(self.action, 'cancel_hook'):
        self.action.cancel_hook()
      dbg.info('cancelled action', self.action)
      self.next = self.update_state
      self.set_color(self.action, self.colors['cancelled'])
    else:
      dbg.info('#G<redoing#> ', prettify(self.action))
      self.save_ckpts(self.action.outputs, self.action)
      try:
        self.action.redo()
      except mgrapi.ReschedulePendingException:
        dbg.info('rescheduling %s' % prettify(self.action))
        return
      else:
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
    """Process all actions until finished."""
    while(self.has_next()):
      self.next()

    pi = mgrapi.PlusInfinity()
    self.save_ckpts(self.mod_state.keys(), mgrapi.TimeInterval(pi, pi))
    dbg.info('repair done')
    PathidTable.dumptables()

  def repair_next_action(self):
    """Process one action and return."""
    # ipopov: comment: this function is not used internally
    if self.next == self.done:
      #dump_graph()
      return False

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
#import xdotviewer, gtk, dot, StringIO
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
  """Remove repair_node from the action history and reexecute."""
  ckpts = {repair_node: repair_cp}
  repair2(ckpts, gui)
