`#!mkcmd
# $Id: hosttype.m,v 1.2 1996/12/26 23:33:39 kb207252 Beta $
# the interface to meta source 1997 (ksb)
#
# This maps the meta source idea of hosttype into the mkcmd key space so we
# can stop building all those machine.h files by hand -- ksb
# Under the 1997 version of meta source main.h _is_ machine.h (and main.c
# contains machine.c as well).
#
# N.B. we reserve the "L7_" name space for keys in the l7_ files.

require "hosttype.mh"

key "HostType" 1997 init {
	"'HOSTTYPE`"
}

# the groups of hosts that look pretty close for seven point leverage
key "L7_HPUX_TYPES" 1 init {
	"HPUX7" "HPUX8" "HPUX9" "HPUX10"
}
key "L7_IRIX_TYPES" 1 init {
	"IRIX4" "IRIX5" "IRIX6"
}
key "L7_SUN_TYPES" 1 init {
	"SUN3" "SUN4" "SUN5"
}

'dnl
