#!mkcmd
# $Id: group.m,v 1.1 1998/09/20 18:40:04 ksb Exp $
# Accept a group or gid, look it up as in the group file and deep copy	(ksb)
# the it for later use.  If a (struct group) changes a lot we are going to
# loose big.
from '<sys/types.h>'
from '<sys/param.h>'
from '<ctype.h>'
from '<grp.h>'

require "util_savegrent.m"
require "group.mc"

pointer ["struct group"] type "group" {
	type update "if ((%XxB *)0 != %n) {(void)free((void *)%n);}if ((char *)0 == %N || '\\000' == %N[0]) {%n = (%XxB *)0;} else if ((%XxB *)0 == (%n = %K<group_cvt>1v(%N, \"%qL\"))) {exit(1);}"
	type dynamic
	type comment "%L specify a group by name or gid"
	type keep
	type help "locate %qp as a group"
	type param "group"
}

# the key that keeps us in sync with the client code -- ksb
key "group_cvt" 1 init {
	"group_cvt" "%N" '"%qL"'
}
