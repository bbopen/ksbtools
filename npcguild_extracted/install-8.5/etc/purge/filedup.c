/*
 * $Id: filedup.c,v 8.2 1997/11/11 18:02:56 ksb Exp $
 * keep track of unique inode/dev pairs for me				(ksb)
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>

#include "machine.h"
#include "filedup.h"

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#if USE_MALLOC_H
#include <malloc.h>
#else
#if NEED_MALLOC_EXTERN
extern char *malloc();
#endif
#endif

static AVL *AVpAVInsert = nilAV;	/* used by AVInsert & AVVerify()*/
static AE_ELEMENT *AVpAEInsert = nilAE;
static struct stat *AEpstCur;
static char *AEpcName;

static void
AEInit(pAE)
AE_ELEMENT *pAE;
{
	pAE->kctime = AEpstCur->st_ctime;
	pAE->mydev = AEpstCur->st_dev;
	pAE->myino = AEpstCur->st_ino;
	pAE->pcname = strcpy(malloc(strlen(AEpcName)+1),AEpcName);
}

static int
AECmp(pAE)
AE_ELEMENT *pAE;
{
	register int cmp;
	if (0 == (cmp = pAE->mydev - AEpstCur->st_dev)) {
		if (0 == (cmp = pAE->myino - AEpstCur->st_ino)) {
			return 0;
		}
	}
	return cmp;
}

/*
 * make an AVL tree empty						(ksb)
 *
 * in most code "AVL *pAVRoot = nilAV;" is better than calling this routine
 */
void
AVInit(ppAV)
AVL **ppAV;
{
	*ppAV = nilAV;
}

/*
 * Fix an AVL tree node that is in violation of the rules		(dsg)
 * calls look like:
 *     AV_rotate(ppAVRoot, AV_LCHILD)
 *     AV_rotate(ppAVRoot, AV_RCHILD)
 * to fix nodes that are left tipped, and right tipped respectivly
 *
 * return
 *	1: if the tree is shorter after the rotate (always true for insert)
 *	0: if same depth
 */
static int
AV_rotate(ppAVRoot, fSide)
AVL **ppAVRoot;
register int fSide;
{
	register AVL *pAVCopy, *pAVFirst, *pAVSecond;
	register int fOther;

	pAVCopy = *ppAVRoot;
	pAVFirst = pAVCopy->AVsbpAVchild[fSide];

	fOther = 1^fSide;
						/* Three point rotate	*/
	if (fSide == pAVFirst->AVbtipped) {
		pAVCopy->AVbtipped = AV_BAL_CENTER;
		pAVFirst->AVbtipped = AV_BAL_CENTER;

		*ppAVRoot = pAVFirst;
		pAVCopy->AVsbpAVchild[fSide] = pAVFirst->AVsbpAVchild[fOther];
		pAVFirst->AVsbpAVchild[fOther] = pAVCopy;

						/* Five point rotate	*/
	} else if (AV_BAL_CENTER != pAVFirst->AVbtipped) {
		pAVSecond = pAVFirst->AVsbpAVchild[fOther];

		if (AV_BAL_CENTER == pAVSecond->AVbtipped) {
			pAVCopy->AVbtipped = AV_BAL_CENTER;
			pAVFirst->AVbtipped = AV_BAL_CENTER;
		} else if (fSide == pAVSecond->AVbtipped) {
			pAVCopy->AVbtipped = fOther;
			pAVFirst->AVbtipped = AV_BAL_CENTER;
		} else {
			pAVCopy->AVbtipped = AV_BAL_CENTER;
			pAVFirst->AVbtipped = fSide;
		}
		pAVSecond->AVbtipped = AV_BAL_CENTER;
		*ppAVRoot = pAVFirst->AVsbpAVchild[fOther];
		pAVFirst->AVsbpAVchild[fOther] = pAVSecond->AVsbpAVchild[fSide];
		pAVSecond->AVsbpAVchild[fSide] = pAVCopy->AVsbpAVchild[fSide];
		pAVCopy->AVsbpAVchild[fSide] = pAVSecond->AVsbpAVchild[fOther];
		pAVSecond->AVsbpAVchild[fOther] = pAVCopy;

	/*
	 * Insert: we shouldn't be here
	 * Delete: a 3-point or a 5-point rotate works here, so we
	 * choose the less expensive one.  Also, this is the case
	 * where the balance factors come out skewed, and the tree
	 * doesn't shrink.
	 */
	} else {
		pAVCopy->AVbtipped = fSide;
		pAVFirst->AVbtipped = fOther;

		*ppAVRoot = pAVFirst;
		pAVCopy->AVsbpAVchild[fSide] = pAVFirst->AVsbpAVchild[fOther];
		pAVFirst->AVsbpAVchild[fOther] = pAVCopy;

		return 0;
	}
	return 1;
}

/*
 * find a node keyed on AVpAEInsert -- set AVpAVInsert to inserted	(dsg)
 * element if fAdd
 *
 * calling sequence:
 *	{ AECmp must know the "current key now" }
 *	AVInsert(& pAVRoot, fAdd);
 *
 * result:
 *	AVpAVInsert is inserted element (don't touch!)
 *	AVpAEInsert points to user data in the record
 *
 * returns:
 *	true if tree became deeper
 */
static int
AVInsert(ppAVRoot, fAdd)
AVL **ppAVRoot;
int fAdd;
{
	register AVL *pAVCopy;
	auto int fCmp;

	pAVCopy = *ppAVRoot;
	if (nilAV == pAVCopy) {
		if (fAdd) {
			AVpAVInsert = *ppAVRoot = (AVL *)malloc(sizeof(AVL));
			AVpAVInsert->AVbtipped = AV_BAL_CENTER;
			AVpAVInsert->AVsbpAVchild[AV_LCHILD] = nilAV;
			AVpAVInsert->AVsbpAVchild[AV_RCHILD] = nilAV;
			AVpAEInsert = & AVpAVInsert->AE_data;
			AEInit(AVpAEInsert);
			return 1;
		}
		AVpAEInsert = nilAE;
		AVpAVInsert = nilAV;
	} else if (0 == (fCmp = AECmp(& pAVCopy->AE_data))) {
		AVpAVInsert = pAVCopy;
		AVpAEInsert = & AVpAVInsert->AE_data;
	} else if (fCmp < 0) {
		if (AVInsert(& pAVCopy->AVsbpAVchild[AV_LCHILD], fAdd)) {
			switch (pAVCopy->AVbtipped) {
			case AV_BAL_LEFT:
				(void)AV_rotate(ppAVRoot, AV_LCHILD);
				break;
			case AV_BAL_CENTER:
				pAVCopy->AVbtipped = AV_BAL_LEFT;
				return 1;
			case AV_BAL_RIGHT:
				pAVCopy->AVbtipped = AV_BAL_CENTER;
				break;
			}
		}
	} else if (AVInsert(& pAVCopy->AVsbpAVchild[AV_RCHILD], fAdd)) {
		switch (pAVCopy->AVbtipped) {
		case AV_BAL_RIGHT:
			(void)AV_rotate(ppAVRoot, AV_RCHILD);
			break;
		case AV_BAL_CENTER:
			pAVCopy->AVbtipped = AV_BAL_RIGHT;
			return 1;
		case AV_BAL_LEFT:
			pAVCopy->AVbtipped = AV_BAL_CENTER;
			break;
		}
	}
	return 0;
}

#if 0
/*
 * a prototype function for AVScan					(ksb)
 */
int
AV_prototype(pAE)
AE_ELEMENT *pAE;
{
	/* bar(pAE->foo); */
	return 0;		/* compatible with MICE			*/
}
#endif /* example */

/*
 * scan an avl tree inorder calling pFunc with each each node		(ksb)
 *
 * the function passed should return non-zero for stop (MICE)
 */
int
AVScan(pAVRoot, pFunc)
AVL *pAVRoot;
int (*pFunc)();			/* prototype AV_prototype		*/
{
	register int r;

	while (nilAV != pAVRoot) {
		if (0 != (r = AVScan(pAVRoot->AVsbpAVchild[AV_LCHILD], pFunc)))
			return r;
		if (0 != (r = (*pFunc)(& pAVRoot->AE_data)))
			return r;
		pAVRoot = pAVRoot->AVsbpAVchild[AV_RCHILD];
	}
	return 0;
}


#if 0
/*
 * return name for this node in tree					(ksb)
 */
char *
FDName(ppAV, pst)
AVL **ppAV;
struct stat *pst;
{
	/* see if it was the last one we found
	 */
	if (nilAE != AVpAEInsert && pst->st_ino == AVpAEInsert->myino &&
	     pst->st_dev == AVpAEInsert->mydev)
		return AVpAEInsert->pcname;

	AEpstCur = pst;
	(void)AVInsert(ppAV, 0);
	AEpstCur = (struct stat *)0;

	if (nilAE != AVpAEInsert)
		return AVpAEInsert->pcname;
	return (char *)0;
}
#endif

/*
 * return name that is in tree						(ksb)
 */
char *
FDAdd(ppAV, pcName, pst)
AVL **ppAV;
struct stat *pst;
char *pcName;
{
	AEpstCur = pst;
	AEpcName = pcName;
	(void)AVInsert(ppAV, 1);
	AEpcName = (char *)0;
	AEpstCur = (struct stat *)0;
	return AVpAEInsert->pcname;
}

#if TEST
char *progname = "frtest";

int
main(argc, argv)
int argc;
char **argv;
{
	auto char acFile[MAXPATHLEN+2];
	auto struct stat stBuf;
	register char *pcEnd;
	auto AVL *pAV;
	extern char *strchr();

	AVInit(& pAV);

	while ((char *)0 != fgets(acFile, MAXPATHLEN+1, stdin)) {
		if ((char *)0 == (pcEnd = strchr(acFile, '\n'))) {
			fprintf(stderr, "%s: name too long\n", progname);
			exit(1);
		}
		*pcEnd = '\000';
		if (-1 == lstat(acFile, &stBuf)) {
			fprintf(stderr, "%s: stat: %s: %s\n", progname, acFile, strerror(errno));
			continue;
		}
		printf("%s %s\n", acFile, FDAdd(&pAV, acFile, &stBuf));
	}
	exit(0);
}
#endif
