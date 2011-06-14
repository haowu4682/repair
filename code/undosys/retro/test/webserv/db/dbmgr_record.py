#!/usr/bin/python

test_queries = [
    "INSERT INTO PERSON (USERNAME, PASSWORD, SALT) VALUES ('alice', 'abcdef', 'uvwxyz')",
    "INSERT INTO PERSON (USERNAME, PASSWORD, SALT) VALUES ('bob', 'abcdef', 'uvwxyz')",
    "UPDATE PERSON SET PROFILE='Evil profile' WHERE USERNAME='alice'",
    "UPDATE PERSON SET PROFILE='Bob profile' WHERE USERNAME='bob'",
    "SELECT USERNAME FROM PERSON WHERE USERNAME='bob'",
    "SELECT SALT FROM PERSON WHERE USERNAME='bob'",
    "SELECT * FROM PERSON WHERE USERNAME='bob' AND PASSWORD='abcdef'",
    "SELECT USERNAME FROM PERSON WHERE USERNAME='alice'",
    "SELECT SALT FROM PERSON WHERE USERNAME='alice'",
    "SELECT * FROM PERSON WHERE USERNAME='alice' AND PASSWORD='abcdef'",
    ]

import subprocess, os, sys

popen   = subprocess.Popen
thisdir = os.path.abspath(os.path.dirname(__file__))

## setup test schema
print "++++++++++ Setting up schema for test tables ++++++++++"
os.system("%s/psql.sh -f %s/../zoobar/pgsql/schema.sql" % (thisdir, thisdir))
os.system("%s/dbparts_create.py" % thisdir)
for fn in ['/tmp/retro/retro_rerun', '/tmp/retro/php.log', '/tmp/retro/db.log']:
    if os.path.exists(fn): os.unlink(fn)

## generate the postgres wrapper for php
print "\n++++++++++ Generating postgres wrapper for php ++++++++++"
os.system("%s/gen_pg_wrapper.py %s/pg_wrapper.py" % (thisdir, thisdir))

## run the test queries 
print "\n++++++++++ Running the test queries ++++++++++"
for q in test_queries:
    print 'Running query:', q
    env = {}
    env['DB_QUERY'] = q
    popen([thisdir + "/dbq.php"], env = env).wait()
    
