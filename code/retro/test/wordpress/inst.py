#!/usr/bin/python
import os
import re
import sys
import urllib2
import optparse

from subprocess import Popen, PIPE
from BeautifulSoup import BeautifulSoup

root = "http://wordpress.org/download/release-archive/"

def fetch(url,ver=None):
    html = BeautifulSoup(urllib2.urlopen(url).read())
    html = html.findAll("table")[0]
    menu = html.findAll("tr")[1:]

    if ver == None:
        print "Choose:"

    # filteringhtml
    menu = [m for m in menu if "IIS" not in m.contents[0].contents[0]]

    for i, m in enumerate(menu):
        tag  = m.contents[0].contents[0]
        link = m.contents[4].a["href"]

        if ver == None:
            print "%s) %s %s" % (str(i).rjust(2), tag.ljust(15), link.split("/")[-1])
        else:
            # check version that we are looking for
            if ver in tag:
                ver = i
                break

    if ver == None:
        ver = int(raw_input(">> "))

    # link
    return menu[ver].contents[4].a["href"]

def parse():
    p = optparse.OptionParser()
    p.add_option("-c", help="configure only",
                 dest="only_config", action="store_true", default=False)

    return p.parse_args()

if __name__=="__main__":
    (opts, args) = parse()

    if not opts.only_config:
        url = fetch(root, ver=(args[0] if len(args) == 1 else None))
        print "[!] downloading", url
        zip = urllib2.urlopen(url).read()
        tar = url.split("/")[-1]

        # creating directory
        dir = tar.replace(".tar.gz", "")
        os.system("mkdir -p %s" % dir)

        print "[!] uncompressing", tar, " size:", len(zip)
        p = Popen(["/bin/tar", "zxv", "-C", dir, "--strip-components=1", "--"], stdin=PIPE)
        p.stdin.write(zip)
        p.stdin.close()

        if p.wait() != 0:
            with open(tar, "wb") as f:
                f.write(zip)
                print "failed to uncompress, check %s" % tar
                exit(1)

        # link
        print "[!] link:", dir
        os.system("rm -f wordpress")
        os.system("ln -s '%s' wordpress" % dir)

    # configuration
    with open("wordpress/wp-config-sample.php") as fd:
        wp_config = fd.read()
        
        db = eval(open("conf/wordpress.py").read())
        
        wp_config = wp_config.replace("database_name_here"  , db["DB_NAME"]    )
        wp_config = wp_config.replace("username_here"       , db["DB_USER"]    )
        wp_config = wp_config.replace("password_here"       , db["DB_PASSWORD"])
        
        with open("wordpress/wp-config.php", "w") as fw:
            fw.write(wp_config)

        print "[!] configured"
        print " db  :", db["DB_NAME"]
        print " user:", db["DB_USER"]
        print " pass:", db["DB_PASSWORD"]
        
        print "[!] wp-config.php was created"
        
    print "[!] ./run.sh and open localhost:8888"
