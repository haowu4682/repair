#!/bin/sh
echo E=/bin/cat > mybashrc

echo line1 > safe.txt
echo foo >> foo.txt
./attacker.sh
./append.sh
./last.sh