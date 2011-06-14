#!/bin/sh
D=`/bin/pwd`

$D/init-retro.py
$D/mounts.sh

cd /mnt/retro/trunk/tmp/contents
$D/../../ctl/retroctl -o /tmp/retro -p -- ./script.sh
