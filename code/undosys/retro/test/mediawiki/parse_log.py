#!/usr/bin/python

import sys
import os
import urllib
import optparse

def parse_args():
    p = optparse.OptionParser()
    p.add_option("-s", help="summary of logs",
                 dest="summary", action="store_true", default=False)
    p.add_option("-v", help="verify logs (not yet implemented)",
                 dest="verify", action="store_true", default=False)
    return p.parse_args()

if __name__ == "__main__":
    (opts, args) = parse_args()
    
    log = "/tmp/retro/httpd.log"
    if len(args) == 1:
        log = args[0]

    for line in open(log):
        (rid, time, type, subtype, val) = line.split(" ")
        if opts.summary:
            print rid, time, type
        else:
            print rid, time, type, subtype, urllib.unquote(val),