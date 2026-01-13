/* $Id: cmd.mc,v 8.8 1999/06/12 00:43:58 ksb Exp $
 */

/* fill in the info in a CMDSET structure				(ksb)
 */
void
cmd_init(pCS, pCMCmds, iCount, pcPrompt, iAmbiguous)
CMDSET *pCS;
CMD *pCMCmds;
unsigned int iCount;
char *pcPrompt;
int iAmbiguous;
{
	register unsigned int i;
	register unsigned int iPad;

	if ((CMDSET *)0 == pCS || (0 != iCount && (CMD *)0 == pCMCmds)) {
%		fprintf(stderr, "%%s: cmd_init: nil pointer\n", %b)%;
		exit(1);
	}
	pCS->pcprompt = pcPrompt;
	pCS->pCMcmds = pCMCmds;
	pCS->icmds = iCount;
	pCS->piambiguous = (int *)calloc(iCount+1, sizeof(int));
	if ((int *)0 == pCS->piambiguous) {
%		fprintf(stderr, "%%s: not enough memory (%%d bytes)\n", %b, sizeof(int) * (iCount + 1))%;
		exit(1);
	}
	iPad = 1;
	for (i = 0; i < iCount; ++i) {
		register unsigned int iCur;
		iCur = strlen(pCMCmds[i].pccmd);
		pCS->piambiguous[i] = iAmbiguous > 0 && iAmbiguous < iCur ? iAmbiguous : iCur;
		if (iCur > iPad)
			iPad = iCur;
	}
	for (i = 0; i < iCount; ++i) {
		register unsigned int j, k;
		for (j = i+1; j < iCount; ++j) {
			if (0 != strncmp(pCMCmds[i].pccmd, pCMCmds[j].pccmd, pCS->piambiguous[i])) {
				continue;
			}
			for (k = pCS->piambiguous[i]; '\000' != pCMCmds[i].pccmd[k] && pCMCmds[i].pccmd[k] == pCMCmds[j].pccmd[k]; ++k)
				/* empty */;
			if (pCMCmds[i].pccmd[k] == pCMCmds[j].pccmd[k]) {
%				fprintf(stderr, "%%s: command `%%s' given in table more than once (only first accessible)\n", %b, pCMCmds[i].pccmd)%;
				break;
			}
			if ('\000' != pCMCmds[i].pccmd[k])
				pCS->piambiguous[i] = k+1;
			else
				pCS->piambiguous[j] = k+1;
		}
	}
	pCS->ipad = iPad;
}

/* hook for unknown commands						(ksb)
 */
static int
_cmd_unknown(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
%	fprintf(stderr, "%%s: %%s: unknown command\n", %b, argv[0])%;
	return 1;
}
int (*cmd_unknown)() = _cmd_unknown;
int u_bit_unknown = CMD_NULL;

/* hook for ambiguous commands						(ksb)
 */
static int
_cmd_ambiguous(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	/* we could call cmd_help here to list the options
	 */
%	fprintf(stderr, "%%s: %%s: ambiguous command\n", %b, argv[0])%;
	return 1;
}
int (*cmd_ambiguous)() = _cmd_ambiguous;
int u_bit_ambiguous = CMD_NULL;


/* hook for commands that are turned off				(ksb)
 */
static int
_cmd_not_available(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
%	fprintf(stderr, "%%s: %%s: not available\n", %b, argv[0])%;
	return 2;
}
int (*cmd_not_available)() = _cmd_not_available;
int u_bit_not_available = CMD_NULL;


/* process a command from a string, you can write on the string		(ksb)
 * break args the command
 */
int
cmd_from_string(pCS, pcCmd, piBits)
CMDSET *pCS;
char *pcCmd;
int *piBits;
{
	register CMD *pCM;
	auto int iArgc, iBits;
	auto char *apcArgv[2];
	auto char **ppcArgv;
	register int (*pfiFound)();
	register int iRet, i, iCmdLen;

	iArgc = 0;
	ppcArgv = apcArgv;
	apcArgv[0] = "zero";
	apcArgv[1] = (char *)0;
	u_envopt(pcCmd, & iArgc, & ppcArgv);

	/* look up the command keyword
	 */
	iCmdLen = strlen(ppcArgv[1]);
	pCM = pCS->pCMcmds;

	pfiFound = cmd_unknown;
	iBits = u_bit_unknown;
	for (i = 0; i < pCS->icmds; ++i) {
		if (0 != strncmp(pCM[i].pccmd, ppcArgv[1], iCmdLen)) {
			continue;
		}
		if (iCmdLen < pCS->piambiguous[i]) {
			pfiFound = cmd_ambiguous;
			iBits = u_bit_ambiguous;
			continue;
		}
		if (0 != (pCM[i].facts & CMD_OFF)) {
			pfiFound = cmd_not_available;
			iBits = u_bit_not_available;
		} else  {
			pfiFound = pCM[i].pfi;
			iBits = pCM[i].facts;
		}
		ppcArgv[1] = pCM[i].pccmd;
		break;
	}
	if ((int (*)())0 != pfiFound) {
		iRet = (*pfiFound)(iArgc-1, ppcArgv+1, pCS);
	} else {
		iRet = 0;
	}
	if ((int *)0 != piBits) {
		*piBits = iBits;
	}
	if (0 == (iBits & CMD_NO_FREE)) {
		free((char *)ppcArgv);
	}
	return iRet;
}

/* read commands from a file						(ksb)
 */
int
cmd_from_file(pCS, fp)
CMDSET *pCS;
FILE *fp;
{
	auto FILE *fpPrompt;
	auto char *pcPrompt;
	auto int iRet, fBits;
#if !USE_FGETLINE
	auto char acText[1024];
#endif
	/* we need that 4.4 readline widgit! */

	fBits = iRet = 0;

	fpPrompt = isatty(fileno(stdout)) ? stdout : stderr;
	if (!isatty(fileno(fp)) || !isatty(fileno(fpPrompt))) {
		pcPrompt = (char *)0;
	} else if ((char *)0 != pCS->pcprompt) {
		pcPrompt = pCS->pcprompt;
	} else {
%		pcPrompt = malloc((strlen(%b)|7)+9)%;
		if ((char *)0 != pcPrompt) {
%			(void)strcpy(pcPrompt, %b)%;
			(void)strcat(pcPrompt, "> ");
		}
	}
	do {
		register char *pcEoln;
		if ((char *)0 != pcPrompt) {
			fputs(pcPrompt, fpPrompt);
			fflush(fpPrompt);
		}
#if USE_FGETLINE
		if ((char *)0 == (pcEoln = fgetline(fp, (size_t *)0))) {
			if (EINTR == errno)
				continue;
			break;
		}
#else
		if ((char *)0 == fgets(acText, sizeof(acText), fp)) {
			if (EINTR == errno)
				continue;
			break;
		}
		if ((char *)0 != (pcEoln = strchr(acText, '\n'))) {
			*pcEoln = '\000';
		}
		pcEoln = acText;
#endif
		for (/* not common */; isspace(*pcEoln); ++pcEoln) {
			/* nothing */;
		}
		if ('\000' == *pcEoln) {
			continue;
		}
		iRet |= cmd_from_string(pCS, pcEoln, & fBits);
		if (0 != (fBits & CMD_RET))
			break;
	} while (!feof(fp));
	if ((char *)0 != pcPrompt && pcPrompt != pCS->pcprompt) {
		free(pcPrompt);
	}
	return iRet;
}
