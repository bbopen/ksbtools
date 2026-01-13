#!/bin/sh
# $Id: pblock.sh,v 3.3 1997/04/13 18:38:07 ksb Dist $
#
# Output one of those bogus comment blocks people love			(ksb)
#  {of course the line numbers are going to be wrong after this is
#   inserted into the source file...}
# I'd never use one of these because calls builds a better (more up to date)
# one on the fly.  Some coding conventions require them -- I guess they don't
# know how to use a makefile.
#
PROGNAME=`basename $0 .sh`
PATH=$PATH:/usr/local/bin

if [ "$#" -eq 0 ]
then
	echo "$PROGNAME: usage 'functions' [-Ddef] [-Iincl] [-Udef] [files.c]" 1>&2
	exit 1
fi

# get functions and file names
funcs=$1
shift
if [ "$#" -eq 0 ]
then
	set _ *.c
	shift
fi

# produce comment blocks requested
for f in $funcs
do
	echo "/*"
	calls -vtrl1 -f $f "$@" | sed \
		-e '1d' \
		-e '2s/ *\[.*\]//' \
		-e s'/ see below$//' \
		-e s'/ *[0-9]*/ */'
	echo " */"
done

exit 0
