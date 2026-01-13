/* $Id: cmd_source.mc,v 8.7 1997/10/27 11:55:22 ksb Exp $
 */

/* should we change the prompt?  If one sources /dev/tty they get	(ksb)
 * a subshell -- kinda.
 */
static int
cmd_source(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	register FILE *fpCmds;
	register int iRet;
	extern int errno;

	iRet = 0;
	while (0 != --argc) {
		if ((FILE *)0 == (fpCmds = fopen(*++argv, "r"))) {
%			fprintf(stderr, "%%s: fopen: %%s: %%s\n", %b, *argv, %E)%;
			++iRet;
			continue;
		}
		iRet += cmd_from_file(pCS, fpCmds);
		(void)fclose(fpCmds);
	}
	return iRet;
}
