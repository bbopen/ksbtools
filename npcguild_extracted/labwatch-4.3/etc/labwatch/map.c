/* $Id: map.c,v 3.2 1994/01/29 19:45:43 bj Exp $
 * read and keep the over head view of the machines we maintain		(ksb)
 */
#include <stdio.h>
#include <string.h>
#include "machine.h"
#include "main.h"
#include "map.h"

extern char *malloc(), *calloc(), *realloc();

/* save a string in malloc'd memory					(ksb)
 */
char *
strsave(char *pc)
{
	register char *pcMem;

	if ((char *)0 == (pcMem = malloc((strlen(pc)|3)+1))) {
		return (char *)0;
	}
	return strcpy(pcMem, pc);
}

/* setup a view so we can add lines and such				(ksb)
 */
void
ViewInit(pMV)
VIEW *pMV;
{
#if !defined(VIEW_BASE)
#define VIEW_BASE	32
#endif

	pMV->ppclines = (char **)calloc(VIEW_BASE, sizeof(char *));
	pMV->imax = VIEW_BASE;
	pMV->icur = 0;
}

/* add a line to a view							(ksb)
 */
void
ViewLine(VIEW *pMV, char *pcLine)
{
#if !defined(VIEW_EXTEND)
#define VIEW_EXTEND	48
#endif

	if (pMV->icur >= pMV->imax) {
		pMV->imax += VIEW_EXTEND;
		pMV->ppclines = (char **)realloc((char *)pMV->ppclines, pMV->imax * sizeof(char *));
	}
	pMV->ppclines[pMV->icur++] = strsave(pcLine);
}

MAP *pMPList, *pMPFirst;
int iMapNameLens = 3;

/* locate a map by name, or build a new one				(ksb)
 */
MAP *
MapNew(char *pcDescr, int fBuild)
{
	register MAP **ppMP, *pMPScan, *pMPNew;
	register int iC;
	register char *pcText;

	while (isspace(*pcDescr))
		++pcDescr;
	pcText = pcDescr;
	while ('\000' != *pcText && !isspace(*pcText)) {
		if (isupper(*pcText))
			*pcText = tolower(*pcText);
		++pcText;
	}
	while (isspace(*pcText)) {
		*pcText++ = '\000';
	}
	/* `name -- text' is ok
	 */
	if ('-' == pcText[0] && '-' == pcText[1]) {
		*pcText++ = '\000';
		do {
			*pcText++ = '\000';
		} while (isspace(*pcText));
	}
	if ('\000' == pcDescr[0]) {
		fprintf(stderr, "%s: null map name\n", progname);
		return (MAP *)0;
	}
	/* the map `*' is a read-only alias for the first (default) map
	 */
	if ('*' == pcDescr[0] && '\000' == pcDescr[1]) {
		return pMPFirst;
	}

	for (ppMP = & pMPList; (MAP *)0 != (pMPScan = *ppMP); ppMP = & pMPScan->pMPnext) {
		iC = strcmp(pMPScan->pcname, pcDescr);
		if (0 == iC)
			return pMPScan;
		if (0 > iC)
			continue;
		break;
	}
	if (!fBuild)
		return (MAP *)0;
	pMPNew = (MAP *)malloc(sizeof(MAP));
	pMPNew->pMPnext = pMPScan;
	pMPNew->pcname = strsave(pcDescr);
	pMPNew->pctext = '\000' != pcText[0] ? strsave(pcText) : (char *)0;
	ViewInit(& pMPNew->MVview);
	iC = strlen(pcDescr);
	if (iC > iMapNameLens)
		iMapNameLens = iC;
	if ((MAP *)0 == pMPFirst)
		pMPFirst = pMPNew;
	return *ppMP = pMPNew;
}
