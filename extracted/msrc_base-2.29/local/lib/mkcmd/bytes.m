#!mkcmd
# $Id: bytes.m,v 8.20 1998/11/29 01:17:36 ksb Exp $
# accept size in bytes, kilobytes and the like				(ksb)

require "util_mult.m" "bytes.mi"

long type "bytes" base {
	type update "%n = %K<pm_cvt>/ef;"
	type dynamic
	type param "bytes"
	type keep
	key 1 init {
		"aPMBytes"
	}
	help "a scaled size in bytes, use ? for help"
}
