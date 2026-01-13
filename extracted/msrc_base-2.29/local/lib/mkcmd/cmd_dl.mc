/* $Id: cmd_dl.mc,v 8.31 1999/09/25 20:09:15 ksb Exp $
 */

/* Add commands (in an object format) from a file			(ksb)
 */
static int
cmd_dl(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	register int i, iRet;
	register void *pDLCmds;
	auto CMD_DYNAMIC *pCDMerge;
	auto unsigned int uiMerge;

	iRet = 0;
	for (i = 1; i < argc; ++i) {
%		if ((void *)0 == (pDLCmds = dlopen(argv[i], %K<CDDynamic>2v))) %{
%			fprintf(stderr, "%%s: dlopen: %%s: %%s\n", %b, argv[i], dlerror())%;
			continue;
		}
%		pCDMerge = (CMD_DYNAMIC *)dlsym(pDLCmds, %K<CDDynamic>1qv)%;
		if ((CMD_DYNAMIC *)0 == pCDMerge) {
%			fprintf(stderr, "%%s: %%s: doens't have a \"%K<CDDynamic>1v\" symbol: %%s\n", %b, argv[i], dlerror())%;
			continue;
		}
		if ((int (*)())0 != pCDMerge->pfiinit && ! (pCDMerge->pfiinit)(pDLCmds)) {
			/* the init function should warn stderr -- ksb */
			iRet = 1;
			continue;
		}
		cmd_merge(pCS, pCDMerge->pCMlocal, pCDMerge->icmds, pCDMerge->pfimerge);
		/* do not close the library, we are not (ever) done with it
		 */
	}
	return iRet;
}
