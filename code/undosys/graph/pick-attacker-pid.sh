#!/bin/sh
cat $1 | grep attacker | grep execve | cut -d, -f1
