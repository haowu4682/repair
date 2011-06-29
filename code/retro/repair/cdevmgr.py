import mgrapi, mgrutil

class CdevNode(mgrapi.DataNode):
    def __init__(self, name, fd):
	super(CdevNode, self).__init__(name)
	self.fd = fd
	self.checkpoints.add(mgrutil.InitialState(name + ('init',)))

    @staticmethod
    def get(fd):
	name = (fd,)
	n = mgrapi.RegisteredObject.by_name(name)
	if n is None:
	    n = CdevNode(name, fd)
	return n

    def rollback(self, c):
	assert(isinstance(c, mgrutil.InitialState))
	pass
