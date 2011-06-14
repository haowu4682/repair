#!/usr/bin/python

import sys
import optparse
import json
import urllib

def parse_args():
    p = optparse.OptionParser()
    p.add_option("-g", help="grep", dest='grep', default=None)
    return p.parse_args()

if __name__ == "__main__":
    (opt, args) = parse_args()
    if len(args) == 0:
        print "Usage: %s browser-logfile" % sys.argv[0]
        exit(1)

    for f in args:
        a = json.load(open(f))
        for i in xrange(len(a)):
            d = json.dumps(a[i], sort_keys=True, indent=4)
            if opt.grep and opt.grep not in d:
                continue
            
            print "%s#%d" % (f,i), ": ",
            print urllib.unquote(d)
            print
