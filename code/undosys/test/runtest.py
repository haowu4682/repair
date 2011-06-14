#!/usr/bin/python
import os, os.path
import subprocess
import time
import ctypes
import imp
import inspect
import sqlite3
import unittest

pwd = os.path.dirname(os.path.realpath(__file__))
root = "/mnt/undofs/d"
logger = open(pwd + "/test.log", "w")

utmp = imp.load_source("utmp", pwd + "/../graph/utmp.py")

def banner():
	stack = inspect.stack()
	p = inspect.getframeinfo(stack[1][0]).function
	pp = inspect.getframeinfo(stack[2][0]).function
	del stack
	s = pp + " (" + p + ")"
	logger.write("\n\n" + s + "\n" + '='*len(s) + "\n\n")
	logger.flush()

def file_blocks(fn, blksize):
	r = []
	with open(fn, "rb") as f:
		while True:
			s = f.read(blksize)
			if len(s) == 0:
				break
			assert(len(s) == blksize)
			r.append(s)
	return r

def file_lines(fn):
	with open(fn, "rb") as f:
		return [line.rstrip("\n") for line in f]

class UndoTest(unittest.TestCase):

	def attack(self, sh):
		os.chdir(pwd)
		banner()
		r = subprocess.call("./" + sh, stdout=logger, stderr=logger)
		self.assertEqual(r, 0)
		os.chdir(root)

	def repair(self):
		os.chdir(pwd)
		os.chdir("../graph")
		banner()
		r = subprocess.call(["make", "repair"], stdout=logger, stderr=logger)
		self.assertEqual(r, 0)
		os.chdir(root)

	def mount_repair(self):
		subprocess.call(pwd + "/mount.sh")
		try:
			self.repair()
		finally:
			subprocess.call(pwd + "/umount.sh")

	def test_bdb(self):
		self.attack("setup-bdb.sh")
		exp0 = ["Alice => Atlanta", "Alice => Atlanta", "Bob => District 9"]
		exp1 = list(exp0)
		exp1[-1] = "Bob => Boston"
		with open("foo.txt") as f:
			act0 = [s.rstrip("\n") for s in f.readlines()]
		self.assertEqual(exp0, act0)
		self.repair()
		with open("foo.txt") as f:
			act1 = [s.rstrip("\n") for s in f.readlines()]
		self.assertEqual(exp1, act1)

	def test_rename(self):
		data = range(10)
		self.attack("setup-mv.sh")
		with open("test.txt") as f:
			r0 = [int(s.strip("\n")) for s in f.readlines()]
		self.assertNotEqual(data, r0)
		self.repair()
		with open("test.txt") as f:
			r1 = [int(s.strip("\n")) for s in f.readlines()]
		self.assertEqual(data, r1)
		self.assertFalse(os.path.exists("foo.txt"))

	def test_login(self):
		sz = ctypes.sizeof(utmp.utmp)
		self.attack("setup-utmp.sh")
		self.assertEqual(1 * sz, os.path.getsize("var/run/utmp"))
		self.assertEqual(9 * sz, os.path.getsize("var/log/wtmp"))
		self.repair()
		self.assertEqual(1 * sz, os.path.getsize("var/run/utmp"))
		self.assertEqual(6 * sz, os.path.getsize("var/log/wtmp"))
		# eve shouldn't be there
		t = lambda s: ctypes.cast(ctypes.create_string_buffer(s), \
			ctypes.POINTER(utmp.utmp)).contents
		for e in map(t, file_blocks("var/log/wtmp", sz)):
			if e.ut_type == utmp.USER_PROCESS:
				self.assertEqual("alice", e.ut_user)
	
	def test_useradd(self):
		self.attack("setup-useradd.sh")
		lines0 = [x.split(":") for x in file_lines("etc/passwd")]
		n0 = [line[0] for line in lines0]
		u0 = [line[2] for line in lines0]
		self.assertEqual(["alice", "eve", "bob"], n0[-3:])
		del n0[-2]
		del u0[-2]
		self.mount_repair()
		lines1 = [x.split(":") for x in file_lines("etc/passwd")]
		n1 = [line[0] for line in lines1]
		u1 = [line[2] for line in lines1]
		self.assertEqual(n0, n1)
		self.assertEqual(u0, u1)

	def test_shadow(self):
		self.attack("setup-shadow.sh")
		alice0 = file_lines("etc/passwd")[-2]
		t0 = file_lines("time.txt")[0]
		self.repair()
		alice1 = file_lines("etc/passwd")[-1]
		t1 = file_lines("time.txt")[0]
		self.assertEqual(alice0, alice1)
		# make sure NO re-execution
		self.assertEqual(t0, t1)

	def test_bash(self):
		self.attack("setup-bash.sh")
		self.assertTrue(os.path.exists('foo.txt'))
		self.assertTrue(os.path.exists('bar.txt'))
		lines0 = file_lines('foo.txt')
		self.assertEqual(len(lines0), 2)
		self.repair()
		self.assertTrue(os.path.exists('foo.txt'))
		self.assertFalse(os.path.exists('bar.txt'))
		lines1 = file_lines('foo.txt')
		self.assertEqual(len(lines1), 1)
		self.assertEqual(lines0[0], lines1[0])

	def test_sqlite(self):
		self.attack("setup-sqlite.sh")
		db = sqlite3.connect("test.db")
		self.assertEqual(5, len(db.execute("select * from undo").fetchall()))
		db.close()
		self.repair()
		db = sqlite3.connect("test.db")
		self.assertEqual(6, len(db.execute("select * from undo").fetchall()))
		db.close()


if __name__ == "__main__":
	assert(os.getuid() == 0)
	logger.write(time.asctime() + "\n")
	suite = unittest.TestLoader().loadTestsFromTestCase(UndoTest)
	unittest.TextTestRunner(verbosity=2).run(suite)
#	unittest.main()
