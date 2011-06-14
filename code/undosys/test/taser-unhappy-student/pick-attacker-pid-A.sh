#! /bin/sh

CP_PID=`cat $1 | grep exec | grep cp | head -1 | cut -d, -f1`
../../replay/dump2 record.log | grep clone | grep $CP_PID | cut -d, -f2