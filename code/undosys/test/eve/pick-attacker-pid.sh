#! /bin/sh
echo `cat $1 | grep exec | grep sshd | head -1 | cut -d, -f1`,`cat $1 | grep exec | grep sshd | sed 1,2d | head -1 | cut -d, -f1`
