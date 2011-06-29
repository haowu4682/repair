#!/bin/sh
D=`/bin/pwd`

if [ -z "$2" ]; then
    echo "usage $0 test-case"
    exit 1
fi

$D/init-retro.py

rm -rf /mnt/retro/trunk/tmp/$1
cp -rf $D/$1 /mnt/retro/trunk/tmp
chmod -R o+rw /mnt/retro/trunk/tmp/$1

btrfs subvolume snapshot /mnt/retro/trunk /mnt/retro/snap-`date +%s`

cd /mnt/retro/trunk/tmp/$1
$D/../ctl/retroctl -o /tmp -p -- $2 $3 $4
