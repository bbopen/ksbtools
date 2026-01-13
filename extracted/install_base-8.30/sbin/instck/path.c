/*
 * $Id: path.c,v 8.3 1997/02/15 21:12:47 ksb Exp $
 * Copyright 1990 Purdue Research Foundation, West Lafayette, Indiana
 * 47907.  All rights reserved.
 *
 * Written by Kevin S Braunsdorf, ksb@cc.purdue.edu, purdue!ksb
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company or the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 *
 * 1. Neither the authors nor Purdue University are responsible for any
 *    consequences of the use of this software.
 *
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Credit to the authors and Purdue
 *    University must appear in documentation and sources.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 4. This notice may not be removed or altered.
 */

/*
 * routines to keep track of many files in a UNIX file system		(ksb)
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "machine.h"
#include "configure.h"
#include "install.h"
#include "main.h"
#include "special.h"
#include "instck.h"
#include "path.h"

#if STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

int chSep = '/';

static int (*pFunc)();
static char acBuffer[MAXPATHLEN];

static int
RecurPath(pCM, pch)
COMPONENT *pCM;
char *pch;
{
	register int i = 0;

	if (nilCM == pCM) {
		return 0;
	}
	for (*pch++ = chSep; (COMPONENT *)0 != pCM; pCM = pCM->pCMsibling) {
		(void)strcpy(pch, pCM->pccomp);
		if (pCM->fprint)
			(*pFunc)(acBuffer, & pCM->user_data);
		i = RecurPath(pCM->pCMchild, strchr(pch, '\000'));
		if (0 != i) {
			break;
		}
	}
	return i;
}

int
ApplyPath(pCM, l_func)
COMPONENT *pCM;
int (*l_func)();
{
	auto int (*temp)();
	auto int i = 0;

	temp = pFunc;
	if (nilCM != pCM) {
		pFunc = l_func;
		i = RecurPath(pCM, acBuffer);
	}
	pFunc = temp;
	return i;
}

/*
 * add a (unique) path element
 *	AddPath(& pCMPath, "/usr/local/bin/foo");
 *
 * does a lot of unneded work some times, but a good hack at 2:30 am
 */
PATH_DATA *
AddPath(ppCM, pch)
COMPONENT **ppCM;
char *pch;
{
	static char acEnd[] = "\000";
	register char *pchTemp;
	register COMPONENT *pCM = nilCM;

	while ('\000' != *pch) {
		while (chSep == *pch) {		/* /a//b		*/
			++pch;
		}

		if ((char *)0 == (pchTemp = strchr(pch, chSep))) {
			pchTemp = acEnd;
		} else {
			*pchTemp = '\000';
		}
		while (nilCM != (pCM = *ppCM)) {	/* level match	  */
			if (0 == strcmp(pCM->pccomp, pch)) {
				if ('\000' == pchTemp[1]) {
					/* break 2 ! */
					goto escape;
				}
				goto contin;
			}
			ppCM = & pCM->pCMsibling;
		}

		/* not at this level, add it at end
		 */
		pCM = newCM();
		pCM->pCMsibling = nilCM;
		pCM->pCMchild = nilCM;
		pCM->pccomp = malloc((strlen(pch)|7)+1);
		PUInit(& pCM->user_data);
		(void)strcpy(pCM->pccomp, pch);
		pCM->fprint = 0;
		*ppCM = pCM;

	contin:
		*pchTemp++ = chSep;
		pch = pchTemp;
		ppCM = & pCM->pCMchild;
	}
escape:
	if (nilCM != pCM)
		pCM->fprint = 1;
	return (nilCM != pCM) ? & pCM->user_data : (PATH_DATA *)0;
}
