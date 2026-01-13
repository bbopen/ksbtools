# $Id: date.m,v 8.8 1999/06/12 00:43:58 ksb Exp $
# demo of the date code							(ksb)

require "util_date.m"

long type "date" {
#	before "%K<DateInit>/ef;"
	type dynamic
	type param "when"
	type help "provide a date in digits"
	type update "%n = %K<DateCvt>/ef;"
	init '"now"'
}

key "DateInit" 1 init {
	"%K<DateCvt>1ev" '%i' '"initial %qL"'
}
