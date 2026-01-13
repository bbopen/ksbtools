#!mkcmd
# $Id: util_whittle.m,v 8.8 2003/03/14 22:06:35 ksb Exp $
#
# used in socket code to remove the extra junk on a FQDM,
# David Stevens at Purdue had the original idea about 1988
# in his version of rwhod. -- ksb
from '<sys/param.h>'

%hi
extern char *Whittle(/* char *, char * */);
%%
%c
/* remove from Host1 those domains common to both Host1 and Host2	(ksb)
 * we really should not write on Host2, but we do.
 */
char *
Whittle(pcHost1, pcHost2)
register char *pcHost1, *pcHost2;
{
	register char *p1, *p2, *po;
	auto char acDef2[MAXHOSTNAMELEN+1];

	if ((char *)0 == pcHost2) {
		if (-1 == gethostname(acDef2, sizeof(acDef2))) {
			return pcHost1;
		}
		pcHost2 = acDef2;
	}
	po = (char *)0;
	while ((char *)0 != (p1 = strrchr(pcHost1, '.')) && (char*)0 != (p2 = strrchr(pcHost2, '.')) && 0 == strcmp(p1, p2)) {
		*p1 = '\000';
		if ((char *)0 != po) {
			*po = '.';
		}
		*(po = p2) = '\000';
	}
	if ((char *)0 != po) {
		*po = '.';
	}
	return pcHost1;
}
%%
