#!/bin/sh
IP=${1:-laocalhost}
echo attack >> client-safe.txt
# connect to the server
nc ${IP} 3333 <<EOF
attack
EOFb
