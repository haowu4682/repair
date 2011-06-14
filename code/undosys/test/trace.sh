#!/bin/sh
echo E=/bin/echo > mybashrc

echo hello > safe.txt
echo x2 >> foo.txt
./attacker.sh
./append.sh
rm -f bar.txt
cp foo.txt bar.txt
