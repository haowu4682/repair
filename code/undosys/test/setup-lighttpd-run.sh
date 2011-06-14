#!/bin/sh -x
if id | grep -qv uid=0; then
    echo "Must run setup as root"
    exit 1
fi

service apache2 stop
service lighttpd stop

rm -f /tmp/record.log
D=`pwd`

# strace
#( cd /mnt/undofs/d && $D/../strace-4.5.19/strace -f -o /tmp/record.log chroot /mnt/undofs/d /usr/sbin/lighttpd -D -f /etc/lighttpd/lighttpd.conf.dpkg-new)

# syslog

modprobe nfs

( cd $D/../syslogs && make insmod )
( cd /mnt/undofs/d && $D/../syslogs/example/loader/loader /usr/sbin/chroot /mnt/undofs/d /usr/sbin/lighttpd -D -f /etc/lighttpd/lighttpd.conf.dpkg-new)

# normal
#( cd /mnt/undofs/d && chroot /mnt/undofs/d /usr/sbin/lighttpd -D -f /etc/lighttpd/lighttpd.conf.dpkg-new)