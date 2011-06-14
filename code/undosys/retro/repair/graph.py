import pickle
import weakref
import collections

from util import memorize

def _pack(ob):
	s = pickle.dumps(ob, pickle.HIGHEST_PROTOCOL)
	if isinstance(s, str): # python 2.x, convert for blob
		s = buffer(s)
	return s

def builddb(db, edge_gen):
	c = db.cursor()
	c.executescript("""
		create table R(ticket integer primary key, name text);
		create table V(name text primary key, data blob);
		create table E(src text, dst text, primary key(src, dst));
	""")

	for r in edge_gen:
		# bulid db
		name = repr(r)
		c.execute("insert into R values (?, ?)", (r.ticket, name))
		c.execute("insert into V values (?, ?)", (name, _pack(r)))
		(rs, ws) = r.nrdep()
		c.executemany("insert or ignore into V values (?, ?)",
			[(repr(v), _pack(v)) for v in rs|ws])
		c.executemany("insert or ignore into E values (?, ?)",
			[(repr(v), name) for v in rs] + [(name, repr(v)) for v in ws])

	db.commit()
	c.close()

def all_actions(db):
	c1 = db.cursor()
	c1.execute('select name from R order by ticket', ())
	while True:
		r1 = c1.fetchone()
		if r1 is None: break
		c2 = db.cursor()
		c2.execute('select data from V where name=?', (r1[0],))
		yield pickle.loads(c2.fetchone()[0])
		assert c2.fetchone() is None
		c2.close()
	c1.close()

# for weakref
class _List(list): pass

def _select(ob, attr, sql, parameters):
	r = getattr(ob, attr, lambda: None)()
	if r is None:
		c = ob._conn.cursor()
		c.execute(sql, parameters)
		rows = c.fetchall()
		c.close()
		r = _List([node_by_name(str(row[0]), ob._conn) for row in rows])
		setattr(ob, attr, weakref.ref(r))
	return r

type_mgr_map = {}
def register_mgr(obtype, nodetype):
	type_mgr_map[obtype] = nodetype

conn_node_map = collections.defaultdict(lambda: {})
def node_by_name(name, conn):
	assert isinstance(name, str)

	if conn_node_map[conn].has_key(name):
		return conn_node_map[conn][name]

	# load ob
	c = conn.cursor()
	c.execute("select data from V where name=?", (name,))
	obdata = c.fetchone()[0]
	assert c.fetchone() is None
	c.close()
	ob = pickle.loads(obdata)

	obtype = type(ob)
	while obtype is not None:
		if type_mgr_map.has_key(obtype):
			r = type_mgr_map[obtype](name, conn, ob)
			conn_node_map[conn][name] = r
			return r
		obtype = obtype.__base__

	raise Exception('Missing manager for', type(ob))

class Node(object):
	def __init__(self, name, conn, ob):
		assert isinstance(name, str)
		self.name = name
		self._conn = conn
		self.ob = ob
	def __eq__(self, other):
		return other and self.name == other.name
	def __ne__(self, other):
		return not other or self.name != other.name
	def __hash__(self):
		return hash(self.name)
	def __repr__(self):
		return self.name

class DataNode(Node):
	@property
	def readers(self):
		return _select(self, "_out",
		"select dst from E where src=?", (self.name,))
	@property
	def writers(self):
		return _select(self, "_in",
		"select src from E where dst=?", (self.name,))
	def checkpts(self):
		return []
	def rollback(self, ts):
		raise Exception('rollback missing for ' + type(self).__name__)

class ActionNode(Node):
	def __init__(self, name, conn, ob):
		super(ActionNode, self).__init__(name, conn, ob)
		self.ticket = self.ob.ticket
		self.tic    = self.ob.tic
		self.tac    = self.ob.tac
	def __lt__(self, other):
		# assert issubclass(type(other), ActionNode)
		return self.ticket < other.ticket
	def __cmp__(self, other):
		# assert issubclass(type(other), ActionNode)
		return self.ticket.__cmp__(other.ticket)
	@property
	def inputs(self):
		return _select(self, "_in",
		"select src from E where dst=?", (self.name,))
	@property
	def outputs(self):
		return _select(self, "_out",
		"select dst from E where src=?", (self.name,))
	def redo(self, dryrun):
		raise Exception('redo missing for ' + type(self).__name__)
	def equiv(self):
		return False
