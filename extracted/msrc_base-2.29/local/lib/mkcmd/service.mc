/* $Id: service.mc,v 8.3 2000/08/31 21:55:49 ksb Exp $
 * C support for the service data type in mkcmd -- ksb
 */

/* Look up the service entry, figure a size and copy the whole thing	(ksb)
 * into a single malloc'd block. Just like a hostent.  We convert:
 *	service
 *	number
 *	service/proto
 *	number/proto
 * default proto is the second key value (tcp might be a good one)
 * Since we can't write on the pcName given we must copy out the name.
 */
struct servent *
%%K<service_cvt>1v(pcName, pcParam)
char *pcName, *pcParam;
{
	register struct servent *pSEFound;
	register char *pcScan, *pcProto, *pcTemp;
	register int iFudge = sizeof(long int) - 1;
%	static char acDefProto[] = "%qK<service_cvt>2v"%;
	extern int errno;

	pcTemp = calloc((strlen(pcName)|iFudge)+1, sizeof(char));
	(void)strcpy(pcTemp, pcName);
	if ((char *)0 == (pcProto = strchr(pcTemp, '/'))) {
		pcProto = acDefProto;
	} else {
		*pcProto++ = '\000';;
	}
	if ('\000' == pcProto) {
		pcProto = acDefProto;
	}

	for (pcScan = pcTemp; '\000' != *pcScan; ++pcScan) {
		if (!isdigit(*pcScan))
			break;
	}
	if ('\000' != *pcScan) {
		pSEFound = getservbyname(pcTemp, pcProto);
	} else if ((struct servent *)0 == (pSEFound = getservbyport(htons(atoi(pcTemp)), pcProto))) {
		register char *pcMem;

		pSEFound = calloc(1, ((sizeof(struct servent) + sizeof(char *) + 32 + strlen(pcProto))|15)+1);
		pcMem = (char *)& pSEFound[1];
		pSEFound->s_port = atoi(pcTemp);
		pSEFound->s_aliases = (char **)pcMem;
		pcMem += sizeof(char *);
		pSEFound->s_aliases[0] = (char *)0;
		pSEFound->s_name = pcMem;
		sprintf(pcMem, "%d", pSEFound->s_port);
		pcMem += strlen(pcMem)+1;
		pSEFound->s_proto = strcpy(pcMem, pcProto);
	}
	if ((struct servent *)0 == pSEFound) {
%		fprintf(stderr, "%%s: %%s: getservbyname: %%s/%%s: %%s\n", %b, pcParam, pcTemp, pcProto, %E)%;
		free(pcTemp);
		return (struct servent *)0;
	}
	free(pcTemp);

%	return %K<util_saveservent>1v(pSEFound)%;
}
