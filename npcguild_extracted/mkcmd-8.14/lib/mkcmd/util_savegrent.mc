/* $Id: util_savegrent.mc,v 8.12 1998/09/20 18:50:26 ksb Exp $
 * save a group entry in malloc'd space					(ksb)
 */
struct group *
%%K<util_savegrent>1v(pGRFound)
struct group *pGRFound;
{
	register struct group *pGRMem;
	register int iMem;
	register void *pvMem;
	register char *pcStack;
	register int iFudge = sizeof(int) - 1;
	extern int errno;
%@if group_argvs[1]
	register int i;
	register char *pcScan;
%@endif
%@foreach "group_argvs"
%	auto int u_w_%a = 0;
%@endfor

	if ((struct group *)0 == pGRFound) {
		return (struct group *)0;
	}

	iMem = sizeof(struct group);
%@foreach "group_argvs"
%	for (i = 0%; (char *)0 != (pcScan = pGRFound->%a[i])%; ++i) %{
		iMem += (strlen(pcScan)|iFudge)+1;
	}
%	iMem += sizeof(char *) * (u_w_%a = i+1)%;
%@endfor
%@foreach "group_strings"
%	if ((char *)0 != pGRFound->%a) %{
%		iMem += (strlen(pGRFound->%a)|iFudge)+1%;
	}
%@endfor

	/* ok we have his size, now build a copy we can keep
	 */
	if ((void *)0 == (pvMem = calloc(iMem, sizeof(char)))) {
%		fprintf(stderr, "%%s: calloc: %%d: %%s\n", %b, iMem, %E)%;
		return (struct group *)0;
	}
	pGRMem = (struct group *)pvMem;
	pcStack = (char *)(pGRMem+1);

%@foreach "group_box"
%	pGRMem->%a = pGRFound->%a%;
%@endfor
%@foreach "group_strings"
%	if ((char *)0 != pGRFound->%a) %{
%		pGRMem->%a = strcpy(pcStack, pGRFound->%a)%;
		pcStack += (strlen(pcStack)|iFudge)+1;
	}
%@endfor
%@foreach "group_argvs"
%	pGRMem->%a = (char **)pcStack%;
%	pcStack = (char *)& pGRMem->%a[u_w_%a]%;
%	for (i = 0%; (char *)0 != (pcScan = pGRFound->%a[i])%; ++i) %{
%		pGRMem->%a[i] = strcpy(pcStack, pcScan)%;
		pcStack += (strlen(pcStack)|iFudge)+1;
	}
%	pGRMem->%a[i] = (char *)0%;
%@endfor
	return pGRMem;
}
