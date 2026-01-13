#!mkcmd
# $Id: rc.m,v 1.1 1997/10/06 19:35:59 ksb Beta $
# convert the util_rc.m stuff into a new mkcmd type

require "util_rc.m"

file["r"] type "rc" {
	late
	update "%n = %ZK/ef;"
	key 2 init "RcOpen" {
		3
	}
	before "%K<RcInit>/ef;"
	help "open a run commands file"
}
