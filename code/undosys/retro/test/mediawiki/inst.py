#!/usr/bin/python
import os
import re
import sys
import urllib2

from subprocess import Popen, PIPE
from BeautifulSoup import BeautifulSoup

root = "http://download.wikimedia.org/mediawiki/"

def fetch(url, f=None):
    html = BeautifulSoup(urllib2.urlopen(url).read())
    menu = html.findAll("tr")[2:]

    if f:
        menu = filter(lambda i: re.search(f, str(i.contents[0].a)), menu)

    print "Choose:"
    for i, m in enumerate(menu):
        link = m.contents[0].a.contents[0]
        date = m.contents[1].contents[0]
        print "%s) %s %s" % (str(i).rjust(2), date, link.ljust(20))
        
    i = int(raw_input(">> "))
    
    return menu[i].contents[0].a.contents[0]

def pg_setup():
    thisdir = os.path.dirname(os.path.abspath(__file__))
    pgdir   = thisdir + "/../webserv/db"
    os.system("%s/pg_setup.py setup" % pgdir)
    os.system("%s/pg_inst/bin/createuser -s wikiuser" % pgdir)
    os.system("%s/pg_inst/bin/createdb -h localhost wikidb -U wikiuser" % pgdir)

if __name__=="__main__":

    # help
    if "-h" in sys.argv or "--help" in sys.argv:
        print "usage:"
        print " -m: mysql (default:postgre)"
        print " -s: skip downloading and configure mediawiki"
        print " -pg: setup postgres"
        exit(0)

    db = "postgre"
    if "-m" in sys.argv:
        db = "mysql"

    # skip downloading
    if not "-s" in sys.argv:
        ver = fetch(root)
        tar = fetch(root + ver, "tar.gz[^.]")
        url = root + (ver + "/" + tar)

        print "[!] downloading", url
        zip = urllib2.urlopen(url).read()

        print "[!] uncompressing", tar, " size:", len(zip)
        p = Popen(["/bin/tar", "zxv", "--"], stdin=PIPE)
        p.stdin.write(zip)
        p.stdin.close()

        if p.wait() != 0:
            with open(tar, "wb") as f:
                f.write(zip)
            print "failed to uncompress, check %s" % tar
            exit(1)

        # link
        os.system("rm -f mediawiki")

        dir = tar.replace(".tar.gz", "")
        os.system("ln -s '%s' mediawiki" % dir)

        print "[!] link:", dir

    # configure
    print "[!] %s options:" % db
    
    conf = "%sconf.php" % ("pg" if db == "postgre" else "my")
    os.system("cat conf/%s | grep _POST" % conf)

    if db == "postgre" and "-pg" in sys.argv:
        pg_setup()
    os.system("rm -f mediawiki/LocalSettings.php")
    os.system("cp conf/%s mediawiki/config" % conf)
    os.system("cd mediawiki/config; ./%s > /dev/null 2>/dev/null" % conf)
    os.system("mv mediawiki/config/LocalSettings.php mediawiki")

    print "[!] copy LocalSettings.php"
    print "[!] ./run.sh and open localhost:8888/index.php/Main_Page"

