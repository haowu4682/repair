#!/bin/sh

rm /mnt/retro/trunk/test.txt
touch /mnt/retro/trunk/test.txt
echo hello >> /mnt/retro/trunk/test.txt
echo world >> /mnt/retro/trunk/test.txt
echo garbage > /mnt/retro/trunk/test.txt
rm /mnt/retro/trunk/test.txt
