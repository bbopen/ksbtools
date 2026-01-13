#!mkcmd
from '<sys/types.h>'
from '<sys/socket.h>'
from '<sys/signal.h>'
from '<sys/time.h>'
from '<sys/param.h>'
from '<netinet/in.h>'
from '<stdarg.h>'
from '<netdb.h>'
from '<fcntl.h>'
from '<errno.h>'
from '<stdio.h>'
from '<ctype.h>'
from '"daemon.h"'
from '"machine.h"'
from '"main.h"'

require "std_help.m" "std_version.m" "util_whittle.m"

augment action 'V' {
	user 'VersionInfo();'
}

%i
static char rcsid[] =
	"$Id: labwatch.m,v 3.16 1997/10/02 18:25:31 ksb Exp $";

#if HAVE_TERMIOS
#include <termios.h>
#endif

#if HAVE_SELECT_H
#include <sys/select.h>
#endif

#if HAVE_STDLIB
#include <stdlib.h>
#endif

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif
%%

%h
#define AVEN_LEN	3

extern char *DiffStr(int iDiff);
extern int aiLoadWeight[AVEN_LEN];
extern int iFScale;
%%

integer variable "iConsoleUserWeight" { hidden init "5" }
integer variable "iTtyUserWeight" { hidden init "1" } 

%c

static void VersionInfo()
{
	printf("%s: labd reports on %d/udp, labstat clients connect on %d/tcp\n", progname, iLoadPort, iClientPort);
	printf("%s: default ack port is %d/udp\n", progname, iDefaultAckPort);
#if KEEP_STATISTICS
	printf("%s: server keeps ticket statistics\n", progname);
#endif
}

int aiLoadWeight[AVEN_LEN] = { 0, 0, 1 };
int iFScale = XMIT_SCALE;

/* parse the T and L options
 */
static int ParseNum(char cThis, char **ppcArg, int fFirst)
{
	char *pcArg = *ppcArg;

	if (! fFirst && ',' != *(pcArg++) ||  (! isdigit(*pcArg))) {
		fprintf(stderr, "%s: -%c invalid parameter\n", progname, cThis);
		exit(1);
	}

	return strtol(pcArg, ppcArg, 10);
}

static void ParseT(char cThis, char *pcArg)
{
	iConsoleUserWeight = ParseNum(cThis, &pcArg, 1);
	iTtyUserWeight = ParseNum(cThis, &pcArg, 0);
	if ('\000' != pcArg[0]) {
		fprintf(stderr, "%s: -%c invalid parameter\n", progname, cThis);
		exit(1);
	}
}

/* iFScale is the divisor of iLoadFac it must be
 * XMIT_SCALE * the sum of the LoadWeights
 */
static void ParseL(char cThis, char *pcArg)
{
	aiLoadWeight[0] = ParseNum(cThis, &pcArg, 1);
	aiLoadWeight[1] = ParseNum(cThis, &pcArg, 0);
	aiLoadWeight[2] = ParseNum(cThis, &pcArg, 0);
	if ('\000' != pcArg[0]) {
		fprintf(stderr, "%s: -%c invalid parameter\n", progname, cThis);
		exit(1);
	}
	iFScale = aiLoadWeight[0] + aiLoadWeight[1] + aiLoadWeight[2];
	iFScale *= XMIT_SCALE;
}

char *DiffStr(int iDiff)
{
	static char acBuf[100];
	char *pcBuf = acBuf;
	int dd, hh, mm, ss;

	if (0 > iDiff) {
		iDiff = -iDiff;
		*pcBuf++ = '-';
	}
	dd = iDiff / 86400;
	if (0 != dd) {
		sprintf(pcBuf, (1 == dd) ? "%d day " : "%d days ", dd);
		pcBuf += strlen(pcBuf);
	}
	iDiff %= 86400;

	hh = iDiff / 3600; iDiff %= 3600;
	mm = iDiff / 60; iDiff %= 60;
	ss = iDiff;
	sprintf(pcBuf, "%02d:%02d:%02d", hh, mm, ss);

	return acBuf;
}
%%

basename "labwatch" ""

long 'c' {
	named "iClientTimeout"
	param "secs"
	init "30"
	help "time to wait before timing out on labstat client"
}

long 'w' {
	named "iDeathWait"
	param "secs"
	init "900"
	help "time to wait before declaring an machine dead"
}

function 'T' {
	named "ParseT"
	param "console,tty"
	help "the weight of logins on the console, other ttys"
}

function 'L' {
	named "ParseL"
	param "one,five,fifteen"
	help "the weight of the one, five and fifteen minute loads"
}

long 'A' {
	hidden named "iDefaultAckPort"
	init "29047"
	param "port"
	help 'port to guess for "expected" hosts'
}

long 'N' {
	named "iNumHostsGuess"
	param "nhosts"
	init "200"
	help "first guess as to how many hosts there are"
}

file ["r"] 'C' {
	static named "fpExpectFile" "pcExpectFile"
	param "file"
	help "config file with machine list and maps"
}

boolean 'n' {
	named "fExec"
	init "1"
	help "don't daemonize on startup"
}

boolean 'd' {
	named "fDebug"
	init "0"
	help "run in debug mode, implies -n"
	user "%rnn = !%rni;"
}

%c
/* parse symbolic port names
 */
static int ParsePort(char *pcArg, char *pcProto)
{
	register struct servent *pSE;

	if (isdigit(pcArg[0])) {
		return atoi(pcArg);
	}
	if ((struct servent *)0 != (pSE = getservbyname(pcArg, pcProto))) {
		return ntohs(pSE->s_port);
	}
	fprintf(stderr, "%s: getservbyname: %s: invalid symbolic port\n", progname, pcArg);
	exit(1);
	/* NOTREACHED */ /* codecenter is just WRONG */
}
%%

int variable "iLoadPort" {
	param "labd-port"
	init dynamic 'ParsePort("lablisten", "udp")'
	update '%n = ParsePort(%N, "udp");'
	help "labd clients report load data on this udp port"
}

int variable "iClientPort" {
	param "client-port"
	init dynamic 'ParsePort("labinfo", "tcp")'
	update '%n = ParsePort(%N, "tcp");'
	help "labstat clients connect to this tcp port" 
}

left [ "iLoadPort" ] [ "iClientPort" ] {
}

exit {
	from '"host.h"'
	named "Loadsrv"
	update "%n();"
}

from '"map.h"'
from '"client.h"'
from '"ops.h"'
from '"host.h"'
%c

#ifdef POSIX
#include <limits.h>
#include <unistd.h>
#endif

static void SigFunc(int iSig)
{
	if (! fExec) {
		printf("%s: going down on signal %d...\n", progname, iSig);
	}
	close(fdLoadSock);
	close(fdClientSock);
	exit(1);
}

void Loadsrv()
{
	register CLIENT *pCtemp, *pCnext;
	register int fdMax;
	register int iLen;
	register char *pcAckStr;
	struct timeval timeout;
	auto struct sockaddr_in SINrecv;
	auto struct fd_set FDSread, FDSwrite;
	auto char acLoadBuf[MAXUDPSIZE];
	auto int iJunk;
	auto CLIENT *pChead = (CLIENT *)0;

	if (fDebug) {
		printf("%s: starting up in debug mode, pid %d\n", progname, getpid());
	}

	/* chdir to / to avoid locking any automounted filesystems that
	 * someone was so foolish as to start us from
	 */
	if (-1 == chdir("/")) {
		fprintf(stderr, "%s: chdir /: %s  (you're hosed!)\n", progname, strerror(errno));
		exit(1);
	}

	if ((FILE *)0 != fpExpectFile) {
		ReadExpectList(fpExpectFile);
	}
	ClientInit();

	HostInit();

	FD_ZERO(&FDSread);
	if (!fExec) {
		FD_SET(0, &FDSread);
		FD_SET(1, &FDSread);
		FD_SET(2, &FDSread);
	}
	signal(SIGPIPE, SIG_IGN);
	FD_SET(fdLoadSock, &FDSread);
	FD_SET(fdClientSock, &FDSread);
	Daemon(SigFunc, &FDSread, fExec); /* does some signals for us */
	if (!fExec) {
		OpInit();
	}

	alarm(iDeathWait+1);

	/* set up for first pass
	 */
	timeout.tv_sec = iDeathWait;
	timeout.tv_usec = 0;
#ifdef DYNIX
	fdMax = getdtablesize();
#else
#ifdef POSIX
	fdMax = OPEN_MAX;		/* from <limits.h>	*/
#else
	fdMax = NOFILE;
#endif /* POSIX */
#endif /* DYNIX */

	for (;;) {
		FD_ZERO(&FDSread);
		FD_ZERO(&FDSwrite);
		FD_SET(fdLoadSock, &FDSread);
		FD_SET(fdClientSock, &FDSread);
		if (! fExec)
			FD_SET(fileno(stdin), &FDSread);
		for (pCtemp = pChead; (CLIENT *)0 != pCtemp; pCtemp = pCtemp->next) {
			if (-1 == pCtemp->fdSock) {
				if (!fExec) {
					printf("file desciptor BOTCH (tell ksb or ben) for host\n");
				}
				continue;
			}
			if (0 == pCtemp->iLen)
				FD_SET(pCtemp->fdSock, &FDSread);
			else
				FD_SET(pCtemp->fdSock, &FDSwrite);
		}

		switch (select(fdMax, &FDSread, &FDSwrite, 0, &timeout)) {
		case -1:
			if (EINTR == errno)
				continue;
			if (! fExec) {
				fprintf(stderr, "%s: select: %s\n", progname, strerror(errno));
				exit(1);
			}
			break;

		case 0:
			/* we timed out
			 */
			continue;

		default:
			break;
		}

		if (!fExec && FD_ISSET(fileno(stdin), &FDSread)) {
			ProcessOpCmd();
		}

		/* we have to pull the next point or run down
		 * the dead chain
		 */
		for (pCtemp = pChead; (CLIENT *)0 != pCtemp; pCtemp = pCnext) {
			pCnext = pCtemp->next;
			if (FD_ISSET(pCtemp->fdSock, &FDSread))
				ProcessClientRead(pCtemp);
			else if (FD_ISSET(pCtemp->fdSock, &FDSwrite))
				ProcessClientWrite(pCtemp);
		}

		if (FD_ISSET(fdClientSock, &FDSread)) {
			int fdNewSock;

			iJunk = sizeof(struct sockaddr_in);
			if (-1 != (fdNewSock = accept(fdClientSock, (struct sockaddr *)&SINrecv, &iJunk))) {
				(void) NewClient(&pChead, fdNewSock, SINrecv.sin_addr);
			}
			if (fDebug) {
				printf("%s: received client connection\n", progname);
			}
		}

		if (FD_ISSET(fdLoadSock, &FDSread)) {
			int iFromLen = sizeof(struct sockaddr_in);

			while (-1 != (iLen = recvfrom(fdLoadSock, acLoadBuf, sizeof(acLoadBuf), 0, (struct sockaddr *)&SINrecv, &iFromLen))) {
				if (fDebug) {
					struct hostent *pHEent;

					if ((struct hostent *)0 != (pHEent = gethostbyaddr((char *)&SINrecv.sin_addr.s_addr, sizeof(long), AF_INET)) && (char *)0 != pHEent->h_name) {
						printf("%s: received load message from %s port %d:\n%s", progname, pHEent->h_name, SINrecv.sin_port, acLoadBuf);
					}
				}
				switch (acLoadBuf[0]) {
				case '<':
					/* ugh! syslogd traffic! */
					acLoadBuf[iLen] = '\000';
					ProcessSyslog(SINrecv.sin_addr, acLoadBuf);
					break;
				case '#':
					/* a comment from a superd00d */
					acLoadBuf[iLen] = '\000';
					ProcessComment(SINrecv.sin_addr, acLoadBuf);
					break;
				default:
					pcAckStr = ProcessLoad(SINrecv.sin_addr, SINrecv.sin_port, acLoadBuf);
					/* ack their nice report
					 */
					iLen = strlen(pcAckStr) + 1;
					if (iLen != sendto(fdLoadSock, pcAckStr, iLen, 0, (struct sockaddr *)&SINrecv, sizeof(SINrecv))) {
						if (! fExec) {
							fprintf(stderr, "%s: sendto: %s\n", progname, strerror(errno));
						}
						exit(1);
					}
					break;
				}
#if ! USE_NONBLOCK_RECV
				break;
#endif
			}
			if (-1 == iLen && EWOULDBLOCK != errno && EINTR != errno) {
				fprintf(stderr, "%s: recvfrom: %s\n", progname, strerror(errno));
			}
		}
	}
}
