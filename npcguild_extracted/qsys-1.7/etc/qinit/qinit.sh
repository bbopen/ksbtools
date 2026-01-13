#!/bin/sh
# init the queues listed ar positional params
# >an example shell command line parser that conforms to the getopt	(ksb)
# >standard, Kevin Braunsdorf, ksb@cc.purdue.edu
#
# $Compile(*): ${sed-sed} -e '/^#[^!]/d' < %f > prog && chmod 755 prog

# our program name and usage message
progname=`basename $0`
usage="$progname: usage [-h | -V] [queues]"

# how to slide a single letter option off the beginning of a bundle
# -barf -> -arf
slide='P=$1; shift; set _ -`expr "$P" : '\''-.\(.*\)'\''` ${1+"$@"}; shift'
param='if [ $# -lt 2 ]; then echo "$progname: missing value for $1" 1>&2 ; exit 1; fi'

# read an environment variable as well as the command line options:
# protect this script from leading -x's with a bogus underbar, then remove it
set _ $QINIT ${1+"$@"}
shift

# get the options from the command line (+ any variables)
while [ $# -gt 0 ]
do
	case "$1" in
	--)
		shift
		break
		;;
	-h|-h*)
		cat <<HERE
$usage
h	print this help message
queues	queues to build (or fix), provide full path if not in default spool
HERE
		exit 0
		;;
	-V|-V*)
		echo "$progname: "'$Id: qinit.sh,v 1.3 1994/02/11 16:21:16 ksb Exp $'
		exit 0
		;;
	-*)
		echo "$usage" 1>&2 
		exit 1
		;;
	*)
		break
		;;
	esac
done

# get the dirs left on the line

for Qp in "${@-_Q_DIR_}"
do
	if expr "$Qp" : '/.*' >/dev/null
	then
		Q="$Qp"
	else
		Q=_Q_SPOOL_/$Qp
	fi

	[ -d "$Q" ] || mkdir "$Q" || exit 2
	chmod 0770 "$Q"
	>"$Q/.qlock"
	chgrp _Q_GROUP_ "$Q" "$Q/.qlock" || exit 4
	chmod 0660 "$Q/.qlock"
	if [ -f "$Q"/.q-opts ]
	then
		echo 1>&2 "$PROGNAME: $Qp already has .q-opts"
	fi
done

exit 0
