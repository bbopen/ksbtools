#!/bin/sh
# $Id: preend.sh,v 3.1 1997/11/24 01:09:26 ksb Exp $
# Freebsd startup script for /usr/local/etc/rd.d -- ksb

if [ -x /usr/local/etc/preend ] ; then
	echo -n " preend"
	/usr/local/etc/preend -p
fi
