#!/bin/bash
if [ $# -ne 2 ] ; then
    echo "usage: $0 dir1 dir2";
    exit 1;
fi

# check db sequence
diff <(cat $1/db.log | cut -d" " -f3,4) <(cat $2/db.log | cut -d" " -f3,4)

# check HTTP_X requests
diff <(cat $1/httpd.log | grep HTTP_X | cut -d" " -f3-) <(cat $2/httpd.log | grep HTTP_X | cut -d" " -f3-)