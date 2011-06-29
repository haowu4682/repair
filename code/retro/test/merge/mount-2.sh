#!/bin/bash
D=$(readlink -f $(dirname $0))

IMAGE=$D/../retro.img
ROOT_MOUNT_POINT=/media/btrfs
TRUNK_MOUNT_POINT_BASE=/mnt/retro
TRUNK_MOUNT_POINT=$TRUNK_MOUNT_POINT_BASE/trunk
SNAP_SPEC=snap-$(date +%s)
SNAP_MOUNT_POINT=$TRUNK_MOUNT_POINT_BASE/$SNAP_SPEC

ROOT_MOUNT_OPTS=loop,defaults,subvolid=0

umount $TRUNK_MOUNT_POINT
# Mount snapshot-1 as trunk
mount -o loop,defaults,subvol=trunk-snap2 $IMAGE $TRUNK_MOUNT_POINT
