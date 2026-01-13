/* $Id: cmd_exec.mc,v 8.7 1997/10/27 11:55:22 ksb Exp $
 */

/* fork and exec the program given after the end of argv[0]		(ksb)
 * the full path to the program must be hidden after the command name
 * "name\0/the/file/path/to/name" in the command list
 */
static int
cmd_fs_exec(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	register char *pcPath;
	extern char **environ;
	register int iPid, iGot;
	auto unsigned int wBuf;

	pcPath = argv[0]+(strlen(argv[0])+1);
	if ('\000' == pcPath[0]) {
		pcPath = argv[0];
	}

	if (0 == (iPid = fork())) {
		execve(pcPath, argv, environ);
%		fprintf(stderr, "%%s: execve: %%s: %%s\n", %b, pcPath, %E)%;
		exit(4);
	}
	if (-1 == iPid) {
%		fprintf(stderr, "%%s: fork: %%s\n", %b, %E)%;
		return 1;
	}
	while (-1 != (iGot = wait((void *)&wBuf))) {
		if (iPid == iGot)
			break;
	}
	return (wBuf >> 8) & 0xff;
}
%%
