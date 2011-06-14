#! /bin/sh
cat $1 | grep make | grep exec | head -1 | cut -d, -f1
