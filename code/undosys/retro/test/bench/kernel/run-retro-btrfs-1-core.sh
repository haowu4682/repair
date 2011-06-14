#!/bin/sh -x

if [ "$(id -u)" != "0" ]; then 
    echo "run as root"
    exit 1
fi

btrfs subvolume snapshot /mnt/retro/trunk /mnt/retro/snap-`date +%s`

KERNEL=/mnt/retro/trunk/tmp/linux-2.6.32

cd $KERNEL
make clean
cd -

df -h /mnt/retro >>$ROOT/retro-log-size

ROOT=/home/ssd/Temp

sudo /bin/rm -rf $ROOT/retro-log
sudo /bin/rm -rf $ROOT/retro-zip

mkdir -p $ROOT/retro-log
mkdir -p $ROOT/retro-zip

../../../ctl/retroctl -p -r -x result.log -d $KERNEL -o $ROOT/retro-log -- make vmlinux

df -h /mnt/retro >>$ROOT/retro-log-size
