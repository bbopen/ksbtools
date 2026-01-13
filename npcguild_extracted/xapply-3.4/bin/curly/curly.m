%c
/*
 * curly -- expand {...} as csh(1)					(ksb)
 *
 * Copyright 1988, All Rights Reserved
 *	Kevin S Braunsdorf
 *	ksb@fedex.com
 *	Math/Sci Building, Purdue Univ
 *	West Lafayette, IN
 *
 *  `You may redistibute this code as long as you don't claim to have
 *   written it. -- ksb'
 *
 * We are limited to not eating backslash escapes because that would be
 * very confusing to the user.  If you need a file name {a}.c don't call
 * this routine.  Simple.  (If we did use \ as special, then \.c would
 * need to be quoted from us... it never ends, so we let the shells worry
 * about \ quoting for us.)
 *
 * We don't expand other globbing characters, because ksh will expand
 * those for us when it reads our output in `quotes`.
 *
 * The command
 *	$ curly c{1,2,3,4,5}.c
 * outputs
 *	c1.c c2.c c3.c c4.c c5.c
 *
 * So use you might use
 *	$ tar xv `curly c{1,2,3,4,5}.c`
 * to extract them from tape.
 *
 * If we are given no arguments we can read stdin for strings to glob.
 *
 * Improvments:
 *
 * This code could be mixed with other globbing code to fully emulate
 * csh(1) globbing in a few minutes,  I have no need of this (yet).
 *
 * We can avoid the malloc/strcpy/strcat in DoExpr if we build right
 * context right to left in a large buffer; this buffer could limit the
 * size of the glob expression, but other factors limit it already.
 *
 * $Compile: ${cc-cc} ${DEBUG--O} ${SYS--Dbsd} %f -o %F
 */
%%
from '<sys/param.h>'
from '<sys/types.h>'
from '<stdio.h>'
from '"machine.h"'

basename "curly" ""

%i
static char rcsid[] =
	"$Id: curly.m,v 3.3 1997/04/08 21:24:52 ksb Exp $";

#if !defined(MAXPATHLEN)
#define MAXPATHLEN	1024
#endif

static char acName[MAXPATHLEN+1];
extern void DoExpr(), DoList();

#define FIRST_GUESS	96	/* begin with MAXPATHLEN * this		*/
#define NEXT_GUESS	32	/* we hedge with MAXPATHLEN * this	*/
#define GRAB		16	/* size chunk to read (<= NEXT_GUESS)	*/

static char acNoMem[] = "%s: out of memory\n";
%%

%c
/* Here we call fgets() to read a glob expression to process.		(ksb)
 * Repeat until end of file.
 */
void
DoStdin(pcAccum)
register char *pcAccum;
{
	auto char acLine[MAXPATHLEN*GRAB];
	static char *pcLine = (char *)0;
	static unsigned uBufLen = 0;
	register unsigned uPos;
	register char *pcNewLine;

	acLine[MAXPATHLEN*GRAB-1] = '\000';
	if ((char *)0 == pcLine) {
		uBufLen = MAXPATHLEN*FIRST_GUESS;
		pcLine = malloc(uBufLen);
		if ((char *)0 == pcLine) {
			fprintf(stderr, acNoMem, progname);
			exit(1);
		}
	}
	uPos = 0;
	while (NULL != fgets(acLine, MAXPATHLEN*GRAB-1, stdin)) {
		pcNewLine = strrchr(acLine, '\n');
		if (0 == uPos && (char *)0 != pcNewLine) {
			*pcNewLine = '\000';
			DoExpr(pcAccum, acLine, "\n");
			continue;
		}
		if ((char *)0 != pcNewLine) {
			*pcNewLine = '\000';
		}
		if (uPos + MAXPATHLEN*GRAB-1 > uBufLen) {
			uBufLen += MAXPATHLEN*NEXT_GUESS;
			pcLine = realloc(pcLine, uBufLen);
		}
		strcpy(pcLine+uPos, acLine);
		if ((char *)0 == pcNewLine) {	/* we got chars, no end yet */
			uPos += MAXPATHLEN*GRAB-2;
			continue;
		}
		/* we have a line */
		DoExpr(pcAccum, pcLine, "\n");
		uPos = 0;
	}
}

/*
 * find a matching close char for the open we just ate, or (char *)0	(ksb)
 *	pc = FindMatch("test(a,b))+f(d)", '(', ')', 1);
 *			         ^ pc points here
 */
static char *
FindMatch(pcBuf, cOpen, cClose, iLevel)
char *pcBuf;
char cOpen, cClose;
int iLevel;
{
	while ('\000' != *pcBuf) {
		if (cClose == *pcBuf) {
			--iLevel;
		} else if (cOpen == *pcBuf) {
			++iLevel;
		}
		if (0 == iLevel)
			return pcBuf;
		++pcBuf;
	}
	return (char *)0;
}

/* if we can locate a curly expression in our expression if the form:	(ksb)
 *	 	left { list } right
 *	1) copy left side to pcAccum,
 *	2) add right to our right context (malloc a new buffer if needed)
 *	3) call DoList(pcAccum, list, right)
 * or if we find no such curly expression
 *	1) copy all nonspecial chars to pcAccum
 *	2) recurse with DoExpr(pcAccum, pcRight, "")
 */
void
DoExpr(pcAccum, pcExpr, pcRight)
register char *pcAccum;
char *pcExpr, *pcRight;
{
	register char *pcClose;
	register char *pcComma;
	register char *pcTemp;
	register unsigned int uLen;

	while ('{' != *pcExpr && '\000' != *pcExpr) {	/*}*/
		*pcAccum++ = *pcExpr++;
	}

	switch (*pcExpr) {
	case '\000':
		if (*pcRight == '\000') {	/* no right context	*/
			if (pcAccum != acName) {
				*pcAccum = '\000';
				fputs(acName, stdout);
			}
		} else {
			DoExpr(pcAccum, pcRight, "");
		}
		break;
	case '{':
		pcClose = FindMatch(pcExpr, '{', '}', 0);
		/*
		 * if an open is unbalanced we ignore it.
		 */
		if ((char *)0 == pcClose) {
			*pcAccum++ = *pcExpr++;
			DoExpr(pcAccum, pcExpr, pcRight);
			break;
		}
		*pcClose++ = '\000';
		pcComma = pcExpr+1;

		/*
		 * Now that the expr is cracked we can optimize if the
		 * additional right context is empty.  If it is not we
		 * have to compute a new right context.
		 */
		uLen = strlen(pcClose);
		if (0 == uLen) {
			DoList(pcAccum, pcComma, pcRight);
		} else {
			uLen += strlen(pcRight);
			pcTemp = malloc(uLen+1);
			(void) strcpy(pcTemp, pcClose);
			(void) strcat(pcTemp, pcRight);
			DoList(pcAccum, pcComma, pcTemp);
			free(pcTemp);
		}
		*--pcClose = '}';
		break;
	}
}

/*
 * do a comma separated list of terms with known right context		(ksb)
 *	1) loop through exprs at this level
 *	2) call DoExpr(pcAccum, SubExpr, Right)
 */
void
DoList(pcAccum, pcList, pcRight)
register char *pcAccum;
char *pcList, *pcRight;
{
	register char *pcThis;
	register int iLevel;

	iLevel = 0;

	for (pcThis = pcList; '\000' != *pcList; ++pcList) {
		switch (*pcList) {
		case '{':
			++iLevel;
			break;
		case '}':
			--iLevel;
			break;
		default:
			break;
		case ',':
			if (0 == iLevel) {
				*pcList = '\000';
				DoExpr(pcAccum, pcThis, pcRight);
				*pcList = ',';
				pcThis = pcList+1;
			}
			break;
		}
	}
	DoExpr(pcAccum, pcThis, pcRight);
}

/* this kludge keeps us more csh(1) compatible				(ksb)
 */
static void
DoParam(pcPat)
char *pcPat;
{
	if ('{' == pcPat[0] && '}' == pcPat[1] && '\000' == pcPat[2]) {
		fputs("{}\n", stdout);
		return;
	}
	DoExpr(acName, pcPat, "\n");
}
%%

zero {
	named "DoStdin"
	update "DoStdin(acName);"
}
every {
	named "DoParam"
	param "exprs"
	help "expression to expand"
}
