/*@Header@*/
/*@Version $Revision: 6.2 $@*/
/* $Id: srtunq.h,v 6.2 2010/02/26 21:30:39 ksb Exp $
 * include file for memory resident unique sorting routines.
 */

/* database entry */
typedef struct srtbl {
	struct srtbl *srt_prev;		/* parent			*/
	struct srtbl *srt_less;		/* something < srt_str		*/
	struct srtbl *srt_more;		/* something > srt_str		*/
	char srt_str[1];		/* dynamic: 1 for null at EOS	*/
} SRTENTRY;

/* database tag */
typedef struct srtent {
	SRTENTRY *srt_top;		/* root of the tree		*/
	SRTENTRY *srt_next;		/* pointer for srtget		*/
} SRTTABLE;

extern void
	srtinit(),			/* init for srtin		*/
	srtfree(),			/* free a database		*/
	srtdtree(),			/* recursive delete of subtree	*/
	srtgti();			/* init for srtgets		*/
extern char
	*srtin(),			/* insert string - return err	*/
	*srtmem(),			/* string is a member of tree	*/
	*srtgets();			/* get next string		*/
extern int
	srtdel(),			/* delete from a unique list	*/
	srtapply(),			/* scan the tree		*/
	srtapply2();			/* scan the tree with a state	*/
