#!/bin/bash
D=$(readlink -f $(dirname $0))

echo ivo
echo $D

$D/prepare-mounts.sh
umount /dev/loop*
umount /dev/loop*

mkdir -p /tmp/retro/1 /tmp/retro/2

cd /

$D/mount-1.sh
stat /mnt/retro/trunk/tmp/contents/safe.txt
cd /mnt/retro/trunk/tmp/contents
$D/init-retro.py
$D/../../ctl/retroctl -o /tmp/retro/1 -p -- ./script-1.sh

cd /
mount
$D/mount-2.sh
mount
stat /mnt/retro/trunk/tmp/contents/safe.txt
cd /mnt/retro/trunk/tmp/contents
$D/init-retro.py
$D/../../ctl/retroctl -o /tmp/retro/2 -p -- ./script-2.sh
