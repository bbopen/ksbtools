#!mkcmd
# $Id: seconds.m,v 8.20 1998/11/28 16:43:14 ksb Exp $
# accept time in seconds, years, minutes and the like			(ksb)

require "util_mult.m" "seconds.mi"

long type "seconds" base {
	type update "%n = %K<pm_cvt>/ef;"
	type dynamic
	type param "seconds"
	type keep
	key 1 init {
		"aPMSeconds"
	}
	help "a scaled time in seconds, use ? for help"
}
