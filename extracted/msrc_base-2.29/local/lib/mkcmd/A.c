/* $Id: A.c,v 8.8 2004/09/01 15:26:37 ksb Exp $
 * literal text included from a tempate
 * based on Keith Bostic's getopt in comp.sources.unix volume1
 * modified for mkcmd use.... by ksb@cc.purdue.edu (Kevin Braunsdorf)
 */

#if GETOPT || GETARG
/* IBMR2 (AIX in the real world) defines
 * optind and optarg in <stdlib.h> and confuses the hell out
 * of the C compiler.  So we use those externs.  I guess we will
 * have to stop using the old names. -- ksb
 */
#ifdef _AIX
#include <stdlib.h>
#else
static int
	optind = 1;		/* index into parent argv vector	*/
static char
	*optarg;		/* argument associated with option	*/
#endif
#endif /* only if we use them */

#if ENVOPT
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
static char *u_mynext(register char *u_pcScan, char *u_pcDest)
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
static int u_envopt(char *cmd, int *pargc, char *(**pargv))
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
#endif /* u_envopt called */

#if GETARG
/*
 * return each non-option argument one at a time, EOF for end of list
 */
static int u_getarg(int nargc, char **nargv)
{
	if (nargc <= optind) {
		optarg = (char *) 0;
		return EOF;
	}
	optarg = nargv[optind++];
	return 0;
}
#endif /* u_getarg called */


#if GETOPT
static int
	optopt;			/* character checked for validity	*/

/* get option letter from argument vector, also does -number correctly
 * for nice, xargs, and stuff (these extras by ksb)
 * does +arg if you give a last argument of "+", else give (char *)0
 * also does xargs -e if spec'd as "e::" -- not sure I like that at all,
 * but it is the most often asked for extension (rcs's -t option also).
 */
static int u_getopt(int nargc, char **nargv, char *ostr, char *estr)
{
	register char	*oli;		/* option letter list index	*/
	static char	EMSG[] = "";	/* just a null place		*/
	static char	*place = EMSG;	/* option letter processing	*/

	if ('\000' == *place) {		/* update scanning pointer */
		if (optind >= nargc)
			return EOF;
		if (nargv[optind][0] != '-') {
			register int iLen;
			if ((char *)0 != estr && 0 == strncmp(estr, nargv[optind], iLen = strlen(estr))) {
				optarg = nargv[optind++]+iLen;
				return '+';
			}
			return EOF;
		}
		place = nargv[optind];
		if ('\000' == *++place)	/* "-" (stdin)		*/
			return EOF;
		if (*place == '-' && '\000' == place[1]) {
			/* found "--"		*/
			++optind;
			return EOF;
		}
	}				/* option letter okay? */
	/* if we find the letter, (not a `:')
	 * or a digit to match a # in the list
	 */
	if ((optopt = *place++) == ':' ||
	 ((char *)0 == (oli = strchr(ostr,optopt)) &&
	  (!(isdigit(optopt)||'-'==optopt) || (char *)0 == (oli = strchr(ostr, '#'))))) {
		if('\000' == *place) ++optind;
		return '?';
	}
	if ('#' == *oli) {		/* accept as -digits, not "-" alone */
		optarg = place -1;
		while (isdigit(*place)) {
			++place;
		}
		if('\000' == *place) ++optind;
		return isdigit(place[-1]) ? '#' : '?';
	}
	if (':' != *++oli) {		/* don't need argument */
		optarg = (char *)0;
		if ('\000' == *place)
			++optind;
	} else {				/* need an argument */
		if ('\000' != *place || ':' == oli[1]) {
			optarg = place;
		} else if (nargc <= ++optind) {	/* no arg!! */
			place = EMSG;
			return '*';
		} else {
			optarg = nargv[optind];	/* skip white space */
		}
		place = EMSG;
		++optind;
	}
	return optopt;			/* dump back option letter */
}
#endif /* u_getopt called */
