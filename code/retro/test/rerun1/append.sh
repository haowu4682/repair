#!/bin/sh
. ./mybashrc
$E safe.txt  >> foo.txt
echo line2 >> safe.txt
