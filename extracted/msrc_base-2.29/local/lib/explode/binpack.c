/*@Version $Revision: 6.6 $@*/
/*@Header@*/
/* $Id: binpack.c,v 6.6 2009/10/16 15:41:17 ksb Exp $
 * pack data into bins						(mwm&ksb)
 *
 * $Compile: ${cc=cc} ${cc_debug=-g} -DTEST -o %F %f
 * $Test: (for w in 13 2 7 3 5 11 17 0; do echo $w; done)| ./%F 20
 */
#include <stdio.h>
#include <stdlib.h>

/*@Shell %echo ${MACHINE_H+"$MACHINE_H"}%@*/
#include "binpack.h"

/*@Explode pack@*/
/* one way to "bucket" the incoming data			(ksb)
 */
size_t
VE_log2(n)
	VEWEIGHT n;
{
	register size_t res;

	for (res = 0; n > 0; ++res) {
		n >>= 1;
	}
	return res;
}

/* another way to "bucket" the incoming data			(ksb)
 */
size_t
VE_unfib(n)
	VEWEIGHT n;
{
	register size_t res;
	register VEWEIGHT fib1 = 1, fib2 = 2, fib3;

	for (res = 0; fib1 < n; ++res) {
		fib3 = fib2 + fib1;
		fib1 = fib2;
		fib2 = fib3;
	}
	return res;
}

size_t (*VE_scale)(VEWEIGHT) = VE_unfib;


typedef struct VQnode {
	VE key;
	VEWEIGHT VWbias;
	struct VQnode *pVQnext;
} VEQ;
#define nilVQ	((VEQ *)0)

/* pack data into bins, order for packing at least		    (mwm&ksb)
 * return the number of bins that are the correct size,
 * keys which were too big to fit in a bin are stuck on the end
 *
 * outline:
 *	put the keys into buckets by some log or inverse exp functions
 *	select keys from the buckets in reverse order to fill buckets
 *	tricky hueristic allows us to skip one `stopper' in a bucket
 * order:
 *	order N memory
 *	about order N time
 *	rebias happens about 2*(ln[2] sizeof(VEWEIGHT)) times, a constant
 */
size_t
VEbinpack(v, n, w, b, puOver)
VE *v;			/* vector of keys				*/
size_t n;		/* length of key vector				*/
VEWEIGHT (*w)(VE_MASS_DECL);	/* map VE to her weight			*/
VEWEIGHT b;		/* bin size					*/
size_t *puOver;		/* number of over-sized elements		*/
{
	VEQ
		*pVQLast,	/* temp pointer for re-biasing code	*/
		*pVQList,	/* hold memory to free at end		*/
		*pVQTemp,	/* free node list			*/
		**ppVQBuckets,	/* list of keys for each class		*/
		*pVQTooBig,	/* list of items too big		*/
		*pVQSaveone;	/* a little heuristic hack		*/
	register int
		minbucket,	/* min bucket to search			*/
		maxbucket,	/* max bucket to search			*/
		i;		/* 1..n or 1..scaleb so as to visit all	*/
	register size_t
		scaleb,		/* scaleb holds VE_scale(b)		*/
		slot,		/* slot holds VE_scale((*w)(key))	*/
		small_enough,	/* items weighted <=b			*/
		out,		/* items put in the outgoing list	*/
		bins;		/* number of bins we had to make	*/
	VEWEIGHT
		VWMin,		/* lightest key weigth			*/
		VWAdd,		/* total we have biases by		*/
		weight,		/* current weight			*/
		space_left;	/* value left in this box		*/

	if (0 == n)
		return 0;

	VWMin = (*w)(VE_MASS(v[0]));
	scaleb = VE_scale(b)+1;
	ppVQBuckets = (VEQ **)calloc(scaleb+1, sizeof(VEQ *));
	pVQList = pVQTemp = (VEQ *)calloc(n, sizeof(VEQ));

	/* find min weight, compute all weights, throw out too bigs
	 */
	pVQSaveone = pVQTooBig = nilVQ;
	for (small_enough = i = 0; i < n; ++i, ++pVQTemp){
		weight = (*w)(VE_MASS(v[i]));
		if (weight > b) {
			pVQTemp->pVQnext = pVQTooBig;
			pVQTemp->key = v[i];
			pVQTooBig = pVQTemp;
			continue;
		}
		if (weight < VWMin)
			VWMin = weight;
		pVQTemp->key = v[i];
		pVQTemp->VWbias = weight;
		pVQTemp->pVQnext = pVQSaveone;
		pVQSaveone = pVQTemp;
		++small_enough;
	}

	VWAdd = (VEWEIGHT)0;
	bins = out = 0;

	/* re-bias and stuff in buckets
	 */
rebias:
	VWAdd += VWMin;
	minbucket = scaleb;
	maxbucket = 0;
	while (nilVQ != (pVQTemp = pVQSaveone)) {
		pVQSaveone = pVQTemp->pVQnext;

		weight = (pVQTemp->VWbias -= VWMin);
		slot = VE_scale(weight);
		pVQTemp->pVQnext = ppVQBuckets[slot];
		ppVQBuckets[slot] = pVQTemp;

		if (maxbucket < slot)
			maxbucket = slot;
		if (minbucket > slot)
			minbucket = slot;
	}

	/* Form cool lists first, take as many as we can from each
	 * bucket, then try to skip one stopper... this helps more
	 * than you might think.
	 */
	while (small_enough > 0) {
		space_left = b;
		for (i = maxbucket; i >= minbucket; --i) {
			while (nilVQ != ppVQBuckets[i] && (ppVQBuckets[i]->VWbias + VWAdd) <= space_left) {
				v[out++] = ppVQBuckets[i]->key;
				space_left -= ppVQBuckets[i]->VWbias + VWAdd;
				ppVQBuckets[i] = ppVQBuckets[i]->pVQnext;
				--small_enough;
			}
			/* if there is one `stopper' in this list try to
			 * get past it.  Next time through we will have
			 * more room, maybe. We end up exausting the buckets
			 * below us too fast if we don't do this.
			 */
			if (nilVQ == (pVQSaveone = ppVQBuckets[i])) {
				continue;
			}
			ppVQBuckets[i] = ppVQBuckets[i]->pVQnext;
			while (nilVQ != ppVQBuckets[i] && (ppVQBuckets[i]->VWbias + VWAdd) <= space_left) {
				v[out++] = ppVQBuckets[i]->key;
				space_left -= ppVQBuckets[i]->VWbias + VWAdd;
				ppVQBuckets[i] = ppVQBuckets[i]->pVQnext;
				--small_enough;
			}
			pVQSaveone->pVQnext = ppVQBuckets[i];
			ppVQBuckets[i] = pVQSaveone;
		}
		++bins;
		if (small_enough == 0)
			break;
		while (nilVQ == ppVQBuckets[minbucket])
			++minbucket;
		while (nilVQ == ppVQBuckets[maxbucket])
			--maxbucket;
		/* we can't mess up with only 2 items left, just do it
		 */
		if (minbucket+1 < maxbucket || small_enough < 3)
			continue;
		/* find a new spread, we have to differentiate
		 * lots of `median' values here
		 */
		pVQSaveone = ppVQBuckets[minbucket];
		ppVQBuckets[minbucket] = nilVQ;
		VWMin = pVQSaveone->VWbias;
		for (pVQTemp = pVQSaveone; nilVQ != pVQTemp; pVQTemp = pVQTemp->pVQnext) {
			if (VWMin > pVQTemp->VWbias)
				VWMin = pVQTemp->VWbias;
			pVQLast = pVQTemp;
		}
		if (minbucket != maxbucket) {
			pVQLast->pVQnext = ppVQBuckets[maxbucket];
			ppVQBuckets[maxbucket] = nilVQ;
		}
		goto rebias;
	}

	/* stick the broken ones in the list anyway, the user can look at'm
	 */
	if ((size_t *)0 != puOver) {
		*puOver = 0;
	}
	while (nilVQ != pVQTooBig) {
		v[out++] = pVQTooBig->key;
		pVQTooBig = pVQTooBig->pVQnext;
		if ((size_t *)0 != puOver)
			++*puOver;
	}

	/* return memory we used
	 */
	free((void *)pVQList);
	free((void *)ppVQBuckets);

	return bins;
}

/*@Explode assemble@*/
/* Hand a user defined function each bin in turn for easy traversal	(ksb)
 * Return the sum of the display funtions.
 */
int
VEassemble(VE *pVE, size_t iHave, VEWEIGHT (*w)(VE_MASS_DECL), VEWEIGHT wSize, int (*pfiDisplay)(VE *, size_t))
{
	register size_t wSum, wHold;
	register size_t i;
	register int iRet;

	if (0 == iHave) {
		return 0;
	}
	wHold = 0;
	wSum = (*w)(VE_MASS(pVE[0]));
	for (i = 1; i < iHave; ++i) {
		wSum += (*w)(VE_MASS(pVE[i]));
		if (wSum > wSize) {
			iRet += pfiDisplay(pVE+wHold, i-wHold);
			wHold = i;
			wSum = (*w)(VE_MASS(pVE[i]));
		}
	}
	iRet += pfiDisplay(pVE+wHold, i-wHold);
	return iRet;
}

/*@Remove@*/
#if defined(TEST)
/*@Explode test@*/

/* Test output for a list of integers as a bin				(ksb)
 */
int
Display(VE *piList, size_t iCount)
{
	register char *pcSep = "bin: ";

	while (iCount-- > 0) {
		printf("%s%d", pcSep, *piList++);
		pcSep = ", ";
	}
	printf("\n");
	return 0;
}

/* Test function converts an interger to iteself as a weight		(ksb)
 */
static VEWEIGHT
Ident(VE v)
{
	return v;
}

char *progname =
	"binpack";

/* Give me bin size ($1) and a list of integets to pack on stdin	(ksb)
 */
int
main(argc, argv)
int argc;
char **argv;
{
	auto size_t iMade, iOver, iRead, iAlloc;
	auto VEWEIGHT wSize;
	register int *piHold;

	if (argc != 2 || 0 == (wSize = atoi(argv[1]))) {
		printf("%s: usage binsize\n", progname);
		exit(1);
	}
	iRead = 0;
	iAlloc = 1024;
	piHold = calloc(iAlloc, sizeof(VE));
	for (;;) {
		if ((int *)0 == piHold) {
			fprintf(stderr, "%s: no memory\n", progname);
			exit(1);
		}
		if (1 != scanf("%d", &piHold[iRead])) {
			break;
		}
		if (++iRead == iAlloc) {
			iAlloc += iAlloc/2;
			piHold = (VE *)realloc((void *)piHold, sizeof(int)*iAlloc);
		}
	}
	iMade = VEbinpack(piHold, iRead, Ident, wSize, & iOver);
	VEassemble(piHold, iRead, Ident, wSize, Display);
	printf("bins made: %d, overs %d\n", iMade, iOver);
	free((void *)piHold);
	exit(0);
}
/*@Remove@*/
#endif
