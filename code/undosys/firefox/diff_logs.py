#!/usr/bin/python

import sys
import optparse
import json
import urllib
import difflib
import cStringIO

def parse_args():
    p = optparse.OptionParser()
    p.add_option("-q", help="quiet",
                 dest='queit', default=False, action='store_true')
    p.add_option("-t", help="ignore time stamp",
                 dest='ignore_ts', default=True, action='store_false')
    return p.parse_args()

def diff_logs(f1, f2, pn1="A", pn2="B", tough = False):
    rtn = cStringIO.StringIO()
    
    f1 = json.load(f1)
    f2 = json.load(f2)
    
    for i in xrange(min(len(f1), len(f2))):
        # ignore time/random/date
        if not tough:
            for f in (f1[i], f2[i]):
                if "timeStamp" in f:
                    f["timeStamp"] = 0
                if "evType" in f and f["evType"] in ["random", "date"]:
                    f["value"] = 0
        
        d1 = urllib.unquote(json.dumps(f1[i], sort_keys=True, indent=4))
        d2 = urllib.unquote(json.dumps(f2[i], sort_keys=True, indent=4))

        for l in difflib.unified_diff(d1.splitlines(True),
                                      d2.splitlines(True),
                                      fromfile = pn1,
                                      tofile   = pn2):
            rtn.write(l.rstrip())
            rtn.write("\n")

    return rtn.getvalue()

if __name__ == "__main__":
    (opts, args) = parse_args()
    if len(args) != 2:
        print "Usage: %s log1 log2" % sys.argv[0]
        exit(1)

    out = ""
    with open(args[0]) as f1:
        with open(args[1]) as f2:
            out = diff_logs(f1, f2, args[0], args[1], opts.ignore_ts)
            if not opts.queit and out:
                print out

    exit(1 if out else 0)