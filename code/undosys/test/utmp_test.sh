#!/bin/sh

./utmp_set login  tx01
./utmp_set login  tx02
./utmp_set logout tx01
./utmp_set login  tx03
./utmp_set logout tx02
./utmp_set logout tx03

