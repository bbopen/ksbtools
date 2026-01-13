#!mkcmd
# $Id: example_mult.m,v 8.8 1997/10/14 01:16:18 ksb Beta $
#
# An example of scaled input units using "util_mult" types		(ksb)
#
# we don't require "inches.m" "bytes.m" or "seconds.m" because the
# type construct below does that for us.

require "std_help.m"

basename "mult" ""

type "bytes" 'b' {
	named "iBytes" "pcBytes"
	init '"1k"'
	after 'printf("%%s: %%ld bytes\\n", %b, %n);'
}
type "inches" 'i' {
	named "iInches" "pcInches"
	init '"1f"'
	after 'printf("%%s: %%ld inches\\n", %b, %n);'
}
type "seconds" 's' {
	named "iTimer" "pcTimer"
	init '"3h"'
	after 'printf("%%s: %%ld seconds\\n", %b, %n);'
}
