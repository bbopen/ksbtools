/*@Version $Revision: 6.0 $@*/
/*@Header@*/
/*
 * $Compile: cc -g -DTEST -o %F %f
 * $Explode: explode -v -o av %f
 * $Id: avl.c,v 6.0 2000/07/30 15:57:11 ksb Exp $
 * @(#)
 *
 * Adel'son-Vel'skii - Landis binary tree routines
 * Kevin Braunsdorf & Scott Guthridge, Copyright 1987
 */
#include <ctype.h>
#include <stdio.h>

/*@Shell %echo ${MACHINE_H+"$MACHINE_H"}%@*/
#include "avl.h"

/*@Explode init@*/
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

/*@Explode fix@*/
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
int
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

/*@Explode ins@*/
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
AVL *AVpAVInsert = nilAV;		/* used by AVInsert & AVVerify()*/
AE_ELEMENT *AVpAEInsert = nilAE;

int
AVInsert(ppAVRoot, fAdd)
AVL **ppAVRoot;
int fAdd;
{
	extern char *malloc();
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

/*@Explode del@*/
/*
 * globals for AV_side()
 * These are used internally for AV_side() and AV_dive() and should not be
 * altered by other routines.
 */
static int fSide;		/* side we are diving on		*/
static int fDive;		/* 'other side' for AVdive		*/
static AVL **ppAVDest;		/* pointer to node AV_side replaces	*/

/*
 * Dive down the tree to find the nearest bottom element on <fSide>.	(dsg)
 * Move it to *ppAVDest then dink the old *ppAVDest.  return 1 if the
 * tree became shorter.
 *
 * Do NOT call "AV_dive" with empty sub-trees.
 */
static int
AV_dive(ppAVRoot)
AVL **ppAVRoot;
{
	register AVL *pAVCopy;
	register AVL *pAVTemp;

	pAVCopy = *ppAVRoot;
							/* base case	*/
	if (nilAV == pAVCopy->AVsbpAVchild[fDive]) {
		/* fix current node's root pointer		*/
		*ppAVRoot = pAVCopy->AVsbpAVchild[fSide];

		/* replace destination node with current one	*/
		pAVTemp = *ppAVDest;
		*ppAVDest = pAVCopy;
		pAVCopy->AVsbpAVchild[AV_LCHILD] = pAVTemp->AVsbpAVchild[AV_LCHILD];
		pAVCopy->AVsbpAVchild[AV_RCHILD] = pAVTemp->AVsbpAVchild[AV_RCHILD];
		pAVCopy->AVbtipped = pAVTemp->AVbtipped;

		/* Dink destination node			*/
		AEDelete(& pAVTemp->AE_data);
		free(pAVTemp);
		return 1;
	} else if (AV_dive(& pAVCopy->AVsbpAVchild[fDive])) {
		/* Other side became shorter	*/
		if (AV_BAL_CENTER == pAVCopy->AVbtipped) {
			pAVCopy->AVbtipped = fSide;
		} else if (fSide != pAVCopy->AVbtipped) {
			pAVCopy->AVbtipped = AV_BAL_CENTER;
			return 1;
		} else {
			return AV_rotate(ppAVRoot, fSide);
		}
	}
	return 0;
}

/*
 * front-end for AV_dive()						(dsg)
 * dives down an AVL tree and replaces the destination node with
 * the next or previous node in the tree
 *
 * The sub tree on fSide must NOT be empty.
 *
 * ppAVRoot
 *	pointer to root of AVL tree
 * fSide
 *	AV_LCHILD: previous	(steps left, dives right)
 *	AV_RCHILD: next		(steps right, dives left)
 *
 * set globals
 * record if the tree becomes shorter in r
 * fix altered child
 * return result of dive
 */
static int
AV_side(ppAVRoot, fArg)
AVL **ppAVRoot;
int fArg;
{
	auto AVL *pAVScrap;	/* used to keep AV_dive out of free mem	*/
	auto int r;		/* our return value			*/

	ppAVDest = ppAVRoot;
	fDive = 1 ^ (fSide = fArg);
	pAVScrap = (*ppAVRoot)->AVsbpAVchild[fSide];
	r = AV_dive(& pAVScrap);
	(*ppAVDest)->AVsbpAVchild[fSide] = pAVScrap;
	return r;
}

/*
 * find and delete a node
 *
 * return 1 if the tree became shorter
 */
int
AVDelete(ppAVRoot)
AVL **ppAVRoot;
{
	register AVL *pAVCopy = *ppAVRoot;
	auto int fCmp;
	auto int fChange = AV_BAL_CENTER;

	if (nilAV == pAVCopy) {
		AENotfound();
	} else if (0 == (fCmp = AECmp(& pAVCopy->AE_data))) {
		if (nilAV == pAVCopy->AVsbpAVchild[AV_LCHILD]) {
			*ppAVRoot = pAVCopy->AVsbpAVchild[AV_RCHILD];
			AEDelete(& pAVCopy->AE_data);
			free(pAVCopy);
			return 1;

		} else if (nilAV == pAVCopy->AVsbpAVchild[AV_RCHILD]) {
			*ppAVRoot = pAVCopy->AVsbpAVchild[AV_LCHILD];
			AEDelete(& pAVCopy->AE_data);
			free(pAVCopy);
			return 1;

		} else if (AV_BAL_LEFT == pAVCopy->AVbtipped) { /* interior	*/
			if (AV_side(ppAVRoot, AV_LCHILD)) {
				pAVCopy = *ppAVRoot;
				fChange = AV_BAL_RIGHT;
			}
		} else if (AV_side(ppAVRoot, AV_RCHILD)) {
			pAVCopy = *ppAVRoot;
			fChange = AV_BAL_LEFT;
		}
	} else if (fCmp < 0) {
		if (AVDelete(& pAVCopy->AVsbpAVchild[AV_LCHILD])) {
			fChange = AV_BAL_RIGHT;
		}
	} else if (AVDelete(& pAVCopy->AVsbpAVchild[AV_RCHILD])) {
		fChange = AV_BAL_LEFT;
	}

	switch (fChange) {
	case AV_BAL_CENTER:
		return 0;
	case AV_BAL_RIGHT:			/* left subtree became shorter	*/
		switch (pAVCopy->AVbtipped) {
		case AV_BAL_RIGHT:
			return AV_rotate(ppAVRoot, AV_RCHILD);
		case AV_BAL_CENTER:
			pAVCopy->AVbtipped = AV_BAL_RIGHT;
			break;
		case AV_BAL_LEFT:
			pAVCopy->AVbtipped = AV_BAL_CENTER;
			return 1;
		}
		break;
	case AV_BAL_LEFT:			/* right subtree became shorter	*/
		switch (pAVCopy->AVbtipped) {
		case AV_BAL_LEFT:
			return AV_rotate(ppAVRoot, AV_LCHILD);
		case AV_BAL_CENTER:
			pAVCopy->AVbtipped = AV_BAL_LEFT;
			break;
		case AV_BAL_RIGHT:
			pAVCopy->AVbtipped = AV_BAL_CENTER;
			return 1;
		}
		break;
	}
	return 0;
}

/*@Explode count@*/
/*
 * passed a pointer to a AVL tree (LAT list) this routine returns	(ksb)
 * a count of the number of elements in the tree (list)
 *
 * we avoid left recursion because of the LAT list case
 */
AVCARDINAL
AVCount(pAV)
register AVL *pAV;
{
	register AVCARDINAL i;
	register AVL *pAVLeft;

	i = 0;
	while (nilAV != pAV) {
		if (nilAV != (pAVLeft = pAV->AVsbpAVchild[AV_LCHILD]))
			i += AVCount(pAVLeft);
		pAV = pAV->AVsbpAVchild[AV_RCHILD];
		++i;
	}
	return i;
}

/*@Explode lat@*/
/*
 * takes a neat bushy AVL tree and flatens it into a LAT list		(ksb)
 * the list created by this routine may be traversed via r-children
 *
 * a pointer to the new list is stored in *ppAVRet, the return value is
 * the address of the last r-child in the list (it does contain nilAV)
 *
 * find least {left most}, leaving crumb trail (return pointer to least)
 * left -> right, proccess left subtree, until hit nil right child
 * from root -> right, process right subtree, instert & replace
 */
AVL **
AVLat(pAVCur, ppAVRet)
register AVL *pAVCur;	/* current root to flaten		*/
AVL **ppAVRet;		/* where the new anchor point is put	*/
{
	register AVL *pAVLast, *pAVNext, **ppAV;

	if (nilAV == pAVCur) {
		*ppAVRet = nilAV;
		return ppAVRet;
	}

	pAVLast = nilAV;
	while (nilAV != pAVCur) {
		pAVNext = pAVCur->AVsbpAVchild[AV_LCHILD];
		pAVCur->AVsbpAVchild[AV_LCHILD] = pAVLast;
		pAVLast = pAVCur;
		pAVCur = pAVNext;
	}
	*ppAVRet = pAVLast;

	while (nilAV != (pAVNext = pAVLast->AVsbpAVchild[AV_LCHILD])) {
		ppAV = & pAVLast->AVsbpAVchild[AV_RCHILD];
		if (nilAV != (pAVCur = *ppAV)) {
			ppAV = AVLat(pAVCur, ppAV);
		}
		*ppAV = pAVNext;
		pAVLast->AVsbpAVchild[AV_LCHILD] = nilAV;
		pAVLast = pAVNext;
	}

	while (nilAV != (pAVNext = pAVLast->AVsbpAVchild[AV_RCHILD])) {
		ppAV = & pAVLast->AVsbpAVchild[AV_RCHILD];
		if (nilAV != (pAVCur = pAVNext->AVsbpAVchild[AV_LCHILD])) {
			ppAV = AVLat(pAVCur, ppAV);
		}
		*ppAV = pAVNext;
		pAVNext->AVsbpAVchild[AV_LCHILD] = nilAV;
		pAVLast = pAVNext;
	}
	return & pAVLast->AVsbpAVchild[AV_RCHILD];
}

/*@Explode cons@*/
/*
 * construct an AVL tree from a LAT list				(dsg)
 *
 * pass this routine the pointer to the LAT AVL tree, a count of the
 * number of nodes in the list, and an address to hang the new tree from
 *
 * in general, one subtree of any node is a complete and full binary tree
 */
AVL *
AVConstruct(pAVLat, iCount, ppAVRoot)
AVL *pAVLat, **ppAVRoot;
AVCARDINAL iCount;
{
	register AVL *pAVNext;
	register AVCARDINAL iBitvec;
	register AVCARDINAL iPower;
	auto AVL *pAVTemp;
	auto int fBal;
	static int fTip = 0;

	for (;;) {
		switch (iCount) {
		case 0:
			*ppAVRoot = nilAV;
			return pAVLat;

		case 1:
			*ppAVRoot = pAVLat;
			pAVNext = pAVLat->AVsbpAVchild[AV_RCHILD];
			pAVLat->AVbtipped = AV_BAL_CENTER;
			pAVLat->AVsbpAVchild[AV_RCHILD] = nilAV;
			return pAVNext;

		case 2:
			*ppAVRoot = pAVLat;
			pAVLat->AVbtipped = AV_BAL_RIGHT;
			pAVNext = pAVLat->AVsbpAVchild[AV_RCHILD];
			pAVNext->AVbtipped = AV_BAL_CENTER;
			pAVTemp = pAVNext->AVsbpAVchild[AV_RCHILD];
			pAVNext->AVsbpAVchild[AV_RCHILD] = nilAV;
			return pAVTemp;

		default:
			break;
		}
		for (iBitvec = ~0; 0 != (iBitvec & iCount); iBitvec <<= 1)
			/* null */;
		--iCount;		/* take off root node		*/
		iBitvec ^= iBitvec << 1;/* next power of two		*/
		iBitvec >>= 1;		/* this power of two (2**n)	*/
		iPower = iBitvec >> 1;	/* last power of two		*/
		--iBitvec;		/* sum 2**n up to (~including) n*/
		if (iPower <= iCount - iBitvec) {
			fBal = AV_BAL_CENTER;
		} else if ((iPower >> 1) <= (iCount - iBitvec)) {
			if (0 == (fTip ^= 1)) {
				fBal = AV_BAL_LEFT;
			} else {
				iBitvec = iCount - iBitvec;
				fBal = AV_BAL_RIGHT;
			}
		} else {
			iBitvec -= iPower;
			if (0 == (fTip ^= 1)) {
				fBal = AV_BAL_RIGHT;
			} else {
				iBitvec = iCount - iBitvec;
				fBal = AV_BAL_LEFT;
			}
		}
		*ppAVRoot = pAVNext = AVConstruct(pAVLat, iBitvec, & pAVTemp);
		pAVNext->AVsbpAVchild[AV_LCHILD] = pAVTemp;
		pAVNext->AVbtipped = fBal;

		/* no tail recurse	*/
		pAVLat = pAVNext->AVsbpAVchild[AV_RCHILD];
		iCount -= iBitvec;
		ppAVRoot = & pAVNext->AVsbpAVchild[AV_RCHILD];
	}
	/*NOTREACHED*/
}

/*@Explode merge@*/
/*
 * merge two avl trees, destroy both lesser trees to build it 		(ksb)
 *
 * if pi is not nil it is pointer to a AVCARDINAL to fill in with the count of
 * the number of node in the (new) tree
 */
AVL *
AVMerge(pAVOne, pAVTwo, pi)
register AVL *pAVOne, *pAVTwo;
AVCARDINAL *pi;
{
	register AVL **ppAV;
	register AVCARDINAL iCount;
	register int fCmp;
	auto AVL *pAVA, *pAVB;

	(void) AVLat(pAVOne, & pAVA);
	(void) AVLat(pAVTwo, & pAVB);
	pAVOne = pAVA;
	pAVTwo = pAVB;
	ppAV = & pAVA;
	iCount = (AVCARDINAL)0;
	while (nilAV != pAVOne && nilAV != pAVTwo) {
		if (0 == (fCmp = AEMerge(& pAVOne->AE_data, & pAVTwo->AE_data))) {
			pAVB = pAVTwo;
			pAVTwo = pAVTwo->AVsbpAVchild[AV_RCHILD];
			AESame(& pAVB->AE_data);
			free(pAVB);
			continue;
		} else if (fCmp < 0) {
			*ppAV = pAVOne;
			pAVOne = pAVOne->AVsbpAVchild[AV_RCHILD];
		} else {
			*ppAV = pAVTwo;
			pAVTwo = pAVTwo->AVsbpAVchild[AV_RCHILD];
		}
		ppAV = & (*ppAV)->AVsbpAVchild[AV_RCHILD];
		++iCount;
	}
	if (nilAV != pAVOne) {
		*ppAV = pAVOne;
	} else {
		*ppAV = pAVTwo;
		pAVOne = pAVTwo;
	}

	while (nilAV != pAVOne) {
		pAVOne = pAVOne->AVsbpAVchild[AV_RCHILD];
		++iCount;
	}
	if ((AVCARDINAL *)0 != pi)
		*pi = iCount;

	(void) AVConstruct(pAVA, iCount, & pAVB);
	return pAVB;
}

/*@Explode zap@*/
/*
 * calls free on every element left in the tree				(ksb)
 */
void
AVFree(pAVRoot)
register AVL *pAVRoot;
{
	register AVL *pAVTemp;

	while (nilAV != pAVRoot) {
		AVFree(pAVRoot->AVsbpAVchild[AV_LCHILD]);
		pAVTemp = pAVRoot;
		pAVRoot = pAVRoot->AVsbpAVchild[AV_RCHILD];
		AEFree(& pAVTemp->AE_data);
		free(pAVTemp);
	}
}

/*@Explode scan@*/
#ifdef undef
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
#endif example

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
		AVScan(pAVRoot->AVsbpAVchild[AV_LCHILD], pFunc);
		if (0 != (r = (*pFunc)(& pAVRoot->AE_data)))
			return r;
		pAVRoot = pAVRoot->AVsbpAVchild[AV_RCHILD];
	}
	return 0;
}

/*@Explode print@*/
/*
 * print an avl tree (terse format)					(ksb)
 *
 * a minus sign (-) is a balanced node
 * a      slash (/) is tipped left
 * a back-slash (\) is tipped right
 */
void
AVPrint(pAV)
AVL *pAV;
{
	if (nilAV == pAV) {
		putchar('~');
	} else {
		AEPrint(& pAV->AE_data);
		if (nilAV != pAV->AVsbpAVchild[AV_LCHILD] || nilAV != pAV->AVsbpAVchild[AV_RCHILD]) {
			putchar("-/\\"[1+pAV->AVbtipped]);
			putchar('(');
			AVPrint(pAV->AVsbpAVchild[AV_LCHILD]);
			putchar(',');
			AVPrint(pAV->AVsbpAVchild[AV_RCHILD]);
			putchar(')');
		}
	}
}

/*@Explode verify@*/
char	/* printf("balance for %08x should be %s.\n", AVpAVInsert, pchErr); */
	AVErrNone[] =	"in {1, 0 -1}",
	AVErrLeft[] =	"left-balanced",
	AVErrRight[] =	"right-balanced",
	AVErrCenter[] =	"center-balanced";
/*
 * recursively find depths of AVL subtrees then verify trees and balance
 * factors -- return depth
 */
int
AVVerify(pAVRoot)
AVL *pAVRoot;
{
	auto int iLeft, iRight, iMax;

	if (nilAV == pAVRoot) {
		return 0;
	}
	iLeft = AVVerify(pAVRoot->AVsbpAVchild[AV_LCHILD]);
	iRight = AVVerify(pAVRoot->AVsbpAVchild[AV_RCHILD]);
	iMax = (iLeft + iRight + 1) / 2;
	iLeft -= iRight;
	switch (iLeft) {
	case 1:
		if (AV_BAL_LEFT != pAVRoot->AVbtipped) {
			AVpAVInsert = pAVRoot;
			Squeak(AVErrLeft);
		}
		break;
	case 0:
		if (AV_BAL_CENTER != pAVRoot->AVbtipped) {
			AVpAVInsert = pAVRoot;
			Squeak(AVErrCenter);
		}
		break;
	case -1:
		if (AV_BAL_RIGHT != pAVRoot->AVbtipped) {
			AVpAVInsert = pAVRoot;
			Squeak(AVErrRight);
		}
		break;
	default:
		AVpAVInsert = pAVRoot;
		Squeak(AVErrNone);
		break;
	}
	return iMax + 1;
}

/*@Explode seq@*/
static AVL
	*pAVHold = nilAV,		/* hold the root		*/
	**ppAVSeq;			/* trace the right children	*/
static AVCARDINAL
	AViHold;			/* count the nodes		*/

/*
 * start a lat list of AVL nodes for a call to consturct		(ksb)
 */
int
AVBegin()
{
	if (nilAV != pAVHold) {
		return 1;
	}
	ppAVSeq = & pAVHold;
	AViHold = (AVCARDINAL)0;
	return 0;
}

/*
 * make a new link in the sequence
 */
AE_ELEMENT *
AVStep()
{
	extern char *malloc();
	register AVL *pAVTemp;

	*ppAVSeq = pAVTemp = (AVL *)malloc(sizeof(AVL));
	pAVTemp->AVbtipped = AV_BAL_CENTER;
	pAVTemp->AVsbpAVchild[AV_LCHILD] = nilAV;
	ppAVSeq = &  pAVTemp->AVsbpAVchild[AV_RCHILD];
	++AViHold;
	return & pAVTemp->AE_data;
}

/*
 * this little routine is called when all seqential data has been read
 * to form the AVL tree from it
 */
AVL *
AVEnd()
{
	auto AVL *pAVMem;
	(void) AVConstruct(pAVHold, AViHold, & pAVMem);
	pAVHold = nilAV;
	return pAVMem;
}

/*@Remove@*/
#ifdef TEST
/*@Explode token@*/
char yytext[80];
int iCount = 0;
AVCARDINAL iRoot = 0;
int fPrompt;

/*
 * a simple token get routine for our parser
 */
TOKEN
gettoken()
{
	static int c = '\n';
	static char sbPrompt[] = "(avltest) ";
	register char *pchText;
	register int t;

	for (;;) {
		while (isspace(c)) {
			if (fPrompt && '\n' == c)
				fprintf(stdout, "%s", sbPrompt);
			c = getchar();
		}

		if ('#' == c) {
			do {
				c = getchar();
			} while ('\n' != c && EOF !=c);
			continue;
		}

		if ('\"' == c) {
			while ('\"' != (t = getchar())) {
				if ('\\' == t) {
					t = getchar();
					switch (t) {
					case 'b': t = '\b'; break;
					case 'f': t = '\f'; break;
					case 'n': t = '\n'; break;
					case 't': t = '\t'; break;
					case 'r': t = '\r'; break;
					case '\n': continue;
					default:
						break;
					}
				}
				putchar(t);
			}
			c = getchar();
			continue;
		}
		break;
	}
	if (isalpha(c)) {
		pchText = yytext;
		do {
			*pchText++ = c;
			c = getchar();
		} while (isalnum(c) || '_' == c);
		*pchText = '\000';
		return IDENTIFIER;
	}
	t = c;
	c = getchar();
	switch (t) {
	case EOF:
		return TEOF;
	case '{':
	case '[':
		return AUTOON;
	case '}':
	case ']':
		return AUTOOFF;
	case '@':
	case '!':
		return STATUS;
	case '=':
	case '+':
		return INC;
	case '_':
	case '-':
		return DEC;
	case '^':
	case '%':
		return EXCHANGE;
	case ':':
	case ';':
		return MERGE;
	case '*':
	case '&':
		return FREE;
	case '/':
	case '?':
		return PRINT;
	case '\\':
	case '|':
		return VERBOSE;
	case '`':
	case '~':
		return DEL;
	case '<':
	case ',':
		return LAT;
	case '>':
	case '.':
		return CONSTRUCT;
	case '(':
		return BEGIN;
	case ')':
		return END;
	default:
		fprintf(stdout, "unknown cmd %c\n", t);
		while ('\n' != c)
			c = getchar();
	case '$':
		return HELP;
	}
}

/*@Explode support@*/
/*@Insert "extern int fPrompt;"@*/
/*@Insert "extern AVCARDINAL iCount, iRoot;"@*/
/*@Insert "extern char yytext[];"@*/

/*
 * we can init upon being made
 */
void
AEInit(pAEInsert)
AE_ELEMENT *pAEInsert;
{
	extern char *strcpy(), *malloc();
	pAEInsert->pchname = strcpy(malloc(strlen(yytext)+1),yytext);
	pAEInsert->inumber = iCount++;
	++iRoot;
}

/*
 * simple print routine
 */
int
AEPrint(pAETemp)
AE_ELEMENT *pAETemp;
{
	if (nilAE == pAETemp) {
		fprintf(stdout, "{nil}");
	} else {
		fprintf(stdout, "%s=%d", pAETemp->pchname, pAETemp->inumber);
	}
	return 0;
}

/*
 * free an AE
 */
AEFree(pAE)
AE_ELEMENT *pAE;
{
	--iRoot;
}

/*
 * any compare would work
 */
int
AECmp(pAE)
AE_ELEMENT *pAE;
{
	extern int strcmp();
#ifdef MICE
	int r;
	r = strcmp(yytext, pAE->pchname);
	fprintf(stdout, "%s %s %d\n", yytext, pAE->pchname, r);
	return r;
#else
	return strcmp(yytext, pAE->pchname);
#endif
}

/*
 * if we wish to merge two spaces
 */
int
AEMerge(pAELeft, pAERight)
AE_ELEMENT *pAELeft, *pAERight;
{
	extern int strcmp();
	return strcmp(pAELeft->pchname, pAERight->pchname);
}

/*
 * anything to print out an error message
 */
int
yyerror(pch)
char *pch;
{
	fprintf(stdout, "%s\n", pch);
}

/*
 * for our call to AVScan
 */
int
AEDump(pAE)
AE_ELEMENT *pAE;
{
	fprintf(stdout, "%3d\t%s\n", pAE->inumber, pAE->pchname);
	return 0;
}

/*@Explode main@*/
/*@Insert "extern TOKEN gettoken();"@*/
/*@Insert "extern int AEDump();"@*/
/*@Insert "extern int fPrompt;"@*/
/*@Insert "extern AVCARDINAL iRoot;"@*/
/*
 * clue me in...
 */
static char *sbpchHelp[] = {
	"avltest is a program for observing AVL trees dynamically:\n",
	"\tuse +(-) to turn on(off) output of located symbols\t(also =(_)\n",
	"\tuse {(}) to turn on(off) auto insert mode\t\t(also [(]))\n",
	"\tuse ^ to exchange for alternate tree\t\t\t(also %)\n",
	"\tuse : to merge two trees\t\t\t\t(also ;)\n",
	"\tuse * to free the tree\t\t\t\t\t(also &)\n",
	"\tuse > to construct an AVL tree from a lat list\t\t(also .)\n",
	"\tuse < to lat an AVL tree\t\t\t\t(also ,)\n",
	"\tuse / to show a tree's structure\t\t\t(also ?)\n",
	"\tuse \\ to dump a tree\t\t\t\t\t(also |)\n",
	"\tuse ~ to remove from the tree\t\t\t\t(also `)\n",
	"\tuse ! for a status line\t\t\t\t\t(also @)\n",
	"\tuse ( id1 id2 ... ) for in in order input\n",
	"\tuse $ for this message\n",
	(char *)0
};
/*
 * with the above message
 */
void
help()
{
	register char **ppch;
	for (ppch = sbpchHelp; (char *)0 != *ppch; ++ppch) {
		fputs(*ppch, stdout);
	}
}

/*
 * print of stats on a AVL tree
 */
void
pavstat(pchName, pAV, cnt)
char *pchName;
AVL *pAV;
AVCARDINAL cnt;
{
	register AVCARDINAL temp;
	register int depth;

	temp = AVCount(pAV);
	depth = AVVerify(pAV);
	fprintf(stdout, "%10s tree @ 0x%06x has %3d element", pchName, pAV, temp);
	if (1 != temp)
		putchar('s');
	if (temp != cnt)
		fprintf(stdout, " (passed %3)", cnt);
	fprintf(stdout, " max depth of %d.\n", depth);
}

/*
 * pretty complex little test shell...					(ksb)
 */
int
main(argc, argv)
int argc;
char **argv;
{
	static int fAdd = 0;
	static int fInorder = 0, fPrint = 0;
	static AVCARDINAL iAlt = 0;
	static AVL *pAVRoot, *pAVAlt, *pAVTemp;
	auto TOKEN TokenCur;
	register AVCARDINAL iTemp;

	if (1 != argc) {
		if (2 != argc) {
			yyerror("too many arguments (source file only)");
			exit(1);
		}
		if (NULL == freopen(argv[1], "r", stdin)) {
			perror(argv[1]);
			exit(1);
		}
	}
	fAdd = 1;
	if (fPrompt = isatty(fileno(stdin)))
		fprintf(stdout, "use '$' for help\n");
	TokenCur = gettoken();
	AVInit(& pAVRoot);
	AVInit(& pAVAlt);
	while (TokenCur != TEOF) {	/* really need a K switch, of course */
		switch (TokenCur) {
		case TEOF:
			fprintf(stdout, "end of data\n");
			exit(0);
		case HELP:
			help();
		case STATUS:
			fprintf(stdout, "auto insert is %d\n", fAdd);
			fprintf(stdout, "auto print is %d\n", fPrint);
			pavstat("primary", pAVRoot, iRoot);
			pavstat("alternate", pAVAlt, iAlt);
			break;
		case AUTOOFF:
			fAdd = 0;
			break;
		case AUTOON:
			fAdd = 1;
			break;
		case EXCHANGE:
			pAVTemp = pAVRoot;
			pAVRoot = pAVAlt;
			pAVAlt = pAVTemp;
			iTemp = iRoot;
			iRoot = iAlt;
			iAlt = iTemp;
			break;
		case MERGE:
			pAVRoot = AVMerge(pAVRoot, pAVAlt, & iRoot);
			pAVAlt = nilAV;		/* see AVInit		*/
			iAlt = 0;
			break;
		case FREE:
			AVFree(pAVRoot);
			pAVRoot = nilAV;
			iRoot = 0;
			break;
		case IDENTIFIER:
			if (fInorder) {
				AVpAEInsert = AVStep();
				AEInit(AVpAEInsert);
				break;
			}
			/* AVpAEInsert = ;  yytext */
			(void)AVInsert(& pAVRoot, fAdd);
			if (fPrint) {
				AEPrint(AVpAEInsert);
				putchar('\n');
			}
			break;
		case PRINT:
			AVPrint(pAVRoot);
			putchar('\n');
			break;

		case VERBOSE:
			(void) AVScan(pAVRoot, AEDump);
			putchar('\n');
			break;
		case DEL:
			TokenCur = gettoken();
			if (IDENTIFIER != TokenCur) {
				yyerror("avltest: identifier expected");
				exit(1);
			}
			(void) AVDelete(& pAVRoot);
			break;
		case LAT:
			(void) AVLat(pAVRoot, & pAVTemp);
			pAVRoot = pAVTemp;
			break;
		case CONSTRUCT:
			(void) AVConstruct(pAVRoot, iRoot, & pAVTemp);
			pAVRoot = pAVTemp;
			break;
		case INC:
			fPrint = 1;
			break;
		case DEC:
			fPrint = 0;
			break;
		case BEGIN:
			if (0 != AVBegin()) {
				yyerror("AVBegin: sequence in progress");
				break;
			}
			if (nilAV != pAVRoot) {
				yyerror("warning: old tree zapped");
				AVFree(pAVRoot);
				pAVRoot = nilAV;	/* see AVinit	*/
				iRoot = 0;
			}
			fInorder = 1;
			break;
		case END:
			pAVRoot = AVEnd();
			fInorder = 0;
			break;
		}
		TokenCur = gettoken();
	}
}

/*@Remove@*/
#ifdef undef
/*@Header@*/
#include <stdio.h>

/*@Explode worst@*/
/*
 * weigh a perfect binary search tree for n elements			(ksb)
 */
int
best(n)
register int n;
{
	register int p, sum, depth;

	p = 1;
	sum = 0;
	depth = 0;
	while (n >= p) {
		sum += depth * p;
		n -= p;
		p <<= 1;
		++depth;
	}
	sum += depth * n;
	return sum;
}

/*
 * this little program was used to compare AVL worst-case to		(ksb)
 * a perfrect log2(n) search tree.
 */
int
main(argc, argv)
int argc;
char **argv;
{
	extern int atoi();
	static int N[33], L[33], D[33], B[33];
	static double R[33], ap;
	register int k;
	register int max = 27;

	if (argc == 2)
		max = atoi(argv[1]);
	N[0] = 1; L[0] = 0; D[0] = 1; B[0] = 0; R[0] = 100.0;
	N[1] = 2; L[1] = 1; D[1] = 2; B[1] = 1; R[1] = 100.0;
	for (k = 1; k <= 32; ++k) {
		D[k] = D[k-1] + 1;
		N[k] = N[k-1] + N[k-2] + 1;
		L[k] = N[k-1] + N[k-2] + L[k-1] + L[k-2];
		B[k] = best(N[k]);
		R[k] = 100.0 * (double) L[k] / (double) B[k];
	}

	ap = 0.0;
	fprintf(stdout, "\
depth\tnodes\t     b-path\t     w-path\taverage path lenght %%\n\
-----\t-----\t     ------\t     ------\t------- ---- ------ -\n");
	for (k = 0; k < max; ++k) {
		fprintf(stdout, "%3d\t%3d\t%11d\t%11d\t%lf\n",
			D[k], N[k], B[k], L[k], R[k]);
		ap += R[k];
	}
	fprintf(stdout, "The average path is %lf of the best case tree.\n", ap/(double)max);
	exit(0);
}
/*@Remove@*/
#endif
#endif TEST
