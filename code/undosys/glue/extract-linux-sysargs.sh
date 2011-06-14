#!/bin/sh
LINUX_SRC=/home/nickolai/refsrc/linux-git
find $LINUX_SRC -type f \
    | grep -v 'arch/[^x][^8][^6]' \
    | xargs perl -n -e 'if (/^SYSCALL_DEFINE/) { chomp; while (/,$/) { $_.=<>; chomp; } s/[ \t][ \t]/ /g; print; print "\n"; }'
