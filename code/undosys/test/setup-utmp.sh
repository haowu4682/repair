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

echo "#!/bin/sh
chroot /mnt/undofs/d ./trace.sh
" > /mnt/undofs/d/start.sh

echo '#!/usr/bin/expect
set timeout -1
spawn login -f $argv
expect "\\\\$"
send "exit\\n"
expect eof
' > /mnt/undofs/d/login.expect

echo "#!/bin/sh
./login.expect alice
./attacker.sh
./login.expect alice
" > /mnt/undofs/d/trace.sh

echo "#!/bin/sh
./login.expect eve
" > /mnt/undofs/d/attacker.sh

chmod a+x /mnt/undofs/d/*.sh /mnt/undofs/d/*.expect
rm -f /tmp/record.log

./mount.sh

chroot /mnt/undofs/d useradd -m alice
chroot /mnt/undofs/d useradd -m eve

D=`pwd`
( cd /mnt/undofs/d && $D/../strace-4.5.19/strace -f -o /tmp/record.log ./start.sh )

./umount.sh

chmod -R a+rw /mnt/undofs/d/var
chmod -R a+rw /mnt/undofs/snap
