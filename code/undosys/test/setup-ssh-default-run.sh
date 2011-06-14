#!/bin/sh -x
if id | grep -qv uid=0; then
    echo "Must run setup as root"
    exit 1
fi

## the initial root password is 'root'
chroot /mnt/undofs/d /usr/sbin/usermod --password '$6$o1Y92sqe$DqB3onZIXGZrGMeJ99anZi4LbokMWzx9QtxYpkUk9vQR.3qBe2stx5xaJGPyjy8qnYmARSjvOhy0THimA8/dR0' root

touch /mnt/undofs/d/restart-sshd
rm -f /tmp/record.log
rm /mnt/undofs/snap/*

( cd /mnt/undofs/d/tmp/linux-2.6.31 && make clean )
( cd /mnt/undofs/d && ./ssh-server.sh )
