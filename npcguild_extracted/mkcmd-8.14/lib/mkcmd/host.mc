/* $Id: host.mc,v 8.5 1999/06/13 17:18:29 ksb Exp $
 * C support for host data type in mkcmd -- ksb
 */

/* look up the host entry, figure a size and copy the whole thing	(ksb)
 * into a single malloc'd block.  It is worth it to keep the callers
 * interface a simple as possible.  Really.  Then one can just free
 * the returned pointer, or not.
 *
 * Accept "*" as "INADDR_ANY".  Strange hack, that.
 */
struct hostent *
%%K<host_cvt>1v(pcName, pcParam)
char *pcName, *pcParam;
{
	register struct hostent *pHEFound;
	register char *pcScan;
	register unsigned char *puc;
	register int iC;
	auto struct in_addr IATemp;
	auto int iDec;
	extern int errno;

	for (pcScan = pcName; '\000' != *pcScan; ++pcScan) {
		if ('.' != *pcScan && !isdigit(*pcScan)) {
			break;
		}
	}

	if ('\000' != *pcScan) {
		pHEFound = gethostbyname(pcName);
	} else {
		puc = (unsigned char *)& IATemp.s_addr;
		pcScan = pcName;
		for (iC = 0; iC < 4; ++iC) {
			if ((char *)0 == pcScan || '\000' == pcScan[0]) {
%				fprintf(stderr, "%%s: %%s: gethostbyname: %%s: not an IPv4 address\n", %b, pcParam, pcName)%;
				return (struct hostent *)0;
			}
			sscanf(pcScan, "%d", & iDec);
			if ((char *)0 != (pcScan = strchr(pcScan, '.'))) {
				++pcScan;
			}
			*puc++ = iDec & 0xff;
		}
		if ((char *)0 != pcScan) {
%			fprintf(stderr, "%%s: %%s: gethostbyname: %%s: extra stuff on IPv4 address\n", %b, pcParam, pcName)%;
			return (struct hostent *)0;
		}
		pHEFound = gethostbyaddr((char *)& IATemp, sizeof(IATemp), AF_INET);
	}
	if ((struct hostent *)0 == pHEFound) {
%		fprintf(stderr, "%%s: %%s: gethostbyname: %%s: %%s\n", %b, pcParam, pcName, %E)%;
		return (struct hostent *)0;
	}
%	return %K<util_savehostent>1v(pHEFound)%;
}
