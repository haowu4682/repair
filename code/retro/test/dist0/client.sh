#!/bin/sh

IP=${1:-localhost}

#
# after attack
# $ cat client-safe.txt
#  line-1
#  attack
#  line-2
#  line-3
#
# after repair
#  cat client-safe.txt
#  line-1
#  line-2
#  line-3
# 

echo line-1 > client-safe.txt

./attacker ${IP}
./done ${IP}

# <=>
# close the connect
# nc ${IP} 3333 <<EOF
# done
# EOF

# append second line
echo line-2 >> client-safe.txt

# we never re-execute ./append.sh
./append.sh
