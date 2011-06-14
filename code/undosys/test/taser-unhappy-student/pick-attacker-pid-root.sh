#! /bin/sh
cat $1 | grep exec | grep chmod | head -1 | cut -d, -f1
