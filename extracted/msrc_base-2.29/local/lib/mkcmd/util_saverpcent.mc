/* $Id: util_saverpcent.mc,v 8.31 2000/11/01 16:49:32 ksb Exp $
 * C support for rpc data type in mkcmd -- ksb
 */

/* Deep copy a rpcent structure into a fresh buffer			(ksb)
 * One can safely free the returned pointer to recover all the memory.
 */
struct rpcent *
%%K<util_saverpcent>1v(pRPSource, pcParam)
struct rpcent *pRPSource;
char *pcParam;
{
	register struct rpcent *pRPMem;
	register int iMem, iAliases, i;
	register char *pcStack;
	register int iFudge = sizeof(long int) - 1;
	register void *pvMem;

	if ((struct rpcent *)0 == pRPSource) {
		return (struct rpcent *)0;
	}

	iMem = (strlen(pRPSource->r_name)|iFudge)+1;
	iAliases = 0;
	if ((char **)0 != pRPSource->r_aliases) for (; (char *)0 != pRPSource->r_aliases[iAliases]; ++iAliases) {
		iMem += (strlen(pRPSource->r_aliases[iAliases])|iFudge)+1;
	}

	/* ok we have his size, now build a copy we can keep
	 */
	iMem += (sizeof(struct rpcent) + (iAliases + 1) * sizeof(char *));
	iMem |= iFudge;
	++iMem;

	if ((void *)0 == (pvMem = calloc(iMem, sizeof(char)))) {
%		fprintf(stderr, "%%s: %%s: calloc: %%d: %%s\n", %b, pcParam, iMem, %E)%;
		return (struct rpcent *)0;
	}
	pRPMem = (struct rpcent *)pvMem;
	pRPMem->r_number = pRPSource->r_number;
	pRPMem->r_aliases = (char **)(pRPMem+1);
	pcStack = (char *) (& pRPMem->r_aliases[iAliases+1]);
	pRPMem->r_name = strcpy(pcStack, pRPSource->r_name);
	pcStack += (strlen(pcStack)|iFudge)+1;

	for (i = 0; i < iAliases; ++i) {
		pRPMem->r_aliases[i] = strcpy(pcStack, pRPSource->r_aliases[i]);
		pcStack += (strlen(pcStack)|iFudge)+1;
	}
	pRPMem->r_aliases[i] = (char *)0;

	return pRPMem;
}
