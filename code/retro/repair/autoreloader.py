#! /usr/bin/python

import os
import sys
import time

pid  = None
last = 0

if __name__ == '__main__':
    f = sys.argv[1]
    while True:
        stat = os.stat(f).st_mtime
        if last < stat:
            print ">>> %s" % time.ctime()
            last = stat
            if pid != None:
                os.system("kill -9 %d" % pid)
                time.sleep(3)

            pid = os.fork()
            if pid == 0:
                os.execvp(sys.argv[1], sys.argv[1:])
                exit(1)

        time.sleep(1)
