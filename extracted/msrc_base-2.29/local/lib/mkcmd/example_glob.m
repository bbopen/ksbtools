#!mkcmd
# $Id: example_glob.m,v 8.5 1997/10/14 01:16:17 ksb Beta $

require "util_glob.m" "glob_ieee.m" "std_help.m"

basename "glob" ""

every {
	named "Matches"
	param "glob"
	help "output matches for each expression"
}

%c
void
Matches(pcIt)
char *pcIt;
{
	register char **ppcFound;
	register int i;

	if ((char **)0 == (ppcFound = util_glob(pcIt))) {
		printf("%s: failed\n", pcIt);
		return;
	}
	printf("%s:\n", pcIt);
	for (i = 0; (char *)0 != ppcFound[i]; ++i) {
		printf(" %s\n", ppcFound[i]);
	}
	util_glob_free(ppcFound);
}
