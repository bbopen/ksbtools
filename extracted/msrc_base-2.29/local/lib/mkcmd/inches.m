#!mkcmd
# $Id: inches.m,v 8.20 1998/11/29 01:17:36 ksb Exp $
# accept length in inches, feet, and the like				(ksb)

require "util_mult.m" "inches.mi"

long type "inches" base {
	type update "%n = %K<pm_cvt>/ef;"
	type dynamic
	type param "inches"
	type keep
	key 1 init {
		"aPMInches"
	}
	help "a scaled length in inches, use ? for help"
}
