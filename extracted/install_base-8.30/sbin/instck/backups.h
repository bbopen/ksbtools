/* $Id: backups.h,v 8.3 1997/01/12 20:08:51 kb207252 Exp $
 * Do all the work required to check -1 backups in an OLD directory	(ksb)
 * you won't believe how hard this is if you want to handle the most
 * general (and common!) cases.  Usually a Vendor patch script has
 * inadvetently left the file system in a bad state for install/purge.
 *
 * For example a Sun patch moved "vi" to "vi.FCS" and forgot to fix one
 * of links to it (like "edit") either in the same directory or another one.
 * We look hard to repair it.
 */

/* one way to fix a broken backup directory
 */
typedef struct LSnode {
	int wuser;			/* user temp area for selection	*/
	int (*pfiMay)();		/* can we use this one?		*/
	struct DIRENT *(*pfiDo)();	/* function to call		*/
	char *pctext;			/* description			*/
	char *pclong;			/* long help text		*/
} LINK_STRATEGY;

extern struct DIRENT *FixBackup(/* pcDir, pcOld, ppDEBacks, iBacks, pST */);
extern int YesNo(/* fpIn, piAns */);
