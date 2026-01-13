/* $Id: cmd_parse.mc,v 8.7 1997/10/27 11:55:22 ksb Exp $
 */

/* read commands from a file (these commands are usually binary		(ksb)
 * programs that we fork/execve when evaluated).
 *	[!] [@] path argv0 [params] -- helptext
 */
int
cmd_parse(fp, ppCM, piCount)
FILE *fp;
CMD **ppCM;
unsigned int *piCount;
{
	register char *pcEoln, *pcMem;
	auto int fActs;
#if !USE_FGETLINE
        auto char acText[4096];
#endif
	auto unsigned iMax, iCur;
	auto CMD *pCM;

	iMax = 16;
	iCur = 0;
	pCM = (CMD *)calloc(iMax, sizeof(CMD));
	for (;;) {
		register char *pcPath, *pcArgv0, *pcParams;
		register unsigned int uLen;
#if USE_FGETLINE
		if ((char *)0 == (pcEoln = fgetline(fp, (size_t *)0))) {
			break;
		}
#else
		if ((char *)0 == fgets(acText, sizeof(acText), fp)) {
			break;
		}
		if ((char *)0 != (pcEoln = strchr(acText, '\n'))) {
			*pcEoln = '\000';
		}
		pcEoln = acText;
#endif
		while (isspace(*pcEoln)) {
			++pcEoln;
		}
		if ('#' == *pcEoln || '\000' == *pcEoln) {
			continue;
		}
		fActs = 0;
		while ('!' == *pcEoln || '@' == *pcEoln || isspace(*pcEoln)) {
			register char c = *pcEoln++;
			if (isspace(c)) {
				continue;
			}
			fActs |= '!' == c ? CMD_OFF : CMD_HIDDEN;
		}
		pcPath = pcEoln++;
		while (!isspace(*pcEoln) && '\000' != *pcEoln)
			++pcEoln;
		if ('\000' == *pcEoln) {
			return 2;
		}
		*pcEoln++ = '\000';
		while (isspace(*pcEoln)) {
			++pcEoln;
		}

		/* set the command name if none provided, else
		 * take the first word of the usage
		 */
		pcArgv0 = pcEoln;
		while ('\000' != *pcEoln) {
			register char *pcHack;
			if ('-' != pcEoln[0] || '\000' == pcEoln[1]) {
				++pcEoln;
				continue;
			}
			if ('-' != pcEoln[1]) {
				pcEoln += 2;
				continue;
			}
			/* inv. found -- token */
			pcHack = pcEoln-1;
			while (pcHack > pcArgv0 && isspace(*pcHack)) {
				*pcHack-- = '\000';
			}
			*pcEoln++ = '\000';
			*pcEoln++ = '\000';
			break;
		}
		if ('\000' == pcArgv0[0]) {
			if ((char *)0 == (pcArgv0 = strrchr(pcPath, '/')))
				pcArgv0 = pcPath;
			else
				++pcArgv0;
		}
		while (isspace(*pcEoln)) {
			++pcEoln;
		}
		pcParams = pcArgv0;
		while (!isspace(*pcParams) && '\000' != *pcParams) {
			++pcParams;
		}
		while (isspace(*pcParams)) {
			*pcParams++ = '\000';
		}
		if ('\000' == *pcParams) {
			pcParams = (char *)0;
		}
		if (iCur == iMax) {
			iMax += 32;
			pCM = (CMD *)realloc((void *)pCM, sizeof(CMD)*iMax);
		}
		uLen = (strlen(pcArgv0)+1+strlen(pcPath)+strlen(pcEoln)+1+((char *)0 == pcParams ? 0 : strlen(pcParams)+1));
		pcMem = malloc((uLen|7)+1);
		if ((CMD *)0 == pCM || (char *)0 == pcMem) {
%			fprintf(stderr, "%%s: not enough memory\n", %b)%;
			return 4;
		}
		pCM[iCur].pccmd = pcMem;
		(void)strcpy(pcMem, pcArgv0);
		pcMem += strlen(pcMem)+1;
		(void)strcpy(pcMem, pcPath);
		pcMem += strlen(pcMem)+1;

		if ((char *)0 != (pCM[iCur].pcparams = pcParams)) {
			pCM[iCur].pcparams = pcMem;
			(void)strcpy(pcMem, pcParams);
			pcMem += strlen(pcMem)+1;
		}

		pCM[iCur].pcdescr = pcMem;
		(void)strcpy(pcMem, pcEoln);

		pCM[iCur].pfi = cmd_fs_exec;
		pCM[iCur++].facts = fActs;
	}
	*ppCM = pCM;
	*piCount = iCur;

	return 0;
}
