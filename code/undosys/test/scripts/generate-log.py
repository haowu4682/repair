#! /usr/bin/python

import os

for (root,dirs,files) in os.walk("./hot/tmp/record") :
    for f in files :
        if f.endswith(".dot") :
            continue

        log = os.path.join(root,f)

        if os.path.exists(log+".dot") :
            print "Pass: %s" % log
            continue
        
        cmd = "/home/taesoo/Working/undosys/undosys/graph/graph.py -o %s.dot -l %s" % (log,log)
        print cmd
        if os.system( cmd ) != 0 :
            exit(1)
