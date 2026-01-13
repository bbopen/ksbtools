%c
/*
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
%%
require "std_help.m" "std_version.m"

from '<sys/types.h>'
from '<sys/param.h>'
from '<sys/time.h>'
from '<sys/socket.h>'
from '<sys/file.h>'
from '<sys/stat.h>'
from '<sys/resource.h>'
from '<sys/wait.h>'
from '<sys/ioctl.h>'
from '<fcntl.h>'
from '<netinet/in.h>'
from '<netdb.h>'
from '<stdio.h>'
from '<ctype.h>'
from '<errno.h>'
from '<signal.h>'
from '<pwd.h>'

from '"cons.h"'
from '"machine.h"'
from '"consent.h"'
from '"client.h"'
from '"group.h"'
from '"master.h"'
from '"access.h"'
from '"readcfg.h"'
from '"daemon.h"'

basename "conserver" ""

%h
extern char rcsid[];
extern struct sockaddr_in in_port;
extern char acMyHost[256];
#if defined(SERVICE)
extern char acService[];
#endif
%%

%i
char rcsid[] =
	"$Id: conserver.m,v 8.3 2000/05/09 00:42:32 ksb Exp $";

struct sockaddr_in in_port;
char acMyHost[256];		/* staff.cc.purdue.edu			*/
static char **argv_orig;	/* for restart code			*/

#if defined(SERVICE)
char acService[] = SERVICE;
#endif

#if HAVE_SELECT_H
#include <sys/select.h>
#endif
%%

%c
char acMyAddr[4];	/* "\200\76\7\1"			*/


/* convert the access word on the command line to a letter		(ksb)
 */
static char
a_func(pcArg)
char *pcArg;
{
	register char cRet;

	cRet = '\000' == *pcArg ? 'r' : *pcArg;
	if (isupper(cRet)) {
		cRet = tolower(cRet);
	}
	switch (cRet) {
	case 'r':
	case 'a':
	case 't':
		break;
	default:
		fprintf(stderr, "%s: unknown access type `%s\'\n", progname, pcArg);
		exit(3);
	}
	return cRet;
}

/* output more information than the std version stuff			(ksb)
 */
static void
ExtVersion()
{
	auto char acA1[16], acA2[16];

	printf("%s: default access type `%c\'\n", progname, chDefAcc);
	printf("%s: default escape sequence `%s%s\'\n", progname, FmtCtl(DEFATTN, acA1), FmtCtl(DEFESC, acA2));
	printf("%s: configuration in `%s\'\n", progname, pcConfig);
	printf("%s: limited to %d group%s with %d member%s\n", progname, MAXGRP, MAXGRP == 1 ? "" : "s", MAXMEMB, MAXMEMB == 1 ? "" : "s");
#if defined(SERVICE)
	{
		struct servent *pSE;
		if ((struct servent *)0 == (pSE = getservbyname(acService, "tcp"))) {
			fprintf(stderr, "%s: getservbyname: %s: %s\n", progname, acService, strerror(errno));
			return;
		}
		printf("%s: service name `%s\'", progname, pSE->s_name);
		if (0 != strcmp(pSE->s_name, acService)) {
			printf(" (which we call `%s\')", acService);
		}
		printf(" on port %d\n", ntohs((u_short)pSE->s_port));
	}
#else
#if defined(PORT)
	printf("%s: on port %d\n", progname, PORT);
#else
	printf("%s: no default service or port\n", progname);
#endif
#endif
#if DO_VIRTUAL
	printf("%s: virtual console support installed%s\n", progname,
#if HAVEPTYD
		", with ptyd"
#else
#if HAVE_PTSNAME
		", with ptsname"
#else
		""
#endif
#endif
	);
#endif
#if DO_ANNEX
	printf("%s: network console support installed\n", progname);
#endif
	BaudRates(stdout);
	fflush(stdout);
}
%%

init 3 "argv_orig = argv;"
init "setup();"

letter 'a' {
	named "chDefAcc"
	param "type"
	init "'r'"
	update "%n = a_func(%N);"
	help "set the default access type",
}

char* 'C' {
	named "pcConfig"
	init "CONFIG"
	param "config"
	help "specify an alternate configuration file"
}

boolean 'd' {
	named "fDaemon"
	help "become a daemon, output to /dev/null"
}
boolean 'n' {
	named "fAll"
	init "1"
	help "do not output summary stream to stdout"
}

toggle 's' {
	named "fSoftcar"
	help "set the soft carrier flag, ignore hardware DTR"
}
augment action 'V' {
	user "ExtVersion();"
}
boolean 'v' {
	named "fVerbose"
	help "be verbose on startup"
}

after { }
list {
	param "lines"
	help "limit the lines served to those listed"
}
exit { }

%c
/* find out where/who we are						(ksb)
 * parse optons
 * read in the config file, open the log file
 * spawn the kids to drive the console groups
 * become the master server
 * shutdown with grace
 * exit happy
 */

static int
setup()
{
	register int i, j;
	extern int optopt;
	auto struct hostent *hpMe;

	(void)setpwent();
#if USE_SETLINEBUF
	setlinebuf(stderr);
#endif
#if USE_SETVBUF
	setvbuf(stderr, NULL, _IOLBF, BUFSIZ);
#endif

	(void)time(& tServing);
	(void)gethostname(acMyHost, sizeof(acMyHost));
	if ((struct hostent *)0 == (hpMe = gethostbyname(acMyHost))) {
		fprintf(stderr, "%s: gethostbyname: %s: %s\n", progname, acMyHost, hstrerror(h_errno));
		exit(1);
	}
#if NEED_FDQN_HACK
	/* if the hostname gethostname() returns is not the full qual. dom.
	 * name of the host then we have to ask gethostbyname().  Under SunOS
	 * even gethostbyname() lies and sez the canonical name of the host
	 * is the name requested -- so we look it up again by address to
	 * force the name to be the real FDQN.  gregf@sequent.com noted some
	 * of this and I coded this (yucko) solution.		-- ksb
	 */
#if NEED_REV_FDQN
	{	register struct hostent *hpUnQual = hpMe;

		hpUnQual = hpMe;
		hpMe = gethostbyaddr(hpUnQual->h_addr, hpUnQual->h_length, hpUnQual->h_addrtype);
		if ((struct hostent *)0 == hpMe) {
			fprintf(stderr, "%s: %s: reverse lookup for FDQN hack returned nil\n", progname, hpUnQual->h_name);
			exit(1);
		}
	}
#endif
	(void)strcpy(& acMyHost[0], hpMe->h_name);
#endif

	if (sizeof(acMyAddr) != hpMe->h_length || AF_INET != hpMe->h_addrtype) {
		fprintf(stderr, "%s: wrong address size (%d != %d) or adress family (%d != %d)\n", progname, sizeof(acMyAddr), hpMe->h_length, AF_INET, hpMe->h_addrtype);
		exit(1);
	}
#if USE_STRINGS
	(void)bcopy(hpMe->h_addr, &acMyAddr[0], hpMe->h_length);
#else
	(void)memcpy(&acMyAddr[0], hpMe->h_addr, hpMe->h_length);
#endif

	/* if no one can use us we need to come up with a default
	 */
	if (0 == iAccess) {
		SetDefAccess(hpMe);
	}
}

FILE *fpConfig;

after_func()
{
	if (0 != geteuid()) {
		fprintf(stderr, "%s: must be the superuser\n", progname);
		exit(1);
	}

	/* read the config file
	 */
	if ((FILE *)0 == (fpConfig = fopen(pcConfig, "r"))) {
		fprintf(stderr, "%s: fopen: %s: %s\n", progname, pcConfig, strerror(errno));
		exit(1);
	}
	ReadCfg(pcConfig, fpConfig);

#if USE_FLOCK
	/* we lock the configuration file so that two identical
	 * conservers will not be running together  (but two with
	 * different configurations can run on the same host).
	 */
	if (-1 == flock(fileno(fpConfig), LOCK_NB|LOCK_EX)) {
		fprintf(stderr, "%s: %s is locked, won\'t run more than one conserver?\n", progname, pcConfig);
		exit(3);
	}
#endif


#if USE_SETLINEBUF
	setlinebuf(stdout);
#endif
#if USE_SETVBUF
	setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
#endif
}

/* serve only the listed lines, or all lines on this host		(ksb)
 * if none are provided
 */
static void
list_func(argc, argv)
int argc;
char **argv;
{
	register unsigned int k, i, j;
	register char **ppcVec;

	if (0 == argc) {
		return;
	}
	ppcVec = (char **)calloc(argc+1, sizeof(char *));
	if ((char **)0 == ppcVec) {
		fprintf(stderr, "%s: calloc: %s\n", progname, strerror(errno));
		exit(1);
	}
	for (k = 0; k < argc; ++k) {
		ppcVec[k] = argv[k];
	}
	for (i = 0; i < MAXGRP; ++i) {
		if (0 == aGroups[i].imembers) {
			continue;
		}
		/* remove all the console's that were not in ppcVec
		 */
		j = 0;
		while (j < aGroups[i].imembers) {
			for (k = 0; k < argc; ++k) {
				if (0 == strcmp(ppcVec[k], aGroups[i].pCElist[j].server))
					break;
			}
			/* found this one, keep it
			 */
			if (argc != k) {
				while (k < argc) {
					ppcVec[k] = ppcVec[k+1];
					++k;
				}
				--argc;
				++j;
				continue;
			}
			if (fVerbose) {
				printf("%s: group %d removed %s\n", progname, i, aGroups[i].pCElist[j].server);
			}
			if (-1 != aGroups[i].pCElist[j].fdtty) {
				(void)close(aGroups[i].pCElist[j].fdtty);
			}
			--aGroups[i].imembers;
			for (k = j; k < aGroups[i].imembers; ++k) {
				aGroups[i].pCElist[k] = aGroups[i].pCElist[k+1];
			}
		}
	}
	if (0 != argc) {
		fprintf(stderr, "%s: unknown console line%s:", progname, 1 == argc ? "" : "s");
		for (k = 0; k < argc; ++k) {
			fprintf(stderr, " %s", ppcVec[k]);
		}
		fprintf(stderr, ".\n");
		exit(1);
	}
}

/* stop putting kids back, and shoot them				(ksb)
 */
exit_func()
{
	register unsigned int i, j;
	auto REMOTE
		*pRCUniq;	/* list of uniq console servers		*/
	auto fd_set kmask;

	pRCUniq = FindUniq(pRCList);

	/* output internal details if verbose, like peer console servers
	 * and access types for hosts/nets
	 */
	if (fVerbose) {
		register REMOTE *pRC;

		for (i = 0; i < iAccess; ++i) {
			printf("%s: %s: access type '%c'\n", progname, pACList[i].pcwho, pACList[i].ctrust);
		}
		for (pRC = pRCUniq; (REMOTE *)0 != pRC; pRC = pRC->pRCuniq) {
			printf("%s: peer server on `%s'\n", progname, pRC->rhost);
		}
	}
	(void)fflush(stdout);
	(void)fflush(stderr);
	(void)endpwent();

	/* this is the mask of files we hold open across the daemonize call
	 */
	FD_ZERO(&kmask);
	FD_SET(fileno(fpConfig), &kmask);
	if (!fDaemon) {
		FD_SET(fileno(stdout), &kmask);
		FD_SET(fileno(stderr), &kmask);
	}

	Daemon(SIG_DFL, &kmask, fDaemon);

	/* spawn all the children, so fix kids has an initial pid
	 */
	for (i = 0; i < MAXGRP; ++i) {
		if (0 == aGroups[i].imembers) {
			continue;
		}

		Spawn(& aGroups[i], fileno(fpConfig));
		if (fVerbose) {
			printf("%s: group %d on port %d\n", progname, i, ntohs((u_short)aGroups[i].port));
		}
		for (j = 0; j < aGroups[i].imembers; ++j) {
			if (-1 != aGroups[i].pCElist[j].fdtty)
				(void)close(aGroups[i].pCElist[j].fdtty);
		}
	}

	/* block in Master until we get a quit or signal
	 */
	j = Master(pRCUniq);

	(void)signal(SIGCHLD, SIG_DFL);
	for (i = 0; i < MAXGRP; ++i) {
		if (0 == aGroups[i].imembers) {
			continue;
		}
		if (-1 == kill(aGroups[i].pid, SIGTERM)) {
			fprintf(stderr, "%s: kill: %s\n", progname, strerror(errno));
		}
	}

#if USE_FLOCK
	(void)flock(fileno(fpConfig), LOCK_UN);
#endif
	(void)fclose(fpConfig);

	/* The master process wants to restart, re-execve ourself	(ksb)
	 * If we were started though a symlink (as root) you have root
	 * already and don't need this code to break root (al la sendmail).
	 */
	if ('r' == j) {
		extern char **environ;
		fprintf(stderr, "%s: restarted as %s\n", progname, argv_orig[0]);
		execve(argv_orig[0], argv_orig, environ);
		fprintf(stderr, "%s: execve: %s: %s\n", progname, argv_orig[0], strerror(errno));
	}
	exit(0);
}
%%
