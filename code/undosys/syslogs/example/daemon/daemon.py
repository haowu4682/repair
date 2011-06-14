#! /usr/bin/python
#
# archiving /tmp/record/* to /tmp/record/*.gz
#
# 1. every 5 min (if not zero)
# 
# /tmp/record/[time-when-fetching]
#

import os
import gzip
import time

from glob import glob

def daemon_loop( current, time_limit, size_limit, archive ) :
    #
    # is undosys working?
    #
    if not os.path.exists( '/proc/undosys/stat' ) :
        return False

    files = glob( "/tmp/record/*.log" )

    if ( current > time_limit ) and len( files ) != 0 :
        
        for f in files :
            orig = os.stat( f ).st_size
            os.system( 'bash -c "tar --remove-files -czf %s.tar.gz %s &> /dev/null"' % (f,f) )
            zzip = os.stat( "%s.tar.gz" % f ).st_size

            print "Compressed %s(%d) -> %s(%d)" % (f, orig, f, zzip)
            
        return True

    return False

if __name__ == "__main__" :
    time_unit  = 10
    time_limit = 30
    size_limit = 10000
    
    archive = "/tmp/record"
    os.system( "mkdir -p %s" % archive )
    
    current = 0
    
    while True :
        if daemon_loop( current, time_limit, size_limit, archive ) :
            currnet = 0
        time.sleep(time_unit)
        current += time_unit

        
