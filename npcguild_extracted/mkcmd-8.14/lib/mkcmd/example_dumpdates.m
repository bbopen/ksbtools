#!mkcmd
# $Id: example_dumpdates.m,v 8.7 1999/06/12 00:43:58 ksb Exp $
# build with [and w/o] "time_tz.m" and (maybe) "util_fgetln.m" to see
# that atotm works best if you set the timezone stuff. -- ksb
#
from '<sys/types.h>'

basename "dumpdates" ""

# lines in /etc/dumpdates look like:
#	/dev/rdsk/c201d5s0 0 Fri Mar 24 09:54:08 1995
char* named "pcDev" {
	param "device"
	help "path to raw disk device"
}
integer named "iLevel" "pcLevel" {
	param "level"
	help "level of dump recorded"
}
type "time" named "tWhen" "pcWhen" {
	help "when the dump was started"
}
file["r"] name "fpDDates" "pcDates" {
	init '"/etc/dumpdates"'
	routine "ReadDDates" {
		list "pcDev" "iLevel" "tWhen"
		update 'printf("level: %%s %%d %%ld\\n", pcDev, iLevel, tWhen);'
	}
	user "%mI(%n);"
}
cleanup "ReadDDates(fpDDates);"
