#! /bin/sh

D=`/bin/pwd`
#python \
#   -d \
python -m trace --ignore-dir=/usr/lib/ -c -C /tmp/covers/ \
  $D/../../repair/repair-os.py /tmp/retro \
    -i /mnt/retro/trunk/tmp/contents/attacker.sh
