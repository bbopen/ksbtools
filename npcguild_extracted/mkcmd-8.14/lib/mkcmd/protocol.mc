/* $Id: protocol.mc,v 8.1 1998/07/06 00:06:10 ksb Exp $
 * C support for the protocol data type in mkcmd -- ksb
 */

/* Look up the protocol entry, figure a size and copy the whole thing	(ksb)
 * into a single malloc'd block. Just like a hostent.  We convert:
 *	protocol
 *	number
 */
struct protoent *
%%K<protocol_cvt>1v(pcName, pcParam)
char *pcName, *pcParam;
{
	register struct protoent *pPEFound, *pPEMem;
	register int iMem, iAliases, i;
	register char *pcStack;
	register int iFudge = sizeof(long int) - 1;
	register void *pvMem;
	extern int errno;

	for (pcStack = pcName; '\000' != *pcStack; ++pcStack) {
		if (!isdigit(*pcStack))
			break;
	}
	/* LLL there should be help code here for "?" as a protocol
	 * (included on a key %@if test) -- ksb.
	 */
	if ('\000' == *pcStack) {
		pPEFound = getprotobynumber(atoi(pcName));
	} else {
		pPEFound = getprotobyname(pcName);
	}
	if ((struct protoent *)0 == pPEFound) {
%		fprintf(stderr, "%%s: %%s: getprotobyname: %%s: %%s\n", %b, pcParam, pcName, %E)%;
		return (struct protoent *)0;
	}

	iMem = (strlen(pPEFound->p_name)|iFudge)+1;
	for (iAliases = 0; (char *)0 != pPEFound->p_aliases[iAliases]; ++iAliases) {
		iMem += (strlen(pPEFound->p_aliases[iAliases])|iFudge)+1;
	}

	/* ok we have his size, now build a copy we can keep
	 */
	iMem += (sizeof(struct protoent) +
		(iAliases + 1) * sizeof(char *) +
		sizeof(char *)) | iFudge ;

	if ((void *)0 == (pvMem = calloc(iMem+1, sizeof(char)))) {
%		fprintf(stderr, "%%s: %%s: calloc: %%d: %%s\n", %b, pcParam, iMem, %E)%;
		return (struct protoent *)0;
	}
	pPEMem = (struct protoent *)pvMem;
	pPEMem->p_proto = pPEFound->p_proto;
	pPEMem->p_aliases = (char **)(pPEMem+1);
	pcStack = (char *)& pPEMem->p_aliases[iAliases+1];
	pPEMem->p_name = strcpy(pcStack, pPEFound->p_name);
	pcStack += (strlen(pcStack)|iFudge)+1;

	for (i = 0; i < iAliases; ++i) {
		pPEMem->p_aliases[i] = strcpy(pcStack, pPEFound->p_aliases[i]);
		pcStack += (strlen(pcStack)|iFudge)+1;
	}
	pPEMem->p_aliases[i] = (char *)0;

	return pPEMem;
}
