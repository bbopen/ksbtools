/* $Id: magic.h,v 8.0 1994/09/09 09:07:24 ksb Dist $
 */

#if HAVE_PROTO
extern char *BadLoad(int, int);
extern int IsStripped(int, struct stat *);
extern int IsRanlibbed(int, struct stat *);
#else
extern char *BadLoad();
extern int IsStripped();
extern int IsRanlibbed();
#endif
