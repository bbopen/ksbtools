/* $Id: util_fsearch.mc,v 8.10 1998/09/16 13:37:42 ksb Exp $
 */

/* we take a vector of possible lexicons and a $PATH mocking		(ksb)
 * list of places to search for them.  I'd rather not take the $PATH
 * compatible, but it is the most common use, so I used it.
 */
char *
%%K<util_fsearch>1v(ppcScan, pcPath)
char **ppcScan;
char *pcPath;
{
	register char *pcThis, *pcCursor, *pcWrite;
	auto char acFull[MAXPATHLEN+4];
	auto struct stat stDummy;
%	static char acDefPath[] = %K<util_fsearch_cvt>eqv%;

	if ((char *)0 == pcPath) {
		pcPath = acDefPath;
	}

	/* search lexicons for full paths ref, or
	 * relatives with each prefix tried
	 */
	while ((char *)0 != (pcThis = *ppcScan++)) {
		acFull[0] = '\000';
		if ('$' == pcThis[0]) {
			if ((char *)0 == (pcThis = getenv(pcThis+1))) {
				continue;
			}
		}
		if ('~' == pcThis[0]) {
%			pcThis = util_home(acFull, pcThis+1)%;
			if ((char *)0 == pcThis) {
				continue;
			}
			(void)strcat(acFull, pcThis);
		} else if ('/' == pcThis[0]) {
			(void)strcpy(acFull, pcThis);
		}
		if ('\000' != acFull[0]) {
			if (-1 != stat(acFull, & stDummy)) {
				return strdup(acFull);
			}
			continue;
		}

		/* search colon-separated prefix path
		 */
		pcWrite = (char *)0;
		for (pcCursor = pcPath; /**/; ++pcCursor) {
			if ((char *)0 == pcWrite) {
				if ('\000' == *pcCursor)
					break;
				pcWrite = acFull;
			}
			if (':' != *pcCursor && '\000' != *pcCursor) {
				*pcWrite++ = *pcCursor;
				continue;
			}
			if (acFull != pcWrite && pcWrite[-1] != '/') {
				*pcWrite++ = '/';
			}
			(void)strcpy(pcWrite, pcThis);
			if (-1 != stat(acFull, & stDummy)) {
				return strdup(acFull);
			}
			if ('\000' == *pcCursor) {
				break;
			}
			pcWrite = (char *)0;
		}
	}
	return (char *)0;
}
