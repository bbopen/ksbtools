/* $Id: util_timebox.mc,v 8.7 1997/10/11 18:29:07 ksb Beta $
 */

/* set a time range							(bj)
 */
static void
%%K<RangeCvt>1ev(pcCvt, plStart, plEnd, pcWho)
char *pcCvt, *pcWho;
time_t *plStart;
time_t *plEnd;
{
	register char *pcStart = pcCvt;
	register char *pcTemp;

	if ((char *)0 == pcCvt) {
		return;
	}
	if ('?' == *pcCvt) {
%		printf("%%s: %%s: time format is \"[date]-[date]\"\n", %b, pcWho)%;
%		(void)%K<DateCvt>1ev(pcCvt, pcWho)%;
		exit(0);
	}

	/* this could be controlled by a key, or the user might
	 * set them better.  Maybe an RangeInit key would be better?
	 */
	*plStart = 0L;
	*plEnd = 0x7fffffff;	/* XXX unsigned?  hmm */

	/* empty means all times
	 */
	if ('\000' == *pcCvt) {
		return;
	}
	if ('-' != *pcCvt) {
%		pcTemp = %K<DateParse>1ev(pcCvt, plStart)%;
		if ((char *)0 == pcTemp) {
%			fprintf(stderr, "%%s: %%s: start time `%%s' malformed\n", %b, pcWho, pcCvt)%;
%			%H/pcTemp/H/pcWho/K<RangeError>/e<cat>
			/*NOTREACHED*/
		}
		pcCvt = pcTemp;
	}
	while (isspace(*pcCvt)) {
		++pcCvt;
	}
	if ('-' != *pcCvt) {
		if ((char *)0 != (pcTemp = strchr(pcCvt, '-'))) {
			*pcTemp = '\000';
%			fprintf(stderr, "%%s: %%s: trailing junk `%%s' on start time\n", %b, pcWho, pcCvt)%;
		} else {
%			fprintf(stderr, "%%s: %%s: time `%%s' does not express a range\n", %b, pcWho, pcStart)%;
		}
%		%H/pcTemp/H/pcWho/K<RangeError>/e<cat>
		/*NOTREACHED*/
	}
	do {
		++pcCvt;
	} while (isspace(*pcCvt));
	/* let plain '-' express "all time" or trailing '-' indicate no
	 * ending time
	 */
	if ('\000' == *pcCvt) {
		return;
	}
%	pcTemp = %K<DateParse>1ev(pcCvt, plEnd)%;
	if ((char *)0 == pcTemp) {
%		fprintf(stderr, "%%s: %%s: end time `%%s' malformed\n", %b, pcWho, pcCvt)%;
%		%H/pcTemp/H/pcWho/K<RangeError>/e<cat>
		/*NOTREACHED*/
	}
	if ('\000' != *pcTemp) {
%		fprintf(stderr, "%%s: %%s: trailing junk `%%s' on time range\n", %b, pcWho, pcTemp)%;
%		%H/pcTemp/H/pcWho/K<RangeError>/e<cat>
		/*NOTREACHED*/
	}
}
