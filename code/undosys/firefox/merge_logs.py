#!/usr/bin/python

import os
import sys
import optparse
import json
import urllib

def parse_args():
    p = optparse.OptionParser()
    p.add_option("-o", help="out",
                 dest='out', metavar="FILE", default="-", action='store')
    return p.parse_args()

def merge_logs(out, args):
    # to sort by right way
    def __convert(f):
        f = f.split("-")
        return (f[0], int(f[1]), int(f[2]))

    log = []
    for pn in sorted(map(__convert, args)):
        pn = "%s-%d-%d" % pn
        with open(pn) as f:
            log.append(f.read()[1:-1])
            
    out.write("[")
    out.write(",".join(log))
    out.write("]")
    
if __name__ == "__main__":
    (opts, args) = parse_args()
    if len(args) == 0:
        print "Usage: %s log1 log2 log3 .. [dest]" % sys.argv[0]
        exit(1)

    if opts.out == "-":
        out = sys.stdout
    else:
        if os.path.exists(opts.out):
            print >> sys.stderr, "%s file exists" % opts.out
            exit(1)
        out = open(opts.out, "w")
        
    merge_logs(out, args)

    out.close()