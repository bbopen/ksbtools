# $Id: example_date.m,v 8.7 1997/10/11 18:29:07 ksb Beta $
# demo of the date code							(ksb)

require "std_help.m"

basename "birthday" ""

type "date" 'b' {
	named "lBorn" "pcBirth"
	param "born"
	help "a date converted to std format"
	after 'printf("%%s: born %%s", %b, ctime(& %n));'
}
