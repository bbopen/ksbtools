#!mkcmd
# $Id: rc.m,v 8.3 2004/12/15 22:20:18 ksb Exp $
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
