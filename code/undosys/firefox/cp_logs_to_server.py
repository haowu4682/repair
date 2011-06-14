#!/usr/bin/python

import os, sys, ff_utils

def file_read(fn):
    with open(fn) as fp:
        return fp.read()

if len(sys.argv) < 2:
    print "Usage: %s profiles" % sys.argv[0]
    exit(1)

thisdir = os.path.dirname(os.path.abspath(__file__))
args    = sys.argv[1:]
outdir  = '/tmp/retro/logs/'
for p in args:
    retrodir = ff_utils.profile_dir(p) + "/retro"
    clientid = file_read(retrodir + "/clientId").strip()
    outd = outdir + clientid
    ind  = retrodir + "/logs"
    
    print "++ copying logs from %s to %s" % (ind, outd)
    os.makedirs(outd)
    os.system("cp -r %s/* %s" % (ind, outd))
