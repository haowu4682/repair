#!/bin/sh -x

cd $(dirname $0)
if [ `which fusecompress` = "" ]; then
    echo "apt-get install fusecompress"
fi

./umount-zip.sh 2> /dev/null

rm -rf /tmp/retro-zip
rm -rf /tmp/retro-log

./mount-zip.sh