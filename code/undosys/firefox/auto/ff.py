import os
import sys
import dbg
import optparse
import glob

from time      import sleep
from functools import partial

# 
# wmctrl, xdotool, xte
#

def test_env():
    if hasattr(test_env, "done"):
        return
    
    for f in ["wmctrl", "xdotool", "xte"]:
        if os.system("which %s >/dev/null" % f) != 0:
            print "apt-get install wmctrl xdotool xautomation"
            exit(1)
            
    setattr(test_env, "done", True)
    
# 
# config
# 
CTRL_TIME = 50000

#
# util
#
def sh(cmd,tok=True):
    dbg.sh(cmd)
    try:
        txt = os.popen(cmd).read().strip()
    except:
        txt = ""
        
    if tok:
        return txt.split("\n")
    return txt

def util_focus(w):
    if isinstance(w, wnd):
        sh("wmctrl -i -r " + w.wid + " 2>/dev/null")
    else:
        sh("xdotool windowfocus " + w + " 2>/dev/null")

def util_get_cur():
    tok = sh("xdotool getmouselocation", False).split()
    return (int(tok[0].split(":")[1]),
            int(tok[1].split(":")[1]))

def util_set_cur(x, y):
    sh("xdotool mousemove %d %d" % (x, y))

def util_get_wnd():
    return sh("xdotool getwindowfocus", False)

def util_abspath(pn):
    return os.path.abspath(os.path.join(os.path.dirname(__file__), pn))

# 
# cmd class
# 
class cmd:
    def __init__(self):
        self.opts  = ["click", "dclick", "type", "wait", "key", "shift_type"]
        self.stack = []

        for o in self.opts:
            setattr(self, o, partial(self.__cmd, o))
            
    def __cmd(self, name, *args):
        self.stack.append((name, args))

    def execute(self, w):
        util_focus(w)
        cmd = []
        for (c, args) in self.stack:
            cmd.extend(getattr(w, c)(*args))
        dbg.debugm("!", "executing:\n  " + "\n  ".join(cmd))
        sh("xte %s" % " ".join("'%s'" % c for c in cmd))

class wnd(object):
    def __init__(self, grep):
        self.rx = 0
        self.ry = 0
        self.wid = None
        self.pid = -1
        self.phr = grep
        
        self.search(grep)
        
    def search(self, grep):
        for w in sh("wmctrl -l -G -p"):
            tok = w.split()
            if (type(grep) == int and str(grep) == w[2]) or \
                    (type(grep) == str and grep in w):
                
                self.wid = tok[0]
                self.pid = int(tok[2])
                self.rx  = int(tok[3])/2
                self.ry  = int(tok[4])/2

                dbg.debug("found: %s, (%s,%s) = %s by %s" \
                              % (self.pid, self.rx, self.ry, self.wid, grep))
                
                break
            
    def click(self, x, y):
        cmd = []
        cmd.append("mousemove %d %d" % (x + self.rx, y + self.ry))
        cmd.append("mouseclick 1")
        cmd.append("usleep %d" % CTRL_TIME)
        return cmd
    
    def dclick(self, x, y):
        cmd = []
        cmd.append("mousemove %d %d" % (x + self.rx, y + self.ry))
        cmd.append("mouseclick 1")
        cmd.append("usleep %d" % CTRL_TIME)
        return cmd

    def type(self, s):
        cmd = []
        cmd.append("str %s" % s)
        cmd.append("usleep %d" % (CTRL_TIME*2))
        return cmd

    def key(self, s):
        cmd = []
        cmd.append("key %s" % s)
        cmd.append("usleep %d" % (CTRL_TIME*2))
        return cmd

    def shift_type(self, s):
        cmd = []
        cmd.append("keydown Shift_R")
        cmd.append("str %s" % s)
        cmd.append("keyup Shift_R")
        cmd.append("usleep %d" % (CTRL_TIME*2))
        return cmd
    
    def key(self, s):
        cmd = []
        cmd.append("key %s" % s)
        cmd.append("usleep %d" % (CTRL_TIME*2))
        return cmd
    
    def wait(self, s):
        return ["usleep %d" % (s*1000000)]

    def resize(self, w, h):
        sh("wmctrl -i -r " + self.wid + " -e 0,-1,-1,%d,%d" % (max(w,10), max(h,10)))

    def move(self, x, y):
        sh("wmctrl -i -r " + self.wid + " -e 0,%d,%d,-1,-1" % (max(x,0), max(y,0)))
        self.search(self.phr)

    def close(self):
        sh("wmctrl -c " + self.phr)
        os.wait()

global g_cur_pos
global g_act_wnd

g_cur_pos = None
g_act_wnd = None
g_testing = None

def launch(profile, url, width = 1000, height = 800, x = 100, y = 100, use_default=False):
    global g_cur_pos
    global g_act_wnd
    
    if os.fork() == 0:
        os.execv("/usr/bin/firefox", ["/usr/bin/firefox",
                                      "--no-remote",
                                      "-P",
                                      profile,
                                      url])
        os.exit(1)

    sleep(1)

    g_cur_pos = util_get_cur()
    g_act_wnd = util_get_wnd()

    # avoid confusing events in log files
    util_set_cur(0,0)
    
    w = wnd("Mozilla Firefox")

    if not use_default:
        w.resize(width, height)
        w.move(x, y)
    
    # calm down
    sleep(0.5)
    
    return w

def parse_args():
    global CTRL_TIME
    global g_testing
    
    p = optparse.OptionParser()
    p.add_option("-u", help="starting url",
                 dest="url", action="store", type="string", default="")
    p.add_option("-p", help="firefox profile",
                 dest="profile", action="store", type="string", default="dev")
    p.add_option("-r", help="record logs under /tmp/undo",
                 dest="record", action="store_true", default=False)
    p.add_option("-c", help="clean up /tmp/undo",
                 dest="clean", action="store_true", default=False)
    p.add_option("-t", help="regression test against",
                 dest="test", metavar="FILE", action="store", default=None)
    p.add_option("-d", help="set delay (default=%d)" % CTRL_TIME,
                 dest="delay", type="int", action="store", default=0)
    
    (opts, args) = p.parse_args()

    # init ./run.py
    run.init(opts.profile)
    
    # convert to file://
    if opts.url:
        if not opts.url.startswith("file://"):
            opts.url = "file://" + os.path.abspath(opts.url)

    # check args on regression test
    if opts.test:
        if not os.path.exists(opts.test):
            print >> sys.stderr, "can't find %s file" % opts.test
            exit(1)
        opts.record = True
        opts.clean  = True

    # clean up logs
    if opts.clean:
        set_clean()
            
    # check undo dir
    if not os.path.exists(run.undoDir):
        os.makedirs(run.undoDir)
        
    # record log
    if opts.record:
        set_record()

    # control time delay
    if opts.delay != 0:
        CTRL_TIME = opts.delay

    # set test flag
    g_testing = opts.test
    
    return (opts, args)

# 
# ff undo ext controller
#

# XXX better to have different name rather than run
sys.path.append(util_abspath(".."))

import run
from merge_logs import merge_logs
from diff_logs  import diff_logs

def set_record():
    run.write_to_file(run.modeFile, "record")

def set_done():
    if os.path.exists(run.modeFile):
        run.write_to_file(run.modeFile, "done")

def set_clean():
    run.cleanup_logs()
    
def install_ext():
    pass

# exit here
def done():
    global g_cur_pos
    global g_act_wnd
    
    set_done()

    # restore
    util_set_cur(*g_cur_pos)
    util_focus(g_act_wnd)
    
    if g_testing:
        # 1. merge logs to tmp
        tmp = os.tmpfile()
        merge_logs(tmp, glob.glob(run.undoDir + "/record/log*"))
        tmp.seek(0)
        
        # 2. diff
        out = None
        with open(g_testing) as ref:
            out = diff_logs(ref, tmp, g_testing, "NEW")
            if out:
                print out

        # for safy
        assert out != None
        
        # diff => 1, same => 0
        exit(1 if out else 0)

test_env()