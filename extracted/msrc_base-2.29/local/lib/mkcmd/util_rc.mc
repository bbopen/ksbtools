/* $Id: util_rc.mc,v 8.7 1997/10/27 11:55:22 ksb Exp $
 */

/* the file in question must look like it was presented on the
 * User's command line someplace.  We force it with an init key.
 * The client file conversion _must_ have a
 *	before "%K<RcInit>/ef;"
 */
%#if !defined(%K<RcInit>1v)
%#define %K<RcInit>1v(Mf)	((Mf) = 1)
#endif

/* search for the filename requested by any of				(ksb)
 * the common aliases in pcRcPath:
 *	${KEEP}		(if given)
 * then in each directory
 *	${BASE}
 *	${BASE}rc
 *	.${BASE}rc
 *
 * If ppcKeep is provided we stash the path we opened in there
 * (malloc'ing space).
 * If it (*ppcKeep) gives up a full path we'll just try that one.
 *
 * This function is called from the client file as
 *	late update '%n = %K<RcOpen>/ef;'
 * the unnamed key in the option should have (as value 1) the
 * default basename to try.
 */
FILE *
%%K<RcOpen>1v(pcBasename, ppcKeep, ppcTemplates)
char *pcBasename, **ppcKeep, **ppcTemplates;
{
	extern char *getenv();
	register char *pcKeep, *pcColon, *pcScanPath;
	register FILE *fpRet;
	register int iLen;
	auto char acProbe[MAXPATHLEN+1];
	static char *apcDefTemp[] = {
		/* "%s", "%src", ".%src", (char *)0 */
%		%K<RcFormats>eq+", "x(char *)0
	};

	if ((char **)0 == ppcTemplates) {
		ppcTemplates = apcDefTemp;
	}
	pcKeep = (char **)0 != ppcKeep ? *ppcKeep : (char *)0;
	if ((char *)0 != pcKeep && ('/' == pcKeep[0] || ('.' == pcKeep[0] && '/' == pcKeep[1]))) {
		(void)strcpy(acProbe, pcKeep);
		fpRet = fopen(pcKeep, "r");
	} else if ((char *)0 == (pcScanPath = pcRcPath) || '\000' == pcScanPath[0]) {
		return (FILE *)0;
	} else {
		register char **ppcScan;
		if ((char *)0 != pcKeep && '\000' != pcKeep[0]) {
			pcBasename = pcKeep;
		} else if ((char *)0 == pcBasename || '\000' == pcBasename[0]) {
%			pcBasename = %b%;
		}
		if ((char *)0 == pcBasename || '\000' == pcBasename[0]) {
			return (FILE *)0;
		}

		/* N.B. iLen and pcColon are used to update the
		 * loop, do not touch after set a top of loop
		 */
		for (fpRet = (FILE *)0; '\000' != *pcScanPath; pcScanPath += iLen + ((char *)0 != pcColon)) {
			register char *pcTail;
			register int iTail, iMark;

			iLen = (char *)0 == (pcColon = strchr(pcScanPath, ':')) ?  strlen(pcScanPath) : pcColon - pcScanPath;

			/* Check for leading ~ on rc path,
			 * we (than) have to check for valid login name
			 * or homedir ($HOME).
			 *
			 * Mend the rest when util_home didn't take all
			 */
			if ('~' != pcScanPath[0]) {
				(void)strncpy(acProbe, pcScanPath, iLen);
				acProbe[iLen] = '\000';
			} else if ((char *)0 == (pcTail = util_home(acProbe, pcScanPath+1))) {
%				fprintf(stderr, "%%s: getrc: %%s: no such login\n", %b, acProbe)%;
				continue;
			} else if ('\000' != *pcTail) {
				register int iSpan;

				iTail = iLen - (pcTail - pcScanPath);
				iSpan = strlen(acProbe);
				(void)strncpy(acProbe+iSpan, pcTail, iTail);
				acProbe[iSpan+iTail] = '\000';
			}

			iMark = strlen(acProbe);
			acProbe[iMark++] = '/';
			for (ppcScan = ppcTemplates; (char *)0 != *ppcScan; ++ppcScan) {
				(void)sprintf(acProbe+iMark, *ppcScan, pcBasename);
				if ((FILE *)0 != (fpRet = fopen(acProbe, "r"))) {
					break;
				}
			}
			if ((FILE *)0 != fpRet) {
				break;
			}
		}
	}
	if ((FILE *)0 != fpRet && (char **)0 != ppcKeep) {
		pcKeep = malloc((strlen(acProbe)|7)+1);
		if ((char *)0 != pcKeep) {
			(void)strcpy(pcKeep, acProbe);
		}
		*ppcKeep = pcKeep;
	} else if ((char **)0 != ppcKeep) {
		*ppcKeep = (char *)0;
	}
	return fpRet;
}

