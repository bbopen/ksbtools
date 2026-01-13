/* match a regular expression to a string with the native R.E. engine
 * $Id: match.c,v 5.3 2009/01/10 18:41:48 ksb Exp $
 *
 * This is not in the least thread safe.  Don't even go there.
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#include "machine.h"
#include "match.h"

extern char *progname;

#if REGEX || HP_REGCOMP
#include <regex.h>
#endif

#if REGCOMP
#include <regexp.h>
#endif

#if REGCMP
#include <stdlib.h>
#endif

#if USE_LIBGEN
#include <libgen.h>
#endif

#if REGEX
static regex_t aRECPat[1];
static regmatch_t aRM[10];
#else
#if RECOMP
extern char *re_comp();
#else
#if REGCMP
/* extern char *regcmp(); from in libgen.h */
static char *pcPattern = (char *)0;
#else
#if REGCOMP
static regex *pRECPat;
#else
static char *last_pattern = (char *)0;
#endif
#endif
#endif
#endif

#if REGEX
size_t
regerror(int code, const regex_t *preg, char *errbuf, size_t errbuf_size)
{
	fprintf(stderr, "%s: %s\n", progname, errbuf);
}
#endif

#if REGCOMP
static int
regerror(char *s)
{
	fprintf(stderr, "%s: %s\n", progname, s);
}
#endif

/* Return the match for any parenthesis in the last RE match		(ksb)
 * Expr is the string matched, Count is the \0..\9 as an interger (0..9),
 * &pcEnd (when given as not (char **)0) is 1 past the string.
 * We return (char *)0 for "no such match" else the address of the
 * first matching char.
 */
char *
ReParen(pcExpr, iCount, ppcStop)
char *pcExpr, **ppcStop;
int iCount;
{
	register char *pcRet, *pcEos;
	register int iLen;

	if (iCount < 0 || iCount > 9) {
		errno = EINVAL;
		return (char *)0;
	}
	pcRet = (char *)0;
	pcEos = (char *)0;
	iLen = 0;
	/* recover the match space 0 ...9
	 */
#if REGEX
	aRM[10];
	pcRet = -1 == aRM[iCount].rm_so  ? (char *)0 : pcExpr + aRM[iCount].rm_so;
	if ((char *)0 != pcRet) {
		pcEos = pcExpr + aRM[iCount].rm_eo;
	}
#else
#if RECOMP
	/* does it internally */
#else
#if REGCMP
	/* does it internally */
#else
#if REGCOMP
	/* does it internally */
#else
	if (0 == iCount && (char *)0 != (pcRet = last_pattern)) {
		pcEos = last_patern + strlen(pcRet);
	} else
#endif
#endif
#endif
	/* \0 .. \9 not supported? */
	{
		static int fTold = 0;
		if (!fTold) {
			fTold = 1;
			fprintf(stderr, "%s: re_comp: doesn't support substring expansion\n", progname);
		}
	}
#endif
	if ((char **)0 != ppcStop) {
		*ppcStop = pcEos;
	}
	return pcRet;
}

/* return 1 if pcStr ~= m/pcPat/				(ksb/aab)
 */
int
ReMatch(pcStr, pcPat)
char *pcStr, *pcPat;
{
	register int fMatch;
#if RECOMP
	register char *errmsg;
#endif

	/* compile the RE, and save it for next call (if needed)
	 */
#if REGEX
	if (0 != regcomp(aRECPat, pcPat, REG_EXTENDED|REG_ICASE)) {
		/* see regerror() above */
		return 0;
	}
#else
#if RECOMP
	/* (re_comp handles a null pattern internally,
	 *  so there is no need to check for a null pattern here.)
	 */
	if ((char *)0 != (errmsg = re_comp(pcPat))) {
		fprintf(stderr, "%s: re_comp: %s\n", progname, errmsg);
		return 0;
	}
#else
#if REGCMP
	if ((char *)0 == pcPat || *pcPat == '\000') {
		/* A null pattern means use the previous pattern.
		 * The compiled previous pattern is in pcPattern, so just use it.
		 */
		if ((char *)0 == pcPattern) {
			fprintf(stderr, "%s: regcmp: No previous regular expression\n", progname);
			return 0;
		}
	} else {
		/* Otherwise compile the given pattern.
		 */
		register char *s;
		if ((char *)0 == (s = regcmp(pcPat, 0))) {
			fprintf(stderr, "%s: regcmp: Invalid pattern\n", progname);
			return 0;
		}
		if ((char *)0 != pcPattern) {
			free(pcPattern);
		}
		pcPattern = s;
	}
#else
#if REGCOMP
	if (0 != (pRECPat = regcomp(pcPat)) {
		/* see regerror() above */
		return 0;
	}
#else
	if ((char *)0 == pcPat || *pcPat == '\000') {
		/* Null pattern means use the previous pattern.
		 */
		if ((char *)0 == last_pattern) {
			fprintf(stderr, "%s: strcmp: No previous regular expression\n", progname);
			return 0;
		}
		pcPat = last_pattern;
	} else {
		if ((char *)0 != last_pattern)
			free((void *)last_pattern);
		last_pattern = strdup(pcPat);;
	}
#endif
#endif
#endif
#endif
	/* Check the string, if we can't find an RE lib use strcmp at least
	 */
#if REGEX
	fMatch = (0 == regexec(aRECPat, pcStr, sizeof(aRM)/sizeof(aRM[0]), aRM, 0));
#else
#if REGCMP
	fMatch = ((char *)0 != regex(pcPattern, pcStr));
#else
#if RECOMP
	fMatch = (1 == re_exec(pcStr));
#else
#if REGCOMP
	fMatch = (1 == regexec(pRECPat, pcStr));
#else
	fMatch = 0 == strcmp(pcPat, pcStr);
#endif
#endif
#endif
#endif
	return fMatch;
}
