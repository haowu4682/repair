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

cp ../libundo/*.so  /mnt/undofs/d || exit 2

echo '
useradd -m alice 2> /dev/null
echo alice:alice | chpasswd
userdel eve 2> /dev/null
' > /mnt/undofs/d/prepare.sh

echo "#!/bin/sh
chroot /mnt/undofs/d ./trace.sh
" > /mnt/undofs/d/start.sh

E="LD_LIBRARY_PATH=. LD_PRELOAD=libcwrap.so"
echo "#!/bin/sh
./attacker.sh
$E /usr/sbin/sshd -d -p 2022
" > /mnt/undofs/d/trace.sh

echo "#!/bin/sh
useradd eve
" > /mnt/undofs/d/attacker.sh

echo '#!/usr/bin/expect
spawn ssh alice@localhost -p 2022 -o "PreferredAuthentications password" -o "StrictHostKeyChecking no" -o "UserKnownHostsFile /dev/null"
expect "password:"
send "alice\\n"
expect "\\\\$"
send "exit\\n"
wait eof
' > /mnt/undofs/d/alice.expect

chmod a+x /mnt/undofs/d/*.sh /mnt/undofs/d/*.expect
rm -f /tmp/record.log

./mount.sh

chroot /mnt/undofs/d /prepare.sh

D=`pwd`
( cd /mnt/undofs/d && $D/../strace-4.5.19/strace -f -o /tmp/record.log ./start.sh & )

sleep 2
/mnt/undofs/d/alice.expect
wait $!

./umount.sh

chmod -R a+rw /mnt/undofs/snap

