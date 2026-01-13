/* $Id: links.h,v 8.3 1999/06/20 10:53:07 ksb Exp $
 */

/* this structure holds all the info for a link pair
 * the up link can be anywhere on the fs, the old link must be in
 * the $PWD (and OLD dir).  Links in other OLD dirs are in acup's.
 */
typedef struct LInode {
	char etactic;			/* our best tactic		*/
	char acinst[MAXPATHLEN+1];	/* file in parent dir (if any)	*/
	char acback[MAXPATHLEN+1];	/* bad link in OLD, or -1 of BL	*/
	char *pcitail;			/* last slash in acinst		*/
	char *pcbtail;			/* last slash in acback		*/
	char *pcotail;			/* slash before "OLD" in back	*/
	struct stat *pstup;		/* stat buffer for file in ..	*/
	struct stat *pstold;		/* stat buffer for file in OLD	*/
	struct DIRENT **ppDEold;	/* one of the original BLs	*/
	struct DIRENT **ppDEup;		/* we had this info, BTW	*/
} LINK;

extern int FindLinks(/* pcDir, pcOld, ppDEBacks, iBacks, pST, pLI */);
#if VINST
extern int FindMtPt(/* struct stat *, char * */);
extern int SearchHarder(/* char *, struct stat *, char **, int */);
#endif
