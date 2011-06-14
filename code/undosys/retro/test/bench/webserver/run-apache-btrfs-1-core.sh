#!/bin/sh -x

if [ "$(id -u)" != "0" ]; then 
    echo "run as root"
    exit 1
fi

killall apache2

# off apache
apache2ctl stop 2> /dev/null

# init log archive
#../init-zip.sh
#mkdir /tmp/retro-log

ROOT=/home/ssd/Temp

sudo /bin/rm -rf $ROOT/retro-log
sudo /bin/rm -rf $ROOT/retro-zip

mkdir -p $ROOT/retro-log
mkdir -p $ROOT/retro-zip

sudo cp /var/www/1k.file /mnt/retro/trunk/var/www
touch /mnt/retro/trunk/etc/apache2/error.log
sudo cp `which taskset` /mnt/retro/trunk/usr/bin

btrfs subvolume snapshot /mnt/retro/trunk /mnt/retro/snap-`date +%s`

APACHE_LOG_DIR=. \
APACHE_RUN_USER=www-data  \
APACHE_RUN_GROUP=www-data \
../../../ctl/retroctl -r -x profile.log -p -o $ROOT/retro-log -- chroot /mnt/retro/trunk taskset 0x00000001 apache2 -k start
