#!mkcmd
# $Id: time_tz.m,v 8.2 1997/10/12 01:09:35 ksb Beta $
# autoset Atotm's key for the local time zone				(ksb)
from '<sys/time.h>'

require "time.m"

# fix the call back in the time type, heheh
type "time" type "time_tz" {
	key 2 {
		"%K<AtotmTz>1ev"
	}
}

key "AtotmTz" 2 init {
	"Atz_West"
}

init 3 "%K<AtotmTz>1ev = AtotmAutoZone();"

# if someone over's our key this variable is dead, that's all.
%i
static int Atz_West;	/* minutes west for atotm code */
%%

%c
/* find the local time zone for atotm conversions			(ksb)
 */
static int
AtotmAutoZone()
{
	auto struct timeval tvThis;
	auto struct timezone tzThis;

	gettimeofday(& tvThis, & tzThis);
	return tzThis.tz_minuteswest;
}
%%
