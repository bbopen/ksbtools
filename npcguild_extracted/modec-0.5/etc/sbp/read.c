/* $Id: read.c,v 1.16 1999/07/14 15:14:28 ksb Exp $
 * read a fstab/vfstab/checklist/filesystems file and record
 * the stuff we need to know.
 *
 * hfs type == HPUX
 * second column = "-" Solaris vfstab format
 */
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include "main.h"
#include "read.h"

int iTableType = 0;

/* return the number of column in the line				(ksb)
 * "a colum is a transition from non-white to white-space or eoln"
 * the line ens at '\000' or '\n' or '#'
 */
static int
ColCount(pc)
char *pc;
{
	register int iRet, iState;

	if ('\000' == *pc) {
		return 0;
	}
	iState = isspace(*pc);
	for (iRet = 0; '\000' != *pc && '\n' != *pc && '#' != *pc; ++pc) {
		if (isspace(*pc)) {
			if (0 == iState)
				++iRet;
			iState = 1;
			continue;
		}
		iState = 0;
	}
	if (0 == iState) {
		++iRet;
	}
	return iRet;
}

/* return a strsave'd copy of this many colunms				(ksb)
 * We write on pcOrig, but we put it back.
 */
static char *
SaveCols(ppcOrig, iCols)
char **ppcOrig;
int iCols;
{
	register char *pc, *pcRet;
	register int iCount, iState, cSave;

	pc = *ppcOrig;
	iState = isspace(*pc);
	for (iCount = 0; '\000' != *pc && '\n' != *pc && '#' != *pc; ++pc) {
		if (isspace(*pc)) {
			if (0 == iState && ++iCount == iCols) {
				goto found;
			}
			iState = 1;
			continue;
		}
		iState = 0;
	}
	if (0 == iState && ++iCount == iCols) {
 found:
		cSave = *pc;
		*pc = '\000';
		pcRet = strdup(*ppcOrig);
		*pc = cSave;
		*ppcOrig = pc;
		return pcRet;
	}
	return (char *)0;
}

static void
ChopWhite(pcOrig)
char *pcOrig;
{
	register int i;

	for (i = strlen(pcOrig); i-- > 0; /* nada */) {
		if (!isspace(pcOrig[i]))
			break;
		pcOrig[i] = '\000';
	}
	
}

static char **ppcIndex;
static SBP_ENTRY *pSBPBackup;
static int iLines;

/* get the whole file,							(ksb)
 * create the pairs list
 */
SBP_ENTRY *
ReadFs(pcFile)
char *pcFile;
{
	auto struct stat stLen;
	auto SBP_ENTRY *pSBPPrimary, *pSBPIgnore;
	register char *pcMem, *pcNext, *pcCursor;
	register int iMax, iActive;
	register SBP_ENTRY *pSBP, **ppSBPActive, **ppSBPBackup, **ppSBPIgnore;
	register FILE *fpData;
	register int iMtLen, iSrcLen;

	if ((FILE *)0 == (fpData = fopen(pcFile, "rb"))) {
		fprintf(stderr, "%s: fopen: %s: %s\n", progname, pcFile, strerror(errno));
		exit(1);
	}
	if (-1 == fstat(fileno(fpData), & stLen)) {
		fprintf(stderr, "%s: fstat: %d: %s\n", progname, fileno(fpData), strerror(errno));
		exit(1);
	}
	iMax = (stLen.st_size|1023)+1;
	pcMem = calloc(iMax, sizeof(char));
	if ((char *)0 == pcMem) {
		fprintf(stderr, "%s: calloc: %d: %s\n", progname, iMax, strerror(errno));
		exit(2);
	}
	if (stLen.st_size != (iActive = fread(pcMem, sizeof(char), iMax, fpData))) {
		fprintf(stderr, "%s: internal size check failed\n got %d/%d expected %d\n", progname, iActive, iMax, (int)stLen.st_size);
		exit(2);
	}
	fclose(fpData);
	/* assert(pcMem[iActive] == '\000'); */

	/* Handle the case where the last line is missing \n as a whole line
	 */
	iLines = 0;
	for (pcCursor = pcMem; (char *)0 != pcCursor; pcCursor = pcNext) {
		pcNext = strchr(pcCursor, '\n');
		if ((char *)0 != pcNext) {
			*pcNext++ = '\000';
		} else if (pcMem+iActive == pcCursor) {
			break;
		}
		++iLines;
	}
	ppcIndex = (char **)calloc(iLines+2, sizeof(char *));
	if ((char **)0 == ppcIndex) {
		fprintf(stderr, "%s: calloc: %d,%d: %s\n", progname, iLines+2, sizeof(char *), strerror(errno));
		exit(3);
	}
	/* one list of /backup starts,
	 * one list of no /backup starts
	 */
	pSBPPrimary = pSBPBackup = (SBP_ENTRY *)0;
	pSBP = (SBP_ENTRY *)calloc(iLines+2, sizeof(SBP_ENTRY));
	if ((SBP_ENTRY *)0 == pSBP) {
		fprintf(stderr, "%s: calloc: %d,%d: %s\n", progname, iLines+2, sizeof(int), strerror(errno));
		exit(3);
	}
	ppSBPActive = & pSBPPrimary;
	ppSBPBackup = & pSBPBackup;
	ppSBPIgnore = & pSBPIgnore;

	iMtLen = strlen(pcMtPoint);
	iSrcLen = strlen(pcMtRoot);
	pcCursor = pcMem;
	/* /dev/dsk/c201d0s3  /home/dspd6  hfs  rw,suid   0 3 # hpux
	 * /dev/ccd11c        /var/ftp/pub ufs  rw,async  0 5 # BSD
	 * /dev/dsk/c0t0d0s6  -            /var ufs       4 no - # solaris
	 */
	for (iMax = 0; iMax < iLines; pcCursor = pcNext) {
		auto char *pcTemp, *pcDev, *pcMount;
		register int iTemp;

		ppcIndex[iMax++] = pcCursor;
		pcNext = pcCursor + strlen(pcCursor)+1;

		while (isspace(*pcCursor)) {
			++pcCursor;
		}
		if ('#' == pcCursor[0]) {
			continue;
		}

		pcTemp = pcCursor;
		iTemp = ColCount(pcCursor);
		if (0 == iTemp) {
			continue;
		}
		if (0 == iTableType) {
			iTableType = iTemp;
		} else if (iTemp != iTableType) {
			fprintf(stderr, "%s: %s:%d: variable columns (%d != %d)\n", progname, pcFile, iMax, iTemp, iTableType);
			exit(8);
		}
		switch (iTemp) {
		case 6:
			pcDev = SaveCols(& pcTemp, 1);
			break;
		case 7:
			pcDev = SaveCols(& pcTemp, 2);
			break;
		default:
			fprintf(stderr, "%s: %s:%d: bad format line:\n\"%s\"\n", progname, pcFile, iMax, pcCursor);
			exit(32);
		}

		/* ignore NFS sources host:file
		 */
		if ((char *)0 != strchr(pcDev, ':')) {
			continue;
		}
		while (isspace(*pcTemp)) {
			++pcTemp;
		}
		pcMount = SaveCols(& pcTemp, 1);
		ChopWhite(pcMount);

		/* build the book work
		 */
		pSBP->iline = iMax;
		pSBP->pcdev = pcDev;
		pSBP->pcmount = pcMount;
		pSBP->pcrest = pcTemp;
		pSBP->wflags = 0;
		pSBP->pBPswap = pSBP->pBPnext = (SBP_ENTRY *)0;
		if (0 == strncmp(pcMtPoint, pcMount, iMtLen) && ('/' == pcMount[iMtLen] || '\000' == pcMount[iMtLen])) {
			*ppSBPBackup = pSBP;
			ppSBPBackup = & pSBP->pBPnext;
		} else if (0 == strncmp(pcMtRoot, pcMount, iSrcLen) && (1 == iSrcLen || '/' == pcMount[iSrcLen] || '\000' == pcMount[iSrcLen])){
			*ppSBPActive = pSBP;
			ppSBPActive = & pSBP->pBPnext;
		} else {
			*ppSBPIgnore = pSBP;
			ppSBPIgnore = & pSBP->pBPnext;
		}
		++pSBP;
	}

	/* pair them.
	 */
	for (pSBP = pSBPBackup; (SBP_ENTRY *)0 != pSBP; pSBP = pSBP->pBPnext) {
		register SBP_ENTRY *pSBPMatch;
		register char *pcBack, *pcMatch;

		pcBack = pSBP->pcmount+iMtLen;
		if ('/' == *pcBack) {
			++pcBack;
		}
		for (pSBPMatch = pSBPPrimary; (SBP_ENTRY *)0 != pSBPMatch; pSBPMatch = pSBPMatch->pBPnext) {
			if ('/' != pSBPMatch->pcmount[0])
				continue;
			pcMatch = pSBPMatch->pcmount+iSrcLen;
			if ('/' == *pcMatch)
				++pcMatch;
			if (0 == strcmp(pcBack, pcMatch))
				break;
		}
		if ((SBP_ENTRY *)0 != pSBPMatch) {
			pSBP->pBPswap = pSBPMatch;
			pSBPMatch->pBPswap = pSBP;
			continue;
		}
		fprintf(stderr, "%s: %s: has no primary partition\n", progname, pSBP->pcmount);
		if (!fVerbose) {
			exit(11);
		}
		fprintf(stderr, "%s: for \"%s\"I see these primary partitions:\n", progname, pcMtRoot);
		for (pSBPMatch = pSBPPrimary; (SBP_ENTRY *)0 != pSBPMatch; pSBPMatch = pSBPMatch->pBPnext) {
			printf(" %s\n", pSBPMatch->pcmount);
		}
		exit(11);
	}
	return pSBPBackup;
}

/* find the active param for the line, if there is one			(ksb)
 */
SBP_ENTRY *
BPNow(iCur)
int iCur;
{
	register SBP_ENTRY *pSBP;

	for (pSBP = pSBPBackup; (SBP_ENTRY *)0 != pSBP; pSBP = pSBP->pBPnext) {
		if (iCur == pSBP->iline) {
			return pSBP;
		}
		if (iCur == pSBP->pBPswap->iline) {
			return pSBP->pBPswap;
		}
	}
	return (SBP_ENTRY *)0;
}

/* There is a much better way but I don't feel like coding it		(ksb)
 * (so we do a couple of scan though the stuff, bah.)
 * We'll also time stamp the file in the comments if you put a line
 *	#@ sbp run of MARK
 * in to get us started.
 */
void
DumpFs(fpOut)
FILE *fpOut;
{
	register int i;
	register SBP_ENTRY *pBP;
	static char acStamp[] =
		"#@ sbp run of";
	auto time_t tNow;

	for (i = 0; i++ < iLines; /* nada */) {
		pBP = BPNow(i);
		if ((SBP_ENTRY *)0 != pBP) {
			fprintf(fpOut, "%s %s%s\n", pBP->pBPswap->pcdev, pBP->pcmount, pBP->pcrest);
			continue;
		}
		if (0 == strncmp(ppcIndex[i-1], acStamp, sizeof(acStamp)-1)) {
			time(& tNow);
			fprintf(fpOut, "%s %s", acStamp, ctime(& tNow));
			continue;
		}
		fprintf(fpOut, "%s\n", ppcIndex[i-1]);
	}
	fflush(fpOut);
}
