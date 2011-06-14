#! /usr/bin/python
#
# compressing /mnt/undofs/snap
#
# 1. whenever it reach to the threashold (500M)
# 2. every 5 min (if not zero)
#
# /tmp/snap/[time-when-compressing]
#

import os
import gzip
import time
import tempfile

from glob import glob
from optparse import OptionParser

def daemon_loop( current, time_limit, len_limit, base, archive ) :
    #
    # is undosys working?
    #
    if not os.path.exists( base ) :
        return False

    logs = glob( base + "/*" )

    if ( current > time_limit and len(logs) >= 1 ) or len( logs ) > len_limit :
        (fd, tmp) = tempfile.mkstemp( text=True )
        
        out = os.fdopen( fd, "w" )
        out.write( "\n".join( logs ) )
        out.close()

        zzip = time.time()
        cmd = 'bash -c "tar --remove-files --files-from=%s -czf %s/%s.tar.gz &> /dev/null"' \
            % ( tmp, archive, zzip )

        os.system( cmd )        
        os.system( "rm %s" % tmp )

        print "compressed %d files => %s (%d bytes)" \
            % ( len(logs), zzip, os.stat( "%s/%s.tar.gz" % ( archive, zzip ) ).st_size )
        
        return True

    return False

if __name__ == "__main__" :
    parser = OptionParser()
    parser.add_option( "-b", "--base", default="/mnt/undofs/snap", type="string",
                       dest="base", help="snapshot base directory" )
    parser.add_option( "-t", "--time", default=60, type="int",
                       dest="time", help="time limit as second" )
    parser.add_option( "-s", "--size", default=100000, type="int",
                       dest="size", help="size limit as KB" )
    parser.add_option( "-d", "--dest", default="/tmp/snap", type="string",
                       dest="dest", help="snapshot archive" )

    opt, args = parser.parse_args()
    
    os.system( "mkdir -p %s" % opt.dest )
    time_unit = 1

    current = 0
    while True :
        if daemon_loop( current, opt.time, 100, opt.base, opt.dest ) :
            currnet = 0
            
        time.sleep( time_unit )
        current += time_unit

        
