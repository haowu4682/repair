#! /usr/bin/python

import sys

def stat( log ) :
    nodes = set()
    edges = set()
    
    # skip first & last
    for line in open( log ) :
        line = line.rstrip()
        if line[-1] != ';' :
            continue

        # edge & node
        if line.find( " -> " ) >= 0 :
            # NOT ignore label
            # ele = line.split('"')
            # edges.add('"%s" -> "%s"' % (ele[1],ele[3]))
            edges.add(line)
        else :
            nodes.add(line.split('"')[1])

    return nodes, edges

nodes = set()
edges = set()

if len(sys.argv) == 1 :
    print "usage: %s *.dot" % sys.argv[0]
    exit(1)

for dot in sys.argv[1:] :
    n, e = stat( dot )
    print "Checking: %s => Node:%d, Edge:%d" % (dot, len(n), len(e))
    
    nodes = nodes.union(n)
    edges = edges.union(e)

print "Total: Node:%d, Edge:%d" % (len(nodes), len(edges))
            
