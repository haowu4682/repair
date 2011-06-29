#!/usr/bin/python

import sys

mark = " * @"
excludes = ["mmap", "munmap", "exit", "libc", "etc", "access", "close"]

# find the line
on = False
for l in sys.stdin.readlines():
    if on :
       if " * [" in l:
           on = False
       else:
           if "!" in l:
               print l,
           continue
       
    if mark in l and all(not l in e for e in excludes):
       on = True
       print l,

