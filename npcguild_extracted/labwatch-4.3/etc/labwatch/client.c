#include <sys/types.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "daemon.h"
#include "machine.h"
#include "main.h"
#include "host.h"
#include "map.h"
#include "client.h"
#include "ops.h"

#if USE_TERMIOS
#include <termios.h>
#endif


static char rcsid[] =
	"$Id: client.c,v 3.9 1997/11/14 22:41:13 ksb Exp $";

/* the listener we create when initialized
 */
int fdClientSock = -1;

#if KEEP_STATISTICS
int iTicketsGranted, iTicketsDenied, iTicketsSuggested;
#endif /* KEEP_STATISTICS */

/* list of unused client records, put here when clients disconnect
 */
static CLIENT *pCunused = (CLIENT *)0;

/* format like `printf' and then append the result to the out		(bj)
 * buffer on the client structure.  if it won't fit, return -1 else 0
 */
int Cprintf(CLIENT *pCthis, char *fmt, ...)
{
	char acBuf[sizeof(pCthis->acBuf)];
	int iLen;
	va_list va;

	va_start(va, fmt);
	vsprintf(acBuf, fmt, va);
	va_end(va);
	iLen = strlen(acBuf);
	if (iLen > sizeof(pCthis->acBuf) - pCthis->iLen) {
		errno = EAGAIN;
		return -1;
	}
	strcpy(pCthis->acBuf + pCthis->iLen, acBuf);
	pCthis->iLen += iLen;
	return 0;
}

/* called from the main loop after it accepts a client connection.	(bj)
 */
void NewClient(CLIENT **prev, int fdSock, struct in_addr addr)
{
	static char acHostname[MAXHOSTNAMELEN], acRevision[] = "$Revision: 3.9 $";
	static char *pcVersion;
	static int fSetup = 0;
	CLIENT *next = *prev;
	CLIENT *pCresult;

	if (! fSetup) {
		fSetup = 1;
		gethostname(acHostname, MAXHOSTNAMELEN);
		pcVersion = strchr(acRevision, ' ') + 1;
		*strchr(pcVersion, ' ') = '\000';
	}
	
	if (-1 == fdSock) {
		fprintf(stderr, "%s: fdSock lost before found\n", progname);
		return;
	}
	if (-1 == fcntl(fdSock, F_SETFL, O_NDELAY)) {
		fprintf(stderr, "%s: fcntl: %s\n", progname, strerror(errno));
		return;
	}
	if ((CLIENT *)0 != pCunused) {
		pCresult = pCunused;
		pCunused = pCunused->next;
	} else {
		pCresult = (CLIENT *)malloc(sizeof(CLIENT));
	}
	pCresult->next = next;
	if (next)
		next->prev = &pCresult->next;
	pCresult->fdSock = fdSock;
	pCresult->addr = addr;
	pCresult->tLastActive = time((time_t *)0);
	pCresult->iLen = 0;
	pCresult->fill_func = (void (*)(CLIENT *))0;
	pCresult->prev = prev;
	*prev = pCresult;

	/* jsmith wants a banner message
	 */
	Cprintf(pCresult, "220 %s labwatch %s ready.  Type ? for an idle client.\n", acHostname, pcVersion);
}

void DeleteClient(CLIENT *pCthis)
{
	if (-1 == pCthis->fdSock) { /* this was already deleted, oops! */
		return;
	}
	shutdown(pCthis->fdSock, 2);
	close(pCthis->fdSock);
	pCthis->fdSock = -1;
	*(pCthis->prev) = pCthis->next;
	if ((CLIENT *)0 != pCthis->next) {
		pCthis->next->prev = pCthis->prev;
	}
	pCthis->next = pCunused;
	pCunused = pCthis;
}

static int cmd_HELO(CLIENT *pCthis, char *pcArgs)
{
	if ((char *)0 == pcArgs) {
		Cprintf(pCthis, "250 Hello, nice to meet you.  You're looking for SMTP.\n", pcArgs);
	} else {
		Cprintf(pCthis, "250 Hello %s!  Nice to meet you.  You're looking for SMTP.\n", pcArgs);
	}
	return 0;
}

static int cmd_QUIT(CLIENT *pCthis, char *pcArgs)
{
	Cprintf(pCthis, "221 closing connection\n");
	return 1;
}

static int cmd_PID(CLIENT *pCthis, char *pcArgs)
{
	Cprintf(pCthis, "211 %d\n", getpid());
	return 0;
}

static int cmd_VERSION(CLIENT *pCthis, char *pcArgs)
{
	Cprintf(pCthis, "212 %s version %s\n", progname, rcsid);
	return 0;
}

static int cmd_WIZ(CLIENT *pCthis, char *pcArgs)
{
	/* N.B.  --this is a joke!--
	 */
	Cprintf(pCthis, "330 Ok, here's a root shell!\n# ");
	return 0;
}

static int cmd_XYZZY(CLIENT *pCthis, char *pcArgs)
{
	Cprintf(pCthis, "511 Nothing happens here.\n");
	return 0;
}

/* choose an idle workstation						(bj/ksb)
 * if the client comes from a workstation we monitor, we may
 * return the same machine
 */
static int cmd_WHICH(CLIENT *pCthis, char *pcArgs)
{
	register HOST *pHthem, *pHfound;
	register struct hostent *pHEent;
	auto int so;
	auto struct in_addr addr;
	auto struct sockaddr_in in_port;

        so = sizeof(in_port);
	if ((char *)0 != pcArgs && (struct hostent *)0 != (pHEent = gethostbyname(pcArgs)) && (char *)0 != pHEent->h_name) {
#if USE_STRINGS
		bcopy(pHEent->h_addr, &addr, pHEent->h_length);
#else
		memcpy(&addr, pHEent->h_addr, pHEent->h_length);
#endif
		pHthem = LookupHost(addr, 0);
        } else if (-1 != getpeername(pCthis->fdSock, (struct sockaddr *)&in_port, &so)) {
		/* look at the peer info for a guess?
		 */
		pHthem = LookupHost(in_port.sin_addr, 0);
	} else {
		pHthem = (HOST *)0;
	}
	if ((HOST *)0 == (pHfound = HeapSuggest(pHthem, 1))) {
#if KEEP_STATISTICS
		++iTicketsDenied;
#endif /* KEEP_STATISTICS */
		Cprintf(pCthis, "451 no tickets available now\n");
	} else {
#if KEEP_STATISTICS
		if (pHthem == pHfound) {
			++iTicketsSuggested;
		}
		++iTicketsGranted;
#endif /* KEEP_STATISTICS */
		/* XXX should be 213 XXX */
		Cprintf(pCthis, "230 %s (%d user%s load of %4.2lf)\n", pHfound->acName, pHfound->iUsers, 1 == pHfound->iUsers ? "" : "s", (double)(pHfound->iLoadFac)/iFScale);
	}
	return 0;
}

/* the bogus Xyplex wants us to do echo -- just tell them when they	(ksb)
 * want to know and get the heck out...
 */
static int cmd_Xyplex(CLIENT *pCthis, char *pcArgs)
{
	register HOST *pHfound;

	if ((HOST *)0 == (pHfound = HeapDequeue(1))) {
#if KEEP_STATISTICS
		++iTicketsDenied;
#endif /* KEEP_STATISTICS */
		Cprintf(pCthis, "\n\rNo tickets available now.  Try again in a few minutes, please.\n\r");
	} else {
#if KEEP_STATISTICS
		++iTicketsGranted;
#endif /* KEEP_STATISTICS */
		Cprintf(pCthis, "\n\rPlease type \"telnet %s\" (it has %d user%s and a load of %4.2lf)\n\r", pHfound->acName, pHfound->iUsers, 1 == pHfound->iUsers ? "" : "s", (double)(pHfound->iLoadFac)/iFScale);
	}
	return 1;
}

#if KEEP_STATISTICS
static int cmd_STAT(CLIENT *pCthis, char *pcArgs)
{
	Cprintf(pCthis, "219 stats %x:%x:%x:%x\n", tLastZeroStats, iTicketsGranted, iTicketsDenied, iTicketsSuggested);
	return 0;
}
#endif /* KEEP_STATISTICS */

static void cmd_MAP_tailer(CLIENT *pCthis)
{
	if (-1 != Cprintf(pCthis, "%d done\n", 225, iHeaped)) {
		pCthis->fill_func = (void (*)(CLIENT *))0;
	}
}

static void cmd_MAP_pump(CLIENT *pCthis)
{
	while (pCthis->u.map.inext < pCthis->u.map.pMP->MVview.icur && -1 != Cprintf(pCthis, "%s\n", pCthis->u.map.pMP->MVview.ppclines[pCthis->u.map.inext])) {
		pCthis->u.map.inext++;
	}
	if (pCthis->u.map.inext >= pCthis->u.map.pMP->MVview.icur) {
		pCthis->fill_func = cmd_MAP_tailer;
	}
}

static void cmd_MAP_names(CLIENT *pCthis)
{
	while ((MAP *)0 != pCthis->u.map.pMP) {
		if ((char *)0 != pCthis->u.map.pMP->pctext) {
			if (-1 == Cprintf(pCthis, "%-*s %s\n", iMapNameLens, pCthis->u.map.pMP->pcname, pCthis->u.map.pMP->pctext))
				break;
		} else if (-1 == Cprintf(pCthis, "%s\n", pCthis->u.map.pMP->pcname)) {
			break;
		}
		pCthis->u.map.pMP = pCthis->u.map.pMP->pMPnext;
	}
	if ((MAP *)0 == pCthis->u.map.pMP) {
		pCthis->fill_func = cmd_MAP_tailer;
	}
}

static int cmd_MAP(CLIENT *pCthis, char *pcArgs)
{
	pCthis->u.map.inext = 0;
	if ((MAP *)0 == pMPList) {
		Cprintf(pCthis, "%d no maps configured\n", 325);
		return 0;
	}

	if ((char *)0 != pcArgs) {
		pCthis->u.map.pMP = MapNew(pcArgs, 0);
		if ((MAP *)0 == pCthis->u.map.pMP) {
			Cprintf(pCthis, "%d no map %s\n", 324, pcArgs);
			return 0;
		}
		pCthis->fill_func = cmd_MAP_pump;
		Cprintf(pCthis, "%d- %s\n", 225, ((char *)0 != pCthis->u.map.pMP->pctext) ? pCthis->u.map.pMP->pctext : pCthis->u.map.pMP->pcname);
	} else {
		pCthis->u.map.pMP = pMPList;
		pCthis->fill_func = cmd_MAP_names;
		Cprintf(pCthis, "%d- map list\n", 225);
	}
	(pCthis->fill_func)(pCthis);
	return 0;
}

static void cmd_RAW_tailer(CLIENT *pCthis)
{
	if (-1 != Cprintf(pCthis, "%d %d tickets\n", pCthis->u.raw.iflag + 215, iHeaped)) {
		pCthis->fill_func = (void (*)(CLIENT *))0;
	}
}

static int cmd_RAW_print_host(CLIENT *pCthis, HOST *pHthis)
{
	if (2 == pCthis->u.raw.iflag) {	 /* kid output */
		register unsigned char *puc;
		puc = (unsigned char *)&pHthis->addr;
		return Cprintf(pCthis, " %s:%x:%02x%02x%02x%02x:%x\n", pHthis->acName, pHthis->tLastReport, puc[0], puc[1], puc[2], puc[3], pHthis->iPort);
	}
	if (1 == pCthis->u.raw.iflag) {  /* 'fVerbose' */
		return Cprintf(pCthis, " %s:%x:%x:%x,%x,%x:%x:%s %s:%d:%s\n", pHthis->acName, pHthis->tBoottime, pHthis->tLastReport, pHthis->avenrun[0], pHthis->avenrun[1], pHthis->avenrun[2], pHthis->tConsoleLogin, pHthis->acConsoleUser, pHthis->acUsers, pHthis->iUniqUsers, pHthis->acFSlist);
	}
	return Cprintf(pCthis, " %s:%x,%x,%x:%x:%s %s:%s\n", pHthis->acName, pHthis->avenrun[0], pHthis->avenrun[1], pHthis->avenrun[2], pHthis->tConsoleLogin, pHthis->acConsoleUser, pHthis->acUsers, pHthis->acFSlist);
}

static void cmd_RAW_fill_func(CLIENT *pCthis)
{
	register int iSubnet = pCthis->u.raw.isubnet, iHost = pCthis->u.raw.ihost;

	if (256 != iHost)
		goto YUCK;

	for (; iSubnet < 256; ++iSubnet) {
		if ((HOST **)0 == appHsubnets[iSubnet])
			continue;
		iHost = 0;
		YUCK:
		for (; iHost < 256; ++iHost) {
			if ((HOST *)0 == appHsubnets[iSubnet][iHost]) {
				continue;
			}
			if (-1 == cmd_RAW_print_host(pCthis, appHsubnets[iSubnet][iHost])) {
				pCthis->u.raw.isubnet = iSubnet;
				pCthis->u.raw.ihost = iHost;
				return;
			}
		}
	}
	pCthis->fill_func = cmd_RAW_tailer;
}

static int cmd_RAW(CLIENT *pCthis, char *pcArgs, int fLongForm)
{
	register struct hostent *pHEent;
	register HOST *pHthis;
	auto struct in_addr addr;

	pCthis->u.raw.iflag = fLongForm;
	if ((char *)0 == pcArgs) {
		Cprintf(pCthis, "%d-%s dump.  [not more than %d hosts now]\n", pCthis->u.raw.iflag + 215, pCthis->u.raw.iflag ? "long" : "regular", iHeapSize);
		pCthis->fill_func = cmd_RAW_fill_func;
		pCthis->u.raw.isubnet = 0;
		pCthis->u.raw.ihost = 256;
		cmd_RAW_fill_func(pCthis);
	} else {
		if ((struct hostent *)0 == (pHEent = gethostbyname(pcArgs)) || (char *)0 == pHEent->h_name) {
			Cprintf(pCthis, "550 %s: %s\n", pcArgs, hstrerror(h_errno));
			return 0;
		}
#if USE_STRINGS
		bcopy(pHEent->h_addr, &addr, pHEent->h_length);
#else
		memcpy(&addr, pHEent->h_addr, pHEent->h_length);
#endif
		if ((HOST *)0 == (pHthis = LookupHost(addr, 0))) {
			Cprintf(pCthis, "504 host not managed\n");
			return 0;
		}
		Cprintf(pCthis, "%d", pCthis->u.raw.iflag + 215);
		cmd_RAW_print_host(pCthis, pHthis);
	}
	return 0;
}

static int cmd_DUMP(CLIENT *pCthis, char *pcArgs)
{
	return cmd_RAW(pCthis, pcArgs, 0);
}

static int cmd_LONG(CLIENT *pCthis, char *pcArgs)
{
	return cmd_RAW(pCthis, pcArgs, 1);
}

static int cmd_KIDS(CLIENT *pCthis, char *pcArgs)
{
	return cmd_RAW(pCthis, pcArgs, 2);
}

static int cmd_AUTHORS(CLIENT *pCthis, char *pcArgs)
{
	Cprintf(pCthis, "218 %s written by Ben Jackson and Kevin Braunsdorf\n", progname);
	return 0;
}

typedef struct {
	char *pcName;
	int (*func)(CLIENT *, char *);
	char *pcHelp;
} CMD;

static int cmd_HELP(CLIENT *, char *);

/* command name, function to call, help str (NULL = don't list in help)
 */
CMD aCMDs[] = {
	{ "which",	cmd_WHICH,	"what machine is most idle" },
	{ "?",		cmd_Xyplex,	"what machine and disconnect" },
	{ "?\015",	cmd_Xyplex,	NULL },
	{ "raw",	cmd_DUMP,	NULL },
	{ "dump",	cmd_DUMP,	"dump the database" },
	{ "long",	cmd_LONG,	"dump even more of the database" },
#if KEEP_STATISTICS
	{ "stat",	cmd_STAT,	"see some statistics" },
	{ "stats",	cmd_STAT,	NULL },
#endif /* KEEP_STATISTICS */
	{ "helo",	cmd_HELO,	"say hello ala SMTP" },
	{ "help",	cmd_HELP,	"this help message" },
	{ "kid",	cmd_KIDS,	"list the client labd contact info"},
	{ "kids",	cmd_KIDS,	NULL },
	{ "map",	cmd_MAP,	"provide position information" },
	{ "pid",	cmd_PID,	"the server's PID" },
	{ "wiz",	cmd_WIZ,	NULL },
	{ "xyzzy",	cmd_XYZZY,	NULL },
	{ "version",	cmd_VERSION,	"show the server version" },
	{ "authors",	cmd_AUTHORS,	"show the list of authors" },
	{ "quit",	cmd_QUIT,	"close connection" },
	{ "bye",	cmd_QUIT,	NULL },
	{ "exit",	cmd_QUIT,	NULL },
	{ "\015",	cmd_Xyplex,	NULL },	/* Xyplex does telnet neg. */
	{ NULL, NULL, NULL },
};

static int cmd_HELP(CLIENT *pCthis, char *pcArgs)
{
	register CMD *pCMD;
	register int i;

	Cprintf(pCthis, "214-The following commands are recognized:\n");
	for (pCMD = 0, i = 0; (char *)0 != aCMDs[i].pcName; ++i)
		if ((char *)0 != aCMDs[i].pcHelp)
			Cprintf(pCthis, "   %-8s %s\n", aCMDs[i].pcName, aCMDs[i].pcHelp);
	Cprintf(pCthis, "214 Direct comments to bj@cc.purdue.edu\n");

	return 0;
}

void ProcessClientWrite(CLIENT *pCthis)
{
	register int iLen;

	if (0 >= (iLen = write(pCthis->fdSock, pCthis->acBuf, pCthis->iLen))) {
		DeleteClient(pCthis);
		return;
	}
	pCthis->tLastActive = time((time_t *)0);
	pCthis->iLen -= iLen;
#if USE_STRINGS
	bcopy(pCthis->acBuf + iLen, pCthis->acBuf, pCthis->iLen);
#else
	memcpy(pCthis->acBuf, pCthis->acBuf + iLen, pCthis->iLen);
#endif
	if ((void (*)(CLIENT *))0 != pCthis->fill_func)
		(pCthis->fill_func)(pCthis);
}

void ProcessClientRead(CLIENT *pCthis)
{
	register char *pcArgs;
	register CMD *pCMD;
	register int iLen, i;
	auto char acBuf[256];

	if (0 >= (iLen = read(pCthis->fdSock, acBuf, sizeof(acBuf)))) {
		DeleteClient(pCthis);
		return;
	}
	if (!isprint(acBuf[0])) {
		write(pCthis->fdSock, acBuf, iLen);
		return;
	}
	pCthis->tLastActive = time((time_t *)0);
	while (iLen > 0 && isspace(acBuf[iLen - 1]))
		--iLen;
	acBuf[iLen] = '\000';
	if (fDebug) {
		printf("%s: received command `%s' on client socket\n", progname, acBuf);
	}

	pcArgs = strchr(acBuf, ' ');
	if ((char *)0 != pcArgs) {
		pcArgs[0] = '\000';
		++pcArgs;
	}
	for (i = 0; '\000' != acBuf[i]; ++i)
		acBuf[i] = tolower(acBuf[i]);
	if (0 == iLen) {
		/* blank line means "do the default thing"
		 */
		pCMD = aCMDs;
	} else {
		for (pCMD = 0, i = 0; (char *)0 != aCMDs[i].pcName; ++i) {
			if (0 == strcmp(aCMDs[i].pcName, acBuf)) {
				pCMD = aCMDs + i;
				break;
			}
		}
	}

	if ((CMD *)0 == pCMD) {
#if 1
		if (!isprint(acBuf[0])) {
			Cprintf(pCthis, "500 Non-printing junk (");
			for (i = 0; '\000' != acBuf[i]; ++i) {
				Cprintf(pCthis, "\\%03o", 0xff & acBuf[i]);
			}
			Cprintf(pCthis, ")\n");
		}
#endif
		if (-1 == Cprintf(pCthis, "500 Command %s not recognized.  Try HELP for help.\n", acBuf)) {
			DeleteClient(pCthis);
		}
		return;
	}
	if ((int (*)(CLIENT *, char *))0 == pCMD->func) {
		/* nothing */;
	} else if ((pCMD->func)(pCthis, pcArgs)) {
		ProcessClientWrite(pCthis); /* try to flush */
		DeleteClient(pCthis);
	}
}

void ClientInit()
{
	auto struct sockaddr_in SINrecv;

	/* client connections
	 */
	if (-1 == (fdClientSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))) {
		fprintf(stderr, "%s: socket: %s\n", progname, strerror(errno));
		exit(1);
	}

	SINrecv.sin_family = AF_INET;
	SINrecv.sin_addr.s_addr = htonl(INADDR_ANY);
	SINrecv.sin_port = htons(iClientPort);
	if (-1 == bind(fdClientSock, (struct sockaddr *)&SINrecv, sizeof(struct sockaddr_in))) {
		fprintf(stderr, "%s: bind: %s\n", progname, strerror(errno));
		exit(1);
	}

	(void)listen(fdClientSock, LISTENBACKLOG);
}
