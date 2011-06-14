#!/bin/bash
D=$(readlink -f $(dirname $0))

IMAGE=$D/../retro.img
ROOT_MOUNT_POINT=/media/btrfs
TRUNK_MOUNT_POINT_BASE=/mnt/retro
TRUNK_MOUNT_POINT=$TRUNK_MOUNT_POINT_BASE/trunk
SNAP_MOUNT_POINT=$TRUNK_MOUNT_POINT_BASE/$SNAP_SPEC

ROOT_MOUNT_OPTS=loop,defaults,subvolid=0

SNAP_MOUNT_POINT=$(find $TRUNK_MOUNT_POINT_BASE -maxdepth 1 -name "snap-*")
SNAP_SPEC=$(basename $SNAP_MOUNT_POINT)

mount -o loop,defaults,subvol=$SNAP_SPEC $IMAGE $SNAP_MOUNT_POINT
