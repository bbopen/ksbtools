/* $Id: util_unity.mc,v 8.3 2004/12/15 22:20:18 ksb Exp $
 */

/* Find the DNS RR for the local caches.				(ksb)
 *
 * Return 0 for failed, 1 for OK to fetch data
 * remembers RR name so "unity_init((char *)0)" can re-read DNS record.
 * Call "unity_init((char *)0)" at _least_ every 3 minutes, or
 * just before a new transaction comes in if it has been 180 seconds
 * since the last refresh.
 *
 * This keeps the list of unity hosts up to the minute.
 */
int
unity_init(pcRoundRobin)
char *pcRoundRobin;
{
	register struct hostent *pHELook;

	if ((char *)0 == pcRoundRobin) {
		pcRoundRobin = pcUnityRR;
	}
	if ((char *)0 == pcRoundRobin) {
		errno = EFAULT;
		return 0;
	}
	if ((struct hostent *)0 == (pHELook = gethostbyname(pcRoundRobin))) {
		/* caller read h_errno */
		errno = ENOENT;
		return 0;
	}
	iUnityScan = 0;
	pcUnityRR = pcRoundRobin;
	if ((struct hostent *)0 != pHEUnityRR) {
		free((void *)pHEUnityRR);
	}
%	pHEUnityRR = %K<util_savehostent>1v(pHELook)%;

	/* we don't expect the service to change for the life of the process
	 */
	if ((struct servent *)0 != pSEUnity) {
		return 1;
	}

%	pSEUnity = getservbyname(%K<unity_rr>2v, %K<unity_rr>3v)%;
	if ((struct servent *)0 == pSEUnity) {
		errno = ENOENT;
		return 0;
	}
%	pSEUnity = %K<util_saveservent>1v(pSEUnity)%;

	/* ready for file transfers
	 */
	return 1;
}

/* read the local file into RAM asap					(ksb)
 */
static char *
unity_cache(fpHave, fKeep)
FILE *fpHave;
int fKeep;
{
	static char *pcBuf = (char *)0;
	static int iMax = 0;
	auto struct stat stHave;
	register int iSize, iGuess, cc;
	register char *pcMem;

	/* the file might be growing, but guess how big it is
	 */
	if (-1 == fstat(fileno(fpHave), & stHave)) {
		iGuess = 0 == iMax ? 10240 : iMax;
	} else {
		iGuess = (stHave.st_size | 511) + 1;
	}
	if ((char *)0 == pcBuf) {
		pcMem = calloc(iGuess, sizeof(char));
	} else if (iMax < iGuess) {
		pcMem = realloc(pcBuf, iGuess * sizeof(char));
	}
	if ((char *)0 == pcMem) {
		return (char *)0;
	}
	iMax = iGuess;
	pcBuf = pcMem;

	iSize = 0;
	while (0 != (cc = fread(pcMem, sizeof(char), iMax-iSize, fpHave))) {
		if (-1 == cc) {
			return (char *)0;
		}
		iSize += cc;
		pcMem += cc;
		if (iMax > iSize + 1) {
			continue;
		}
		iGuess = iMax + 4096;	/* file grew! add more space */
		pcMem = realloc(pcBuf, iGuess * sizeof(char));
		if ((char *)0 == pcMem) {
			return (char *)0;
		}
		pcBuf = pcMem;
		pcMem += iSize;
		iMax = iGuess;
	}
	*pcMem = '\000';
	pcMem = pcBuf;
	if (fKeep) {
		pcBuf = (char *)0;
		iMax = 0;
	}
	return pcMem;
}

/* Recover a file from the unity cache.					(ksb)
 * if you set fKeep (to non-zero) we malloc space for you, else we
 * use the same buffer over and over.  You pick.  Call "unity_free" on
 * and buffer you keep when you are done with it.
 */
char *
unity_recover(pcState, fKeep)
char *pcState;
int fKeep;
{
	register int i;
	auto char *apcRotate[2];
	static char acMode[] = "octet";
	register char **ppcAddr;
%@if unity_cache[1]
	auto char acLook[MAXPATHLEN+2];
	register char *pcRet;
	register FILE *fpTemp;
%@endif

	/* Search the local cache first.
	 * define as many local cache dirs as you need in the key
	 * <unity_cache> as C expressions that yeild strings.
	 * XXX should use "snprintf" to be safe (not on Suns, sigh).
	 */
%@foreach "unity_cache"
%	(void)sprintf(acLook, "%%s/%%s", %a, pcState)%;
	if ((FILE *)0 != (fpTemp = fopen(acLook, "rb"))) {
		pcRet = unity_cache(fpTemp, fKeep);
		fclose(fpTemp);
		return pcRet;
	}
%@endfor

	if ((struct hostent *)0 == pHEUnityRR) {
		errno = ENOTCONN;
		return (char *)0;
	}
	ppcAddr = pHEUnityRR->h_addr_list;
	pHEUnityRR->h_addr_list = apcRotate;
	apcRotate[1] = (char *)0;
	for (i = iUnityScan;;) { 
		apcRotate[0] = ppcAddr[i];
		if (0 == tftp_open(pHEUnityRR, pSEUnity) && 0 == tftp_get((FILE *)0, pcState, acMode))
			break;
		++i;
		if ((char *)0 == ppcAddr[i])
			i = 0;
		if (i == iUnityScan) {
			pHEUnityRR->h_addr_list = ppcAddr;
			return (char *)0;
		}
	}
	pHEUnityRR->h_addr_list = ppcAddr;
	return tftp_ram((void *)0, fKeep, (char *)0);
}

/* dispose of the memory we got from a recover call			(ksb)
 * olny call if you set "fKeep" in the recover call!!
 */
void
unity_free(pcBlob)
char *pcBlob;
{
	if ((char *)0 != pcBlob) {
		free((void *)pcBlob);
	}
}

/* close the unity layer						(ksb)
 */
void
unity_close()
{
	tftp_close();
}

/* Publish a unity file so everyone can see it				(ksb)
 * The hard way would be to open the tftp chat ourselves -- not me.
 */
void
unity_offload(pcState)
char *pcState;
{
	register FILE *fpFifo;

%	if ((FILE *)0 != (fpFifo = fopen(%K<unity_fifo>1v, "wb"))) %{
		fprintf(fpFifo, "%s", pcState);
		fclose(fpFifo);
	}
	/* failed to off-load, sigh.  I hope we stay up! */
}
