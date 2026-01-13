# $Id: std_fts.m,v 8.8 1998/09/25 14:46:57 ksb Exp $
#
# This is the std interface to the fts routine, stolen from the 4.4
# chmod documentation.
#
# use "%rRU" for a boolean to see is -R was presented at all
# use "%rRn" for the fts options requested
#
# Just augment R's key to call a routine to fts_open the params
# and scan them (see the default one provided):
#	augment boolean 'R' { key { "myscan" }}
#
# Or, augment list to take any other action you want:
#	augment boolean 'R' { augment list { update "something_else();" }}
#
# N.B. one cannot require "std_fts.m" and augment the option in the same
# file, you *must* put this file on the command line before the augment
# file.  Or a hack is to require both the "std_fts.m" and "my_fts.m" from
# an upper level file (since the requires are done in order). -- ksb
#
from "<sys/types.h>"
from "<sys/stat.h>"
from "<fts.h>"

require "util_errno.m"

# one might want to set FTS_NOCHDIR, FTS_NOSTAT in the 3 key value,
# or in a after trigger.  (See std_xdev.m for FTS_XDEV.)
key "fts_walk" 2 initialize {
	"u_fts_walk" "%a" "%rRn"
}

# the boolean below is really a (short int) for FTS options		(ksb)
boolean 'R' {
	track local named "fts_opts"
	update "%n = FTS_PHYSICAL;"
	help "operate recursively on file hierarchies"

	action 'H' {
		update "%rRn = FTS_PHYSICAL|FTS_COMFOLLOW;"
		help "follow command line symbolic links"
	}
	action 'L' {
		update "%rRn = FTS_LOGICAL;"
		help "follow all symbolic links"
	}
	action 'P' {
		update "%rRn = FTS_PHYSICAL;"
		help "follow no symbolic links"
	}
	list {
		update '%rRK<fts_walk>/ef;'
		param "files"
		help "file hierarchies to traverse"
	}
}
