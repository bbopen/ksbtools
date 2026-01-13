/* $Id: util_savehostent.mc,v 1.2 1999/06/13 17:18:29 ksb Exp $
 * C support for host data type in mkcmd -- ksb
 */

/* Deep copy a hostent structure into a fresh buffer			(ksb)
 */
struct hostent *
%%K<util_savehostent>1v(pHESource, pcParam)
struct hostent *pHESource;
char *pcParam;
{
	register struct hostent *pHEMem;
	register int iMem, iAliases, iAddrs, i;
	register char *pcStack;
	register int iFudge = sizeof(long int) - 1;
	register void *pvMem;
	extern int errno;

	if ((struct hostent *)0 == pHESource) {
		return (struct hostent *)0;
	}

	iMem = (strlen(pHESource->h_name)|iFudge)+1;
	for (iAliases = 0; (char *)0 != pHESource->h_aliases[iAliases]; ++iAliases) {
		iMem += (strlen(pHESource->h_aliases[iAliases])|iFudge)+1;
	}
	for (iAddrs = 0; (char *)0 != pHESource->h_addr_list[iAddrs]; ++iAddrs) {
		/* count them */
	}

	/* ok we have his size, now build a copy we can keep
	 */
	iMem += (sizeof(struct hostent) +
		(iAliases + 1) * sizeof(char *)	+
		(iAddrs + 1) * sizeof(char *) +
		sizeof(char *) + pHESource->h_length * iAddrs);
	iMem |= iFudge;
	++iMem;

	if ((void *)0 == (pvMem = calloc(iMem, sizeof(char)))) {
%		fprintf(stderr, "%%s: %%s: calloc: %%d: %%s\n", %b, pcParam, iMem, %E)%;
		return (struct hostent *)0;
	}
	pHEMem = (struct hostent *)pvMem;
	pHEMem->h_addrtype = pHESource->h_addrtype;
	pHEMem->h_length = pHESource->h_length;
	pHEMem->h_aliases = (char **)(pHEMem+1);
	pHEMem->h_addr_list = &  pHEMem->h_aliases[iAliases+1];
	pcStack = (char *) (& pHEMem->h_addr_list[iAddrs+1]);
	pHEMem->h_name = strcpy(pcStack, pHESource->h_name);
	pcStack += (strlen(pcStack)|iFudge)+1;

	for (i = 0; i < iAliases; ++i) {
		pHEMem->h_aliases[i] = strcpy(pcStack, pHESource->h_aliases[i]);
		pcStack += (strlen(pcStack)|iFudge)+1;
	}
	pHEMem->h_aliases[i] = (char *)0;

	for (i = 0; i < iAddrs; ++i) {
#if USE_BCOPY
		bcopy(pHESource->h_addr_list[i], pcStack, pHESource->h_length);
#else
		memcpy(pcStack, pHESource->h_addr_list[i], pHESource->h_length);
#endif
		pHEMem->h_addr_list[i] = pcStack;
		pcStack += pHESource->h_length;
	}
	pHEMem->h_addr_list[i] = (char *)0;

	return pHEMem;
}
