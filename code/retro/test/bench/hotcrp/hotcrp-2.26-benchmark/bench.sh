#!/bin/sh
#
# resin, paper.php 91ms per request - unoptimized on laptop
#
# USE_ZEND_ALLOC=0 NCHAIR=1 NPC=2 NAUTH=3 time /disk/yipal/obj/php-taint-cgi/sapi/cli/php -N 1 paper.php 2>&1 > x.html

NCHAIR=2
NPC=33
NAUTH=300
PAPERSIZE=100000

REPS=1000

BENCH=`/bin/pwd`
RETRO=$BENCH/../../../..

# choose php
PHP_ORIG="/usr/bin/php"

# logs
ORIG_LOG=orig.log
RESULTS=results.log

USE_ZEND_ALLOC=0

bench () {
    DIR=$1
    DB=$2
    PHP=$3
    USE_POLICY=$4
    if [ -z "$5" ]; then
        OUT_LOG=/dev/null
    else
        OUT_LOG=$5
    fi

    if [ "$6" = "retro" ]; then
	ROOT=/home/ssd/Temp

	/bin/rm -rf $ROOT/retro-log
	/bin/rm -rf $ROOT/retro-zip

	mkdir -p $ROOT/retro-log
	mkdir -p $ROOT/retro-zip

	cmd="$RETRO/ctl/retroctl -p -r -x result.log -o $ROOT/retro-log --"

	sudo insmod $RETRO/trace/retro.ko
	sudo insmod $RETRO/trace/iget.ko
    else
	cmd=""
    fi

    echo "=========================="
    echo "Creating DB"
    echo "=========================="
    cd ../$DIR
    HOTCRP_DB=$DB sudo ./Code/createdb.sh

    echo "=========================="
    echo "Removing old sessions"
    rm -f /tmp/sess_$DB*

    echo "=========================="
    echo "Adding users and papers to DB, creating sessions"
    $PHP $BENCH/../hotcrp-2.26-benchmark/create_users.php $DB $NCHAIR $NPC $NAUTH $PAPERSIZE $USE_POLICY $DB

    echo "=========================="
    echo "Running Test"

    cd $BENCH/../hotcrp-2.26-orig

    echo -n `date` $6 "\t" `cat /sys/devices/system/cpu/online` "\t" $DIR paper.php "\t" >> $BENCH/$RESULTS
    USE_ZEND_ALLOC=$USE_ZEND_ALLOC \
    NCHAIR=$NCHAIR \
    NPC=$NPC \
    NAUTH=$NAUTH \
    SESSNAME=$DB \
    $cmd $BENCH/bench.py $REPS $PHP paper.php 2>&1 > $BENCH/$OUT_LOG
    cat /tmp/dump >> $BENCH/$RESULTS

    cd $BENCH

    # 
    # original code
    # 
    # time $PHP -N $REPS paper.php 2>&1 \
    # > $OUT_LOG | \
    # perl -ne "if (/(\d+):([\d.]+)elapsed/) {\$sec = \$1 * 60 + \$2; printf (\"%f\n\", (\$sec/$REPS));}" >> ../$RESULTS
}

rm -f $RESIN_LOG $ORIG_LOG
touch $RESULTS

mode="plain"
if [ "$1" ] ; then
    mode=$1
fi

bench "hotcrp-2.26-orig" "orig" $PHP_ORIG "0" $ORIG_LOG $mode

if [ ! -z "$RESIN_LOG" ]; then
  cat $RESIN_LOG | egrep blind | wc
fi
if [ ! -z "$ORIG_LOG" ]; then
  cat $ORIG_LOG | egrep blind | wc
fi

cat $RESULTS | tail
