/* $Id: enum.mc,v 8.7 1997/10/18 19:21:48 ksb Beta $
 */

/* find the enum in the list						(ksb)
 * else return the enum"_max" value
 */
unsigned
%%K<EnumCvt>1ev(pcIn, ppcList)
char *pcIn, **ppcList;
{
	register int i;

	if ((char *)0 == pcIn) {
		return 0;
	}
	if ('?' == pcIn[0] && '\000' == pcIn[1]) {
		register int iLen, iT;
		iLen = 7;
		for (i = 0; (char *)0 != ppcList[i]; ++i) {
			iT = strlen(ppcList[i]);
			if (iT > iLen)
				iLen = iT;
		}
		++iLen;
		iT = 0;
		for (i = 0; (char *)0 != ppcList[i]; ++i) {
			printf(" %*s", iLen, ppcList[i]);
			iT += iLen;
			if (iT+iLen > 79) {
				printf("\n");
				iT = 0;
			}
		}
		if (iT > 0 )
			printf("\n");
		return 0;
	}
	for (i = 0; (char *)0 != ppcList[i]; ++i) {
		if (0 == strcasecmp(pcIn, ppcList[i]))
			break;
	}
	return i;
}
