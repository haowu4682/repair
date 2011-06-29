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

ROOT=/home/ssd/Temp

sudo /bin/rm -rf $ROOT/retro-log
sudo /bin/rm -rf $ROOT/retro-aux

sudo /bin/rm -rf $ROOT/retro-zip1
sudo /bin/rm -rf $ROOT/retro-zip2

mkdir -p $ROOT/retro-log
mkdir -p $ROOT/retro-aux
mkdir -p $ROOT/retro-zip1
mkdir -p $ROOT/retro-zip2

fusecompress $ROOT/retro-zip1 $ROOT/retro-log
# fusecompress $ROOT/retro-zip2 $ROOT/retro-aux

# XXX take a snapshot
../../../ctl/retroctl2 -p -u 1000 -r -x result.log -d $KERNEL -o $ROOT/retro-log -i $ROOT/retro-aux -- make vmlinux
