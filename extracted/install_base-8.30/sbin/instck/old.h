/* $Id: old.h,v 8.1 2001/10/24 01:26:13 ksb Exp $
 */
extern char acOLD[];

#if HAVE_PROTO
extern void OldCk(int, char **, CHLIST *);
extern int FilterOld(int, char **, CHLIST *);
extern int IsEmpty(DIR *);
#else
extern void OldCk();
extern int FilterOld();
extern int IsEmpty();
#endif
