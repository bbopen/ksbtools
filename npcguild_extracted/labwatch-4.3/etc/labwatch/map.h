/* $Id: map.h,v 3.1 1994/01/21 15:23:47 bj Beta $
 */

typedef struct MVnode {
	char **ppclines;
	int icur;
	int imax;
} VIEW;

typedef struct MPnode {
	char *pcname;
	char *pctext;
	struct MPnode *pMPnext;
	VIEW MVview;
} MAP;

extern MAP *MapNew(char *, int);
extern void MapLine(MAP *, char *);
extern MAP *pMPList;
extern int iMapNameLens;
