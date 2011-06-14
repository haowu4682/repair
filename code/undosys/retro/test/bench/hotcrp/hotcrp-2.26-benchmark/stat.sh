#! /bin/sh -e

mkdir -p $1
cd $1

cp ../results.log .
/bin/rm ../results.log

ROOT=/home/ssd/Temp

ls -al $ROOT/retro-log > ls.log.txt
ls -al $ROOT/retro-zip > ls.zip.txt
ls -al $ROOT/retro-aux > ls.aux.txt

cp -rf $ROOT/retro-log .
cp -rf $ROOT/retro-aux .

cd ..
chown -R taesoo:taesoo $1

fusermount -u $ROOT/retro-log