## TODO:
## support multiple browser sessions

import os, json, mgrapi, mgrutil

events = {
    # Keyboard events
    "keydown"  : "KeyboardEvent",
    "keypress" : "KeyboardEvent",
    "keyup"    : "KeyboardEvent",

    # Mouse events
    "click"         : "MouseEvent",
    "contextmenu"   : "MouseEvent",
    "dblclick"      : "MouseEvent",
    "DOMMouseScroll": "MouseEvent",
    "drag"          : "MouseEvent",
    "dragdrop"      : "MouseEvent",
    "dragend"       : "MouseEvent",
    "dragenter"     : "MouseEvent",
    "dragexit"      : "MouseEvent",
    "draggesture"   : "MouseEvent",
    "dragover"      : "MouseEvent",
    "mousedown"     : "MouseEvent",
    "mousemove"     : "MouseEvent",
    "mouseout"      : "MouseEvent",
    "mouseover"     : "MouseEvent",
    "mouseup"       : "MouseEvent",
}

class LocationCkpt(mgrapi.TimeInterval, mgrapi.TupleName):
    def __init__(self, ts, newurl):
        self.name = ('LocationCkpt', newurl)
        self.newurl = newurl
        super(LocationCkpt, self).__init__(ts, ts)

class UserActor(mgrapi.ActorNode):
    theone = None
    def __init__(self):
        super(UserActor, self).__init__(('user',))
        self.checkpoints.add(mgrutil.InitialState(self.name + ('ckpt', )))
    @staticmethod
    def get():
        if UserActor.theone == None:
            UserActor.theone = UserActor()
        return UserActor.theone
    
class UserInput(mgrapi.Action):
    def __init__(self, actor, useraction):
        super(UserInput, self).__init__(useraction.name + ('input',), actor)
        self.action = useraction
    def register(self):
        self.inputset.add(self.action)
        self.outputset.add(PageDataNode.get(self.action.data['frameId'],
                                            self.action.data['pageId']))

class FrameActor(mgrapi.ActorNode):
    def __init__(self, name, frameid):
        super(FrameActor, self).__init__(name)
        self.frameid = frameid
        self.checkpoints.add(mgrutil.InitialState(self.name + ('ckpt', )))
    @staticmethod
    def get(frameid):
        name = ('frame', frameid)
        n = mgrapi.RegisteredObject.by_name(name)
        if n is None:
            n = FrameActor(name, frameid)
        return n
    
class FramePageLoad(mgrapi.Action):
    def __init__(self, actor, req):
        super(FramePageLoad, self).__init__(req.name + ('load',), actor)
        self.req = req
    def register(self):
        self.inputset.add(FrameLocationNode.get(self.actor.frameid))
        self.outputset.add(self.req)
    
class HTTPResponse(mgrapi.Action):
    def __init__(self, actor, resp):
        super(HTTPResponse, self).__init__(resp.name + ('resp_action',), actor)
        self.resp = resp
    def register(self):
        self.inputset.add(self.resp)
        self.outputset.add(PageDataNode.get(self.actor.frameid, self.resp.data['pageId']))
    
class FrameLocationNode(mgrapi.DataNode):
    def __init__(self, name, frameid):
        super(FrameLocationNode, self).__init__(name)
        self.frameid = frameid
        self.checkpoints.add(mgrutil.InitialState(self.name + ('ckpt', )))
    @staticmethod
    def get(frameid):
        name = ('floc', frameid)
        n = mgrapi.RegisteredObject.by_name(name)
        if n is None:
            n = FrameLocationNode(name, frameid)
        return n

class PageDataNode(mgrapi.DataNode):
    def __init__(self, name, frameid, pageid):
        super(PageDataNode, self).__init__(name)
        self.frameid = frameid
        self.pageid = pageid
        self.checkpoints.add(mgrutil.InitialState(self.name + ('ckpt', )))
    @staticmethod
    def get(frameid, pageid):
        name = ('pdata', frameid, pageid)
        n = mgrapi.RegisteredObject.by_name(name)
        if n is None:
            n = PageDataNode(name, frameid, pageid)
        return n
    
def file_read(fn):
    with open(fn) as fp: return fp.read()

debugflag = False
def debug(msg):
    if debugflag:
        print msg

logdir = None
def set_logdir(pn):
    global logdir
    logdir = pn
    
def load(h, what):
    currentPages = {}
    
    ## Read the logs
    maxSessionId = int(file_read(logdir + "/sessionId"))
    debug("++ maxSessionId = %d" % maxSessionId)
    
    logFilePrefix = logdir + "/logfile-"
    
    ts = 0
    for sessionId in xrange(maxSessionId):
        debug("++ Processing session %d" % sessionId)
        
        logFileIdx = 0
        while True:
            logfile = "%s%d-%d" % (logFilePrefix, sessionId, logFileIdx)
            logFileIdx += 1
            if not os.path.exists(logfile):
                break

            debug("++++ Processing logfile " + logfile)
            logfp = open(logfile)
            log = json.load(logfp)
            logfp.close()
            
            ## on FrameId:
            ##     create a new FrameActor and FrameLoc
            ##     
            ## on HTTPRequest:
            ##     if isMainDoc:
            ##         new PageData node corresponding to the new page (with a ckpt)
            ##         ckpt FrameLoc, (XXX: ReferAction writes page URL to FrameLoc)
            ##         PageLoad reads from FrameLoc and writes to RequestData
            ##     else:
            ##         HTTPReqAction reads from PageData and writes to RequestData
            ##       
            ##     HTTPRespAction reads from ResponseData and writes to PageData
            ##     
            ## on PageId: nothing
            ## 
            ## on TabId: nothing
            ## 
            ## on any user input event (keydown, mouseover, etc.,):
            ##     add a new UserInput to the UserActor
            ##     connect the event to PageData
            for entry in log:
                ts += 1
                
                debug("")
                debug(str(entry))
                    
                if entry['evType'] == 'FrameId':
                    debug("++++ Processing frame " + str(entry['id']))
                    f = FrameActor.get(entry['id'])
                    fl = FrameLocationNode.get(entry['id'])
                elif entry['evType'] == 'PageId':
                    debug("++++ Processing page" + str(entry['id']))
                    pd = PageDataNode.get(entry['parentFrame'], entry['id'])
                elif entry['evType'] == 'HTTPRequest':
                    debug("++++ Processing HTTP request")
                    ## XXX: extend HTTPRequest and HTTPResponse to support
                    ## non-pageload http requests 
                    if entry['isMainDoc']:
                        f = FrameActor.get(entry['frameId'])
                        pd = PageDataNode.get(entry['frameId'], entry['pageId'])

                        floc = FrameLocationNode.get(entry['frameId'])
                        floc.checkpoints.add(LocationCkpt((ts,0), entry['URI']['asciiSpec']))
                        
                        req_data = mgrutil.BufferNode(f.name + ('htreq', entry['pageId']),
                                                      (ts,3), entry)
                        
                        pl = FramePageLoad(f, req_data)
                        pl.tic = (ts,1)
                        pl.tac = (ts,2)
                        pl.connect()
                elif entry['evType'] == 'HTTPResponse':
                    debug("++++ Processing HTTP Response")
                    if entry['isMainDoc']:
                        f = FrameActor.get(entry['frameId'])
                        pd = PageDataNode.get(entry['frameId'], entry['pageId'])
                        
                        resp_data = mgrutil.BufferNode(f.name + ('htresp', entry['pageId']),
                                                       (ts,3), entry)
                        
                        htresp = HTTPResponse(f, resp_data)
                        htresp.tic = (ts,1)
                        htresp.tac = (ts,2)
                        htresp.connect()
                elif entry['evType'] == 'event' and entry['type'] in events:
                    ## XXX: add info about event into event name (eg: key pressed
                    ## mouse loc, DOM element)
                    debug("++++ Processing %s event" % entry['type'])
                    ua = UserActor.get()
                    useraction = mgrutil.BufferNode((entry['type'], ts),
                                                    (ts,3), entry)
                    inp = UserInput(ua, useraction)
                    inp.tic = (ts,1)
                    inp.tac = (ts,2)
                    inp.connect()
