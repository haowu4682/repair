#!/bin/sh -x

if id | grep -qv uid=0; then
    echo "Must run setup as root"
    exit 1
fi

for i in `seq 1 7`; do
    echo 0 > "/sys/devices/system/cpu/cpu${i}/online"
done

echo 1 > "/sys/devices/system/cpu/cpu1/online"
echo 1 > "/sys/devices/system/cpu/cpu2/online"
echo 1 > "/sys/devices/system/cpu/cpu3/online"
