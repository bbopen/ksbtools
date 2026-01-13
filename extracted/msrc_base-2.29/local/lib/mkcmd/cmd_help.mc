/* $Id: cmd_help.mc,v 8.7 1997/10/27 11:55:22 ksb Exp $
 */

/* the routine to format help text					(ksb)
 */
static int
cmd_help(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	register CMD *pCM;
	register unsigned int i, j;
	register int iRet, iCmdLen;

	iRet = 0;
	pCM = pCS->pCMcmds;
	if (1 == argc) {
%@foreach "help_banner"
%%Xa
%@endfor
		for (j = i = 0; i < pCS->icmds; ++i) {
			if (0 != ((CMD_HIDDEN|CMD_OFF) & pCM[i].facts))
				continue;
			if ((char *)0 == pCM[i].pcdescr) {
				continue;
			}
			printf("%-*s %s\n", pCS->ipad, pCM[i].pccmd, pCM[i].pcdescr);
		}
		return 0;
	}
	while (++argv, --argc > 0) {
		register int fFound;
		fFound = 0;
		iCmdLen = strlen(*argv);
		for (i = 0; i < pCS->icmds; ++i) {
			if (0 != strncmp(pCM[i].pccmd, *argv, iCmdLen))
				continue;
			fFound = 1;
			if (0 != (CMD_OFF & pCM[i].facts)) {
				printf("%s: is a disabled command\n", pCM[i].pccmd);
				continue;
			}
			if ((char *)0 != pCM[i].pcdescr) {
				if ((char *)0 != pCM[i].pcparams) {
					register int lCmd = strlen(pCM[i].pccmd) + 1 + strlen(pCM[i].pcparams);

					if (lCmd <= pCS->ipad) {
						printf("%s %s%*s ", pCM[i].pccmd, pCM[i].pcparams, pCS->ipad - lCmd, "");
					} else {
						printf("%s %s\n%*s", pCM[i].pccmd, pCM[i].pcparams, pCS->ipad+1, "");
					}
				} else {
					printf("%-*s ", pCS->ipad, pCM[i].pccmd);
				}
				puts(pCM[i].pcdescr);
				continue;
			}
			/* see if one does the same thing
			 */
			for (j = 0; j < pCS->icmds; ++j) {
				if (i == j || (char *)0 == pCM[j].pcdescr)
					continue;
				if (pCM[i].pfi == pCM[j].pfi && pCM[i].facts == pCM[j].facts)
					break;
			}
			if (j == pCS->icmds) {
				printf("%s: is a command\n", pCM[i].pccmd);
				continue;
			}
			printf("%s is like `%s' - %s\n", pCM[i].pccmd, pCM[j].pccmd, pCM[j].pcdescr);
		}
		if (fFound) {
			continue;
		}
%		fprintf(stderr, "%%s: %%s: unknown command\n", %b, *argv)%;
		iRet = 1;
	}
	return iRet;
}

/* plus the bonus commands routine at NO EXTRA COST (mostly)		(ksb)
 */
static int
cmd_commands(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	register CMD *pCM;
	register unsigned int i, j;

	pCM = pCS->pCMcmds;
	for (j = i = 0; i < pCS->icmds; ++i) {
		if (0 != ((CMD_HIDDEN|CMD_OFF) & pCM[i].facts))
			continue;
		if (0 != (j % 4)) {
			fputc(' ', stdout);
		}
		if (0 == (++j % 4)) {
			printf("%s\n", pCM[i].pccmd);
		} else {
			printf("%-19s", pCM[i].pccmd);
		}
	}
	if (0 != (j % 4)) {
		putc('\n', stdout);
	}
	return 0;
}
