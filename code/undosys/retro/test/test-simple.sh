#!/bin/sh
D=`/bin/pwd`

$D/init-retro.py

rm -rf /mnt/retro/trunk/tmp/simplest
cp -rf $D/simplest /mnt/retro/trunk/tmp

btrfs subvolume snapshot /mnt/retro/trunk /mnt/retro/snap-`date +%s`

cd /mnt/retro/trunk/tmp/simplest
$D/../ctl/retroctl -o /tmp/retro -p -- ./simplest.sh
