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

cp ../libundo/bdb/bdbtest \
    /mnt/undofs/d || exit 2

T="./bdbtest"
echo "#!/bin/sh
$T write Alice Atlanta
$T write Bob Boston
./attacker.sh
$T read Alice > foo.txt
$T read Alice Bob >> foo.txt
" > /mnt/undofs/d/trace.sh

echo "#!/bin/sh
$T write Bob 'District 9'
" > /mnt/undofs/d/attacker.sh

chmod a+x /mnt/undofs/d/*.sh

rm -f /tmp/record.log
D=`pwd`

( cd /mnt/undofs/d && $D/../strace-4.5.19/strace -f -o /tmp/record.log ./trace.sh )

chmod a+rw /mnt/undofs/d/*.db /mnt/undofs/d/*.txt
chmod -R a+r /mnt/undofs/snap
