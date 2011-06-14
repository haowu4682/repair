#!/bin/sh

IP=${1:-localhost}

./alice1.sh ${IP}
./attacker.sh ${IP}