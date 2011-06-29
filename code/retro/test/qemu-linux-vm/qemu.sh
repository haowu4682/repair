#!/bin/sh
qemu-system-x86_64 \
    -hda ./fs.img \
    -hdb ./btrfs.img \
    -kernel /home/nickolai/refsrc/linux-git/arch/x86/boot/bzImage \
    -append "root=/dev/sda init=/init.sh console=ttyS0 kgdboc=ttyS0" \
    -m 256 \
    -redir tcp:9922::22 \
    -serial telnet:localhost:12345,server \
    -nographic

