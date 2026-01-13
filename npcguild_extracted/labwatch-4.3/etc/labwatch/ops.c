/* $Id: ops.c,v 3.16 1997/11/14 22:41:13 ksb Exp $
 */
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
	"$Id: ops.c,v 3.16 1997/11/14 22:41:13 ksb Exp $";


static aiStateTot[ANY_STATE+1];
int iExpected = 0;
 
static void op_STATUS_collect(HOST *pHthis)
{
	++aiStateTot[pHthis->eState];
}

static int op_STATUS(char *pcArgs)
{
	time_t tNow;
	int iAlarmSecs;
	register int i;

	time(&tNow);
	iAlarmSecs = alarm(0);
	for (i = 0; i <= ANY_STATE; ++i) {
		aiStateTot[i] = 0;
	}
	ForeachHostState(op_STATUS_collect, ANY_STATE);
	printf("==================================== %.24s =====\n", ctime(&tNow));
	printf("=  Next death scan in %s\n", DiffStr(iAlarmSecs));
	printf("=  Total %d unseen, %d dead, %d alive, %d tickets available\n", aiStateTot[QUICK], aiStateTot[DEAD], aiStateTot[ALIVE], iHeaped);
	alarm(iAlarmSecs+1);
	return 0;
}

/* output all downtime to log for stat scan (monthly)			(ksb)
 */
static time_t tMark;
static void op_MARK_force(HOST *pHthis)
{
	register time_t tDiff;

	if (DEAD != pHthis->eState) {	/* sanity check only */
		pHthis->tMarked = 0;
		return;
	}
	tDiff = tMark - ((0 != pHthis->tMarked) ? pHthis->tMarked : pHthis->tLastReport);
	printf("MARK: %s, down for %s\n", pHthis->acName, DiffStr(tDiff));
	pHthis->tMarked = tMark;
}

static int op_MARK(char *pcArgs)
{
	register int iAlarmSecs;
	register int i;
	auto struct tm tmNow;
	auto char acStamp[32];	/* like "19640215 16:20:00" */

	iAlarmSecs = alarm(0);
	time(&tMark);
	printf("==================================== %.24s =====\n", ctime(&tMark));
	ForeachHostState(op_MARK_force, DEAD);
	tmNow = *localtime(& tMark);
	(void)strftime(acStamp, sizeof(acStamp), "%Y%m%d %H:%M:%S", &tmNow);
	printf("=== MARK %s ===\n", acStamp);
	fflush(stdout);
	alarm(iAlarmSecs+1);
	return 0;
}

/* sync downline reports for monthly summary				(ksb)
 */
static void SigMark(int iSig)
{
	if (fDebug) {
		printf("%s: received SIGUSR2, MARK time...\n", progname);
	}
	op_MARK((char *)0);
#if !HAVE_STICKY_SIGNALS
	signal(iSig, SigMark);
#endif
}

void op_DEAD_scan(HOST *pHthis)
{
	if (DEAD != pHthis->eState) {
		printf("DEAD: %s last response at %s", pHthis->acName, pHthis->tLastReport ? ctime(&pHthis->tLastReport) : "??\n");
	}
	pHthis->eState = DEAD;
	pHthis->tMarked = 0;
	if (-1 != pHthis->iHeapPos) {
		/* let's not give away dead machines */
		(void) HeapDequeue(pHthis->iHeapPos);
	}
}

static int op_DEAD(char *pcArgs)
{
	time(&tMark);
	ForeachHostStr(op_DEAD_scan, pcArgs);
	return 0;
}

void op_INFO_print_host(HOST *pHthis)
{
	register int i;
	register time_t tNow;

	switch (pHthis->eState) {
	case QUICK:
		printf("%s, not yet reporting\n", pHthis->acName);
		break;
	case DEAD:
	case ALIVE:
		tNow = time((time_t *)0);
		printf("%s, %s %.24s, last seen %s ago\n", pHthis->acName, (DEAD == pHthis->eState) ? "[DEAD] last booted" : "up since", ctime(&pHthis->tBoottime), DiffStr(tNow - pHthis->tLastReport));
		if ('\000' != pHthis->acConsoleUser[0]) {
			printf("  console user: %s, on for %s", pHthis->acConsoleUser, DiffStr(tNow - pHthis->tConsoleLogin));
		}
		printf("  %d unique user%s, %d total\n", pHthis->iUniqUsers, 1 == pHthis->iUniqUsers ? "" : "s", pHthis->iUsers);
		printf("  load average");
		for (i = 0; i < AVEN_LEN; ++i) {
			printf(" %4.2lf", (double)(pHthis->avenrun[i])/XMIT_SCALE);
		}
		printf("\n");
		break;
	case ANY_STATE:	/* -Wall error supressing fire */
		break;
	}
}

static int op_DELETE(char *pcArgs)
{
	register HOST *pHthis;
	register struct hostent *pHEent;
	auto struct in_addr addr;

	if ((char *)0 == pcArgs) {
		printf("please supply the name of a host\n");
		return 0;
	}

	if ((struct hostent *)0 == (pHEent = gethostbyname(pcArgs)) || (char *)0 == pHEent->h_name) {
		fprintf(stderr, "%s: %s: %s\n", progname, pcArgs, hstrerror(h_errno));
		return 0;
	}
#if USE_STRINGS
	bcopy(pHEent->h_addr, &addr, pHEent->h_length);
#else
	memcpy(&addr, pHEent->h_addr, pHEent->h_length);
#endif
	pHthis = LookupHost(addr, 2);
	if ((HOST *)0 == pHthis) {
		printf("%s not managed\n", pcArgs);
	} else {
		switch (pHthis->eState) {
		case ANY_STATE:
		case DEAD:
			break;
		case QUICK:
			if (0 == --iExpected) {
				printf("all expected machines reporting.\n");
			}
			break;
		case ALIVE:
			if (-1 != pHthis->iHeapPos) {
				HeapDequeue(pHthis->iHeapPos);
			}
			break;
		}
		printf("%s deleted\n", pcArgs);
	}
	return 0;
}

/*
 * add another host to the expect list.					(bj)
 * NB:  this is also called by ReadExpectList below.
 * We can take a "name FQDN" or "name IPADDR" form to add
 * hosts that gethostbyname() can't really do forw/rev on. -- ksb
 */
static int op_EXPECT(char *pcArgs)
{
	register HOST *pHthis;
	register struct hostent *pHEent;
	register char *pcFQDN; /* or IP */
	auto struct in_addr addr;

	if ((char *)0 == pcArgs) {
		printf("please supply the name of a host\n");
		return 0;
	}

	if ((char *)0 != (pcFQDN = strchr(pcArgs, ' ')) || (char *)0 != (pcFQDN = strchr(pcArgs, '\t'))) {
		*pcFQDN++ = '\000';
	} else {
		pcFQDN = pcArgs;
	}
	if ((struct hostent *)0 == (pHEent = gethostbyname(pcFQDN)) || (char *)0 == pHEent->h_name) {
		fprintf(stderr, "%s: %s: %s\n", progname, pcFQDN, hstrerror(h_errno));
		return 0;
	}
#if USE_STRINGS
	bcopy(pHEent->h_addr, &addr, pHEent->h_length);
#else
	memcpy(&addr, pHEent->h_addr, pHEent->h_length);
#endif

	pHthis = LookupHost(addr, 1);
	/* add one to the count of expected hosts */
	if (QUICK != pHthis->eState) {
		if (ANY_STATE == pHthis->eState) {
			pHthis->iPort = htons(iDefaultAckPort);
			(void)strncpy(pHthis->acName, pcArgs, sizeof(pHthis->acName));
		} else if (-1 != pHthis->iHeapPos) {
			HeapDequeue(pHthis->iHeapPos);
		}
		++iExpected;
		pHthis->eState = QUICK;
	}

	return 0;
}

static void op_FORCE_doit(HOST *pHthis)
{
	pHthis->fShowMe = 1;
	AckFunc(pHthis);
}

static int op_FORCE(char *pcArgs)
{
	char *pcAck = "!";

	if ('\000' == *pcArgs) {
		printf("please supply the name of a host\n");
		return 0;
	}
	SetupAckFunc("!");
	printf("sending a request...\n");
	ForeachHostStr(op_FORCE_doit, pcArgs);
	return 0;
}

static int op_INFO(char *pcArgs)
{
	struct hostent *pHEent;
	struct in_addr addr;
	HOST *pHthis;

	if ('\000' == *pcArgs) {
		printf("please supply the name of a host\n");
		return 0;
	}
	ForeachHostStr(op_INFO_print_host, pcArgs);
	return 0;
}

static int op_KICK(char *pcArgs)
{
	printf("kicking machines...\n");
	AckeachHostStr("!", pcArgs);
	return 0;
}

static int iCols = 0, iCount = 0;

static void op_STATE_print(HOST *pHthis)
{
	printf("%11s", pHthis->acName);
	++iCount;
	if (++iCols > 6) {
		iCols = 0;
		putchar('\n');
	}
}

static int op_LIST(char *pcArgs)
{
	ForeachHostStr(op_STATE_print, pcArgs);
	printf("%s[%d total]\n", iCols != 0 ? "\n" : "", iCount);
	iCount = iCols = 0;
	return 0;
}

static int op_TICKET(char *pcArgs)
{
	HOST *pHfound;

	if (0 == iHeaped) {
		printf("no tickets available now\n");
		return 0;
	}
	pHfound = HeapPeek();
	printf("top ticket is %s (%d user%s load of %4.2lf)\n", pHfound->acName, pHfound->iUsers, 1 == pHfound->iUsers ? "" : "s", (double)(pHfound->iLoadFac)/iFScale);

	return 0;
}

#if KEEP_STATISTICS

time_t tLastZeroStats;

static int op_ZEROSTATS(char *pcArgs)
{
	printf("resetting statistics.\n");
	iTicketsGranted = iTicketsDenied = iTicketsSuggested = 0;
	time(& tLastZeroStats);
	return 0;
}

static int op_REPORTSTATS(char *pcArgs)
{
	auto time_t tNow;

	time(& tNow);
	printf("As of %.19s, data collected over the last %s\n", ctime(& tNow), DiffStr(tNow - tLastZeroStats));
	printf("   tickets    granted     denied   suggested       total\n");
	printf("   totals    %8d   %8d    %8d    %8d\n", iTicketsGranted, iTicketsDenied, iTicketsSuggested, iTicketsGranted + iTicketsDenied);
	return 0;
}
#endif /* KEEP_STATISTICS */

static int op_VERSION(char *pcArgs)
{
	printf("version %s\n", rcsid);
	return 0;
}

static int op_SHOOT(char *pcArgs)
{
	printf("telling running labd to quit...\n");
	AckeachHostStr("q", pcArgs);
	return 0;
}

static int op_STEAL(char *pcArgs)
{
	op_EXPECT(pcArgs);	/* XXX -ksb */
	printf("redirecting running labd to us...\n");
	AckeachHostStr(">", pcArgs);
	return 0;
}

static int op_QUIT(char *pcArgs)
{
	/* we want the signal handler to do our cleanup */
	kill(getpid(), SIGTERM);
	/* how'd we get here? */
	return 0;
}

static int op_COMMENT(char *pcArgs)
{
	return 0;
}

typedef struct {
	char *pcName;
	int (*func)(char *);
	char *pcHelp;
	char *pcDefaultArgs;
} OP;

static int op_HELP(char *);

static OP aOPs[] = {
	{ "list",	op_LIST,	"list machines", "dead" },
	{ "info",	op_INFO,	"show known info", "" },
	{ "force",	op_FORCE,	"query machine then show info", "dead" },
	{ "finger",	op_FORCE,	NULL, "dead" },
	{ "kick",	op_KICK,	"ask machines for new info", "unseen" },
	{ "add",	op_EXPECT,	"add machines to expect list", "" },
	{ "expect",	op_EXPECT,	NULL, "" },
	{ "delete",	op_DELETE,	"delete all info about machine", "" },
	{ "dead",	op_DEAD,	"force a host to be dead", ""},
	{ "mark",	op_MARK,	"mark dead machines for stats", ""},
	{ "status",	op_STATUS,	"status report", "" },
	{ "stats",	op_STATUS,	NULL, "" },
	{ "top",	op_TICKET,	"show the top ticket", "" },
	{ "ticket",	op_TICKET,	NULL, "" },
	{ "version",	op_VERSION,	"display server version info", "" },
#if KEEP_STATISTICS
	{ "zero",	op_ZEROSTATS,	"zero the current statistics", "" },
	{ "report",	op_REPORTSTATS,	"report the current statistics", "" },
#endif /* KEEP_STATISTICS */
	{ "shoot",	op_SHOOT,	"tell running labd to quit", "all" },
	{ "steal",	op_STEAL,	"redirect running labd to us", "unseen" },
	{ "quit",	op_QUIT,	"quit the server", "" },
	{ "help",	op_HELP,	"this help text", "" },
	{ "?",		op_HELP,	NULL, "" },
	{ "h",		op_HELP,	NULL, "" },
	{ "#",		op_COMMENT,	NULL, "" },
	{ NULL, NULL, NULL }
};

#define DEF_OP	"status"

static int op_HELP(char *pcArgs)
{
	register int i;

	if ('\000' != *pcArgs) {
		printf("sorry, help is not available on specific topics.\n");
		return 0;
	}
	for (i = 0; (char *)0 != aOPs[i].pcName; ++i) {
		if ((char *)0 == aOPs[i].pcHelp)
			continue;
		printf("   %-8s %s\n", aOPs[i].pcName, aOPs[i].pcHelp);
	}
	return 0;
}

void ProcessOpCmd()
{
	auto char acBuf[256];
	register char *pcArgs;
	register OP *pOP;
	register int iLen, i;

	if (0 >= (iLen = read(0, acBuf, sizeof(acBuf)))) {
		clearerr(stdin);
		printf("^D doesn't do anything.\n");
		return;
	}
	while (iLen > 0 && isspace(acBuf[iLen - 1]))
		--iLen;
	acBuf[iLen] = '\000';
	if (0 == iLen) {
		(void)strcpy(acBuf, DEF_OP);
		iLen = strlen(acBuf);
	}
	if (fDebug) {
		printf("%s: received command `%s' on stdin\n", progname, acBuf);
	}

	/* cut off the command after the first space, and remove leading
	 * whitespace from the args
	 */
	pcArgs = acBuf;
	while ('\000' != *pcArgs && !isspace(*pcArgs)) ++pcArgs;
	while(isspace(*pcArgs)) *pcArgs++ = '\000';

	for (i = 0; '\000' != acBuf[i]; ++i) {
		if (!isupper(acBuf[i]))
			continue;
		acBuf[i] = tolower(acBuf[i]);
	}

	for (pOP = (OP *)0, i = 0; (char *)0 != aOPs[i].pcName; ++i) {
		if (0 == strcmp(aOPs[i].pcName, acBuf)) {
			pOP = aOPs + i;
			break;
		}
	}
	if ((OP *)0 == pOP) {
		printf("Command `%s\' not understood.  Try `help\' for help.\n", acBuf);
		return;
	}
	(pOP->func)(('\000' != *pcArgs) ? pcArgs : pOP->pcDefaultArgs);
}

/* print a comment we got from labd, which got it from a superuser	(bj)
 * who used `labd -M "message"'.
 * the text we have is "#user: msg", we format it to "# host: user: msg"
 * and display it on the operator interface.
 */
void ProcessComment(struct in_addr addr, char *pcMsg)
{
	register HOST *pHO;

	if (fExec) {
		/* we're daemonized, what should we do?  syslog this?
		 */
		return;
	}
	++pcMsg;	/* we know it's a # */
	
	if ((HOST *)0 != (pHO = LookupHost(addr, 0))) {
		printf("# %s: %s\n", pHO->acName, pcMsg);
	} else {
		register struct hostent *pHEent;
		register char *pcHostname;
		char acBuf[20];

		if ((struct hostent *)0 == (pHEent = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET)) || (char *)0 == pHEent->h_name) {
			unsigned long lYuck = addr.s_addr;
			sprintf(acBuf, "%d.%d.%d.%d", (lYuck >> 24) & 0xff, (lYuck >> 16) & 0xff, (lYuck >> 8) & 0xff, lYuck & 0xff);
			pcHostname = acBuf;
		} else {
			pcHostname = pHEent->h_name;
		}
		printf("# [unseen] %s: %s\n", pcHostname, pcMsg);
	}
}

/* we read a file like
 *  host1
 *  host2 FQDN
 *  %% map-name
 *  view-data
 *  %% map-name
 *  view ...
 */
void ReadExpectList(FILE *fpRead)
{
	auto char acBuf[256];
	register MAP *pMP = (MAP *)0;

	if (fDebug) {
		printf("%s: reading list of expected hosts...\n", progname);
		fflush(stdout);
	}

	iExpected = 0;
	while (fgets(acBuf, sizeof(acBuf), fpRead)) {
		acBuf[strlen(acBuf) - 1] = '\000';	/* zap \n */
		if ((MAP *)0 == pMP && ('\000' == acBuf[0] || isspace(acBuf[0]) || '#' == acBuf[0]))
			continue;
		if ('%' == acBuf[0] && '%' == acBuf[1]) {
			pMP = MapNew(acBuf+2, 1);
			continue;
		}
		if ((MAP *)0 != pMP) {
			ViewLine(& pMP->MVview, acBuf);
			continue;
		}
		if (fDebug) {
			printf("%s%s", acBuf, 5 == iExpected % 6 ? "\n" : " ");
		}
		op_EXPECT(acBuf);
	}
	fclose(fpRead);
	if (fDebug) {
		putchar('\n');
		fflush(stdout);
	}
}

#if HAVE_WINSIZE
/*
 * this is just a hack, it can be left out				(bj)
 */
static void SigWINCH(int iSig)
{
	static int iArea;
	auto struct winsize size;

	if (-1 == ioctl(0, TIOCGWINSZ, &size)) {
		signal(SIGWINCH, SIG_IGN);
		return;
	}
	if (SIGWINCH == iSig) {
		if (iArea > size.ws_col * size.ws_row) {
			printf("%s: hey, it's getting kinda crowded in here, buddy.\n", progname);
		} else {
			printf("%s: ahhh!  thanks for the extra space.\n", progname);
		}
	}
	iArea = size.ws_col * size.ws_row;
}
#endif /* HAVE_WINSIZE */

static time_t tStart;

void OpInit()
{
	(void)time(& tStart);
#if KEEP_STATISTICS
	tLastZeroStats = tStart;
#endif /* KEEP_STATISTICS */

#if HAVE_WINSIZE
	signal(SIGWINCH, SigWINCH);
	SigWINCH(0);
#endif
	signal(SIGUSR2, SigMark);
	if (! fExec) {
		printf("==================================== %.24s =====\n", ctime(&tStart));
#if KEEP_STATISTICS
		printf("= %s ready, keeping statistics...\n", progname);
#else
		printf("= %s ready...\n", progname);
#endif /* KEEP_STATISTICS */
	}
}
