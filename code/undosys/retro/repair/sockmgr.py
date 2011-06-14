import mgrapi, mgrutil
import dbg
import socket
import functools
import time
import os

class SocketNode(mgrapi.DataNode):
    def __init__(self, name, inode):
        super(SocketNode, self).__init__(name)
        self.dev        = inode.dev
        self.ino        = inode.ino
        self.dip        = inode.dip   if hasattr(inode, "dip"  ) else None
        self.dport      = inode.dport if hasattr(inode, "dport") else None
        self.sport      = inode.sport if hasattr(inode, "sport") else None

        self.checkpoints.add(mgrutil.InitialState(name + ('init',)))

    @staticmethod
    def get(inode):
        name = ('sock', inode.dev, inode.ino)
        n = mgrapi.RegisteredObject.by_name(name)
        if n is None:
            n = SocketNode(name, inode)

        if hasattr(inode, "dip"):
            n.dip = inode.dip
        if hasattr(inode, "dport"):
            n.dport = inode.dport
        if hasattr(inode, "sport"):
            n.sport = inode.sport

        return n

    def __repr__(self):
        name = ':'.join(map(str, self.name))
        if self.sport and self.dip and self.dport:
            name += ":%s->%s@%s" % (self.sport, self.dip, self.dport)
        return name

    def rollback(self, c):
        assert(isinstance(c, mgrutil.InitialState))
        return
