#!/bin/sh

export APACHE_RUN_USER=${USER}
export APACHE_RUN_GROUP=${USER}
export APACHE_PID_FILE=/tmp/apache2.pid
export APACHE_LOG_DIR=/tmp
export APACHE_LOG_ERR=/tmp/apache-error.log
export APACHE_LOG_CUS=/tmp/custom-error.log
export APACHE_MEDIAWIKI=`pwd`/mediawiki
export APACHE_PORT=8888

apache2 -d `pwd` -k start -c "ServerName localhost" -f ./conf/apache2.conf

