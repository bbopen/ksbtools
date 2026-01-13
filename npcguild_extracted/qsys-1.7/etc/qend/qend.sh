#!/bin/sh
# $Id: qend.sh,v 1.4 1997/01/05 22:06:43 kb207252 Exp $

PROGNAME=`basename $0 .sh`
usage="$PROGNAME: usage [-prio] [queues]"

# how to slide a single letter option off the beginning of a bundle
# -barf -> -arf
slide='P=$1; shift; set _ -`expr "$P" : '\''-.\(.*\)'\''` ${1+"$@"}; shift'
param='if [ $# -lt 2 ]; then echo "$PROGNAME: missing value for $1" 1>&2 ; exit 1; fi'

# default values for all the flags, or leave unset for a ${flag-value) form
PRIO=--5

# read an environment variable as well as the command line options:
# protect this script from leading -x's with a bogus underbar, then remove it
set _ $QEND ${1+"$@"}
shift

# get the options from the command line (+ any variables)
while [ $# -gt 0 ]
do
	case "$1" in
	-[0-9]|-[-0-9][0-9]|-[-0-9][0-9][0-9])
		PRIO="$1"
		shift
		;;
	--)
		shift
		break
		;;
	-h|-h*)
		cat <<HERE
$usage
$PROGNAME: usgae -h
$PROGNAME: usgae -V
-prio	set the priority of the scheduled end
h	print this help message
V	show version information
queues	queues to shutdown
HERE
		exit 0
		;;
	-V|-V*)
		echo "$PROGNAME: $Id: qend.sh,v 1.4 1997/01/05 22:06:43 kb207252 Exp $"
		exit 0
		;;
	-*)
		echo "$usage" 1>&2 
		exit 1
		;;
	*)
		# process and continue for intermixed options & args
		break
		;;
	esac
done

# find all the queues on the system, be default
if [ $# -eq 0 ]
then
	cd _Q_SPOOL_
	set _ `find * \( -type d -group _Q_GROUP_ -print -prune \) -o -prune`
	shift
fi

if [ "`whoami`" != root -a "`whoami`" != _Q_MASTER_ ]
then
	echo 1>&2 "$PROGNAME: I doubt this will work for you..."
fi

# some shells don't grok:
#	MESG=`cd $Q 2>&1`
# sigh.  shell is powerful, emulation of shell is buggy! -- ksb

for Q in "${@-qsys}"
do
	cd _Q_SPOOL_
	if MESG=`cd $Q 2>&1`
	then
		qadd ${PRIO} -N '* * * End of production * * *' 'exec ksh -c "kill -TERM \$PPID"'
		continue
	fi
	echo 1>&2 "$PROGNAME: $MESG"
done

exit 0
