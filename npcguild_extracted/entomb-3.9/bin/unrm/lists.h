/* $Id: lists.h,v 3.2 1994/07/11 00:15:38 ksb Exp $
 * we use this struct to build a linked list (sorted cronologically
 * by deletion time) of all the files entombed for this user on all
 * the filesystems on which we do entombing.
 */

#if LINUX
#include <time.h>
#endif

typedef struct _le {
	struct _le *next;	/* next in the list		*/
	char *name;		/* name of this file		*/
	long size;		/* filesize in bytes		*/
	time_t date;		/* time entombed		*/
	struct TIStruct *pTI;	/* pointer to tomb info		*/
	short mark;		/* a general-purpose mark	*/
} LE;

#define newLE()	((LE *)malloc(sizeof(LE)))
#define nilLE	((LE *)0)

extern int namecmp();
extern void SLInsert();
extern LE *Retrieve();
extern void Mark();
extern void UnMark();
extern void ClearMarks();
extern void StrikeMarks();
extern int IsMarked();
