/* $Id: read.h,v 1.3 2000/01/30 23:55:01 ksb Exp $
 * sync backup partitions pair structure
 */

typedef struct SBPnode {
	int iline;		/* line number of this side		*/
	char *pcdev;		/* the full device specification	*/
	char *pcmount;		/* the mount point			*/
	char *pcrest;		/* the rest of the original line	*/
	int wflags;		/* flags for the work part		*/
	struct SBPnode *pBPswap;/* the partition we mirror or shadow	*/
	struct SBPnode *pBPnext;/* the next partition in our list	*/
} SBP_ENTRY;

extern int iTableType;		/* 0, for not, 6 for no raw, 7 for raw	*/

#if HAVE_ANSI_EXTERN
extern SBP_ENTRY *ReadFs(char *);
extern void DumpFs(FILE *);
#else
extern SBP_ENTRY *ReadFs();
extern void DumpFs();
#endif
