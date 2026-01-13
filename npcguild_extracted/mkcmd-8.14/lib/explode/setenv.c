/*@Version $Revision: 6.0 $@*/
/*@Explode@*/
/* $Id: setenv.c,v 6.0 2000/07/30 15:57:11 ksb Exp $
 */
#include <stdio.h>
#include "machine.h"

#define nil	((char *)0)
extern char *strcpy(), *strcat();

/*
 * set an environment variable						(ksb)
 * for unsetenv call with pchValue set to nil
 *
 * This routine modifies the global "environ" and will
 * build a new list if it must grow (it doesn't free the old one
 * since it may not have been malloc'd)  It will overwrite the
 * old value if the new one will fit, else is mallocs one large enough.
 *
 * returns nothing (an int)
 */
int
setenv(pchName, pchValue)
char *pchName, *pchValue;
{
	extern char **environ, *calloc(), *malloc();
	extern int strlen();
	register char **ppch, *pch;
	register int len, nv;

	nv = 0;
	len = strlen(pchName);
	for (ppch = environ; nil != (pch = *ppch); ++ppch) {
		if (0 == strncmp(pch, pchName, len) && '=' == pch[len]) {
			break;
		}
		++nv;
	}

	if (nil == pchValue) {
		if (nil == pch)	{
			return ;
		}
		while (nil != (ppch[0] = ppch[1])) {
			++ppch;
		}
		return ;
	}

	if (nil == pch) {
		++nv;
		ppch = (char **)calloc(nv+1, sizeof(char *));
		while (nv) {
			--nv;
			ppch[nv+1] = environ[nv];
		}
		environ = ppch;
	}

	len += strlen(pchValue) + 1;
	if (nil == pch || strlen(pch) < len)
		pch = malloc(len+1);
	strcpy(pch, pchName);
	strcat(pch, "=");
	strcat(pch, pchValue);
	*ppch = pch;
}
