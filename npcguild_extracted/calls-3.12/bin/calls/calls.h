/*
 * $Id: calls.h,v 3.2 1991/06/23 14:00:26 ksb Dist $
 *
 * calls   print out the calling struture of a C program	(ksb)
 */

#define MAXDEPTH	99		/* max output depth level	*/
#define PAPERWIDTH	96		/* limits tabbing		*/
#define TABWIDTH	8		/* width of a \t		*/

#define LOOK_FUNCS	0
#define LOOK_VARS	1

typedef struct CLnode {
	struct CLnode *pCLnext;
	struct HTnode *pHTlist;
} LIST;
#define nilCL	((LIST *) 0)
#define newCL()	((LIST *)malloc(sizeof(LIST)))

extern char acCmd[];
extern int bAll, bReverse, fLookFor;
extern char *pcProg;
extern void Dostdin(), Process();

extern char *strchr(), *strcat();
