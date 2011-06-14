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
    --include=wget,texlive,texinfo,openssh-server,nginx,python,php5-cgi,php5-cli,build-essential \
    lucid /mnt/undofs/d http://ubuntu.media.mit.edu/ubuntu/ || exit 2
    
## set things up for sshd
mkdir -p /mnt/undofs/d/var/run/sshd
chmod 0755 /mnt/undofs/d/var/run/sshd
mount --bind /proc    /mnt/undofs/d/proc
mount --bind /dev/pts /mnt/undofs/d/dev/pts

## the initial root password is 'root'
chroot /mnt/undofs/d /usr/sbin/usermod --password '$6$o1Y92sqe$DqB3onZIXGZrGMeJ99anZi4LbokMWzx9QtxYpkUk9vQR.3qBe2stx5xaJGPyjy8qnYmARSjvOhy0THimA8/dR0' root

cat >/mnt/undofs/d/ssh-server.sh <<EOM
#!/bin/sh
while test -f /mnt/undofs/d/restart-sshd; do
  LD_PRELOAD=/lib/libcwrap.so chroot /mnt/undofs/d /usr/sbin/sshd -d -p 2022
done
EOM
chmod 755 /mnt/undofs/d/ssh-server.sh
touch /mnt/undofs/d/restart-sshd

ed /mnt/undofs/d/etc/pam.d/common-session <<EOM
/pam_unix.so/s/^/#/
wq
EOM

ed /mnt/undofs/d/etc/pam.d/sshd <<EOM
/pam_motd.so/s/^/#/
wq
EOM

ed /mnt/undofs/d/etc/pam.d/login <<EOM
/pam_motd.so/s/^/#/
wq
EOM

cp ../libundo/libcwrap.so    /mnt/undofs/d/lib
cp ../libundo/libundo.so     /mnt/undofs/d/lib
cp ../libundo/libundowrap.so /mnt/undofs/d/lib

# rm -f /tmp/record.log
# D=`pwd`
# ( cd /mnt/undofs/d && $D/../strace-4.5.19/strace -f ./ssh-server.sh )

