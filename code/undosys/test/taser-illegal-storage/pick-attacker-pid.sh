#! /bin/sh
cat $1 | grep rm | grep exec | head -1 | cut -d, -f1
