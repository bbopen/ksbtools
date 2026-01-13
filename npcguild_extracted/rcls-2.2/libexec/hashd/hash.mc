/* $Id: hash.mc,v 2.1 2000/06/06 13:42:02 ksb Exp $
 * check the -b option's data						(ksb)
 */
static void
BitSweep(iCount, pcList)
int iCount;
char *pcList;
{
	if (iCount > BITS_PER_DIGIT * HASH_LENGTH || iCount < 1) {
%		fprintf(stderr, "%%s: -b: start %%d out of range (1 to %%d)\n", %b, iCount, BITS_PER_DIGIT * HASH_LENGTH)%;
		exit(1);
	}
	if (strlen(pcList) > iCount) {
%		fprintf(stderr, "%%s: -b: too many bits in `%%s'\n", %b, pcList)%;
		exit(2);
	}
}

/* output the hash for the present number and incr it			(ksb)
 */
static char *
NextHash(fdFile, pcHash)
int fdFile;
char *pcHash;	/* points to at least HASH_LENGTH+1 characters	*/
{
			/* the binary number from the file */
	auto char acValue[HASH_LENGTH*BITS_PER_DIGIT+8];
	register int i, cc;
	register char cScan;
	register HASH_MAP *pHM;
	register int iOrig;

	if (-1 == lockf(fdFile, F_LOCK, 0L)) {
		return (char *)0;
	}
	acValue[0] = '0';
	iOrig = read(fdFile, acValue+1, sizeof(acValue)-1);
	if (iOrig+1 < sizeof(acValue)) {
		acValue[iOrig+1] = '\000';
	}

	/* 0 lenght -> start a counter
	 */
	cScan = '\n';
	if (0 == iOrig) {
		cc = 1;
	} else for (cc = 1; cc < sizeof(acValue); ++cc) {
		cScan = acValue[cc];
		switch (cScan) {
		case '0':
		case '1':
			continue;
		case '\000':
		case '\n':
			break;
		default:
			return (char *)0;
		}
		break;
	}
	if ('\n' != cScan && '\000' != cScan) {
		return (char *)0;
	}
	acValue[cc] = '\000';
	/* INV: acValue must be a string matching /^0[01]*$/
	 */
	for (i = 0; i <= HASH_LENGTH; ++i) {
		pcHash[i] = 0;
	}

	for (i = cc, pHM = aHMMap; i > 0; ++pHM) {
		cScan = acValue[--i];
		if ('0' == cScan)
			continue;
		pcHash[pHM->ipos] |= pHM->ibit;
	}
	/* We need to skip the forced bits here ZZZ BUG XXX
	 * this bug wont show (at FedEx) until about 2^40th hashs
	 * have been given out.
	 */
	for (i = cc; '1' == acValue[--i]; ) {
		acValue[i] = '0';
	}
	acValue[i] = '1';
	acValue[cc++] = '\n';
	i = '0'  == acValue[0] && '\n' != acValue[1];
	if (0 != lseek(fdFile, 0L, SEEK_SET) || (cc-i) != write(fdFile, acValue+i, cc-i)) {
		return (char *)0;
	}
	if (0 != lseek(fdFile, 0L, SEEK_SET) || -1 == lockf(fdFile, F_ULOCK, 0L)) {
		return (char *)0;
	}
	if (fGaveB) for (pHM = & aHMMap[iStart-1]; '\000' != *pcBits; ++pcBits, --pHM) {
		cScan = *pcBits;
		if ('0' == cScan)
			pcHash[pHM->ipos] &= ~pHM->ibit;
		else if ('1' == cScan)
			pcHash[pHM->ipos] |= pHM->ibit;
	}
	for (i = 0; i < HASH_LENGTH; ++i) {
		pcHash[i] = acBase64[(int)pcHash[i]];
	}
	pcHash[i] = '\000';
	return pcHash;
}
