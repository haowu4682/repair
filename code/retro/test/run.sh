#!/bin/sh

D=`dirname $0`

$D/init-retro.py
$D/../ctl/retroctl -o /tmp -p -- "$@"

