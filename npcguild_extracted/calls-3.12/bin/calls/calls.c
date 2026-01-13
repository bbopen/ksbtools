/*
 * $Id: calls.c,v 3.13 1997/04/13 23:02:56 ksb Dist $
 *
 * main.c -- calls mainline, trace calling sequences of C programs	(ksb)
 *
 * Copyright 1990 Purdue Research Foundation, West Lafayette, Indiana
 * 47907.  All rights reserved.
 *
 * Written by Kevin S Braunsdorf, ksb@cc.purdue.edu, purdue!ksb
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company or the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 *
 * 1. Neither the authors nor Purdue University are responsible for any
 *    consequences of the use of this software.
 *
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Credit to the authors and Purdue
 *    University must appear in documentation and sources.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 4. This notice may not be removed or altered.
 *
 *@This is a modified version: ksb continues work on this file after leaving
 *@Purdue. -- ksb
 */
#ifndef lint
static char copyright[] =
"@(#) Copyright 1990 Purdue Research Foundation.\nAll rights reserved.\n";
#endif

#ifdef pdp11
#include <sys/types.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#include "machine.h"
#include "scan.h"
#include "main.h"
#include "calls.h"

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

extern int errno;

/* globals */
char
	acCmd[] = "cmd line";	/* kludge to notify user of error	*/
/* locals */
static LINENO
	iOLines = 0;		/* line number				*/
static char
	acItself[] =		/* normal healthy reverse recursion	*/
		" <<< itself >>>",
	acRecur[] =		/* normal healthy recursion		*/
		" <<< recursive >>>",
	acIsoRecur[] =		/* isolated recursion might be dead code*/
		" <<< isolated recursion >>>";

/* friends */
extern int
	iLineNo;		/* pulled from scanner			*/

/* invoke cpp on file, call Level1					(ksb)
 */
void
Process(pcFilename, outname)
register char *pcFilename, *outname;
{
	extern FILE *popen();
	register char *pcNull;
	register int ret;

	if (access(pcFilename, 04) != 0) {
		(void) fprintf(stderr, "%s: access: %s: %s\n", pcProg, pcFilename, strerror(errno));
		return;
	}
	pcNull = acCppCmd + strlen(acCppCmd);
	(void)strcpy(pcNull, pcFilename);
	if (NULL == (fpInput = popen(acCppCmd, "r"))) {
		(void) fprintf(stderr, "%s: popen: %s: %s\n", pcProg, acCppCmd, strerror(errno));
	} else {
		iLineNo = 0;
		Level1(outname);
		if (0 != (ret = pclose(fpInput))) {
			(void) fprintf(stderr, "%s: `%s' returned %d\n", pcProg, acCppCmd, ret);
		}
	}
	*pcNull = '\000';
}

/* copy stdin to temp file, call Process on file			(ksb)
 */
void
Dostdin()
{
	iLineNo = 1;
	fpInput = stdin;
	Level1("stdin");
	fseek(stdin, 0L, 1);	/* clear end of file */
}

/* check for recursive calls, prevents endless output			(ksb)
 */
int
IsActive(pHT, pCLCheck)
register HASH *pHT;
register LIST *pCLCheck;
{
	while (nilCL != pCLCheck) {
		if (pHT == pCLCheck->pHTlist)
			return nilCL == pCLCheck->pCLnext ? 2 : 1;
		pCLCheck = pCLCheck->pCLnext;
	}
	return 0;
}

/* sort a call level if the customer doesn't like real order		(ksb)
 */
static int
HTOrder(ppINLeft, ppINRight)
INST **ppINLeft, **ppINRight;
{
	return strcasecmp(ppINLeft[0]->pHTname->pcname, ppINRight[0]->pHTname->pcname);
}

/* the user (most likely Christian Marcotte, scoob@vlsi.polymtl.ca)	(ksb)
 * wants to see the descendants in alpha order rather than call order.
 */
INST *
SortEm(pINMix)
INST *pINMix;
{
	register INST *pINScan;
	register int iLen;
	static INST **ppINLens = (INST **)0;
	static int iVec = 0;

	if ((INST *)0 == pINMix || (INST *)0 == pINMix->pINnext) {
		return pINMix;
	}

	pINScan = pINMix;
	for (iLen = 1; (INST *)0 != (pINScan = pINScan->pINnext); ++iLen) {
		/* count them */
	}
	if (iVec < iLen) {
		if (iVec != 0) {
			free((void *)ppINLens);
		}
		iVec = (iLen|7)+1;
		if ((INST **)0 == (ppINLens = (INST **)calloc(iVec, sizeof(INST *)))) {
			fprintf(stderr, "%s: out of memory\n", pcProg);
			iVec = 0;
			return pINMix;
		}
	}
	
	pINScan = pINMix;
	for (iLen = 0; (INST *)0 != pINScan; ++iLen, pINScan = pINScan->pINnext) {
		ppINLens[iLen] = pINScan;
	}
	ppINLens[iLen] = (INST *)0;
	qsort((void *)ppINLens, iLen, sizeof(INST *), HTOrder);
	while (iLen-- > 0) {
		ppINLens[iLen]->pINnext = ppINLens[iLen+1];
	}
	return ppINLens[0];
}

/* output a (sub)tree in pretty form it might be reversed		(ksb)
 */
void
Output(pHTFunc, tabc, iMaxLev, pcRecursion, pCLLast)
register HASH *pHTFunc;
int tabc, iMaxLev;
char *pcRecursion;
LIST *pCLLast;
{
	static char acDashes[] = "\n----------";
	auto LIST LIThis;
	register INST *pINTemp;
	register int i;

	if (bHideExt && (char *)0 == pHTFunc->pcfile) {
		return;
	}
	/* output tsort info
	 */
	if (bTsort) {
		pHTFunc->iline = ++iOLines;
		if (pHTFunc->localp) {
			printf("%s/%s %s/%s\n", pHTFunc->pcname, pHTFunc->pcfile, pHTFunc->pcname, pHTFunc->pcfile);
		} else {
			printf("%s %s\n", pHTFunc->pcname, pHTFunc->pcname);
		}
		for (pINTemp = pHTFunc->pINcalls; pINTemp; pINTemp = pINTemp->pINnext) {
			if (fLookFor != LOOK_VARS && 0 == pINTemp->ffunc)
				continue;
			++iOLines;
			if (pHTFunc->localp) {
				printf("%s/%s ", pHTFunc->pcname, pHTFunc->pcfile);
			} else {
				printf("%s ", pHTFunc->pcname);
			}
			if (pINTemp->pHTname->localp) {
				(void) printf("%s/%s\n", pINTemp->pHTname->pcname, pINTemp->pHTname->pcfile);
			} else {
				(void) printf("%s\n", pINTemp->pHTname->pcname);
			}
			printf("\n");
		}
		tabc = 1;
		goto dump_kids;
	}

	++iOLines;
	(void) printf("\n%5d", iOLines);
	for (i = 0; i < tabc*TABWIDTH; i++ )
		putchar(' ');
	(void) printf("%s", pHTFunc->pcname);
	if (bVerbose || fLookFor == LOOK_VARS) {
		if (-1 != pHTFunc->isrcend) {
			printf("()");
		}
	}
	++tabc;

	if ((char *)0 == pHTFunc->pcfile) {
		if (bVerbose) {
			if (-1 != pHTFunc->isrcend)
				(void) printf(" [extern]");
		}
		return;
	}
	switch (IsActive(pHTFunc, pCLLast)) {
	case 2:
		(void) printf(pcRecursion);
		return;
	case 1:
		(void) printf(acRecur);
		return;
	default:
		break;
	}

	if (pHTFunc->listp) {
		if (bVerbose)
			(void) printf(" (requested)");
		(void) printf(" see below");
		return;
	}
	if (!bVerbose && 0 != pHTFunc->iline) {
		if ((0 == bReverse) ? (0 != pHTFunc->pINcalls) : 0 != pHTFunc->calledp)
			(void) printf(" see line %d", pHTFunc->iline);
		return;
	}

	(void) printf(pHTFunc->localp ?
		(pHTFunc->inclp ? " [static by inclusion from " : " [static in ") :
		(pHTFunc->inclp ? " [included from " : " ["));
	if (0 != pHTFunc->isrcline) {
		(void) printf("%s(%d)]", pHTFunc->pcfile, pHTFunc->isrcline);
	} else {
		(void) printf("%s]", pHTFunc->pcfile);
	}

	if (0 != pHTFunc->iline) {
		if ((0 == bReverse) ? (0 != pHTFunc->pINcalls) : 0 != pHTFunc->calledp)
			(void) printf(" see line %d", pHTFunc->iline);
		return;
	}

	pINTemp = pHTFunc->pINcalls;
	if (0 == pINTemp) {
		pHTFunc->iline = iOLines;
		return;
	}

	if (iMaxLev <= 0) {
		if (pHTFunc->iline != 0) {
		    (void) printf(" see line %d", pHTFunc->iline);
		} else {
		    (void) printf(" see below");
		}
		return;
	}

	LIThis.pHTlist = pHTFunc;
	LIThis.pCLnext = pCLLast;
	if (bVerbose) {
		if (bReverse) {
			(void) printf(" is called by");
		} else {
			(void) printf(" calls");
		}
	}

	if (tabc * TABWIDTH >= iWidth) {
		(void) printf(acDashes);
		tabc = 0;
	}

dump_kids:
	--iMaxLev;
	if (fAlpha) {
		pINTemp = SortEm(pINTemp, (INST *)0);
	}
	for (pHTFunc->iline = iOLines; (INST *)0 != pINTemp; pINTemp = pINTemp->pINnext) {
		if (fLookFor != LOOK_VARS && 0 == pINTemp->ffunc)
			continue;
		Output(pINTemp->pHTname, tabc, iMaxLev, pcRecursion, & LIThis);
	}
	if (0 == tabc) {
		(void) printf(acDashes);
	}
}

/* merge two sorted lists						(ksb)
 */
static void
Merge(ppHTHead, pHTScan, pHTNext)
HASH **ppHTHead;
HASH *pHTScan, *pHTNext;
{
	while ((HASH *)0 != pHTScan && (HASH *)0 != pHTNext) {
		if (0 > strcasecmp(pHTScan->pcname, pHTNext->pcname)) {
			*ppHTHead = pHTScan;
			ppHTHead = & pHTScan->pHTnext;
			pHTScan = *ppHTHead;
			continue;
		}
		*ppHTHead = pHTNext;
		ppHTHead = & pHTNext->pHTnext;
		pHTNext = *ppHTHead;
	}
	/* tack on the end of the list
	 */
	if ((HASH *)0 != pHTScan) {
		*ppHTHead = pHTScan;
	} else {
		*ppHTHead = pHTNext;
	}
}

/* order the hash list in dictonary order				(ksb)
 * that is, fold case in compare (strcasecmp)
 */
ReOrder(ppHTHead)
HASH **ppHTHead;
{
	register HASH *pHTScan, *pHTNext;

	if ((HASH *)0 == (pHTScan = *ppHTHead)) {
		return;
	}
	for (/* nothing */; (HASH *)0 != pHTScan; pHTScan = pHTNext) {
		/* the list is already in order
		 */
		if ((HASH *)0 == (pHTNext = pHTScan->pHTnext))
			return;
		if (0 <= strcasecmp(pHTScan->pcname, pHTNext->pcname))
			break;
	}

	/* split the list at the case change and merge into order
	 */
	pHTScan->pHTnext = (HASH *)0;
	Merge(ppHTHead, *ppHTHead, pHTNext);
}

/* add the functions the user asks for to a list to output later	(ksb)
 */
static LIST **ppCL;

AddFunc(pcName, fFlag, pcFile)
char *pcName, *pcFile;
int fFlag;
{
	register HASH *pHTList;

	pHTList = Search(pcName, fFlag, 1);
	pHTList->listp = 1;
	pHTList->pcfile = pcFile;
	*ppCL = newCL();
	(*ppCL)->pCLnext = 0;
	(*ppCL)->pHTlist = pHTList;
	ppCL = & (*ppCL)->pCLnext;
}

/* parse args, add files, call output
 */
int
main(argc, argv)
int argc;
char **argv;
{
	static LIST *pCLRoot;
	register HASH *pHTList;
	auto HASH *pHTMaster;

	ppCL = & pCLRoot;
	options(argc, argv);
	*ppCL = nilCL;

	while (pCLRoot) {		/* print requested trees	*/
		pCLRoot->pHTlist->listp = 0;
		Output(pCLRoot->pHTlist, 1, iLevels, bReverse ? acItself : acRecur, nilCL);
		putchar('\n');
		pCLRoot = pCLRoot->pCLnext;
	}

	ReOrder(& pHTRoot[0]);
	ReOrder(& pHTRoot[1]);
	Merge(& pHTMaster, pHTRoot[0], pHTRoot[1]);

	if (!bTerse) {			/* print other trees		*/
		for (pHTList = pHTMaster; pHTList; pHTList = pHTList->pHTnext) {
			if (bReverse) {
				if (! pHTList->calledp) {
					/* nothing */;
				}
				if ((LOOK_VARS != fLookFor) && (-1 == pHTList->isrcend)) {
					continue;
				}
			} else if (pHTList->calledp) {
				continue;
			} else if (-1 == pHTList->isrcend) {
				continue;
			}
			if (NULL == pHTList->pcfile || 0 != pHTList->iline) {
				continue;
			}
			Output(pHTList, 1, iLevels, bReverse ? acItself : pHTList->calledp ? acRecur : acIsoRecur, nilCL);
			putchar('\n');
		}
		for (pHTList = pHTMaster; pHTList; pHTList = pHTList->pHTnext) {
			if (!pHTList->intree) {
				continue;
			}
			if (-1 == pHTList->isrcend) {
				continue;
			}
			if ((LOOK_VARS != fLookFor) && -1 == pHTList->isrcend) {
				continue;
			}
			if (NULL == pHTList->pcfile || 0 != pHTList->iline || -1 == pHTList->isrcend) {
				continue;
			}
			Output(pHTList, 1, iLevels, bReverse ? acItself : pHTList->calledp ? acRecur : acIsoRecur, nilCL);
			putchar('\n');
		}
	}

	if (bIndex) {			/* print index			*/
		printf("\fIndex:\n");
		for (pHTList = pHTMaster; pHTList; pHTList = pHTList->pHTnext) {
			if ((LOOK_VARS != fLookFor) && -1 == pHTList->isrcend) {
				continue;
			}
			if (!bExtern && NULL == pHTList->pcfile)
				continue;
			if (bOnly && 0 == pHTList->iline)
				continue;

			if (pHTList->localp) {
				(void) printf("static");
			}
			printf("\t%-16s", pHTList->pcname);
			if (((char *) 0) != pHTList->pcfile) {
				(void) printf(" %s", pHTList->pcfile);
			}
			if (0 != pHTList->isrcline) {
				(void) printf("(%d", pHTList->isrcline);
				if (-1 != pHTList->isrcend)
					(void) printf(",%d", pHTList->isrcend);
				putchar(')');
			}
			if (0 != pHTList->iline && (bReverse || pHTList->isrcend != -1)) {
				(void) printf(" see line %d", pHTList->iline);
			}
			putchar('\n');
		}
	}
	exit(0);
}
