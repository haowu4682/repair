#!/bin/sh -x
cd /mnt/undofs/snap || exit 1
for D in *; do
    btrfsctl -D $D .
done
