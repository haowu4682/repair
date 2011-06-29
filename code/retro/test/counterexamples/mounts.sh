#!/bin/bash
D=`/bin/pwd`

IMAGE=$D/../retro.img
ROOT_MOUNT_POINT=/media/btrfs
TRUNK_MOUNT_POINT_BASE=/mnt/retro
TRUNK_MOUNT_POINT=$TRUNK_MOUNT_POINT_BASE/trunk
SNAP_SPEC=snap-$(date +%s)
SNAP_MOUNT_POINT=$TRUNK_MOUNT_POINT_BASE/$SNAP_SPEC

ROOT_MOUNT_OPTS=loop,defaults,subvolid=0

umount $ROOT_MOUNT_POINT

for f in $TRUNK_MOUNT_POINT_BASE/* ; do test -d $f && umount $f ; done
rm -fr $TRUNK_MOUNT_POINT_BASE
mkdir -p $TRUNK_MOUNT_POINT_BASE

mkdir -p $TRUNK_MOUNT_POINT $ROOT_MOUNT_POINT $SNAP_MOUNT_POINT

mkfs -t btrfs $IMAGE

mount -o $ROOT_MOUNT_OPTS -t btrfs $IMAGE $ROOT_MOUNT_POINT
cd $ROOT_MOUNT_POINT
btrfs subvolume create trunk
cd ..

# Let trunk/ be mounted on $TRUNK_MOUNT_POINT. (But let subvol 0 be
# mounted on $ROOT_MOUNT_POINT.) Now,

# mount trunk and copy files
mount -o loop,defaults,subvol=trunk $IMAGE $TRUNK_MOUNT_POINT
rm -rf $TRUNK_MOUNT_POINT/tmp/contents
mkdir -p $TRUNK_MOUNT_POINT/tmp
cp -fvr $D/contents $TRUNK_MOUNT_POINT/tmp
mkdir -p $TRUNK_MOUNT_POINT/{.inodes,.snap}
umount $TRUNK_MOUNT_POINT

# create snapshots
#
# snap1 and snap2 are to be modified by the simultaneous processes.
# the $SNAP_SPEC snapshot is actually used by Retro during
# re-execution.
cd $ROOT_MOUNT_POINT
btrfs subvolume snapshot trunk trunk-snap1
btrfs subvolume snapshot trunk trunk-snap2
btrfs subvolume snapshot trunk $SNAP_SPEC
cd ..

# Mount snapshot-1 as trunk
mount -o loop,defaults,subvol=trunk-snap1 $IMAGE $TRUNK_MOUNT_POINT
umount $ROOT_MOUNT_POINT

# Also mount snap-$date snapshot, for the use of the repair
mount -o loop,defaults,subvol=$SNAP_SPEC $IMAGE $SNAP_MOUNT_POINT
