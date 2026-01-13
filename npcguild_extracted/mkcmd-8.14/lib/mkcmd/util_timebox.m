# $Id: util_timebox.m,v 8.7 1997/10/11 18:29:07 ksb Beta $
# Adapted from Ben Jackson <bjj%sequent.com@internet.fedex.com>
#
# Add time-ranges to a mkcmd						(bj)
#
# The range looks like [YYYYMMDD[hh[mm[ss]]]]-[YYYYMMDD[hh[mm[ss]]]]
#	"-"		all times (used in a before to set the default)
#	"19950330-"	Mar 30, 1995 and on
#	"-199509260923"	up until Sept 26, 1995 at 9:23am
#	"19740105-1995022308"	between Jan 5, 1974 and Feb 23, 1995 at 8am
from '<ctype.h>'

require "util_date.m"
require "util_timebox.mc" "util_timebox.mi"

# this key gets the names of the user buffers to define
key "RangeDecls" 1 init {
	'%ZK1ev' '%ZK2ev'
}
key "RangeCvt" 1 init {
	"RangeCvt" "%n" "& %ZK1ev" "& %ZK2ev" '"%qL"'
}
key "RangeInit" 1 init {
	"%K<RangeCvt>1v" "%i" "& %ZK1ev" "& %ZK2ev" '"initial %qL"'
}

# What to do on a conversion error for time ranges?
# One can reset the first value in this key to force another action on
# a bad Range conversion.  This is a bug in that all time conversions have
# to have the same fail behavior.  Well... don't tempt me.
key "RangeError" 1 init {
	"exit(1);"
}
