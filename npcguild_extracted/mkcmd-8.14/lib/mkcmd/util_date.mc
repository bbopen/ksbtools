/* helper for date type.  Parse one time in YYYYMMDD[hh[mm[ss]]]	(bj)
 * plus takes "now" for the current time.
 * $Id: util_date.mc,v 8.8 1999/06/14 14:34:08 ksb Exp $
 */
static char *
%%K<DateParse>1v(pc, ptime)
char *pc;
time_t *ptime;
{
	auto int Y, M, D, h, m, s;
	auto struct tm tm;

	if ('n' == pc[0] && 'o' == pc[1] && 'w' == pc[2]) {
		(void)time(ptime);
		return pc+3;
	}

	if (3 != sscanf(pc, "%04d%02d%02d", &Y, &M, &D)) {
		return (char *)0;
	}
	pc += 8;	/* skip YYYYMMDD */

	/* don't check for \000, scanf does that.
	 */
	h = m = s = 0;
	if ('-' != *pc && 1 == sscanf(pc, "%02d", &h)) {
		pc += 2;
		if ('-' != *pc && 1 == sscanf(pc, "%02d", &m)) {
			pc += 2;
			/* accept '.ss' like date does */
			if ('.' == *pc)
				++pc;
			if ('-' != *pc && 1 == sscanf(pc, "%02d", &s)) {
				pc += 2;
			}
		}
	}

	tm.tm_year = Y - 1900;
	tm.tm_mon = M - 1;
	tm.tm_mday = D;
	tm.tm_hour = h;
	tm.tm_min = m;
	tm.tm_sec = s;
	tm.tm_isdst = -1;
	*ptime = mktime(& tm);

	return pc;
}

/* use the above to scan a date from the command line			(ksb)
 */
static long
%%K<DateCvt>1v(pcCvt, pcWho)
char *pcCvt, *pcWho;
{
	register char *pcTemp;
	auto long tThis;

	if ((char *)0 == pcCvt) {
		return 0L;
	}

	if ('?' == pcCvt[0] && '\000' == pcCvt[1]) {
%		printf("%%s: %%s: date format is \"YYYYMMDD[hh[mm[ss]]]\" or \"now\"\n", %b, pcWho)%;
		exit(0);
	}

%	pcTemp = %K<DateParse>1v(pcCvt, &tThis)%;
	if ((char *)0 == pcTemp) {
%		fprintf(stderr, "%%s: %%s: %%s: malformed date, use ? for help\n", %b, pcWho, pcCvt)%;
%		%K<DateExit>1ev
		/*NOTREACHED*/
		return 0L;
	}
	if ('\000' != *pcTemp) {
%		fprintf(stderr, "%%s: %%s: ...%%s: junk on the end of the date\n", %b, pcWho, pcTemp)%;
%		%K<DateExit>1ev
	}
	return tThis;
}
