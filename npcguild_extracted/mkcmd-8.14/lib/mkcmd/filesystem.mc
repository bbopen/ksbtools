/* $Id: filesystem.mc,v 1.1 1999/04/21 17:22:11 ksb Exp $
 * a primarive user interface to statfs, -- ksb
 */

/* It looks about like what I though					(ksb)
 * allocate a (struct statfs) and fill it in per the param,
 * we try to guess if you give us "-" as "not the tty one"
 */
struct statfs *
%%K<fs_convert>1ev(pcGiven)
char *pcGiven;
{
	register struct statfs *pSFRet;
	register int iRet;

	pSFRet = (struct statfs *)calloc(1, sizeof(struct statfs));
	if ((struct statfs *)0 == pSFRet) {
		return (struct statfs *)0;
	}
	if ('-' == pcGiven[0] && '\000' == pcGiven[1]) {
		iRet = fstatfs(isatty(0), pSFRet);
		if (-1 == iRet && EBADF == errno) {
			iRet = fstatfs(! isatty(0), pSFRet);
		}
	} else {
		iRet = statfs(pcGiven, pSFRet);
	}
	if (-1 != iRet) {
		return pSFRet;
	}

	iRet = errno;
	free((void *)pSFRet);
	errno = iRet;
	return (struct statfs *)0;
}
