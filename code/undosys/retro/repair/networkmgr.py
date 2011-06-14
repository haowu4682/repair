import mgrapi, mgrutil
import dbg
import socket
import functools
import time
import os

class NetworkNode(mgrapi.DataNode):
    def __init__(self, name, inode):
        super(NetworkNode, self).__init__(name)
        self.master     = True
        self.dip        = inode.dip
        self.dport      = inode.dport
        self.sport      = set()
        self.inodes     = set()
        self.checkpoints.add(mgrutil.InitialState(name + ('init',)))

    @staticmethod
    def lookup(dip, dport):
        name = ('network', dip, dport)
        return mgrapi.RegisteredObject.by_name(name)
        
    @staticmethod
    def get(inode):
        name = ('network', inode.dip, inode.dport)
        n = mgrapi.RegisteredObject.by_name(name)
        if n is None:
            n = NetworkNode(name, inode)
        n.sport.add(inode.sport)
        n.inodes.add(inode)
        return n

    def rollback(self, c):
        assert(isinstance(c, mgrutil.InitialState))

        import dctrl
        if self.master and isinstance(c, mgrutil.InitialState):
                # explicitly load readers & writers
                for r in self.readers | self.writers:
                    pass
                
                dbg.remote("@@@ %s->%s:%s" % (self.sport, self.dip, self.dport))

                # create request function
                req = functools.partial(dctrl.send_request, self.dip, dctrl.PORT)

                # XXX get my ip
                ip = os.popen("hostname -I").read().strip()
                
                # check server state
                if req("state") == "init":
                    # load system
                    req("ready", "socket", ip, self.dport, self.sport)
                    
                # check again for sure
                assert req("state") == "ready"

                # rollback and propagate
                req("rollback", "socket", ip, self.sport, self.dport)
                
