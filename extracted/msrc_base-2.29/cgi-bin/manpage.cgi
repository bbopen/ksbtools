#!/bin/ksh
# $Id: manpage.cgi,v 1.3 2008/05/12 19:56:19 ksb Exp $
#
# called for example: http://www.sac.fedex.com/cgi-bin/manpage.cgi?chown+2
# to get chown from man 2 rather than perhaps man 8, etc.
#
: ${MANPATH:=/usr/share/man:/usr/local/man:/usr/X11R6/man:/usr/man}
: ${manpath:=$MANPATH}
export manpath MANPATH PATH
set _ `echo ${QUERY_STRING} | tr '&' ' '`
shift
THEMANPAGE=${1:-intro}
THESECTION=$2
echo "Content-type: text/html"
echo "Pragma: no-cache"
echo ""
echo "<html><head><title>Manpage CGI | $THEMANPAGE</title>"
echo "<meta NAME="DOCUMENT-STATE" CONTENT="DYNAMIC" /></head>"
echo "<body><pre>"
/usr/bin/man -M ${manpath} ${THESECTION:+"$THESECTION"} "${THEMANPAGE}" 2>&1 |
/usr/bin/colcrt - 2>&1 |
sed -e '/\/\*[  ]*\(<[^>]*>\)[  ]*\*\//{
	s//\1/
	n
}' -e 's/&/\&amp;/g' -e 's/</\&lt;/g' -e 's/>/\&gt;/g' \
	-e 's@\([^	 ][^ 	]*\)(\([0-9][a-zA-Z]*\))@<A href="/cgi-bin/manpage.cgi?\1\&\2">&</A>@g'
echo "</pre><hr></body></html>"

exit 0
