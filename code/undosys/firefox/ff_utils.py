#!/usr/bin/python

import ConfigParser, os

def profile_dir(profile):
    cfg   = ConfigParser.ConfigParser()
    ffdir = os.path.expanduser('~/.mozilla/firefox/')
    cfg.read(os.path.join(ffdir, 'profiles.ini'))
    for s in cfg.sections():
        if cfg.has_option(s, "name") and cfg.get(s, "name") == profile:
            if cfg.has_option(s, "path"):
                return os.path.join(ffdir, cfg.get(s, "path"))
            else:
                return None
    return None

