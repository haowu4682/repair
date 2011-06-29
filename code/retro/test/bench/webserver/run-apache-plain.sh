#!/bin/sh -x

if [ "$(id -u)" != "0" ]; then 
    echo "run as root"
    exit 1
fi

sudo killall apache2

# off apache
apache2ctl stop 2> /dev/null

APACHE_RUN_USER=www-data  \
APACHE_RUN_GROUP=www-data \
taskset 0x00000001 apache2 -k start 2>&1 > profile.log
