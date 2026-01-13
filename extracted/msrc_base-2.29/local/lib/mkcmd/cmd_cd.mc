/* $Id: cmd_cd.mc,v 8.7 1997/10/27 11:55:22 ksb Exp $
 */

/* change this process's current working directory			(ksb)
 * Features:
 *	default to $HOME
 *	do single parameter form
 *	do ~login and ~
 *	globs if util_glob.m does
 * do NOT do forms that require getpwd() or getwd() support from ksh:
 *	two parameter form
 *	cd -, or ~-
 */
static int
cmd_cd(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	extern char *getenv();
	register char *pcDest, *pcTo, **ppcDirs;
	auto char *pcCmdname;
	auto char acBuf[MAXPATHLEN+1];

	pcCmdname = argv[0];
	if (2 < argc) {
%		fprintf(stderr, "%%s: cd: usage [directory]\n", %b)%;
		return 1;
	}

	pcTo = argv[1];
	if ((char *)0 == pcTo || '~' == pcTo[0]) {
		pcTo = util_home(acBuf, (char *)0 == pcTo ? "" : pcTo+1);
		if ((char *)0 == pcTo) {
			if ('\000' == *acBuf) {
%				fprintf(stderr, "%%s: %%s: no $HOME available\n", %b, pcCmdname)%;
				return 1;
			}
%			fprintf(stderr, "%%s: %%s: %%s: no such login\n", %b, pcCmdname, acBuf)%;
			return 1;
		}
		pcDest = acBuf + strlen(acBuf);
	} else {
		pcDest = acBuf;
	}
	(void)strcpy(pcDest, pcTo);

	if ((char **)0 == (ppcDirs = util_glob(acBuf))) {
%		fprintf(stderr, "%%s: %%s: glob: out of memory\n", %b, pcCmdname)%;
		return 12;
	}
	if ((char **)-1 == ppcDirs) {
%		fprintf(stderr, "%%s: %%s: glob: %%s\n", %b, pcCmdname, %E)%;
		return errno;
	}
	if ((char *)0 == ppcDirs[0]) {
		pcDest = acBuf;
	} else if ((char *)0 != ppcDirs[1]) {
%		fprintf(stderr, "%%s: %%s: bad argument count\n", %b, pcCmdname)%;
		util_glob_free(ppcDirs);
		return 22;
	} else {
		pcDest = ppcDirs[0];
	}
	if (-1 == chdir(pcDest)) {
%		fprintf(stderr, "%%s: %%s: chdir: %%s: %%s\n", %b, pcCmdname, pcDest, %E)%;
		util_glob_free(ppcDirs);
		return 8;
	}
	util_glob_free(ppcDirs);
	return 0;
}
