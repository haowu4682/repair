#!/bin/sh -x

ROOT=/home/ssd/Temp

mkdir -p $ROOT/retro-zip
mkdir -p $ROOT/retro-log

fusecompress $ROOT/retro-zip $ROOT/retro-log