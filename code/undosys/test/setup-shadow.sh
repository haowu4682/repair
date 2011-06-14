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

cp ../libundo/*.so /mnt/undofs/d/ || exit 2

echo '#!/bin/sh
useradd -m alice 2> /dev/null
echo alice:alice | chpasswd
userdel eve 2> /dev/null
' > /mnt/undofs/d/prepare.sh

echo "#!/bin/sh
chroot /mnt/undofs/d ./trace.sh
" > /mnt/undofs/d/start.sh

E="LD_PRELOAD=libcwrap.so LD_LIBRARY_PATH=."
echo "#!/bin/sh
./attacker.sh
$E ./login.py alice alice > time.txt
" > /mnt/undofs/d/trace.sh

echo "#!/bin/sh
useradd eve
" > /mnt/undofs/d/attacker.sh

echo "#!/usr/bin/python
import pwd
import spwd
import grp
import crypt
import sys
import time

user = sys.argv[1]
pw = pwd.getpwnam(user)
sp = spwd.getspnam(user)
passwd = sys.argv[2]
assert sp.sp_pwd == crypt.crypt(passwd, sp.sp_pwd)
gr = grp.getgrnam(user)
assert pw.pw_gid == gr.gr_gid
print time.ctime()
" > /mnt/undofs/d/login.py

chmod a+x /mnt/undofs/d/*.sh /mnt/undofs/d/*.py
rm -f /tmp/record.log

./mount.sh

chroot /mnt/undofs/d /prepare.sh

D=`pwd`
( cd /mnt/undofs/d && $D/../strace-4.5.19/strace -f -o /tmp/record.log ./start.sh )

./umount.sh

chmod -R a+rw /mnt/undofs/snap
