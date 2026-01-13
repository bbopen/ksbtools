#!mkcmd
# $Id: time.m,v 8.4 1997/10/27 11:35:48 ksb Beta $
# The conversion you want "14 Aug 1981" -> time_t			(ksb)
# the converted time is put in %n_date by default, set the API
# key in the trigger to change the name... but we declare and define it
# for you. -- ksb

from '<time.h>'

require "util_time.m"
# require "time_tz.m" for better conversion (in local time)

key "TimeCvt" 1 init "Atotm" {
	1 "%N" 1
}

long type "time" base {
	type dynamic
	type update "%n = %K<TimeCvt>/ef;"
	type param "date"
	type help "provide a date in text format"
	type comment "%L converts from almost any western time format."
	key 2 init {
		"0"	# default time offest west in minutes
	}
}
