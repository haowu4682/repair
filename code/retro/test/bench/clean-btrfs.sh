#!/bin/sh

for f in /mnt/retro/snap-* ; do sudo btrfs subvolume delete $f ; done
sudo /bin/rm -rf /mnt/retro/trunk/.inodes
sudo /bin/mkdir  /mnt/retro/trunk/.inodes

