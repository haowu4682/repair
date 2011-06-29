#!/bin/sh

# IP=${1:-localhost}

# after attack
# $ cat server-safe.txt
#  line-1
#  attack
#  lind-2

# after repair
# $ cat server-safe.txt
#  line-1
#  line-2

echo line-1 > server-safe.txt
while true; do
    cmd=$(nc -l $IP 3333)
    if [ "$cmd" = "done" ]; then
        break
    fi
    echo "$cmd" >> server-safe.txt
done
echo line-2 >> server-safe.txt

