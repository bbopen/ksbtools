# $Id: util_home.m,v 8.8 1998/09/16 13:34:24 ksb Exp $
# Find the home directory of a login (used by util_rc)			(ksb)
#
from '<sys/types.h>'
from '<pwd.h>'

require "util_home.mh" "util_home.mi" "util_home.mc"

# if you bind me to a string the update could be
#	update "%K<util_home>/ef;"
key "util_home" 2 init {
	"util_home" "%n" "%N"
}
