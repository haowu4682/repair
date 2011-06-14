#!/usr/bin/python

import os
import time

IP=os.popen("hostname -I").read()
if os.fork() == 0:
    os.system("sudo ./test-suite.sh dist0 ./tcpserver")
    exit(0)
time.sleep(1)
os.system('ssh -t root@vm "cd retro/test; ./test-suite.sh dist0 ./client.sh %s"' % IP)
os.wait()
os.system("cat /mnt/retro/trunk/tmp/dist0/server-safe.txt")
