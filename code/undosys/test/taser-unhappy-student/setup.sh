#! /bin/sh

chroot /mnt/undofs/d adduser --force-badname P
chroot /mnt/undofs/d adduser --force-badname A
chroot /mnt/undofs/d adduser --force-badname B

# grades
cat > /mnt/undofs/d/home/P/grades <<EOF
A=A-
B=A0
C=C+
D=C-
EOF


