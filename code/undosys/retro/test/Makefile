#
# To use a real disk for the btrfs partition, symlink
# retro.img to /dev/sdb, /dev/sdb5, or whatever.
#

TRUNK  = /mnt/retro/trunk
CHROOT = chroot $(TRUNK)

all:
	echo "make [mount|umount|debootstrap]"

login:
	$(CHROOT) /bin/bash

sshd:
	$(CHROOT) /usr/sbin/sshd -p 2022 -d

mount:
	mkdir -p /mnt/retro
	grep -q \ /mnt/retro\  /proc/mounts || \
		mount -o loop -t btrfs retro.img /mnt/retro
	grep -q \ $(TRUNK)/proc\  /proc/mounts || \
		mount /proc $(TRUNK)/proc -t proc
	grep -q \ $(TRUNK)/sys\  /proc/mounts || \
		mount /sys $(TRUNK)/sys -t sysfs
	grep -q \ $(TRUNK)/sys/kernel/debug\  /proc/mounts || \
		mount /sys/kernel/debug $(TRUNK)/sys/kernel/debug -t debugfs
	grep -q \ $(TRUNK)/dev/pts\  /proc/mounts || \
		mount /dev/pts $(TRUNK)/dev/pts -t devpts

umount:
	grep -q \ $(TRUNK)/dev/pts\  /proc/mounts ; test 1 = $$? || \
		umount $(TRUNK)/dev/pts
	grep -q \ $(TRUNK)/sys/kernel/debug\  /proc/mounts ; test 1 = $$? || \
		umount $(TRUNK)/sys/kernel/debug
	grep -q \ $(TRUNK)/sys\  /proc/mounts ; test 1 = $$? || \
		umount $(TRUNK)/sys
	grep -q \ $(TRUNK)/proc\  /proc/mounts ; test 1 = $$? || \
		umount $(TRUNK)/proc
	grep -q \ /mnt/retro\  /proc/mounts ; test 1 = $$? || \
		umount /mnt/retro

image:
	test -e retro.img || dd if=/dev/zero of=retro.img bs=1024k count=8192
	mkfs -t btrfs retro.img

imagereset:
	mount -o loop -t btrfs retro.img /mnt/retro
	btrfs subvolume delete $(TRUNK)
	btrfs subvolume snapshot /mnt/retro/fresh $(TRUNK)
	umount /mnt/retro

# don't use btrfsctl; its return value is weird
debootstrap: image
	mkdir -p /mnt/retro
	mount -o loop -t btrfs retro.img /mnt/retro
	btrfs subvolume create $(TRUNK)
	mkdir $(TRUNK)/.inodes
	mkdir $(TRUNK)/.snap
	debootstrap --variant=minbase --components=main,universe \
	    --include=apt,wget,openssh-server,nginx,php5-mysql,php5-cgi,php5-cli,python,texinfo,apache2,mysql-server\
	    maverick $(TRUNK) http://ubuntu.media.mit.edu/ubuntu/
	mkdir -p $(TRUNK)/var/run/sshd
	$(CHROOT) useradd -m alice
	echo alice:alice | $(CHROOT) chpasswd
	echo root:root | $(CHROOT) chpasswd
	btrfs subvolume snapshot $(TRUNK) /mnt/retro/fresh
	umount /mnt/retro

# create archive
zip:
	mkdir -p /tmp/retro-log
	mkdir -p /tmp/retro-zip
	fusecompress /tmp/retro-zip /tmp/retro-log

unzip:
	du -sh --apparent-size /tmp/retro-log
	du -sh --apparent-size /tmp/retro-zip
	fusermount -u /tmp/retro-log

.PHONY : all login mount umount image imagereset debootstrap

