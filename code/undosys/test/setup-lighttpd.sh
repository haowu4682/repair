#!/bin/sh -x
if id | grep -qv uid=0; then
    echo "Must run setup as root"
    exit 1
fi

umount /mnt/undofs/d/proc 2> /dev/null
umount /mnt/undofs/d/dev/pts 2> /dev/null

if losetup -a | grep -q /dev/shm/undofs.img; then
    echo "Loopback FS mounted, unmounting.."
    umount /mnt/undofs || exit 2
fi

rm -rf /mnt/undofs || exit 2

mkdir -p /mnt/undofs/d || exit 2
chmod 755 /mnt/undofs/d || exit 2
mkdir /mnt/undofs/snap || exit 2

chmod -R a+w  /mnt/undofs/d || exit 2
chmod -R a+rw /mnt/undofs/snap || exit 2

debootstrap --variant=minbase --components=main,universe \
    --include=lighttpd,build-essential,libfam0 \
    lucid /mnt/undofs/d http://ubuntu.media.mit.edu/ubuntu/ || exit 2

## set things up for sshd
mkdir -p /mnt/undofs/d/var/run/sshd
chmod 0755 /mnt/undofs/d/var/run/sshd
mount --bind /proc    /mnt/undofs/d/proc
mount --bind /dev/pts /mnt/undofs/d/dev/pts

cat > /mnt/undofs/d/var/www/index.html <<EOF
<html><body><h1>It works!</h1>
<p>This is the default web page for this server.</p>
<p>The web server software is running but no content has been added, yet.</p>
</body></html>
EOF

#cp /usr/lib/libfam.so.0 /mnt/undofs/d/usr/lib/libfam.so.0

chmod 0755 /mnt/undofs/d/var/www/index.html

touch /mnt/undofs/d/var/log/lighttpd/error.log
chown www-data:www-data /mnt/undofs/d/var/log/lighttpd/error.log

touch /mnt/undofs/d/var/log/lighttpd/access.log
chown www-data:www-data /mnt/undofs/d/var/log/lighttpd/access.log