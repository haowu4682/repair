#!/bin/sh

thisdir="`dirname $0`"

if [ X"$1" = X"--reset" ]; then
    RESET="yes"
    shift
fi

if [ $# -eq 0 ]; then
    WORKLOAD="${thisdir}/workload.sh"
else
    WORKLOAD="$@"
fi

if [ X"$RESET" = X"yes" ]; then
    rm -f /tmp/retro/db.log /tmp/retro/httpd.log
fi

if [ -f '/tmp/retro/use_postgres' ]; then
    if [ X"$RESET" = X"yes" ]; then
        db/pg_setup.py setup
        db/psql.sh -f ${thisdir}/zoobar/pgsql/schema.sql
    fi
    python db/gen_pg_wrapper.py db/pg_wrapper.php
else
    if [ X"$RESET" = X"yes" ]; then
        git checkout ${thisdir}/zoobar/db/zoobar/Person.txt
        git checkout ${thisdir}/zoobar/db/zoobar/Transfers.txt
        rm -rf ${thisdir}/zoobar/db/zoobar/.snap.*
    fi
fi

make -C ${thisdir}
${thisdir}/httpd 8080 ${thisdir}/zoobar &
PID=$!

echo "Waiting so that httpd can initialize..."
sleep 3
${WORKLOAD}

kill $PID
