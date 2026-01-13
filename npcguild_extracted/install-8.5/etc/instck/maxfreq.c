/*
 * Copyright 1990 Purdue Research Foundation, West Lafayette, Indiana
 * 47907.  All rights reserved.
 *
 * Written by D. Scott Guthridge, aho@cc.purdue.edu			(dsg)
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
 */

/*
 * $Compile(test): ${cc-cc} ${cc_debug--g} -DTEST -o %F %f
 * $Id: maxfreq.c,v 8.2 1997/02/15 21:12:47 ksb Exp $
 * @(#)
 */

/*@Header		@*/
#include <stdio.h>		/* for stderr		*/
#include "machine.h"
#include "configure.h"
#include "main.h"
#include "gen.h"
#include "maxfreq.h"

#if STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

/*@Explode config	@*/
#ifdef TEST

/*
 * copy an ME element
 *
 * This happens only when a new element is added to the
 * cache. Return non-zero on failure.
 */
int
MECopy(pMEDest, pMESource)
	ME_ELEMENT *pMEDest, *pMESource;
{
	(void)strcpy(pMEDest->ac, pMESource->ac);
	return 0;
}

/*
 * compare two ME elements return 0 if they are equal; non-zero if they
 * are not
 */
int
MECompare(pME1, pME2)
	ME_ELEMENT *pME1, *pME2;
{
	return strcmp(pME1->ac, pME2->ac);
}
#endif /* TEST	*/

/*@Explode inc		@*/
#if defined(TEST)
extern char *malloc();
#endif

/*
 * insert/search for a node then increment the count
 * (this often means moving it to preserve the lower/equal rules)
 *
 * returns the user-defined part of the node it finds or
 * (ME_ELEMENT *)0 if the copy or malloc fails
 */
ME_ELEMENT *
MFIncrement(ppMFRoot, pME)
	MAXFREQ **ppMFRoot;
	ME_ELEMENT *pME;
{
	register MAXFREQ *pMF;
	register MAXFREQ **ppMFAnchor;
	register MAXFREQ *pMFColumn;
	register MAXFREQ **ppMFOneBack;
	register MAXFREQ **ppMFTwoBack;

	ppMFTwoBack = (MAXFREQ **)0;
	ppMFOneBack = ppMFRoot;
	ppMFAnchor = ppMFRoot;
	for (pMFColumn = *ppMFRoot; nilMF != pMFColumn; pMFColumn = pMFColumn->pMFlower) {
		for (pMF = pMFColumn; nilMF != pMF; pMF = pMF->pMFequal) {
			if (0 == MECompare(pME, & pMF->ME)) {
				/*
				 * remove this one even though it may
				 * be in the right place -- it's too
				 * expensive to find out
				 */
				++pMF->icount;
				if (nilMF != pMF->pMFequal) {
					*ppMFAnchor = pMF->pMFequal;
					pMF->pMFequal->pMFlower = pMF->pMFlower;
				} else {
					*ppMFAnchor = pMF->pMFlower;
				}
				if ((MAXFREQ **)0 == ppMFTwoBack) {
					pMF->pMFlower = *ppMFRoot;
					pMF->pMFequal = nilMF;
					*ppMFRoot = pMF;
					return & pMF->ME;
				}
				if ((*ppMFTwoBack)->icount == pMF->icount) {
					pMF->pMFlower = (*ppMFTwoBack)->pMFlower;
					pMF->pMFequal = *ppMFTwoBack;
					(*ppMFTwoBack)->pMFlower = nilMF;
					*ppMFTwoBack = pMF;
					return & pMF->ME;
				}
				pMF->pMFlower = *ppMFOneBack;
				pMF->pMFequal = nilMF;
				*ppMFOneBack = pMF;
				return & pMF->ME;
			}
			ppMFAnchor = & pMF->pMFequal;
		}
		ppMFAnchor = & pMFColumn->pMFlower;
		ppMFTwoBack = ppMFOneBack;
		ppMFOneBack = & pMFColumn->pMFlower;
	}
	/*
	 * not found -- push a new element on the top of the last column
	 *
	 * assert(nilMF == pMFColumn);
	 * assert(nilMF == pMF);
	 */
	if (nilMF == (pMF = (MAXFREQ *)malloc(sizeof(MAXFREQ)))) {
		return (ME_ELEMENT *)0;
	}
	pMF->pMFlower = nilMF;
	if (0 != MECopy(& pMF->ME, pME)) {
		return (ME_ELEMENT *)0;
	}
	pMF->icount = 1;
	if ((MAXFREQ **)0 != ppMFTwoBack && 1 == (*ppMFTwoBack)->icount) {
		pMF->pMFequal = *ppMFTwoBack;
		*ppMFTwoBack = pMF;
		return & pMF->ME;
	}
	pMF->pMFequal = nilMF;
	*ppMFOneBack = pMF;
	return & pMF->ME;
}

/*@Explode free	@*/
/* 
 * free a maxfreq cache
 */
static void
MFFree(pMFRoot)
	MAXFREQ *pMFRoot;
{
	register MAXFREQ *pMF;
	register MAXFREQ *pMFFree;
	register MAXFREQ *pMFColumn;

	for (pMFColumn = pMFRoot; nilMF != pMFColumn; pMFColumn = pMFColumn->pMFlower) {
		for (pMF = pMFColumn; nilMF != pMF; /*EMPTY*/) {
			pMFFree = pMF;
			pMF = pMF->pMFequal;
			free((char *)pMFFree);
		}
	}
}

/*@Explode check	@*/
/*
 * determine whether a value is among those with the highest
 * frequency
 *
 * return 0 for no; non-zero for yes.
 */
int
MFCheckMax(pMFRoot, pME)
	register MAXFREQ *pMFRoot;
	ME_ELEMENT *pME; 
{
	while (nilMF != pMFRoot) {
		if (0 == MECompare(pME, & pMFRoot->ME)) {
			return 1;
		}
		pMFRoot = pMFRoot->pMFequal;
	}
	return 0;
}

/*@Explode scan	@*/
/*
 * scan over the nodes of a maxfreq cache
 *
 * Stop the scan and return the non-zero value if the given function
 * returns non-zero.
 */
int
MFScan(pMFRoot, pfi)
	MAXFREQ *pMFRoot;
	int (*pfi)();
{
	register MAXFREQ *pMF;
	register int iRet;
	register MAXFREQ *pMFColumn;

	for (pMFColumn = pMFRoot; nilMF != pMFColumn; pMFColumn = pMFColumn->pMFlower) {
		for (pMF = pMFColumn; nilMF != pMF; pMF = pMF->pMFequal) {
			if (0 != (iRet = (*pfi)(pMF))) {
				return iRet;
			}
		}
	}
	return 0;
}

/*@Explode test		@*/
#ifdef TEST
char *progname;

/*
 * print routine prototyped for MFScan					(dsg)
 */
static int
PrintNode(pMF)
	MAXFREQ *pMF;
{
	(void)printf("%7d %s", pMF->icount, pMF->ME.ac);
	return 0;
}

/* a driver								(dsg)
 * do maxfreq computation on small strings (group names, for example)
 */
int
main(argc, argv)
	int argc;
	char **argv;
{
	auto ME_ELEMENT ME;
	auto MAXFREQ *pMFRoot;

	progname = argv[0];
	pMFRoot = nilMF;
	while ((char *)0 != fgets(& ME.ac[0], MAXBUF, stdin)) {
#ifdef TESTCHECK
		(void)printf("before insertion element is ");
		if (0 == MFCheckMax(pMFRoot, & ME)) {
			(void)printf("not ");
		}
		(void)printf("among those with highest freq\n");
#endif /* TESTCHECK	*/
		(void)MFIncrement(& pMFRoot, & ME);
	}
	(void)MFScan(pMFRoot, PrintNode);
	MFFree(pMFRoot);

	exit(0);
}
#endif /* TEST	*/
