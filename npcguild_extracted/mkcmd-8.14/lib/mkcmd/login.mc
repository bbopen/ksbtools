/* $Id: login.mc,v 1.2 2000/07/30 20:16:58 ksb Exp $
 * C support for login data type in mkcmd -- ksb
 */

/* look up the login entry, figure a size and copy the whole thing	(ksb)
 * into a single malloc'd block.  It is worth it to keep the callers
 * interface a simple as possible.  Really.  Then one can just free
 * the returned pointer, or not.
 */
struct passwd *
%%K<login_cvt>1v(pcName, pcParam)
char *pcName, *pcParam;
{
	register struct passwd *pPWFound;
	register char *pcScan;
	extern int errno;

	for (pcScan = pcName; '\000' != *pcScan; ++pcScan) {
		if (!isdigit(*pcScan)) {
			break;
		}
	}
%@if login_help
	/* check for the help token
	 */
%	if (0 == strcmp(pcName, %K<login_help>1qv)) %{
%		printf("%%s: %%s: provide a login name from the list in /etc/passwd\n", %b, pcParam)%;
		exit(0);
	}
%@endif

	if ('\000' != *pcScan) {
		pPWFound = getpwnam(pcName);
	} else {
		pPWFound = getpwuid((uid_t)atoi(pcName));
	}
	if ((struct passwd *)0 == pPWFound) {
%		fprintf(stderr, "%%s: %%s: getpwent: %%s: %%s\n", %b, pcParam, pcName, %E)%;
		return (struct passwd *)0;
	}
%	return %K<util_savepwent>1v(pPWFound)%;
}
