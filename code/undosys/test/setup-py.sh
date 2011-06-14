#!/bin/sh -x
if id | grep -qv uid=0; then
    echo "Must run setup as root"
    exit 1
fi

if losetup -a | grep -q /dev/shm/undofs.img; then
    echo "Loopback FS mounted, unmounting.."
    umount /mnt/undofs || exit 2
fi

dd if=/dev/zero of=/dev/shm/undofs.img bs=1024k count=256 || exit 2
mkfs -t btrfs /dev/shm/undofs.img || exit 2

mkdir -p /mnt/undofs || exit 2
mount -o loop -t btrfs /dev/shm/undofs.img /mnt/undofs || exit 2

btrfsctl -S d /mnt/undofs
chmod 755 /mnt/undofs/d || exit 2
mkdir /mnt/undofs/snap || exit 2
cp attacker.py funcs.py trace.py ../libundo/undo.py ../libundo/libundo.so \
	/mnt/undofs/d || exit 2
touch /mnt/undofs/d/f.dat || exit 2

rm -f /tmp/record.log
D=`pwd`
( cd /mnt/undofs/d && $D/../strace-4.5.19/strace -f -o /tmp/record.log ./trace.py )
chmod -R a+w /mnt/undofs/d

