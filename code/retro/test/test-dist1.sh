#!/usr/bin/python

import os
import time

IP=os.popen("hostname -I").read()
if os.fork() == 0:
    os.system("sudo ./init-retro.py")
    os.system("sudo cp -rf dist1 /mnt/retro/trunk/tmp")
    os.system("sudo ../bin/retroctl -c /mnt/retro/trunk -d /mnt/retro/trunk -p -o /tmp -- /tmp/dist1/server.sh")
    exit(0)
    
time.sleep(1)
os.system('ssh -t root@vm "cd retro/test; ./test-suite.sh dist1 ./client.sh %s"' % IP)

# kill server.sh & sshd
os.system('sudo kill `pgrep -n sshd` `pgrep server.sh`')