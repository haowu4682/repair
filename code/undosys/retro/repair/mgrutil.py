import mgrapi

class InitialState(mgrapi.TimeInterval, mgrapi.TupleName):
    def __init__(self, name):
	self.name = name
	t = mgrapi.MinusInfinity()
	super(InitialState, self).__init__(t, t)

class StatelessActor(mgrapi.ActorNode):
    def __init__(self, name):
	super(StatelessActor, self).__init__(name)
	self.checkpoints.add(InitialState(name + ('ckpt',)))
    def rollback(self, cp):
	assert(isinstance(cp, InitialState))

class BufferCheckpoint(mgrapi.TimeInterval, mgrapi.TupleName):
    def __init__(self, name, t, data):
	self.name = name
	self.data = data
	super(BufferCheckpoint, self).__init__(t, t)

class BufferNode(mgrapi.DataNode):
    def __init__(self, name, t, origdata):
	super(BufferNode, self).__init__(name)
	self.origdata = origdata
	self.data = origdata
	self.checkpoints.add(BufferCheckpoint(name + ('ckpt0',),
					      mgrapi.MinusInfinity(),
					      None))
	self.checkpoints.add(BufferCheckpoint(name + ('ckpt1',),
					      t, origdata))
    def rollback(self, cp):
	assert(isinstance(cp, BufferCheckpoint))
	self.data = cp.data

