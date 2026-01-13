/*
 * manage a data structure that keeps the modes of a list of values
 * $Id: maxfreq.h,v 8.0 1996/03/13 15:26:58 kb207252 Dist $
 *
 * Assumptions:
 *	there aren't many different values (usually less than 16)
 *	long chains of the same number are common
 */

/*
 * test configuration
 */
#ifdef TEST
#define MAXBUF	100

typedef union MEnode {
	char ac[MAXBUF];
} ME_ELEMENT;

extern int MECopy();
extern int MECompare();
extern char *progname;
extern int main();
#endif /* TEST	*/

/*
 * this is the data structure used for keeping the max frequency
 *
 * The algorithm to maintain it is O(n^2) / K; we try to keep K large
 * by predicting that the input contains long repeating sequences of
 * the same key.
 */
typedef struct MFnode {
	struct MFnode *pMFlower;/* list of nodes with lower freq's	*/
	struct MFnode *pMFequal;/* list of nodes with equal freq's	*/
	ME_ELEMENT ME;
	int icount;
} MAXFREQ;
#define nilMF	((MAXFREQ *)0)

extern ME_ELEMENT *MFIncrement();
/* extern void MFFree(); */
extern int MFCheckMax(), MFScan();
