/* $Id: list.h,v 8.0 1997/01/29 02:41:57 ksb Exp $
 * keep a list of strings, maybe uniq'd or not				(ksb)
 * @Key: LI/List@
 */

#ifdef documentation
/*
Declare a buffer of type (LIST), then call the Init routine with the
address of the buffer and a `discard unique' flag (0 == keep all, 1 == keep
only unique lines.

Add strings to the end with Add, add (char *)0 if you wish (those are
always unique, BTW.

Get the whole list with Cur, optionally get the number of entries.

Free with Free(& LI, free).

Make/Close not implemented.
 */
#endif

typedef unsigned int LIST_INDEX;

#if !defined(LIST_KICK)
#define LIST_KICK	12
#endif

typedef struct LInode {
	char **ppclist;			/* strings accepted		*/
	LIST_INDEX *puinext;		/* previous string w/ this first*/
	LIST_INDEX uiend;		/* number of entries		*/
	LIST_INDEX uimax;		/* number of slots		*/
} LIST;

extern char **ListCur(/* pLI, puiN */);
extern void ListInit(/* pLI, fUniq */);
extern void ListAdd(/* pLI, pcline */);
extern int ListReplace(/* pLI, pcline, uiWhere */);
extern void _ListGrow(/* puiN */), (*ListGrow)(/* puiN */);
extern void ListFree(/* pLI, void (*)(char *) */);

#define ListStart()	0		/* MICE				*/
#define ListEnd()	0
