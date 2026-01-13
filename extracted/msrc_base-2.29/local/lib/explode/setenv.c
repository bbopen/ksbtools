/*@Version $Revision: 6.6 $@*/
/*@Explode@*/
/* $Id: setenv.c,v 6.6 2008/09/06 20:03:47 ksb Exp $
 */
#include <stdio.h>
/*@Shell %echo ${MACHINE_H+"$MACHINE_H"}%@*/

#if HAVE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

/* When stdlib has a prototype for setenv it must match ours, so
 * don't use the normal USE_STDLIB, because if it had a working
 * setenv(3) we wouldn't be using this emulation of it. -- ksb
 */
#if USE_MALLOC_H
#include <malloc.h>
#else
#if REALLY_USE_STDLIB
#include <stdlib.h>
#else

/* We need to extern malloc and calloc w/o "size_t_ in-scope, maybe.
 * So guess based on the fact than an (unsigned int) must be able to
 * hold the difference between any two pointers (from the same vector).
 */
#if !defined(EMU_UINT32)
#define EMU_UINT32 unsigned int
#endif
extern void *malloc(EMU_UINT32);
extern void *calloc(EMU_UINT32, EMU_UINT32);
#endif
#endif



/* Set an environment variable						(ksb)
 * for unsetenv call with pcValue as (char *)0
 *
 * This routine modifies the global "environ" and will
 * build a new list if it must grow (it doesn't free the old one
 * since it may not have been malloc'd)  It will overwrite the
 * old value if the new one will fit, else is malloc's one large enough.
 *
 * Returns nothing 0, -1 like a system call would.
 */
int
setenv(pcName, pcValue, fOver)
const char *pcName, *pcValue;
int fOver;
{
	extern char **environ;
	register char **ppc, *pc;
	register int iLen, nv;

	nv = 0;
	iLen = strlen(pcName);
	for (ppc = environ; (char *)0 != (pc = *ppc); ++ppc) {
		if (0 == strncmp(pc, pcName, iLen) && '=' == pc[iLen]) {
			break;
		}
		++nv;
	}

	if ((char *)0 == pcValue) {
		if ((char *)0 == pc)	{
			return 0;
		}
		while ((char *)0 != (ppc[0] = ppc[1])) {
			++ppc;
		}
		return 0;
	}

	if ((char *)0 == pc) {
		++nv;
		ppc = (char **)calloc(nv+1, sizeof(char *));
		if ((char **)0 == ppc) {
			return -1;
		}
		while (nv) {
			--nv;
			ppc[nv+1] = environ[nv];
		}
		environ = ppc;
	} else if (0 == fOver) {
		return 0;
	}

	iLen += strlen(pcValue) + 1;
	if ((char *)0 == pc || strlen(pc) < iLen) {
		pc = (char *)malloc(iLen+1);
	}
	(void)strcpy(pc, pcName);
	(void)strcat(pc, "=");
	(void)strcat(pc, pcValue);
	*ppc = pc;
	return 0;
}
