/*@Header@*/
/* $Id: uuid.c,v 2.0 2000/06/05 23:46:46 ksb Exp $
 * map a Unique User IDentifier to a path				(ksb)
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <strings.h>
#include "uuid.h"

/* do the actual look in the filesysem					(ksb)
 * ZZZ this is stub for the real one ZZZ
 */
int
MapPath(pcOut, pcUUID)
char *pcOut, *pcUUID;
{
	pcOut[0]  = pcUUID[0];
	pcOut[1]  = pcUUID[1];
	pcOut[2]  = '/';
	pcOut[3]  = pcUUID[2];
	pcOut[4]  = pcUUID[3];
	pcOut[5]  = '/';
	pcOut[6]  = pcUUID[4];
	pcOut[7]  = pcUUID[5];
	pcOut[8]  = '/';
	pcOut[9]  = pcUUID[6];
	pcOut[10] = pcUUID[7];
	pcOut[11] = '/';
	pcOut[12] = pcUUID[8];
	pcOut[13] = pcUUID[9];
	pcOut[14] = '\000';
	return 0;
}

#if !defined(UP_CACHE_SIZE)
#define UP_CACHE_SIZE	8192
#endif

typedef struct UPnode {
	char acuuid[12];		/* 10 character + '\000'	*/
	char acpath[MAXPATHLEN+4];	/* result for this uuid		*/
} UUID_PATH;

static UUID_PATH aUPTrans[UP_CACHE_SIZE];
static int iNextTrans, iMaxTrans;
static UUID_PATH *pUPLast;

/* initalize the cache, or clear it if you like				(ksb)
 */
void
MapInit()
{
	register int i;

	iNextTrans = iMaxTrans = 0;
	for (i = 0; i < sizeof(aUPTrans)/sizeof(aUPTrans[0]); ++i) {
		aUPTrans[i].acuuid[0] = '\000';
	}
	pUPLast = (UUID_PATH *)0;
}

/* Do the mapping, the result is only good until the next time you call	(ksb)
 * this function.  (as usual)
 */
char *
MapUUID(pcUUID)
char *pcUUID;
{
	register int i, iFirst, iCmp;
	register UUID_PATH *pUP;

	/* did we just do this guy?
	 */
	iCmp = *pcUUID++;
	if ((UUID_PATH *)0 != pUPLast && pUPLast->acuuid[0] == iCmp) {
		if (0 == strcmp(pUPLast->acuuid+1, pcUUID)) {
			return pUPLast->acpath;
		}
	}
	/* look in the translation table
	 */
	for (i = 0; i < iMaxTrans; ++i) {
		pUP = & aUPTrans[i];
		if ('\000' == (iFirst = pUP->acuuid[0]) || iCmp != iFirst) {
			continue;
		}
		if (0 == strcmp(pUP->acuuid+1, pcUUID)) {
			return pUP->acpath;
		}
	}
	/* not there, find a slot and map her
	 */
	if (sizeof(aUPTrans)/sizeof(aUPTrans[0]) == iNextTrans) {
		iNextTrans = 0;
	}
	pUP = & aUPTrans[iNextTrans];
	if (0 != MapPath(pUP->acpath, --pcUUID)) {
		return (char *)0;
	}

	/* install in cache and return
	 */
	(void)strcpy(pUP->acuuid, pcUUID);
	if (++iNextTrans > iMaxTrans) {
		iMaxTrans = iNextTrans;
	}
	return pUP->acpath;
}
