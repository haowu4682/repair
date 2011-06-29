#!/bin/sh -x

if [ "$(id -u)" != "0" ]; then
    echo "run as root"
    exit 1
fi

./umount-zip.sh
./stat.py