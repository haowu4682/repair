#! /bin/sh
cat $1 | grep ./lc | grep exec | head -1 | cut -d, -f1
