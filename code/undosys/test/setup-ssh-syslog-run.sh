#!/bin/sh -x
if id | grep -qv uid=0; then
    echo "Must run setup as root"
    exit 1
fi

touch /mnt/undofs/d/restart-sshd
rm -f /tmp/record.log
rm /mnt/undofs/snap/*

modprobe nfs
pgrep daemon.py > /dev/null || (echo "Please execute syslogs/example/daemon/daemon.py to fetch record.log" && exit 1)

D=`pwd`

# Apache
# rm -f /mnt/undofs/d/tmp/apr-1.3.3.tar.gz
# rm -rf /mnt/undofs/d/tmp/apr-1.3.3
# cp $D/../test/apr-1.3.3.tar.gz /mnt/undofs/d/tmp

# Kernel
# rm -rf /mnt/undofs/d/tmp/linux-2.6.31
# cp -rf $D/../test/linux-2.6.31 /mnt/undofs/d/tmp

( cd /mnt/undofs/d/tmp/linux-2.6.31 && make clean )
( cd $D/../syslogs && make insmod )
( cd /mnt/undofs/d && $D/../syslogs/example/loader/loader ./ssh-server.sh )
