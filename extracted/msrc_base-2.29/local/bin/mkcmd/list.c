/* $Id: list.c,v 8.1 1997/10/13 16:20:12 ksb Exp $
 * keep a list of strings in the order they were presented		(ksb)
 * duplicated strings may be suppressed with a flag on the init call
 *
 * $Compile(TEST): ${cc-cc} ${cflags--g} -DTEST %f -o %F
 */
#include <sys/types.h>
#include <stdio.h>

#include "machine.h"
#include "main.h"
#include "list.h"
#include "mkcmd.h"

#if USE_SYS_PARAM_H
#include <sys/param.h>
#endif

#if USE_MALLOC_H
#include <malloc.h>
#else
#if NEED_MALLOC_EXTERN
extern char *malloc(), *calloc(), *realloc();
extern void free();
#endif
#endif

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

extern char *progname;

/* emulate the mice environment						(ksb)
 */
static void
OutOfMem(pcMesg)
char *pcMesg;
{
	fprintf(stderr, "%s: %s: out of memory\n", progname, pcMesg);
	exit(MER_LIMIT);
}

#if !HAVE_STRDUP
/*
 * save a string in malloced memory					(ksb)
 */
char *
strdup(pc)
char *pc;
{
	register char *pcRet;

	if ((char *)0 == (pcRet = malloc(((unsigned)strlen(pc)|3)+1))) {
		OutOfMem("strdup: malloc");
	}
	return strcpy(pcRet, pc);
}
#endif

/* add elements to the list in chunks, this sizes the new guess		(ksb)
 */
void
_ListGrow(puiN)
LIST_INDEX *puiN;
{
	if (0 == *puiN) {
		*puiN = LIST_KICK;
	} else {
		*puiN *= 5;
		*puiN /= 3;
	}
}

/* this function pointer set the grow stratagy				(ksb)
 */
void (*ListGrow)() =
	_ListGrow;

/* set up for a list							(ksb)
 */
void
ListInit(pLI, fUniq)
LIST *pLI;
int fUniq;
{
	pLI->uiend = 0;
	pLI->uimax = 0;
	(*ListGrow)(& pLI->uimax);
	pLI->ppclist = (char **)calloc(pLI->uimax, sizeof(char *));
	if ((char **)0 == pLI->ppclist) {
		OutOfMem("calloc: list");
	}
	if (fUniq) {
		pLI->puinext = (LIST_INDEX *)calloc(pLI->uimax, sizeof(LIST_INDEX));
		if ((LIST_INDEX *)0 == pLI->puinext) {
			OutOfMem("calloc: uniq");
		}
	} else {
		pLI->puinext = (LIST_INDEX *)0;
	}
}

/* return the list as it stands now					(ksb)
 */
char **
ListCur(pLI, puiN)
LIST *pLI;
LIST_INDEX *puiN;
{
	if ((LIST_INDEX *)0 != puiN) {
		*puiN = pLI->uiend;
	}
	return pLI->ppclist;
}

/* append/add a string to the list					(ksb)
 * N.B.: nil slots are *always* added
 */
void
ListAdd(pLI, pcAdd)
LIST *pLI;
char *pcAdd;
{
	register LIST_INDEX uiPos, ui;
	register char *pc;

	uiPos = pLI->uiend;	/* shut up gcc -Wall */
	if ((LIST_INDEX *)0 != pLI->puinext && (char *)0 != pcAdd) {
		for (ui = pLI->uiend; ui > 0; ) {
			pc = pLI->ppclist[ui-1];
			if ((char *)0 == pc || pc[0] != pcAdd[0]) {
				--ui;
				continue;
			}
			break;
		}
		if (0 != ui) {
			register LIST_INDEX uiLast;

			uiPos = --ui;
			do {
				uiLast = ui;
				if (0 == strcmp(pcAdd, pLI->ppclist[ui])) {
					/* XXX should be DisposeData? */
					return;
				}
			} while (uiLast != (ui = pLI->puinext[ui]));
		}
	}

	/* do we more space?
	 */
	if (pLI->uiend == pLI->uimax) {
		(*ListGrow)(& pLI->uimax);
		pLI->ppclist = (char **)realloc((void *)pLI->ppclist, sizeof(char *)*pLI->uimax);
		if ((char **)0 == pLI->ppclist) {
			OutOfMem("realloc: list");
		}

		if ((LIST_INDEX *)0 != pLI->puinext) {
			pLI->puinext = (LIST_INDEX *)realloc((void *)pLI->puinext, sizeof(LIST_INDEX)*pLI->uimax);
			if ((LIST_INDEX *)0 == pLI->puinext) {
				OutOfMem("realloc: uniq");
			}
		}
	}

	/* add element */
	if ((LIST_INDEX *)0 != pLI->puinext) {
		pLI->puinext[pLI->uiend] = uiPos;
	}
	pLI->ppclist[pLI->uiend++] = pcAdd;
}

/* We need to replace a line in the list, take care of			(ksb)
 * uniq links and such, if they are on.
 *
 * If the line is a duplicate of a line before the replacement
 * point replace with (char *)0,
 * if the line is a duplicate of a line after it, replace that one
 * with (char *)0
 *
 * If DisposeData is defined call it on any non-nil pointer that is over
 * written.
 *
 * return 0 for OK, 1 for duplicate, -1, for fail
 *
 * N.B. no one said the back/next link *had* to be monotonicly decreasing.
 * (other list routines make them so, but we do not have to, as long as
 * the biggest one in the list is the head of the list.
 */
int
ListReplace(pLI, pcReplace, uiWhere)
LIST *pLI;
char *pcReplace;
LIST_INDEX uiWhere;
{
	register char *pc, *pcHad;
	register LIST_INDEX uiNext, uiOld;
	register LIST_INDEX uiInsert, uiHead;

	if (uiWhere >= pLI->uiend) {
		return -1;
	}

	/* do the work, return if not unique'd or the work was nothing
	 * on same unique list
	 */
	pcHad = pLI->ppclist[uiWhere];
	if ((LIST_INDEX *)0 == pLI->puinext || pcHad == pcReplace) {
		pLI->ppclist[uiWhere] = pcReplace;
		return 0;
	}

	/* record the old next pointer to put in before us
	 */
	uiOld = pLI->puinext[uiWhere];

	/* find the linked lists to delete from
	 */
	if ((char *)0 != pcHad) {
		register LIST_INDEX uiDel;

		for (uiDel = pLI->uiend; uiDel > 0; ) {
			pc = pLI->ppclist[uiDel-1];
			if ((char *)0 == pc || pc[0] != pcHad[0]) {
				--uiDel;
				continue;
			}
			break;
		}
		/* we have problems, can't find the string in
		 * the list! (We know it is there.) XXX
		 */
		if (0 == uiDel) {
			return 1;
		}
		/* if the head of the list is the one we do not
		 * have to replace a next pointer
		 */
		if (--uiDel != uiWhere) {
			while (uiWhere != (uiNext = pLI->puinext[uiDel])) {
				 uiDel = uiNext;
			}
			if (uiWhere == uiOld)
				pLI->puinext[uiDel] = uiDel;
			else
				pLI->puinext[uiDel] = uiOld;
		}
	}

	/* insert the new one in the back list
	 */
	pLI->ppclist[uiWhere] = pcReplace;
	if ((char *)0 == pcReplace) {
		return 0;
	}

	for (uiInsert = pLI->uiend; uiInsert > 0; ) {
		pc = pLI->ppclist[uiInsert-1];
		if ((char *)0 == pc || pc[0] != pcReplace[0]) {
			--uiInsert;
			continue;
		}
		break;
	}

	/* no others on this list-- put in end mark and return
	 */
	if (0 == uiInsert) {
		pLI->puinext[uiWhere] = uiWhere;
		return 0;
	}

	uiHead = --uiInsert;
	while (uiWhere < (uiNext = pLI->puinext[uiInsert]) && uiNext != uiInsert) {
		uiInsert = uiNext;
	}
	pLI->puinext[uiInsert] = uiWhere;
	pLI->puinext[uiWhere] = (uiNext == uiInsert) ? uiWhere : uiNext;

	/* fix duplicates if we have them
	 */
	uiNext = uiHead;
	do {
		if (uiWhere == (uiHead = uiNext)) {
			continue;
		}
		if (0 == strcmp(pcReplace, pLI->ppclist[uiHead])) {
			if (uiHead < uiWhere) {
				(void)ListReplace(pLI, (char *)0, uiHead);
			} else {
				(void)ListReplace(pLI, (char *)0, uiWhere);
			}
			return 1;
		}
	} while (uiHead != (uiNext = pLI->puinext[uiHead]));

	return 0;
}

#if !defined(MAX_CHARS)
#define	MAX_CHARS 256
#endif

/* sqeeze the (char *)0's out of a list					(ksb)
 *
 * N.B. "pLI->uiend" is not a valid link LIST_INDEX,
 * so we use it to mean "not seen yet".
 */
void
ListCompact(pLI)
LIST *pLI;
{
	register LIST_INDEX uiScan, uiWrite;
	register char *pcCur;
	auto LIST_INDEX auiLast[MAX_CHARS];
	register int cScan;

	for (cScan = 0; cScan < MAX_CHARS; ++cScan) {
		auiLast[cScan] = pLI->uiend;
	}
	uiWrite = 0;
	for (uiScan = 0; uiScan < pLI->uiend; ++uiScan) {
		if ((char *)0 == (pcCur = pLI->ppclist[uiScan])) {
			continue;
		}
		pLI->ppclist[uiWrite] = pcCur;

		/* Update the unique back pointers:
		 * if we've not seen one put in the infinite loop
		 * to mark the end of the chain.
		 */
		if ((LIST_INDEX *)0 != pLI->puinext) {
			if (auiLast[(int)*pcCur] == pLI->uiend)
				pLI->puinext[uiWrite] = uiWrite;
			else
				pLI->puinext[uiWrite] = auiLast[(int)*pcCur];
			auiLast[(int)*pcCur] = uiScan;
		}
		++uiWrite;
	}
	pLI->uiend = uiWrite;
}

/* free the memory in a LI node						(ksb)
 * Pass down (void (*)())0 or `free' to free the memory held
 * by the strings in the list.
 */
void
ListFree(pLI, pfvDispose)
LIST *pLI;
void (*pfvDispose)(/* char * */);
{
	register unsigned int u;

	if ((void (*)())0 == pfvDispose) {
		pfvDispose = free;
	}
	for (u = 0; u < pLI->uiend; ++u) {
		register char *pc;
		pc = pLI->ppclist[u];
		if ((char *)0 != pc) {
			(*pfvDispose)(pc);
		}
	}
	if ((LIST_INDEX *)0 != pLI->puinext) {
		free((void *)pLI->puinext);
		pLI->puinext = (LIST_INDEX *)0;
	}
	if ((char **)0 != pLI->ppclist) {
		free((void *)pLI->ppclist);
		pLI->ppclist = (char **)0;
	}
	pLI->uimax = pLI->uiend = 0;
}

#if TEST
char *progname = "listtest";

/* Test program for simple list functions, should be mkcmd'd but	(ksb)
 * it is part of mkcmd.
 * This version doesn't test all the new code, just the part I use lots.
 */
int
main(argc, argv)
int argc;
char **argv;
{
	auto LIST LI;
	register char **ppc, *pc;
	register int i;

	ListInit(& LI, 0);
	ListAdd(& LI, "one");
	ListAdd(& LI, "two");
	ListAdd(& LI, "three");
	ListAdd(& LI, "four");
	ListAdd(& LI, "five");
	ListAdd(& LI, "six");
	ListAdd(& LI, "four");

	for (i = 1; argc > i; ++i) {
		ListAdd(& LI, argv[i]);
	}

	ListAdd(& LI, (char *)0);

	ppc = ListCur(& LI, (LIST_INDEX *)0);
	while ((char *)0 != (pc = *ppc++)) {
		puts(pc);
	}
	exit(0);
}
#endif
