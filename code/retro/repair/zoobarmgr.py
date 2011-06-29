import os, urllib, dbg, mgrapi, mgrutil, subprocess, util, difflib, time, httpd, sys
import apache

## XXX some things to clean up:
##  -- duplicate code parsing strings from PHP logs & during re-exec

## DB node and action

def cancel_action(a):
    if isinstance(a, BrowserPageExit):
        return

    tocancel = [a]
    if isinstance(a, BrowserReqStart):
        for x in a.argsnode.readers:
            tocancel += x.actor.actions
    for x in tocancel:
        if isinstance(x, PhpDbCall):
            tocancel += x.argsnode.readers
        if isinstance(x, PhpExit):
            tocancel += x.retnode.readers
    for x in tocancel:
        x.cancel = True
        x.color = 'style=filled color=red3'

class DbSnapshot(mgrapi.TimeInterval, mgrapi.TupleName):
    def __init__(self, ts, pn):
	self.name = ('DbCheckpoint', pn)
	self.pn = pn
	super(DbSnapshot, self).__init__(ts, ts)

class DbNode(mgrapi.DataNode):
    def __init__(self, name, db_path):
	super(DbNode, self).__init__(name)
	self.db_path = db_path
	for f in os.listdir(db_path):
	    if f.startswith('.snap.'):
		ts = int(f.split('.')[2])
		self.checkpoints.add(DbSnapshot((ts,), db_path + '/' + f))
	assert len(self.checkpoints) >= 1
    @staticmethod
    def get(db_path):
	name = ('db', db_path)
	n = mgrapi.RegisteredObject.by_name(name)
	if n is None:
	    n = DbNode(name, db_path)
	return n
    def copy_dir(self, src, dst):
        for f in os.listdir(dst):
            if not f.startswith('.'):
                os.unlink(dst + f)
        for f in os.listdir(src):
            if f.startswith('.'):
                continue
            with open(src + f, 'r') as f1:
                with open(dst + f, 'w') as f2:
                    f2.write(f1.read())
    def rollback(self, cp):
	assert(isinstance(cp, DbSnapshot))
	real_dir = self.db_path + '/'
	snap_dir = cp.pn + '/'
        self.copy_dir(snap_dir, real_dir)
    def save_checkpoint(self, cp):
        assert(isinstance(cp, DbSnapshot))
        real_dir = self.db_path + '/'
        snap_dir = cp.pn + '/'
        self.copy_dir(real_dir, snap_dir)

class DbQueryAction(mgrapi.Action):
    def __init__(self, name):
	actor = mgrutil.StatelessActor(name + ('qactor',))
	self.argsnode = None
	self.retnode = None
	self.dbnode = None
	super(DbQueryAction, self).__init__(name, actor)
    def equiv(self):
	if self.argsnode.data != self.argsnode.origdata: return False
	if self.argsnode.data is None: return False
	if not self.argsnode.data['query'].upper().startswith('SELECT'):
	    return False
	d = self.dbq(self.argsnode.data)
	return d == self.retnode.origdata
    def dbq(self, argsdata):
	env = { 'DB_DIR': argsdata['dir'] + '/',
		'DB_QUERY': urllib.quote(argsdata['query']) }
        thisdir = os.path.dirname(os.path.abspath(__file__))
	p = subprocess.Popen([ thisdir + '/../test/webserv/zoobar/dbq.php' ],
			     env = env,
			     stdin = subprocess.PIPE,
			     stdout = subprocess.PIPE)
	(out, err) = p.communicate()
	return urllib.unquote(out)
    def redo(self):
	if self.argsnode.data is None: return
	self.retnode.data = self.dbq(self.argsnode.data)
    def register(self):
	self.inputset.add(self.argsnode)
	self.outputset.add(self.retnode)
	if self.argsnode and self.argsnode.data:
	    if self.argsnode.data['dir']:
		self.dbnode = DbNode.get(self.argsnode.data['dir'])
	    if not self.argsnode.data['query'].upper().startswith('SELECT') \
	       and self.dbnode:
		self.outputset.add(self.dbnode)
	if self.dbnode:
	    self.inputset.add(self.dbnode)

## PHP actor and actions

class PhpActor(mgrapi.ActorNode):
    def __init__(self, name, pid):
	super(PhpActor, self).__init__(name)
	self.pid = pid
	self.checkpoints.add(mgrutil.InitialState(self.name + ('ckpt',)))

	self.run = None
	self.stdout = None
	self.runmsg = None
	self.log_file = ('/tmp/.log_q.%d' % pid)
	self.get_file = ('/tmp/.get_q.%d' % pid)
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
    def run_poll(self, action):
	while True:
	    if self.run.poll() is not None:
		for a in [x for x in self.actions if x > action]:
		    if not isinstance(a, PhpExit):
                        cancel_action(a)
		return
	    with open(self.log_file, 'r') as f: m = f.read()
	    if '\n' in m:
		self.runmsg = m
		open(self.log_file, 'w').close()
		break
	dbg.debug('php message', self.runmsg.strip())

	(_, _, type, _, _) = self.runmsg.strip().split(' ')
	nextact = min([x for x in self.actions if x > action])
	ts = util.between(action.tac, nextact.tic)
	if type == 'query':
	    q = DbQueryAction(('nquery', self.pid, time.time()))
	    q.tic = ts+(3,)
	    q.tac = ts+(4,)
	    q.argsnode = mgrutil.BufferNode(q.name + ('args',), ts+(2,), None)
	    q.retnode = mgrutil.BufferNode(q.name + ('ret',), ts+(5,), None)
	    q.connect()

	    c = PhpDbCall(self, q.argsnode)
	    c.tic = ts
	    c.tac = ts+(1,)
	    c.connect()

	    r = PhpDbRet(self, q.retnode)
	    r.tic = ts+(6,)
	    r.tac = ts+(7,)
	    r.connect()
	else:
	    raise Exception('unknown PHP message type', type)

class PhpDbCall(mgrapi.Action):
    def __init__(self, actor, argsnode):
	self.argsnode = argsnode
	super(PhpDbCall, self).__init__(argsnode.name + ('phpdbcall',), actor)
    def redo(self):
	(_, _, type, db_path, dataq) = self.actor.runmsg.strip().split(' ')
	assert(type == 'query')
	self.argsnode.data = { 'dir':   db_path,
			       'query': urllib.unquote(dataq) }
    def register(self):
	self.outputset.add(self.argsnode)

class PhpDbRet(mgrapi.Action):
    def __init__(self, actor, retnode):
	self.retnode = retnode
	super(PhpDbRet, self).__init__(retnode.name + ('phpdbret',), actor)
    def equiv(self):
	return self.retnode.data == self.retnode.origdata
    def redo(self):
	dbg.debug('php response', self.retnode.data)
	with open(self.actor.get_file, 'w') as f:
	    f.write(urllib.quote(self.retnode.data))
	self.actor.run_poll(self)
    def register(self):
	self.inputset.add(self.retnode)

class PhpCkpt(mgrapi.TimeInterval, mgrapi.TupleName):
    def __init__(self, ts, pn):
	self.name = ('PhpCkpt', pn)
	self.pn = pn
	super(PhpCkpt, self).__init__(ts, ts)

class PhpScript(mgrapi.DataNode):
    def __init__(self, name, path):
        self.path = path
        self.modified = False
        super(PhpScript, self).__init__(name)
        self.checkpoints.add(PhpCkpt(mgrapi.MinusInfinity(), path))
    @staticmethod
    def get(path):
	name = ('phpscript', path)
	n = mgrapi.RegisteredObject.by_name(name)
	if n is None:
	    n = PhpScript(name, path)
	return n
    def rollback(self, cp):
        assert(isinstance(cp, PhpCkpt))

class PhpScriptUpdate(mgrapi.Action):
    def __init__(self, name, script):
        actor = mgrutil.StatelessActor(name + ('uactor',))
        self.script = script
        super(PhpScriptUpdate, self).__init__(name, actor)
    def redo(self):
        self.script.modified = True
    def register(self):
        self.outputset.add(self.script)

class PhpStart(mgrapi.Action):
    def __init__(self, actor, argsnode):
	self.argsnode = argsnode
	super(PhpStart, self).__init__(argsnode.name + ('phpstart',), actor)
    def equiv(self):
	return self.argsnode.data == self.argsnode.origdata
    def redo(self):
	a = self.actor
	d = self.argsnode.data

        ## merge php env vars from origdata into new data
        od = self.argsnode.origdata
        for k in od['env']:
            if k.startswith('PHP') and k not in d['env']:
                d['env'][k] = od['env'][k]
        
	try: os.unlink(a.log_file)
	except OSError: pass
	try: os.unlink(a.get_file)
	except OSError: pass
	os.mknod(a.log_file)
	os.mkfifo(a.get_file)

	pn = d['env']['SCRIPT_FILENAME']
	env = dict(d['env'])
	env['RETRO_RERUN'] = 'yes'
	env['RETRO_LOG_FILE'] = a.log_file
	env['RETRO_GET_FILE'] = a.get_file

	# in case missing REDIRECT_STATUS (because of apache)
	if not "REDIRECT_STATUS" in env:
	    env["REDIRECT_STATUS"] = "200"
	    
	a.run = subprocess.Popen([ pn ], env = env, cwd = d['cwd'],
				 stdin = subprocess.PIPE,
				 stdout = subprocess.PIPE)
	util.WriterThread(a.run.stdin, d['post']).start()
	a.stdout = util.ReaderThread(a.run.stdout)
	a.stdout.start()
	a.run_poll(self)
    def register(self):
	self.inputset.add(self.argsnode)
            
class PhpExit(mgrapi.Action):
    def __init__(self, actor, retnode):
	self.retnode = retnode
	super(PhpExit, self).__init__(retnode.name + ('phpexit',), actor)
    def redo(self):
	a = self.actor
	a.stdout.join()
	self.retnode.data = a.stdout.data
	a.run = None
	a.stdout = None
	#dbg.test('PHP output new', self.retnode.data)
	#dbg.test('PHP output old', self.retnode.origdata)
    def register(self):
	self.outputset.add(self.retnode)

## HTTP actor and actions

class HttpResponse(mgrapi.Action):
    def __init__(self, retnode):
	self.retnode = retnode
	actor = mgrutil.StatelessActor(retnode.name + ('htactor',))
	super(HttpResponse, self).__init__(retnode.name + ('htresp',), actor)
    def equiv(self):
	return self.retnode.data == self.retnode.origdata
    def redo(self):
	print 'HTTP response changed -- compensating action'
	diff = difflib.unified_diff(self.retnode.origdata.splitlines(True),
				    self.retnode.data.splitlines(True),
				    'original ' + str(self.retnode.name),
				    'new ' + str(self.retnode.name))
	print reduce(lambda x, y: x+y, diff)
    def register(self):
	self.inputset.add(self.retnode)

class BrowserPageActor(mgrapi.ActorNode):
    ## XXX: use client id in addition to client's page id
    def __init__(self, name, clientid, pageid):
        super(BrowserPageActor, self).__init__(name)
        self.clientid = clientid
        self.pageid = pageid
        self.checkpoints.add(mgrutil.InitialState(self.name + ('ckpt',)))
    @staticmethod
    def get(clientid, pageid, ts):
        assert(clientid is not None)
        name = ('bp', clientid, pageid)
        n = mgrapi.RegisteredObject.by_name(name)
        if n is None:
            n = BrowserPageActor(name, clientid, pageid)
            
            ## Add start action
            bp_start = BrowserPageStart(n)
            bp_start.tic = bp_start.tac = ts
            bp_start.connect()
            
            ## Add exit action -- not useful currently since the timestamps
            ## are PlusInfinity
            bp_exit = BrowserPageExit(n)
            bp_exit.tic = mgrapi.PlusInfinity()
            bp_exit.tac = mgrapi.PlusInfinity()
            bp_exit.connect()
            
        return n
    def rollback(self, cp):
        assert(isinstance(cp, mgrutil.InitialState))
        
        ## adjust the timestamps of BrowserPageExit -- hacky
        prev = None
        for x in sorted(self.actions):
            if isinstance(x, BrowserPageExit):
                x.tic = prev.tac + (0,1,)
                x.tac = prev.tac + (0,2,)
                return
            prev = x
    def waitreq(self, action):
        while True:
            ## check if browser exited -- replay is complete when browser exits
            if self.browser.poll() is not None:
                ## cancel all the remaining actions
                for a in self.actions:
                    if a > action: cancel_action(a)
                self.httpd.shutdown()
                return False

            if self.httpd.has_request():
                ## read the request
                req = self.httpd.get_request()
                d = {'env': {}, 'post': ''}
                for line in req:
                    (pids, tss, type, subtype, dataq) = line.split(' ')
                    data = urllib.unquote(dataq)            
                    if type == 'httpreq_env':
                        d['env'][subtype] = data
                    if type == 'httpreq_post':
                        d['post'] = data
                    if type == 'httpreq_cwd':
                        d['cwd'] = data

                ## check whether the req matches the next recorded req in line 
                ## for this page. if so, set the http req data
                nextact = min([a for a in self.actions if a > action])
                if isinstance(nextact, BrowserReqStart) and \
                   'HTTP_X_REQ_ID' in nextact.argsnode.origdata['env'] and \
                   'HTTP_X_REQ_ID' in d['env']:
                    r1 = nextact.argsnode.origdata['env']['HTTP_X_REQ_ID']
                    r2 = d['env']['HTTP_X_REQ_ID']
                    if r1 == r2:
                        nextact.httpreq = d
                        return True

                print 'got unmatched http request ', d
                if 'HTTP_X_PAGE_ID' in d['env'] and d['env']['HTTP_X_PAGE_ID'] == self.pageid:
                    print 'unmatched req has same page id. processing it...'

                    ts = util.between(action.tac, nextact.tic)

                    ## create a php actor and attach:
                    ##     BrowserReqStart -> htargs -> PhpStart, and
                    ##     PhpExit -> htret -> BrowserReqExit
                    php_pid = hash(time.time())  ## unique dummy 32-bit pid
                    p_actor = PhpActor.get(php_pid)

                    ## htargs
                    args = mgrutil.BufferNode(p_actor.name + ('htargs',), ts+(1,), d)

                    ## BrowserReqStart
                    brs = BrowserReqStart(self, args)
                    brs.tic = ts+(2,)
                    brs.tac = ts+(3,)
                    brs.connect()
                    brs.connect_script()
                    brs.httpreq = d

                    ## Phpstart
                    ph_call = PhpStart(p_actor, args)
                    ph_call.tic = ts+(4,)
                    ph_call.tac = ts+(5,)
                    ph_call.connect()

                    ## htret
                    ret = mgrutil.BufferNode(p_actor.name + ('htret',), ts+(6,), "")

                    ## PhpExit
                    ph_call = PhpExit(p_actor, ret)
                    ph_call.tic = ts+(7,)
                    ph_call.tac = ts+(8,)
                    ph_call.connect()

                    ## BrowserReqExit
                    bre = BrowserReqExit(self, ret)
                    bre.tic = ts+(9,)
                    bre.tac = ts+(10,)
                    bre.connect()
                
                    return True

            time.sleep(0.05)

class BrowserPageStart(mgrapi.Action):
    def __init__(self, actor):
        super(BrowserPageStart, self).__init__(actor.name + ('bpstart',), actor)
    def redo(self):
        ## start httpd and browser
	mode = open("/tmp/retro/note").read().strip()
	if mode == "apache":
	    self.actor.httpd = apache.Apache.get()
	elif mode == "httpd":
	    self.actor.httpd = httpd.Httpd.get()
	else:
	    raise "Mode should be one of {httpd|apache}"
        thisdir = os.path.dirname(os.path.abspath(__file__))
        if self.get_browser() == "firefox":
            self.actor.browser = subprocess.Popen(["python", thisdir + "/../../firefox/run.py",
                                                   "--cmd", "redo",
                                                   "--profile", "_repair_",
                                                   "--clientid", self.actor.clientid,
                                                   "--pageid", self.actor.pageid])
        else:
            self.actor.browser = subprocess.Popen(["python", thisdir + "/../test/webserv/workload.py",
                                                   "redo", self.actor.pageid])
            
        ## wait for next browser request
        self.actor.waitreq(self)

    def get_browser(self):
        for a in self.actor.actions:
            if isinstance(a, BrowserReqStart) and \
               'HTTP_USER-AGENT' in a.argsnode.origdata['env']:
                if a.argsnode.origdata['env']['HTTP_USER-AGENT'].find('Firefox') != -1:
                    return 'firefox'
                else:
                    return 'wget'
        return 'wget'

class BrowserPageExit(mgrapi.Action):
    def __init__(self, actor):
        super(BrowserPageExit, self).__init__(actor.name + ('bpexit',), actor)
    def redo(self):
        self.actor.httpd.shutdown()
        try:
            self.actor.browser.kill()
        except:
            pass

class BrowserReqStart(mgrapi.Action):
    def __init__(self, actor, argsnode):
        self.argsnode = argsnode
        super(BrowserReqStart, self).__init__(argsnode.name + ('breqstart',), actor)
    def redo(self):
        self.argsnode.data = self.httpreq if hasattr(self, 'httpreq') else self.argsnode.origdata
    def register(self):
        self.outputset.add(self.argsnode)
    def connect_script(self):
        pn = self.argsnode.data['env']['SCRIPT_FILENAME']
        self.script = PhpScript.get(pn)
        self.inputset.add(self.script)
        self.script.rset.add(self)

class BrowserReqExit(mgrapi.Action):
    def __init__(self, actor, retnode):
        self.retnode = retnode
        super(BrowserReqExit, self).__init__(retnode.name + ('breqexit',), actor)
    def equiv(self):
        ## XXX: do a true check for equivalence 
        return False 
    def redo(self):
	print "=" * 60
	print self.retnode.data
	print "-" * 60
        self.actor.httpd.send_response(self.retnode.data)
        self.actor.waitreq(self)
    def register(self):
        self.inputset.add(self.retnode)

## load into memory from log files

logdir = None
def set_logdir(pn):
    global logdir
    logdir = pn

def load(h, what):
    dbg.load('loading name', h)

    dbf = open(os.path.join(logdir, "db.log"))
    htf = open(os.path.join(logdir, "httpd.log"))

    q = {}
    for l in dbf.readlines():
	(pids, tss, type, db_path, dataq) = l.strip().split(' ')
	pid = int(pids)
	ts = int(tss)
	php = PhpActor.get(pid)
	data = urllib.unquote(dataq)
	if h:
	    if h[0] == 'php':
		if h[1] != pid: continue
	    elif h[0] == 'db':
		if h[1] != db_path: continue
	    else:
		continue

	if type == 'query':
	    qname = ('dbq', pid, ts)
	    if mgrapi.RegisteredObject.by_name(qname):
		q[pid] = None
		continue

	    q[pid] = DbQueryAction(qname)
	    q[pid].tic = (ts,2)
	    q[pid].argsnode = mgrutil.BufferNode(q[pid].name + ('args',), (ts,2),
						 { 'dir': db_path, 'query': data })
	    x = PhpDbCall(php, q[pid].argsnode)
	    x.tic = (ts,1)
	    x.tac = (ts,2)
	    x.connect()
	if type == 'query_result' and q[pid]:
	    q[pid].tac = (ts,2)
	    q[pid].retnode = mgrutil.BufferNode(q[pid].name + ('ret',), (ts,2), data)
	    q[pid].connect()
	    x = PhpDbRet(php, q[pid].retnode)
	    x.tic = (ts,2)
	    x.tac = (ts,3)
	    x.connect()

    for l in htf.readlines():
	x = l.strip().split(' ')
	if len(x) == 4:
	    (pids, tss, type, subtype) = x
	    dataq = ''
	else:
	    (pids, tss, type, subtype, dataq) = x
	pid = int(pids)
	ts = int(tss)
	data = urllib.unquote(dataq)
	if h:
	    if h[0] != 'php' or h[1] != pid: continue

	p_actor = PhpActor.get(pid)
	if type == 'httpreq_start':
	    qname = p_actor.name + ('htargs',)
	    if mgrapi.RegisteredObject.by_name(qname):
		q[pid] = None
		continue

	    q[pid] = {'env': {}, 'post': ''}
	    an = mgrutil.BufferNode(qname, (ts,2), q[pid])

	    ph_call = PhpStart(p_actor, an)
	    ph_call.tic = (ts,5)
	    ph_call.tac = (ts,6)
	    ph_call.connect()
	if not q[pid]: continue

	if type == 'httpreq_env':
	    q[pid]['env'][subtype] = data
            if subtype == 'HTTP_X_CLIENT_ID':
                ## XXX: relies on CLIENT_ID coming before PAGE_ID
                p_actor.clientid = data
            if subtype == 'HTTP_X_PAGE_ID':
                p_actor.pageid = data

        if type == 'httpreq_end':
                qname = p_actor.name + ('htargs',)
                an = mgrapi.RegisteredObject.by_name(qname)

                p_actor_ts = min(p_actor.actions).tic[0]
                bp = BrowserPageActor.get(p_actor.clientid, p_actor.pageid, (p_actor_ts, 0))
                
                breq_start = BrowserReqStart(bp, an)
                breq_start.tic = (p_actor_ts, 3)
                breq_start.tac = (p_actor_ts, 4)
                breq_start.connect()
                breq_start.connect_script() 
            
	if type == 'httpreq_post':
	    q[pid]['post'] = data
	if type == 'httpreq_cwd':
	    q[pid]['cwd'] = data
	if type == 'httpresp':
	    an = mgrutil.BufferNode(p_actor.name + ('htret',), (ts,2), data)

	    ht_call = HttpResponse(an)
	    ht_call.tic = (ts,7)
	    ht_call.tac = (ts,8)
	    ht_call.connect()

	    ph_call = PhpExit(p_actor, an)
	    ph_call.tic = (ts,3)
	    ph_call.tac = (ts,4)
	    ph_call.connect()
	    
	    bp = BrowserPageActor.get(p_actor.clientid, p_actor.pageid, ts)
	    breq_end = BrowserReqExit(bp, an)
	    breq_end.tic = (ts,5)
	    breq_end.tac = (ts,6)
	    breq_end.connect()

mgrapi.LoaderMap.register_loader('php', load)
mgrapi.LoaderMap.register_loader('db',  load)

