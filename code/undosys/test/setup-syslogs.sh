#!/bin/sh -x
if id | grep -qv uid=0; then
    echo "Must run setup as root"
    exit 1
fi

if losetup -a | grep -q /dev/shm/undofs.img; then
    echo "Loopback FS mounted, unmounting.."
    umount /mnt/undofs || exit 2
fi

rm -rf /mnt/undofs/snap
rm -rf /mnt/undofs/d

mkdir -p /mnt/undofs/d
chmod 755 /mnt/undofs/d

mkdir -p /mnt/undofs/snap
cp append.sh attacker.sh trace.sh /mnt/undofs/d
echo x1 > /mnt/undofs/d/foo.txt

rm -f /tmp/record.log

D=`pwd`

modprobe nfs
pgrep daemon.py > /dev/null || echo "Please execute syslogs/example/daemon/daemon.py to fetch record.log"

# take the first snapshot
cp -rf /mnt/undofs/d /mnt/undofs/snap/d

( cd $D/../syslogs && make insmod )
( cd /mnt/undofs/d && time -f "real: %e\nuser: %U\nsys : %S" $D/../syslogs/example/loader/loader ./trace.sh )

chmod -R a+w  /mnt/undofs/d
chmod -R a+rw /mnt/undofs/snap
 
cp /proc/undosys/record ../graph/record.log
