#!/usr/bin/python

from syscall import syscalls
import osloader

import os.path
import record
import sys

from curses.ascii import isgraph

stats = {}
pids  = {}

def main(fn):
	# f = record.zopen(fn)
	f = open(fn)
	base = os.path.basename(f.name)
	if base.startswith("syscall"):
		read = record.read_syscall
	elif base.startswith("kprobes"):
		read = record.read_kprobes
	else:
		raise "unrecognized file"

	print "=" * 60
	print f.name
	print "-" * 60

	cache = (-1, 0)
	while True:
		try:
			pos = f.tell()
			r = read(f)
			length = f.tell() - pos
			ts = ".".join([str(e) for e in r.ts])
			print("%08X:%-4s " % (pos, length) + str(r.sid & 0xFFFFFF) + " " + str(r))
			if read == record.read_syscall:
				key = (r.nr, r.usage)
				size, count, time, time_cnt = stats.setdefault(key, (0, 0, 0, 0))

				# exit
				if (r.usage & EXIT) and cache[0] != -1:
					if r.nr == cache[0]:
						time_cnt += 1
						time += r.ts[0] - cache[1]

				stats[key] = (size + length, count + 1, time, time_cnt)
				cache = (r.nr, r.ts[0])

				if not r.pid in pids:
					pids[r.pid] = r

		except EOFError:
			break
		except IOError:
			length = 60
			width  = 16

			f.seek(pos - width * length / 5, os.SEEK_SET)

			pos = f.tell()
			dump = f.read(width * length)
			print "-" * 5, "ERROR", "-" * 70
			for l in range(length):
				print "%08X" % (pos + l * width) + "| ",
				print " ".join("%02X" % ord(c) for c in dump[l*width:(l+1)*width]),
				print "|",
				print "".join("%c" % ord(c) if isgraph(ord(c)) else "."
					      for c in dump[l*width:(l+1)*width])
			print "=" * 5, "ERROR", "=" * 70

		except KeyboardInterrupt:
			break

def last():
	if stats:
		results = [(v, k) for k, v in stats.items()]
		results.sort(reverse=True)
		sys.stderr.write("\n     size   cnt   ave       time  #cnt     syscall\n")
		sys.stderr.write("=" * 70 + "\n")
		syms = ["", ">", "<", "<>"]
		for (size, count, time, time_cnt), (nr, usage) in results:
			name = syscalls[nr].name
			avr_time = (time/time_cnt if time_cnt != 0 else time)
			sys.stderr.write("%9s %5s %5s %10s %5s %5s %s\n" \
						 % (size, count, size/count,
						    avr_time, time_cnt,
						    syms[usage], name))
		sys.stderr.write("=" * 70 + "\n")

		sys.stderr.write("pids:\n")
		for (pid,r) in pids.iteritems():
			sys.stderr.write("  %s:%s\n" % (pid, r))

if __name__ == "__main__":
	d = True
	for fn in sys.argv[1:]:
		if d:
			osloader.set_logdir(os.path.dirname(fn))
			d = False
		main(fn)
	last()
