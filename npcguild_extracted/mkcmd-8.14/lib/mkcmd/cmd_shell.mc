/* $Id: cmd_shell.mc,v 8.7 1997/10/27 11:55:22 ksb Exp $
 */

/* a subshell for the user						(ksb)
 */
static int
cmd_shell(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	register char *pcShell;
	register int iPid, i;
	extern char *getenv();

	if ((char *)0 == (pcShell = getenv("SHELL"))) {
		pcShell = "/bin/sh";
	}
	/* mess with PS1? */
	switch (iPid = fork()) {
	case -1:
%		fprintf(stderr, "%%s: fork: %%s\n", %b, %E)%;
		return 1;
	case 0:
		if (argc == 1) {
			execlp(pcShell, pcShell, "-i", (char *)0);
		} else {
			argv[0] = pcShell;
			execvp(pcShell, argv);
		}
%		fprintf(stderr, "%%s: execlp: %%s: %%s\n", %b, pcShell, %E)%;
		exit(1);
	default:
		break;
	}
	while (-1 != (i = wait((void *)0))) {
		if (iPid == i)
			break;
	}
	return 0;
}
