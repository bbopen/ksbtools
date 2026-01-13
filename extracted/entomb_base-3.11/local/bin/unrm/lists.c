/* $Id: lists.c,v 3.2 1994/07/11 00:15:36 ksb Exp $
 */
#include <sys/types.h>
#include "lists.h"


/*
 * SLInsert - insert the given element into the list at the point specified
 *	by the given function.
 */
void
SLInsert(ppLElist, pLEelem, func)
LE **ppLElist, *pLEelem;
int (*func)(/* pLE, pLE */);
{
	register int i;

	for (;;) {
		if (nilLE == *ppLElist) {
			*ppLElist = pLEelem;
			pLEelem->next = nilLE;
			return;
		}
		i = (*func)(*ppLElist, pLEelem);
		if (i < 0) {
			/* advance */
			ppLElist = & (*ppLElist)->next;
			continue;
		}
		pLEelem->next = *ppLElist;
		*ppLElist = pLEelem;
		return;
	}
}


/* the function that should be the default for Retrieve and Delete	(ksb)
 */
int
namecmp(pLE, pchFile)
LE *pLE;
char *pchFile;
{
	return strcmp(pLE->name, pchFile);
}

/* Return a pointer to the element indicated by cmpfunc, or	    (mjb/ksb)
 * nilLE if no such element can be found.  This function had a
 * really bad interface (error prone) so I fixed it -- ksb
 */
LE *
Retrieve(pLElist, cmpfunc, pcCmpTo)
LE *pLElist;
int (*cmpfunc)(/* pLE, pcCmp */);
char *pcCmpTo;
{
	if ((int (*)())0 == cmpfunc) {
		cmpfunc = namecmp;
	}
	while (nilLE != pLElist) {
		if (0 == (*cmpfunc)(pLElist, pcCmpTo))
			return pLElist;
		pLElist = pLElist->next;
	}
	return nilLE;
}

/* these function make sure we only visit a file once per command	(mjb)
 */
void
Mark(pLE)
LE *pLE;
{
	pLE->mark = 1;
}


/* tell if this file is already marked by this pass			(mjb)
 */
int
IsMarked(pLE)
LE *pLE;
{
	return pLE->mark;
}

/* abort this mark -- the file was not really OK			(mjb)
 */
void
UnMark(pLE)
LE *pLE;
{
	pLE->mark = 0;
}


/* allow the marked files to be considered next time			(ksb)
 */
void
ClearMarks(pLE)
register LE *pLE;
{
	while (nilLE != pLE) {
		pLE->mark = 0;
		pLE = pLE->next;
	}
}

/* remove the marked files from the list of files in the crypt		(ksb)
 */
void
StrikeMarks(ppLE)
register LE **ppLE;
{
	register LE *pLECur;

	while (nilLE != (pLECur = *ppLE)) {
		if (pLECur->mark) {
			*ppLE = pLECur->next;
		} else {
			pLECur->mark = 0;;
			ppLE = & pLECur->next;
		}
	}
}
