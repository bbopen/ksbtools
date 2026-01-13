#!mkcmd
# $Id: argv.m,v 8.11 2010/01/31 21:08:41 ksb Exp $
#
# A mkcmd type for a list of "words" on the command line.		(ksb)
# For example (given -a is this type, -P int)
#	./prog -a one -b -a two -P3 -a three
# then %ran is a PPM of (char *) with {"one" , "two", "three", (char *)0}
# When you provide a (non-NULL) init it is always the first word.

require "util_ppm.m" "argv.mc"

pointer ["struct PPMnode"] type "argv" "u_argv_init" {
	type update "u_argv_add(%n, %a);"
	type dynamic
	type comment "construct an argv from parameter words given"
	type help "assemble an argument vector from parameters"
	type param "words"
}

init 4 "u_argv_init();"
