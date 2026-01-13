/* $Id: util_saveservent.mc,v 1.1 1999/06/13 18:10:13 ksb Exp $
 */

/* Deep copy a service entry into malloc'd space			(ksb)
 * We loose if anyone adds stuff to a (struct servent).
 */
struct servent *
%%K<util_saveservent>1v(pSEFound)
struct servent *pSEFound;
{
	register struct servent *pSEMem;
	register int iMem, iAliases, i;
	register char *pcStack;
	register int iFudge = sizeof(long int) - 1;
	register void *pvMem;
	extern int errno;

	if ((struct servent *)0 == pSEFound) {
		return (struct servent *)0;
	}

	iMem = (strlen(pSEFound->s_name)|iFudge)+1;
	iMem += (strlen(pSEFound->s_proto)|iFudge)+1;
	for (iAliases = 0; (char *)0 != pSEFound->s_aliases[iAliases]; ++iAliases) {
		iMem += (strlen(pSEFound->s_aliases[iAliases])|iFudge)+1;
	}

	/* ok we have his size, now build a copy we can keep
	 */
	iMem += (sizeof(struct servent) +
		(iAliases + 1) * sizeof(char *) +
		sizeof(char *)) | iFudge ;

	if ((void *)0 == (pvMem = calloc(iMem+1, sizeof(char)))) {
%		fprintf(stderr, "%%s: calloc: %%d: %%s\n", %b, iMem, %E)%;
		return (struct servent *)0;
	}
	pSEMem = (struct servent *)pvMem;
	pSEMem->s_port = pSEFound->s_port;
	pSEMem->s_aliases = (char **)(pSEMem+1);
	pcStack = (char *)&  pSEMem->s_aliases[iAliases+1];
	pSEMem->s_name = strcpy(pcStack, pSEFound->s_name);
	pcStack += (strlen(pcStack)|iFudge)+1;
	pSEMem->s_proto = strcpy(pcStack, pSEFound->s_proto);
	pcStack += (strlen(pcStack)|iFudge)+1;

	for (i = 0; i < iAliases; ++i) {
		pSEMem->s_aliases[i] = strcpy(pcStack, pSEFound->s_aliases[i]);
		pcStack += (strlen(pcStack)|iFudge)+1;
	}
	pSEMem->s_aliases[i] = (char *)0;

	return pSEMem;
}
