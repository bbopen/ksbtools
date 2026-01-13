#!/bin/sh
# $Id: submit.sh,v 5.1 2000/01/06 15:53:37 ksb Exp $
# Bugs: this doesn't let you turnin an empty file, which was a
# requested feature by a client.  I think that is silly. -- ksb

# change your course name/number here
COURSE=cs100

# our program name and usage message
PROGNAME=`basename $0`
USAGE="$PROGNAME: usage [-vh] [files]"
PATH=/usr/local/bin:$PATH

# how to slide a single letter option off the beginning of a bundle
# -barf -> -arf
slide='P=$1; shift; set _ -`expr "$P" : '\''-.\(.*\)'\''` ${1+"$@"}; shift'
param='if [ $# -lt 2 ]; then echo "$progname: missing value for $1" 1>&2 ; exit 1; fi'

# default values for all the flags, or leave unset for a ${flag-value) form
VFLAG=""

# get the options from the command line (+ any variables)
while [ $# -gt 0 ]
do
	case "$1" in
	-v)
		VFLAG=-v
		shift
		;;
	-v*)
		VFLAG=-v
		eval "$slide"
		;;
	--)
		shift
		break
		;;
	-h|-h*)
		cat <<HERE
$usage
h      output this help message
v      be verbose
files  files to choke the user with
(if no files, list active project)
HERE
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

# get the files left on the line
if [ $# -eq 0 ]
then
	exec turnin -c$COURSE -l
fi

for file
do
	[ -s $file ] && continue
	echo "$PROGNAME: $file is empty"
	exit 1
done
exec turnin -c$COURSE ${VFLAG} "$@"
