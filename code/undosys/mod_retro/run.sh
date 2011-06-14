#!/bin/sh

export APACHE_RUN_USER=${USER}
export APACHE_RUN_GROUP=${USER}
export APACHE_PID_FILE=/tmp/apache2.pid
export APACHE_LOG_DIR=/tmp
export APACHE_LOG_ERR=/tmp/apache-error.log
export APACHE_LOG_CUS=/tmp/custom-error.log
export APACHE_WEBDIR=`pwd`/../retro/test/webserv/zoobar
export APACHE_PORT=8888
export APACHE_MOD_RETRO=`pwd`

if [ -e ${APACHE_PID_FILE} ] ; then
  kill -9 `cat ${APACHE_PID_FILE}` 2> /dev/null
fi

rm -f ${APACHE_PID_FILE}

apache2 -X -d `pwd` -k start -c "ServerName localhost" -f ./conf/apache2.conf

