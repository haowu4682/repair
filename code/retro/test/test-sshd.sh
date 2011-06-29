#! /bin/sh
./init-retro.py
../bin/retroctl -c /mnt/retro/trunk -d /mnt/retro/trunk -p -o /tmp -- /usr/sbin/sshd -d -p 2022