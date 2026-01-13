#!/bin/sh
# $Id: pfunc.sh,v 3.2 1996/07/13 01:31:14 kb207252 Dist $
#
# command to list given functions from a file				(ksb)
#
PROGNAME=`basename $0 .sh`

PATH=$PATH:/usr/local/bin

case $# in
0)
	echo "$PROGNAME: usage 'functions' [-Ddef] [-Iincl] [-Udef] [files.c]" 1>&2
	exit 1 ;;
*)
	funcs=$1
	shift ;;
esac

calls -tix "$@" | sed -e "/Index:/d" -e 's/^static//' -e 's/[ 	]*//' >/tmp/flist$$
for func in $funcs
do
	grep -e "^$func[ 	]" /tmp/flist$$ >/tmp/fp$$ || continue
	while read line 
	do
		set _ `echo $line | sed -e s'/[()]/ /g'`
		sed -n -e "$4p" <$3
	done </tmp/fp$$
done

rm -f /tmp/flist$$ /tmp/fp$$

exit 0
