#!/bin/sh -x
if id | grep -qv uid=0; then
    echo "Must run setup as root"
    exit 1
fi

if losetup -a | grep -q /dev/shm/undofs.img; then
    echo "Loopback FS mounted, unmounting.."
    umount /mnt/undofs || exit 2
fi

rm -rf /mnt/undofs
mkdir -p /mnt/undofs/d || exit 2
chmod 755 /mnt/undofs/d || exit 2
mkdir /mnt/undofs/snap || exit 2
cp append.sh attacker.sh trace.sh /mnt/undofs/d || exit 2
echo x1 > /mnt/undofs/d/foo.txt || exit 2

rm -f /tmp/record.log
D=`pwd`
( cd /mnt/undofs/d && time -f "real: %e\nuser: %U\nsys : %S" ./trace.sh )
chmod -R a+w /mnt/undofs/d

