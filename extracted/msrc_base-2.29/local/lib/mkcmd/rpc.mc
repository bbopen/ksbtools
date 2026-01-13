/* $Id: rpc.mc,v 8.32 2000/11/01 16:49:32 ksb Exp $
 * C support for rpc program data type in mkcmd -- ksb
 */

/* look up the rpc entry, figure a size and copy the whole thing	(ksb)
 * into a single malloc'd block.  It is worth it to keep the callers
 * interface a simple as possible.  Really.  Then one can just free
 * the returned pointer, or not.
 */
struct rpcent *
%%K<rpc_cvt>1v(pcName, pcParam)
char *pcName, *pcParam;
{
	register struct rpcent *pRPFound;
	register char *pcScan, *pcType = "number";

	for (pcScan = pcName; '\000' != *pcScan; ++pcScan) {
		if (!isdigit(*pcScan)) {
			break;
		}
	}

	/* When the request was numeric and we didn't find it in the file
	 * build a fake one with the name "number" and the number,
	 * no aliases -- ksb
	 */
	if ('\000' != *pcScan) {
		pRPFound = getrpcbyname(pcName);
		pcType = "name";
	} else {
		register int iProg;
		static struct rpcent RPFake;

		iProg = atoi(pcName);
		pRPFound = getrpcbynumber(iProg);
		if ((struct rpcent *)0 == pRPFound && 0 != iProg) {
			RPFake.r_name = pcName;
			RPFake.r_number = iProg;
			RPFake.r_aliases = (char **)0;
			pRPFound = & RPFake;
		}
	}
	if ((struct rpcent *)0 == pRPFound) {
%		fprintf(stderr, "%%s: %%s: getrpcby%%s: %%s: %%s\n", %b, pcParam, pcType, pcName, %E)%;
		return (struct rpcent *)0;
	}
%	return %K<util_saverpcent>1v(pRPFound)%;
}
