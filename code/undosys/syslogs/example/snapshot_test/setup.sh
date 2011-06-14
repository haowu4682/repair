#!/bin/sh -x
if id | grep -qv uid=0; then
    echo "Must run setup as root"
    exit 1
fi

if losetup -a | grep -q /dev/shm/undofs.img; then
    echo "Loopback FS mounted, unmounting.."
    umount /mnt/undofs || exit 2
fi

# dd if=/dev/zero of=/dev/shm/undofs.img bs=1024k count=256 || exit 2
# mkfs -t btrfs /dev/shm/undofs.img || exit 2

# mkdir -p /mnt/undofs || exit 2
# mount -o loop -t btrfs /dev/shm/undofs.img /mnt/undofs || exit 2

rm -rf /mnt/undofs/d
rm -rf /mnt/undofs/snap

mkdir -p /mnt/undofs/d
mkdir -p /mnt/undofs/snap
mkdir -p /mnt/undofs/snap/d

cp trace.sh attacker.sh /mnt/undofs/snap/d
cp trace.sh attacker.sh /mnt/undofs/d

rm -f /tmp/record.log
D=`pwd`

modprobe nfs

( cd $D/../syslogs && make insmod )
( cd /mnt/undofs/d && $D/../syslogs/example/loader/loader ./trace.sh )

chmod -R a+w /mnt/undofs/d
chmod -R a+w /mnt/undofs/snap/d

cp /proc/undosys/record $D/../simple_test/record.log

