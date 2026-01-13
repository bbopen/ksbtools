#!/bin/ksh
# $Id: mkowner.ksh,v 2.3 1998/04/07 13:00:19 ksb Exp $
# By Dan Lebovitz and Kevin Braunsdorf, Oct 1997
#
# This script reads the output of "ls -la" and produces a .owners on	(lebo)
# standard out.
#
# Syntax:
#	mkowner.ksh [owner] [files] > .owners
#
PROGNAME=`basename $0 .ksh`

# If the owner given as $1 is empty or not there use *.*, else		(ksb)
# make sure it is in the correct format. Convert Posix login:group
# to BSD login.group as a nice feature.
case $# in
0)
	OWNER='*.*' ;;
*)
	OWNER=$1 ; shift ;;
esac
if expr "${OWNER}" : '..*\...*' >/dev/null ; then
	:
elif expr "${OWNER}" : '..*:..*' >/dev/null ; then
	OWNER=`echo ${OWNER} | sed -e 's/:/./'`
elif grep "^${OWNER}:" </etc/passwd >/dev/null ; then
	OWNER="${OWNER}.*"
elif grep "^${OWNER}:" </etc/group >/dev/null ; then
	OWNER="*.${OWNER}"
else
	echo "$PROGNAME: $OWNER: bad format for installus (use login.group)" 1>&2
	exit 1
fi

ls -la ${1+"$@"} |
sed -e '/^[Tt]otal [0-9]*$/d' -e '/ \.\.$/d' |
cut -c1-10,15-33,54-80 |
awk '{printf "%-24s '"$OWNER"'\t-m %s -o %-8s -g %-8s\n", $4, $1, $2, $3;}'

exit
