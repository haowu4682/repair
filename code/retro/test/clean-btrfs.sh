#!/bin/sh

if [ "x$1" = "x" ]; then
    echo "Usage: $0 retro-root-dir"
    exit 1
fi

D=$1
for f in $1/snap-* ; do test -d $f && btrfs subvolume delete $f ; done
/bin/rm -rf $D/trunk/.inodes
/bin/mkdir  $D/trunk/.inodes

