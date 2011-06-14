#! /bin/sh

D=`/bin/pwd`
#python -m trace --ignore-dir=/usr/lib/ -t \
python \
  $D/../../repair/repair-os.py /tmp/retro -i \
    /mnt/retro/trunk/tmp/contents/attacker.sh
