#! /bin/sh

rm -rf /mnt/undofs/d/root/src
rm -rf /mnt/undofs/d/root/hfiles
rm -rf /mnt/undofs/d/root/backup

mkdir -p /mnt/undofs/d/root/src
mkdir -p /mnt/undofs/d/root/hfiles
mkdir -p /mnt/undofs/d/root/backup

# 0. 
cat > /mnt/undofs/d/root/src/project.c <<EOF
#include "p1.h"
int main(void) {
  printf("project1!\n");
  return 0;
}
EOF

cp /mnt/undofs/d/root/src/project.c /mnt/undofs/d/root/backup/project.c.bak

cat > /mnt/undofs/d/root/hfiles/p1.h <<EOF
// p1.h
#include <stdio.h>
EOF

cat > /mnt/undofs/d/root/hfiles/p2.h <<EOF
// p2.h
#include <stdio.h>
EOF

