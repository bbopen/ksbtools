/* $Id: util_getlogingr.mc,v 1.3 2000/07/29 19:06:21 ksb Exp $
 * Find out what groups a login belongs to, no limits			(ksb)
 * (you can qsort them yourself).
 * The returned vector is suitable for free'ing. -- ksb
 */
extern struct group *getgrent();

/* getlogingr - get group information for the given login id		(ksb)
 */
int
%%K<util_getlogingr>1v(pcLogin, ppGRFound)
char *pcLogin;
gid_t **ppGRFound;
{
	register struct group *grp;
	register int i, ngroups;
	register gid_t *pGRList;
	register int iMax;

%	iMax = %K<getlogingr>1v%;
	ngroups = 0;
	pGRList = (gid_t *)calloc(iMax, sizeof(gid_t));
	setgrent();
	while (grp = getgrent()) {
		for (i = 0; grp->gr_mem[i]; ++i) {
			register char *pcFast;
			if (pcLogin[0] != (pcFast = grp->gr_mem[i])[0]) {
				continue;
			}
			if (0 != strcmp(pcFast, pcLogin)) {
				continue;
			}
			while (iMax <= ngroups) {
%				iMax += %K<getlogingr>2v;
			}
			pGRList = (gid_t *)realloc((void *)pGRList, iMax * sizeof(gid_t));
			if ((gid_t *)0 == pGRList) {
				return -1;
			}
			pGRList[ngroups++] = grp->gr_gid;
			break;
		}
	}
	endgrent();

	*ppGRFound = pGRList;
	return ngroups;
}
