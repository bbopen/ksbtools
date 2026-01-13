# $Id: timebox.m,v 8.9 1999/06/14 13:30:11 ksb Exp $
require "util_timebox.m"

char* type "timebox" "timebox_clients" {
	type param "range"
	type help "provide a start and end time"
	type update "%K<RangeCvt>/ef;"
	type dynamic
	update "%XxSu"
	key 2 init {
		"%n_start" "%n_end"
	}
}
