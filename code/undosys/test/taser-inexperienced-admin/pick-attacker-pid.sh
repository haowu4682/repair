#! /bin/sh

cat $1 | grep gallery | grep As-album | head -1 | cut -d, -f1
