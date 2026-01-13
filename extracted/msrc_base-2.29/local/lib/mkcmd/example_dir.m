# $Id: example_dir.m,v 8.5 1998/09/25 14:46:57 ksb Exp $
# Example usage for mkcmd dir interface -- ksb.
# Just define a trigger of (type "dir") to get an open "(DIR *)" on
# the %n name and the name on the %N keep (you must provide both names).
from "<stdio.h>"

require "std_help.m" "util_errno.m"

basename "fakels" ""

type "dir" 'd' {
	named "pDIR" "pcDir"
	cleanup "FakeLs();"
	help "scan dir as the target directory"
}

%hi
#if !defined(DIRENT)
#define DIRENT dirent
#endif
%%

%c
/* a really bare bones "proof of concept" ls to show we have the	(ksb)
 * correct directory open
 */
FakeLs()
{
	register struct DIRENT *pDP;

	if ((DIR *)0 == pDIR) {
		return;
	}
	printf("%s: %s\n", progname, pcDir);
	while ((void *)0 != (pDP = readdir(pDIR))) {
		printf(" %s\n", pDP->d_name);
	}
}
%%
