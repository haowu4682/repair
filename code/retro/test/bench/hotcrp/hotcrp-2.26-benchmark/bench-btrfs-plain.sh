#!/bin/sh
. ./bench-btrfs-common.sh
bench "hotcrp-2.26-orig" "orig" $PHP_ORIG "0" $ORIG_LOG ""
