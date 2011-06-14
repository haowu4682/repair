#!/bin/sh -x

root=$(/bin/pwd)/$(dirname $0)
if [ "$(id -u)" != "0" ]; then 
    echo "run as root"
    exit 1
fi

KERNEL=$root/linux-2.6.32

cd $KERNEL
make clean
cd -

ROOT=/tmp

sudo /bin/rm -rf $ROOT/retro-log
mkdir -p $ROOT/retro-log

sudo /bin/rm -rf $ROOT/squashfs-zip

# XXX take a snapshot
../../../ctl/retroctl -p -u 1000 -r -x result.log -d $KERNEL -o $ROOT/retro-log -- make vmlinux

sudo time mksquashfs $ROOT/retro-log $ROOT/squashfs-zip