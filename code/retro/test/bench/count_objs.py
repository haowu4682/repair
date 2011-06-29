#!/usr/bin/python

import sys

action = 0
object = 0

for l in sys.stdin.readlines():
    items = l.split()

    cat = items[0]
    obj = int(items[1])
    
    if len(items) == 3:
        obj += eval(items[2])

    print "%s -> %d" % (cat, obj)

    if cat.find("Action") != -1:
        action += obj
    else:
        object += obj
        
print "Action: %s" % action
print "Object: %s" % object
