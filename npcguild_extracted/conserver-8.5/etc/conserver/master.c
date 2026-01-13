/*
 * $Id: master.c,v 8.3 2000/05/09 00:42:32 ksb Exp $
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
#include "master.h"
#include "readcfg.h"
#include "main.h"

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#if USE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <sys/resource.h>

#ifdef POSIX
#include <limits.h>
#include <unistd.h>
#endif

#if NEED_GNU_TYPES
#include <gnu/types.h>
#endif

#if HAVE_SELECT_H
#include <sys/select.h>
#endif

extern char *crypt();
extern time_t time();


/* check all the kids and respawn as needed.				(fine)
 * Called when master process receives SIGCHLD
 */
static SIGRET_T
FixKids(arg)
int arg;
{
	register int i, pid;
	auto long tyme;
	auto WAIT_T UWbuf;

#if HAVE_WAIT3
	while (-1 != (pid = wait3(& UWbuf, WNOHANG, (struct rusage *)0)))
#else
	while (-1 != (pid = wait(& UWbuf)))
#endif
	{
		if (0 == pid) {
			break;
		}
		/* stopped child is just continued
		 */
		if (WIFSTOPPED(UWbuf) && 0 == kill(pid, SIGCONT)) {
			continue;
		}

		for (i = 0; i < MAXGRP; ++i) {
			if (0 == aGroups[i].imembers)
				continue;
			if (pid != aGroups[i].pid)
				continue;

			/* this kid kid is dead, start another
			 */
			Spawn(& aGroups[i]);
			tyme = time((long *)0);
			printf("%s: %s: exit(%d), restarted %s", progname, aGroups[i].pCElist[0].server, WEXITSTATUS(UWbuf), ctime(&tyme));
		}
	}
#if !HAVE_STICKY_SIGNALS
	(void)signal(SIGCHLD, FixKids);
#endif
}

static int fSawQuit = '\000';

/* kill all the kids and exit.						(ksb)
 * Called when master process receives SIGTERM
 */
static SIGRET_T
QuitIt(arg)
int arg;
{
#if !HAVE_STICKY_SIGNALS
	(void)signal(SIGTERM, QuitIt);
#endif
	fSawQuit = 'q';
}

/* kill all the kids and restart.					(ksb)
 * Called when master process receives SIGUSR2
 */
static SIGRET_T
Restart(arg)
int arg;
{
#if !HAVE_STICKY_SIGNALS
	(void)signal(SIGTERM, Restart);
#endif
	fSawQuit = 'r';
}


/* this routine is used by the master console server process		(ksb)
 */
int
Master(pRCUniq)
REMOTE
	*pRCUniq;		/* list of uniq console servers		*/
{
	register char *pcArgs;
	register int i, j, cfd;
	register REMOTE *pRC, *pRCFound;
	register int nr, prnum, found, msfd;
	register struct hostent *hpPeer;
	auto char cType;
	auto int so;
	auto fd_set rmask, rmaster;
	auto char acIn[1024], acOut[BUFSIZ];
	auto struct sockaddr_in master_port, response_port;
	auto int iTrue = 1;

	/* set up signal handler */
	(void)signal(SIGCHLD, FixKids);
	(void)signal(SIGTERM, QuitIt);
	(void)signal(SIGHUP, SIG_IGN);
	(void)signal(SIGUSR2, Restart);

	/* set up port for master to listen on
	 */
#if USE_STRINGS
	(void)bzero((char *)&master_port, sizeof(master_port));
#else
        (void)memset((void *)&master_port, 0, sizeof(master_port));
#endif
	master_port.sin_family = AF_INET;
	*(u_long *)&master_port.sin_addr = INADDR_ANY;
#if defined(SERVICE)
	{
		struct servent *pSE;
		if ((struct servent *)0 == (pSE = getservbyname(acService, "tcp"))) {
			fprintf(stderr, "%s: getservbyname: %s: %s\n", progname, acService, strerror(errno));
			return;
		}
		master_port.sin_port = pSE->s_port;
	}
#else
#if defined(PORT)
	master_port.sin_port = htons((u_short)PORT);
#else
	fprintf(stderr, "%s: no port or service compiled in?\n", progname);
#endif
#endif

	if ((msfd=socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "%s: socket: %s\n", progname, strerror(errno));
		return;
	}
#if defined(SO_REUSEADDR) && defined(SOL_SOCKET)
	if (setsockopt(msfd, SOL_SOCKET, SO_REUSEADDR, (char *)&iTrue, sizeof(iTrue))<0) {
		fprintf(stderr, "%s: setsockopt: %s\n", progname, strerror(errno));
		return;
	}
#endif
	if (bind(msfd, (struct sockaddr *)&master_port, sizeof(master_port))<0) {
		fprintf(stderr, "%s: bind: %s\n", progname, strerror(errno));
		return;
	}
	if (listen(msfd, SOMAXCONN) < 0) {
		fprintf(stderr, "%s: listen: %s\n", progname, strerror(errno));
	}

	FD_ZERO(&rmaster);
	FD_SET(msfd, &rmaster);
	for (fSawQuit = '\000'; '\000' == fSawQuit; /* can't close here */) {
		rmask = rmaster;

		if (-1 == select(msfd+1, &rmask, (fd_set *)0, (fd_set *)0, (struct timeval *)0)) {
			fprintf(stderr, "%s: select: %s\n", progname, strerror(errno));
			continue;
		}
		if (!FD_ISSET(msfd, &rmask)) {
			continue;
		}
		so = sizeof(response_port);
		cfd = accept(msfd, (struct sockaddr *)&response_port, &so);
		if (cfd < 0) {
			fprintf(stderr, "%s: accept: %s\n", progname, strerror(errno));
			continue;
		}


		so = sizeof(in_port);
		if (-1 == getpeername(cfd, (struct sockaddr *)&in_port, &so)) {
			CSTROUT(cfd, "getpeername failed\r\n");
			(void)close(cfd);
			continue;
		}
		so = sizeof(in_port.sin_addr);
		if ((struct hostent *)0 == (hpPeer = gethostbyaddr((char *)&in_port.sin_addr, so, AF_INET))) {
			CSTROUT(cfd, "unknown peer name\r\n");
			(void)close(cfd);
			continue;
		}
		if ('r' == (cType = AccType(hpPeer))) {
			CSTROUT(cfd, "access from your host refused\r\n");
			(void)close(cfd);
			continue;
		}

#if TEST_FORK
		/* we should fork here, or timeout
		 */
		switch(fork()) {
		default:
			(void)close(cfd);
			continue;
		case -1:
			CSTROUT(cfd, "fork failed, try again\r\n");
			(void)close(cfd);
			continue;
		case 0:
			break;
		}
#endif
		/* handle the connection
		 * (port lookup, who, users, or quit)
		 */
		CSTROUT(cfd, "ok\r\n");
		for (i = 0; i < sizeof(acIn); /* i+=nr */) {
			if (0 >= (nr = read(cfd, &acIn[i], sizeof(acIn)-1-i))) {
				i = 0;
				break;
			}
			i += nr;
			if ('\n' == acIn[i-1]) {
				acIn[i] = '\000';
				--i;
				break;
			}
		}
		if (i > 0 && '\n' == acIn[i-1]) {
			acIn[--i] = '\000';
		}
		if (i > 0 && '\r' == acIn[i-1]) {
			acIn[--i] = '\000';
		}
		if (0 == i) {
			fprintf(stderr, "%s: lost connection\n", progname);
#if TEST_FORK
			exit(1);
#else
			(void)close(cfd);
			continue;
#endif
		}
		if ((char *)0 != (pcArgs = strchr(acIn, ':'))) {
			*pcArgs++ = '\000';
		} else if ((char *)0 != (pcArgs = strchr(acIn, ' '))) {
			*pcArgs++ = '\000';
		}
		/* If we added a command "add:passwd\nentry"
		 * then we could for a group to run the requested console.
		 * Of course we'd reject requests if aGroups is full,
		 * or is aConsoles is full.		-- ksb
		 * We'd have to redo some code in readcfg.c to allow
		 * parsing the test into the CONSENT.  Humm.	XXX/LLL
		 */
		if (0 == strcmp(acIn, "help")) {
			static char *apcHelp[] = {
				"call    provide port for given machine\r\n",
				"groups  provide ports for group leaders\r\n",
				"help    this help message\r\n",
				"master  provide a list of master servers\r\n",
				"pid     provide pid of master process\r\n",
				"quit    terminate conserver\r\n",
				"restart drop all connections and reload\r\n",
				"version provide version info for server\r\n",
				(char *)0
			};
			register char **ppc;
			for (ppc = apcHelp; (char *)0 != *ppc; ++ppc) {
				(void)write(cfd, *ppc, strlen(*ppc));
			}
#if TEST_FORK
			exit(0);
#else
			(void)close(cfd);
			continue;
#endif
		}
		if (0 == strcmp(acIn, "quit") || 0 == strcmp(acIn, "restart")) {
			register struct passwd *pwd;

			if ('t' == cType) {
				CSTROUT(cfd, "trusted -- terminated\r\n");
				goto q_common;
			} else if ((char *)0 == pcArgs) {
				CSTROUT(cfd, "must be trusted to terminate\r\n");
			} else if ((struct passwd *)0 == (pwd = getpwuid(0))) {
				CSTROUT(cfd, "no root passwd?\r\n");
			} else if (0 == CheckPass(pwd, (char *)0, pcArgs)) {
				CSTROUT(cfd, "Sorry.\r\n");
			} else {
				CSTROUT(cfd, "ok -- terminated\r\n");
 q_common:
				fSawQuit = acIn[0];
#if TEST_FORK
				kill(getppid(), 'q' == fSawQuit ? SIGTERM : SIGUSR2);
#endif
			}
#if TEST_FORK
			exit(0);
#else
			(void)close(cfd);
			continue;
#endif
		}
		if (0 == strcmp(acIn, "pid")) {
#if TEST_FORK
			sprintf(acOut, "%d\r\n", getppid());
			(void)write(cfd, acOut, strlen(acOut));
			exit(0);
#else
			sprintf(acOut, "%d\r\n", getpid());
			(void)write(cfd, acOut, strlen(acOut));
			(void)close(cfd);
			continue;
#endif
		}
		if (0 == strcmp(acIn, "groups")) {
			register int iSep = 1;

			for (i = 0; i < MAXGRP; ++i) {
				if (0 == aGroups[i].imembers)
					continue;
				sprintf(acOut, ":%d"+iSep, ntohs((u_short)aGroups[i].port));
				(void)write(cfd, acOut, strlen(acOut));
				iSep = 0;
			}
			CSTROUT(cfd, "\r\n");
#if TEST_FORK
			exit(0);
#else
			(void)close(cfd);
			continue;
#endif
		}
		if (0 == strcmp(acIn, "master")) {
			register int iSep = 1;

			if (0 != iLocal) {
				sprintf(acOut, "@%s", acMyHost);
				(void)write(cfd, acOut, strlen(acOut));
				iSep = 0;
			}
			for (pRC = pRCUniq; (REMOTE *)0 != pRC; pRC = pRC->pRCuniq) {
				sprintf(acOut, ":@%s"+iSep, pRC->rhost);
				(void)write(cfd, acOut, strlen(acOut));
				iSep = 0;
			}
			CSTROUT(cfd, "\r\n");
#if TEST_FORK
			exit(0);
#else
			(void)close(cfd);
			continue;
#endif
		}
		if (0 == strcmp(acIn, "version")) {
			sprintf(acOut, "version `%s\'\r\n", rcsid);
			(void)write(cfd, acOut, strlen(acOut));
#if TEST_FORK
			exit(0);
#else
			(void)close(cfd);
			continue;
#endif
		}
		if (0 != strcmp(acIn, "call")) {
			CSTROUT(cfd, "unknown command\r\n");
			(void)close(cfd);
#if TEST_FORK
			exit(0);
#else
			(void)close(cfd);
			continue;
#endif
		}

		/* look up the machine to call
		 */
		found = 0;
		pRCFound = (REMOTE *)0;
		/* look for a local machine */
		for (i = 0; i < MAXGRP; ++i) {
			if (0 == aGroups[i].imembers)
				continue;
			for (j = 0; j < aGroups[i].imembers; ++j) {
				if (0 != strncmp(pcArgs, aGroups[i].pCElist[j].server, strlen(pcArgs))) {
					continue;
				}
				prnum = ntohs((u_short)aGroups[i].port);
				/* exact match for `host2' vs `host21'	(ksb)
				 */
				if (0 == strcmp(pcArgs, aGroups[i].pCElist[j].server)) {
					goto exact_match;
				}
				++found;
			}
		}
		/* look for a remote server */
		for (pRC = pRCList; (REMOTE *)0 != pRC; pRC = pRC->pRCnext) {
			if (0 != strncmp(pcArgs, pRC->rserver, strlen(pcArgs))) {
				continue;
			}
			pRCFound = pRC;
			if (0 == strcmp(pcArgs, pRC->rserver)) {
				goto exact_match;
			}
			++found;
		}
		switch (found) {
		case 0:
			sprintf(acOut, "server %s not found\r\n", pcArgs);
			break;
 exact_match:
			/* we have the complete hostname -- don't look at
			 * any partial matches
			 */
		case 1:
			if ((REMOTE *)0 != pRCFound) {
				sprintf(acOut, "@%s\r\n", pRCFound->rhost, pcArgs);
			} else {
				sprintf(acOut, "%d\r\n", prnum);
			}
			break;
		default:
			sprintf(acOut, "ambigous server abbreviation, %s\r\n", pcArgs);
			break;
		}
		(void)write(cfd, acOut, strlen(acOut));
		(void)close(cfd);
#if TEST_FORK
		exit(0);
#endif
	}
	return fSawQuit;
}
