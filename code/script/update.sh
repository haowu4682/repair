#!/bin/sh
git checkout repair && git pull
make
cd trace; make -C ctl && make -C trace

