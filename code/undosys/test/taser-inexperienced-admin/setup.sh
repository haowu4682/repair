#! /bin/sh

chroot /mnt/undofs/d adduser --force-badname A

cp -f gallery/gallery /mnt/undofs/d/home/A
rm -rf gallery/gallery /mnt/undofs/d/home/A/mygallery
