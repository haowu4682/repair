#!/bin/sh -x
JAIL=/jail
ZOOBAR=zoobar
ZOOBARPS=zoobar-ps

create_socket_dir() {
    local dirname="$1"
    local ownergroup="$2"
    local perms="$3"

    mkdir -p $dirname
    chown $ownergroup $dirname
    chmod $perms $dirname
}

set_perms() {
    local ownergroup="$1"
    local perms="$2"
    local pn="$3"

    chown $ownergroup $pn
    chmod $perms $pn
}

mkdir -p $JAIL
cp -p index.php $JAIL

./chroot-copy.sh zookd $JAIL
./chroot-copy.sh zookfs $JAIL
./chroot-copy.sh zooksvc $JAIL

./chroot-copy.sh /usr/bin/php-cgi $JAIL
./chroot-copy.sh /usr/bin/php $JAIL

mkdir -p $JAIL/etc
cp /etc/localtime $JAIL/etc

create_socket_dir $JAIL/echosvc 61010:61010 755

cp -r $ZOOBAR $JAIL/zoobar
cp -r $ZOOBARPS $JAIL/zoobar-ps

