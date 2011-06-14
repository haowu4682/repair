#!/bin/sh
echo attack >> foo.txt
echo Z=/bin/cat >> mybashrc
rm -f foo2.txt
cp foo.txt foo2.txt
cp /bin/ls ./xbin
./xbin > xout.txt
rm xbin
