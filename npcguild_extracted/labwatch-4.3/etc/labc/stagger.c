/* $Id: stagger.c,v 2.1 1994/06/07 11:26:52 ksb Two $
 * contact the TCP port of labwatch for block data
 */
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <signal.h>
#include <netdb.h>
#include <pwd.h>
#include <errno.h>

#include "machine.h"
#include "main.h"

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#if USE_STDLIB
#include <stdlib.h>
#else
extern char *realloc(), *calloc(), *malloc();
#endif

/* panic -- we have no more memory					(ksb)
 */
static void
OutOfMem()
{
	static char acNoMem[] = ": out of memory\n";

	write(2, progname, strlen(progname));
	write(2, acNoMem, sizeof(acNoMem)-1);
	exit(1);
}


/* remove from "host1" those domains common to "host1" and "host2"	(dls)
 */
static char *
whittle(host1, host2)
char *host1, *host2;
{
	char *p1, *p2;

	p1 = strchr(host1, '.');
	p2 = strchr(host2, '.');
	while (p1 != (char*)0 && p2 != (char*)0) {
		if (strcmp(p1+1, p2+1) == 0) {
			*p1 = '\000';
			break;
		}
		p1 = strchr(p1+1, '.');
		p2 = strchr(p2+1, '.');
	}
	return host1;
}

static char
	acLocalhost[] =		/* the loopback device			*/
		"localhost",
	acThisHost[256],	/* what the remote host would call us	*/
	acMyName[256];		/* what we call ourselves		*/

static struct sockaddr_in
	local_port;		/* the looback address, if local use it	*/


/* send out some data along the connection				(ksb)
 */
static void
SendOut(fd, pcBuf, iLen)
int fd, iLen;
char *pcBuf;
{
	register int nr;

	while (0 != iLen) {
		if (-1 == (nr = write(fd, pcBuf, iLen))) {
			fprintf(stderr, "%s: lost connection\n", progname);
			exit(3);
		}
		iLen -= nr;
		pcBuf += nr;
	}
}


/* read a reply from the load server					(ksb)
 * if pcWnat == (char *)0 we strip \r\n from the and and return strlen
 */
static char *
ReadReply(fd, pcBuf, iLen, iWant)
int fd, iLen;
char *pcBuf;
int iWant;
{
	register int nr, j, iKeep;

	iKeep = iLen;
	for (j = 0; j < iLen; /* j+=nr */) {
		switch (nr = read(fd, &pcBuf[j], iLen-1)) {
		case 0:
			if (iKeep != iLen) {
				break;
			}
			/* fall through */
		case -1:
			fprintf(stderr, "%s: lost connection\n", progname);
			exit(3);
		default:
			j += nr;
			iLen -= nr;
			if ('\n' == pcBuf[j-1]) {
				pcBuf[j] = '\000';
				break;
			}
			if (0 == iLen) {
				fprintf(stderr, "%s: reply too long\n", progname);
				exit(3);
			}
			continue;
		}
		break;
	}
	/* in this case the called wants a line of text
	 * remove the cr/lf sequence and any trailing spaces
	 * (s/[ \t\r\n]*$//)
	 */
	if (0 == iWant) {
		while (0 != j && isspace(pcBuf[j-1])) {
			pcBuf[--j] = '\000';
		}
	} else if (iWant != atoi(pcBuf)) {
		fprintf(stderr, "%s: out of sync (%d != %.10s)\n", progname, iWant, pcBuf);
		exit(2);
	} else {	/* skip key number we checked */
		while (isdigit(*pcBuf))
			++pcBuf;
		while (isspace(*pcBuf))
			++pcBuf;
	}
	return pcBuf;
}

#define BUF_START	8192
#define BUF_CHUNK	2048
#define BUF_MIN		1400

/* read in the lump bewteen 214-... and 214 ...				(ksb)
 * iMax+1 for the \000 on the end
 */
static char *
GetRawData(fd, piMark, piLast)
int fd;
int *piMark, *piLast;
{
	register int iRem, iMax, i, nr, iCursor;
	register char *pcBuf, *pcTemp;
	auto char *pcSpace;
	auto int iDeadMark;

	if ((int *)0 == piMark) {
		piMark = & iDeadMark;
	}

	iMax = BUF_START;
	if ((char *)0 == (pcBuf = malloc(iMax+1))) {
		OutOfMem();
	}

        iRem = iMax;
        iCursor = i = 0;
        while (0 < (nr = read(fd, pcBuf+i, iRem))) {
                i += nr;
		pcBuf[i] = '\000';
		while ((char *)0 != (pcTemp = strchr(pcBuf+iCursor, '\n'))) {
			if (!isdigit(pcBuf[iCursor])) {
				iCursor = pcTemp-pcBuf+1;
				continue;
			}
			*piMark = strtol(pcBuf+iCursor, & pcSpace, 0);
			if (' ' != *pcSpace) {
				iCursor = pcTemp-pcBuf+1;
				continue;
			}
			if ((int *)0 != piLast) {
				*piLast = iCursor;
			}
			return pcBuf;
		}
                iRem -= nr;
                if (iRem >= BUF_MIN) {
                        continue;
                }
                iMax += BUF_CHUNK;
                if ((char *)0 == (pcBuf = realloc(pcBuf, iMax+1))) {
                        OutOfMem();
                }
                iRem += BUF_CHUNK;
        }

	return (char *)0;
}

/* poll the running loadsrv for block info			    (ksb&ben)
 *	GetBlcokData(apc, fdServer, "long\n", 216);
 * N.B. apc points to 3 character pointers in memory (head, block, tail)
 */
static int
GetBlockData(ppcOut, fd, pcCmd, piMark)
char **ppcOut;
int fd;
char *pcCmd;
int *piMark;
{
	register char *pcText, *pcBlock, *pcFoot;
	auto int iLast, iMark;

	SendOut(fd, pcCmd, strlen(pcCmd));
	pcText = GetRawData(fd, & iMark, & iLast);
	if ((char *)0 == pcText) {
		return 0;
	}

	/* munch the `216 ' off the footer
	 */
	*piMark = atoi(pcText+iLast);
	while (isdigit(pcText[iLast])) {
		pcText[iLast++] = '\000';
	}
	pcFoot = pcText + iLast+1;

	/* this finds the head, inval. iLast
	 */
	while (isdigit(*pcText)) {
		*pcText++ = '\000';
	}
	++pcText;

	if ((char *)0 != (pcBlock = strchr(pcText, '\n'))) {
		*pcBlock++ = '\000';
	}

	ppcOut[0] = pcText;
	ppcOut[1] = pcBlock;
	ppcOut[2] = pcFoot;

	return 1;
}

/* set the port for socket connection					(ksb)
 * return the fd for the new connection; if we can use the loopback, do
 * as a side effect we set ThisHost to a short name for this host.
 * host:port works							(bj)
 */
static int
GetTcpPort(pPort, pcToHost, sPort)
struct sockaddr_in *pPort;
char *pcToHost;
short sPort;
{
	register int fd;
	register char *pcTemp;
	register struct hostent *hp;
	auto char acIntro[256];

	if (0 == strcmp(pcToHost, strcpy(acThisHost, acMyName))) {
		(void)strcpy(pcToHost, acLocalhost);
#if USE_STRINGS
		(void)bcopy((char *)&local_port.sin_addr, (char *)&pPort->sin_addr, sizeof(local_port.sin_addr));
#else
		memcpy((char *)&pPort->sin_addr, (char *)&local_port.sin_addr, sizeof(local_port.sin_addr));
#endif
	} else if ((struct hostent *)0 != (hp = gethostbyname(pcToHost))) {
#if USE_STRINGS
		(void)bcopy((char *)hp->h_addr, (char *)&pPort->sin_addr, hp->h_length);
#else
		memcpy((char *)&pPort->sin_addr, (char *)hp->h_addr, hp->h_length);
#endif
	} else {
		fprintf(stderr, "%s: gethostbyname: %s: %s\n", progname, pcToHost, hstrerror(h_errno));
		return -1;
	}
	pPort->sin_port = htons(sPort);
	pPort->sin_family = AF_INET;

	/* make hostname short, if we are calling ourself, chop at first dot
	 */
	if (0 == strcmp(pcToHost, acLocalhost)) {
		register char *pcChop;
		if ((char *)0 != (pcChop = strchr(acThisHost, '.'))) {
			*pcChop = '\000';
		}
	} else {
		(void)whittle(acThisHost, pcToHost);
	}

	/* set up the socket to talk to the server for all load data
	 * (it will tell us who to talk to to get a real connection)
	 */
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "%s: socket: %s\n", progname, strerror(errno));
		exit(1);
	}
	if (connect(fd, (struct sockaddr *)pPort, sizeof(*pPort)) < 0) {
		fprintf(stderr, "%s: connect: %s:%d: %s\n", progname, pcToHost, ntohs(pPort->sin_port), strerror(errno));
		return -1;
	}

	(void)ReadReply(fd, acIntro, sizeof(acIntro), 220);
	return fd;
}

typedef struct HInode {		/* information we know about a client */
	char *pchost;
	char *pclastping;
	char *pcport;
	char *pcipaddr;
	/* computed */
	long lseg;
	long tping;
} HOSTINFO;

/* order the hosts for binary serach					(ksb)
 */
int
CompHost(ppHILeft, ppHIRight)
HOSTINFO **ppHILeft, **ppHIRight;
{
	return strcmp(ppHILeft[0]->pchost, ppHIRight[0]->pchost);
}

typedef struct SInode {		/* labwatch servers we have spoken to	*/
	char *pcname;
	int ipid;
	struct sockaddr_in client_port;
	int s;
	HOSTINFO **ppHIsort;
	int ihosts;
	struct SInode *pSInext;
} SERVER;

static SERVER *pSIList;

/* find the loadsrv daemon out on the net, if it is on our		(ksb)
 * machine we must look it up by the hostname
 */
static SERVER *
FindServer(pcName, sPort)
char *pcName;
short sPort;
{
	register SERVER *pSIScan, **ppSI, *pSINew;
	register char *pcTemp;
	register int iCmp;
	register struct hostent *hp;

	if (0 == strcmp(acLocalhost, pcName)) {
		pcName = acMyName;
	}
	if ((struct hostent *)0 == (hp = gethostbyname(pcName))) {
		fprintf(stderr, "%s: gethostbyname: %s: %s\n", progname, pcName, hstrerror(h_errno));
		return (SERVER *)0;
	}

	for (ppSI = & pSIList; (SERVER *)0 != (pSIScan = *ppSI); ppSI = & pSIScan->pSInext) {
		if (0 == (iCmp = strcmp(pSIScan->pcname, hp->h_name))) {
			return pSIScan;
		}
		if (iCmp > 0)
			break;
	}

	if ((SERVER *)0 == (pSINew = (SERVER *)malloc(sizeof(SERVER)))) {
		OutOfMem();
	}
	pSINew->pcname = strdup(hp->h_name);
	pSINew->ipid = 0;
	pSINew->s = GetTcpPort(& pSINew->client_port, pSINew->pcname, sPort);
	if (-1 == pSINew->s) {
		free(pSINew->pcname);
		free((char *)pSINew);
		return (SERVER *)0;
	}
	pSINew->ppHIsort = (HOSTINFO **)0;
	pSINew->ihosts = 0;
	pSINew->pSInext = pSIScan;
	*ppSI = pSINew;

	return pSINew;
}

/* close all the open server connections				(ksb)
 */
static void
CloseServers()
{
	register SERVER *pSI;
	auto char acClose[256+2];

	for (pSI = pSIList; (SERVER *)0 != pSI; pSI = pSI->pSInext) {
		if (-1 == pSI->s)
			continue;
		SendOut(pSI->s, "quit\n", 5);
		ReadReply(pSI->s, acClose, sizeof(acClose), 221);
		close(pSI->s);
		pSI->s = -1;
	}
}

/* call GetBlockData to get the host info in long format		(ksb)
 */
static void
GetPingData(pSIFrom)
SERVER *pSIFrom;
{
	auto char *apcChunk[3];
	register char *pcText, *pcBlock;
	register HOSTINFO *pHIData, **ppHISort;
	register int iMax, i;
	auto int iMark;

	if ((HOSTINFO **)0 != pSIFrom->ppHIsort) {
		return;
	}

	if (0 == GetBlockData(apcChunk, pSIFrom->s, "kids\n", &iMark)) {
		exit(1);
	}
	if (217 != iMark) {
		fprintf(stderr, "%s: out of sync (%d != %d)\n", progname, 216, iMark);
		exit(2);
	}

	/* pcText is header, pcBlock is the data
	 *  header: `long dump.  [not more than 200 hosts now]'
	 *   block: ` fe___:ping:port:ip\n fe___:....\n'
	 *  footer: `177 tickets\n'		[ignored]
	 */
	pcText = apcChunk[0];
	pcBlock = apcChunk[1];

	while ('\000' != *pcText && ! isdigit(*pcText)) {
		++pcText;
	}
	iMax = atoi(pcText);

	if ((HOSTINFO *)0 == (pHIData = (HOSTINFO *)calloc(iMax, sizeof(HOSTINFO)))) {
		OutOfMem();
	}
	if ((HOSTINFO **)0 == (ppHISort = (HOSTINFO **)calloc(iMax+1, sizeof(HOSTINFO *)))) {
		OutOfMem();
	}

	/* we saw the header, we checked the footer, and the space
	 * if the colons are broken we dump core, and rightfully so.
	 */
	i = 0;
	while ('\000' != *pcBlock) {
		if (' ' != *pcBlock++) {
			fprintf(stderr, "%s: out of sync in host data table\n", progname);
			exit(2);
		}
		pHIData[i].pchost = pcBlock;
		pcBlock = strchr(pcBlock, ':');
		*pcBlock++ = '\000';
		pHIData[i].pclastping = pcBlock;
		pcBlock = strchr(pcBlock, ':');
		*pcBlock++ = '\000';
		pHIData[i].pcipaddr = pcBlock;
		pcBlock = strchr(pcBlock, ':');
		*pcBlock++ = '\000';
		pHIData[i].pcport = pcBlock;
		pcBlock = strchr(pcBlock, '\n');
		*pcBlock++ = '\000';

		pHIData[i].lseg = 0;
		ppHISort[i] = & pHIData[i];
		++i;
	}
	qsort(ppHISort, i, sizeof(HOSTINFO *), CompHost);

	pSIFrom->ppHIsort = ppHISort;
	pSIFrom->ihosts = i;
}

/* look to see if this server knows anything about this host		(ksb)
 * N.B. we use the `long' output for data
 */
static HOSTINFO *
FindHost(pSIIn, pcHost)
SERVER *pSIIn;
char *pcHost;
{
	register int iLo, iHi, iMid, iCmp;
	register HOSTINFO **ppHISort;

	if ((HOSTINFO **)0 == (ppHISort = pSIIn->ppHIsort)) {
		GetPingData(pSIIn);
		if ((HOSTINFO **)0 == (ppHISort = pSIIn->ppHIsort)) {
			fprintf(stderr, "%s: server %s failed hosts data collection\n", progname, pSIIn->pcname);
			return (HOSTINFO *)0;
		}
	}

	if (0 == (iHi = pSIIn->ihosts)) {
		return (HOSTINFO *)0;
	}
	if (0 == strcmp(pcHost, ppHISort[0]->pchost)) {
		return ppHISort[0];
	}
	iLo = 0;
	iMid = --iHi;
	do {
		if (0 == (iCmp = strcmp(pcHost, ppHISort[iMid]->pchost))) {
			return ppHISort[iMid];
		}
		if (iCmp < 0) {
			iHi = iMid;
		} else {
			iLo = iMid;
		}
		iMid = (iLo + iHi + 1) / 2;
	} while (iLo+1 < iHi);

	return (HOSTINFO *)0;
}


/* mainline for load client program					(ksb)
 * setup who we are, and what our loopback addr is
 * get a shutdown pid
 */
/*ARGSUSED*/
static int
TcpInit(psPort)
short *psPort;
{
	register struct hostent *hp;
	auto struct servent *pSE;
	auto char acServ[] = "labinfo";

	if ((struct servent *)0 == (pSE = getservbyname(acServ, "tcp"))) {
		fprintf(stderr, "%s: getservbyname: %s: %s\n", progname, acServ, strerror(errno));
		exit(1);
	}
	*psPort = ntohs(pSE->s_port);

	/* get the out hostname and the loopback devices IP address
	 * (for trusted connections mostly)
	 */
	if (-1 == gethostname(acMyName, sizeof(acMyName)-1)) {
		fprintf(stderr, "%s: gethostname: %s\n", progname, hstrerror(h_errno));
		exit(2);
	}
	if ((struct hostent *)0 != (hp = gethostbyname(acLocalhost))) {
#if USE_STRINGS
		(void)bcopy((char *)hp->h_addr, (char *)&local_port.sin_addr, hp->h_length);
#else
		memcpy((char *)&local_port.sin_addr, (char *)hp->h_addr, hp->h_length);
#endif
	} else {
		acLocalhost[0] = '\000';
	}

}

/* we have iHosts which report to our sever, look back in time		(ksb)
 * to see where in a modulo lTotal frame they report.  Quantize to
 * lSegs segments of length lLen and count how many happend in each segment
 * mark them with which segment they are in as a side effect
 */
static long *
BoxUp(lTotal, lSeg, lLen, iHosts, ppHIList)
long lTotal, lSeg, lLen;
int iHosts;
HOSTINFO **ppHIList;
{
	register int i;
	register long *plCounts;
	auto time_t tNow, tThen;
	register HOSTINFO *pHIThis;

	plCounts = (long *)calloc(lSeg, sizeof(long));
	if ((long *)0 == plCounts) {
		OutOfMem();
	}
	time(& tNow);
	for (i = 0; i < iHosts; ++i) {
		pHIThis = ppHIList[i];
		tThen = strtol(pHIThis->pclastping, (char *)0, 16);

		pHIThis->lseg = ((tNow - tThen) % lTotal) / lLen;
		if (pHIThis->lseg < 0)
			pHIThis->lseg += lSeg;
		printf("%10s %4d%s", pHIThis->pchost, pHIThis->lseg, 3 == (i & 3) ? "\n" : "\t");
		plCounts[pHIThis->lseg]++;
	}
	return plCounts;
}


/* all this code to shift the labd daemons into a stagger'd report	(ksb)
 * pattern.  If they are all clustered reporting in a few seconds
 * this routine should `unbunch' them shuch that they report evenly
 * across the whole report interval, more or less
 *
 * if provided argv[1] is the number of seconds to distribute the
 * client reports over, default is 140 seconds (read from labd with 't')
 */
int
cmStagger(argc, argv)
int argc;
char **argv;
{
	static int fTcpFired = 0;
	static short sPort;
	register SERVER *pSICur;
	register int i;
	auto long lPer, iMaxQuiet, lRem, lTotal, lLen;
	register long *plCounts;
	register long lSeg, iHosts;
	auto struct timeval tmStag;

	if (!fTcpFired) {
		fTcpFired = 1;
		TcpInit(& sPort);
	}

	if ('\000' == acServer[0] || (SERVER *)0 == (pSICur = FindServer(acServer, sPort))) {
		fprintf(stderr, "%s: stagger: no server\n", progname);
		return 1;
	}

	GetPingData(pSICur);
	if (fDebug) {
		for (i = 0; i < pSICur->ihosts; ++i) {
			printf("%-15s%s", pSICur->ppHIsort[i]->pchost, 3 == (i & 3) ? "\n" : "\t");
		}
		if ((i & 3) != 0) {
			printf("\n");
		}
	}
	iHosts = pSICur->ihosts;

	if (3 == argc) {
		iMaxQuiet = atol(argv[2]);
		lTotal = atol(argv[1]);
	} else if (2 == argc) {
		iMaxQuiet = 3;
		lTotal = atol(argv[1]);
	} else {
		auto char *pcCvt;
		auto char acTime[MAXUDPSIZE];

		if (Tell("t", acTime, 1)) {
			fprintf(stderr, "%s: stagger: couldn't find time spec\n", progname);
			return 1;
		}
		iMaxQuiet = strtol(acTime, & pcCvt, 16);
		if ('*' != pcCvt[0]) {
			fprintf(stderr, "%s: stagger: time spec format changed\n", progname);
			return 1;
		}
		lTotal = strtol(pcCvt+1, & pcCvt, 16);
	}
	if (0 == lTotal) {
		fprintf(stderr, "%s: cannot stagger over zero seconds\n", progname);
		return 1;
	}

	tmStag.tv_sec = lTotal;
	lRem = tmStag.tv_sec % iHosts;
	tmStag.tv_sec /= iHosts;
	tmStag.tv_usec = 100000 * lRem / iHosts;

	/* lPer is how many hosts should report in any interval
	 * lSeg is how many intervals (segments) we have
	 * tmStag is the actual time and interval is long
	 * if that is < 1 second we have lPer (> 1) per seond max
	 */
	if (0 == tmStag.tv_sec) {
		lLen = 1;
		lPer = (iHosts+lTotal-1)/lTotal;
	} else {
		lLen = lTotal / iHosts;
		lPer = iHosts * lLen / lTotal;
	}
	lSeg = (lTotal+lLen-1) / lLen;
	if (lPer < 1)
		lPer = 1;
	/* so now he have from 1 to lPer hosts reporting per tmStag
	 * seconds.    (lPer * lSeg >= iHosts)
	 * and a segments (lLen * lSeg >= lTotal)
	 */
	printf("%ld hosts report each %ld seconds * %ld segments for %ld total\n", lPer, lLen, lSeg, iHosts);

	plCounts = BoxUp(lTotal, lSeg, lLen, iHosts, pSICur->ppHIsort);

	for (i = 0; i < lSeg; ++i) {
		printf("%3d: %2d\n", i, plCounts[i]);
	}
	return 0;
}
