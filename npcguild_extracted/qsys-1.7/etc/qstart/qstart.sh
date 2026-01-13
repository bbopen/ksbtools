#!/bin/ksh
# By Kevin Braunsdorf (ksb@cc.purdue.edu)
# $Id: qstart.sh,v 1.5 1995/09/19 22:19:18 kb207252 Exp $

# set timezone from /etc/profile -- we can't source it `cause it
# does stty stuff and cats /etc/motd. sigh!
eval `grep TZ= /etc/profile`
export TZ

PATH=/bin:/usr/bin:_Q_BIN_:_Q_ETC_:/etc:/usr/ucb
export PATH
cd _Q_SPOOL_
umask 022

exec >>_Q_LOGFILE_ 2>&1 </dev/null
/bin/echo "`date`: Qmgr started on `hostname`"

for q in `find * \( -type d -group qmgr -print -prune \) -o -prune `
do
	sleep `expr 5 + $RANDOM % 20`
	if [ -f $q/.q-opts ] ; then
		su _Q_MASTER_ < $q/.q-opts &
	else
		echo "exec _Q_ETC_/qmgr -d $q -1" | su _Q_MASTER_ &
	fi
done
wait

/bin/echo "`date`: Qmgr shutdown on `hostname`"
