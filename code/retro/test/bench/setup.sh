#!/bin/sh -x

sudo insmod ../../trace/retro.ko
sudo insmod ../../trace/iget.ko

fusermount -u /tmp/retro-log

while [ -e "/tmp/retro-log.zip.*" ]; do
    echo "Waiting for fuse-zip to stop"
    sleep 1
done

rm -f /tmp/retro.log.zip
rm -f /tmp/[0-9.]+
