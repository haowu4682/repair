#!/bin/bash
echo 1 > branch
./attacker.sh
./compute branch safe.txt
