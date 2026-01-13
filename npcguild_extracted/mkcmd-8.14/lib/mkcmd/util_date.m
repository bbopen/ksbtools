# $Id: util_date.m,v 8.7 1997/10/11 18:29:07 ksb Beta $
# Adapted from Ben Jackson <bjj%sequent.com@internet.fedex.com>
#
# Add dates and							(bj)
#
# The date looks like [YYYYMMDD[hh[mm[ss]]]]
#	YYYYMMDD[hh[mm[ss]]]
#	now
# or	?				(for help)
from '<time.h>'

require "util_date.mc" "util_date.mi"

# Change this key to change the name of the date parser.
key "DateParse" 2 init {
	"DateParse" "%N" '& some_time_t'
}
key "DateCvt" 2 init {
	"DateCvt" "%N" '"%qL"'
}

# change this key to change the abort code in DateParse
key "DateExit" 1 init {
	"exit(1);"
}
