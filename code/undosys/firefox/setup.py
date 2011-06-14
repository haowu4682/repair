#!/usr/bin/python

import optparse, os, shutil, sys, ff_utils

extension = {
    'NAME'      : 'undosys',
    'ID'        : 'undosys@pdos.csail.mit.edu',
    'LONG_NAME' : 'Webapp Undo',
    'DESC'      : 'Browser extension for web application undo',
    'VER'       : '0.1',
    'CREATOR'   : 'rameshch@pdos.csail.mit.edu',
    'URL'       : 'http://pdos.csail.mit.edu'
}

def install_dir(profile):
    extensions_dir = os.path.join(ff_utils.profile_dir(profile), 'extensions')
    return os.path.join(extensions_dir, extension['ID'])

def parse_args():
    p = optparse.OptionParser()
    p.add_option("-p", help="Firefox profile to use", dest='profile', action='store',
                 type='string', default='default')
    p.add_option("-c", help="Command [install|uninstall]", dest='cmd',
                 action='store', type='string', default='install')
    (opts, args) = p.parse_args()
    
    ## Ensure options are valid
    if opts.cmd != "install" and opts.cmd != "uninstall":
        p.error("invalid command '%s'" % opts.cmd)
    if ff_utils.profile_dir(opts.profile) == None:
        p.error("invalid firefox profile '%s'" % opts.profile)
        
    return opts

def uninstall(profile):
    print "Uninstalling extension from profile:", profile
    instdir = install_dir(profile)
    shutil.rmtree(instdir, ignore_errors=True)

def install(profile):
    print "Installing extension to profile:", profile
    thisdir = os.path.abspath(os.path.dirname(sys.argv[0]))
    srcdir = os.path.join(thisdir, 'extfiles')

    instdir = install_dir(profile)
    if os.path.exists(instdir): shutil.rmtree(instdir)
    
    shutil.copytree(srcdir, instdir, ignore=shutil.ignore_patterns('.svn'))
    for dir, _, files in os.walk(instdir):
        for f in files:
            with open(os.path.join(dir, f), 'r') as fp: s = fp.read()
            for i in extension:
                s = s.replace('%%EXT_%s%%' % i, extension[i])
            with open(os.path.join(dir, f), 'w') as fp: fp.write(s)
    
def setup():
    options = parse_args()
    if options.cmd == "uninstall":
        uninstall(options.profile)
    elif options.cmd == "install":
        install(options.profile)

if __name__ == "__main__":
    setup()
