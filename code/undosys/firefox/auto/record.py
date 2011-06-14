#! /usr/bin/python

import ff
import os
import time

def get_ff():
    return ff.sh("xdotool search --title Mozilla", False)

def xev():
    child = os.fork()
    if child == 0:
        wid = get_ff()
        if wid:
            os.system("xev -id 0x%x | grep -A 1 ^Leave" % int(wid))
        exit()
    return child

if __name__ == "__main__":
    child = xev()
    while True:
        if get_ff():
            time.sleep(1)
        else:
            os.kill(child, 9)
            os.wait()
            time.sleep(0.5)
            child = xev()
