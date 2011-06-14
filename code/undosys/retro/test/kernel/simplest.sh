#!/bin/sh
#
# after being attacked
# 
#  cat safe.txt
#  line-1
#  attack
#  line-2
#  line-3
#
# but we expected under if you repair
# 
#  cat safe.txt
#  line-1
#  line-2
#  line-3
# 

echo line-1 > safe.txt
./attacker.sh
echo line-2 >> safe.txt
./append.sh
# cp safe.txt safe-copy.txt
./workload.sh

