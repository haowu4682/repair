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

#PHP_RESIN="/disk/yipal/obj/php-taint-opt/sapi/cli/php"
#PHP_ORIG="/disk/yipal/obj/php-opt/sapi/cli/php"
PHP_ORIG="/usr/bin/php"

#RESIN_LOG=resin.log
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
        OUT_LOG=../$5
    fi

    cd ../$DIR
    HOTCRP_DB=$DB sudo ./Code/createdb.sh

    echo "Removing old sessions"
    rm -f /tmp/sess_$DB*

    echo "Adding users and papers to DB, creating sessions"
    $PHP ../hotcrp-2.26-benchmark/create_users.php $DB $NCHAIR $NPC $NAUTH $PAPERSIZE $USE_POLICY $DB

    echo "Running Test"

    printf "$DIR paper.php $REPS " >> ../$RESULTS
    USE_ZEND_ALLOC=$USE_ZEND_ALLOC \
    NCHAIR=$NCHAIR \
    NPC=$NPC \
    NAUTH=$NAUTH \
    SESSNAME=$DB \
    /home/taesoo/Working/undosys/lang-ifc/hotcrp-2.26-benchmark/bench.py $REPS $PHP paper.php 2>&1 >> ../$RESULTS

    # 
    # original code
    # 
    # time $PHP -N $REPS paper.php 2>&1 \
    # > $OUT_LOG | \
    # perl -ne "if (/(\d+):([\d.]+)elapsed/) {\$sec = \$1 * 60 + \$2; printf (\"%f\n\", (\$sec/$REPS));}" >> ../$RESULTS
    
    cd ..
}

benchsys () {
    DIR=$1
    DB=$2
    PHP=$3
    USE_POLICY=$4
    if [ -z "$5" ]; then
        OUT_LOG=/dev/null
    else
        OUT_LOG=../$5
    fi

    cd ../$DIR
    HOTCRP_DB=$DB sudo ./Code/createdb.sh

    echo "Removing old sessions"
    rm -f /tmp/sess_$DB*

    echo "Adding users and papers to DB, creating sessions"
    $PHP ../hotcrp-2.26-benchmark/create_users.php $DB $NCHAIR $NPC $NAUTH $PAPERSIZE $USE_POLICY $DB

    echo "Running Test"

    # load syslogs
    cd /home/taesoo/Working/undosys/undosys/syslogs
    modprobe nfs
    make insmod
    cd /home/taesoo/Working/undosys/lang-ifc/$DIR

    printf "$DIR paper.php $REPS " >> ../$RESULTS
    USE_ZEND_ALLOC=$USE_ZEND_ALLOC \
    NCHAIR=$NCHAIR \
    NPC=$NPC \
    NAUTH=$NAUTH \
    SESSNAME=$DB \
    /home/taesoo/Working/undosys/undosys/syslogs/example/loader/loader /home/taesoo/Working/undosys/lang-ifc/hotcrp-2.26-benchmark/bench.py $REPS $PHP paper.php 2>&1 >> ../$RESULTS
    
    cd ..
}

benchsys_test () {
    DIR=$1
    DB=$2
    PHP=$3
    USE_POLICY=$4
    if [ -z "$5" ]; then
        OUT_LOG=/dev/null
    else
        OUT_LOG=../$5
    fi

    cd ../$DIR
    #HOTCRP_DB=$DB sudo ./Code/createdb.sh

    echo "Removing old sessions"
    #rm -f /tmp/sess_$DB*

    echo "Adding users and papers to DB, creating sessions"
    #$PHP ../hotcrp-2.26-benchmark/create_users.php $DB $NCHAIR $NPC $NAUTH $PAPERSIZE $USE_POLICY $DB

    echo "Running Test"

    # load syslogs
    # cd /home/taesoo/Working/undosys/undosys/syslogs
    # modprobe nfs
    # make insmod
    # cd /home/taesoo/Working/undosys/lang-ifc/$DIR

    printf "$DIR paper.php $REPS " >> ../$RESULTS
    USE_ZEND_ALLOC=$USE_ZEND_ALLOC \
    NCHAIR=$NCHAIR \
    NPC=$NPC \
    NAUTH=$NAUTH \
    SESSNAME=$DB \
    /home/taesoo/Working/undosys/undosys/syslogs/example/loader/loader /home/taesoo/Working/undosys/lang-ifc/hotcrp-2.26-benchmark/bench.py $REPS $PHP paper.php 2>&1 >> ../$RESULTS
    
    cd ..
}

rm -f $RESIN_LOG $ORIG_LOG
touch $RESULTS

#bench    "hotcrp-2.26-orig" "orig" $PHP_ORIG "0" $ORIG_LOG
benchsys "hotcrp-2.26-orig" "orig" $PHP_ORIG "0" $ORIG_LOG
#benchsys_test "hotcrp-2.26-orig" "orig" $PHP_ORIG "0" $ORIG_LOG

# if [ ! -z "$RESIN_LOG" ]; then
#   cat $RESIN_LOG | egrep blind | wc
# fi
if [ ! -z "$ORIG_LOG" ]; then
  cat $ORIG_LOG | egrep blind | wc
fi

cat $RESULTS
