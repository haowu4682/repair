#!/bin/sh

thisdir="`dirname $0`"
db_params="`cat $thisdir/connect_params | sed -e 's/host=/-h /' | sed -e 's/user=/-U /' | sed -e 's/dbname=//' | tr -d \'`"
$thisdir/pg_inst/bin/psql ${db_params} $@
