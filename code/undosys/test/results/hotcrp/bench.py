#! /usr/bin/env python

import os
import sys

import warnings
warnings.filterwarnings("ignore")

def parse( cmd ) :
    (m,s) = cmd.split()[-1].split("m")
    s = s[:-1]
    return float(m)*60.0 + float(s)

def time( cmd ) :
    rtn = os.popen3( '/usr/bin/time %s' % cmd )[2].readlines()
    rtn = rtn[0].split()[2].replace("elapsed", "").split(":")
    return int(rtn[0]) * 60 + float(rtn[1])

sum_add_r = 0.0
sum_add_u = 0.0
sum_add_s = 0.0

reps = int(sys.argv[1])

total = 0.0
for i in range(reps) :
    r = time(" ".join(sys.argv[2:]))
    total += r

print "reps:%d\telapsed:%f" % (reps,total)
