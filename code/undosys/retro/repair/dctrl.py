#! /usr/bin/python

import os
import sys
import dbg
import socket
import pickle

from twisted.internet.protocol import Factory, Protocol, ClientFactory
from twisted.internet  import reactor
from twisted.protocols import basic

import osloader
import attacker
import mgrapi
import ctrl
import time
import networkmgr
import mgrutil
import threading
import sockmgr

from collections import defaultdict

def The(singleton, *args) :
    if not hasattr(singleton, "__instance") :
        singleton.__instance = singleton(*args)

    return singleton.__instance

# default port of dctrl daemon
PORT = 8080

class CtrlFSM(ctrl.BasicRepairCtrl):
    def __init__(self):
        self.curstate = ""
        self.states   = {}

        # info for ctrl
        super(CtrlFSM, self).__init__({})

    # FSM related
    def set_state(self, name):
        if self.curstate:
            dbg.state("[state] %s -> %s" % (self.curstate, name))
        self.curstate = name

    def add_state(self, name, trans):
        if name not in self.states:
            self.states[name] = {}
        self.states[name].update(trans)

    def get_trans(self):
        return self.states.get(self.curstate, {})

    def repair(self):
        # dbg.trace("cs:%s" % str(self.cur_state))
        # dbg.trace("dirty:%s" % str(self.dirty))
        # dbg.trace("action:%s" % str(ctrl.pick_action(self.cur_state, self.dirty)))

        return self.repair_next_action()

def rtn_cmd(fd, args):
    fd.write(pickle.dumps(args))

def cmd_repair(fd, args):
    """repairing attacks: hint attacker"""
    if len(args) < 1:
        cmd_help(fd)
        return

    hint = args[0]
    pick = "pick_attack_execve" if len(args) < 2 else args[1]
    name = attacker.find_attack_node(logd, pick, hint)

    attack = mgrapi.RegisteredObject.by_name_load(name)
    if attack is None:
        dbg.error('missing attack node:', name)
        return

    chkpt = max(c for c in attack.checkpoints if c < min(attack.actions))

    assert chkpt
    assert len(attack.actions) == 1

    for a in attack.actions:
        dbg.info("cancel: %s" % a.argsnode)
        a.argsnode.data = None
        a.cancel = True

    dbg.info("pick:", chkpt)

    The(CtrlFSM).set_state("repairing")
    ctrl.repair(attack, chkpt)
    The(CtrlFSM).set_state("init")

    rtn_cmd(fd, "done")

def cmd_help(fd, args=None):
    rtn = "usage:\n"
    for s, cmds in The(CtrlFSM).states.iteritems():
        rtn += "  in `%s' state:\n" % s
        for c in cmds:
            rtn +="    - %-15s : %s\n" % (c, cmds[c].__doc__)

    rtn_cmd(fd, rtn)

def cmd_rollback(fd, args):
    """rollback request from other dctrls"""

    if args[0] == "socket":
        sip   = args[1]
        sport = args[2]
        dport = args[3]

        # network node
        for p in sport:
            o = networkmgr.NetworkNode.lookup(sip, p)
            assert o
            dbg.trace("found %s:%s for %s" % (sip, p, o))
            dbg.trace("r:%s" % str(o.readers))
            dbg.trace("w:%s" % str(o.writers))

            # make sock behave like slave
            o.master = False
            chk = min(o.checkpoints)
            The(CtrlFSM).rollback(o, chk)

        The(CtrlFSM).set_state("repairing")

        # intentionally sleep to yield python thread
        time.sleep(1)

        rtn_cmd(fd, "done")

def cmd_redo(fd, args):
    pass

def cmd_ready(fd, args):
    """index osloader: ready for what, info"""

    dbg.trace("loading: %s" % str(args))
    if args[0] == "socket":
        # XXX
        # for now, load everything
        osloader.load(None,None)
        dbg.trace("#%d objs loaded" % len(mgrapi.RegisteredObject.all()))

        # now ready for undoing/redoing
        The(CtrlFSM).set_state("ready")

    rtn_cmd(fd, "done")

def cmd_invalid(fd, args):
    rtn_cmd(fd, "invalid data(%s)\n" % args)

def cmd_state(fd, args):
    rtn_cmd(fd, "%s" % The(CtrlFSM).curstate)

The(CtrlFSM).add_state("init", {"repair"   : cmd_repair,
                                "ready"    : cmd_ready })

The(CtrlFSM).add_state("ready",{"repair"   : cmd_repair,
                                "rollback" : cmd_rollback,
                                "redo"     : cmd_redo })

The(CtrlFSM).add_state("repairing",{})

The(CtrlFSM).set_state("init")

global gcmds
gcmds = {"help"     : cmd_help,
         "state"    : cmd_state}

class CmdGate(Protocol):
    def dataReceived(self, data):
        global gcmds

        toks = pickle.loads(data)
        cmds = The(CtrlFSM).get_trans()

        dbg.trace("[%s] got: %s" % (The(CtrlFSM).curstate, toks))

        if len(toks) == 0:
            cmd_invalid(self.transport, toks)
            return

        f = cmds.get(toks[0], None)
        if f is None:
            f = gcmds.get(toks[0], None)

        if f:
            f(self.transport, toks[1:])
        else:
            cmd_invalid(self.transport, toks)

def daemon_main():
    f = Factory()
    f.protocol = CmdGate
    reactor.listenTCP(PORT, f)
    reactor.run()

def send_request(ip, port, *cmds):
    try:
        f = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        f.connect((ip, port))
        f.send(pickle.dumps(cmds))
        return pickle.loads(f.recv(1024))
    except:
        pass
    return "connection refused"

global logd
logd = "/tmp"

# repair event generators
class Worker(threading.Thread):
    def run(self):
        while True:
            if The(CtrlFSM).curstate == "repairing":
                if The(CtrlFSM).repair() == False:
                    The(CtrlFSM).set_state("ready")
            else:
                # repsonse time
                time.sleep(0.1)

if __name__ == '__main__':
    if "-d" in sys.argv:
        if len(sys.argv) > 2:
            logd = sys.argv[-1]
        osloader.set_logdir(logd)
        Worker().start()
        daemon_main()
    else:
        rtn = send_request(os.popen("hostname -I").read(), PORT, *sys.argv[1:])
        print "=" * 60
        print "%s" % rtn
        print "=" * 60
