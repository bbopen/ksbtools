#!mkcmd
# $Id: util_fsearch.m,v 8.11 1999/06/22 11:36:22 ksb Exp $
#
# search for a file as an environment variable, or element of a path	(ksb)
# for example items can be like
#	/usr/bin/vi	full path
#	vi		in a path
#	~joe/bin/axe	rooted a a users home
#	$VISUAL		one of the above indirect
# paths can be colon separated lists of
#	/usr/bin	full path
#	""		dot
#	~ksb/bin	rooted a user's home
# first one found wins, return the full path in malloc'd space;
# caller free's it (or not).
from '<sys/param.h>'
from '<sys/types.h>'
from '<sys/stat.h>'

require "util_home.m" "util_fsearch.mc" "util_fsearch.mi" "util_fsearch.mh"

key "util_fsearch" 1 init {
	"util_fsearch" "%N" "(char *)0"
}

key "util_fsearch_path" 1 init once {
	"/bin" "/usr/bin" "/usr/sbin"
}

key "util_fsearch_cvt" 1 init {
	"%K<util_fsearch_path>/i,:,1.2-$d"
}
