/* $Id: old.h,v 8.0 1994/09/09 09:06:38 ksb Dist $
 */
extern char acOLD[];

#if HAVE_PROTO
extern void OldCk(int, char **, CHLIST *);
extern int FilterOld(int, char **, CHLIST *);
#else
extern void OldCk();
extern int FilterOld();
#endif
