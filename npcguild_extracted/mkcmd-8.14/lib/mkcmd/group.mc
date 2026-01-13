/* $Id: group.mc,v 1.2 1998/09/20 18:40:04 ksb Exp $
 * C support for group data type in mkcmd -- ksb
 */

/* look up the group entry, figure a size and copy the whole thing	(ksb)
 * into a single malloc'd block.  It is worth it to keep the callers
 * interface a simple as possible.  Really.  Then one can just free
 * the returned pointer, or not.
 */
struct group *
%%K<group_cvt>1v(pcName, pcParam)
char *pcName, *pcParam;
{
	register struct group *pGEFound;
	register char *pcScan;
	extern int errno;

	for (pcScan = pcName; '\000' != *pcScan; ++pcScan) {
		if (!isdigit(*pcScan)) {
			break;
		}
	}
%@if group_help
	/* check a key for help token XXX -- ksb
	 */
%@endif

	if ('\000' != *pcScan) {
		pGEFound = getgrnam(pcName);
	} else {
		pGEFound = getgrgid((gid_t)atoi(pcName));
	}
	if ((struct group *)0 == pGEFound) {
%		fprintf(stderr, "%%s: %%s: getgrent: %%s: %%s\n", %b, pcParam, pcName, %E)%;
		return (struct group *)0;
	}

%	return %K<util_savegrent>1v(pGEFound)%;
}
