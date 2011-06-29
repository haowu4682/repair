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

D=`pwd`

echo line-1 > blah
$D/append
echo line-2 >> blah
