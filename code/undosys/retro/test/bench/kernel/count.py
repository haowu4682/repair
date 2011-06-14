#!/usr/bin/python

import os
import sys

root = sys.argv[1]

total = 0.0

def calc_size(c):
    if c == "M" : return 10**6
    if c == "G" : return 10**9
    if c == "K" : return 10**3
    raise Exception(c)

aux = os.popen("du -h %s/retro-aux" % root).readlines()[-1].split()
log = os.popen("du -h %s/retro-log" % root).readlines()[-1].split()
for (i,size) in enumerate(aux + log):
    if i % 2 == 1:
        continue
    total += float(size[:-1]) * calc_size(size[-1])

print "\n".join(aux + log)
print total

