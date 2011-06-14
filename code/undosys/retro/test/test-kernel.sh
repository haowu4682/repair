#!/bin/sh
D=`/bin/pwd`

cd /mnt/retro/trunk/tmp/linux-2.6.32
make clean
cd -

$D/init-retro.py

cp -rf $D/kernel /mnt/retro/trunk/tmp
rm -f /mnt/retro/trunk/tmp/kernel/safe.txt
btrfs subvolume snapshot /mnt/retro/trunk /mnt/retro/snap-`date +%s`

cd /mnt/retro/trunk/tmp/kernel
$D/../ctl/retroctl-partial -o /tmp -p -- ./simplest.sh

# cd /mnt/retro/trunk
# $D/../ctl/retroctl -o /tmp -p -- chroot /mnt/retro/trunk /tmp/simplest/simplest.sh
