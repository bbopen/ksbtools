/* $Id: getopt.mc,v 8.13 2004/09/01 15:26:37 ksb Exp $
 * Based on Keith Bostic's getopt in comp.sources.unix volume1
 * modified for mkcmd use by ksb@fedex.com (Kevin S Braunsdorf).
 */

%#if %K.1v || %K<getopt_shared>1v
/* breakargs - break a string into a string vector for execv.
 *
 * Note, when done with the vector, merely "free" the vector.
 * Written by Stephen Uitti, PUCC, Nov '85 for the new version
 * of "popen" - "nshpopen", that doesn't use a shell.
 * (used here for the as filters, a newer option).
 *
 * breakargs is copyright (C) Purdue University, 1985
 *
 * Permission is hereby given for its free reproduction and
 * modification for All purposes.
 * This notice and all embedded copyright notices be retained.
 */

/* this trys to emulate shell quoting, but I doubt it does a good job	(ksb)
 * [[ but not substitution -- that would be silly ]]
 */
%#if %K-1v
static char *u_mynext(register char *u_pcScan, char *u_pcDest)
#else
static char *
u_mynext(u_pcScan, u_pcDest)
register char *u_pcScan, *u_pcDest;
#endif
{
	register int u_fQuote;

	for (u_fQuote = 0; *u_pcScan != '\000' && (u_fQuote||(*u_pcScan != ' ' && *u_pcScan != '\t')); ++u_pcScan) {
		switch (u_fQuote) {
		default:
		case 0:
			if ('"' == *u_pcScan) {
				u_fQuote = 1;
				continue;
			} else if ('\'' == *u_pcScan) {
				u_fQuote = 2;
				continue;
			}
			break;
		case 1:
			if ('"' == *u_pcScan) {
				u_fQuote = 0;
				continue;
			}
			break;
		case 2:
			if ('\'' == *u_pcScan) {
				u_fQuote = 0;
				continue;
			}
			break;
		}
		if ((char*)0 != u_pcDest) {
			*u_pcDest++ = *u_pcScan;
		}
	}
	if ((char*)0 != u_pcDest) {
		*u_pcDest = '\000';
	}
	return u_pcScan;
}

/* given an envirionment variable insert it in the option list		(ksb)
 * (exploded with the above routine)
 */
%#if %K-1v
%%K<getopt_shared>2v int %K<envopt>1v(char *cmd, int *pargc, char *(**pargv))
#else
%%K<getopt_shared>2v int
%%K<envopt>1v(cmd, pargc, pargv)
char *cmd, *(**pargv);
int *pargc;
#endif
{
	register char *p;		/* tmp				*/
	register char **v;		/* vector of commands returned	*/
	register unsigned sum;		/* bytes for malloc		*/
	register int i, j;		/* number of args		*/
	register char *s;		/* save old position		*/

	while (*cmd == ' ' || *cmd == '\t')
		cmd++;
	p = cmd;			/* no leading spaces		*/
	i = 1 + *pargc;
	sum = sizeof(char *) * i;
	while (*p != '\000') {		/* space for argv[];		*/
		++i;
		s = p;
		p = u_mynext(p, (char *)0);
		sum += sizeof(char *) + 1 + (unsigned)(p - s);
		while (*p == ' ' || *p == '\t')
			p++;
	}
	++i;
	/* vector starts at v, copy of string follows nil pointer
	 * the extra 7 bytes on the end allow use to be alligned
	 */
	v = (char **)malloc(sum+sizeof(char *)+7);
	if ((char **)0 == v)
		return 0;
	p = (char *)v + i * sizeof(char *); /* after nil pointer */
	i = 0;				/* word count, vector index */
	v[i++] = (*pargv)[0];
	while (*cmd != '\000') {
		v[i++] = p;
		cmd = u_mynext(cmd, p);
		p += strlen(p)+1;
		while (*cmd == ' ' || *cmd == '\t')
			++cmd;
	}
	for (j = 1; j < *pargc; ++j)
		v[i++] = (*pargv)[j];
	v[i] = (char *)0;
	*pargv = v;
	*pargc = i;
	return i;
}
%#endif /* %K<envopt>1v called */

%#if %K.2v || %K<getopt_shared>1v
/*
 * return each non-option argument one at a time, EOF for end of list
 */
%#if %K-1v
%%K<getopt_shared>2v int %K<getarg>1v(int nargc, char **nargv)
#else
%%K<getopt_shared>2v int
%%K<getarg>1v(nargc, nargv)
int nargc;
char **nargv;
#endif
{
%	if (nargc <= %K<getopt_switches>1ev) %{
%		%K<getopt_switches>2ev = (char *)0%;
		return EOF;
	}
%	%K<getopt_switches>2ev = nargv[%K<getopt_switches>1ev++]%;
	return 0;
}
%#endif /* %K<getarg>1v called */


%#if %K.3v || %K<getopt_shared>1v
/* get option letter from argument vector, also does -number correctly
 * for nice, xargs, and stuff (these extras by ksb)
 * does +arg if you give a last argument of "+", else give (char *)0
 * alos does xargs -e if spec'd as "e::" -- not sure I like that at all,
 * but it is the most often asked for extension (rcs's -t option also).
 */
%#if %K-1v
%%K<getopt_shared>2v int %K<getopt>1v(int nargc, char **nargv, char *ostr, char *estr)
#else
%%K<getopt_shared>2v int
%%K<getopt>1v(nargc, nargv, ostr, estr)
int nargc;
char **nargv, *ostr, *estr;
#endif
{
	register char	*oli;		/* option letter list index	*/
	static char	EMSG[] = "";	/* just a null place		*/
	static char	*place = EMSG;	/* option letter processing	*/

	/* when we have no optlist, reset the option scan
	 */
	if ((char *)0 == ostr) {
%		%K<getopt_switches>1ev = 1%;
		place = EMSG;
		return EOF;
	}
	if ('\000' == *place) {		/* update scanning pointer */
%		if (%K<getopt_switches>1ev >= nargc)
			return EOF;
%		if (nargv[%K<getopt_switches>1ev][0] != '-') %{
			register int iLen;
%			if ((char *)0 != estr && 0 == strncmp(estr, nargv[%K<getopt_switches>1ev], iLen = strlen(estr))) %{
%				%K<getopt_switches>2ev = nargv[%K<getopt_switches>1ev++]+iLen%;
				return '+';
			}
			return EOF;
		}
%		place = nargv[%K<getopt_switches>1ev]%;
		if ('\000' == *++place)	/* "-" (stdin)		*/
			return EOF;
		if (*place == '-' && '\000' == place[1]) {
			/* found "--"		*/
%			++%K<getopt_switches>1ev%;
			return EOF;
		}
	}				/* option letter okay? */
	/* if we find the letter, (not a `:')
	 * or a digit to match a # in the list
	 */
%	if ((%K<getopt_switches>3ev = *place++) == ':' ||
%	 ((char *)0 == (oli = strchr(ostr, %K<getopt_switches>3ev)) &&
%	  (!(isdigit(%K<getopt_switches>3ev)|| '-' == %K<getopt_switches>3ev) || (char *)0 == (oli = strchr(ostr, '#'))))) %{
%		if(!*place) ++%K<getopt_switches>1ev%;
		return '?';
	}
	if ('#' == *oli) {		/* accept as -digits, not "-" alone */
%		%K<getopt_switches>2ev = place-1%;
		while (isdigit(*place)) {
			++place;
		}
%		if(!*place) ++%K<getopt_switches>1ev%;
		return isdigit(place[-1]) ? '#' : '?';
	}
	if (':' != *++oli) {		/* don't need argument */
%		%K<getopt_switches>2ev = (char *)0%;
		if ('\000' == *place)
%			++%K<getopt_switches>1ev%;
	} else {				/* need an argument */
		if ('\000' != *place || ':' == oli[1]) {
%			%K<getopt_switches>2ev = place%;
%		%} else if (nargc <= ++%K<getopt_switches>1ev) %{	/* no arg!! */
			place = EMSG;
			return '*';
		} else {
%			%K<getopt_switches>2ev = nargv[%K<getopt_switches>1ev]%;	/* skip white space */
		}
		place = EMSG;
%		++%K<getopt_switches>1ev%;
	}
%	return %K<getopt_switches>3ev%;			/* dump back option letter */
}
%#endif /* %K<getopt>1v called */
