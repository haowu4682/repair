#! /bin/sh

D=`/bin/pwd`
#python -m trace --ignore-dir=/usr/lib/ -t \
python \
  $D/../../repair/merge.py /tmp/retro/1 /tmp/retro/2
