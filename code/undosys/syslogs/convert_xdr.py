#! /usr/bin/python

import os

fd = open( "../strace-4.5.19/record_xdr.c" )

xdr = []

for l in fd :
    if l.find( "xdrs->x_op" ) >= 0 :
        # skip until returning the block
        for l in fd :
            if l.find( "return TRUE" ) >= 0 :
                # feed '}'
                break
        continue

    if l.find( "\t}" ) >= 0 :
        continue
    
    print l,
