#!mkcmd
# $Id: util_ppm.m,v 8.31 2000/05/29 20:41:25 ksb Exp $
# Keep a buffer of malloc'd memory we can grow as needed.		(ksb)
# I've coded this 100 times and I'm sick of (redoing) it.

# This doesn't exhibit my usual flair for the absurdly complex :-),
# and it only works for ANSI hosts, so there.  Great for RPC services.
from '<stdlib.h>'

%hi
#if !defined(UTIL_PPM_ONCE)
#define UTIL_PPM_ONCE	1
typedef struct PPMnode {
	void *pvbase;		/* present start			*/
	unsigned int uimax;	/* max we can hold now			*/
	unsigned int uiwide;	/* how wide is one element?		*/
	unsigned int uiblob;	/* how many elements do we grow by?	*/
} PPM_BUF;

extern PPM_BUF *util_ppm_init(PPM_BUF *, unsigned int, unsigned int);
extern void *util_ppm_size(PPM_BUF *, unsigned int);
extern void util_ppm_free(PPM_BUF *);
extern int util_ppm_print(PPM_BUF *, FILE *);
#endif
%%

%c
/* init a PPM, let me know how big each element is			(ksb)
 * and about how many to allocate when we run out, or pick 0 for
 * either and well guess at the other.
 */
PPM_BUF *
util_ppm_init(PPM_BUF *pPPM, unsigned int uiWidth, unsigned int uiBlob)
{
	register unsigned int iTemp;

	if ((PPM_BUF *)0 == pPPM)
		return (PPM_BUF *)0;
	pPPM->uimax = 0;
	pPPM->pvbase = (void *)0;

	pPPM->uiwide = uiWidth ? uiWidth : 1 ;
	iTemp = pPPM->uiwide < 2048 ? 8191-3*pPPM->uiwide : 1024*13-1;
	pPPM->uiblob = uiBlob ? uiBlob : (iTemp/pPPM->uiwide | 3)+1;
	return pPPM;
}

/* resize this buffer and return the new base address.			(ksb)
 * When we fail to resize we do NOT release the previous buffer.
 */
void *
util_ppm_size(PPM_BUF *pPPM, unsigned int uiCount)
{
	register unsigned int uiRound;
	register void *pvRet;

	if ((PPM_BUF *)0 == pPPM) {
		return (void *)0;
	}
	if (pPPM->uimax >= uiCount) {
		return pPPM->pvbase;
	}
	uiRound = uiCount / pPPM->uiblob;
	uiRound *= pPPM->uiblob;

	/* this could overflow, but we can't fix that
	 */
	if (uiRound < uiCount) {
		uiRound += pPPM->uiblob;
	}
	uiCount = uiRound;
	uiRound *= pPPM->uiwide;

	if ((void *)0 == pPPM->pvbase) {
		pvRet = (void *)malloc(uiRound);
	} else {
		pvRet = (void *)realloc(pPPM->pvbase, uiRound);
	}
	if ((void *)0 != pvRet) {
		pPPM->pvbase = pvRet;
		pPPM->uimax = uiCount;
	}
	return pvRet;
}

/* frees the present buffer, but doesn't forget the init		(ksb)
 */
void
util_ppm_free(PPM_BUF *pPPM)
{
	if ((PPM_BUF *)0 == pPPM) {
		return;
	}
	if (0 != pPPM->uimax && (void *)0 != pPPM->pvbase) {
		free(pPPM->pvbase);
		pPPM->pvbase = (void *)0;
	}
	pPPM->uimax = 0;
}

#if defined(DEBUG)
/* for debugging							(ksb)
 */
int
util_ppm_print(PPM_BUF *pPPM, FILE *fpOut)
{
	fprintf(fpOut, "ppm %p", (void *)pPPM);
	if ((PPM_BUF *)0 == pPPM) {
		fprintf(fpOut, " <is NULL>\n");
		return 0;
	}
	fprintf(fpOut, ": <max %u, width %u, blob %u> base %p\n", pPPM->uimax, pPPM->uiwide, pPPM->uiblob, pPPM->pvbase);
	return 1;
}
#endif
%%
