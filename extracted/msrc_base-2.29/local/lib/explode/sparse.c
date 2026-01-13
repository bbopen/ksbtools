/*@Version: $Revision: 1.9 $@*/
/* $Compile: gcc -g -Wall -DTEST -o %F %f
 * $Compile: gcc -DSP_RECURSE=1 -g -Wall -DTEST -o %F %f
 * $Compile: gcc -DSP_ALLOC_CACHE=4096 -g -Wall -DTEST -o %F %f
 */
/*@Header@*/
/* sparse spaces of integers -- ksb
 * Sorry about the (void *) being the only type we support.
 */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

/*@Shell %echo ${MACHINE_H+"$MACHINE_H"}%@*/
#include "sparse.h"
extern char *progname;

/*@Explode unfib@*/
/* The first number (0) and the last (4,294,967,295) are not in the
 * natural Fibonacci sequence.  We replace the first "1" with "0" to
 * cover the case where we need to represent a zero, and we replace
 * the next number (4,807,526,976) with 2^32-1 to avoid the overflow
 * caused by the computer's binary representation.
 * To get a 16 bit version remove the UL suffix and replace all
 * from  "46368, 75025, 121393..." with "46368, 65535".  If you have
 * wider bit ranges you should add the missing numbers up to
 * 2^N-1 for your machine (also restore the 2^32-1 to the real number).
 * Of course you should change the sp_key typedef to be the correct size.
 */
sp_key sp_fibs[] = {
		 0UL,	       1UL,	     2UL,	  3UL,		5UL,
		 8UL,	      13UL,	    21UL,	 34UL,	       55UL,
		89UL,	     144UL,	   233UL,	377UL,	      610UL,
	       987UL,	    1597UL,	  2584UL,      4181UL,	     6765UL,
	     10946UL,	   17711UL,	 28657UL,     46368UL,	    75025UL,
	    121393UL,	  196418UL,	317811UL,    514229UL,	   832040UL,
	   1346269UL,	 2178309UL,    3524578UL,   5702887UL,	  9227465UL,
	  14930352UL,	24157817UL,   39088169UL,  63245986UL,	102334155UL,
	 165580141UL,  267914296UL,  433494437UL, 701408733UL, 1134903170UL,
	1836311903UL, 2971215073UL,
#if ULONG_MAX == 4294967295UL
	4294967295UL
#else
	    4807526976UL, 7778742049UL, 12586269025UL, 20365011074UL,
	    32951280099UL, 53316291173UL, 86267571272UL, 139583862445UL,
	    225851433717UL, 365435296162UL, 591286729879UL, 956722026041UL,
	    1548008755920UL, 2504730781961UL, 4052739537881UL,
	    6557470319842UL, 10610209857723UL, 17167680177565UL,
	    27777890035288UL, 44945570212853UL, 72723460248141UL,
	    117669030460994UL, 190392490709135UL, 308061521170129UL,
	    498454011879264UL, 806515533049393UL, 1304969544928657UL,
	    2111485077978050UL, 3416454622906707UL, 5527939700884757UL,
	    8944394323791464UL, 14472334024676221UL, 23416728348467685UL,
	   37889062373143906UL, 61305790721611591UL, 99194853094755497UL,
	  160500643816367088UL, 259695496911122585UL,
	  420196140727489673UL, 679891637638612258UL,
	 1779979416004714189UL, 2880067194370816120UL,
	 4660046610375530309UL, 7540113804746346429UL,
	12200160415121876738UL, 18446744073709551615UL
#endif
};

/* Return the slot in the sp_fibs vector we should hang from		(ksb)
 */
size_t
sp_unfib(sp_key wN)
{
	register int i;

	if ((sp_key *)0 == sp_fibs) {
		fprintf(stderr, "%s: sparse space: uninialized\n", progname);
		exit(1);
	}
	/* LLL we can search better here -- ksb */
	for (i = 0; /* nada */; ++i) {
		if (sp_fibs[i] < wN)
			continue;
		if (wN == sp_fibs[i])
			return i;
		break;
	}
	return i-1;
}

/*@Explode init@*/
#if defined(SP_ALLOC_CACHE)
#if !defined(SP_ALLOC_TRIM)
#define SP_ALLOC_TRIM	8
#endif
#endif

/* Called before any other function to build the tables we need:	(ksb)
 *	the vector for the space we asked for, all spaces start at zero
 * if you need a biased space wrap us in a structure with the [start,end].
 * N.B. witbh SP_ALLOC_CACHE set we can _never_ cleanup the malloc space.
 */
void **
sp_init(sp_key wRange)
{
	register unsigned int iNeed;
#if defined(SP_ALLOC_CACHE)
	static void **ppvCache;
	static unsigned int iHold;
	register void **ppvRet;
#endif

	iNeed = sp_unfib(wRange)+1;
#if defined(SP_ALLOC_CACHE)
	/* We won't discard SP_ALLOC_TRIM slots, we try to directly allocate
	 * any larger request to save the space.  When we can we grab a
	 * slab of SP_ALLOC_CACHE, but if that is tuned too small we add
	 * ~64 so we'll hit at least once after this (ksb)
	 */
	if (iNeed > iHold && iHold < SP_ALLOC_TRIM) {
		register void **ppvNew;
		register int iNew;

		iNew = SP_ALLOC_CACHE;
		if (iNew < iNeed + 2*SP_ALLOC_TRIM) {
			iNew |= 7;
			iNew += 57;
		}
		if ((void **)0 != (ppvNew = (void **)calloc(iNew, sizeof(void *)))) {
			iHold = iNew;
			ppvCache = ppvNew;
		}
	}
	if (iNeed <= iHold) {
		iHold -= iNeed;
		ppvRet = ppvCache;
		if (iHold < 2) {	/* can never hit in sparse */
			iHold = 0;
			ppvCache = (void **)0;
		} else {
			ppvCache += iNeed;
		}
		return ppvRet;
	}
#endif
	return (void **)calloc(iNeed, sizeof(void *));
}

/*@Explode index@*/
/* Find a element given the scalar index into the space.		(ksb)
 * Just scan down the list looking for a direct node, or (initing) and
 * bouncing through indirect nodes.  Like an old-school filesystem.
 */
void **
sp_index(void **ppvFrom, sp_key wFind)
{
	register int iU;

	for (;;) {
		iU = sp_unfib(wFind);
		ppvFrom += iU;
		if (SP_TERM(iU))
			return ppvFrom;
		wFind -= sp_fibs[iU];
		if ((void *)0 == *ppvFrom) {
			*ppvFrom = (void *)sp_init(sp_fibs[iU-1]);
#if SP_VERBOSE
			printf("[%d] got %d slots for %lu\n", iU, iU-1, sp_fibs[iU-1]);
#endif
		}
		ppvFrom = (void **)*ppvFrom;
	}
}

/*@Explode walk@*/
/* We walk all the node we created, don't itterate through with		(ksb)
 * index, or you'll make them all!  This calls the mapping function
 * on each user defined element (from wLow to wHi) in order.
 * The recursive version of this is slower for sparse spaces.
 */
#if SP_RECURSE == 0
int
sp_walk(void **ppvFrom, sp_key wLow, sp_key wHi, int (*pfiMap)(sp_key, void *))
{
	register int iRet = 0;
	register void **ppvScan;
	register sp_key wFind, wAdv;
	register int iU;

	do {
		ppvScan = ppvFrom;
		wFind = wLow;
		for (;;) {
			iU = sp_unfib(wFind);
			ppvScan += iU;
			if (SP_TERM(iU)) {
				wAdv = 1;
				break;
			}
			wFind -= sp_fibs[iU];
			/* Skip the whole bucket below a NULL, we know how
			 * many would have been in the bucket (Fib[u-1]). (ksb)
			 */
			if ((void *)0 == *ppvScan) {
				ppvScan = (void **)0;
				wAdv = sp_fibs[iU-1];
				if (iU+1 == sizeof(sp_fibs)/sizeof(sp_fibs[0]))
					return 0;
				if (wAdv > sp_fibs[iU+1] - wLow)
					wAdv = sp_fibs[iU+1] - wLow;
				break;
			}
			ppvScan = (void **)*ppvScan;
		}
		if ((void **)0 != ppvScan && (void *)0 != *ppvScan) {
			iRet |= (*pfiMap)(wLow, *ppvScan);
		}
		if (wLow == wHi)
			break;
#if SP_VERBOSE
		printf("%lu skip forward %lu\n", wLow, wAdv);
#endif
		wLow += wAdv;
	} while (0 == iRet);
	return iRet;
}
#else
/* This version assumes that the first call's ppvFrom is aligned	(ksb)
 * at the zero (root) vector {viz. wOffset starts at zero}.
 */
int
sp_walk(void **ppvFrom, sp_key wLow, sp_key wHi, int (*pfiMap)(sp_key, void *))
{
	register int iRet = 0, iTrap;
	static sp_key wOffset = (sp_key)0;
	register sp_key wLoop, wAdv, wMax;
	register int iU;
	register void **ppvDown;
	auto sp_key wKeep;

#if SP_VERBOSE
	printf("walk from %lu [%lu-%lu] spans %lu\n", wOffset, wLow, wHi, wHi-wLow);
#endif
	wKeep = wOffset;
	for (iTrap = 0, wLoop = wLow; !iTrap && 0 == iRet && wLoop <= wHi; wLoop += wAdv) {
		iU = sp_unfib(wLoop-wKeep);
		if (SP_TERM(iU)) {
			wAdv = (sp_key)1;
			if ((void *)0 != ppvFrom[iU]) {
				iRet |= (*pfiMap)(wLoop, ppvFrom[iU]);
			}
			iTrap = wLoop == wHi;
			continue;
		}
		wOffset = wKeep + sp_fibs[iU];
		wAdv = sp_fibs[iU-1];
		if (sp_fibs[iU+1] - wOffset < wAdv) {
			wAdv = sp_fibs[iU+1] - wOffset;
			if ((sp_key)0 == wAdv)
				wAdv = (sp_key)1;
		}
		iTrap = wAdv > wHi || wHi - wAdv < wLoop;
		if ((void **)0 == (ppvDown = (void **)ppvFrom[iU])) {
			continue;
		}
		wMax = wKeep + sp_fibs[iU+1] - (sp_key)1;
		if (iTrap || wHi < wMax) {
			wMax = wHi;
		}
		iRet |= sp_walk(ppvDown, wLoop, wMax, pfiMap);
	}
	wOffset = wKeep;
	return iRet;
}
#endif

/*@Explode exists@*/
/* Check for the existance of a single key value			(ksb)
 */
static int
sp_probe(sp_key wN, void *pvJunk)
{
	return 1;
}
/* Uses the function above to look for the named key in the space	(ksb)
 */
int
sp_exists(void **ppvRoot, sp_key wKey)
{
	return sp_walk(ppvRoot, wKey, wKey, sp_probe);
}

/*@Removed@*/
#if defined(TEST)
/*@Explode main@*/
char *progname = "sparse space test";

/* Show what we mapped into the space,					(ksb)
 * return non-zero for a "found it, stop" behavior.
 */
static int
tMap(sp_key w, void *pv)
{
	printf("%lu %s\n", w, (char *)pv);
	return 0;
}

int
main(int argc, char **argv)
{
	register int i;
	auto void **ppvRoot, **ppvFound;
	static sp_key awTest[] = {
		/* common base cases */
		15UL, 16UL, 17UL, 18UL, 19UL, 20UL, 21UL,
		8UL, 9UL, 10UL, 11UL, 12UL, 13UL, 14UL,
		/* powers of ten demo */
		100UL, 1000UL, 10000UL, 100000UL, 1000000UL, 10000000UL,
		/* Alfred Baster's number */
		5271009UL,
		/* worst case numbers */
		2971215071UL, 1836311901UL,
		/* corner case check */
		SP_MAX_KEY-1, SP_MAX_KEY, 0
	};

	/* try an empty tree first (that was a bug).
	 */
	ppvRoot = sp_init(SP_MAX_KEY);
	sp_walk(ppvRoot, 0, SP_MAX_KEY, tMap);

	/* add stuff and try a sparse tree
	 */
	for (i = 0; i < sizeof(awTest)/sizeof(awTest[0]); ++i) {
#if SP_VERBOSE
		register int iU;

		iU = sp_unfib(awTest[i]);
		printf("%9lu  %2d [%lu, %lu)\n", awTest[i], iU, sp_fibs[iU], sp_fibs[iU+1]);
#endif
		ppvFound = sp_index(ppvRoot, awTest[i]);
		((char **)ppvFound)[0] = (i & 2) ? "red" : "blue";
	}
	sp_walk(ppvRoot, 0, SP_MAX_KEY, tMap);
	exit(0);
}
/*@Remove@*/
#endif /* test driver */
