/* $Id: mode_out.mc,v 8.3 2004/12/15 22:20:18 ksb Exp $
 * require after "mode.m" to allow mode -> ls-style string conversion
 */

/* Convert an integer mode into a symbolic format			(ksb)
 * We trap nulls, so you don't _have_ to allocate space, but we
 * are not thread-safe or anything in that case.
 */
char *
%%K<mode_out>1v(pcLMode, pMC)
char *pcLMode;		/* alpha bits buffer (10 chars or more)		*/
u_MODE_CVT *pMC;	/* mode as converted from user			*/
{
	register int i, iBit;
	register char *pcRet;
	auto mode_t mMode, mOptional;
	static char acDef[12];

	if ((char *)0 == pcLMode) {
		pcLMode = acDef;
	}
	pcRet = pcLMode;
	mMode = (u_MODE_CVT *)0 == pMC ? 0 : pMC->iset;
	mOptional = (u_MODE_CVT *)0 == pMC ? 0 : pMC->ioptional;
	if ((u_MODE_CVT *)0 != pMC && '\000' != pMC->cfmt) {
		*pcLMode++ = pMC->cfmt;
	}
	/* if symbolic won't show it fall back to %04o/%o	(ksb)
	 *	- can't shot optional u+s,g+s,a+t,
	 *	- can't show optional +x under forced u+s,g+s,a+t
	 * we show node type in this case, strangely (d750/7000).
	 */
	if ((0 != (mOptional & 07000) ||
		(0 != (mOptional & 00100) && 0 != (mMode & 04000)) ||
		(0 != (mOptional & 00010) && 0 != (mMode & 02000)) ||
		(0 != (mOptional & 00001) && 0 != (mMode & 01000)))) {
		sprintf(pcLMode, "%04o/%o", mMode, mOptional);
		return pcRet;
	}
	(void)strcpy(pcLMode, u_mode_acOnes);
	for (i = 0, iBit = 0400; i < 9; ++i, iBit >>= 1) {
		if (0 == (mMode & iBit))
			pcLMode[i] = '-';
	}
	u_mode_ebit(pcLMode+2, S_ISUID & mMode, 's', 'x');
	u_mode_ebit(pcLMode+5, S_ISGID & mMode, 's', 'x');
	u_mode_ebit(pcLMode+8, S_ISVTX & mMode, 't', 'x');
	for (i = 0, iBit = 0400; i < 9; ++i, iBit >>= 1) {
		if (0 == (mOptional & iBit))
			continue;
		if ('-' == pcLMode[i])
			pcLMode[i] = '\?';
	}
	return pcRet;
}
