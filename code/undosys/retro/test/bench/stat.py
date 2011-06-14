#! /usr/bin/python

import os
import sys

pwd = sys.path[0]

# ============================================================
# check fuse-zip
if os.system('dpkg -s fusecompress | grep -q "ok installed"') != 0:
    print "apt-get isntall fuse-zip"
    exit(1)

# ============================================================
# log size

# log archive
# log = "/tmp/retro-log"

# mount
# os.system("%s/mount-zip.sh" % pwd)

# size = os.stat(log + ".zip").st_size

# print "=" * 60
# print "compressed log size (%s): %d" % (log, size)

# ============================================================
# 2. object statistics

# mount
# umount
log = sys.argv[1]
cmd = pwd + "/../../repair/dot2.py -s %s > /dev/null" % log
print "=" * 60
os.system(cmd)
