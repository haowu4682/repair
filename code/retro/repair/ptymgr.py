import mgrapi, mgrutil

class PtyNode(mgrapi.DataNode):
    def __init__(self, name, ptsno, gen):
	super(PtyNode, self).__init__(name)
	self.ptsno = ptsno
	self.gen = gen
	self.buf = None
	## XXX where does the origdata for the pty come from?
	self.checkpoints.add(mgrutil.InitialState(name + ('init',)))
    @staticmethod
    def get(ptsno, gen):
	name = ('pts', ptsno, gen)
	n = mgrapi.RegisteredObject.by_name(name)
	if n is None:
	    n = PtyNode(name, ptsno, gen)
	return n
    def rollback(self, c):
	assert(isinstance(c, mgrutil.InitialState))
	self.buf = ''

## XXX add a PtyReader action (with a StatelessActor) to compute diffs
## XXX model bi-directional data flow

