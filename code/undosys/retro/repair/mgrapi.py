import dbg
import util

class SomeInfinity(object): pass
class MinusInfinity(SomeInfinity):
  def __cmp__(self, other):
    if isinstance(other, MinusInfinity): return 0
    return -1
class PlusInfinity(SomeInfinity):
  def __cmp__(self, other):
    if isinstance(other, PlusInfinity): return 0
    return 1

class TimeInterval(object):
  """Base class used for time ordering of action history graph.

  (x = (a, b) < (c, d) = y) iff (x != y and b <= c)"""
  def __init__(self, tic, tac):
    assert(tic <= tac)
    assert(isinstance(tic, tuple) or isinstance(tic, SomeInfinity))
    assert(isinstance(tac, tuple) or isinstance(tac, SomeInfinity))
    self.tic = tic
    self.tac = tac
  def __cmp__(self, other):
    if self is other:
      return 0
    if self.tic == self.tac == other.tic == other.tac:
      return 0
    if self.tac <= other.tic:
      return -1
    if self.tic >= other.tac:
      return 1
    raise Exception('cannot order overlapping intervals', self, other, self.tic, self.tac, other.tic, other.tac)

class LoaderMap(object):
  loaders = {}
  @staticmethod
  def register_loader(prefix, loader):
    LoaderMap.loaders.setdefault(prefix, []).append(loader)
  @staticmethod
  def load_by_name(name, what):
    loaders = LoaderMap.loaders.get(name[0], [])
    for loader in loaders:
      loader(name, what)

class TupleName(object):
  def __init__(self, name):
    assert(isinstance(name, tuple))
    self.name = name
  def __repr__(self):
    return ':'.join(map(str, self.name))

class RegisteredObject(TupleName):
  name_map = {}                                 # This variable is static
  def __init__(self, name, osloader = None):
    """None for osloader refers to the global static OsLoader
    instance. If a value is passed in for osloader, it will be used
    instead."""
    super(RegisteredObject, self).__init__(name)
    if RegisteredObject.name_map.has_key(name):
      raise Exception('duplicate object', name)
    RegisteredObject.name_map[name] = self
    self.osloader = osloader
    dbg.load('registered object', name)

  @staticmethod
  def by_name(name):
    return RegisteredObject.name_map.get(name, None)

  @staticmethod
  def by_name_load(name):
    if not RegisteredObject.by_name(name):
      LoaderMap.load_by_name(name, None)
    return RegisteredObject.name_map[name]

  @staticmethod
  def all():
    l = []
    for n in RegisteredObject.name_map:
      l.append(RegisteredObject.name_map[n])
    return l
    # why not just
    #  return RegisteredObject.name_map.values()
    # ?
  @staticmethod
  def clear_cache():
    RegisteredObject.name_map = {}

class Node(RegisteredObject):
  def __init__(self, name):
    super(Node, self).__init__(name)
    self.checkpoints = set()
    self.loaded = set()
  def load(self, what):
    if what not in self.loaded:
      self.loaded.add(what)
      LoaderMap.load_by_name(self.name, what)
  @property
  def checkpts(self):
    return self.checkpoints

  def rollback(self, cp):
    # must be overridden
    raise Exception('rollback missing for ' + type(self).__name__)

  def save_checkpoint(self, cp):
    pass

class DataNode(Node):
  def __init__(self, name):
    super(DataNode, self).__init__(name)
    self.rset = set()
    self.wset = set()
  @property
  def readers(self):
    self.load('readers')
    return set(self.rset)
  @property
  def writers(self):
    self.load('writers')
    return set(self.wset)

class ActorNode(Node):
  def __init__(self, name):
    super(ActorNode, self).__init__(name)
    self.actionset = set()
  @property
  def actions(self):
    self.load('actions')
    return set(self.actionset)

class Action(RegisteredObject, TimeInterval):
  def __init__(self, name, actor, osloader = None):
    super(Action, self).__init__(name, osloader)
    self.actornode = actor
    self.inputset = set()
    self.outputset = set()
    self.cancel = False
  @property
  def actor(self):
    return self.actornode
  @property
  def inputs(self):
    return set(self.inputset)
  @property
  def outputs(self):
    return set(self.outputset)
  def equiv(self):
    return False
  def redo(self):
    raise Exception('redo missing for ' + type(self).__name__)
  def register(self):
    pass
  def connect(self):
    """Update the input and output nodes of this action to reflect the
    relevant dependencies to the action.

    E.g., if this action, self, names an output node n, update
    n.writers to reflect that self is a writer of n."""
    (ni, no) = (len(self.inputs), len(self.outputs))
    self.register()
    self.actor.actionset.add(self)
    for n in self.inputs:  n.rset.add(self)
    for n in self.outputs: n.wset.add(self)
    return (ni, no) == (len(self.inputs), len(self.outputs))
    #ipopov: this seems to return
    #   False if the operation did something
    #   True if it was a nop

class ReschedulePendingException(Exception):
  pass
