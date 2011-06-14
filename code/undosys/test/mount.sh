#!/bin/sh

mkdir /mnt/undofs/d/lib 2> /dev/null
mkdir /mnt/undofs/d/lib64 2> /dev/null
mkdir /mnt/undofs/d/proc 2> /dev/null
mkdir /mnt/undofs/d/dev 2> /dev/null
mkdir /mnt/undofs/d/usr 2> /dev/null
mkdir /mnt/undofs/d/bin 2> /dev/null

mkdir /mnt/undofs/d/root 2> /dev/null
mkdir /mnt/undofs/d/home 2> /dev/null
mkdir /mnt/undofs/d/tmp 2> /dev/null

mount --bind /lib /mnt/undofs/d/lib
mount --bind /lib64 /mnt/undofs/d/lib64
mount --bind /proc /mnt/undofs/d/proc
mount --bind /dev /mnt/undofs/d/dev
mount --bind /usr /mnt/undofs/d/usr
mount --bind /bin /mnt/undofs/d/bin

mount --bind /dev/pts /mnt/undofs/d/dev/pts

if [ ! -d /mnt/undofs/d/etc ]; then
    mkdir /mnt/undofs/d/etc
    cp -f /etc/passwd /etc/group /etc/shadow /etc/login.defs \
	/etc/securetty /etc/nsswitch.conf \
	/mnt/undofs/d/etc
    cp -f -R /etc/pam.d /etc/default /etc/security /etc/ssh \
	/mnt/undofs/d/etc
fi

if [ ! -d /mnt/undofs/d/var ]; then
    mkdir -p /mnt/undofs/d/var/run/sshd
    mkdir -p /mnt/undofs/d/var/log
    touch /mnt/undofs/d/var/run/utmp
    touch /mnt/undofs/d/var/log/wtmp
    touch /mnt/undofs/d/var/log/lastlog
fi

