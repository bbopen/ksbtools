# $Id: util_getlogingr.m,v 1.1 1999/06/22 11:45:02 ksb Exp $
from '<sys/param.h>'
from '<sys/types.h>'
from '<grp.h>'

require "util_getlogingr.mi" "util_getlogingr.mc"

key "util_getlogingr" 1 init {
	"util_getlogingr" "%N" "& %n"
}
key "getlogingr" 1 init {
	"16" "112"
}
