#!mkcmd
# $Id: util_savegrent.m,v 8.11 1998/09/20 18:38:08 ksb Exp $
# Accept a group or gid, look it up as in the group file and deep copy	(ksb)
# the it for later use.  If a (struct group) changes a lot we are going to
# loose big.
from '<grp.h>'

require "util_savegrent.mh" "util_savegrent.mi" "util_savegrent.mc"

# the key that keeps us in sync with the client code -- ksb
key "util_savegrent" 1 init {
	"util_savegrent" "%n"
}

key "group_box" 2 init {
	"gr_gid"
}

key "group_strings" 2 init {
	"gr_name"
	"gr_passwd"
}

# we have to be able to declare a local named u_w_%a for each below -- ksb
key "group_argvs" 2 init {
	"gr_mem"
}
