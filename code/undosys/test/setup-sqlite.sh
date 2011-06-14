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

S="sqlite3 test.db"

echo "#!/bin/sh
./attacker.sh
$S < more.sql
" > /mnt/undofs/d/trace.sh

echo "#!/bin/sh
$S < attacker.sql
" > /mnt/undofs/d/attacker.sh

echo "create table undo(name varchar(30) primary key, about text);
insert into undo values('FK', 'super');
insert into undo values('NZ', 'super');
insert into undo values('TK', 'super');
insert into undo values('XW', 'stupid');
" > /mnt/undofs/d/prepare.sql

echo "delete from undo where about = 'stupid';
" > /mnt/undofs/d/attacker.sql

echo "insert into undo values('rtm', \"he's never wrong\");
insert into undo values('sbw', \"impossible is nothing\");
" > /mnt/undofs/d/more.sql

chmod a+x /mnt/undofs/d/*.sh
rm -f /tmp/record.log

#./mount.sh

( cd /mnt/undofs/d && $S < prepare.sql )

D=`pwd`
( cd /mnt/undofs/d && $D/../strace-4.5.19/strace -f -o /tmp/record.log ./trace.sh )

#./umount.sh

chmod -R a+rw /mnt/undofs/snap /mnt/undofs/d
