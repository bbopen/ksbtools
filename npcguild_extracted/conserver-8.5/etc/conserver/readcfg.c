/*
 * $Id: readcfg.c,v 8.1 1998/06/07 16:20:46 ksb Exp $
 *
 * Copyright (c) 1990 The Ohio State University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by The Ohio State University and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <pwd.h>

#include "cons.h"
#include "machine.h"
#include "consent.h"
#include "client.h"
#include "group.h"
#include "access.h"
#include "readcfg.h"
#include "master.h"
#include "main.h"

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

GRPENT
	aGroups[MAXGRP];		/* even spread is OK		*/
CONSENT
	aConsoles[MAXGRP*MAXMEMB];	/* gross over allocation        */
REMOTE
	*pRCList;		/* list of remote consoles we know about */
int
	iLocal;			/* number of local consoles		*/

ACCESS
	*pACList;		/* `who do you love' (or trust)		*/
int
	iAccess;		/* how many access restrictions we have	*/


/* read in the configuration file, fill in all the structs we use	(ksb)
 * to manage the consoles
 */
void
ReadCfg(pcFile, fp)
char *pcFile;
register FILE *fp;
{
	register GRPENT *pGE;
	register int iG, minG;
	auto int iLine;
	auto char acIn[BUFSIZ];
	register GRPENT *pGEAll;
	register CONSENT *pCE;
	register REMOTE **ppRC;

	pGEAll = aGroups;		/* fill in these structs	*/
	pCE = aConsoles;
	ppRC = & pRCList;
	iLocal = 0;

	iG = minG = 0;
	iLine = 0;
	while (fgets(acIn, sizeof(acIn)-1, fp) != NULL) {
		register char *pcLine, *pcSpeed, *pcLog, *pcGroup, *pcRem, *pcPass;
#if DO_POWER
		register char *pcPower;
#endif

		++iLine;
		for (pcRem = acIn+strlen(acIn)-1; pcRem >= acIn; --pcRem) {
			if (!isspace(*pcRem))
				break;
			*pcRem = '\000';
			if (pcRem == acIn)
				break;
		}
		if ('#' == acIn[0] || '\000' == acIn[0]) {
			continue;
		}
		if ('%' == acIn[0] && '%' == acIn[1] && '\000' == acIn[2]) {
			break;
		}
		/* name:[passwd]:line[@host]:speed:log:[group][:power]\n
		 */
		if ( (char *)0 == (pcPass = strchr(acIn, ':')) ||
		     (char *)0 == (pcLine = strchr(pcPass+1, ':')) ||
		     (char *)0 == (pcSpeed = strchr(pcLine+1, ':')) ||
		     (char *)0 == (pcLog  = strchr(pcSpeed+1, ':')) ||
		     (char *)0 == (pcGroup = strchr(pcLog+1, ':'))) {
			fprintf(stderr, "%s: %s(%d) bad config line `%s'\n", progname, pcFile, iLine, acIn);
			continue;
		}
		*pcPass++ = '\000';
		*pcLine++ = '\000';
		*pcSpeed++ = '\000';
		*pcLog++ = '\000';
		*pcGroup++ = '\000';
#if DO_POWER
		if ((char *)0 != (pcPower = strchr(pcGroup, '\n'))) {
			*pcPower = '\000';
		}
		if ((char *)0 != (pcPower = strchr(pcGroup, ':'))) {
			*pcPower++ = '\000';
		}
#endif

		/* any member of a group can set a `group password' for
		 * all group members (since we can slide with `^Ec;')
		 */
		if ('\000' == *pcPass) {
			pcPass = (char *)0;
		}

		/* if this server remote?
		 * (contains an '@host' where host is not us)
		 * if so just add it to a linked list of remote hosts
		 * I'm sure most sites will never use this code (ksb)
		 */
		if ((char *)0 != (pcRem = strchr(pcLine, '@')) &&
		   ((*pcRem++ = '\000'), 0 != strcmp(acMyHost, pcRem))) {
			register REMOTE *pRCTemp;
			pRCTemp = (REMOTE *)calloc(1, sizeof(REMOTE));
			if ((REMOTE *)0 == pRCTemp) {
				CSTROUT(2, "out of memory!\n");
				exit(32);
			}
			(void)strcpy(pRCTemp->rhost, pcRem);
			(void)strcpy(pRCTemp->rserver, acIn);
			*ppRC = pRCTemp;
			ppRC = & pRCTemp->pRCnext;
			if (fVerbose) {
				printf("%s: %s remote on %s\n", progname, acIn, pcRem);
			}
			continue;
		}

		/* take the same group as the last line, by default
		 */
		if ('\000' == pcGroup[0]) {
			if (MAXMEMB == pGEAll[iG].imembers) {
				++iG;
			}
		} else if ('+' == pcGroup[0] && ('\n' == pcGroup[1] || '\000' == pcGroup[1])) {
			++iG;
		} else {
			iG = atoi(pcGroup);
		}
		if (iG < minG || iG >= MAXGRP) {
			fprintf(stderr, "%s: %s(%d) group number out of bounds %d <= %d < %d\n", progname, pcFile, iLine, minG, iG, MAXGRP);
			exit(1);
		}
		minG = iG;
		pGE = pGEAll+iG;
		if (0 == pGE->imembers++) {
			pGE->pCElist = pCE;
		}
		if (pGE->imembers > MAXMEMB) {
			fprintf(stderr, "%s: %s(%d) group %d has more than %d members -- but we'll give it a spin\n", progname, pcFile, iLine, iG, MAXMEMB);
		}
		if ((char *)0 != pcPass) {
			if ('\000' != pGE->passwd[0] && 0 != strcmp(pcPass, pGE->passwd)) {
				fprintf(stderr, "%s: %s(%d) group %d has more than one password, first taken\n", progname, pcFile, iLine, iG);
			} else if ((int)strlen(pcPass) > MAXPSWDLEN) {
				fprintf(stderr, "%s: %s(%d) password too long (%d is the limit)\n", progname, pcFile, iLine, iG, MAXPSWDLEN);
			} else {
				(void)strcpy(pGE->passwd, pcPass);
			}
		}

		/* fill in the console entry
		 */
		if (sizeof(aConsoles)/sizeof(CONSENT) == iLocal) {
			fprintf(stderr, "%s: %s(%d) %d is too many consoles for hard coded tables, adjust MAXGRP or MAXMEMB\n", progname, pcFile, iLine, iLocal);
			exit(1);
		}
		(void)strcpy(pCE->server, acIn);
		(void)strcpy(pCE->lfile, pcLog);
#if DO_ANNEX
		pCE->fannexed = 0;
#endif
#if DO_VIRTUAL
		pCE->fvirtual = 0;
		pCE->ipid = -1;
#endif
#if DO_POWER
		/* get the power controller code for this line
		 * e.g. "B1:power@bipolar.com"	-- ksb
		 * the default key should be the console name, in group.c
		 * the default controller is none
		 */
		if ((char *)0 != pcPower && '\000' != *pcPower) {
			register char *pcStone;
			if ((char *)0 != (pcStone = strchr(pcPower, ':'))) {
				*pcStone++ = '\000';
			}
			(void)strncpy(pCE->acpower, pcPower, sizeof(pCE->acpower));
			pCE->acpower[sizeof(pCE->acpower)-1] = '\000';
			pcPower = pcStone;
		} else {
			pCE->acpower[0] = '\000';
		}
		if ((char *)0 != pcPower && '\000' != *pcPower) {
			(void)strncpy(pCE->acctlr, pcPower, sizeof(pCE->acctlr));
			pCE->acctlr[sizeof(pCE->acctlr)-1] = '\000';
		} else {
			pCE->acctlr[0] = '\000';
		}
#endif
		pCE->fuseflow = 1;
		if ('!' == pcLine[0]) {
#if DO_ANNEX
			pCE->fannexed = 1;
			if ((char *)0 == (pCE->pcannex = malloc((strlen(pcLine)|7)+1))) {
				OutOfMem();
			}
			(void)strcpy(pCE->pcannex, pcLine+1);
			pCE->haxport = atoi(pcSpeed);
#else
			fprintf(stderr, "%s: %s(%d) this server doesn't provide any annex console support\n", progname, pcFile, iLine);
			exit(9);
#endif
		} else if ('|' == pcLine[0]) {
#if DO_VIRTUAL
			pCE->fvirtual = 1;
			if ((char *)0 == (pCE->pccmd = malloc((strlen(pcLine)|7)+1))) {
				OutOfMem();
			}
			(void)strcpy(pCE->pccmd, pcLine+1);
			(void)strcpy(pCE->dfile, "/dev/null");
			XlateSpeed(pcSpeed, & pCE->pbaud, & pCE->pparity);
#else
			fprintf(stderr, "%s: %s(%d) this server doesn't provide any virtual console support\n", progname, pcFile, iLine);
			exit(9);
#endif
		} else {
			(void)strcpy(pCE->dfile, pcLine);
			pCE->fuseflow = XlateSpeed(pcSpeed, & pCE->pbaud, & pCE->pparity);
		}

		if (fVerbose) {
#if DO_ANNEX
			if (pCE->fannexed)
				printf("%s: %d: %s with annex %s:%d logged to %s\n", progname, iG, acIn, pCE->pcannex, pCE->haxport, pcLog);
			else
#endif
#if DO_VIRTUAL
			if (pCE->fvirtual)
				printf("%s: %d: %s with command `%s' logged to %s\n", progname, iG, acIn, pCE->pccmd, pcLog);
			else
#endif
				printf("%s: %d: %s is on %s (%s%c) logged to %s\n", progname, iG, acIn, pCE->dfile, pCE->pbaud->acrate, pCE->pparity->ckey, pcLog);
		}
		++pCE, ++iLocal;
	}
	*ppRC = (REMOTE *)0;

	/* make a vector of access restructions
	 */
	iG = iAccess = 0;
	pACList = (ACCESS *)0;
	while (fgets(acIn, sizeof(acIn)-1, fp) != NULL) {
		register char *pcRem, *pcMach, *pcNext, *pcMem;
		auto char cType;
		auto int iLen;

		++iLine;
		for (pcRem = acIn+strlen(acIn); pcRem >= acIn; --pcRem) {
			if (!isspace(*pcRem))
				break;
			*pcRem = '\000';
			if (pcRem == acIn)
				break;
		}
		if ('#' == acIn[0] || '\000' == acIn[0]) {
			continue;
		}
		if ('%' == acIn[0] && '%' == acIn[1] && '\000' == acIn[2]) {
			break;
		}
		if ((char *)0 == (pcNext = strchr(acIn, ':'))) {
			fprintf(stderr, "%s: %s(%d) missing colon?\n", progname, pcFile, iLine);
			exit(3);
		}
		do {
			*pcNext++ = '\000';
		} while (isspace(*pcNext));
		switch (acIn[0]) {
		case 'a':		/* allowed, allow, allows	*/
		case 'A':
			cType = 'a';
			break;
		case 'r':		/* rejected, refused, refuse	*/
		case 'R':
			cType = 'r';
			break;
		case 't':		/* trust, trusted, trusts	*/
		case 'T':
			cType = 't';
			break;
		default:
			fprintf(stderr, "%s: %s(%d) unknown access key `%s\'\n", progname, pcFile, iLine, acIn);
			exit(3);
		}
		while ('\000' != *(pcMach = pcNext)) {
			while (!isspace(*pcNext)) {
				++pcNext;
			}
			while ('\000' != *pcNext && isspace(*pcNext)) {
				*pcNext++ = '\000';
			}
			if (iAccess < iG) {
				/* still have room */;
			} else if (0 != iG) {
				iG += 8;
				pACList = (ACCESS *)realloc((char *)pACList, iG * sizeof(ACCESS));
			} else {
				iG = MAXGRP;
				pACList = (ACCESS *)malloc(iG * sizeof(ACCESS));
			}
			if ((ACCESS *)0 == pACList) {
				OutOfMem();
			}
			/* use loopback interface for local connections
			 */
			if (0 == strcmp(pcMach, acMyHost)) {
				pcMach = "127.0.0.1";
			}
			iLen = strlen(pcMach);
			if ((char *)0 == (pcMem = malloc(iLen+1))) {
				OutOfMem();
			}
			pACList[iAccess].ctrust = cType;
			pACList[iAccess].ilen = iLen;
			pACList[iAccess].pcwho = strcpy(pcMem, pcMach);
			++iAccess;
		}
	}
}
