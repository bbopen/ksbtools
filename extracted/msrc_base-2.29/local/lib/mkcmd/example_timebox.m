#!mkcmd
# $Id: example_timebox.m,v 8.7 1997/10/11 18:29:07 ksb Beta $

require "std_help.m"

basename "timebox" ""

type "timebox" 't' {
	after 'printf("%%s:\\tfrom %%s ", %b, ctime(& %ZK1ev));printf("\\t  to %%s", ctime(& %ZK2ev));'
	help "example timebox"
}
