/* $Id: mk.c,v 5.23 2009/11/18 16:10:52 ksb Exp $, use that code
 * mk: detect and execute a compilation command
 *	(formerly called 'compile')
 *
 * example marker line:
 *	$Compile: cc -c -O %f
 *	$Compile (TEKECS): cc -c -DLOG -O %f
 *	$Compile (DEBUG): cc -c -g -DDEBUG %f
 *	$Compile=1: cc ....
 *	$Compile,cpu=100: ....
 *
 * marker lines can also be drawn from a standard template
 *
 * This program searches for the first occurence of a marker (DEFLTMARK)
 * in the first block of the named file(s), grabs the line on which
 * the marker occurs, performs some filename substitutions on the line,
 * and prints the line (typically a shell command line) on the stdout.
 * It can also set resource limits for valid(1L).
 *
 * this programs currently makes lots of substitutions (see the man page).
 *
 * command-line switches:
 *	see mk.m or main.c (or run mk -h)
 *
 * (c) Copyright 1983, Steven McGeady.  All rights reserved.
 *
 * Written by S. McGeady, Intel, Inc., mcg@mipon2.intel.com		(sm)
 *	      Kevin S Braunsdorf, ksb@cc.purdue.edu, purdue!ksb		(ksb)
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company or the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 *
 * 1. The authors are not held responsible for any consequences of the
 *    use of this software.
 *
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Credit to the authors must appear
 *    in documentation and sources.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 4. This notice may not be removed or altered.
 *
 * All bug fixes and improvements should be mailed to the authors,
 * if you can find them.
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sysexits.h>
#include <unistd.h>

#include "machine.h"
#include "main.h"
#include "mk.h"
#include "rlimsys.h"
#include "dicer.h"

#if !USE_STDLIB
extern char *getenv(), *malloc();
#else
#include <stdlib.h>
#endif

#if !defined(MAXPATHLEN)
#define MAXPATHLEN	1024
#endif

#if RESOURCE
#include <sys/resource.h>
#include <sys/wait.h>
#endif

#if defined(S_IFLNK)
#define LSTAT lstat
#else
#define LSTAT stat
#endif

#if !defined(EPIX)
#include <sys/time.h>
#endif

extern char *rcsname();

#define	MAXLINE		BUFSIZ
#define EXIT_EXACT	0x00	/* an exact match			*/
#define EXIT_ANY	0x01	/* any value				*/
#define EXIT_NOT	0x02	/* not what was given			*/
typedef struct ECnode {
	int ekey;		/* key for a match			*/
	int ivalue;		/* value to match			*/
} CODE;
#define MK_IGNORE_L	-9
extern char *findmarker(FILE *fin, char buf[MAXLINE], CODE *pexitcode, int count, int (pfiWatch)(FILE *, char *, void *), void *pvData);

char acOutMem[] = "%s: out of memory!\n";
static char *pcMark = (char *)0,
	*pcSub = (char *)0,
	*pcActual = (char *)0,
	*pcPipeLine = (char *)0;
static int marklen, sublen;
static char *curfile, chFType;
static char acSearch[MAXPATHLEN+1];


#if defined(BEFORE)
static char acDefBefore[] = BEFORE;
#endif	/* default pre-scan	*/

#if defined(TEMPL)
static char acDefAfter[] = TEMPL;
#endif	/* default post-scan	*/

/* option a good version description					(ksb)
 */
void
Version()
{
	printf("%s: %s\n", progname, "$Id: mk.c,v 5.23 2009/11/18 16:10:52 ksb Exp $");
	printf("%s:  %s\n", progname, DicerVersion);
	printf("%s: %%~ `%s\'\n", progname, pcTildeDir);
	printf("%s: -e `%s\'\n", progname,
		(char *)0 != pchExamine ? pchExamine :

#if defined(BEFORE)
		acDefBefore
#else
		""
#endif	/* default pre-scan	*/
	);
	printf("%s: -t `%s\'\n", progname,
		(char *)0 != pchTemplates ? pchTemplates :
#if defined(TEMPL)
		acDefAfter
#else
		""
#endif	/* default post-scan	*/
	);
}

/* Make me a tmp file I can unlink later				(ksb)
 */
static char *
FindSpace(char *pcTail, char *pcSearch)
{
	register char *pcColon;
	register int iLen, iOpen;
	auto char acTry[MAXPATHLEN+4];

	if ((char *)0 == pcTail || '\000' == *pcTail) {
		pcTail = "/mkXXXXXX";
	}
	if ((char *)0 == pcSearch) {
		pcSearch = "/tmp:/var/tmp";
	}
	for (/* param */; (char *)0 != pcSearch; pcSearch = pcColon) {
		if ((char *)0 == (pcColon = strchr(pcSearch, ':'))) {
			(void)strncpy(acTry, pcSearch, sizeof(acTry));
			if (0 == (iLen = strlen(acTry)))
				acTry[iLen++] = '.';
		} else {
			iLen = pcColon-pcSearch;
			if (iLen > sizeof(acTry)-2)
				continue;
			++pcColon;
			(void)strncpy(acTry, pcSearch, iLen);
		}
		if ('/' != *pcTail) {
			acTry[iLen++] = '/';
		}
		strncpy(acTry+iLen, pcTail, sizeof(acTry)-iLen);
		if (-1 != (iOpen = mkstemp(acTry))) {
			close(iOpen);
			return strdup(acTry);
		}
	}
	return (char *)0;
}

/* This has to be global so recursive translits can see it :-(
 */
static char **ppcHold = (char **)0;

/* We collect all the here document temp file names, then rm them	(ksb)
 * when we are done.  We know there are usually less than 8 %J's in a line.
 * N.B. They are all malloc'd strings.
 */
char **
AddClean(char *pcKeep)
{
	static int iMax = 0, iCur = 0;
	register int i;

	if ((char *)0 == pcKeep) {
		for (i = 0; i < iCur; ++i) {
			unlink(ppcHold[i]);
			free((void *) ppcHold[i]);
			ppcHold[i] = (char *)0;
		}
		iCur = 0;
		return (char **)0;
	}
	if (0 == iMax) {
		iMax = 8;
		ppcHold = (char **)calloc(iMax, sizeof(char *));
	}
	if (iCur+1 == iMax) {
		iMax += 8;
		ppcHold = (char **)realloc((void *)ppcHold, sizeof(char *)*iMax);
	}
	if ((char **)0 == ppcHold) {
		return (char **)0;
	}
	ppcHold[iCur++] = pcKeep;
	ppcHold[iCur] = (char *)0;
	return ppcHold;
}

/* strip the case from an ident upto len or a char in pchStop		(ksb)
 */
void
stripc(pchIdent, len, pchStop)
char *pchIdent, *pchStop;
int len;
{
	register int i;

	for (i = 0; i < len; ++i, ++pchIdent) {
		if ((char *)0 != strchr(pchStop, *pchIdent))
			break;
		if (isupper(*pchIdent))
			*pchIdent = tolower(*pchIdent);
	}
}

/* set the marker string for this file					(ksb)
 * strip off the leaning space, copy to safe storage and
 * (maybe) strip case.
 */
void
setmark(pcIn)
char *pcIn;
{
	static char acDefMark[] = DEFMARK;

	while ((char *)0 != pcIn && isspace(*pcIn)) {
		++pcIn;
	}
	if ((char *)0 == pcIn || '\000' == *pcIn) {
		pcIn = acDefMark;
	}

	marklen = strlen(pcIn);
	if ((char *)0 != pcMark)
		free(pcMark);
	pcMark = malloc(strlen(pcIn)+1);
	if ((char *)0 == pcMark) {
		fprintf(stderr, acOutMem, progname);
		exit(EX_OSERR);
	}
	(void)strcpy(pcMark, pcIn);
	if (fCase)
		stripc(pcMark, marklen, "");
}

/* set the submarker for this file					(ksb)
 */
void
setsub(pcIn)
char *pcIn;
{
	if ((char *)0 != pcSub) {
		free(pcSub);
	}
	if ((char *)0 == pcIn || '\000' == *pcIn) {
		pcSub = (char *)0;
		sublen = 0;
		return;
	}

	while (isspace(*pcIn)) {
		++pcIn;
	}
	sublen = strlen(pcIn);
	pcSub = malloc(strlen(pcIn)+1);
	if ((char *)0 == pcMark) {
		fprintf(stderr, acOutMem, progname);
		exit(EX_OSERR);
	}
	(void)strcpy(pcSub, pcIn);
	if (fCase)
		stripc(pcSub, sublen, "");
}

/* eat the submarker portion 						(sm)
 */
char *
eatsub(pch, smark)
char *pch;
char *smark;
{
	while (isspace(*pch)) {
		++pch;
	}

	if ((char *)0 != smark) {
		if ('(' != *pch) {
			return (char *)0;
		}

		do
			++pch;
		while (isspace(*pch));
		if (fCase)
			stripc(pch, sublen, "):");
		if ('*' == smark[0] && '\000' == smark[1]) {
			while (')' != pch[0] && '\000' != pch[0])
				++pch;
		} else if ('*' == pch[0]) {
			++pch;
		} else if (0 != strncmp(pch, smark, sublen)) {
			return (char *)0;
		} else {
			pch += sublen;
		}

		while (isspace(*pch)) {
			++pch;
		}
		if (')' != *pch) {
			return (char *)0;
		}
		do {
			++pch;
		} while (isspace(*pch));
	} else if ('(' == *pch) {
		while (')' != *pch)
			++pch;
		++pch;
		while (isspace(*pch))
			++pch;
	}
	return pch;
}

static char *pcDollarDollar = (char *)0;

/* find the end of the command string					(ksb)
 */
void
cut(pch)
char *pch;
{
	register char *q;
	register int c;

	pcDollarDollar = (char *)0;
	for (q = pch; '\000' != (c = *q); ++q) {
		switch (c) {
		case LEADCHAR:
			if (LEADCHAR == q[1]) {
				pcDollarDollar = q;
		case '\n':
				q[0] = '\000';
				return;
			}
			break;
		case '\\':
			if ('\000' != q[1])
				++q;
			break;
		default:
			break;
		}
	}
}

/* Called to keep the lines in a here document as we find the end of it	(ksb)
 * called at the end to backup the Here stream to the line with the end token.
 */
static int
CloneHere(FILE *fpIn, char pcLine[MAXLINE], void *pvData)
{
	static long wWas = 0L;
	register int iLen;

	if ((char *)0 == pcLine || (void *)0 == pvData) {
		return fseek(fpIn, wWas, SEEK_SET);
	}
	wWas = ftell(fpIn);
	if ((char *)0 != pcPipeLine && 0 == strncmp(pcLine, pcPipeLine, iLen = strlen(pcPipeLine))) {
		pcLine += iLen;
	}
	return fputs(pcLine, (FILE *)pvData);
}

static int
	fLocalAll = 0;		/* value for %+/%-			*/

/* Copy chars with C escapes etc...					(sm/ksb)
 * since ANSI C won't let us write on constant strings we need fColon
 * to stop on a colon durring template expansions.  piQuit is a flag
 * for `we have seen a %.' to the caller.
 */
char *
translit(dst, src, fColon, piQuit, piGoto, piSemi, ppcMatches, fpJSrc)
register char *dst;
register char *src, **ppcMatches;
int fColon, *piQuit, *piGoto, *piSemi;
FILE *fpJSrc;
{
#define MAXR	32		/* max recursive call or printf fmt	*/
	register char *tp;
	register char *xp, *pcParam, *pcBrack, *pcMixer, *pcLookHere;
	auto char *pcEntry;		/* where we started		*/
	auto char acTemp[MAXR];		/* buf for recursive calls	*/
	auto int i;			/* need to scanf into this	*/
	register int num;
	auto int cSize;			/* saved size/sign in %#	*/
	auto int fGrave;		/* change %`name` into ${name}	*/
	auto FILE *fpMagic;		/* read magic number		*/
	auto int fStr;			/* flag for %#...%s		*/
	auto unsigned uCall;
	auto long wCascade;
	auto char **ppcCascade;

#ifdef DEBUG
	fprintf(stderr, "translit(%s)\n", src);
#endif

	pcEntry = dst;
	ppcCascade = ppcHold;
	wCascade = 0L;
	fGrave = 0;
	pcLookHere = pcBrack = pcParam = (char *)0;
	pcMixer = (char *)0;
	while (*src) {
		switch (*src) {
		case '%':
			if ('(' == *++src) {
				pcMixer = dst;
				++src;
			}
			if ('[' == *src) {
				pcBrack = dst;
				++src;
			}
			switch (*src) {
			case '.':
				if (debug)
					fprintf(stderr, "%s: %s: %%. rejects file\n", progname, curfile);
				*piQuit = 1;
				return (char *)0;
			case ';':
				if ((int *)0 == piSemi) {
					fprintf(stderr, "%s: %s: %%; from bad context\n", progname, curfile);
					return (char *)0;
				}
				if (debug)
					fprintf(stderr, "%s: %s: %%;: last eligible item\n", progname, curfile);
				*piSemi = 1;
				break;
			case '^':
				if (debug)
					fprintf(stderr, "%s: %s: %%^ rejects line\n", progname, curfile);
				return (char *)0;

			case '|':	/* pipeline modifier for here docs */
				/* %| cDel expander cDel */
				{
				auto int len, iCur;
				auto char cDel, *pcSave;

				++src;
				if ('\000' == (cDel = *src++)) {
					if ((char *)0 != pcPipeLine) {
						free((void *)pcPipeLine);
						pcPipeLine = (char *)0;
					}
					return (char *)0;
				}
				len = (strlen(src)|1023)+1;
				if ((char *)0 == (pcPipeLine = malloc(len))) {
					fprintf(stderr, "%s: %s: out of memory\n", progname, curfile);
					return (char *)0;
				}
				for (iCur = 0; cDel != *src; ++iCur) {
					if (iCur >= len || '\000' == *src) {
						if (debug) {
							fprintf(stderr, "%s: %s: unterminated %%| (delimiter '%c')\n", progname, curfile, cDel);
						}
						return (char *)0;
					}
					pcPipeLine[iCur] = *src++;
				}
				pcPipeLine[iCur] = '\000';
				pcSave = strdup(pcPipeLine);
				if ((char *)0 == translit(pcPipeLine, pcSave, 0, piQuit, piGoto, piSemi, (char **)0, (FILE *)0)) {
					if (debug)
						fprintf(stderr, "%s: %s: here document prefix failed to expand `%s\'\n", progname, curfile, pcSave);
					free((void *)pcSave);
					free((void *)pcPipeLine);
					pcPipeLine = (char *)0;
					return (char *)0;
				}
				free((void *)pcSave);
				}
				break;
			case 'A':
			case 'a':
				if (fAll || fFirst) {
					if ('a' == *src)
						*dst++ = '-';
					*dst++ = fFirst ? 'A' : 'a';
				}
				break;
			case '+':
				if (fVerbose && !fLocalAll) {
					fprintf(stderr, "%s: %s: engage step mode\n", progname, curfile);
				}
				fLocalAll = 1;
				break;
			case '-':
				if (fVerbose && fLocalAll) {
					fprintf(stderr, "%s: %s: end step mode\n", progname, curfile);
				}
				fLocalAll = 0;
				break;

			case 'B':
				(void)strcpy(dst, progname);
				dst += strlen(dst);
				break;

			case 'b':
				(void)strcpy(dst, pathname);
				dst += strlen(dst);
				break;

			case 'C':
			case 'c':	/* mk command line flags */
				if (fConfirm) {
					if ('c' == *src)
						*dst++ = '-';
					*dst++ = 'c';
				}
				break;

			case 'D':	/* if not remote, fail	*/
			case 'd':	/* directory part	*/
				if ((tp = strrchr(curfile, '/')) == NULL) {
					if ('D' == *src) {
						if (debug)
							fprintf(stderr, "%s: %s is not remote\n", progname, curfile);
						return (char*)0;
					}
					break;
				}
				if (tp == curfile) {	/* example: `/dev' */
					*dst++ = '/';
					break;
				}
				*tp = '\000';
				(void)strcpy(dst, curfile);
				dst += strlen(curfile);
				*tp = '/';
				break;

			case 'e':
				if ((char *)0 == pchExamine)
					break;
				*dst++ = '-';
				*dst++ = 'e';
			case 'E':
				if ((char *)0 == pchExamine)
					break;
				(void)strcpy(dst, pchExamine);
				dst += strlen(dst);
				break;

			case 'f':	/* full filename */
				(void)strcpy(dst, curfile);
				dst += strlen(dst);
				break;

			case 'F':	/* file part only */
			case 'G':	/* like %F but don't chop off ext */
				if ((tp = strrchr(curfile, '/')) == NULL) {
					tp = curfile;
				} else if (curfile == tp && '\000' == tp[1]) {
					/* nothing, slash is `/' */
				} else {
					tp++;
				}
				if ('G' != *src && (xp = strrchr(tp, '.')) != NULL) {
					*xp = '\000';
					(void)strcpy(dst, tp);
					*xp = '.';
				} else {
					(void)strcpy(dst, tp);
				}
				dst += strlen(dst);
				break;

			case 'g':	/* goto file-name */
				if ((int *)0 == piGoto) {
					fprintf(stderr, "%s: goto in bad context (not searching a file)\n", progname);
					return (char *)0;
				}
				*piGoto = 'g';
				break;

			case 'h':	/* hot wire one marker to another */
				/* like: `$Compile: Cc%h' */
				if ((int *)0 == piGoto) {
					fprintf(stderr, "%s: hotwire in bad context (not searching a file)\n", progname);
					return (char *)0;
				}
				*piGoto = 'h';
				break;
			case 'H':	/* hotware a submarker to another? */
				/* like: `$Compile(IBMR2): RS6000%h' */
				if ((int *)0 == piGoto) {
					fprintf(stderr, "%s: submarker hotwire in bad context (not searching a file)\n", progname);
					return (char *)0;
				}
				*piGoto = 'H';
				break;
			case 'I':
			case 'i':
				if (fCase) {
					if ('i' == *src)
						*dst++ = '-';
					*dst++ = 'i';
				}
				break;

			case 'J':	/* here documents, build one -- ksb */
				if (0 != wCascade) {
					fseek(fpJSrc, wCascade, SEEK_SET);
				}
				if ((FILE *)0 == fpJSrc) {
					if (debug)
						fprintf(stderr, "%s: %%J: %s: rejects, not reading a file\n", progname, curfile);
					return (char *)0;
				}
				if ((char *)0 == (pcHere = FindSpace("mkhereXXXXXX", getenv("TMPDIR")))) {
					if (debug)
						fprintf(stderr, "%s: %%J: %s: rejects, no temporary file space (check $TMPDIR)\n", progname, curfile);
					return (char *)0;
				}
				{
				static char acEndOfHere[MAXLINE*2];
				register char *pcToken;
				auto CODE wCode;
				auto FILE *fpHere;
				auto long wAllTheWay;

				if ((FILE *)0 == (fpHere = fopen(pcHere, "wb"))) {
					if (debug)
						fprintf(stderr, "%s: %%J: %s: rejects, %s\n", progname, pcHere, strerror(errno));
					pcHere = (char *)0;
					return (char *)0;
				}
				wAllTheWay = ftell(fpJSrc);
				while ((char *)0 != (pcLookHere = findmarker(fpJSrc, acEndOfHere, &wCode, MK_IGNORE_L, CloneHere, (void *)fpHere))) {
					if ((char *)0 != (pcToken = strstr(acEndOfHere, "%j")) || ((char *)0 != pcDollarDollar && (pcToken = strstr(pcDollarDollar+1, "%j")))) {
						pcLookHere = pcToken+2;
						while (isspace(*pcLookHere))
							++pcLookHere;
						break;
					}
					if ((char *)0 != pcDollarDollar) {
						*pcDollarDollar = LEADCHAR;
					} else {
						strcat(acEndOfHere, "\n");
					}
					CloneHere(fpJSrc, acEndOfHere, fpHere);
				}
				fclose(fpHere);
				ppcCascade = AddClean(pcHere);

				if ((char *)0 == pcLookHere) {
					fseek(fpJSrc, wAllTheWay, SEEK_SET);
					if (debug)
						fprintf(stderr, "%s: %%J: %s: rejects, no end of the here text in file\n", progname, curfile);
					return (char *)0;
				}
				/* rescan the end line, as the next possbile
				 */
				wCascade = ftell(fpJSrc);
				CloneHere(fpJSrc, (char *)0, (void *)0);
				while (isspace(*src))
					++src;
				}
				break;
			case 'j':	/* here documents, give name -- ksb */
				if ((char *)0 == pcHere) {
					if (debug)
						fprintf(stderr, "%s: %%j: %s: rejects, no active here ducument\n", progname, curfile);
					return (char *)0;
				}
				(void)strcpy(dst, pcHere);
				dst += strlen(dst);
				break;
			case '?':
				if ((char *)0 == pcLookHere) {
					if (debug)
						fprintf(stderr, "%s: %%?: %s: rejects, no active here ducument\n", progname, curfile);
					return (char *)0;
				}
				(void)strcpy(dst, pcLookHere);
				dst += strlen(dst);
				break;
			case LEADCHAR:	/* text after the $$ token, if any */
				if ((char *)0 == (xp = pcDollarDollar)) {
					if (debug)
						fprintf(stderr, "%s: %%%c: %s: rejects, no %c%c on this template\n", progname, LEADCHAR, curfile, LEADCHAR, LEADCHAR);
					return (char *)0;
				}
				xp += 2;
				while (isspace(*xp))
					++xp;
				(void)strcpy(dst, xp);
				dst += strlen(dst);
				if ('\n' == dst[-1])
					--dst;
				break;

			case 'K':	/* the marker delimiter */
				*dst++ = LEADCHAR;
			case 'k':
				*dst++ = LEADCHAR;
				break;

			case 'l':
				*dst++ = '-';
				*dst++ = 'l';
			case 'L':
				sprintf(dst, "%d", lines);
				dst += strlen(dst);
				break;

			case 'm':	/* marker we need */
			case 'M':	/* lower case marker for make rules */
				if ('*' == pcMark[0])
					*dst++ = '\\';
				(void)strcpy(dst, pcMark);
				if ('M' == *src && !fCase)
					stripc(dst, marklen, "");
				dst += marklen;
				break;

			case 'N':
			case 'n':
				if (!fExec) {
					if ('n' == *src)
						*dst++ = '-';
					*dst++ = 'n';
				}
				break;

			case 'o':
				*dst++ = '-';
			case 'O': /* all switches, none can fail */
				(void)translit(dst, "%A%C%I%N%V", 0, piQuit, (int *)0, (int *)0, (char **)0, (FILE *)0);
				dst += strlen(dst);
				break;

			case 'P':
			case 'p':	/* prefix		*/
				acTemp[0] = '%';
				acTemp[1] = *src + ('Q' - 'P');
				acTemp[2] = '.';
				acTemp[3] = '\000';
				if ((tp = translit(dst, acTemp, 0, piQuit, (int *)0, (int *)0, (char **)0, (FILE *)0)) == NULL) {
					return (char *)0;
				}
				dst += strlen(dst);
				break;

			case 'Q':	/* prefix-x, mostly internal	*/
				if ((tp = strrchr(curfile, '/')) == NULL) {
			case 'q':
					tp = curfile;
				} else if (curfile == tp && '\000' == tp[1]) {
					/* nothing, slash is `/' */
				} else {
					++tp;
				}
				if ('\000' == *++src)
					break;
				if ((char *)0 == (xp = strrchr(tp, *src))) {
					if (debug)
						fprintf(stderr, "%s: %%q: no %c in \"%s\"\n", progname, *src, tp);
					return (char *)0;
				}
				*xp = '\000';
				(void)strcpy(dst, curfile);
				*xp = *src;
				dst += strlen(dst);
				break;

			case 'R':	/* rcsbasename	*/
				if ((tp = strrchr(curfile, '/')) == NULL) {
			case 'r':	/* rcsname	*/
					tp = curfile;
				} else if (curfile == tp && '\000' == tp[1]) {
					/* nothing, slash is `/' */
				} else {
					tp++;
				}
				tp = rcsname(tp);
				if ((char *)0 == tp) {
					if (debug)
						fprintf(stderr, "%s: no rcsfile for %s\n", progname, curfile);
					return (char *)0;
				}
				(void)strcpy(dst, tp);
				dst += strlen(dst);
				break;

			case 's': /* submarker we need */
			case 'S': /* lower case submarker for make rules */
				if ((char *)0 == pcSub) {
					if (debug)
						fprintf(stderr, "%s: no submarker for %s\n", progname, curfile);
					return (char *)0;
				}
				if ('*' == pcSub[0])
					*dst++ = '\\';
				(void)strcpy(dst, pcSub);
				if ('S' == *src && !fCase)
					stripc(dst, sublen, "");
				dst += sublen;
				break;

			case 't':
				if ((char *)0 == pchTemplates)
					break;
				*dst++ = '-';
				*dst++ = 't';
			case 'T':
				if ((char *)0 == pchTemplates)
					break;
				(void)strcpy(dst, pchTemplates);
				dst += strlen(dst);
				break;

			case 'U':	/* extender-x, mostly internal	*/
				if ((tp = strrchr(curfile, '/')) == NULL) {
			case 'u':
					tp = curfile;
				} else if (curfile == tp && '\000' == tp[1]) {
					/* nothing, slash is `/' */
				} else {
					++tp;
				}
				if ('\000' == *++src)
					break;
				if ((char *)0 == (xp = strrchr(tp, *src))) {
					if (debug)
						fprintf(stderr, "%s: %%u: no %c in \"%s\"\n", progname, *src, tp);
					return (char *)0;
				}
				++xp;
				(void)strcpy(dst, xp);
				dst += strlen(dst);
				break;

			case 'v':
				*dst++ = '-';
			case 'V':
				*dst++ = fVerbose ? 'v' : 's';
				if (debug)
					*dst++ = 'V';
				break;

			case 'W':
			case 'w':
				if ((tp = strrchr(acSearch, '/')) == NULL) {
					if ('W' == *src) {
						if (debug)
							fprintf(stderr, "%s: %s is not remote\n", progname, acSearch);
						return (char*)0;
					}
					break;
				}
				*tp = '\000';
				(void)strcpy(dst, acSearch);
				dst += strlen(dst);
				*tp = '/';
				break;

			case 'X':
			case 'x':	/* extension		*/
				acTemp[0] = '%';
				acTemp[1] = *src + ('U' - 'X');
				acTemp[2] = '.';
				acTemp[3] = '\000';
				if ((tp = translit(dst, acTemp, 0, piQuit, (int *)0, (int *)0, (char **)0, (FILE *)0)) == NULL) {
					return (char *)0;
				}
				dst += strlen(dst);
				break;

			case 'Y':
				if ((*++src == '~') ? *++src == chFType : *src != chFType) {
					if (debug)
						fprintf(stderr, "%s: %s fails file type %c\n", progname, curfile, *src);
					return (char*)0;
				}
				break;

			case 'y':	/* file type checks	*/
				*dst++ = chFType;
				break;

			case 'Z':
				if ((char *)0 == (tp = strrchr(acSearch, '/'))) {
			case 'z':
					tp = acSearch;
				} else {
					++tp;
				}
				if ('\000' == *tp) {
					if (debug)
						fprintf(stderr, "%s: %s: not a template\n", progname, curfile);
					return (char*)0;
				}
				(void)strcpy(dst, tp);
				dst += strlen(dst);
				break;

			case '~':	/* mk's home directory, so to speak */
				(void)strcpy(dst, pcTildeDir);
				dst += strlen(dst);
				break;

			case '0': case '1':	/* RE match in mapfile */
			case '2': case '3':
			case '4': case '5':
			case '6': case '7':
			case '8': case '9':
				{
				register char **ppc;
				num = *src - '0';
				if ((char **)0 != (ppc = ppcMatches)) {
					if ((char *)0 == (xp = ppc[num])) {
						fprintf(stderr, "%s: %%%c: no match for that subexpression\n", progname, *src);
						return (char *)0;
					}
				} else if ((char **)0 != (ppc = ppcCascade)) {
					while (isdigit(src[1])) {
						num *= 10;
						num += *++src - '0';
					}
					for (i = num; i > 0; --i) {
						if ((char *)0 == *++ppc) {
							fprintf(stderr, "%s: %%%d: not enough %%j files\n", progname, num);
							return (char *)0;
						}
					}
					xp = *ppc;
				} else {
					fprintf(stderr, "%s: %%%c: not in the contect of an RE match, or %%J\n", progname, *src);
					return (char *)0;
				}
				(void)strcpy(dst, xp);
				dst += strlen(dst);
				break;
				}

			case '#':	/* read bytes from a text file	    */
				/* the # escape allows us to check magic
				 * number and such, %#%05ow is a magic number
				 * (use bytes@seek for non-zero offests)
				 * formats and record sizes are so we can
				 * replace file(1)
				 * %# [nbytes] [@seek] [%] fmt [size]
				 * %#!		(full load path)
				 * %#/		(tail load path)
				 */
				if ('f' != chFType) {
					if (debug)
						fprintf(stderr, "%s: %s not a text file\n", progname, curfile);
					return (char*)0;
				}

				if ((FILE *)0 == (fpMagic = fopen(curfile, "rb"))) {
					fprintf(stderr, "%s: fopen: %s: %s\n", progname, curfile, strerror(errno));
					return (char *)0;
				}

				/* read load-card style magic number */
				if ('!' == *++src || '/' == *src) {
					register int c;
					if ('#' != getc(fpMagic) || '!' != getc(fpMagic)) {
						if (debug)
							fprintf(stderr, "%s: %s not #! loaded\n", progname, curfile);
						return (char*)0;
					}
					while (EOF != (c = getc(fpMagic))) {
						if (!isspace(c))
							break;
					}
					for (tp = dst; EOF != c && !isspace(c); c = getc(fpMagic)) {
						if ('/' == c && '/' == *src) {
							dst = tp;
						} else {
							*dst++ = c;
						}
					}
					(void)fclose(fpMagic);
					break;
				}

				/* parse out [nbytes][@lseek] */
				i = 0;
				sscanf(src, "%d", & i);
				if (0 == i) {
					i = 1;
				}
				while (isdigit(*src)) {
					++src;
				}
				if ('@' == *src) {
					auto long seek2;
					auto int r;

					++src;
					sscanf(src, "%ld", &seek2);
					if ('-' == *src || '+' == *src)
						++src;
					while (isdigit(*src))
						++src;
					if (seek2 < 0)
						r = fseek(fpMagic, -seek2, 2);
					else
						r = fseek(fpMagic, seek2, 0);
					if (-1 == r) {
						if (debug)
							fprintf(stderr, "%s: %s: improper seek to %ld\n", progname, curfile, seek2);
						return (char*)0;
					}
				}

				/* parse out printf-fmt [bwl] */
				num = 1;
				acTemp[0] = '%';
				if ('%' == *src) {
					++src;
				}
				fStr = 0;
				while (num < MAXR) {
					switch (acTemp[num++] = *src++) {
					case 's':  /* a '\000' term %c?*/
						acTemp[num-1] = 'c';
						fStr = 1;
						break;
					case 'o':
					case 'c':
					case 'x':
					case 'd':
					case 'u':
						break;
					case 'l': case 'h':
					case ' ': case '.':
					case '-': case '+':
					case '0': case '1':
					case '2': case '3':
					case '4': case '5':
					case '6': case '7':
					case '8': case '9':
						continue;
					case '*':
					default:
						fprintf(stderr, "%s: %s: unsupported printf format `%c'?\n", progname, curfile, src[-1]);
						return (char *)0;
					}
					break;
				}
				if (MAXR == num) {
					fprintf(stderr, "%s: %s: printf format too long\n", progname, curfile);
					return (char *)0;
				}
				acTemp[num] = '\000';

				switch (cSize = *src) {
				default:
					cSize = 'b';
					--src;
					/* fallthrough */
				case 'B':
				case 'b':
					num = sizeof(char);
					break;
				case 'W':
				case 'w':
					num = sizeof(short);
					break;
				case 'i':
				case 'I':
					num = sizeof(int);
					break;
				case 'L':
				case 'l':
					num = sizeof(long);
					break;
				}

				/* write out bytes in the user's format */
				/* od(1) uses a union, as far as I can tell
				 * from the output on various machines...
				 * we will do it this way to -- bletch <ksb>
				 */
				while (i--) {
					union {
						char in[sizeof(long)];
						long L;
						short W;
						char B;
						unsigned long l;
						unsigned short w;
						unsigned char b;
					} U;
					int pos, cr;
					U.l = 0L;
					for (pos = 0; pos < num; ++pos) {
						if (EOF == (cr = getc(fpMagic)))
							break;
						U.in[pos] = (char)cr;
					}
					if (fStr && 0L == U.l) {
						break;
					}
					switch (cSize) {
					case 'B':
						sprintf(dst, acTemp, U.B);
						break;
					case 'b':
						sprintf(dst, acTemp, U.b);
						break;
					case 'W':
						sprintf(dst, acTemp, U.W);
						break;
					case 'w':
						sprintf(dst, acTemp, U.w);
						break;
					case 'L':
						sprintf(dst, acTemp, U.L);
						break;
					case 'l':
						sprintf(dst, acTemp, U.l);
						break;
					}
					dst += strlen(dst);
					if (EOF == cr) {
						break;
					}
				}
				(void)fclose(fpMagic);
				break;

			case '<':	/* map from mapfile		*/
				/* %</usr/local/lib/mk/blade>
				 * Expand the source into a file name, the file
				 * contains a 3 col format:
				 *   expand-text    R.E.   substitution-text
				 *   [expand-text2] R.E.   result2
				 * All are limited to MAXPATHLEN*2, separated
				 * by tabs.  Use \t to embedded a tab, and \s a
				 * space.  Use no R.E. matching delimiters here.
				 */
				fStr = 0;
				{
				auto char acFile[MAXPATHLEN*2+1];
				auto char acBuf[MAXPATHLEN*2+1];
				auto char acOText[MAXPATHLEN*2+1];
				auto char acRE[MAXPATHLEN*2+1];
				auto char acEText[MAXPATHLEN*2+1];
				auto int iSkip, iQuit, iReMap, iLine;
				auto char *apcCell[10];

				tp = src;
				i = 1;
				while (0 != i && '\000' != *++src) {
					switch (*src) {
					case '>':
						--i;
						break;
					case '<':
						++i;
						/* fallthrough */
					default:
						break;
					}
				}
				if (0 != i) {
					fprintf(stderr, "%s: %%<: %s: missing `>' (%s)\n", progname, curfile, tp);
					return (char *)0;
				}
				*src = '\000';
				iQuit = 0;
				tp = translit(acFile, tp+1, 0, &iQuit, (int *)0, piSemi, (char **)0, (FILE *)0);
				*src = '>';

				if ((char *)0 == tp || 0 != iQuit) {
					return (char *)0;
				}
			new_file:
				if ('\000' == acFile[0]) {
					fprintf(stderr, "%s: %s: no file for %%<>\n", progname, curfile);
					return (char *)0;
				}

				if ((FILE *)0 == (fpMagic = fopen(acFile, "rb"))) {
					fprintf(stderr, "%s: %%<: fopen: %s: %s\n", progname, acFile, strerror(errno));
					return (char *)0;
				}

				acEText[0] = '\000';
				iSkip = iQuit = iReMap = 0;
				for (iLine = 1; !iSkip && !iQuit && EOF != (i = getc(fpMagic)); ++iLine) {
					if ('\n' == i) {
						continue;
					}
					if ('#' == i) {
						goto eat;
					}
					ungetc(i, fpMagic);
					if (isspace(i)) {
						/* use last exp. text */
					} else if (acOText[0] = '\000', 1 != fscanf(fpMagic, "%[^\t \n]", acOText)) {
						if (debug) {
							fprintf(stderr, "%s: %s: %s:%d: no test string\n", progname, curfile, acFile, iLine);
						}
						goto eat;
					}
					if ((char *)0 == translit(acEText, acOText, 0, & iQuit, (int *)0, &iSkip, (char **)0, (FILE *)0)) {
						goto eat;
					}
					if (1 != fscanf(fpMagic, "%*[\t ]%[^\t \n]", acBuf) || '\000' == acBuf[0]) {
						if (debug) {
							fprintf(stderr, "%s: %s: %s:%d: no regular expression\n", progname, curfile, acFile, iLine);
						}
						goto eat;
					}
					if ((char *)0 == translit(acRE, acBuf, 0, &iQuit, (int *)0, &iSkip, (char **)0, (FILE *)0)) {
						goto eat;
					}

					if (!ReMatch(acEText, acRE)) {
						if (debug) {
							fprintf(stderr, "%s: %s: %s:%d: %s !~ m/%s/\n", progname, curfile, acFile, iLine, acEText, acRE);
						}
						goto eat;
					}
					if (1 != fscanf(fpMagic, "%*[\t ]%[^\n]", acBuf) || '\000' == acBuf[0]) {
						if (debug) {
							fprintf(stderr, "%s: %s: %s:%d: no result\n", progname, curfile, acFile, iLine);
						}
						goto eat;
					}
					iReMap = '\000';
					for (i = 0; i < sizeof(apcCell)/sizeof(apcCell[0]); ++i) {
						register char *pcStart;
						auto char *pcEnd;
						register int cSwap;

						pcStart = ReParen(acEText, i, & pcEnd);
						if ((char *)0 == (apcCell[i] = pcStart)) {
							continue;
						}
						cSwap = *pcEnd;
						*pcEnd = '\000';
						apcCell[i] = strdup(pcStart);
						*pcEnd = cSwap;
					}
					fStr = ((char *)0 != translit(dst, acBuf, 0, piQuit, & iReMap, &iSkip, apcCell, (FILE *)0));
					for (i = 0; i < sizeof(apcCell)/sizeof(apcCell[0]); ++i) {
						if ((char *)0 == apcCell[i])
							continue;
						free((void *)apcCell[i]);
					}
					if (fStr) {
						break;
					}
					/* if we get here this line lost
					 */
			 eat:
					while (EOF != (i = getc(fpMagic)) && '\n' != i)
						;
				}
				(void)fclose(fpMagic);
				if (iQuit) {
					if (debug)
						fprintf(stderr, "%s: %s: %%<%s>: quit\n", progname, curfile, acFile);
					return (char *)0;
				}
				if (!fStr) {
					if (debug)
						fprintf(stderr, "%s: %s: %%<%s>: no match\n", progname, curfile, acFile);
					return (char *)0;
				}
				switch (iReMap) {
				case 'g': /* open another file */
					(void)strcpy(acFile, dst);
					*dst = '\000';
					goto new_file;
				case 'h': /* normal hotwire */
				case 'H':
					if ((int *)0 != piGoto) {
						*piGoto = iReMap;
						dst += strlen(dst);
						break;
					}
					fprintf(stderr, "%s: %s: %%<%s>: matched bad hotwire context (%s)\n", progname, curfile, acFile, dst);
					return (char *)0;
				default: /* '\000' */
					break;
				}
				if (debug)
					fprintf(stderr, "%s: %s: %%<%s>: matched %s\n", progname, curfile, acFile, dst);
				}
				dst += strlen(dst);
				break;

			case '!':	/* not cmp			*/
			case '=':	/* string cmp			*/
				/* compare and reject/accept */
				{
				register char *pcSrc;
				auto char acLeft[MAXPATHLEN*2+4];
				auto char acRight[MAXPATHLEN*2+4];
				auto int len;
				auto char cDel;

				cSize = *src++;
				if ('\000' == (cDel = *src++)) {
					return (char *)0;
				}

				len = strlen(src);
				if ((char *)0 == (pcSrc = malloc((len|63)+1))) {
					fprintf(stderr, "%s: %s: out of memory\n", progname, curfile);
					return (char *)0;
				}

				xp = pcSrc;
				while (*src != cDel && '\000' != *src)
					*xp++ = *src++;
				*xp = '\000';
				if ('\000' != *src)
					++src;

				if ((char *)0 == translit(acLeft, pcSrc, 0, piQuit, piGoto, piSemi, (char **)0, (FILE *)0)) {
					if (debug)
						fprintf(stderr, "%s: %s: left side failed to expand `%s\'\n", progname, curfile, pcSrc);
					free(pcSrc);
					return (char *)0;
				}

				xp = pcSrc;
				while (*src != cDel && '\000' != *src)
					*xp++ = *src++;
				*xp = '\000';
				if ('\000' != *src)
					(void)++src;

				if ((char *)0 == translit(acRight, pcSrc, 0, piQuit, piGoto, piSemi, (char **)0, (FILE *)0)) {
					if (debug)
						fprintf(stderr, "%s: %s: right side failed to expand `%s\'\n", progname, curfile, pcSrc);
					free(pcSrc);
					return (char *)0;
				}
				free(pcSrc);
				if ((0 != strcmp(acLeft, acRight)) == ('=' == cSize)) {
					if (debug)
						fprintf(stderr, "%s: %s: fails %s %c= %s\n", progname, curfile, acLeft, cSize, acRight);
					return (char *)0;
				}
				}
				--src;
				break;

			case '\"':
			case '`':
				fGrave = 1;
				/* FALLTHROUGH */
			case '{':	/* } */
				*dst++ = '$';
				*dst++ = '{';	/* } */
				pcParam = dst;
				break;
			case '*':	/* actual match marker(submarker) */
				if ((char *)0 == pcActual) {
					if (debug)
						fprintf(stderr, "%s: %s: fails to find actual marker\n", progname, curfile);
					return (char *)0;
				}
				(void)strcpy(dst, pcActual);
				dst += strlen(dst);
				break;
			default:	/* unrecognized chars are copied thru */
				*dst++ = *src;
				break;
			}
			src++;
			break;

		case '\\':
			switch (*++src) {
			case '\n':	/* how would this happen? */
			case 'e':
				++src;
				break;
			case 's':	/* in a pattern match \s for space */
				*dst++ = ' ';
				++src;
				break;
			case 'n':	/* newline */
				*dst++ = '\n';
				++src;
				break;
			case 't':
				*dst++ = '\t';
				++src;
				break;
			case 'b':
				*dst++ = '\b';
				++src;
				break;
			case 'r':
				*dst++ = '\r';
				++src;
				break;
			case 'f':
				*dst++ = '\f';
				++src;
				break;
			case 'v':
				*dst++ = '\013';
				++src;
				break;
			case '\\':
				++src;
			case '\000':
				*dst++ = '\\';
				break;

			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
				num = *src++ - '0';
				for (i = 0; i < 2; i++) {
					if (! isdigit(*src)) {
						break;
					}
					num <<= 3;
					num += *src++ - '0';
				}
				*dst++ = num;
				break;
			case '8': case '9':
				/* 8 & 9 are bogus octals,
				 * cc makes them literals
				 */
				/*fallthrough*/
			default:
				*dst++ = *src++;
				break;
			}
			break;
		case '\"':
		case '`':
			if (fGrave) {
		case /*{*/ '}':
				*dst = '\000';
				if ((char *)0 != pcParam && (char *)0 == getenv(pcParam)) {
					if (debug)
						fprintf(stderr, "%s: %s: %s: not set\n", progname, curfile, pcParam);
					return (char *)0;
				}
				pcParam = (char *)0;
				*dst++ = /*{*/ '}';
				++src, fGrave = 0;
				break;
			}
			*dst++ = *src++;
			break;
		case ':':
			if (fColon) {
				src = "";
				break;
			}
			/*FALLTHROUGH*/
		default:
			*dst++ = *src++;
			break;
		}
		/* check for xapply dicer expression, use that code.
		 * N.B. the top-level of the Mixer is %(...), not %<...>
		 */
		if ((char *)0 != pcBrack) {
			register char *pcErr;
			*dst = '\000';
			dst = pcBrack;
			pcBrack = strdup(dst);
			*dst = '\000';
			pcErr = src;
			uCall = MAXLINE - (dst - pcEntry);
			if ((char *)0 == (src = Dicer(dst, &uCall, src, pcBrack))) {
				if (debug)
					fprintf(stderr, "%s: %s: %%{: dicer: rejects, [%.8s on \"%s\"\n", progname, curfile, pcErr, pcBrack);
				free((void *)pcBrack);
				return (char *)0;
			}
			free((void *)pcBrack);
			dst += strlen(dst);
			pcBrack = (char *)0;
		}
		if ((char *)0 != pcMixer) {
			uCall = MAXLINE - (pcMixer - pcEntry);
			if ((char *)0 == (src = Mixer(pcMixer, &uCall, src, ')'))) {
				fprintf(stderr, "%s: %s %%(: mixer rejects\n", progname, curfile);
				return (char*)0;
			}
			dst = pcMixer+strlen(pcMixer);
			pcMixer = (char *)0;
		}
	}

	*dst = '\000';
	return dst;
}

/* Find a marker line in the given file, put it in a buffer and		(sm/ksb)
 * return a pointer to it.  Limitted to count lines.  Set %* for us too.
 */
char *
findmarker(FILE *fin, char buf[MAXLINE], CODE *pexitcode, int count, int (pfiWatch)(FILE *, char *, void *), void *pvData)
{
	register char *pch, *pcClose;
	extern long atol();

	if ((char *)0 != pcActual) {
		free((void *)pcActual);
		pcActual = (char *)0;
	}
	if ((FILE *)0 == fin)
		return (char *)0;
	if (0 == count)
		count = MK_IGNORE_L;
	while ((char *)0 != (pch = fgets(buf, MAXLINE, fin))) {
		if (MK_IGNORE_L != count && count-- < 1) {
			pch = (char *)0;
			if (debug)
				fprintf(stderr, "%s: out of lines\n", progname);
			break;
		}
		if ((char *)0 == (pch = strchr(buf, LEADCHAR)) || (char *)0 == (pcClose = strpbrk(pch, /*(*/":)"))) {
			if ((int (*)())0 != pfiWatch)
				pfiWatch(fin, buf, pvData);
			continue;
		}
		do {
			++pch;
		} while (isspace(*pch));

		if ((char *)0 != pcActual) {
			free((void *)pcActual);
		}
		if (':' == *pcClose) {
			pcClose[0] = '\000';
			pcActual = strdup(pch);
			pcClose[0] = ':';
		} else {
			register int cKeep;

			cKeep = pcClose[1];
			pcClose[1] = '\000';
			pcActual = strdup(pch);
			pcClose[1] = cKeep;
		}
		if (fCase)
			stripc(pch, marklen, ":("/*)*/);
		if ('*' == pcMark[0] && '\000' == pcMark[1]) {
			while ('(' != pch[0] && ':' != pch[0] && '\000' != pch[0])
				++pch;
		} else if ('*' == pch[0]) {
			++pch;
		} else if (0 != strncmp(pcMark, pch, marklen)) {
			if ((int (*)())0 != pfiWatch)
				pfiWatch(fin, buf, pvData);
			continue;
		} else {
			pch += marklen;
		}
		pch = eatsub(pch, pcSub);
		if ((char *)0 == pch) {
			if ((int (*)())0 != pfiWatch)
				pfiWatch(fin, buf, pvData);
			continue;
		}

		/* set exit code */
		pexitcode->ekey = EXIT_EXACT;
		if ('=' == *pch) {
			++pch;
			while (isspace(*pch))
				++pch;
			if ('~' == *pch) { 	/* ~code	*/
				pexitcode->ekey = EXIT_NOT;
				++pch;
			}
			while (isspace(*pch))
				++pch;
			if ('*' == *pch) { 	/* any code	*/
				pexitcode->ekey |= EXIT_ANY;
				++pch;
			} else {
				pexitcode->ivalue = atol(pch);
				if ('-' == *pch || '+' == *pch)
					++pch;
				while (isdigit(*pch))
					++pch;
			}
			while (isspace(*pch))
				++pch;
		} else {
			pexitcode->ivalue = 0;
		}

#if RESOURCE
		/* get resource limits		*/
		do {
			if (',' == *pch)
				++pch;
			stripc(pch, MAXLINE, ",:");
			pch = rparse(pch);
		} while (',' == *pch);
#else
		/* try to eat resource limits	*/
		pch = strchr(pch, ':');
#endif /* resource limits */

		if ((char *)0 == pch || ':' != *pch) {
			if ((int (*)())0 != pfiWatch)
				pfiWatch(fin, buf, pvData);
			continue;
		}

		/* found mk marker */
		do {
			++pch;
		} while (isspace(*pch));
		cut(pch);
		break;
	}
	return pch;
#undef MK_IGNORE_L
}

/* place a variable in the environment					(ksb)
 */
int
define(pch)
char *pch;
{
	register char *p;

	p = strchr(pch, '=');
	if ((char *)0 == p) {
		fprintf(stderr, "%s: missing `=\' for define of %s\n", progname, pch);
		return 0;
	}
	p[0] = '\000';
	setenv(pch, p+1, 1);
	p[0] = '=';
	return 0;
}

/* remove a variable from the environment				(ksb)
 */
int
undefine(pch)
char *pch;
{
	return setenv(pch, (char *)0, 1);
}

/* we have a filename and are ready to open and find a marker in it	(sm/ksb)
 */
int
process(arg)
char *arg;
{
	auto FILE *fin, *fpSrc;
	auto char *pch, *pcGoto, *pchSecond, *pchBefore, *pchLoop, *pchTrans;
	auto int iRet, fFoundOne, fQuit, fGoto, fTempl;
	auto int fLastTempl, fLastGoto, fLastChance;
	auto CODE exitcode;
	auto char buf[MAXLINE*2];
	auto char combuf[MAXLINE*2];
	auto struct stat stIn;
	auto char chAType, *pcForce;
	static char acLooking[] = "%s: looking for $%s(%s)\n";

#if RESOURCE
	rinit();
	r_fTrace = debug;
#endif /* resource limits */

	fLocalAll = fAll;
	curfile = arg;
	if ('-' == curfile[0] && '\000' == curfile[1]) {
		fprintf(stderr, "%s: stdin not supported\n", progname);
		return 1;
	}
	if (-1 == LSTAT(curfile, & stIn)) {
		fprintf(stderr, "%s: stat: %s: %s\n", progname, curfile, strerror(errno));
		return 1;
	}
	switch (stIn.st_mode & S_IFMT) {
#if defined(S_IFLNK)
	case S_IFLNK:	/* symbolic link */
		chFType = 'l';
		chAType = (-1 != stat(curfile, & stIn) && (S_IFREG == (stIn.st_mode & S_IFMT) || 0 == (stIn.st_mode & S_IFMT))) ? 'f' : '!';
		break;
#endif
#if defined(S_IFIFO)
	case S_IFIFO:	/* fifo */
		chFType = 'p';
		break;
#endif	/* no fifos */
#if defined(S_IFSOCK)
	case S_IFSOCK:	/* socket */
		chFType = 's';
		break;
#endif	/* no sockets */
#if defined(S_IFDOOR)
	case S_IFDOOR:	/* door */
		chFType = 'D';
		break;
#endif	/* no Solaris door type */
#if defined(S_IFWHT)
	case S_IFWHT:	/* white-out */
		chFType = 'w';
		break;
#endif	/* no white-out nodes */
	case S_IFDIR:	/* directory */
		chFType = 'd';
		while ((char *)0 != (pch = strrchr(curfile, '/')) && '\000' == pch[1] && pch != curfile)
			*pch = '\000';
		break;
	case S_IFCHR:	/* character special */
		chFType = 'c';
		break;
	case S_IFBLK:	/* block special */
		chFType = 'b';
		break;
#if defined(S_IFCTG)
	case S_IFCTG:	/* contiguous file [MASSCOMP only?] */
		/* fall through ... treat as plain file */
#endif
#if 0 != S_IFREG
	case 0:		/* older unix systems let plain files be unmarked */
		/* fall through to plain file */
#endif
	case S_IFREG:	/* regular */
		chFType = 'f';
		break;
	default:
		fprintf(stderr, "%s: stat: %s: unknown file type?\n", progname, curfile);
		return 1;
	}
	/* dirs, sockets, etc. don't have text to read			(ksb)
	 * but do open symbolic links, if they point to plain files
	 */
	fpSrc = (FILE *)0;
	if ('f' == chFType || ('l' == chFType && 'f' == chAType)) {
		if (NULL == (fpSrc = fopen(curfile, "r"))) {
			fprintf(stderr, "%s: fopen: %s: %s\n", progname, curfile, strerror(errno));
		}
	}

	/* a lose, once -i (fCase) is set it cannot be turned off
	 * {because we overwrite the marker (and submarker) in place}
	 */
	if ((char *)0 != markstr)
		setmark(markstr);
	else if ('m' == progname[0] && 'k' == progname[1] && '\000' == progname[2])
		setmark((char *)0);
	else
		setmark(progname);
	setsub(submark);

	if (debug) {
		fprintf(stderr, acLooking, progname, pcMark, (char *)0 == pcSub ? "*" : pcSub);
	}
	pchBefore = pchExamine;
#if defined(BEFORE)
	if ((char *)0 == pchBefore) {
		pchBefore = acDefBefore;
	}
#endif	/* default pre-scan	*/
	pchSecond = pchTemplates;
#if defined(TEMPL)
	if ((char *)0 == pchSecond) {
		pchSecond = acDefAfter;
	}
#endif /* default templates	*/

	if (! fExec)
		fVerbose = 1;
	fLastTempl = fFoundOne = 0;
	iRet = 0;
	pcGoto = (char *)0;
	fin = (FILE *)0;

	/* inv: fin is (FILE *)0 or points to a file we opened below
	 */
 new_templ:
	if ((FILE *)0 != fin) {
		(void)fclose(fin);
		fin = (FILE *)0;
	}
	fLastChance = fLastGoto = fGoto = fQuit = 0;
	pcPipeLine = (char *)0;
	AddClean((char *)0);
	pcForce = pcExpand;
	do {
		if ((char *)0 != pcForce) {
			pch = pcForce;
			fLastChance = 1;
		} else {
			pch = findmarker(fin, & buf[0], & exitcode, lines, (int (*)())0, (void *)0);
		}
		if ((char *)0 == pch && (char *)0 != pcGoto && !fLastGoto) {
			fTempl = 0;
			do {
				pchLoop = strchr(pcGoto, ':');
				if ((char *)0 != pchLoop)
					++pchLoop;
				pchTrans = translit(acSearch, pcGoto, 1, & fTempl, (int *)0, &fLastGoto, (char **)0, fin);
				pcGoto = pchLoop;
				if ((FILE *)0 != fin)
					(void)fclose(fin);
				fin = (char *)0 == pchTrans ? (FILE *)0 : fopen(acSearch, "r");
			} while ((FILE *)0 == fin && (char *)0 != pcGoto && !fTempl);
			if (debug && (FILE *)0 != fin) {
				fprintf(stderr, "%s: goto %s\n", progname, acSearch);
			}
			if ((FILE *)0 != fin)
				continue;
		}
		if ((char *)0 == pch && (char *)0 != pchBefore && !fLastTempl) {
			fTempl = 0;
			do {
				pchLoop = strchr(pchBefore, ':');
				if ((char *)0 != pchLoop)
					++pchLoop;
				pchTrans = translit(acSearch, pchBefore, 1, & fTempl, (int *)0, &fLastTempl, (char **)0, fin);
				pchBefore = pchLoop;
				if ((FILE *)0 != fin)
					(void)fclose(fin);
				fin = (char *)0 == pchTrans ? (FILE *)0 : fopen(acSearch, "r");
				if ((FILE *)0 == fin)
					fLastTempl = 0;
			} while ((FILE *)0 == fin && (char *)0 != pchBefore && !fTempl);
			if (debug && (FILE *)0 != fin) {
				fprintf(stderr, "%s: searching template %s\n", progname, acSearch);
			}
			if ((FILE *)0 != fin)
				continue;
		}
		if ((char *)0 == pch && (FILE *)0 != fpSrc) {
			if ((FILE*)0 != fin)
				(void)fclose(fin);
			fin = fpSrc;
			fpSrc = (FILE *)0;
			acSearch[0] = '\000';
			if (debug) {
				fprintf(stderr, "%s: searching file %s\n", progname, curfile);
			}
			continue;
		}
		if ((char *)0 == pch && (char *)0 != pchSecond && !fLastTempl) {
			fTempl = 0;
			do {
				pchLoop = strchr(pchSecond, ':');
				if ((char *)0 != pchLoop)
					++pchLoop;
				pchTrans = translit(acSearch, pchSecond, 1, &fTempl, (int *)0, &fLastTempl, (char **)0, fin);
				pchSecond = pchLoop;
				if ((FILE *)0 != fin)
					(void)fclose(fin);
				fin = (char *)0 == pchTrans ? (FILE *)0 : fopen(acSearch, "r");
				if ((FILE *)0 == fin)
					fLastTempl = 0;
			} while ((FILE *)0 == fin && (char *)0 != pchSecond && !fTempl);
			if (debug && (FILE *)0 != fin) {
				fprintf(stderr, "%s: searching template %s\n", progname, acSearch);
			}
			continue;
		}
		if ((char *)0 == pch) {
			break;
		}
		if ((char *)0 == translit(combuf, pch, 0, &fQuit, &fGoto, &fLastChance, (char **)0, fin)) {
			if (fQuit) {
				goto new_templ;
			}
			/* for this to happen the %; must come before the
			 * check that failed, that is an intentional
			 * `fail here' note to us.		(ksb)
			 */
			if (fLastChance) {
				if ((FILE *)0 != fin) {
					(void)fclose(fin);
				}
				return iRet;
			}
			continue;
		}
		if ('g' == fGoto) {
			/* we never free this :-[		(ksb)
			 */
			pcGoto = malloc(strlen(combuf)+1);
			if ((char *)0 == pcGoto) {
				fprintf(stderr, acOutMem, progname);
				exit(EX_OSERR);
			}
			(void)strcpy(pcGoto, combuf);
			goto new_templ;
		}
		/* hot wire code one marker to another
		 */
		if ('h' == fGoto) {
			register char *pcOpen;
			if ((char *)0 == (pcOpen = strchr(combuf, '('/*)*/))) {
				setmark(combuf);
			} else {
				register char *pcClose;
				*pcOpen++ = '\000';
				if ((char *)0 != (pcClose = strchr(pcOpen, /*(*/')')))
					*pcClose = '\000';
				setmark(combuf);
				setsub(pcOpen);
			}
			if (debug) {
				fprintf(stderr, acLooking, progname, pcMark, (char *)0 == pcSub ? "*" : pcSub);
			}
			fGoto = 0;
			continue;
		}
		if ('H' == fGoto) {
			setsub(combuf);
			if (debug) {
				fprintf(stderr, acLooking, progname, pcMark, (char *)0 == pcSub ? "*" : pcSub);
			}
			fGoto = 0;
			continue;
		}
		fFoundOne = 1;
		if (fExec && fConfirm) {
 help:
			fprintf(stderr, "\t%s  [fnyqh]%s? ", combuf, fLastChance ? "(last chance)" : "");
			(void)fflush(stderr);
			if ((char *)0 == fgets(buf, sizeof(buf), stdin))
				continue;

			for (pch = buf; '\000' != *pch && isspace(*pch); ++pch)
				/* empty */;
			switch (*pch) {
			case 'h':
			case 'H':
			default:
				fprintf(stderr, "f  find the next match%s\nn  next file please\ny  yes -- run the command\nq  quit\nh  this help message\n", fLastChance ? " (ignore last chance)" : "");
				goto help;
			case 'q':
			case 'Q':
				AddClean((char *)0);
				exit(EX_OK);
			case 'y':
			case 'Y':
				break;
			case 'F':
				fLastChance = (char *)0 != pcForce;
				/*fallthrough*/
			case 'f':
				continue;
			case '\n':
			case '\000':
				if (!fLastChance)
					continue;
				/*fallthrough*/
			case 'n':
			case 'N':
				if ((FILE *)0 != fin)
					(void)fclose(fin);
				return iRet;
			}
		} else if (!fExec || fVerbose) {
			fprintf(stderr, "\t%s\n", combuf);
		}
		(void)fflush(stderr);
		if (fExec) {
			auto int cur, code;
#if RESOURCE
			code = rlimsys(combuf);
#else /* use vanilla system */
			code = system(combuf);
			if (0177 == (code & 0xff)) {		/* stopped */
				code = (code >> 8) & 0xff;
			} else if (0 != (code & 0xff)) {	/* killed */
				code = code & 0x7f;
			} else {				/* exit */
				code = (code >> 8) & 0xff;
			}
#endif /* check for resource limits */
			if (debug)
				fprintf(stderr, "%s: command exits %d\n", progname, code);
			switch (exitcode.ekey) {
			case EXIT_EXACT:
				cur = code != exitcode.ivalue;
				break;
			case EXIT_EXACT|EXIT_NOT:
				cur = code == exitcode.ivalue;
				break;
			case EXIT_ANY:
				cur = 0;
				break;
			case EXIT_ANY|EXIT_NOT:
				cur = 1;
				break;
			}
			if (fFirst && 0 == cur) {
				if ((FILE *)0 != fin)
					(void)fclose(fin);
				return 0;
			}
			iRet += cur;
		}
		if ((fLocalAll || fFirst) && !fLastChance) {
			continue;
		}
		if ((FILE *)0 != fin) {
			(void)fclose(fin);
		}
		return iRet;
	} while ((FILE *)0 != fin);
	if (fVerbose && !fFoundOne) {
		fprintf(stderr, "%s: no marker \"%c%s", progname, LEADCHAR, pcMark);
		if ((char *)0 != pcSub) {
			fprintf(stderr, "(%s)", pcSub);
		}
		fprintf(stderr, ":\" %s %s\n", fFoundOne ? "selected from" : "in", curfile);
	}
	if (!fFoundOne)
		++iRet;
	return iRet;
}
