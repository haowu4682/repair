#! /bin/sh

chroot /mnt/undofs/d useradd -m alice 2> /dev/null
echo alice:alice | chroot /mnt/undofs/d chpasswd