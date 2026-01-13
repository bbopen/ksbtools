/* $Id: findfiles.c,v 3.9 2003/04/20 19:36:49 ksb Exp $
 * interact -- everything (almost) we need to do interactive
 *	commands for "unrm".  Uses Steve Uitti's libcmd.
 *
 *	Matthew Bradburn, PUCC UNIX Group	(mjb)
 *	Kevin Brausndorf, PUCC UNIX Group	(ksb)
 */
#include <stdio.h>
#include <sys/param.h>
#include <ctype.h>

#include "machine.h"
#include "libtomb.h"
#include "glob.h"
#include "main.h"
#include "lists.h"
#include "init.h"

#ifndef lint
static char rcsid[] =
	"$Id: findfiles.c,v 3.9 2003/04/20 19:36:49 ksb Exp $";
#endif

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif


/* the given string has sh wildcards? (or not.)				(mjb)
 */
static int		/* Boolean	*/
hasWild(pch)
register char *pch;
{
	while ('\000' != *pch) {
		if ('*' == *pch || '?' == *pch || ']' == *pch)
			return 1;
		++pch;
	}
	return 0;
}

/* CmpGlob -- compare the file from the list of entombed files to the	(ksb)
 * current glob expression.  Return 0, like strcmp, on match.
 */
static int
CmpGlob(pLE, pcGlob)
LE *pLE;
char *pcGlob;	/* pattern we match the file in pLE against.		*/
{
	if (! IsMarked(pLE) && SamePath(pcGlob, pLE->name)) {
		return 0;
	}
	return 1;
}

/* compare path names without globbing					(ksb)
 */
static int
Cmp(pLE, pcGlob)
LE *pLE;
char *pcGlob;
{
	if (! IsMarked(pLE)) {
		return strcmp(pcGlob, pLE->name);
	}
	return 1;
}


/* For each word on the command line, we expand that word	    (mjb/ksb)
 * against the unmarked entombed files.  If an unmarked file
 * matches, we mark it as used.  If no matches are found, we
 * check for matches with the exact word (no globbing).  If
 * there is not at least one file to match each word, an
 * error message is issued.
 */
int
FindFiles(pcIn, func)
char *pcIn;			/* the arguments to glob and apply	*/
int (*func)();			/* what to apply			*/
{
	register int iMatches;	/* num matches this word		*/
	register LE *pLE;	/* a matching file entry		*/
	register int fFirst;	/* take only the first match		*/

	/* if the caller has not opened the tombs yet do so now
	 */
	Init(DOTS_NO);

	/* We only use the first match if there were no wildcards in
	 * the word, else we do all of them.
	 */
	fFirst = ! hasWild(pcIn);
	iMatches = 0;
	for (pLE = Retrieve(pLEFilesList, CmpGlob, pcIn); nilLE != pLE; pLE = Retrieve(pLE->next, CmpGlob, pcIn)) {
		++iMatches;
		Mark(pLE);
		(*func)(pLE);
		if (fFirst) {
			break;
		}
	}

	/* if globbing didn't find any matches, we try without globbing
	 */
	if (0 == iMatches) {
		if (nilLE != (pLE = Retrieve(pLEFilesList, Cmp, pcIn))) {
			++iMatches;
			(*func)(pLE);
			Mark(pLE);
		}
	}
	if (0 == iMatches) {
		if ('*' == *pcIn && '\000' == pcIn[1])
			(void)fprintf(stdout, "%s: no files\n", progname);
		else
			(void)fprintf(stdout, "%s: %s: no matches\n", progname, pcIn);
		(void)fflush(stdout);
	}

	return iMatches;
}
