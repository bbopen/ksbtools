/* $Id: util_cache.mc,v 8.35 2008/12/30 17:04:38 ksb Exp $
 * Cache a file into a mallloc'd space count and break the lines	(ksb)
 * into C strings, we'll read a pipe, but none-to-fast.  We require
 * string.h to get size_t. You can join seq lines by replacing the '\000's.
 * Return a pointer to the first line: calling free(3) on that return
 * value frees _all_ the lines for easy cleanup.  Growing file races used
 * to be a problem, now they are just slow (as we read 512 bytes/race).
 */
char *
%%K<cache_file>1v(int u_iFd, unsigned int *u_puLines)
{
	register char *pc, *pcMem;
	register int iCc;
	register size_t uCur, uMax, uFetch;
	register unsigned int uLines;
	auto struct stat stFd;

	if (-1 == fstat(u_iFd, & stFd)) {
%		fprintf(stderr, "%%s: fstat: %%d: %%s\n", %b, u_iFd, %E)%;
		return (char *)0;
	}
	uFetch = stFd.st_size;
	uMax = (uFetch|1023)+1;
	if ((void *)0 == (pcMem = (char *)malloc(uMax))) {
%		fprintf(stderr, "%%s: malloc: %%lu: %%s\n", %b, (unsigned long)uMax, strerror(errno))%;
		return (char *)0;
	}
	if (uFetch < 1) {
		uFetch = uMax/2;
	}
	for (uCur = 0; 0 < (iCc = read(u_iFd, pcMem+uCur, uFetch)); /*nada*/) {
		uCur += iCc, uFetch -= iCc;
		if (uFetch < 1024) {
			uFetch = 512;
		}
		if (uCur+uFetch > uMax) {
			uMax += 4096;
			pcMem = realloc((void *)pcMem, uMax);
		}
	}
	if (-1 == iCc) {
%		fprintf(stderr, "%%s: read: %%d: %%s\n", %b, u_iFd, %E)%;
		free((void *)pcMem);
		return (char *)0;
	}

	/* zip though the space, count and replace the '\n's
	 */
	pc = & pcMem[uCur];
	if (pc > pcMem && '\n' == pc[-1]) {
		--pc;
	}
	uLines = 0;
	while (pc > pcMem) {
		++uLines;
		*pc = '\000';
		while (pc > pcMem && '\n' != *pc)
			--pc;
	}

	if (pc != pcMem) {
%		fprintf(stderr, "%%s: cache_file: invarient broken\n", %b)%;
		pc = pcMem;
	}
	if ('\n' == *pc) {
		++uLines;
		*pc = '\000';
	}
	if ((unsigned *)0 != u_puLines) {
		*u_puLines = uLines;
	}
	return pcMem;
}

/* Convert the cached file to an argv vector				(ksb)
 * Using a cached file from above build a (char **) index into the lines.
 * Later we can unload the file to reclaim the space with the following func.
 * Return a (char **)0 when we can't allocate space.
 */
char **
%%K<cache_vector>1v(char *pcMem, unsigned int uLines)
{
	register char **ppcRet, **ppcArgv;

	if ((char *)0 == pcMem) {
		return (char **)0;
	}
	if ((char **)0 == (ppcRet = (char **)calloc(uLines+2, sizeof(char *)))) {
		return (char **)0;
	}
	*ppcRet++ = pcMem;
	ppcArgv = ppcRet;
	while (0 < uLines) {
		*ppcArgv++ = pcMem;
		pcMem = strchr(pcMem, '\000')+1;
		--uLines;
	}
	*ppcArgv = (char *)0;
	return ppcRet;
}

/* Dispose of the cached file, give me back the pointer you got		(ksb)
 * from the call to cache_vector, above.  You can mess with the vector
 * (qsort it, etc) but don't go beyond the ends (ppc[-1] is mine, dude).
 * Return 0 always.
 */
int
%%K<cache_dispose>1v(char **u_ppcVector)
{
	if ((char **)0 == u_ppcVector || (char *)0 == u_ppcVector[-1]) {
		return 0;
	}
	--u_ppcVector;
	free((void *)u_ppcVector[0]);
	u_ppcVector[0] = (char *)0;
	free((void *)u_ppcVector);
	return 0;
}
