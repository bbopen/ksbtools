/* $Id: setenv.c,v 2.1 1997/03/11 21:41:16 ksb Exp $
 */
#include <stdio.h>
#include "setenv.h"

/*
 * set an environment variable						(ksb)
 * for unsetenv call with pchValue set to (char *)0
 *
 * This routine modifies the global "environ" and will build a new list
 * if it must grow (it doesn't free the old one since it may not have
 * been malloc'd)  It will overwrite the old value if the new one will
 * fit, else is mallocs one large enough.
 *
 * returns nothing (an int for comaptibility)
 */
int
setenv(pchName, pchValue, fBogus)
char *pchName, *pchValue;
int fBogus;
{
	extern char **environ, *calloc(), *malloc();
	extern int strlen();
	register char **ppch, *pch;
	register int len, nv;

	nv = 0;
	len = strlen(pchName);
	for (ppch = environ; (char *)0 != (pch = *ppch); ++ppch) {
		if (0 == strncmp(pch, pchName, len) && '=' == pch[len]) {
			break;
		}
		++nv;
	}

	if ((char *)0 != pch && !fBogus) {
		return ;
	}
	if ((char *)0 == pchValue) {
		if ((char *)0 == pch)	{
			return ;
		}
		while ((char *)0 != (ppch[0] = ppch[1])) {
			++ppch;
		}
		return ;
	}

	if ((char *)0 == pch) {
		++nv;
		ppch = (char **)calloc(nv+1, sizeof(char *));
		while (nv) {
			--nv;
			ppch[nv+1] = environ[nv];
		}
		environ = ppch;
	}

	len += strlen(pchValue) + 1;
	if ((char *)0 == pch || strlen(pch) < len) {
		pch = malloc(len+1);
	}
	strcpy(pch, pchName);
	strcat(pch, "=");
	strcat(pch, pchValue);
	*ppch = pch;
}
