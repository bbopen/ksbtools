/* $Id: cmd_add.mc,v 8.5 1997/10/10 00:57:37 ksb Beta $
 */

/* Add commands (in a text format) from a file				(ksb)
 * see cmd_parse.m for format details.
 */
static int
cmd_add(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	register int i;
	register FILE *fpCmds;
	auto CMD *pCMMerge;
	auto unsigned int uiMerge;

	for (i = 1; i < argc; ++i) {
		if ((FILE *)0 == (fpCmds = fopen(argv[i], "r"))) {
%			fprintf(stderr, "%%s: fopen: %%s: %%s\n", %b, argv[i], strerror(errno))%;
			continue;
		}
		if (0 == cmd_parse(fpCmds, & pCMMerge, & uiMerge) && 0 != uiMerge) {
			cmd_merge(pCS, pCMMerge, uiMerge, (int (*)())0);
		}
		(void)fclose(fpCmds);
	}
	return 0;
}
