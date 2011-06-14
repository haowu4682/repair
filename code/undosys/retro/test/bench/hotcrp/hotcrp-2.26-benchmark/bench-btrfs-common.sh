NCHAIR=2
NPC=33
NAUTH=300
PAPERSIZE=100000

REPS=1000

BENCH=/mnt/retro/trunk/tmp/hotcrp-2.26-benchmark
CHROOT_BENCH=/tmp/hotcrp-2.26-benchmark
RETRO=/home/taesoo/Working/undosys/undosys/retro

# choose php
PHP_ORIG="/usr/bin/php"

# logs
ORIG_LOG=orig.log
RESULTS=results.log

USE_ZEND_ALLOC=0

bench () {
	rm -f $RESIN_LOG $ORIG_LOG
	touch $RESULTS

    DIR=$1
    DB=$2
    PHP=$3
    USE_POLICY=$4
    if [ -z "$5" ]; then
        OUT_LOG=/dev/null
    else
        OUT_LOG=$5
    fi
    EXTRACTL=$7

    if [ "$6" = "retro" ]; then
        ROOT=/home/ssd/Temp

        /bin/rm -rf $ROOT/retro-log
        /bin/rm -rf $ROOT/retro-zip

        mkdir -p $ROOT/retro-log
        mkdir -p $ROOT/retro-zip

        cmd="$RETRO/ctl/retroctl -d $BENCH/../hotcrp-2.26-orig -p $EXTRACTL -r -x result.log -o $ROOT/retro-log -- "
        
        sudo insmod $RETRO/trace/retro.ko
        sudo insmod $RETRO/trace/iget.ko
    else
        cmd=""
    fi

    CHROOT="chroot /mnt/retro/trunk"

    $CHROOT /usr/sbin/mysqld &
    sleep 5
    
    echo "=========================="
    echo "Creating DB"
    echo "=========================="
    cd /mnt/retro/trunk/tmp/$DIR
    HOTCRP_DB=$DB\
    $CHROOT $CHROOT_BENCH/../hotcrp-2.26-orig/Code/createdb.sh

    echo "=========================="
    echo "Removing old sessions"
    $CHROOT /bin/rm -f /tmp/sess_$DB*

    echo "=========================="
    echo "Adding users and papers to DB, creating sessions"
    cd $BENCH/../hotcrp-2.26-orig
    $CHROOT bash -c "cd $CHROOT_BENCH/../hotcrp-2.26-orig; $PHP $CHROOT_BENCH/create_users.php $DB $NCHAIR $NPC $NAUTH $PAPERSIZE $USE_POLICY $DB"
    $CHROOT /usr/bin/mysqladmin shutdown

	if [ "$6" = "retro" ]; then
		sync
		btrfs subvolume snapshot /mnt/retro/trunk /mnt/retro/snap-`date +%s`
	fi

    echo "=========================="
    echo "Running Test"

    cd $BENCH/../hotcrp-2.26-orig

    echo -n `date` $6 "\t" `cat /sys/devices/system/cpu/online` "\t" $DIR paper.php "\t" >> $BENCH/$RESULTS
    USE_ZEND_ALLOC=$USE_ZEND_ALLOC \
    NCHAIR=$NCHAIR \
    NPC=$NPC \
    NAUTH=$NAUTH \
    SESSNAME=$DB \
    $cmd $CHROOT bash -c "/usr/sbin/mysqld & sleep 5 ; cd $CHROOT_BENCH/../hotcrp-2.26-orig ; $PHP ../hotcrp-2.26-benchmark/create_users2.php $DB $NCHAIR $NPC $NAUTH $PAPERSIZE $USE_POLICY $DB >/dev/null 2>/dev/null ; $CHROOT_BENCH/bench.py $REPS $PHP paper.php ; /usr/bin/mysqladmin shutdown" 2>&1 > $BENCH/$OUT_LOG
    #$cmd $CHROOT bash -c "/usr/sbin/mysqld & sleep 5 ; cd $CHROOT_BENCH/../hotcrp-2.26-orig; echo $CHROOT_BENCH/bench.py $REPS $PHP paper.php ; /bin/bash ; /usr/bin/mysqladmin shutdown"
    $CHROOT cat /tmp/dump >> $BENCH/$RESULTS

    cd $BENCH

    # 
    # original code
    # 
    # time $PHP -N $REPS paper.php 2>&1 \
    # > $OUT_LOG | \
    # perl -ne "if (/(\d+):([\d.]+)elapsed/) {\$sec = \$1 * 60 + \$2; printf (\"%f\n\", (\$sec/$REPS));}" >> ../$RESULTS

	if [ ! -z "$RESIN_LOG" ]; then
  		cat $RESIN_LOG | egrep blind | wc
	fi
	if [ ! -z "$ORIG_LOG" ]; then
  		cat $ORIG_LOG | egrep blind | wc
	fi
	cat $RESULTS | tail
}

