#!/bin/sh
# Install the given mkcmd template files				(ksb)
# Honor the existance of a directory to put new templates in, or
# take -d to force directory builds
# Be nice and use only Bourne Shell compatible commands.
#
PROGNAME=`basename $0 .sh`
USAGE="$PROGNAME: usage [-d] [-t sub] directory templates
$PROGNAME: usage -h
$PROGNAME: usage -V"

# how to slide a single letter option off the beginning of a bundle
# -barf -> -arf
slide='P=$1; shift; set _ -`expr "$P" : '\''-.\(.*\)'\''` ${1+"$@"}; shift'
param='if [ $# -lt 2 ]; then echo "$PROGNAME: missing value for $1" 1>&2 ; exit 1; fi'

# default values for all the flags, or leave unset for a ${flag-value) form
DFLAG=false

# read an environment variable as well as the command line options:
# protect this script from leading -x's with a bogus underbar, then remove it
set _ $UBINST ${1+"$@"}
shift

# get the options from the command line (+ any variables)
while [ $# -gt 0 ]
do
	case "$1" in
	-d)
		DFLAG=true
		shift
		;;
	-d*)
		DFLAG=true
		eval "$slide"
		;;
	-t)
		eval "$param"
		TYPE_DEF=$2
		shift ; shift
		;;
	-t*)
		TYPE_DEF=`expr "$1" : '-.\(.*\)'`
		shift
		;;
	--)
		shift
		break
		;;
	-h|-h*)
		cat <<HERE
$USAGE
d	force directory builds
h	print this help message
t type  default subdirectory for type templates
V       display our version information
files	new template files for mkcmd's library
HERE
		exit 0
		;;
	-V|-V*)
		echo '$Id: ubinst.sh,v 8.34 1999/11/26 20:33:12 ksb Exp $'
		exit 0
		;;
	-*)
		echo "$USAGE" 1>&2
		exit 1
		;;
	*)
		# process and continue for intermixed options & args
		break
		;;
	esac
done

dest=$1
shift

# get the files left on the line
if [ $# -eq 0 ]
then
	echo "$USAGE"
	exit 1
fi
while [ $# -gt 0 ]
do
	file=$1
	shift
	sub=`expr $file : '\([^_]*\)_.*'`
	if [ -z "$sub" -a -n "$TYPE_DEF" ] ;  then
		if expr $file : '.*\.[em][chi]*' >/dev/null ; then
			sub=$TYPE_DEF
		fi
	fi
	into=$dest/$sub/`expr $file : '[^_]*_\(.*\)'`
	if $DFLAG && [ -n "$sub" -a ! -d $dest/$sub ] ; then
		install -d $dest/$sub
	fi
	if [ -z "$sub" -o ! -d $dest/$sub ] ; then
		cmp -s $file $dest/$file || install -c -m 644 $file $dest/$file
		continue
	fi
	# remove the stray, if it exists
	[ -f $dest/$file ] && mv $dest/$file $into
	cmp -s $file $into || install -c $file $into
done

exit 0
