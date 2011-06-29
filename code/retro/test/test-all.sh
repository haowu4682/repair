#!/bin/sh

./test-simple.sh
../repair/repair-os.py /tmp -i /mnt/retro/trunk/tmp/simplest/attacker.sh
./check-simple.sh

./test-rerun0.sh
../repair/repair-os.py /tmp -i /mnt/retro/trunk/tmp/rerun0/attacker.sh
./check-rerun0.sh

./test-rerun1.sh
../repair/repair-os.py /tmp -i /mnt/retro/trunk/tmp/rerun1/attacker.sh
./check-rerun1.sh

./test-rerun2.sh
../repair/repair-os.py /tmp -i /mnt/retro/trunk/tmp/rerun2/attacker.sh
./check-rerun2.sh


