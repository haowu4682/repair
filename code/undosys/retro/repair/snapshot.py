import graph
import logging
import record
import pickle
import kutil
import shutil
import os
import os.path as path

freshroot = "/mnt/retro/fresh"
snaproot  = "/mnt/retro/trunk"

def record_by_ticket(ticket, conn) :
	if ticket <= 0:
		return None
	
	c = conn.cursor()
	c.execute("select name from R where ticket=?", (ticket,))
	name = c.fetchone()[0]
	c.execute("select data from V where name=?", (name,))
	obdata = c.fetchone()[0]
	assert c.fetchone() is None
	c.close()
	
	return pickle.loads(obdata)

def copyfile(src, dst):
	logging.info("%s <- %s" % (dst, src))
	shutil.copyfile(src, dst)

def find_snapshot(ticket, inode, conn):
	logging.info("finding %s@%d" % (inode, ticket))

	# fetch from the snapshot archive
	try:
		os.mkdir(path.join(snaproot, ".snap"))
	except:
		pass
	snapfile = path.join(snaproot, ".snap", "%s.%d" % (inode.ino, ticket))
	if path.exists(snapfile):
		return snapfile

	t = ticket
	while t >= 0:
		# fetch from the fresh root (initial check point)
		if t == 0:
			return kutil.iget(freshroot, inode.ino)

		# search history
		t = t - 1
		
		# apply current system calls
		r = record_by_ticket(t, conn)
		
		# filtering 
		if r.name == "open" and r.ret.inode == inode:
			if r.args["flags"] & os.O_TRUNC:
				logging.info("@%d <- empty %s@%d" % (ticket, inode, r.ticket))
				open(snapfile, "w").close()
			else:
				logging.info("@%d <- %s@%d" % (ticket, inode, r.ticket))
				logging.info("	by %s" % r)

				parent = find_snapshot(r.ticket, inode, conn)
				copyfile(parent, snapfile)

		elif r.name == "write" and r.args["fd"].inode == inode:
			logging.info("@%d <- %s@%d" % (ticket, inode, r.ticket))
			logging.info("	by %s" % r)

			parent = find_snapshot(r.ticket, inode, conn)
			copyfile(parent, snapfile)

			offset = r.args["fd"].offset

			f = open(snapfile, "r+")
			# append
			if offset == -1:
				f.seek(0, os.SEEK_END)
			else:
				f.seek(offset)
			f.write(r.args["buf"])
			f.close()

		elif r.name == "unlink":
			# TODO restore the file from the .inodes
			pass
		else:
			continue

		return snapfile
