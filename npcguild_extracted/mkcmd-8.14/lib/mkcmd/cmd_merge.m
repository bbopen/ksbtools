# $Id: cmd_merge.m,v 8.8 1999/06/12 00:43:58 ksb Exp $
# After you include `cmd.h' you may include this tempate to provide	(ksb)
# a facility to merge new commands into an existing command set
#
from '<stdio.h>'
from '<ctype.h>'

%i
extern int cmd_merge();
%%

%c
/* the default qsort function for cmd entries				(ksb)
 */
static int
_cmd_cmp_alpha(pCMLeft, pCMRight)
CMD *pCMLeft, *pCMRight;
{
	if (!isalpha(pCMLeft->pccmd[0])) {
		if (isalpha(pCMRight->pccmd[0])) {
			return 1;
		}
	} else if (!isalpha(pCMRight->pccmd[0])) {
		return -1;
	}
	return strcmp(pCMLeft->pccmd, pCMRight->pccmd);
}

/* add the list of commands to the existing command set			(ksb)
 */
int
cmd_merge(pCS, pCMMerge, iCount, pfiSort)
CMDSET *pCS;
CMD *pCMMerge;
unsigned int iCount;
int (*pfiSort)();
{
	register CMD *pCMOut, *pCMIn;
	register int iMin;
	register unsigned int i, j, k;

	if ((int (*)())0 == pfiSort) {
		pfiSort = _cmd_cmp_alpha;
	}

	pCMIn = pCS->pCMcmds;
	for (i = 0; i < pCS->icmds; ) {
		for (j = 0; j < iCount; ++j) {
			if (0 == (*pfiSort)(pCMIn+i, pCMMerge+j))
				break;
		}
		if (j == iCount) {
			++i;
			continue;
		}
		pCMIn[i] = pCMMerge[j];
		if (j != --iCount) {
			pCMMerge[j] = pCMMerge[iCount];
		}
	}
	if (0 == iCount) {
		return 0;
	}
	k = iCount + pCS->icmds;
	pCMOut = (CMD *)calloc(k, sizeof(CMD));
	iMin = -1;
	for (i = 0; i < pCS->icmds; ++i) {
		/* XXX this doesn't work if all the commands are
		 * not unique to the Real iAmb given to cmd_init -- ksb
		 */
		if ((-1 == iMin || pCS->piambiguous[i] < iMin) && pCS->piambiguous[i] < strlen(pCMIn[i].pccmd))
			iMin = pCS->piambiguous[i];
		pCMOut[i] = pCMIn[i];
	}
	for (j = 0; j < iCount; ++j) {
		pCMOut[i++] = pCMMerge[j];
	}
	(void)qsort(pCMOut, k, sizeof(CMD), pfiSort);

	if ((int *)0 != pCS->piambiguous) {
		free(pCS->piambiguous);
	}
	cmd_init(pCS, pCMOut, k, pCS->pcprompt, iMin);
	return 0;
}
%%
