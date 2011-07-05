#!/bin/sh
git pull
make
cd retro; make -C ctl && make -C trace

