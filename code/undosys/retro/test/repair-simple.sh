#! /bin/sh

D=`/bin/pwd`
#python -m trace --ignore-dir=/usr/lib/ -t \
python \
  $D/../repair/repair-os.py -d /tmp/retro -i \
    /mnt/retro/trunk/tmp/simplest/attacker.sh
