/* $Id: glob.c,v 3.4 2003/04/20 19:36:49 ksb Exp $
 * is a glob expression the same path as a file name			(ksb)
 * Kevin Braunsdorf, ksb@staff.cc.purdue.edu, pur-ee!ksb, purdue!ksb
 *
 * $Compile: ${cc=cc} -DTEST ${cc_debug=-g} %f -o %F
 * $Cc: ${cc=cc} ${cc_debug=-O} -c %f
 */
#include <stdio.h>
#include <ctype.h>

#include "machine.h"
#include "libtomb.h"
#include "glob.h"

#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif

/*
 * _SamePath()
 *	We want /bin/* to match every file in /bin OK.			(ksb)
 *	return 1 for ==, 0 for !=
 *	
 * N.B. Must call CompPath on pcFile first, or know it is minimal
 */
_SamePath(pcGlob, pcFile, fDot)
char *pcGlob;		/* the pattern to match				*/
char *pcFile;		/* the file to match with			*/
int fDot;		/* are we at the start of pcFile, or post '/'	*/
{
	register char *pc;
	register int iLenGlob, iLenFile;
	auto int bFound, cStop;

	for (;;) { switch (*pcGlob) {
	case '*':		/* match any string			*/
		pc = ++pcGlob;
		iLenGlob = 0;
		while ('\\' != *pc && '?' != *pc && '[' != *pc && '*' != *pc && '\000' != *pc && '/' != *pc) {
			++pc, ++iLenGlob;
		}

			
		iLenFile = 0;
		while ('/' != pcFile[iLenFile] && '\000' != pcFile[iLenFile] &&
		       (!fDot || '.' != pcFile[iLenFile])) {
			++iLenFile;
			fDot = 0;
		}

		bFound = 0;
		do {
			if (iLenGlob == 0 || 0 == strncmp(pcGlob, pcFile, iLenGlob)) {
				if (_SamePath(pc, pcFile+iLenGlob, fDot)) {
					bFound = 1;
					break;
				}
			}
			--iLenFile, ++pcFile;
		} while (iLenFile >= iLenGlob);
		return bFound;
	case '[':		/* any of				*/
		++pcGlob;
		cStop = *pcFile++;
		if (cStop == '/')	/* range never match '/'	*/
			break;
		bFound = 0;
		if ('-' == *pcGlob) {
			bFound = '-' == cStop;
			++pcGlob;
		}
		while (']' != *pcGlob) {
			if ('-' == pcGlob[1]) {
				if (pcGlob[0] <= cStop && cStop <= pcGlob[2])
					bFound = 1;
				pcGlob += 2;
			} else {
				if (pcGlob[0] == cStop)
					bFound = 1;
			}
			++pcGlob;
		}
		++pcGlob;
		if (!bFound)
			break;
		continue;
	case '?':		/* and single char but '/'		*/
		if ('/' == *pcFile || (fDot && '.' == *pcFile) || '\000' == *pcFile)
			break;
		++pcGlob, ++pcFile;
		fDot = 0;
		continue;
	case '\\':		/* next char not special		*/
		++pcGlob;
		/*fall through*/
	case '/':		/* delimiter				*/
		fDot = 1;
		if (*pcGlob != *pcFile)
			break;
		++pcGlob;
		do {
			++pcFile;
		} while ('/' == *pcFile);
		continue;
	default:		/* or any other character		*/
		fDot = 0;
		if (*pcGlob != *pcFile)
			break;
		++pcGlob, ++pcFile;
		continue;
	case '\000':		/* end of pattern, end of file name	*/
		return '\000' == *pcFile;
	} break; }
	return 0;
}


#if defined(TEST)

/* Crack, simple configuration file parser				(ksb)
 */
char *
Crack(pcText, ppcVar)
char *pcText;
char **ppcVar;
{
	register char ***pppc;
	register char **ppc;

	pppc = & ppcVar;
	while ((char **)0 != (ppc = *pppc++)) {
		if ('\000' == *pcText) {
			*ppc = (char *)0;
			continue;
		}
		*ppc = pcText;
		while (!isspace(*pcText) && '\000' != *pcText)
			++pcText;
		if (isspace(*pcText)) {
			*pcText++ = '\000';
		}
		while (isspace(*pcText))
			++pcText;
	}
	return pcText;
}

/*
 * test driver
 */
int
main(argc, argv)
int argc;
char **argv;
{
	auto char acLine[100];
	auto char *pcGlob, *pcPath, *pcAns, *pcComm;

	switch (argc) {
	case 1:
		break;
	case 2:
		if (NULL == freopen(argv[1], "r", stdin)) {
			(void)fprintf(stderr, "%s: freopen(%s): %s\n", argv[0], argv[1], strerror(errno));
			exit(1);
		}
		break;
	default:
		(void)fprintf(stdout, "%s: usage [file]\n", argv[0]);
		exit(1);
	}

	while (NULL != gets(acLine)) {
		pcGlob = acLine;
		while (isspace(*pcGlob))
			++pcGlob;
		if ('#' == *pcGlob || '\000' == *pcGlob)
			continue;
		pcComm = Crack(pcGlob, & pcGlob, & pcPath, & pcAns, (char **)0);
		if (SamePath(pcGlob, pcPath) != ('y' == *pcAns || 'Y' == *pcAns)) {
			printf("%-16s %-16s %-4s %s\n", pcGlob, pcPath, pcAns, (char *)0 == pcComm ? "" : pcComm);
		}
	}
	exit(0);
}

#endif	/* test driver code */
