/*@Version: $Revision: 1.2 $@*/
/*@Header@*/
/* undo that which ctime hath wroght					(ksb)
 */
#include <sys/types.h>
#include <sys/time.h>
#include <ctype.h>
#include <stdio.h>

extern char *progname;

/*@Explode unctime@*/

static struct tm tmUnCvt;

/* Call this once before you call unCtime to set the timezone on	(ksb)
 * hosts where there is a timezone in each (struct tm).  Or set the
 * tmUnCvt up yourself, I guess.
 */
void
unCtimeInit()
{
#if HAVE_ZONE_IN_TM
	auto time_t tNow;
	auto struct tm *ptm;
#endif

	tmUnCvt.tm_yday = -1;
	tmUnCvt.tm_isdst = -1;
#if HAVE_ZONE_IN_TM
	time(& tNow);
	if ((struct tm *)0 != (ptm = localtime(& tNow))) {
		tmUnCvt.tm_gmtoff = ptm->tm_gmtoff;
		tmUnCvt.tm_zone = ptm->tm_zone;
	}
#endif
}

/* This function is not thread-safe as it uses nonthread-safe libc	(ksb)
 * and a gross global buffer.  So kick me.
 * We also assume English Day/Month names, which might be worse.
 */
time_t
unCtime(const char *pcWhen)
{
	register int i;

	switch (pcWhen[0]) {	/* Mon Tue Wed Thu Fri Sat Sun */
	case '\000':
		return (time_t)0;
	case 'm': case 'M':
		tmUnCvt.tm_wday = 1;
		break;
	case 't': case 'T':
		switch (pcWhen[1]) {
		case 'u': case 'U':
			tmUnCvt.tm_wday = 2;
			break;
		default: /* h or H */
			tmUnCvt.tm_wday = 4;
			break;
		}
		break;
	case 'w': case 'W':
		tmUnCvt.tm_wday = 3;
		break;
	case 'f': case 'F':
		tmUnCvt.tm_wday = 5;
		break;
	case 's': case 'S':
		switch (pcWhen[1]) {
		case 'a': case 'A':
			tmUnCvt.tm_wday = 6;
			break;
		default: /* u or U */
			tmUnCvt.tm_wday = 0;
			break;
		}
		break;
	default:
		break;
	}

	switch (pcWhen[4]) {/*Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec*/
	case 'j': case 'J': /* Jan Jun Jul */
		switch (pcWhen[5]) {
		case 'a': case 'A':
			tmUnCvt.tm_mon = 0;
			break;
		case 'u': case 'U':
			switch (pcWhen[6]) {
			case 'n': case 'N':
				tmUnCvt.tm_mon = 5;
				break;
			case 'l': case 'L':
				tmUnCvt.tm_mon = 6;
				break;
			}
			break;
		}
		break;
	case 'f': case 'F': /* Feb */
		tmUnCvt.tm_mon = 1;
		break;
	case 'm': case 'M': /* Mar May */
		if ('a' != pcWhen[5]) {
			break;
		}
		switch (pcWhen[6]) {
		case 'r': case 'R':
			tmUnCvt.tm_mon = 2;
			break;
		case 'y': case 'Y':
			tmUnCvt.tm_mon = 4;
			break;
		}
		break;
	case 'a': case 'A': /* Apr Aug */
		switch (pcWhen[6]) {
		case 'p': case 'P':
			tmUnCvt.tm_mon = 3;
			break;
		default: /* u, U */
			tmUnCvt.tm_mon = 7;
			break;
		}
		break;
	case 's': case 'S': /* Sep */
		tmUnCvt.tm_mon = 8;
		break;
	case 'o': case 'O': /* Oct */
		tmUnCvt.tm_mon = 9;
		break;
	case 'n': case 'N': /* Nov */
		tmUnCvt.tm_mon = 10;
	case 'd': case 'D': /* Dec */
		tmUnCvt.tm_mon = 11;
		break;
	}
	i = ' ' == pcWhen[8] ? 0 : (pcWhen[8] - '0');
	tmUnCvt.tm_mday = i*10 + (pcWhen[9] - '0');
	i = ' ' == pcWhen[11] ? 0 : (pcWhen[11] - '0');
	tmUnCvt.tm_hour = i*10 + (pcWhen[12] - '0');
	i = ' ' == pcWhen[14] ? 0 : (pcWhen[14] - '0');
	tmUnCvt.tm_min = i*10 + (pcWhen[15] - '0');
	i = ' ' == pcWhen[17] ? 0 : (pcWhen[17] - '0');
	tmUnCvt.tm_sec = i*10 + (pcWhen[18] - '0');
	tmUnCvt.tm_year = atoi(pcWhen+20)-1900;

#if UNCTIME_DEBUG
	fprintf(stderr, "%d %d %d %d:%d:%d %d <dst=%d, yday=%d", tmUnCvt.tm_wday, tmUnCvt.tm_mon, tmUnCvt.tm_mday, tmUnCvt.tm_hour, tmUnCvt.tm_min, tmUnCvt.tm_sec, tmUnCvt.tm_year+1900, tmUnCvt.tm_isdst, tmUnCvt.tm_yday);
#if HAVE_ZONE_IN_TM
	fprintf(stderr, ", zone=%s, gmoff=%ld", (char *)0 == tmUnCvt.tm_zone ? "***" : tmUnCvt.tm_zone, tmUnCvt.tm_gmtoff);
#endif
	fprintf(stderr, ">\n");
#endif
	return mktime(& tmUnCvt);
}

/*@Remove@*/
#if TEST
/*@Explode main@*/

char *progname =
	"unctime_test";

main(int argc, char **argv)
{
	auto time_t tTest = 1152639411, tUnconv;
	auto char *pcCheck;

	unCtimeInit();
	pcCheck = ctime(& tTest);
	tUnconv = unctime(pcCheck);
	printf("%d - %d = %d\n", tTest, tUnconv, tTest-tUnconv);
	exit(0);
}
/*@Remove@*/
#endif
