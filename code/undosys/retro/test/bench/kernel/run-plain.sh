#!/bin/sh -x

KERNEL=/mnt/retro/trunk/tmp/linux-2.6.32

cd $KERNEL
make clean
cd -

cd $KERNEL
time make vmlinux
