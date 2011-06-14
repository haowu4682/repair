#!/bin/sh
PID=`cat $1 | grep bar.txt | cut -d, -f1`
L=`cat $1 | grep -n "^$PID" | grep write | grep done | head -1 | cut -d: -f1`

## Lines in graph.py are zero-indexed
expr $L - 1
