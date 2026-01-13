/*
 * $Id: group.c,v 8.6 2000/05/09 00:42:32 ksb Exp $
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
 *
 *
 * Copyright 1992 Purdue Research Foundation, West Lafayette, Indiana
 * 47907.  All rights reserved.
 *
 * Recoded by Kevin S Braunsdorf, ksb@cc.purdue.edu, purdue!ksb
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
#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1990 The Ohio State University.\n\
@(#) Copyright 1992 Purdue Research Foundation.\n\
All rights reserved.\n";
#endif
#include <sys/types.h>
#include <sys/param.h>
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
#include "access.h"
#include "group.h"
#include "main.h"

#if DO_VIRTUAL
#if HAVE_PTYD
#include "local/openpty.h"
#else
extern int FallBack();
#endif /* ptyd */
#endif /* virtual consoles */

#if DO_ANNEX
#include <arpa/telnet.h>
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

#if USE_TERMIO
#include <termio.h>

#else
#if USE_TERMIOS
#include <termios.h>

#else	/* use ioctl stuff */
#include <sgtty.h>
#include <sys/ioctl.h>
#endif
#endif

#if USE_STREAMS
#include <stropts.h>
#endif

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#if HAVE_SHADOW
#include <shadow.h>
#endif

#if HAVE_SELECT_H
#include <sys/select.h>
#endif

extern char *crypt();
extern time_t time();

#if NEED_SHADOW
static char acShadow[] = PATH_SHADOW;

typedef struct spwd {
	char sp_name[16];		/* login name */
	char sp_pwdp[48];		/* FREEBSD uses more that 13 */
	/* we do not emulate the rest (yet) ZZZ */
} SPWD;

/* terrible hack to get around C2 under SunOS 4.1.3_U1			(ksb)
 * no getspnam(3) in libc, so we have a hacked up one here.
 * If you find a better way, let me know -- Please.
 */
struct spwd *
getspname(pcWho)
char *pcWho;
{
	static struct spwd SPThis;
	register FILE *fpShadow;
	register int iLen;
	auto char acLine[132];

	if ((FILE *)0 == (fpShadow = fopen(acShadow, "r"))) {
		fprintf(stderr, "%s: fopen: %s: %s\n", progname, acShadow, strerror(errno));
		return (struct spwd *)0;
	}

	iLen = strlen(pcWho);
	while ((char *)0 != fgets(acLine, sizeof(acLine), fpShadow)) {
		if (0 != strncmp(acLine, pcWho, iLen) || ':' != acLine[iLen]) {
			continue;
		}
		(void)fclose(fpShadow);

		(void)strcpy(SPThis.sp_name, pcWho);
		pcWho = & SPThis.sp_pwdp[0];
		++iLen;
		do {
			*pcWho++ = acLine[iLen];
		} while (':' != acLine[++iLen]);
		*pcWho = '\000';
		return & SPThis;
	}
	(void)fclose(fpShadow);
	return (struct spwd *)0;
}
#endif

/* Is this passwd a match for this user's passwd? 		(gregf/ksb)
 * look up passwd in shadow file if we have to, if we are
 * given a special epass try it first.
 * all the acSalt stuff is for dump crypts which fail on
 * salts with extra characters... sigh
 */
#if USE_MD5
/*
 * MD5 code added by ajk for use on copernicus.physics.purdue.edu,
 * the console server for the Physics Computer Network at Purdue
 * University, which is quite happy to be running FreeBSD.
 *
 * $Id: group.c,v 8.6 2000/05/09 00:42:32 ksb Exp $
 */
static int
MD5CheckPass(pcPass, pcWord)
	char *pcPass, *pcWord;
{
	char acSalt[9];
	char *pcSalt;
	const char *pcMagic = "$1$";
	size_t iMagic;
	char *pcDollar;

	iMagic = strlen(pcMagic);
	pcSalt = pcPass;
	/* skip a preceding magic string */
	if (0 == strncmp(pcSalt, pcMagic, iMagic))
		pcSalt += (int) iMagic;

	/* grab the salt (to the next '$', or 8 chars max.) */
	(void) strncpy(acSalt, pcSalt, 8);
	acSalt[8] = '\0';
	pcDollar = strchr(acSalt, '$');
	if (pcDollar)
		*pcDollar = '\0';

	/* Don't hesitate; authenticate! */
	return 0 == strcmp(pcPass, crypt(pcWord, acSalt));
}

int
CheckPass(pwd, pcEPass, pcWord)
struct passwd *pwd;
char *pcEPass, *pcWord;
{
	if (pcEPass && '\0' != pcEPass[0])
		if (MD5CheckPass(pcEPass, pcWord))
			return 1;
	return MD5CheckPass(pwd->pw_passwd, pcWord);
}
#else /* USE_MD5 */
int
CheckPass(pwd, pcEPass, pcWord)
struct passwd *pwd;
char *pcEPass, *pcWord;
{
	auto char acSalt[4];
#if HAVE_SHADOW
	register struct spwd *spwd;
#endif

	acSalt[2] = '\000';
	acSalt[3] = '\000';
	if ((char *)0 != pcEPass && '#' == pcEPass[0] && '#' == pcEPass[1] && '\000' != pcEPass[2] && 13 > strlen(pcEPass)) {
		/* If the encrypted password field from the config file
		 * looks like ##USER, look up the password for that
		 * user for the match.
		 */
		pwd = getpwnam(pcEPass+2);
		if ((struct passwd *)0 == pwd) {
			return 0;
		}
		pcEPass = (char *)0;
	}
	if ((char *)0 != pcEPass && '\000' != pcEPass[0]) {
		acSalt[0] = pcEPass[0];
		acSalt[1] = pcEPass[1];
		if (0 == strcmp(pcEPass, crypt(pcWord, acSalt))) {
			return 1;
		}
	}
#if HAVE_SHADOW
	if ('x' == pwd->pw_passwd[0] && '\000' == pwd->pw_passwd[1]) {
		spwd = getspnam(pwd->pw_name);
		acSalt[0] = spwd->sp_pwdp[0];
		acSalt[1] = spwd->sp_pwdp[1];
		return 0 == strcmp(spwd->sp_pwdp, crypt(pcWord, acSalt));
	}
#endif
	acSalt[0] = pwd->pw_passwd[0];
	acSalt[1] = pwd->pw_passwd[1];
	return 0 == strcmp(pwd->pw_passwd, crypt(pcWord, acSalt));
}
#endif /* USE_MD5 */

/* on an HUP close and re-open log files so lop can trim them		(ksb)
 * lucky for us: log file fd's can change async from the group driver!
 */
static GRPENT *pGEHup;
static SIGRET_T
ReOpen(arg)
int arg;
{
	register int i;
	register CONSENT *pCE;

#if !HAVE_STICKY_SIGNALS
	(void)signal(SIGHUP, ReOpen);
#endif

	if ((GRPENT *)0 == pGEHup) {
		return;
	}

	for (i = 0, pCE = pGEHup->pCElist; i < pGEHup->imembers; ++i, ++pCE) {
		if (-1 == pCE->fdlog) {
			continue;
		}
		(void)close(pCE->fdlog);
		if (-1 == (pCE->fdlog = open(pCE->lfile, O_RDWR|O_CREAT|O_APPEND, 0666))) {
			CSTROUT(2, "conserver: help! cannot reopen log files!\n");
			continue;
		}
	}
}

#if DO_VIRTUAL
/* on a TERM we have to cleanup utmp entries (ask ptyd to do it)	(ksb)
 */
static SIGRET_T
DeUtmp(sig)
int sig;
{
	register int i;
	register CONSENT *pCE;

#if !HAVE_STICKY_SIGNALS
	(void)signal(SIGTERM, DeUtmp);
#endif

	if ((GRPENT *)0 == pGEHup) {
		return;
	}

	for (i = 0, pCE = pGEHup->pCElist; i < pGEHup->imembers; ++i, ++pCE) {
		if (-1 == pCE->fdtty || 0 == pCE->fvirtual) {
			continue;
		}
		/* running processes get 1 second to shutdown (ksb)
		 */
		if (-1 != pCE->ipid) {
			if (-1 != kill(pCE->ipid, SIGTERM))
				sleep(1);
		}
#if HAVE_PTYD
		(void)closepty(pCE->acslave, pCE->dfile, OPTY_UTMP, pCE->fdlog);
#else
		(void)close(pCE->fdlog);
#endif
	}
	exit(0);
	_exit(0);
	abort();
	/*NOTREACHED*/
}

/* virtual console procs are our kids, when they die we get a CHLD	(ksb)
 * which will send us here to clean up the exit code.  The lack of a
 * reader on the pseudo will cause us to notice the death in Kiddie...
 */
static SIGRET_T
ReapVirt(sig)
int sig;
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
		/* stopped child is just continuted
		 */
		if (WIFSTOPPED(UWbuf) && 0 == kill(pid, SIGCONT)) {
			continue;
		}
	}
#if !HAVE_STICKY_SIGNALS
	(void)signal(SIGCHLD, ReapVirt);
#endif
}
#endif


time_t tServing;	/* the time we started serving requests, i-cmd	*/
			/* XXX: should be in GMT, really		*/

static char acStop[] = {	/* buffer for oob stop command		*/
	OB_SUSP, 0
};
#if DO_POWER
static char acPower[] = {	/* buffer for oob power controller	*/
	OB_POWER, 0
};
#endif

/* routine used by the child processes.				   (ksb/fine)
 * Most of it is escape sequence parsing.
 * fine:
 *	All of it is squirrely code, for which I most humbly apologize
 * ksb:
 *	Note the states a client can be in, all of the client processing
 *	is done one character at a time, we buffer and shift a lot -- this
 *	stops the denial of services attack where a user telnets to the
 *	group port and just hangs it (by not following the protocol).  I've
 *	repaired this by letting all new clients on a bogus console that is
 *	a kinda control line for the group. They have to use the `;'
 *	command to shift to a real console before they can get any (real)
 *	thrills.
 *
 *	If you were not awake in your finite state machine course this code 
 *	should scare the shit out of you; but there are a few invarients:
 *		- the fwr (I can write) bit always is set *after* the
 *		  notification that to the console (and reset before)
 *		- we never look at more than one character at a time, even
 *		  when we read a hunk from the MUX we string it out in a loop
 *		- look at the output (x, u, w) and attach (a, f, ;) commands
 *		  for more clues
 *
 *	NB: the ZZZ markers below indicate places where I didn't have time
 *	    (machine?) to test some missing bit of tty diddling, I'd love
 *	    patches for other ioctl/termio/termios stuff -- ksb
 *		
 */
static void
Kiddie(pGE, sfd)
register GRPENT *pGE;
int sfd;
{
	register CLIENT
		*pCL,		/* console we must scan/notify		*/
		*pCLServing,	/* client we are serving		*/
		*pCLFree;	/* head of free list			*/
	register CONSENT
		*pCEServing,	/* console we are talking to		*/
		*pCE;		/* the base of our console list		*/
	register int iConsole;
	register int i, nr;
	register struct hostent *hpPeer;
	register struct passwd *pwd;
	register char *pcPass;
	auto CONSENT CECtl;		/* our control `console'	*/
	auto char cType;
	auto int iMaxFd, iSelFd, so;
	auto fd_set rmask, rinit;
	auto char acOut[BUFSIZ], acIn[BUFSIZ], acNote[132*2];
	auto CLIENT dude[MAXMEMB];	/* alloc one set per console	*/
#if HAVE_RLIMIT
	struct rlimit rl;
#endif
#if USE_TERMIO
	auto struct sgttyb sty;
#else
#if USE_TERMIOS
	auto struct termios sbuf;
#else /* ioctl, like BSD4.2 */
	auto struct sgttyb sty;
#endif
#endif


	/* turn off signals that master() might turned on
	 * (only matters if respawned)
	 */
	(void)signal(SIGURG, SIG_DFL);
#if DO_VIRTUAL
	(void)signal(SIGTERM, DeUtmp);
	(void)signal(SIGCHLD, ReapVirt);
#else
	(void)signal(SIGTERM, SIG_DFL);
	(void)signal(SIGCHLD, SIG_DFL);
#endif

	/* setup our local data structures and fields, and contol line
	 */
	pCE = pGE->pCElist;
	for (iConsole = 0; iConsole < pGE->imembers; ++iConsole) {
		pCE[iConsole].fup = 0;
		pCE[iConsole].pCLon = pCE[iConsole].pCLwr = (CLIENT *)0;
#if DO_VIRTUAL
		pCE[iConsole].fdlog = -1;
		if (0 == pCE[iConsole].fvirtual) {
			pCE[iConsole].fdtty = -1;
			continue;
		}
		/* open a pty for each vitrual console
		 */
#if HAVE_PTYD
		pCE[iConsole].fdtty = openpty(pCE[iConsole].acslave, pCE[iConsole].dfile, OPTY_UTMP, 0);
#else /* oops, get ptyd soon */
		/* we won't get a utmp entry... *sigh*
		 */
		pCE[iConsole].fdtty = FallBack(pCE[iConsole].acslave, pCE[iConsole].dfile);
#endif /* find openpty */
#else
		pCE[iConsole].fdlog = pCE[iConsole].fdtty = -1;
#endif
	}
	sprintf(CECtl.server, "ctl_%d", pGE->port);
	CECtl.inamelen = strlen(CECtl.server);	/* bogus, of course	*/
	CECtl.acline[CECtl.inamelen++] = ':';
	CECtl.acline[CECtl.inamelen++] = ' ';
	CECtl.iend = CECtl.inamelen;
	(void)strcpy(CECtl.dfile, strcpy(CECtl.lfile, "/dev/null"));
	/* below "" gets us the default parity and baud structs
	 */
	(void)XlateSpeed("", & CECtl.pbaud, & CECtl.pparity);
	CECtl.fdlog = CECtl.fdtty = -1;
	CECtl.fup = 0;
	CECtl.pCLon = CECtl.pCLwr = 0;

	/* set up stuff for the select() call once, then just copy it
	 * rinit is all the fd's we might get data on, we copy it
	 * to rmask before we call select, this saves lots of prep work
	 * we used to do in the loop, but we have to mod rinit whenever
	 * we add a connection or drop one...	(ksb)
	 */
#if HAVE_RLIMIT
	getrlimit(RLIMIT_NOFILE, &rl);
	iMaxFd = rl.rlim_cur;
#else
	iMaxFd = getdtablesize();
#endif
#if defined(FD_SETSIZE)
	if (iMaxFd > FD_SETSIZE) {
		iMaxFd = FD_SETSIZE;
	}
#endif
	FD_ZERO(&rinit);
	FD_SET(sfd, &rinit);
	iSelFd = sfd+1;

	/* open all the files we need for the consoles in our group
	 * if we can't get one (bitch and) flag as down
	 */
	for (iConsole = 0; iConsole < pGE->imembers; ++iConsole) {
		ConsInit(& pCE[iConsole], &rinit);
		if (pCE[iConsole].fdtty >= iSelFd) {
			iSelFd = pCE[iConsole].fdtty + 1;
		}
	}
	if (iSelFd > iMaxFd) {
		fprintf(stderr, "%s: select can't operate on %d files (limit %d)\n [make more smaller groups to work around]", progname, iSelFd, iMaxFd);

		/* do the ones we can , the control port should
		 * still work because we opened it first (by design).
		 */
		iSelFd = iMaxFd;
	}

	/* set up the list of free connection slots, we could just calloc
	 * them, but the stack pages are already in core...
	 */
	pCLFree = dude;
	for (i = 0; i < MAXMEMB-1; ++i) {
		dude[i].pCLnext = & dude[i+1];
	}
	dude[MAXMEMB-1].pCLnext = (CLIENT *)0;

	/* on a SIGHUP we should close and reopen our log files
	 */ 
	pGEHup = pGE;
	signal(SIGHUP, ReOpen);

	/* the MAIN loop a group server
	 */
	pGE->pCLall = (CLIENT *)0;
	while (1) {
		rmask = rinit;

		if (-1 == select(iSelFd, &rmask, (fd_set *)0, (fd_set *)0, (struct timeval *)0)) {
			fprintf(stderr, "%s: select: %s\n", progname, strerror(errno));
			continue;
		}

		/* anything from any console?
		 */
		for (iConsole = 0; iConsole < pGE->imembers; ++iConsole) {
			pCEServing = pCE+iConsole;
			if (!pCEServing->fup || !FD_ISSET(pCEServing->fdtty, &rmask)) {
				continue;
			}
			/* read terminal line */
			if ((nr = read(pCEServing->fdtty, acIn, sizeof(acIn))) < 1) {
				/* carrier lost */
#if DO_ANNEX
				if (pCEServing->fannexed) {
					fprintf(stderr, "%s: lost carrier on %s, %s:%d\n", progname, pCEServing->server, pCEServing->pcannex, pCEServing->haxport);
					FD_CLR(pCEServing->fdtty, &rinit);
					(void)close(pCEServing->fdtty);
					pCEServing->fdtty = -1;
					pCEServing->fup = 0;
				} else
#endif
#if DO_VIRTUAL
				if (pCEServing->fvirtual) {
					fprintf(stderr, "%s: lost carrier on %s, tty %s\n", progname, pCEServing->server, pCEServing->acslave);
					FD_CLR(pCEServing->fdtty, &rinit);
					pCEServing->ipid = -1;
					pCEServing->fup = 0;
					ConsInit(pCEServing, &rinit);
				} else
#endif
				{
					fprintf(stderr, "%s: lost carrier on %s, device %s (%d)\n", progname, pCEServing->server, pCEServing->dfile, nr);
					ConsInit(pCEServing, &rinit);
				}
				continue;
			}
#if CPARITY
			/* clear parity -- the log file looks better this way
			 */
			for (i = 0; i < nr; ++i) {
				acIn[i] &= 127;
			}
#endif

			/* log it and write to all connections on this server
			 */
			(void)write(pCEServing->fdlog, acIn, nr);

			/* output all console info nobody is attached
			 */
			if (fAll && (CLIENT *)0 == pCEServing->pCLwr) {
				/* run through the console ouptut,
				 * add each character to the output line
				 * drop and reset if we have too much
				 * or are at the end of a line (ksb)
				 */
				for (i = 0; i < nr; ++i) {
					pCEServing->acline[pCEServing->iend++] = acIn[i];
					if (pCEServing->iend < sizeof(pCEServing->acline) && '\n' != acIn[i]) {
						continue;
					}
					write(1, pCEServing->acline, pCEServing->iend);
					pCEServing->iend = pCEServing->inamelen;
				}
			}

			/* write console info to clients (not suspended)
			 */
			for (pCL = pCEServing->pCLon; (CLIENT *)0 != pCL; pCL = pCL->pCLnext) {
				if (pCL->fcon) {
					(void)write(pCL->fd, acIn, nr);
				}
			}
		}


		/* anything from a connection?
		 */
		for (pCLServing = pGE->pCLall; (CLIENT *)0 != pCLServing; pCLServing = pCLServing->pCLscan) {
			if (!FD_ISSET(pCLServing->fd, &rmask)) {
				continue;
			}
			pCEServing = pCLServing->pCEto;
#if DEBUG
			if ((pCLServing->fwr != 0) != (pCEServing->pCLwr == pCLServing)) {
				fprintf(stderr, "%s: internal check failed  %s on %s\n", progname, pCLServing->acid, pCEServing->server);
			}
#endif
			/* read connection */
			if ((nr = read(pCLServing->fd, acIn, sizeof(acIn))) == 0) {
				/* reached EOF - close connection */
drop:
				/* re-entry point to drop a connection
				 * (for any other reason)
				 * log it, drop from select list, and count
				 * close gap in table, restart loop
				 */
				if (fVerbose && &CECtl != pCEServing) {
					printf("%s: %s: logout %s\n", progname, pCEServing->server, pCLServing->acid);
				}

				FD_CLR(pCLServing->fd, &rinit);
				while (iSelFd > sfd && !FD_ISSET(iSelFd-1, &rinit)) {
					--iSelFd;
				}
				(void)close(pCLServing->fd);
				pCLServing->fd = -1;

				/* mark as not writer, if he is
				 */
				if (pCLServing->fwr) {
					pCLServing->fwr = 0;
					pCLServing->fwantwr = 0;
					pCEServing->pCLwr = FindWrite(pCEServing->pCLon);
				}

				/* mark as unconnected and remove from both
				 * lists (all clients, and this console)
				 */
				pCLServing->fcon = 0;
				if ((CLIENT *)0 != pCLServing->pCLnext) {
					pCLServing->pCLnext->ppCLbnext = pCLServing->ppCLbnext;
				}
				*(pCLServing->ppCLbnext) = pCLServing->pCLnext;
				if ((CLIENT *)0 != pCLServing->pCLscan) {
					pCLServing->pCLscan->ppCLbscan = pCLServing->ppCLbscan;
				}
				*(pCLServing->ppCLbscan) = pCLServing->pCLscan;

				/* the continue below will advance to a (ksb)
				 * legal client, even though we are now closed
				 * and in the fre list becasue pCLscan is used
				 * for the free list
				 */
				pCLServing->pCLnext = pCLFree;
				pCLFree = pCLServing;
				continue;
			}

			/* always clear parity from the network
			 */
			for (i = 0; i < nr; ++i) {
				acIn[i] &= 127;
			}

			for (i = 0; i < nr; ++i) switch (pCLServing->iState) {
			case S_IDENT:
				/* append chars to acid until white space
				 */
				if (pCLServing->icursor == sizeof(pCLServing->acid)) {
					CSTROUT(pCLServing->fd, "Name too long.\r\n");
					goto drop;
				}
				if (!isspace(acIn[i])) {
					pCLServing->acid[pCLServing->icursor++] = acIn[i];
					continue;
				}
				if (0 != pCLServing->icursor) {
					pCLServing->acid[pCLServing->icursor] = '\000';
				}
				pCLServing->icursor = 0;
				CSTROUT(pCLServing->fd, "host:\r\n");
				pCLServing->iState = S_HOST;
				continue;

			case S_HOST:
				/* append char to buffer, check for \n
				 * continue if incomplete
				 * else swtich to new host
				 */
				if (pCLServing->icursor == sizeof(pCLServing->accmd)) {
					CSTROUT(pCLServing->fd, "Host too long.\r\n");
					goto drop;
				}
				if (!isspace(acIn[i])) {
					pCLServing->accmd[pCLServing->icursor++] = acIn[i];
					continue;
				}
				if (0 != pCLServing->icursor) {
					pCLServing->accmd[pCLServing->icursor] = '\000';
				}
				/* try to move to the given console
				 */
				pCLServing->pCEwant = (CONSENT *)0;
				for (iConsole = 0; iConsole < pGE->imembers; ++iConsole) {
					if (0 == strcmp(pCLServing->accmd, pCE[iConsole].server)) {
						pCLServing->pCEwant = & pCE[iConsole];
						break;
					}
				}
				if ((CONSENT *)0 == pCLServing->pCEwant) {
					sprintf(acOut, "%s: no such console\r\n", pCLServing->accmd);
					(void)write(pCLServing->fd, acOut, strlen(acOut));
					goto drop;
				}

				pCLServing->icursor = 0;
				if ('t' == pCLServing->caccess || &CECtl != pCEServing) {
					goto shift_console;
				}
				CSTROUT(pCLServing->fd, "passwd:\r\n");
				pCLServing->iState = S_PASSWD;
				continue;

			case S_PASSWD:
				/* gather passwd, check and drop or
				 * set new state
				 */
				if (pCLServing->icursor == sizeof(pCLServing->accmd)) {
					CSTROUT(pCLServing->fd, "Passwd too long.\r\n");
					goto drop;
				}
				if ('\n' != acIn[i]) {
					pCLServing->accmd[pCLServing->icursor++] = acIn[i];
					continue;
				}
				pCLServing->accmd[pCLServing->icursor] = '\000';
				if ('\r' == pCLServing->accmd[pCLServing->icursor-1]) {
					pCLServing->accmd[--pCLServing->icursor] = '\000';
				}
				pCLServing->icursor = 0;

				/* in the CONFIG file gave this group a
				 * password use it before root's
				 * password  (for malowany)
				 */
				if ((struct passwd *)0 == (pwd = getpwuid(0))) {
					CSTROUT(pCLServing->fd, "no root passwd?\r\n");
					goto drop;
				}
				if (0 == CheckPass(pwd, pGE->passwd, pCLServing->accmd)) {
					CSTROUT(pCLServing->fd, "Sorry.\r\n");
					printf("%s: %s: %s: bad passwd\n", progname, pCLServing->pCEwant->server, pCLServing->acid);
					goto drop;
				}
	shift_console:
				/* remove from current host
				 */
				if ((CLIENT *)0 != pCLServing->pCLnext) {
					pCLServing->pCLnext->ppCLbnext = pCLServing->ppCLbnext;
				}
				*(pCLServing->ppCLbnext) = pCLServing->pCLnext;
				if (pCLServing->fwr) {
					pCLServing->fwr = 0;
					pCLServing->fwantwr = 0;
					pCEServing->pCLwr = FindWrite(pCEServing->pCLon);
				}

				/* inform operators of the change
				 */
				if (fVerbose) {
					if (&CECtl == pCEServing) {
						printf("%s: %s: login %s\n", progname, pCLServing->pCEwant->server, pCLServing->acid);
					} else {
						printf("%s: %s moves from %s to %s\n", progname, pCLServing->acid, pCEServing->server, pCLServing->pCEwant->server);
					}
				}

				/* set new host and link into new host list
				 */
				pCEServing = pCLServing->pCEwant;
				pCLServing->pCEto = pCEServing;
				pCLServing->pCLnext = pCEServing->pCLon;
				pCLServing->ppCLbnext = & pCEServing->pCLon;
				if ((CLIENT *)0 != pCLServing->pCLnext) {
					pCLServing->pCLnext->ppCLbnext = & pCLServing->pCLnext;
				}
				pCEServing->pCLon = pCLServing;

				/* try for attach on new console
				 */
				if (!pCEServing->fup) {
					CSTROUT(pCLServing->fd, "line to host is down]\r\n");
				} else if (pCEServing->fronly) {
					CSTROUT(pCLServing->fd, "host is read-only]\r\n");
				} else if ((CLIENT *)0 == pCEServing->pCLwr) {
					pCEServing->pCLwr = pCLServing;
					pCLServing->fwr = 1;
					CSTROUT(pCLServing->fd, "attached]\r\n");
					/* this keeps the ops console neat */
					pCEServing->iend = pCEServing->inamelen;
				} else {
					CSTROUT(pCLServing->fd, "spy]\r\n");
				}
				pCLServing->fcon = 1;
				pCLServing->iState = S_NORMAL;
				continue;

			case S_QUOTE:	/* send octal code		*/
				/* must type in 3 octal digits */
				if (isdigit(acIn[i])) {
					pCLServing->accmd[0] *= 8;
					pCLServing->accmd[0] += acIn[i] - '0';
					if (++(pCLServing->icursor) < 3) {
						write(pCLServing->fd, &acIn[i], 1);
						continue;
					}
					pCLServing->accmd[1] = acIn[i];
					pCLServing->accmd[2] = ']';
					write(pCLServing->fd, pCLServing->accmd+1, 2);
					(void)write(pCEServing->fdtty, pCLServing->accmd, 1);
				} else {
					CSTROUT(pCLServing->fd, " aborted]\r\n");
				}
				pCLServing->iState = S_NORMAL;
				continue;

#if DO_POWER
			case S_POWER:	/* the power sequence */
				switch (acIn[i]) {
				case 'k': /* key for this line */
					if ('\000' == pCEServing->acpower[0])
						(void)write(pCLServing->fd, pCEServing->acline, strlen(pCEServing->acline));
					else
						(void)write(pCLServing->fd, pCEServing->acpower, strlen(pCEServing->acpower));
					CSTROUT(pCLServing->fd, "\r\n");
					continue;
				case 's': /* show console to connect */
					(void)write(pCLServing->fd, pCEServing->acctlr, strlen(pCEServing->acctlr));
					CSTROUT(pCLServing->fd, "\r\n");
					continue;
				default:	/* I'm confused */
					CSTROUT(pCLServing->fd, " aborted");
					break;
				case 'b':	/* back to the connection */
					break;
				}
				/* fall through to resume code */
#endif
				/* might fall in from power code */
			case S_SUSP:
				if (!pCEServing->fup) {
					CSTROUT(pCLServing->fd, " -- line down]\r\n");
				} else if (pCEServing->fronly) {
					CSTROUT(pCLServing->fd, " -- read-only]\r\n");
				} else if ((CLIENT *)0 == pCEServing->pCLwr) {
					pCEServing->pCLwr = pCLServing;
					pCLServing->fwr = 1;
					pCLServing->fwantwr = 0;
					CSTROUT(pCLServing->fd, " -- attached]\r\n");
				} else {
					CSTROUT(pCLServing->fd, " -- spy mode]\r\n");
				}
				pCLServing->fcon = 1;
				pCLServing->iState = S_NORMAL;
				continue;

			case S_NORMAL:
				/* if it is an escape sequence shift states
				 */
				if (acIn[i] == pCLServing->ic[0]) {
					pCLServing->iState = S_ESC1;
					continue;
				}
				/* if we can write, write to slave tty
				 */
				if (pCLServing->fwr) {
					(void)write(pCEServing->fdtty, &acIn[i], 1);
					continue;
				}
				/* if the client is stuck in spy mode
				 * give them a clue as to how to get out
				 * (LLL nice to put chars out as ^Ec, rather
				 * than octal escapes, but....)
				 */
				if ('\r' == acIn[i] || '\n' == acIn[i]) {
					static char acA1[16], acA2[16];
					sprintf(acOut, "[read-only -- use %s %s ? for help]\r\n", FmtCtl(pCLServing->ic[0], acA1), FmtCtl(pCLServing->ic[1], acA2));
					(void)write(pCLServing->fd, acOut, strlen(acOut));
				}
				continue;

			case S_HALT1: 	/* halt sequence? */
				pCLServing->iState = S_NORMAL;
				if (acIn[i] != '1') {
					CSTROUT(pCLServing->fd, "aborted]\r\n");
					continue;
				}

				/* send a break
				 */
#if DO_ANNEX
				if (0 != pCEServing->fannexed) {
					auto char acHalt[4];
					acHalt[0] = IAC;
					acHalt[1] = BREAK;
					(void)write(pCEServing->fdtty, acHalt, 2);
					CSTROUT(pCLServing->fd, "requested]\r\n");
					continue;
				}
#endif
#if USE_TERMIO
				if (-1 == ioctl(pCEServing->fdtty, TCSBRK, (char *)0)) {
					CSTROUT(pCLServing->fd, "failed]\r\n");
					continue;
				}
#else
#if USE_TERMIOS || USE_TCBREAK
				CSTROUT(pCLServing->fd, "- ");
				if (-1 == tcsendbreak(pCEServing->fdtty, 0)) {
					CSTROUT(pCLServing->fd, "failed]\r\n");
					continue;
				}
#else
				if (-1 == ioctl(pCEServing->fdtty, TIOCSBRK, (char *)0)) {
					CSTROUT(pCLServing->fd, "failed]\r\n");
					continue;
				}
				CSTROUT(pCLServing->fd, "- ");
				/* ZZZ this should be a state we go into
				 * with a timeout in the console entry and
				 * we shouldn't block here -- ksb
				 * (unless there is one line in this group)
				 */
				sleep(3);
				if (-1 == ioctl(pCEServing->fdtty, TIOCCBRK, (char *)0)) {
					CSTROUT(pCLServing->fd, "failed]\r\n");
					continue;
				}
#endif
#endif
				CSTROUT(pCLServing->fd, "sent]\r\n");
				continue;

			case S_CATTN:	/* redef escape sequence? */
				pCLServing->ic[0] = acIn[i];
				sprintf(acOut, "%s ", FmtCtl(acIn[i], pCLServing->accmd));
				(void)write(pCLServing->fd, acOut, strlen(acOut));
				pCLServing->iState = S_CESC;
				continue;

			case S_CESC:	/* escape sequent 2 */
				pCLServing->ic[1] = acIn[i];
				pCLServing->iState = S_NORMAL;
				sprintf(acOut, "%s  ok]\r\n", FmtCtl(acIn[i], pCLServing->accmd));
				(void)write(pCLServing->fd, acOut, strlen(acOut));
				continue;

			case S_ESC1:	/* first char in escape sequence */
				if (acIn[i] == pCLServing->ic[1]) {
					if (pCLServing->fecho)
						CSTROUT(pCLServing->fd, "\r\n[");
					else
						CSTROUT(pCLServing->fd, "[");
					pCLServing->iState = S_CMD;
					continue;
				}
				/* ^E^Ec or ^_^_^[
				 * pass first ^E (^_) and stay in same state
				 */
				if (acIn[i] == pCLServing->ic[0]) {
					if (pCLServing->fwr) {
						(void)write(pCEServing->fdtty, pCLServing->ic, 1);
					}
					continue;
				}
				/* ^Ex or ^_x
				 * pass both characters to slave tty
				 */
				pCLServing->iState = S_NORMAL;
				if (pCLServing->fwr) {
					(void)write(pCEServing->fdtty, pCLServing->ic, 1);
					(void)write(pCEServing->fdtty, &acIn[i], 1);
				}
				continue;

			case S_CMD:	/* have 1/2 of the escape sequence */
				pCLServing->iState = S_NORMAL;
				switch (acIn[i]) {
				case '+':
				case '-':
					if (0 != (pCLServing->fecho = '+' == acIn[i]))
						CSTROUT(pCLServing->fd, "drop line]\r\n");
					else
						CSTROUT(pCLServing->fd, "no drop line]\r\n");
					break;

				case ';':	/* ;login: */
					CSTROUT(pCLServing->fd, "login:\r\n");
					pCLServing->iState = S_IDENT;
					break;

				case 'a':	/* attach */
				case 'A':
					if (&CECtl == pCEServing) {
						sprintf(acOut, "no -- on ctl]\r\n");
					} else if (!pCEServing->fup) {
						sprintf(acOut, "line to host is down]\r\n");
					} else if (pCEServing->fronly) {
						sprintf(acOut, "host is read-only]\r\n");
					} else if ((CLIENT *)0 == (pCL = pCEServing->pCLwr)) {
						pCEServing->pCLwr = pCLServing;
						pCLServing->fwr = 1;
						sprintf(acOut, "attached]\r\n");
					} else if (pCL == pCLServing) {
						sprintf(acOut, "ok]\r\n", pCL->acid);
					} else {
						pCLServing->fwantwr = 1;
						sprintf(acOut, "no, %s is attached]\r\n", pCL->acid);
					}
					(void)write(pCLServing->fd, acOut, strlen(acOut));
					break;

				case 'c':
				case 'C':
#if USE_TERMIOS
					if (-1 == tcgetattr(pCEServing->fdtty, & sbuf)) {
						CSTROUT(pCLServing->fd, "failed]\r\n");
						continue;
					}
					if (0 != (sbuf.c_iflag & IXOFF)) {
						sbuf.c_iflag &= ~(IXOFF|IXON);
						CSTROUT(pCLServing->fd, "flow OFF]\r\n");
					} else {
						sbuf.c_iflag |= IXOFF|IXON;
						CSTROUT(pCLServing->fd, "flow ON]\r\n");
					}
					(void)tcsetattr(pCEServing->fdtty, TCSANOW, & sbuf);
#else
					if (-1 == ioctl(pCEServing->fdtty, TIOCGETP, (char *)&sty)) {
						CSTROUT(pCLServing->fd, "failed]\r\n");
						break;
					}
					if (0 != (sty.sg_flags & TANDEM)) {
						sty.sg_flags &= ~TANDEM;
						CSTROUT(pCLServing->fd, "flow OFF]\r\n");
					} else {
						sty.sg_flags |= TANDEM;
						CSTROUT(pCLServing->fd, "flow ON]\r\n");
					}
					(void)ioctl(pCEServing->fdtty, TIOCSETP, (char *)&sty);
#endif
					break;

				case 'd':	/* down a console	*/
				case 'D':
					if (&CECtl == pCEServing) {
						CSTROUT(pCLServing->fd, "no -- on ctl]\r\n");
						continue;
					}
					if (!pCLServing->fwr && !pCEServing->fronly) {
						CSTROUT(pCLServing->fd, "attach to down line]\r\n");
						break;
					}
					if (!pCEServing->fup) {
						CSTROUT(pCLServing->fd, "ok]\r\n");
						break;
					}

					pCLServing->fwr = 0;
					pCEServing->pCLwr = (CLIENT *)0;
					ConsDown(pCEServing, &rinit);
					CSTROUT(pCLServing->fd, "line down]\r\n");

					/* tell all who closed it */
					sprintf(acOut, "[line down by %s]\r\n", pCLServing->acid);
					for (pCL = pCEServing->pCLon; (CLIENT *)0 != pCL; pCL = pCL->pCLnext) {
						if (pCL == pCLServing)
							continue;
						if (pCL->fcon) {
							(void)write(pCL->fd, acOut, strlen(acOut));
						}
					}
					break;

				case 'e':	/* redefine escape keys */
				case 'E':
					pCLServing->iState = S_CATTN;
					CSTROUT(pCLServing->fd, "redef: ");
					break;

				case 'f':	/* force attach */
				case 'F':
					if (&CECtl == pCEServing) {
						CSTROUT(pCLServing->fd, "no -- on ctl]\r\n");
						continue;
					} else if (pCEServing->fronly) {
						CSTROUT(pCLServing->fd, "host is read-only]\r\n");
						continue;
					} else if (!pCEServing->fup) {
						CSTROUT(pCLServing->fd, "line to host is down]\r\n");
						continue;
					}
					sprintf(acOut, "attached]\r\n");
					if ((CLIENT *)0 != (pCL = pCEServing->pCLwr)) {
						if (pCL == pCLServing) {
							CSTROUT(pCLServing->fd, "ok]\r\n");
							break;
						}
						pCL->fwr = 0;
						pCL->fwantwr = 1;
						sprintf(acOut, "bumped %s]\r\n", pCL->acid);
						sprintf(acNote, "\r\n[forced to `spy\' mode by %s]\r\n", pCLServing->acid);
						(void)write(pCL->fd, acNote, strlen(acNote));
					}
					pCEServing->pCLwr = pCLServing;
					pCLServing->fwr = 1;
					(void)write(pCLServing->fd, acOut, strlen(acOut));
					break;

				case 'g':	/* group info */
				case 'G':
					/* we do not show the ctl console
					 * else we'd get the client always
					 */
					sprintf(acOut, "group %s]\r\n", CECtl.server);
					(void)write(pCLServing->fd, acOut, strlen(acOut));
					for (pCL = pGE->pCLall; (CLIENT *)0 != pCL; pCL = pCL->pCLscan) {
						if (&CECtl == pCL->pCEto)
							continue;
						sprintf(acOut, " %-24.24s %c %-7.7s %.32s\r\n", pCL->acid, pCL == pCLServing ? '*' : ' ', pCL->fcon ? (pCL->fwr ? "attach" : pCL->fwantwr ? "spy+" : "spy") : "stopped", pCL->pCEto->server);
						(void)write(pCLServing->fd, acOut, strlen(acOut));
					}
					break;

				case 'P':	/* DEC vt100 pf1 */
				case 'h':	/* help			*/
				case 'H':
				case '?':
					HelpUser(pCLServing);
					break;

				/* 'j' should drop others on this console
				 * from spy+ to spy
				 */

				/* 'k' should kill/kick others on this console
				 * this might be fine-line stupid
				 */

#if DO_POWER
				case '\020':	/* ^P power access */
					if (pCEServing->fronly) {
						CSTROUT(pCLServing->fd, "can\'t power read-only host]\r\n");
						continue;
					}
					if (!pCLServing->fwr) {
						CSTROUT(pCLServing->fd, "attach to power]\r\n");
						continue;
					}
					if (1 != send(pCLServing->fd, acPower, 1, MSG_OOB)) {
						CSTROUT(pCLServing->fd, "send failed]\r\n");
						break;
					}
					pCLServing->fcon = 0;
					pCLServing->iState = S_POWER;
					if (pCEServing->pCLwr == pCLServing) {
						pCLServing->fwr = 0;
						pCLServing->fwantwr = 1;
						pCEServing->pCLwr = (CLIENT *)0;
					}
					break;
#endif
				case 'i':	/* info */
				case 'I':	/* info host */
					CSTROUT(pCLServing->fd, "info: ");
					(void)write(pCLServing->fd, acMyHost, strlen(acMyHost));
					if ('\000' != pCEServing->lfile[0]) {
						CSTROUT(pCLServing->fd, ":");
						(void)write(pCLServing->fd, pCEServing->lfile, strlen(pCEServing->lfile));
					} else {
						CSTROUT(pCLServing->fd, " no log");
					}
					CSTROUT(pCLServing->fd, ", started ");
					(void)write(pCLServing->fd, ctime(&tServing), 24);
					CSTROUT(pCLServing->fd, "]\r\n");
					break;

				case 'l':	/* halt character 1	*/
				case 'L':
					if (pCEServing->fronly) {
						CSTROUT(pCLServing->fd, "can\'t halt read-only host]\r\n");
						continue;
					}
					if (!pCLServing->fwr) {
						CSTROUT(pCLServing->fd, "attach to halt]\r\n");
						continue;
					}
					pCLServing->iState = S_HALT1;
					CSTROUT(pCLServing->fd, "halt ");
					break;

				case 'o':	/* close and re-open line */
				case 'O':
					if (&CECtl == pCEServing) {
						CSTROUT(pCLServing->fd, "no -- on ctl]\r\n");
						continue;
					}
					/* with a close/re-open we might
					 * change fd's
					 */
					ConsInit(pCEServing, &rinit);
					if (!pCEServing->fup) {
						sprintf(acOut, "line to host is still down]\r\n");
					} else if (pCEServing->fronly) {
						sprintf(acOut, "up read-only]\r\n");
					} else if ((CLIENT *)0 == (pCL = pCEServing->pCLwr)) {
						pCEServing->pCLwr = pCLServing;
						pCLServing->fwr = 1;
						sprintf(acOut, "up -- attached]\r\n");
					} else if (pCL == pCLServing) {
						sprintf(acOut, "up]\r\n", pCL->acid);
					} else {
						sprintf(acOut, "up, %s is attached]\r\n", pCL->acid);
					}
					(void)write(pCLServing->fd, acOut, strlen(acOut));
					break;

				case '\022':	/* ^R */
					CSTROUT(pCLServing->fd, "^R]\r\n");
					if (pCEServing->iend == pCEServing->inamelen) {
						Replay(pCEServing->fdlog, pCLServing->fd, 1);
					} else {
						write(pCLServing->fd, pCEServing->acline+pCEServing->inamelen , pCEServing->iend-pCEServing->inamelen);
					}
					break;

				case 'R':	/* DEC vt100 pf3 */
				case 'r':	/* replay 20 lines */
					CSTROUT(pCLServing->fd, "replay]\r\n");
					Replay(pCEServing->fdlog, pCLServing->fd, 20);
					break;

				case 'S':	/* DEC vt100 pf4 */
				case 's':	/* spy mode */
					pCLServing->fwantwr = 0;
					if (!pCLServing->fwr) {
						CSTROUT(pCLServing->fd, "ok]\r\n");
						break;
					}
					pCLServing->fwr = 0;
					pCEServing->pCLwr = FindWrite(pCEServing->pCLon);
					CSTROUT(pCLServing->fd, "spying]\r\n");
					break;

				case 'u':	/* hosts on server this */
				case 'U':
					CSTROUT(pCLServing->fd, "hosts]\r\n");
					for (iConsole = 0; iConsole < pGE->imembers; ++iConsole) {
						sprintf(acOut, " %-24.24s %c %-4.4s %-.40s\r\n", pCE[iConsole].server, pCE+iConsole == pCEServing ? '*' : ' ', pCE[iConsole].fup ? "up" : "down", pCE[iConsole].pCLwr ? pCE[iConsole].pCLwr->acid : pCE[iConsole].pCLon ? "<spies>" : "<none>");
						(void)write(pCLServing->fd, acOut, strlen(acOut));
					}
					break;

				case 'v':	/* version */
				case 'V':
					sprintf(acOut, "version `%s\']\r\n", rcsid);
					(void)write(pCLServing->fd, acOut, strlen(acOut));
					break;

				case 'w':	/* who */
				case 'W':
					sprintf(acOut, "who %s]\r\n", pCEServing->server);
					(void)write(pCLServing->fd, acOut, strlen(acOut));
					for (pCL = pCEServing->pCLon; (CLIENT *)0 != pCL; pCL = pCL->pCLnext) {
						sprintf(acOut, " %-24.24s %c %-.7s %s\r\n", pCL->acid, pCL == pCLServing ? '*' : ' ', pCL->fcon ? (pCL->fwr ? "attach" : pCL->fwantwr ? "spy+" : "spy") : "stopped", pCL->actym);
						(void)write(pCLServing->fd, acOut, strlen(acOut));
					}
					break;

				case 'x':
				case 'X':
					CSTROUT(pCLServing->fd, "examine]\r\n");
					for (iConsole = 0; iConsole < pGE->imembers; ++iConsole) {
#if DO_ANNEX
						if (pCE[iConsole].fannexed) {
							sprintf(acOut, " %-24.24s %c on %s:%d\r\n", pCE[iConsole].server, pCEServing == & pCE[iConsole] ? '*' : ' ', pCE[iConsole].pcannex, pCE[iConsole].haxport);
						} else
#endif
#if DO_VIRTUAL
						if (pCE[iConsole].fvirtual) {
							sprintf(acOut, " %-24.24s %c on %-32.32s at %5s%c\r\n", pCE[iConsole].server, pCEServing == & pCE[iConsole] ? '*' : ' ', pCE[iConsole].acslave, pCE[iConsole].pbaud->acrate, pCE[iConsole].pparity->ckey);
						} else
#endif
						sprintf(acOut, " %-24.24s %c on %-32.32s at %5s%c\r\n", pCE[iConsole].server, pCEServing == & pCE[iConsole] ? '*' : ' ', pCE[iConsole].dfile, pCE[iConsole].pbaud->acrate, pCE[iConsole].pparity->ckey);
						(void)write(pCLServing->fd, acOut, strlen(acOut));
					}
					break;

				/* "y" should show everything about
				 * this console in a block
				 */

				case 'z':	/* suspend the client */
				case 'Z':
				case '\032':
					if (1 != send(pCLServing->fd, acStop, 1, MSG_OOB)) {
						break;
					}
					pCLServing->fcon = 0;
					pCLServing->iState = S_SUSP;
					if (pCEServing->pCLwr == pCLServing) {
						pCLServing->fwr = 0;
						pCLServing->fwantwr = 0;
						pCEServing->pCLwr = (CLIENT *)0;
					}
					break;

				case '\t':	/* toggle tab expand	*/
#if USE_TERMIO
					/* ZZZ */
#else
#if USE_TERMIOS
					if (-1 == tcgetattr(pCEServing->fdtty, & sbuf)) {
						CSTROUT(pCLServing->fd, "failed]\r\n");
						continue;
					}
#if !defined(XTABS)		/* XXX hack */
#define XTABS   TAB3
#endif
					if (XTABS == (TABDLY&sbuf.c_oflag)) {
						sbuf.c_oflag &= ~TABDLY;
						sbuf.c_oflag |= TAB0;
					} else {
						sbuf.c_oflag &= ~TABDLY;
						sbuf.c_oflag |= XTABS;
					}
					if (-1 == tcsetattr(pCEServing->fdtty, TCSANOW, & sbuf)) {
						CSTROUT(pCLServing->fd, "failed]\r\n");
						continue;
					}
#else
					/* ZZZ */
#endif
#endif
					CSTROUT(pCLServing->fd, "tabs]\r\n");
					break;

				case 'Q':	/* DEC vt100 PF2 */
				case '.':	 /* disconnect */
				case '\004':
				case '\003':
					CSTROUT(pCLServing->fd, "disconnect]\r\n");
					nr = 0;
					if (!pCEServing->fup) {
						goto drop;
					}
#if USE_TERMIOS
					if (-1 != tcgetattr(pCEServing->fdtty, & sbuf) && 0 == (sbuf.c_iflag & IXOFF)) {
						sbuf.c_iflag |= IXOFF|IXON;
						(void)tcsetattr(pCEServing->fdtty, TCSANOW, & sbuf);
					}
#else
					if (-1 != ioctl(pCEServing->fdtty, TIOCGETP, (char *)&sty) && 0 == (sty.sg_flags & TANDEM)) {
						sty.sg_flags |= TANDEM;
						(void)ioctl(pCEServing->fdtty, TIOCSETP, (char *)&sty);
					}
#endif
					goto drop;

				case ' ':	/* abort escape sequence */
				case '\n':
				case '\r':
					CSTROUT(pCLServing->fd, "ignored]\r\n");
					break;

				case '\\':	/* quote mode (send ^Q,^S) */
					if (pCEServing->fronly) {
						CSTROUT(pCLServing->fd, "can\'t write to read-only host]\r\n");
						continue;
					}
					if (!pCLServing->fwr) {
						CSTROUT(pCLServing->fd, "attach to send character]\r\n");
						continue;
					}
					pCLServing->icursor = 0;
					pCLServing->accmd[0] = '\000';
					pCLServing->iState = S_QUOTE;
					CSTROUT(pCLServing->fd, "quote \\");
					break;
				default:	/* unknown sequence */
					CSTROUT(pCLServing->fd, "unknown -- use `?\']\r\n");
					break;
				}
				continue;
			}
		}


		/* if nothing on control line, get more
		 */
		if (!FD_ISSET(sfd, &rmask)) {
			continue;
		}

		/* accept new connections and deal with them
		 */
		so = sizeof(struct sockaddr_in);
		pCLFree->fd = accept(sfd, (struct sockaddr *)&pCLFree->cnct_port, &so);
		if (pCLFree->fd < 0) {
			fprintf(stderr, "%s: accept: %s\n", progname, strerror(errno));
			continue;
		}

		/* We use this information to verify			(ksb)
		 * the source machine as being local.
		 */
		so = sizeof(in_port);
		if (-1 == getpeername(pCLFree->fd, (struct sockaddr *)&in_port, &so)) {
			CSTROUT(pCLFree->fd, "getpeername failed\r\n");
			(void)close(pCLFree->fd);
			continue;
		}
		so = sizeof(in_port.sin_addr);
		if ((struct hostent *)0 == (hpPeer = gethostbyaddr((char *)&in_port.sin_addr, so, AF_INET))) {
			CSTROUT(pCLFree->fd, "unknown peer name\r\n");
			(void)close(pCLFree->fd);
			continue;
		}
		if ('r' == (cType = AccType(hpPeer))) {
			CSTROUT(pCLFree->fd, "access from your host refused\r\n");
			(void)close(pCLFree->fd);
			continue;
		}

		/* save pCL so we can advance to the next free one
		 */
		pCL = pCLFree;
		pCLFree = pCL->pCLnext;

		/* init the identification stuff
		 */
		sprintf(pCL->acid, "client@%.*s", sizeof(pCL->acid)-10, hpPeer->h_name);
		pCL->tym = time((long *)0);
		(void)strcpy(pCL->actym, ctime(&(pCL->tym)));
		pCL->actym[24] = '\000';

		/* link into the control list for the dummy console
		 */
		pCL->pCEto = & CECtl;
		pCL->pCLnext = CECtl.pCLon;
		pCL->ppCLbnext = & CECtl.pCLon;
		if ((CLIENT *)0 != pCL->pCLnext) {
			pCL->pCLnext->ppCLbnext = & pCL->pCLnext;
		}
		CECtl.pCLon = pCL;

		/* link into all clients list
		 */
		pCL->pCLscan = pGE->pCLall;
		pCL->ppCLbscan = & pGE->pCLall;
		if ((CLIENT *)0 != pCL->pCLscan) {
			pCL->pCLscan->ppCLbscan = & pCL->pCLscan;
		}
		pGE->pCLall = pCL;

		FD_SET(pCL->fd, &rinit);
		if (iSelFd <= pCL->fd) {
			iSelFd = pCL->fd+1;
		}

		/* init the fsm
		 */
		pCL->fecho = 0;
		pCL->iState = S_NORMAL;
		pCL->ic[0] = DEFATTN;
		pCL->ic[1] = DEFESC;
		pCL->caccess = cType;
		pCL->icursor = 0;

		/* mark as stopped (no output from console)
		 * and spy only (on chars to console)
		 */
		pCL->fcon = 0;
		pCL->fwr = 0;
		pCL->fwantwr = 0;
		CSTROUT(pCL->fd, "ok\r\n");

		/* remove from the free list
		 * if we ran out of static connections calloc some...
		 */
		if ((CLIENT *)0 == pCLFree) {
			pCLFree = (CLIENT *)calloc(2, sizeof(CLIENT));
			if ((CLIENT *)0 == pCLFree) {
				CSTROUT(2, "\007no memory in console server, help\007\n\007");
			} else {
				pCLFree->pCLnext = &pCLFree[1];
			}
		}
	}
}

/* create a child process:						(fine)
 * fork off a process for each group with an open socket for connections
 */
void
Spawn(pGE, fdConfig)
GRPENT *pGE;
int fdConfig;
{
	register int pid, sfd;
	auto int so;
	auto struct sockaddr_in lstn_port;
	auto int iTrue = 1;

	/* get a socket for listening
	 */
#if USE_STRINGS
	(void)bzero((char *)&lstn_port, sizeof(lstn_port));
#else
	(void)memset((void *)&lstn_port, 0, sizeof(lstn_port));
#endif
	lstn_port.sin_family = AF_INET;
	*(u_long *)&lstn_port.sin_addr = INADDR_ANY;
	lstn_port.sin_port = 0;

	/* create a socket to listen on
	 * (prepared by master so he can see the port number of the kid)
	 */
	if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "%s: socket: %s\n", progname, strerror(errno));
		exit(9);
	}
#if defined(SO_REUSEADDR) && defined(SOL_SOCKET)
	if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char *)&iTrue, sizeof(iTrue))<0) {
		fprintf(stderr, "%s: setsockopt: %s\n", progname, strerror(errno));
		exit(9);
	}
#endif
	if (bind(sfd, (struct sockaddr *)&lstn_port, sizeof(lstn_port))<0) {
		fprintf(stderr, "%s: bind: %s\n", progname, strerror(errno));
		exit(9);
	}
	so = sizeof(lstn_port);

	if (-1 == getsockname(sfd, (struct sockaddr *)&lstn_port, &so)) {
		fprintf(stderr, "%s: getsockname: %s\n", progname, strerror(errno));
		exit(9);
	}
	pGE->port = lstn_port.sin_port;

	(void)fflush(stderr);
	(void)fflush(stdout);
	switch (pid = fork()) {
	case -1:
		fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		exit(1);
	default:
		(void)close(sfd);
		pGE->pid = pid;
		return;
	case 0:
		/* we can't hold the lock on the config file open */
		close(fdConfig);
		sleep(1);	/* pause to let master get ready */
		break;
	}
	if (listen(sfd, SOMAXCONN) < 0) {
		fprintf(stderr, "%s: listen: %s\n", progname, strerror(errno));
		exit(9);
	}
	Kiddie(pGE, sfd);

	/* should never get here...
	 */
	(void)close(sfd);
	fprintf(stderr, "%s: internal flow error\n", progname);
	exit(1);
	/*NOTREACHED*/
}
