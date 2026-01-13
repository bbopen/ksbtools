#!mkcmd
# $Id: time_tz.m,v 8.6 2003/11/08 21:30:26 ksb Exp $
# autoset Atotm's key for the local time zone				(ksb)
from '<sys/time.h>'

require "time.m"

# fix the call back in the time type, heheh
type "time" type "time_tz" {
	key 2 {
		"%K<AtotmTz>1ev"
	}
}

key "AtotmTz" 1 {
	"Atz_West"
}

init 3 "%K<AtotmTz>1ev = AtotmAutoZone();"

# if someone over's our key this variable is dead, that's all.
integer named "Atz_West" {
	static
	help "minutes west for atotm code"
}

%hi
#if !defined(GETTIMEOFDAY_HAS_ZONE)
#define GETTIMEOFDAY_HAS_ZONE   (defined(FREEBSD)||defined(NETBSD)||defined(OPENBSD)||defined(IBMR2))
#endif

#if !defined(HAVE_ALTZONE)
#define HAVE_ALTZONE	(defined(SUN5))
#endif

#if !defined(EMU_TIME_T)
#if defined(IBMR2)
#define EMU_TIME_T	long
#else
#define EMU_TIME_T	time_t
#endif
#endif
%%
%c
/* find the local time zone for atotm conversions			(ksb)
 */
static int
AtotmAutoZone()
{
#if GETTIMEOFDAY_HAS_ZONE
	auto struct timeval tvThis;
	auto struct timezone tzThis;

	gettimeofday(& tvThis, & tzThis);
	return tzThis.tz_minuteswest;
#else
#if HAVE_ALTZONE
	extern EMU_TIME_T timezone;
	extern EMU_TIME_T altzone;
	extern int daylight;

	tzset();
	return (daylight ? altzone : timezone) / -60;
#else
	extern EMU_TIME_T timezone;

	tzset();
	return timezone / -60;
#endif
#endif
}
%%
