#!/bin/sh -x
if id | grep -qv uid=0; then
    echo "Must run setup as root"
    exit 1
fi

touch /mnt/undofs/d/restart-sshd
rm -f /tmp/record.log
rm /mnt/undofs/snap/*
rm -f /mnt/undofs/d/tmp/apr-1.3.3.tar.gz
rm -rf /mnt/undofs/d/tmp/apr-1.3.3

## the initial root password is 'root'
chroot /mnt/undofs/d /usr/sbin/usermod --password '$6$o1Y92sqe$DqB3onZIXGZrGMeJ99anZi4LbokMWzx9QtxYpkUk9vQR.3qBe2stx5xaJGPyjy8qnYmARSjvOhy0THimA8/dR0' root

D=`pwd`
cp $D/../test/apr-1.3.3.tar.gz /mnt/undofs/d/tmp
( cd /mnt/undofs/d && $D/../strace-4.5.19/strace -q -f -o /tmp/record.log ./ssh-server.sh )

