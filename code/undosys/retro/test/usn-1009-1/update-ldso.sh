#!/bin/sh -ex
#
# To make a machine vulnerable:
#   ./update-ldso.sh ./ld-2.12.1.so-buggy
#
# To patch a machine:
#   ./update-ldso.sh ./ld-2.12.1.so-patched
#

if [ ! -f "$1" ]; then
    echo "Usage: $0 ldso-file"
    exit 1
fi

cp $1 /lib/.stage.ldso
sync
mv /lib/.stage.ldso /lib/ld-2.12.1.so
sync

