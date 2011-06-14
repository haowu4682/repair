import mgrapi, mgrutil

class FifoNode(mgrapi.DataNode):
    def __init__(self, name, fd):
	super(FifoNode, self).__init__(name)
	self.fd = fd
	self.checkpoints.add(mgrutil.InitialState(name + ('init',)))
	
    @staticmethod
    def get(fd):
	name = (fd,)
	n = mgrapi.RegisteredObject.by_name(name)
	if n is None:
	    n = FifoNode(name, fd)
	return n
    
    def rollback(self, c):
	assert(isinstance(c, mgrutil.InitialState))
	self.buf = ''

