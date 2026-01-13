/* save a passwd entry (struct passwd) in dynamic space			(ksb)
 * $Id: util_savepwent.mc,v 8.12 1998/11/29 18:51:59 ksb Exp $
 */
struct passwd *
%%K<util_savepwent>1v(pPWFound)
struct passwd *pPWFound;
{
	register struct passwd *pPWMem;
	register int iMem;
	register void *pvMem;
	register char *pcStack;
	register int iFudge = sizeof(int) - 1;
	extern int errno;

	if ((struct passwd *)0 == pPWFound) {
		return (struct passwd *)0;
	}

	iMem = sizeof(struct passwd);
%@eval foreach "passwd_strings" "%K<HostType>1i/passwd_strings_/v"
%	if ((char *)0 != pPWFound->%a) %{
%		iMem += (strlen(pPWFound->%a)|iFudge)+1%;
	}
%@endfor

	/* ok we have his size, now build a copy we can keep
	 */
	if ((void *)0 == (pvMem = calloc(iMem, sizeof(char)))) {
%		fprintf(stderr, "%%s: calloc: %%d: %%s\n", %b, iMem, %E)%;
		return (struct passwd *)0;
	}
	pPWMem = (struct passwd *)pvMem;
	pcStack = (char *)(pPWMem+1);

%@eval foreach "passwd_box" "%K<HostType>1i/passwd_box_/v"
%	pPWMem->%a = pPWFound->%a%;
%@endfor
%@eval foreach "passwd_strings" "%K<HostType>1i/passwd_strings_/v"
%	if ((char *)0 != pPWFound->%a) %{
%		pPWMem->%a = strcpy(pcStack, pPWFound->%a)%;
		pcStack += (strlen(pcStack)|iFudge)+1;
	}
%@endfor
	return pPWMem;
}
