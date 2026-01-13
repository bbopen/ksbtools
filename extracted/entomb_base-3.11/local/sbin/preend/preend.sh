#!/bin/sh
# $Id: preend.sh,v 3.2 2006/01/27 21:15:22 ksb Exp $
# Freebsd startup script for /usr/local/etc/rd.d -- ksb

# PROVIDE: entomb
# REQUIRE: fsck

case ${1:-help} in
start)
	if [ -x /usr/local/etc/preend ] ; then
		echo -n " preend"
		/usr/local/etc/preend -p
	fi
	;;
stop)
	pkill -TERM -P 1 preend
	;;
*)
	echo "$0: usage {start|stop|help}"
	;;
esac
