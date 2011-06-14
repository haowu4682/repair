import graph
import sysarg

class DirDataNode(graph.DataNode):
	pass

graph.register_mgr(sysarg.dentry, DirDataNode)

