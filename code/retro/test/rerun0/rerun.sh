#!/bin/sh
echo E=/bin/echo > mybashrc
echo line1 > safe.txt
echo foo >> foo.txt
./attacker.sh
./append.sh