#! /usr/bin/python

import sys
import os

def stat(color) :
    procs,funcs,files = [],[],[]
    
    for n in os.popen( "cat %s | grep -v '\->' | grep color | grep %s" % (sys.argv[1],color) ) :
        if n.find( "(procmgr)" ) >= 0 :
            procs.append(n)
        elif n.find( "(fsmgr)" ) >= 0 or \
                n.find( "(dirmgr)" ) >= 0 or \
                n.find( "(offmgr)" ) >= 0 or \
                n.find( "(ptsmgr)" ) >= 0 :
            files.append(n)
        elif n.find( "(pwdmgr)" ) >= 0 :
            funcs.append(n)
        else :
            print "!", n,

    return procs,funcs,files

procs,funcs,files = 0,0,0
for color in ["green","yellow","red"] :
    rtn = [len(i) for i in stat(color)]
    
    procs += rtn[0]
    funcs += rtn[1]
    files += rtn[2]

    print "%-10s :%s" % (color, str(rtn))
    
print "%-10s :%d,%d,%d" % ("Without", procs,funcs,files)
    
procs,funcs,files = 0,0,0
for color in ["yellow","red"] :
    rtn = [len(i) for i in stat(color)]
    
    procs += rtn[0]
    funcs += rtn[1]
    files += rtn[2]
    
print "%-10s :%d,%d,%d" % ("With", procs,funcs,files)

