#!/bin/sh -x
if id | grep -qv uid=0; then
    echo "Must run setup as root"
    exit 1
fi

if losetup -a | grep -q /dev/shm/undofs.img; then
    echo "Loopback FS mounted, unmounting.."
    umount /mnt/undofs || exit 2
fi

rm -rf /mnt/undofs
mkdir -p /mnt/undofs/d || exit 2
chmod 755 /mnt/undofs/d || exit 2
mkdir /mnt/undofs/snap || exit 2

echo "#!/bin/sh
./prepare.py
./attacker.py
" > /mnt/undofs/d/trace.sh

echo "#!/usr/bin/python
with open('test.txt', 'w') as f:
    for i in range(10):
        f.write(str(i) + '\\\\n')
" > /mnt/undofs/d/prepare.py

echo "#!/usr/bin/python
import os
#os.rename('test.txt', 'bar.txt')
with open('test.txt', 'r') as f:
    content = f.readlines()
del content[4]
content += ['10\\\\n', '42\\\\n']
#with open('test.txt', 'w') as f:
with open('foo.txt', 'w') as f:
    f.writelines(content)
os.rename('foo.txt', 'test.txt')
" > /mnt/undofs/d/attacker.py

chmod a+x /mnt/undofs/d/*.py /mnt/undofs/d/*.sh

rm -f /tmp/record.log
D=`pwd`

( cd /mnt/undofs/d && $D/../strace-4.5.19/strace -f -o /tmp/record.log ./trace.sh )

chmod -R a+rw /mnt/undofs/d
chmod -R a+rw /mnt/undofs/snap
