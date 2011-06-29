#! /usr/bin/env python

import os
import sys
import time

print sys.argv

reps = int(sys.argv[1])

beg = time.time()
for i in range(reps) :
    cmd = " ".join(sys.argv[2:])
    print cmd
    os.system(cmd)
end = time.time() - beg


open("/tmp/dump","w").write("reps:%d\telapsed:%s\n" % (reps,end))
