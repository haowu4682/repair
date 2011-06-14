#!/usr/bin/python

import sys, os, getpass

thisdir = os.path.dirname(os.path.abspath(__file__))
instdir = thisdir + "/pg_inst"
user    = getpass.getuser()

def sh(cmd):
    print "\n+++", cmd
    exitval = os.system(cmd)
    if exitval != 0:
        print "ERROR executing shell cmd: %s (exit code = %d)" % (cmd, exitval)
        exit(1)

def build():
    ## unzip
    os.chdir(thisdir)
    sh("rm -rf postgresql-8.3.10 %s" % instdir)
    sh("/bin/tar -zxf postgresql-8.3.10.tar.gz")
    sh("rm postgresql-8.3.10/contrib/spi/timetravel.c")
    sh("ln -s %s/timetravel.c postgresql-8.3.10/contrib/spi/timetravel.c" % thisdir)

    ## build
    os.chdir("%s/postgresql-8.3.10" % thisdir)
    sh("./configure --enable-integer-datetimes --prefix=%s >/dev/null" % instdir)
    sh("make -j8 install >/dev/null")
    sh("make -C contrib -j8 >/dev/null")
    os.chdir(thisdir)

def initdb():
    sh("%s/bin/initdb %s/data" % (instdir, instdir))
    pg_ctl("start")
    sh("%s/bin/createdb -h localhost retro -U %s" % (instdir, user))

    ## setup timetravel 
    spidir = "%s/postgresql-8.3.10/contrib/spi" % thisdir
    with open("%s/timetravel.sql" % spidir) as rfp:
        outstr = rfp.read().replace("$libdir", spidir)
        with open("%s/timetravel.sql.out" % spidir, "w") as wfp: wfp.write(outstr)
    sh("%s/bin/psql -U %s retro < %s/timetravel.sql.out" % (instdir, user, spidir))

    sh("rm -f %s/connect_params" % thisdir)
    with open("%s/connect_params" % thisdir, "w") as fp:
        fp.write("host='localhost' user='%s' dbname='retro'" % user)

def check_install(msg):
    if not os.path.exists("%s/data/PG_VERSION" % instdir):
        print "ERROR %s: it needs to be installed first" % msg
        print "try %s setup" % sys.argv[0]
        exit(1)

def pg_ctl(cmd):
    check_install("%sing postgres" % cmd)
    sh("%s/bin/pg_ctl -w %s -D %s/data" % (instdir, cmd, instdir))

def usage(msg=None):
    if msg is not None: print "ERROR:", msg
    print "Usage: %s [setup|start|stop|restart|kill]" % sys.argv[0]
    exit(1)
    
def main():
    if len(sys.argv) != 2:
        usage()

    cmd = sys.argv[1]
    if cmd == "setup":
        build()
        initdb()
    elif cmd == "start" or cmd == "stop" or cmd == "restart" or cmd == "kill":
        pg_ctl(cmd)
    else:
        usage("invalid cmd" % cmd)

if __name__ == "__main__":
    main()
