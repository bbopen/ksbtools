#!mkcmd
# $Id: example_time.m,v 8.7 1997/10/11 18:57:53 ksb Beta $
# example of text time --ksb

require "std_help.m"

basename "birthday" ""

type "time" 'b' {
	named "lBorn" "pcBirth"
	param "born"
	help "an English time converted to std format"
	after 'printf("%%s: born %%s", %b, ctime(& %n));'
}
