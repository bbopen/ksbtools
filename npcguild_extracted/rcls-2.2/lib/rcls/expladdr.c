/*@Version $Version$@*/
/*@Header@*/

/*@Explode@*/
/* the list must have all of the addresses in the entry's address list	(ksb)
 */
static int
ELCanInclude(pHEList, pHEEntry)
struct hostent *pHEList, *pHEEntry;
{
	register int i, j;
	register char *pcCur, *pcCheck;

	for (i = 0; (char *)0 != (pcCur = pHEEntry->h_addr_list[i]); ++i) {
		for (j = 0; (char *)0 != (pcCheck = pHEList->h_addr_list[j]); ++j) {
			if (pcCur[0] != pcCheck[0] || pcCur[1] != pcCheck[1] || pcCur[2] != pcCheck[2] || pcCur[3] != pcCheck[3])
				continue;
			break;
		}
		if ((char *)0 == pcCheck) {
			return 0;
		}
	}
	return 1;
}

/* $Id: expladdr.c,v 2.0 2000/06/05 22:55:08 ksb Exp $
 *
 * Find explicit addresses for each A record on the input hostname	(ksb)
 * Needs mkcmd's util_savehostent.m (see expladdr.m), you must call
 * that on the hostent * you get from gethostbyname (or the like) to
 * pass to us.  We do not free it either (since you allocated it).
 *
 * You have to free all the strings we made.  Hey, it's your problem to
 * free those, and the index vector as well.
 *
 * When we can't reverse it, or the reverse has an address NOT in the list
 * on it we insert the IP address as a string.
 *
 * XXX When the host has more than one address and you ask for the canonical
 * name (www1.sac) then you get that name back, not the list of the addresses.
 * If you want the addresses use "gethostbyname":  we return a list of names.
 *
 * LLL A useful mutation of this would be one that returned the IP's as
 * text when there was more than one.
 */
char **
ExplAddr(pHESource)
struct hostent *pHESource;
{
	register int i, iAddrs, j;
	register char **ppcRet, *pcName;
	register struct hostent *pHEAlias, *pHEName;

	if ((struct hostent *)0 == pHESource) {
		if ((char **)0 != (ppcRet = util_ppm_size(& PPMAddr, 1))) {
			ppcRet[0] = (char *)0;
		}
		return ppcRet;
	}
	for (iAddrs = 0; (char *)0 != pHESource->h_addr_list[iAddrs]; ++iAddrs) {
		/* count them */
	}
	ppcRet = (char **)util_ppm_size(& PPMAddr, iAddrs+1);
	/* find the name we need for the host
	 */
	for (i = iAddrs = 0; (char *)0 != pHESource->h_addr_list[iAddrs]; ++iAddrs) {
		register unsigned char *puc;
		pHEName = gethostbyaddr(pHESource->h_addr_list[iAddrs], pHESource->h_length, pHESource->h_addrtype);
		pHEAlias = (struct hostent *)0 != pHEName ? gethostbyname(pHEName->h_name) : (struct hostent *)0;
		if ((struct hostent *)0 != pHEAlias && ELCanInclude(pHESource, pHEAlias)) {
			ppcRet[i++] = strdup(pHEAlias->h_name);
			continue;
		}
		pcName = malloc(((4*4)|7)+1);
		puc = (unsigned char *)pHESource->h_addr_list[iAddrs];
		sprintf(pcName, "%d.%d.%d.%d", puc[0], puc[1], puc[2], puc[3]);
		ppcRet[i++] = pcName;
	}
	ppcRet[i] = (char *)0;

	/* remove dups if we get through the inner loop then the
	 * name is unique, add it.
	 * N.B. Do NOT sort these as it reduces the RR usefulness a lot. (mad)
	 */
	iAddrs = 0;
	for (i = 0; (char *)0 != ppcRet[i]; ++i) {
		for (j = i+1; (char *)0 != ppcRet[j]; ++j) {
			if (0 == strcmp(ppcRet[i], ppcRet[j]))
				break;
		}
		if ((char *)0 != ppcRet[j]) {
			continue;
		}
		ppcRet[iAddrs++] = ppcRet[i];

	}
	ppcRet[iAddrs] = (char *)0;
	return ppcRet;
}
