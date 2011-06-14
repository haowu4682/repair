#! /bin/sh
cat $1 | grep passwd | grep exec | head -1 | cut -d, -f1
