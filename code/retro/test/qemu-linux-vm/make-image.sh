#!/bin/sh
dd if=/dev/zero of=fs.img bs=1024k seek=2047 count=1
mke2fs -F -j ./fs.img
mkdir -p /mnt/fsx
mount -o loop ./fs.img /mnt/fsx
trap 'umount /mnt/fsx' 0
debootstrap --variant=minbase --components=main,universe \
	--include=apt,openssh-server,python,vim-tiny,pciutils,dhcp3-client,strace,lvm2 \
	maverick /mnt/fsx \
	http://ubuntu.media.mit.edu/ubuntu/

echo '/dev/sda / ext3 defaults 0 0' >> /mnt/fsx/etc/fstab
echo 'proc /proc proc defaults 0 0' >> /mnt/fsx/etc/fstab
mknod /mnt/fsx/dev/sda b 8 0
mknod /mnt/fsx/dev/sdb b 8 16

cat > /mnt/fsx/init.sh <<EOF
#!/bin/sh
mount -o remount,rw /
echo >/etc/mtab
mount -o remount,rw /
mount /proc
mount -t devpts x /dev/pts
mount -t sysfs x /sys
mount -t debugfs x /sys/kernel/debug 
mount -t selinuxfs x /selinux

hostname vm
export PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
/sbin/dhclient eth0
/etc/init.d/ssh start

while :; do
  echo 'Starting shell..'
  /bin/bash
done
EOF
chmod 755 /mnt/fsx/init.sh

## root's password is 'root'
perl -pi -e 's!root:\*:!root:\$6\$ythweci8\$N7JQfIyxjNo/oyEnMp6uQerq7GHqwpKUsLxYdpLyYVdriX75Ka01bC2u9Nfepn3v.xHN5piV6zH.F9oRUJOfq0:!' /mnt/fsx/etc/shadow

