#!/bin/sh
# Install the given mkcmd template files				(ksb)
# Honor the existance of a directory to put new templates in, or
# take -d to force directory builds
# Be nice and use only Bourne Shell compatible commands.
#
PROGNAME=`basename $0 .sh`
USAGE="$PROGNAME: usage [-dI] [-t sub] directory templates
$PROGNAME: usage -h
$PROGNAME: usage -V"

# how to slide a single letter option off the beginning of a bundle
# -barf -> -arf
slide='P=$1; shift; set _ -`expr _"$P" : _'\''-.\(.*\)'\''` ${1+"$@"}; shift'
param='if [ $# -lt 2 ]; then echo "$PROGNAME: missing value for $1" 1>&2 ; exit 1; fi'

# default values for all the flags, or leave unset for a ${flag-value) form
DFLAG=false
TYPE_DEF=""
INPLACE=false

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
		TYPE_DEF=`expr _"$1" : _'-.\(.*\)'`
		shift
		;;
	-I)
		INPLACE=true
		shift
		;;
	-I*)
		INPLACE=true
		eval "$slide"
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
I	allow files with the same content to stay as-is (modes could be wrong)
t type  default subdirectory for type templates
V       display our version information
files	new template files for mkcmd's library
HERE
		exit 0
		;;
	-V|-V*)
		echo $PROGNAME: '$Id: ubinst.sh,v 8.39 2008/11/13 18:11:28 ksb Exp $'
		exit 0
		;;
	-*)
		echo "$USAGE" 1>&2
		exit 65
		;;
	--)
		shift
		break
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
	exit 65
fi

[ -d $dest ] || mkdir -p $dest
# For each file try to map it so mkcmd can make foo_bar.m into foo/bar.m
# that makes it easier to find stuff in the filesystem, but abstracts it
# in the requires code.  Note the default dir ("type" usually).
for file
do
	into=$dest
	name=`expr $file : '.*/\(.*\)' \| $file`
	sub=`expr $name : '\([^_]*\)_.*'`
	name=`expr \( $name : '[^_]*_\(.*\)' \) \| $name`
	if [ -n "$sub" ] ; then 
		sub="/$sub"
	elif [ -n "$TYPE_DEF" ] &&  expr $file : '.*\.[em][chi]*' >/dev/null ; then
		sub="/$TYPE_DEF"
	fi
	if [ -d "$into$sub" ] ; then
		into=$into$sub
	elif $DFLAG ; then
		install -d -m 755 $into$sub && into=$into$sub
	else
		name=$file
	fi
	# remove any older stray files, when they exist
	if [ _$dest/$file = _$into/$name -o ! -f $dest/$file ] ; then
		: 'no old file to remove'
	elif [ -f $into/$name ] ; then
		rm -f $dest/$file
	else
		mv $dest/$file $into/$name
	fi
	$INPLACE && cmp -s $file $into/$name && continue
	install -c -m 644 $file $into/$name
done

exit 0
