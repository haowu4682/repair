#!/bin/bash 

export APACHE_RUN_USER=${USER}
export APACHE_RUN_GROUP=${USER}
export APACHE_PID_FILE=/tmp/apache2.pid
export APACHE_LOG_DIR=/tmp
export APACHE_LOG_ERR=/tmp/apache-error.log
export APACHE_LOG_CUS=/tmp/custom-error.log
export APACHE_WEBDIR=${2:-`pwd`/mediawiki}
export APACHE_PORT=${1:-8080}
export APACHE_MOD_RETRO=`pwd`/../../../mod_retro

kill_apache() {
  kill -9 `cat ${APACHE_PID_FILE}` 2> /dev/null
}  

atexit() {
  kill_apache
  exit 1
}

if [ -e ${APACHE_PID_FILE} ] ; then
  kill_apache
fi

rm -f ${APACHE_PID_FILE}

# trap atexit INT TERM EXIT

apache2 -d `pwd` -k start -c "ServerName localhost" -f ${APACHE_MOD_RETRO}/conf/apache2.conf

