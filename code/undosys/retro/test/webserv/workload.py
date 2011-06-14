#!/usr/bin/python

import subprocess
import sys
import os

pages = {
    "1": [ ("index.php", "--post-data", "login_username=alice&login_password=alice&submit_registration=Register"),
           ("index.php", "--post-data", "profile_update=Alice-profile&profile_submit=Save") ],
    "2": [ ("index.php", "--post-data", "login_username=bob&login_password=bob&submit_registration=Register"),
           ("index.php?hint=attack", "--post-data", "profile_update=Evil-bob-profile&profile_submit=Save") ],
    "3": [ ("index.php", "--post-data", "login_username=alice&login_password=alice&submit_login=Log in"), 
           ("users.php?user=bob", ) ]
}

redo_pages = {
    "2": [ ("index.php", "--post-data", "login_username=bob&login_password=bob&submit_registration=Register"),
           ("index.php", "--post-data", "profile_update=Nice-bob-profile&profile_submit=Save") ]
}

def wget(args):
    print "+++ Running wget:", args
    subprocess.call(["wget"] + args)
    
def fetch_page(pageid, page):
    url = "http://localhost:8080/"
    cookie_file = "/tmp/wget.cookie"
    if os.path.exists(cookie_file): os.unlink(cookie_file)
    
    for req in page:
        r = list(req)
        r[0] = url + r[0]
        r += [ "--load-cookies", cookie_file, "--save-cookies", cookie_file,
               "--header", "X_PAGE_ID: %s" % pageid,
               "--header", "X_REQ_ID: %s" % pageid,
               "--header", "X_CLIENT_ID: %s" % pageid,
               "-O", "/dev/null" ]
        wget(r)
    
if len(sys.argv) != 3:
    print "Usage: %s [do|redo] pageid" % sys.argv[0]
    exit(1)
    
action = sys.argv[1]
pageid = sys.argv[2]

if action != "do" and action != "redo":
    print "invalid action %s. should be 'do' or 'redo'" % action
    exit(1)
    
page = None
if action == "redo" and pageid in redo_pages:
    page = redo_pages[pageid]
if page == None and pageid in pages:
    page = pages[pageid]
if page == None:
    print "could not find pageid", pageid

fetch_page(pageid, page)
