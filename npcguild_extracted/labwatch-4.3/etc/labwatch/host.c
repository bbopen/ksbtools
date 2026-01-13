/* $Id: host.c,v 3.16 1997/11/14 22:41:13 ksb Exp $
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
#include "daemon.h"
#include "machine.h"
#include "main.h"
#include "host.h"
#include "ops.h"
#include <string.h>
#include <time.h>

#if USE_TERMIOS
#include <termios.h>
#endif

static char rcsid[] =
	"$Id: host.c,v 3.16 1997/11/14 22:41:13 ksb Exp $";


int fdLoadSock = -1;
int iHeapSize = 0;
HOST **appHsubnets[256];

static HOST **ppHheap = (HOST **)0;

/* allocate a new host and initialize it				(bj)
 */
static HOST *NewHost(char *pcName, struct in_addr addr)
{
	static HOST *pHbucketOhosts = (HOST *)0;
	static int iSize = 0;
	static int fFirst = 1;
	static char acMyName[MAXHOSTNAMELEN];
	auto char acTemp[4*4+2];	/* 127.255.255.255 + '\000' */
	register HOST *pHresult;
	register struct hostent *pHEent;

	if (((char *)0 == pcName || '\000' == pcName[0]) && 0 == addr.s_addr) {
		/* must supply either a hostname or address
		 */
		return (HOST *)0;
	}

	if ((HOST *)0 == pHbucketOhosts) {
		iSize = fFirst ? iNumHostsGuess : iNumHostsGuess / 10 + 1;
		pHbucketOhosts = (HOST *) calloc(iSize, sizeof(HOST));
		if (-1 == gethostname(acMyName, sizeof(acMyName))) {
			acMyName[0] = '\000';
		}
		fFirst =0;

		/* there are more hosts, so make the heap bigger
		 */
		iHeapSize += iSize;
		if ((HOST **)0 == ppHheap) {
			ppHheap = (HOST **)malloc(iHeapSize * sizeof(HOST *));
		} else {
			ppHheap = (HOST **)realloc((char *)ppHheap, iHeapSize * sizeof(HOST *));
		}
	}
	pHresult = pHbucketOhosts++;
	if (0 == --iSize) {
		pHbucketOhosts = (HOST *)0;
	}

	if ((char *)0 == pcName || '\000' == pcName[0]) {
		/* get the name from the addr
		 */
		if ((struct hostent *)0 == (pHEent = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET)) || (char *)0 == pHEent->h_name) {
			register unsigned char *puc;
			fprintf(stderr, "%s: %08x: %s\n", progname, addr.s_addr, hstrerror(h_errno));

			puc = (unsigned char *) & addr.s_addr;
			sprintf(acTemp, "%d.%d.%d.%d", puc[0], puc[1], puc[2], puc[3]);
			pcName = acTemp;
		} else {
			pcName = pHEent->h_name;
		}
	} else if (0 == addr.s_addr) {
		/* get the addr from the name
		 */
		if ((struct hostent *)0 == (pHEent = gethostbyname(pcName)) || (char *)0 == pHEent->h_name) {
			fprintf(stderr, "%s: %s: %s\n", progname, pcName, hstrerror(h_errno));
			exit(1);
		}
#if USE_STRINGS
		bcopy(pHEent->h_addr, &addr, pHEent->h_length);
#else
		memcpy(&addr, pHEent->h_addr, pHEent->h_length);
#endif
	}

	strncpy(pHresult->acName, Whittle(pcName, acMyName), sizeof(pHresult->acName));
	pHresult->tLastReport = pHresult->tBoottime = pHresult->tMarked = (time_t)0;
	pHresult->eState = ANY_STATE;
	pHresult->addr = addr;
	pHresult->iHeapPos = -1;

	return pHresult;
}


/* values of fCreate							(bj)
 * 0 -> don't
 * 1 -> please do
 * 2 -> if it is there remove it and give me the corpse (delete it)
 */
HOST *LookupHost(struct in_addr addr, int fCreate)
{
	unsigned long lAddr = ntohl(addr.s_addr);
	unsigned char chSubnet = (lAddr >> 8) & 0xff;
	unsigned char chHost = (lAddr) & 0xff;
	HOST *pHresult = (HOST *)0;

	if ((HOST **)0 == appHsubnets[chSubnet]) {
		if (! fCreate) return (HOST *)0;
		appHsubnets[chSubnet] = (HOST **) calloc(256, sizeof(HOST *));
	}
	if ((HOST *)0 == (pHresult = appHsubnets[chSubnet][chHost])) {
		if (! fCreate) return (HOST *)0;
		pHresult = appHsubnets[chSubnet][chHost] = NewHost(0, addr);
	}
	if (2 == fCreate) {
		appHsubnets[chSubnet][chHost] = (HOST *)0;
	}
	return pHresult;
}

/* Call func for every host in eState (ANY_STATE for all hosts)		(bj)
 */
void ForeachHostState(void (*func)(HOST *), MState eState)
{
	register int iSubnet, iHost;
	register HOST *pHthis;

	for (iSubnet = 0; iSubnet < 256; ++iSubnet) {
		if ((HOST **)0 == appHsubnets[iSubnet])
			continue;
		for (iHost = 0; iHost < 256; ++iHost) {
			if ((HOST *)0 == (pHthis = appHsubnets[iSubnet][iHost]))
				continue;
			if (ANY_STATE != eState && eState != pHthis->eState)
				continue;
			(func)(pHthis);
		}
	}
}

typedef struct {
	char *pcName;
	MState eState;
} KW;

static KW aKWs[] ={
	{ "alive", ALIVE },
	{ "a", ALIVE },
	{ "dead", DEAD },
	{ "d", DEAD },
	{ "quick", QUICK },
	{ "unseen", QUICK },
	{ "u", QUICK },
	{ "all", ANY_STATE },
	{ NULL, 0 }
};

/* values of fCreate							(ksb)
 * 0 -> don't add or delete it
 * 2 -> if it is there remove it and give me the corpse (delete it)
 */
HOST *LookupName(char *pcName, int fCreate)
{
	register int iSubnet, iHost;
	register HOST *pHthis;

	for (iSubnet = 0; iSubnet < 256; ++iSubnet) {
		if ((HOST **)0 == appHsubnets[iSubnet])
			continue;
		for (iHost = 0; iHost < 256; ++iHost) {
			if ((HOST *)0 == (pHthis = appHsubnets[iSubnet][iHost]))
				continue;
			if (0 != strcmp(pcName, pHthis->acName))
				continue;
			if (2 == fCreate)
				appHsubnets[iSubnet][iHost] = (HOST *)0;
			return pHthis;
		}
	}
	return (HOST *)0;
}

/* look for keywords, hostnames, patterns in the string and call	(bj)
 * the func for every set of hosts.  the string is trashed
 */
void ForeachHostStr(void (*func)(HOST *), char *pcList)
{
	register char *pcTemp;
	register KW *pKW;
	register int i;
	register struct hostent *pHEent;
	register HOST *pHthis;
	auto struct in_addr addr;


	/* we assume there is no leading whitespace
	 */
	while ('\000' != *pcList) {
		for (pcTemp = pcList; '\000' != *pcList && !isspace(*pcList); ++pcList) {
			if (isupper(*pcList))
				*pcList = tolower(*pcList);
		}
		if ('\000' != *pcList) {
			*pcList++ = '\000';
		}
		while (isspace(*pcList)) {
			++pcList;
		}

		for (pKW = (KW *)0, i = 0; (char *)0 != aKWs[i].pcName; ++i) {
			if (0 == strcmp(aKWs[i].pcName, pcTemp)) {
				pKW = aKWs + i;
				break;
			}
		}
		if ((KW *)0 != pKW) {
			ForeachHostState(func, pKW->eState);
			continue;
		}

		/* this is expensive, but we have to look here
		 * first --ksb
		 */
		if ((HOST *)0 != (pHthis = LookupName(pcTemp, 0))) {
			(func)(pHthis);
			continue;
		}

		/* XXX  should do partial ip addrs HERE */
		if ((struct hostent *)0 == (pHEent = gethostbyname(pcTemp)) || (char *)0 == pHEent->h_name) {
			printf("%s: %s\n", pcTemp, hstrerror(h_errno));
			continue;
		}
#if USE_STRINGS
		bcopy(pHEent->h_addr, &addr, pHEent->h_length);
#else
		memcpy(&addr, pHEent->h_addr, pHEent->h_length);
#endif
		if ((HOST *)0 == (pHthis = LookupHost(addr, 0))) {
			printf("%s: host not managed\n", pHEent->h_name);
			continue;
		}
		(func)(pHthis);
	}
}

/* UNUSED */
static void XmitHost(void (*func)(HOST *), MState eState)
{
	register int iSubnet, iHost, iMax;
	register HOST *pHthis;
	auto HOST **appHpacked[256];

	for (iMax = iSubnet = 0; iSubnet < 256; ++iSubnet) {
		if ((HOST **)0 == appHsubnets[iSubnet])
			continue;
		appHpacked[iMax++] = appHsubnets[iSubnet];
	}
	for (iHost = 0; iHost < 256; ++iHost) {
		for (iSubnet = 0; iSubnet < iMax; ++iSubnet) {
			if ((HOST *)0 == (pHthis = appHpacked[iSubnet][iHost]))
				continue;
			if (ANY_STATE != eState && eState != pHthis->eState)
				continue;
			(func)(pHthis);
		}
	}
}


static struct sockaddr_in SINack;  /* getting hungry? */
static char *pcAck;
static int iAckLen;

/* set the ack to be sent by AckFunc below				(bj)
 */
void SetupAckFunc(char *pcAckStr)
{
	pcAck = pcAckStr;
	iAckLen = strlen(pcAckStr) + 1;
}

/* Send the ack from SetupAckFunc to the specified host			(bj)
 */
void AckFunc(HOST *pHthis)
{
	if (0 == pHthis->iPort) {
		/* probably added by ExpectFile.  we'll queue this
		 * for later.
		 */
		pHthis->pcAckMesg = pcAck;
		return;
	}
	if (fDebug) {
		printf("%s: signaling %s port %d\n", progname, pHthis->acName, ntohs(pHthis->iPort));
	}
	SINack.sin_addr = pHthis->addr;
	SINack.sin_port = pHthis->iPort;
	if (-1 == sendto(fdLoadSock, pcAck, iAckLen, 0, (struct sockaddr *)&SINack, sizeof(SINack))) {
		fprintf(stderr, "%s: sendto: %s: %s\n", progname, pHthis->acName, strerror(errno));
	}
}

/* send the specified ack to each host in the given state		(bj)
 */
void AckeachHostState(char *pcWith, MState eState)
{
	SetupAckFunc(pcWith);
	ForeachHostState(AckFunc, eState);
}

/* send the specified ack to each host in the list			(bj)
 */
void AckeachHostStr(char *pcWith, char *pcList)
{
	SetupAckFunc(pcWith);
	ForeachHostStr(AckFunc, pcList);
}

static void SigUSR1(int iSig)
{
	/* tell all our followers to run the latest and greatest labd
	 */
	if (fDebug) {
		printf("%s: received SIGUSR1, sending a restart message to my clients.\n", progname);
	}
	AckeachHostState("x", ANY_STATE);
#if !HAVE_STICKY_SIGNALS
	signal(iSig, SigUSR1);
#endif
}

/* SIGHUP tells us to get in sync with the world.  we do it by poking	(bj)
 * all of our labd's and asking for new information.
 */
static void SigHUP(int iSig)
{
	/* tell all our followers to maybe send a report now
	 */
	if (fDebug) {
		printf("%s: received SIGHUP, sending a query message to my clients.\n", progname);
	}
	AckeachHostState("!", ANY_STATE);
#if !HAVE_STICKY_SIGNALS
	signal(iSig, SigHUP);
#endif
}

static time_t tDeathNow;
static int iMaxDiff;

static void DeathCheck(HOST *pHthis)
{
	int iDiff = tDeathNow - pHthis->tLastReport;

	if (iDiff < 0) {
		return;
	}
	if (ALIVE == pHthis->eState && iDiff >= iDeathWait) {
		printf("DEAD: %s last response at %s", pHthis->acName, pHthis->tLastReport ? ctime(&pHthis->tLastReport) : "??\n");
		pHthis->eState = DEAD;
		if (-1 != pHthis->iHeapPos) {
			/* let's not give away dead machines */
			(void) HeapDequeue(pHthis->iHeapPos);
		}
	} else if (iDiff > iMaxDiff) {
		iMaxDiff = iDiff;
	}
}

/* SIGALRM means it's time to look for dead machines			(bj)
 */
static SIGRET_T SigALRM(int iSig)
{
	if (fDebug) {
		printf("%s: initiating death scan\n", progname);
	}
	iMaxDiff = 0;
	time(&tDeathNow);
	ForeachHostState(DeathCheck, ALIVE);
#if !HAVE_STICKY_SIGNALS
	signal(iSig, SigALRM);
#endif
	alarm(iDeathWait - iMaxDiff + 60);
}

int iHeaped = 0;	/* number in heap */

/* insert a host into the heap of hosts to give out for which		(ksb)
 */
static void HeapInsert(pHthis)
HOST  *pHthis;
{
	register int p, np;
	register int iDiff;

	p = ++iHeaped;
	for (/*done*/; p > 1; p = np) {
		np = p/2;
		iDiff = ppHheap[np-1]->iUserFac - pHthis->iUserFac;
		if (0 == iDiff) {
			iDiff = ppHheap[np-1]->iLoadFac - pHthis->iLoadFac;
		}
		if (iDiff <= 0) {
			break;
		}
		ppHheap[p-1] = ppHheap[np-1];
		ppHheap[p-1]->iHeapPos = p;
	}
	ppHheap[p-1] = pHthis;
	pHthis->iHeapPos = p;
}


/* pull a host from the heap, hard to get out clean			(ksb)
 * we have to take one from the end and bubble it up
 * to the top.  N.B.  all our subscripts are off by one
 */
HOST *HeapDequeue(int iN)
{
	register unsigned int p, np;
	register HOST *pHret, *pHrepl;
	register int iDiff;

	if (iHeaped < iN)
		return (HOST *)0;

	pHret = ppHheap[iN-1];
	--iHeaped;
	pHrepl = 0 != iHeaped && iN <= iHeaped ? ppHheap[iHeaped] : (HOST *)0;
	ppHheap[iHeaped] = (HOST *)0;

	for (p = iN; p <= iHeaped; p = np) {
		np = p * 2;
		if (np > iHeaped) {
			break;
		}
		if (np + 1 < iHeaped) {
			iDiff = ppHheap[np-1]->iUserFac - ppHheap[np]->iUserFac;
			if (0 == iDiff) {
				iDiff = ppHheap[np-1]->iLoadFac - ppHheap[np]->iLoadFac;
			}
			if (iDiff > 0) {
				++np;
			}
		}
		ppHheap[p-1] = ppHheap[np-1];
		ppHheap[p-1]->iHeapPos = p;
	}

	for (/*done*/; (HOST *)0 != pHrepl && p > 1; p = np) {
		np = p / 2;
		iDiff = ppHheap[np-1]->iUserFac - pHrepl->iUserFac;
		if (0 == iDiff) {
			iDiff = ppHheap[np-1]->iLoadFac - pHrepl->iLoadFac;
		}
		if (iDiff <= 0) {
			break;
		}
		ppHheap[p-1] = ppHheap[np-1];
		ppHheap[p-1]->iHeapPos = p;
	}
	ppHheap[p-1] = pHrepl;
	if ((HOST *)0 != pHrepl)
		pHrepl->iHeapPos = p;

	pHret->iHeapPos = -1;
	return pHret;
}

/* show us the top of the heap						(ksb)
 */
HOST *HeapPeek(void)
{
	return ppHheap[0];
}

/* the client has a host in mind, can we grant that one?		(ksb)
 * if the host is in the heap and has the same user factor, give it to him
 */
HOST *HeapSuggest(HOST *pHhas, int iTop)
{
	if ((HOST *)0 == pHhas || -1 == pHhas->iHeapPos || ppHheap[0]->iUserFac+1 < pHhas->iUserFac) {
		return HeapDequeue(iTop);
	}
	return HeapDequeue(pHhas->iHeapPos);
}

/* unpack the incoming data into locals					(bj)
 * output changes to the ops,
 * update current status
 * return the string to ACK with
 *
 * note that while we allow for packets with extra fields that we
 * don't recognize, we do assume that all the fields we do know are
 * well formed (and we may blow up if they're not)
 */
char *
ProcessLoad(struct in_addr addr, u_short iPort, char *pcLoadBuf)
{
	static char fSeenKey[256];
	register HOST *pHthis;
	register char *pcTemp;
	register int i, iMissedAcks;
	register char fForSale = 1, fGotBoottime = 0, fGotShutdown = 0;
	register time_t tNewLastReport, tShutdown;
	auto time_t tNewBoottime;

	pHthis = LookupHost(addr, 1);
	pHthis->iPort = iPort;		/* for sending proactive acks */

	/* read the header (the fixed part of the report)
	 */
	if ('-' == *pcLoadBuf) {
		fForSale = 0;
		++pcLoadBuf;
	} else if ('+' == *pcLoadBuf) {
		++pcLoadBuf;
	}
	tNewLastReport = (time_t) strtol(pcLoadBuf, &pcLoadBuf, 16);
	iMissedAcks = strtol(pcLoadBuf + 1, &pcLoadBuf, 16);
	++pcLoadBuf;	/* newline */

	/* erase entries in the host structure that aren't transmitted
	 * if they should be empty
	 */
	pHthis->acConsoleUser[0] = '\000';
	pHthis->tConsoleLogin = (time_t) 0;
	pHthis->acUsers[0] = '\000';

	while ('\000' != *pcLoadBuf) switch(*pcLoadBuf) {
	default:
		/* we don't recognize this field key.  if this is
		 * the first occurance, print a warning to ops.
		 */
		if (0 == fSeenKey[*pcLoadBuf]) {
			if (!fExec) {
				printf("WARNING: unknown key `%c' in load packet from %s.  restart %s?\n", *pcLoadBuf, pHthis, progname);
			}
			fSeenKey[*pcLoadBuf] = 1;
		}
		while ('\n' != *pcLoadBuf++)
			/* empty */ ;
		break;

	case 'b':
		/* boot time in hex seconds since the epoch
		 */
		tNewBoottime = (time_t) strtol(pcLoadBuf + 2, &pcLoadBuf, 16);
		fGotBoottime = 1;
		++pcLoadBuf;	/* newline */
		break;

	case 'd':
		/* shutdown time in hex seconds after tNewLastReport
		 */
		/* ZZZ use these to remove reboot messages */
		tShutdown = (time_t) strtol(pcLoadBuf+2, &pcLoadBuf, 16);
		fGotShutdown = 1;
		++pcLoadBuf;	/* newline */
		break;

	case 'l':
		/* avenrun in hex fixed point (1.00 := XMIT_SCALE)
		 */
		pcLoadBuf += 2;	/* "l:" */
		for (i = 0; i < AVEN_LEN; ++i) {
			pHthis->avenrun[i] = strtol(pcLoadBuf, &pcLoadBuf, 16);
		}
		++pcLoadBuf;	/* newline */
		break;

	case 'c':
		/* console user and login time in hex seconds since the epoch
		 */
		pcLoadBuf += 2;	/* "c:" */
		for (i = 0; i < sizeof(pHthis->acConsoleUser) - 1 && ':' != *pcLoadBuf; ++i, ++pcLoadBuf) {
			pHthis->acConsoleUser[i] = *pcLoadBuf;
		}
		pHthis->acConsoleUser[i] = '\000';
		while (':' != *pcLoadBuf++)
			/* empty */ ;
		pHthis->tConsoleLogin = strtol(pcLoadBuf, &pcLoadBuf, 16);
		++pcLoadBuf;	/* newline */
		break;

	case 'u':
		/* total and unique users in hex
		 */
		pHthis->iUsers = strtol(pcLoadBuf + 2, &pcLoadBuf, 16);
		pHthis->iUniqUsers = strtol(pcLoadBuf + 1, &pcLoadBuf, 16);
		++pcLoadBuf;	/* newline */
		break;

	case 'w':
		/* list of all but console user
		 */
		pcLoadBuf += 2;	/* "w:" */
		for (i = 0; i < sizeof(pHthis->acUsers) - 1 && '\n' != *pcLoadBuf; ++i, ++pcLoadBuf) {
			pHthis->acUsers[i] = *pcLoadBuf;
		}
		pHthis->acUsers[i] = '\000';
		while ('\n' != *pcLoadBuf++)
			/* empty */ ;
		break;

	case 'f':
		/* list of mounted filesystems (with some leading component
		 * stripped off)
		 */
		pcLoadBuf += 2;	/* "f:" */
		for (i = 0; i < sizeof(pHthis->acFSlist) - 1 && '\n' != pcLoadBuf[0]; ++i, ++pcLoadBuf)
			pHthis->acFSlist[i] = pcLoadBuf[0];
		pHthis->acFSlist[i] = '\000';
		++pcLoadBuf;	/* newline */
		break;
	}

	/* update the operators if something interesting
	 * has happened
	 */
	switch (pHthis->eState) {
	case ANY_STATE:
		/* this is what new host entries are initialized to
		 */
		printf("UNEXPECTED: %s\n", pHthis->acName);
		break;
	case QUICK:
		/* this is what op_EXPECT sets the state to.
		 */
		if (0 == --iExpected) {
			printf("all expected machines reporting.\n");
		}
		break;
	default:
		/* if the boot time changed and the machine reported
		 * since then, then adjtime moved it -- else it's a reboot.
		 */
		if (fGotBoottime && 0 != pHthis->tBoottime && tNewBoottime != pHthis->tBoottime && tNewBoottime > pHthis->tLastReport) {
			printf("REBOOTED: %s at %.24s, down for %s\n", pHthis->acName, ctime(& tNewBoottime), (0 == pHthis->tLastReport) ? "??" : DiffStr(time((time_t *)0) - pHthis->tLastReport));
			break;
		}
		if (DEAD == pHthis->eState) {
			register time_t tDiff;
			tDiff = time((time_t *)0) - ((0 != pHthis->tMarked) ? pHthis->tMarked : pHthis->tLastReport);
			printf("ALIVE: %s, down for %s (%d missed report%s)\n", pHthis->acName, DiffStr(tDiff), iMissedAcks, 1 == iMissedAcks ? "" : "s");
		}
	}

	if (fGotBoottime) {
		pHthis->tBoottime = tNewBoottime;
	}
	pHthis->tLastReport = tNewLastReport;
	pHthis->eState = ALIVE;
	pHthis->tMarked = 0;

	/* figure out the "idleness" (or, for cjk, "busyness") factors
	 */
	if ('\000' != pHthis->acConsoleUser[0]) {
		pHthis->iUserFac = iConsoleUserWeight + iTtyUserWeight * (pHthis->iUniqUsers - 1);
	} else {
		pHthis->iUserFac = iTtyUserWeight * pHthis->iUniqUsers;
	}

	pHthis->iLoadFac = 0;
	for (i = 0; i < AVEN_LEN; ++i) {
		pHthis->iLoadFac += aiLoadWeight[i] * pHthis->avenrun[i];
	}

	if (fDebug) {
		printf("%s: %s's user factor is %d, load factor is %d\n", progname, pHthis->acName, pHthis->iUserFac, pHthis->iLoadFac);
	}

	/* put this machine into the heap if it's "For sale" :-)
	 */
	if (fForSale) {
		if (-1 != pHthis->iHeapPos) {
			HeapDequeue(pHthis->iHeapPos);
		}
		HeapInsert(pHthis);
	} else if (-1 != pHthis->iHeapPos) {
		/* their for sale status has changed -- take them out
		 */
		HeapDequeue(pHthis->iHeapPos);
	}

	/* this host was marked interesting since the last time we
	 * saw it, so print out its status
	 */
	if (pHthis->fShowMe) {
		op_INFO_print_host(pHthis);
		pHthis->fShowMe = 0;
	}

	/* if a proactive ack was undeliverable, a clue about it was
	 * left in the HOST structure and we use that ack to reply
	 * this time.
	 */
	pcTemp = (char *)0 == pHthis->pcAckMesg ? "a" : pHthis->pcAckMesg;
	pHthis->pcAckMesg = (char *)0;
	return pcTemp;
}

/* we happen to run on the same port as some old syslog traffic		(bj)
 * that was used to passively monitor the lab.  the main loop
 * recognizes syslog packets and sends them here, where we ignore
 * them (but modularly :-)
 */
void
ProcessSyslog(struct in_addr addr, char *pcLog)
{
	if (fDebug) {
		printf("%s: received syslog: %s\n", progname, pcLog);
	}
}

/* called at startup so we can initialize our corner of the world	(bj)
 */
void HostInit()
{
	auto struct sockaddr_in SINrecv;

	SINack.sin_family = AF_INET;

	/* labd connections
	 */
	if (-1 == (fdLoadSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) {
		fprintf(stderr, "%s: socket: %s\n", progname, strerror(errno));
		exit(1);
	}
#if USE_NONBLOCK_RECV
	if (-1 == fcntl(fdLoadSock, F_SETFL, O_NDELAY)) {
		fprintf(stderr, "%s: fcntl: %s\n", progname, strerror(errno));
		exit(1);
	}
#endif
	SINrecv.sin_family = AF_INET;
	SINrecv.sin_addr.s_addr = htonl(INADDR_ANY);
	SINrecv.sin_port = htons(iLoadPort);
	if (-1 == bind(fdLoadSock, (struct sockaddr *)&SINrecv, sizeof(struct sockaddr_in))) {
		fprintf(stderr, "%s: bind: %s\n", progname, strerror(errno));
		exit(1);
	}

	signal(SIGHUP, SigHUP);
	signal(SIGUSR1, SigUSR1);
	signal(SIGALRM, SigALRM);
}
