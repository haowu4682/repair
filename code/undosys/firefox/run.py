#! /usr/bin/python

import os, sys, shutil, optparse, ff_utils

thisdir   = os.path.dirname(os.path.abspath(sys.argv[0]))

undoDir   = None
modeFile  = None
replayDir    = '/tmp/retro/client_logs'
pageToRepair = '/tmp/retro/pageToRepair'

def init(profile):
    global undoDir, modeFile
    profileDir = ff_utils.profile_dir(profile)
    if profileDir is None:
        print "Invalid firefox profile %s. Create it with 'firefox -P %s' first." % (profile, profile)
        exit(1)
    undoDir = profileDir + "/retro"
    modeFile = undoDir + "/mode"

def read_from_file(fname):
    with open(fname) as fp:
        return fp.read()

def write_to_file(fname, str):
    with open(fname, "w") as f:
        f.write(str)

def cleanup_logs():
    if os.path.exists(undoDir):
        shutil.rmtree(undoDir)

def run_ff(profile):
    os.system("python %s/setup.py -p %s" % (thisdir, profile))
    os.system("firefox --no-remote -P %s" % profile)

def record(profile):
    write_to_file(modeFile, "record")
    run_ff(profile)
    write_to_file(modeFile, "none")

def redo(profile, clientid, pageid):
    testLogFile = replayDir + "/" + clientid + "/" + pageid.replace(".", "/")
    if not os.path.exists(testLogFile):
        print "Error: cannot find first replay log file:", testLogFile
        exit(1)
    
    write_to_file(modeFile, "replay")
    write_to_file(pageToRepair, "%s %s" % (clientid, pageid))
    run_ff(profile)
    write_to_file(modeFile, "none")

def parse_args():
    p = optparse.OptionParser()
    p.add_option("--reset", help="remove/reset log files", dest='reset',
                 action='store_true', default=False)
    p.add_option("--cmd", help="command to run (record | redo). default is 'record'", dest="cmd",
                 action="store", type="string", default="record")
    p.add_option("--profile", help="firefox profile to use. default is 'default'", dest="profile",
                 action="store", type="string", default="default")
    p.add_option("--clientid", help="client id to redo", dest="clientid", action="store",
                 type="string")
    p.add_option("--pageid", help="page id to redo", dest="pageid", action="store",
                 type="string")
    (opts, args) = p.parse_args()
    
    if opts.cmd != "record" and opts.cmd != "redo":
        p.error("invalid command '%s'" % opts.cmd)
    if opts.cmd == "redo" and \
       (opts.clientid is None or opts.pageid is None):
        p.error("need a clientid and pageid to redo")
        
    return opts

def main():
    opts = parse_args()
    init(opts.profile)
    if opts.reset:
        cleanup_logs()
    if not os.path.exists(undoDir):
        os.makedirs(undoDir)
    if opts.cmd == "record":
        record(opts.profile)
    elif opts.cmd == "redo":
        redo(opts.profile, opts.clientid, opts.pageid)

if __name__ == "__main__":
    main()
