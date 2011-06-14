
##
## TODO: remove timezone from timestamps 
##

import mgrapi, mgrutil, datetime, time, psycopg2, timetravel,\
       subprocess, json, urllib, os, sys, re, util, threading
from sqlparser import simpleSQL

thisdir = os.path.dirname(os.path.abspath(__file__))

def db_connect_params():
    return util.file_read("%s/../test/webserv/db/connect_params" % thisdir)

def db_query(db, q):
    cur = db.cursor()
    cur.execute(q)
    return cur

def connect(action, tic, tac):
    action.tic = tic
    action.tac = tac
    action.connect()

## Unbounded values for db columns
class Unbounded(object): pass
class Bottom(Unbounded):
    def __cmp__(self, other):
        if isinstance(other, Bottom): return 0
        return -1
    def __str__(self):
        return "BOT"
class Top(Unbounded):
    def __cmp__(self, other):
        if isinstance(other, Top): return 0
        return 1
    def __str__(self):
        return "TOP"

## represents a range of db column values (lo, hi].
## works for both integers and strings
class Range(object):
    def __init__(self, lo, hi):
        assert(isinstance(lo, Bottom) or isinstance(hi, Top) or type(lo) == type(hi))
        assert(lo <= hi)
        self.lo = lo
        self.hi = hi
    ## this range contains value x
    def contains(self, x):
        assert(isinstance(self.lo, Bottom) or isinstance(x, Unbounded) or type(x) == type(self.lo))
        assert(isinstance(self.hi, Top) or isinstance(x, Unbounded) or type(x) == type(self.hi))
        return (self.lo < x) and (x <= self.hi)
    ## this range overlaps with range r
    def overlaps(self, r):
        assert(isinstance(r, Range))
        return (self.contains(r.lo) or self.contains(r.hi) or \
                r.contains(self.lo) or r.contains(self.hi))
    def sql_where(self, col):
        ret = "" if isinstance(self.lo, Bottom) else "(" + self.sql_str(self.lo) + " < " + col + ")"
        if not isinstance(self.hi, Top):
            ret = ret + (" AND " if ret != "" else "") + ("(" + col + " <= " + self.sql_str(self.hi) + ")")
        return " 1=1 " if ret == "" else ret
    def sql_str(self, v):
        if isinstance(v, int) or isinstance(v, float):
            return str(v)
        else:
            return "'" + str(v) + "'"
    def __str__(self):
        return "[%s,%s]" % (str(self.lo), str(self.hi))

class DbSnapshot(mgrapi.TimeInterval, mgrapi.TupleName):
    def __init__(self, ts, dbpart):
        self.name = ('DbCheckpoint', dbpart, ts)
        super(DbSnapshot, self).__init__(ts, ts)

class DbPartition(mgrapi.DataNode):
    def __init__(self, db, table, col, r):
        assert(isinstance(r, Range))
        name = ('dbpart', table, col, r)
        super(DbPartition, self).__init__(name)
        self.db = db
        self.t = table
        self.c = col
        self.r = r
        rows = db_query(self.db,
                        """SELECT DISTINCT start_ts FROM %s WHERE %s""" % (self.t, self.r.sql_where(self.c))
                        ).fetchall()
        self.checkpoints.add(mgrutil.InitialState(self.name + ('ckpt',)))
        for x in rows:
            ts = self.dt_to_usec(x[0])
            self.checkpoints.add(DbSnapshot((ts,), self))
            
    partitions = None
    @staticmethod
    def load_parts(db):
        if DbPartition.partitions != None: return
        
        ## read the partition column and partitions from a file (manually
        ## generated for now). file format is JSON:
        ## { 'table_name': { 'column': colname, 'type' : int|str, 'ranges': [i_1, i_2, ..., i_n]}}
        p = json.loads(util.file_read('/tmp/retro/db_partitions.txt'))

        ## cast values in ranges to the right type
        for t in p:
            cast = str if p[t]['type'] == 'str' else int
            for i in xrange(len(p[t]['ranges'])):
                p[t]['ranges'][i] = cast(p[t]['ranges'][i])
            p[t]['ranges'].append(Top())

        ## create the db partitions
        for t in p:
            parts = []
            c = p[t]['column']
            r = p[t]['ranges']
            for i in xrange(len(r)):
                parts.append(DbPartition(db, t, c, Range(Bottom() if i==0 else r[i-1], r[i])))
            p[t]['parts'] = parts

        DbPartition.partitions = p

    ## returns the db partitions that cover the range r
    @staticmethod
    def get(db, t, r):
        DbPartition.load_parts(db)

        parts = DbPartition.partitions[t]['parts']
        return [p for p in parts if r.overlaps(p.r)]

    def dt_to_usec(self, dt):
        assert(isinstance(dt, datetime.datetime))
        return int(time.mktime(dt.timetuple()) * 1000000) + int(dt.strftime("%f"))
    def usec_to_dt(self, usec):
        return datetime.datetime.fromtimestamp(float(usec) / 1000000)
    def rollback(self, cp):
        assert(isinstance(cp, mgrutil.InitialState) or isinstance(cp, DbSnapshot))
        ## disable timetravel
        timetravel.disable(self.db, self.t)

        if isinstance(cp, mgrutil.InitialState):
            dt = '-infinity'
        else:
            dt = str(self.usec_to_dt(cp.tic[0]))
        
        ## delete all versions of rows after this checkpoint
        db_query(self.db, """DELETE FROM %s WHERE start_ts > '%s' AND %s""" % (self.t, dt, self.r.sql_where(self.c)))

        ## set end time of entries to infinity
        db_query(self.db, """UPDATE %s SET end_ts='infinity', end_pid=-1 WHERE start_ts <= '%s' AND '%s' < end_ts AND %s""" % \
                          (self.t, dt, dt, self.r.sql_where(self.c)))

        ## enable timetravel
        timetravel.enable(self.db, self.t)

class DbFnCall(mgrapi.Action):
    def __init__(self, db, fn, fid, actor, argsnode, retnode):
        super(DbFnCall, self).__init__((actor.name, fn, fid), actor)
        self.db = db
        self.fn = fn
        self.argsnode = argsnode
        self.retnode = retnode
        self.add_dependencies()
    def unwrap_list(self, a):
        while isinstance(a, list) and len(a) == 1: a = a[0]
        return a
    def is_conjunction(self, where):
        for x in self.unwrap_list(where):
            if not isinstance(x, list) and x != 'where' and x != 'and':
                return False
        return True
    ## returns clauses containing col, along with their position.
    ## eg: pos=2.3 => its the third clause within the 2nd clause
    def get_matches(self, where, tab, col):
        a = self.unwrap_list(where)
            
        ret = []
        for i in xrange(len(a)):
            if not isinstance(a[i], list): continue
            if len(a[i]) == 3: ## top-level clause
                if a[i][0] == col or a[i][0] == tab + '.' + col:
                    ret.append((str(i), a[i]))
            else:
                ret += [(str(i) + '.' + x[0], x[1]) for x in self.get_matches(a[i], tab, col)]
        return ret

    ## if this is a pg_query call, parse the query and
    ## add input/output deps to the right DbPartition nodes
    def add_dependencies(self):
        if self.fn != 'pg_query':
            return

        deps = []
        query = json.loads(urllib.unquote(self.argsnode.origdata))[1]
        tokens = simpleSQL.parseString(query)
        tables = [tokens.tables] if tokens[0] != 'select' else tokens.tables
        for t in tables:
            DbPartition.load_parts(self.db)
            col = DbPartition.partitions[t]['column']
            val = None
            if tokens[0] == 'insert':
                ## currently only support simple INSERTs of the form: 
                ##     (c1, .., cn) VALUES (v1, .., vn) 
                ## so, search for 'col' among (c1, .., cn)
                c = tokens.columns.asList()
                for i in xrange(len(c)):
                    if c[i] == col:
                        val = tokens.vals[i]
            else:
                ## currently support WHERE clauses which are ANDs with
                ## one col=val expression
                where = tokens.where.asList()
                if self.is_conjunction(where): ## WHERE clause has only ANDs
                    m = self.get_matches(where, t, col)
                    ## only one col=val top-level clause
                    if len(m) == 1 and m[0][0].find('.') == -1 and m[0][1][1] == "=":
                        val = m[0][1][2]

            if val is not None:
                deps.extend(DbPartition.get(self.db, t, Range(val, val)))
            else:
                ## add dependency to entire table
                deps.extend(DbPartition.get(self.db, t, Range(Bottom(), Top()))) 

        ## For select add only input dependency; for insert, add output dep;
        ## and for update/delete add both input/output deps
        if tokens[0] != 'insert':
            self.inputset.update(deps)
        if tokens[0] != 'select':
            self.outputset.update(deps)
                    
    def fn_args(self):
        if self.fn != 'pg_query':
            return self.fn + ' ' + self.argsnode.data

        ## for SELECTs, add end_ts='infinity' to the query
        ##
        ## for INSERT/UPDATE/DELETE, write the current ts to a 
        ## file that timetravel trigger uses as the update time
        args = []
        for a in json.loads(urllib.unquote(self.argsnode.data)):
            if a.startswith('SELECT'):
                args.append(a.replace("WHERE ", "WHERE end_ts='infinity' AND "))
            else:
                args.append(a)
            if a.startswith('INSERT') or a.startswith('UPDATE') or a.startswith('DELETE'):
                util.file_write('/tmp/retro/retro_rerun', '')
                 ## XXX: should this be pid of current php process instead of self.actor?
                util.file_write('/tmp/retro/tt_params', str(self.tic[0]) + " " + str(self.actor.pid))
        return self.fn + ' ' + urllib.quote(json.dumps(args))
    def redo(self):
        a = self.actor
        
        ## if php helper process not running, spawn it
        if a.php_helper is None or a.php_helper.poll() is not None:
            a.php_helper_in  = "/tmp/retro/.db_helper.in.%d"  % a.pid
            a.php_helper_out = "/tmp/retro/.db_helper.out.%d" % a.pid
            env = {'PIPE_IN' : a.php_helper_in,
                   'PIPE_OUT': a.php_helper_out }
            util.mkfifo(a.php_helper_in)
            util.mkfifo(a.php_helper_out)
            a.php_helper = subprocess.Popen([ thisdir + '/../test/webserv/db/db_helper.php' ], env=env)

        ## send function request to php helper via pipe
        util.file_write(a.php_helper_in, self.fn_args() + '\n')

        ## read response from php helper and return it in retnode
        ## XXX: check that response is not error
        self.retnode.data = util.file_read(a.php_helper_out).strip()

        a.wait_for_dbcall(self)
    def register(self):
        self.inputset.add(self.argsnode)
        self.outputset.add(self.retnode)

## writes 'data' to 'bufnode' at time 'ts'
class UpdateBufAction(mgrapi.Action):
    def __init__(self, ts, data, bufnode):
        name = bufnode.name + (ts, 'updatebuf')
        actor = mgrutil.StatelessActor(name + ('actor',))
        self.data = data
        self.tic = self.tac = ts
        self.bufnode = bufnode
        super(UpdateBufAction, self).__init__(name, actor)
    def redo(self):
        self.bufnode.data = self.data
    def register(self):
        self.outputset.add(self.bufnode)

class PipeReader(threading.Thread):
    def __init__(self, pn):
        super(PipeReader, self).__init__()
        self.pn = pn
        self.data = None
    def run(self):
        self.data = open(self.pn).read()
    def kill(self):
        self.join(0.1)
        if self.isAlive():
            print "PipeReader(%s) is blocked. Waking it up.." % self.pn
            util.file_write(self.pn, "\n")
            self.join()
        
class PhpActor(mgrapi.ActorNode):
    def __init__(self, name, pid):
	super(PhpActor, self).__init__(name)
	self.pid = pid
	self.checkpoints.add(mgrutil.InitialState(self.name + ('ckpt',)))

	self.run = None
        self.php_helper = None
        self.php_in  = '/tmp/.from_php.%d' % pid
        self.php_out = '/tmp/.to_php.%d' % pid
    @staticmethod
    def get(pid):
	name = ('php', pid)
	n = mgrapi.RegisteredObject.by_name(name)
	if n is None:
	    n = PhpActor(name, pid)
	return n
    def rollback(self, cp):
	assert(isinstance(cp, mgrutil.InitialState))
	if self.run: self.run.kill()
    def wait_for_dbcall(self, action):
        if self.run is None:
            return

        ## return response to prev db func call
        if isinstance(action, DbFnCall):
            util.file_write(self.php_out, action.retnode.data)

        rd = PipeReader(self.php_in)
        rd.start()
        while self.run.poll() is None and rd.data is None:
            time.sleep(0.05)
            
        if self.run.poll() is not None:
            ## redo done for this php actor; cancel remaining actions
            for a in [x for x in self.actions if x > action]:
                if not isinstance(a, PhpExit):
                    a.cancel = True
            rd.kill()
            return

        rd.join()
        fn, fid, args = rd.data.strip().split(':')
        print "PhpActor: wait_for_dbcall: got request:", fn, fid, args

        ## if next action is the same as the requested action, update 
        ## its args. else, create a new action
        nextact = min([x for x in self.actions if x > action])
        if isinstance(nextact, DbFnCall) and nextact.fn == fn:
            ts = max([x for x in nextact.argsnode.checkpts if x < nextact]).tac
            u = UpdateBufAction(util.between(ts, nextact.tic), args, nextact.argsnode)
            u.connect()
        else:
            ts = util.between(action.tac, nextact.tic)
            name = (self.pid, fn, time.time())
            argsnode = mgrutil.BufferNode(name + ('fncall', 'args'), ts, args)
            retnode = mgrutil.BufferNode(name + ('fncall', 'ret'), ts, None)
            fncall = DbFnCall(fn, fid, self, argsnode, retnode)
            connect(fncall, ts+(1,), ts+(2,))

class PhpStart(mgrapi.Action):
    def __init__(self, actor, argsnode):
        self.argsnode = argsnode
        super(PhpStart, self).__init__(argsnode.name + ('phpstart',), actor)
    def redo(self):
        a = self.actor

	util.mkfifo(a.php_in)
	util.mkfifo(a.php_out)
        env = { 'RETRO_LOG_FILE': a.php_in,
                'RETRO_GET_FILE': a.php_out,
                'DB_QUERY': json.loads(urllib.unquote(self.argsnode.data)) }
        a.run = subprocess.Popen([ thisdir + '/../test/webserv/db/dbq.php' ], env = env)
        a.wait_for_dbcall(self)
    def register(self):
        self.inputset.add(self.argsnode)

class PhpExit(mgrapi.Action):
    def __init__(self, actor, retnode):
	self.retnode = retnode
	super(PhpExit, self).__init__(retnode.name + ('phpexit',), actor)
    def redo(self):
        a = self.actor
        try:
            if a.run is not None: a.run.kill()
            os.unlink(a.php_in)
            os.unlink(a.php_out)
            
            if a.php_helper is not None: a.php_helper.kill()
            os.unlink(a.php_helper_in)
            os.unlink(a.php_helper_out)
        except OSError: pass
    def register(self):
        self.outputset.add(self.retnode)

logdir = "/tmp/retro"
def set_logdir(d):
    logdir = d

def load(a, b):
    phpf = open(logdir + "/php.log")
    for line in phpf:
        (pids, tss, type, data) = line.strip().split(':')
        pid = int(pids)
        ts = int(tss)
        php = PhpActor.get(pid)
        if type == "call":
            args = mgrutil.BufferNode((pid, 'pargs'), (ts,), data)
            p_start = PhpStart(php, args)
            connect(p_start, (ts,1), (ts,2))
        elif type == "ret":
            ret = mgrutil.BufferNode((pid, 'pret'), (ts,), data)
            p_exit = PhpExit(php, ret)
            connect(p_exit, (ts,1), (ts,2))
        else:
            raise Exception("unknown type: " + type)
    phpf.close()

    db = psycopg2.connect(db_connect_params())
    dbf = open(logdir + "/db.log")
    for line in dbf:
        (pids, tss, fn, fid, type, data) = line.strip().split(':')
        pid = int(pids)
        ts = int(tss)
        php = PhpActor.get(pid)
        if type == "call":
            args = mgrutil.BufferNode((pid, fn, fid, 'dargs'), (ts,), data)
            args_ts = ts
        elif type == "ret":
            ## XXX: timestamp should be (ts,) but that fails because an UPDATE
            ## fncall's time interval overlaps with checkpoint created
            ## by the update. fix it by adding an extra DbFnRet
            ret = mgrutil.BufferNode((pid, fn, fid, 'dret'), (args_ts,), data)
            fncall = DbFnCall(db, fn, fid, php, args, ret)
            connect(fncall, (args_ts,1), (args_ts,1))
        elif type != "serverpid":
            raise Exception("unknown type: " + type) 
            
def test_rollback():
    db = psycopg2.connect(db_connect_params())
    d = DbPartition.get(db, 'PERSON', Range('alice', 'bob'))
    print "len of d:", len(d)
    for x in d:
        print "**** Checkpoints for ", x, "with range", x.r
        for c in x.checkpts:
            print c

    c = min(d[1].checkpts)
    d[1].rollback(c)

