#!mckmd
# $Id: labstat.m,v 4.10 1998/02/11 14:20:18 ksb Exp $
# $Compile: %b %o -mMkcmd %f && cc_debug="-fwritable-strings ${cc_debug}" %b %o prog.c
# $Mkcmd: ${mkcmd-mkcmd} std_help.m std_version.m %f

%%
static char rcsid[] =
	"$Id: labstat.m,v 4.10 1998/02/11 14:20:18 ksb Exp $";
%%


from "<sys/types.h>"
from "<sys/param.h>"
from "<sys/time.h>"
from "<sys/socket.h>"
from "<netinet/in.h>"
from "<fcntl.h>"
from "<stdio.h>"
from "<netdb.h>"
from "<pwd.h>"
from "<errno.h>"
from "<ctype.h>"

from '"machine.h"'
from '"labstat.h"'
from '"main.h"'

basename "labstat" ""
getenv "LABSTAT"

boolean 'v' {
	named "fVerbose"
	help "explain why machine was selected"
}

# which general output style/mode
exclude "bcflnrxz"
exclude "amsWT"

escape accum [","] {
	named "pcMaps" "pcForce"
	param "maps"
	help "comma separated list of maps to display"
}

boolean 'a' {
	exclude '+'
	named "fAll"
	help "list all managed hosts (text mode)"
	list {
		param ""
		update "if (%# > 0) {fprintf(stderr, \"%%s: too many arguments (%%s%%s)\\n\", %b, argv[%K<getopt_switches>1v], %# > 1 ? \"...\" : \"\");exit(1);}"
		user "list_func(%#, %a);"
	}
}

action 'm' {
	exclude "bcflnrxzLt+"
	update "/* trapped at exit */"
	help "output all the maps available"
	exit {
		named "Maps"
		update "%n();"
	}
}
action 's' {
	exclude "bcflnrxzLt+"
	update "/* trapped at exit */"
	help "report of ticket selection statistics"
	exit {
		named "Report"
		update "%n();"
	}
}
boolean 'T' {
	exclude "bcflnrxzLt+"
	hidden named "fTerm"
	help "terminate the labd/labwatch back end processes"
}
augment action 'V' {
	user '%Ran(%w);Version();'
}
action 'W' {
	exclude "bcflnrxzLt+"
	update "/* trapped at exit */"
	help "select an idle machine for an rlogin connection"
	exit {
		named "Which"
		update "%n();"
	}
}

action 'w' {
	exclude "bcflnrxzLt+W"
	update "/* trapped at exit */"
	help "list machines with the given logins"
	zero {
		update 'fprintf(stderr, "%%s: must provide a list of logins\\n", %b);'
		aborts 'exit(1);'
	}
	list {
		named "Where"
		param "logins"
		help "locate these logins"
	}
}

# how (over head [default], labeled, or text)
boolean 'L' {
	named "fLabel"
	help "label uninteresting hosts"
}

boolean 't' {
	named "fText"
	help "output information in a text format"
}

# which output format for info cell
char* 'F' {
	hidden named "pcTempl"
	init "acTempl"
	user "%rtn = !%rti;"
	help "template for text mode output, implies -t"
}

char* variable "pcDefFmt" {
	init '"u"'
}

action 'b' {
	help "show boot time of hosts"
	update 'pcDefFmt = "b";'
}

action 'c' {
	help "show console login time"
	update 'pcDefFmt = "c";'
}

function 'C' {
	hidden update 'pcDefFmt = %N;'
	help "set the default output character to anything"
}

action 'f' {
	help "show mounted filesystems"
	update 'pcDefFmt = "f";'
}

action 'n' {
	update 'pcDefFmt = "n";'
	help "show user counts (including idle users)"
}

action 'l' {
	update 'pcDefFmt = "l";'
	help "show load average"
}

action 'r' {
	help "show last report time"
	update 'pcDefFmt = "r";'
}

action 'x' {
	update 'pcDefFmt = "x";'
	help "show unresponsive hosts"
}

long 'X' {
	hidden named "iXDeathSecs"
	init "600"
	param "seconds"
	help "dead timeout in seconds"
}

integer 'z' {
	named "iConnect"
	param "minutes"
	init '120'
	user 'pcDefFmt = "z";%n *= 60;'
	help "show console users on longer than minutes"
}


# getting the rest of the command line
after {
}

list {
	param "hosts"
	help "just output information on these hosts"
}

accum[","] 'M' {
	named "pcInMaster"
	param "masters"
	help "a comma separated list of labwatch hosts"
}
integer 'P' {
	hidden named "iPort"
	init '0'
	help "default labwatch port"
}

%i
#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

extern char *getenv();
extern char *getpass();

char acTempl[] = "%{@:h} %{@:r} %{@}";
%%


%c

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
	acMesg[8192+2],		/* the buffer for startup negotiation	*/
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


/* read a reply from the lab watcher					(ksb)
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

#define BUF_START	10240
#define BUF_CHUNK	2048
#define BUF_MIN		140

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

/* poll the running labwatch for block info			    (ksb&ben)
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
GetPort(pPort, pcToHost, sPort)
struct sockaddr_in *pPort;
char *pcToHost;
short sPort;
{
	register int fd;
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
		fprintf(stderr, "%s: connect: %d@%s: %s\n", progname, ntohs(pPort->sin_port), pcToHost, strerror(errno));
		return -1;
	}

	(void)ReadReply(fd, acIntro, sizeof(acIntro), 220);
	return fd;
}

typedef struct HInode {		/* information we know about a client */
	char *pchost;
	char *pcboottime;
	char *pclastping;
	char *pcavenrun;
	char *pcconsoletime;
	char *pcusers;
	char *pcunique;
	char *pcfs;
	/* computed */
	char *pcpermusers;
	int fConsoleUser;
	int fmark;
} HOSTINFO;

/* order the hosts for binary serach					(ksb)
 */
static int
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

/* find the labwatch daemon out on the net, if it is on our		(ksb)
 * machine we must look it up by the hostname
 */
static SERVER *
FindServer(pcName)
char *pcName;
{
	register SERVER *pSIScan, **ppSI, *pSINew;
	register char *pcTemp;
	register short sPort;
	register int iCmp;
	register struct hostent *hp;

	if (0 == strcmp(acLocalhost, pcName)) {
		pcName = acMyName;
	}
	if ((char *)0 != (pcTemp = strchr(pcName, ':'))) {
		*pcTemp = '\000';
		sPort = atoi(pcTemp + 1);
	} else {
		sPort = iPort;
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
	pSINew->s = GetPort(& pSINew->client_port, pSINew->pcname, sPort);
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

	for (pSI = pSIList; (SERVER *)0 != pSI; pSI = pSI->pSInext) {
		if (-1 == pSI->s)
			continue;
		SendOut(pSI->s, "quit\n", 5);
		ReadReply(pSI->s, acMesg, sizeof(acMesg), 221);
		close(pSI->s);
		pSI->s = -1;
	}
}

/* we should get a ``211 pid'' form labwatch				(ksb)
 * extract the pid info for everyone
 */
static void
SetPid(pSI)
SERVER *pSI;
{
	register char *pcText;

	SendOut(pSI->s, "pid\n", 4);
	pcText = ReadReply(pSI->s, acMesg, sizeof(acMesg), 211);

	pSI->ipid = atoi(pcText);
	if (1 == pSI->ipid || 0 == pSI->ipid) {
		fprintf(stderr, "%s: server pid fails sanity check (%d)\n", progname, pSI->ipid);
		exit(1);
	}
}

/* call GetBlockData to get the host info in long format		(ksb)
 */
static void
GetHostData(pSIFrom)
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

	if (0 == GetBlockData(apcChunk, pSIFrom->s, "long\n", &iMark)) {
		exit(1);
	}
	if (216 != iMark) {
		fprintf(stderr, "%s: out of sync (%d != %d)\n", progname, 216, iMark);
		exit(2);
	}

	/* pcText is header, pcBlock is the data
	 *  header: `long dump.  [not more than 200 hosts now]'
	 *   block: ` fe___:.....\n fe___:....\n'
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
		pHIData[i].pcboottime = pcBlock;
		pcBlock = strchr(pcBlock, ':');
		*pcBlock++ = '\000';
		pHIData[i].pclastping = pcBlock;
		pcBlock = strchr(pcBlock, ':');
		*pcBlock++ = '\000';
		pHIData[i].pcavenrun = pcBlock;
		pcBlock = strchr(pcBlock, ':');
		*pcBlock++ = '\000';
		pHIData[i].pcconsoletime = pcBlock;
		pcBlock = strchr(pcBlock, ':');
		*pcBlock++ = '\000';
		if (! (pHIData[i].fConsoleUser = (' ' != pcBlock[0]))) {
			++pcBlock;
		}
		pHIData[i].pcpermusers = pHIData[i].pcusers = pcBlock;
		pcBlock = strchr(pcBlock, ':');
		*pcBlock++ = '\000';
		pHIData[i].pcunique = pcBlock;
		pcBlock = strchr(pcBlock, ':');
		*pcBlock++ = '\000';
		pHIData[i].pcfs = pcBlock;
		pcBlock = strchr(pcBlock, '\n');
		*pcBlock++ = '\000';
		pHIData[i].fmark = 0;

		if ('\000' == pHIData[i].pcusers[0]) {
			pHIData[i].pcpermusers = pHIData[i].pcusers = (char *)0;
			pHIData[i].fConsoleUser = 0;
		}

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
		GetHostData(pSIIn);
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

static time_t tStart;

/* fit all that nice date info into 8 characters		     (ben&ksb)
 */
static char *
PackDate(tWhen)
time_t tWhen;
{
	register char *pcCtime = ctime(& tWhen);
	register int iDiff = tStart - tWhen;
	static char acResult[9];

	(void)strcpy(acResult, "      ");
	acResult[7] = '\000';
	acResult[8] = '\000';

	if (0 == tWhen) {
		return (char *)0;
	}
	if (iDiff < 0) {
		iDiff = -iDiff;
	}
	if (iDiff < 7 * 24 * 60 * 60) {
		(void)strncpy(acResult, pcCtime, 3);
		(void)strncpy(acResult + 3, pcCtime + 11, 5);
		return acResult;
	}
	if (iDiff < 365 * 12 * 60 * 60) {
		(void)strncpy(acResult, pcCtime + 4, 6);
		return acResult;
	}
	(void)strncpy(acResult, pcCtime + 20, 4);
	(void)strncpy(acResult + 5, pcCtime + 4, 3);
	return acResult;
}

/* ksb wants to look professional				     (ben&ksb)
 */
static int
LetterCmp(pchL, pchR)
char *pchL, *pchR;
{
	return *pchL - *pchR;
}

/* take `host' or `host:format' and output it			     (ksb&ben)
 */
static char *
Expand(pSIIn, pcFormat, fCenter)
SERVER *pSIIn;
char *pcFormat;
int fCenter;
{
	register char *pcColon, *pcCenter, *pcTemp;
	register HOSTINFO *pHIThis;
	register int iPad, i;
	register time_t tReport;
	auto char acBuf[9];
	static char acRet[9];

	if ((char *)0 != (pcColon = strchr(pcFormat, ':'))) {
		*pcColon = '\000';
	}

	pHIThis = FindHost(pSIIn, pcFormat);
	if ((char *)0 != pcColon) {
		*pcColon++ = ':';
	} else {
		pcColon = pcDefFmt;
	}

	if ((HOSTINFO *)0 == pHIThis) {
		return "        ";
	}
	pcCenter = fLabel ? pHIThis->pchost : "";

	/* pull out the info the user wants to see in 8 chars, please
	 */
	switch (*pcColon) {
	case 'B':
		pcCenter = "UNSEEN";
		/*fallthrough*/
	case 'b': 	/* boottime		*/
		pcTemp = PackDate((time_t)strtol(pHIThis->pcboottime, (char **)0, 16));
		if ((char *)0 != pcTemp)
			pcCenter = pcTemp;
		break;
	case 'C':
		pcCenter = "IDLE";
		/*fallthrough*/
        case 'c':	/* consoletime		*/
		if (pHIThis->fConsoleUser) {
			pcTemp = PackDate((time_t)strtol(pHIThis->pcconsoletime, (char **)0, 16));
			if ((char *)0 != pcTemp)
				pcCenter = pcTemp;
		}
		break;
	case 'F':
        case 'f':	/* mounted filesystems	*/
		pcCenter = acBuf;
		for (pcTemp = pHIThis->pcfs; /* */; ++pcTemp) {
			if (' ' == pcTemp[0] || '\000' == pcTemp[0]) {
				*pcCenter++ = pcTemp[-1];
				if ('\000' == pcTemp[0])
					break;
			}
		}
		*pcCenter = '\000';
		pcCenter = acBuf;
		qsort(acBuf, strlen(acBuf), 1, LetterCmp);
		if (fLabel && '\000' == acBuf[0]) {
			pcCenter = pHIThis->pchost;
		}
		break;
	case '\000':
	case 'H':
	case 'h':	/* hostname		*/
		pcCenter = pHIThis->pchost;
		break;
	case 'L':	/* load average forced	*/
	case 'l':	/* load average 	*/
		sprintf(acBuf, "%4.2f", (double)strtol(pHIThis->pcavenrun, (char **)0, 16) / XMIT_SCALE);
		if (isupper(*pcColon) || 0 != strcmp("0.00", acBuf)) {
			pcCenter = acBuf;
		}
		break;
	case 'N':
	case 'n':	/* count of idle users	*/
		i = strtol(pHIThis->pcunique, (char **)0, 16);
		if (isupper(*pcColon) || 0 != i) {
			sprintf(acBuf, "%d", i);
			pcCenter = acBuf;
		}
		break;
	case 'O':	/* on console */
		pcCenter = "nobody";
	case 'o':
		if (!pHIThis->fConsoleUser) {
			break;
		}
		if ((char *)0 != (pcTemp = strchr(pHIThis->pcpermusers, ' '))) {
			*pcTemp = '\000';
		}
		(void)strcpy(acBuf, pHIThis->pcpermusers);
		if ((char *)0 != pcTemp) {
			*pcTemp = ' ';
		}
		pcCenter = acBuf;
		break;
	case 'R':
		pcCenter = "NEVER";
		/*fallthrough*/
        case 'r':	/* lastping		*/
		pcTemp = PackDate((time_t)strtol(pHIThis->pclastping, (char **)0, 16));
		if ((char *)0 != pcTemp)
			pcCenter = pcTemp;
		break;
	case 'U':	/* next user or blanks		*/
		pcCenter = "";
		/*fallthrough*/
	case 'u':	/* next user or machine name	*/
		if ((char *)0 == (pcTemp = pHIThis->pcusers)) {
			break;
		}
		if ('-' == pcTemp[0]) {
			++pcTemp;
		}
		if ((char *)0 != (pHIThis->pcusers = strchr(pcTemp, ' '))) {
			++pHIThis->pcusers;
		}
		pcCenter = pcTemp;
		break;
	case 'X':
		pcCenter = "OK";
		/*fallthrough*/
	case 'x':
		tReport = strtol(pHIThis->pclastping, (char **)0, 16);
		if (0 == tReport) {
			pcCenter = "UNSEEN";
		} else if (tStart - tReport >= iXDeathSecs) {
			pcCenter = "DEAD";
		}
		break;
	case 'Z':
        case 'z':	/* users who have been logged in z secs */
		if (!pHIThis->fConsoleUser || tStart - strtol(pHIThis->pcconsoletime, (char **)0, 16) < iConnect) {
			break;
		}
		(void)strncpy(acBuf, pHIThis->pcpermusers, 8);
		if ((char *)0 != (pcTemp = strchr(acBuf, ' '))) {
			*pcTemp = '\000';
		}
		pcCenter = acBuf;
		break;
	default:
		pcCenter = "error";
		break;
	}
	iPad = 8 - strlen(pcCenter);
	if (iPad < 0 || !fCenter)
		iPad = 0;
	iPad /= 2;
	sprintf(acRet, "%.*s%-*.*s", iPad, "        ", 8-iPad, 8-iPad, pcCenter);
	return acRet;
}

/* output one line of a map diagram					(ksb)
 */
static void
Dump(pSIFrom, pcLine, fCenter)
SERVER *pSIFrom;
char *pcLine;
int fCenter;
{
	register char *pcTemp;

	while ('\000' != *pcLine) {
		if ('%' != *pcLine) {
			putchar(*pcLine);
			++pcLine;
			continue;
		}
		if ('{' != *++pcLine) {
			putchar(*pcLine);
			++pcLine;
			continue;
		}
		pcTemp = ++pcLine; /*{*/
		while ('}' != *pcLine && '\000' != *pcLine) {
			++pcLine;
		} /*{*/
		if ('}' == *pcLine) {
			*pcLine = '\000';
			fputs(Expand(pSIFrom, pcTemp, fCenter), stdout); /*{*/
			*pcLine++ = '}';
		} else {
			fputs(pcTemp-2, stdout);
		}
	}
	fputs("\n", stdout);
}

/* expand @ to the host we want to see					(ksb)
 */
static void
MetaExpand(pcOut, pcIn, pHIHost)
char *pcOut, *pcIn;
HOSTINFO *pHIHost;
{
	while ('\000' != *pcIn) {
		if ('@' != (*pcOut = *pcIn++)) {
			++pcOut;
			continue;
		}
		(void)strcpy(pcOut, pHIHost->pchost);
		pcOut += strlen(pcOut);
	}
	*pcOut = '\000';
}


/* mainline for lab client application					(ksb)
 * setup who we are, and what our loopback addr is
 * get a shutdown pid
 */
/*ARGSUSED*/
static int
after_func(dummy)
int dummy;
{
	register struct hostent *hp;
	register struct servent *pSE;
	static char acServ[] = SERVICE;

	if (0 != iPort) {
		/* use the one they set */
	} else if (isdigit(acServ[0])) {
		iPort = atoi(acServ);
	} else if ((struct servent *)0 == (pSE = getservbyname(acServ, "tcp"))) {
		fprintf(stderr, "%s: getservbyname: %s: %s\n", progname, acServ, strerror(errno));
		exit(1);
	} else {
		iPort = ntohs(pSE->s_port);
	}

	if (0 == iPort) {
		fprintf(stderr, "%s: must set a port with -P, none compiled in\n", progname);
		exit(1);
	}

	/* get the out hostname and the loopback devices IP address
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

static char acDefMaster[] = DEF_MASTER;

/* augment the std version string with more info			(ksb)
 */
static void
Version()
{
	printf("%s: connect to labwatch on port %d/tcp\n", progname, iPort);
	printf("%s: default search list `%s'\n", progname, acDefMaster);
}

/* search the labwatch servers for a `thing'				(ksb)
 */
static int
SearchServers(pfiCall, ppcWith)
int (*pfiCall)();
char **ppcWith;
{
	static char acComma[] = ",";
	register char *pcComma, *pcScan;
	register int iRet;
	register SERVER *pSICur;

	pcScan = (char *)0 != pcInMaster ? pcInMaster : acDefMaster;
	for (iRet = 0; 0 == iRet && '\000' != pcScan[0]; *pcComma++ = ',', pcScan = pcComma) {
		if ((char *)0 == (pcComma = strchr(pcScan, ','))) {
			pcComma = acComma;
		}
		*pcComma = '\000';

		if ((SERVER *)0 == (pSICur = FindServer(pcScan))) {
			continue;
		}
		iRet = (*pfiCall)(pSICur, ppcWith);
	}
	return iRet;
}

/* poll one labwatch for an idle machine				(ksb)
 */
static int
S_Report(pSIIn, ppcIgnore)
SERVER *pSIIn;
char **ppcIgnore;
{
	register char *pcText;
	auto char *pcResid;
	auto time_t tReport;
	unsigned long uOK, uOut, uSame;
	static int fFirst = 1;

	SendOut(pSIIn->s, "stats\n", 6);
	pcText = ReadReply(pSIIn->s, acMesg, sizeof(acMesg), 0);
	if (219 == atoi(pcText)) {
		if (fFirst && fVerbose) {
			printf("%24s %7s %7s %7s %s\n", "Since", "Given", "Denied", "Same", "Server");
		}
		fFirst = 0;
		pcText += 4 + 6 /* `219 ' + `stats ' */;
		tReport = strtol(pcText, & pcResid, 16);
		uOK = strtol(pcResid+1, & pcResid, 16);
		uOut = strtol(pcResid+1, & pcResid, 16);
		uSame = strtol(pcResid+1, & pcResid, 16);
		printf("%.24s %7d %7d %7d %s\n", ctime(& tReport), uOK, uOut, uSame, pSIIn->pcname);
	}
	return 0;
}

/* poll all the labwatch's for an idle machine				(ksb)
 */
static void
Report()
{
	(void)SearchServers(S_Report, (char **)0);
	exit(0);
}

/* look for a named login on the clusters				(ksb)
 */
static int
S_Where(pSIIn, ppcWho)
SERVER *pSIIn;
char **ppcWho;
{
	register int i, j, fBanner;
	register HOSTINFO **ppHISort, *pHI;
	register char *pcLogin, *pcHList;

	GetHostData(pSIIn);

	if (fVerbose) {
		printf("%s:\n", pSIIn->pcname);
	}
	for (ppHISort = pSIIn->ppHIsort, i = 0; i < pSIIn->ihosts; ++i, ++ppHISort) {
		pHI = *ppHISort;
		if ((char *)0 == (pcHList = pHI->pcpermusers)) {
			continue;
		}
		fBanner = 1;
		do {
			register char *pcNext;

			if ('-' == pcHList[0])
				++pcHList;
			if ((char *)0 != (pcNext = strchr(pcHList, ' ')))
				*pcNext = '\000';

			for (j = 0; (char *)0 != (pcLogin = ppcWho[j]); ++j) {
				if (0 != strcmp(pcLogin, pcHList)) {
					continue;
				}
				if (fBanner) {
					printf("%s:", pHI->pchost);
					fBanner = 0;
				}
				printf(" %s", pcLogin);
			}

			if ((char *)0 != (pcHList = pcNext)) {
				*pcHList++ = ' ';
			}
		} while ((char *)0 != pcHList);
		if (!fBanner) {
			printf("\n");
		}
	}
	return 0;
}

/* poll all for a given login name (or list)				(ksb)
 */
static void
Where(argc, argv)
{
	(void)SearchServers(S_Where, argv);
	exit(0);
}

/* poll one labwatch for an idle machine				(ksb)
 */
static int
S_Which(pSIIn, ppcIgnore)
SERVER *pSIIn;
char **ppcIgnore;
{
	register char *pcText, *pcSpace;

	SendOut(pSIIn->s, "which\n", 6);
	pcText = ReadReply(pSIIn->s, acMesg, sizeof(acMesg), 0);
	if (230 == atoi(pcText)) {
		pcText += 4;
		if (!fVerbose && (char *)0 != (pcSpace = strchr(pcText, ' '))) {
			*pcSpace = '\000';
		}
		printf("%s\n", pcText);
		return 1;
	}
	return 0;
}

/* poll all the labwatch's for an idle machine				(ksb)
 */
static void
Which()
{
	if (0 == SearchServers(S_Which, (char **)0)) {
		fprintf(stderr, "%s: no idle machines\n", progname);
		exit(1);
	}
	exit(0);
}

/* output all the maps all the servers know about			(ksb)
 */
static int
S_PrintMap(pSIIn, ppcIgnore)
SERVER *pSIIn;
char **ppcIgnore;
{
	auto char *apcChunk[3];
	auto int iMark;
	register char *pcMap;

	if (0 == GetBlockData(apcChunk, pSIIn->s, "map\n", & iMark)) {
		return 0;
	}
	/* out current server doesn't know -- save for next server
	 */
	if (325 == iMark) {
		return 0;
	}
	if (fVerbose) {
		printf("%s:\n", pSIIn->pcname);
	}
	if ((char *)0 == (pcMap = apcChunk[1])) {
		return 0;
	}
	for (; '\000' != pcMap[0]; pcMap += strlen(pcMap)+1) {
		puts(pcMap);
	}
	return 0;
}

/* poll all the labwatch's for all the maps				(ksb)
 */
static void
Maps()
{
	(void)SearchServers(S_PrintMap, (char **)0);
	exit(0);
}

/* the undocumented -T option outputs a shutdown line			(ksb)
 */
/*ARGSUSED*/
static int
S_Kill(pSIIn, ppc)
SERVER *pSIIn;
char **ppc;
{
	SetPid(pSIIn);
	if (0 == strcmp(acMyName, pSIIn->pcname))
		printf("%s: kill -TERM %d\n", progname, pSIIn->ipid);
	else
		printf("%s: rsh %s kill -TERM %d\n", progname, pSIIn->pcname, pSIIn->ipid);
	return 0;
}

/* find all the hosts on this server, and expand in text		(ksb)
 */
/*ARGSUSED*/
static int
S_All(pSIIn, ppc)
SERVER *pSIIn;
char **ppc;
{
	register int i;
	register HOSTINFO **ppHISort;
	register char *pcBuf;

	GetHostData(pSIIn);

	pcBuf = malloc(strlen(pcTempl)*3+1024);
	if ((char *)0 == pcBuf) {
		OutOfMem();
	}

	if (fVerbose) {
		printf("%s:\n", pSIIn->pcname);
	}
	for (ppHISort = pSIIn->ppHIsort, i = 0; i < pSIIn->ihosts; ++i, ++ppHISort) {
		MetaExpand(pcBuf, pcTempl, *ppHISort);
		Dump(pSIIn, pcBuf, 0);
	}
	(void)free(pcBuf);
	return 0;
}

/* expand the host stuff in-line in the map				(ksb)
 */
static void
Tag(pSIFrom, pcLine)
SERVER *pSIFrom;
char *pcLine;
{
	register char *pcTemp, *pcColon;
	register HOSTINFO *pHIThis;

	while ('\000' != *pcLine) {
		if ('%' != *pcLine++) {
			continue;
		}
		if ('{' != *pcLine++) {
			continue;
		}
		pcTemp = pcLine; /*{*/
		while ('}' != *pcLine && '\000' != *pcLine) {
			++pcLine;
		} /*{*/
		if ('}' != *pcLine) {
			continue;
		}

		*pcLine = '\000';
		if ((char *)0 != (pcColon = strchr(pcTemp, ':'))) {
			*pcColon = '\000';
		}

		if ((HOSTINFO *)0 != (pHIThis = FindHost(pSIFrom, pcTemp))) {
			pHIThis->fmark = 1;
		}

		if ((char *)0 != pcColon) {
			*pcColon = ':';
		} /*{*/
		*pcLine++ = '}';
	}
}

/* the Map code found a map and marked all the hosts that would be	(ksb)
 * displayed if we did that -- now output them
 */
static void
TextView(pSIFrom, fForce)
SERVER *pSIFrom;
int fForce;
{
	register int i;
	register HOSTINFO **ppHISort, *pHI;
	register char *pcBuf;

	pcBuf = malloc(strlen(pcTempl)*3+1024);
	if ((char *)0 == pcBuf) {
		OutOfMem();
	}

	for (ppHISort = pSIFrom->ppHIsort, i = 0; i < pSIFrom->ihosts; ++i, ++ppHISort) {
		pHI = *ppHISort;
		if (0 == pHI->fmark && !fForce)
			continue;
		pHI->fmark = 0;
		MetaExpand(pcBuf, pcTempl, pHI);
		Dump(pSIFrom, pcBuf, 0);
	}
	(void)free(pcBuf);
}

/* filter out the maps this server has, leave the rest for the next	(ksb)
 * pass (next server).
 */
static int
S_Map(pSIIn, ppcList)
SERVER *pSIIn;
char **ppcList;		/* a comma separated list of maps to display	*/
{
	static char acComma[] = ",";
	auto char acReq[80];
	register char *pcComma, *pcList;
	register char *pcNext, *pcMem;
	auto char *apcChunk[3];
	auto int iMark;
	auto char *pcMap;

	if ((char *)0 == (pcList = *ppcList) || '\000' == pcList[0]) {
		return 1;
	}

	pcMem = pcNext = malloc((strlen(pcList)|3)+1);
	pcNext[0] = '\000';
	for (/**/; '\000' != pcList[0]; *pcComma++ = ',', pcList = pcComma) {
		if ((char *)0 == (pcComma = strchr(pcList, ','))) {
			pcComma = acComma;
		}
		*pcComma = '\000';

		(void)sprintf(acReq, "map %s\n", pcList);
		if (0 == GetBlockData(apcChunk, pSIIn->s, acReq, & iMark)) {
			continue;
		}
		pcMap = apcChunk[1];
		/* out current server doesn't know -- save for next server
		 * (or 325, no maps at all -- bj)
		 */
		if (324 == iMark || 325 == iMark) {
			if (pcNext != pcMem) {
				*pcNext++ = ',';
			}
			(void)strcpy(pcNext, pcList);
			pcNext += strlen(pcNext);
			continue;
		}

		GetHostData(pSIIn);
		for (/**/; '\000' != pcMap[0]; pcMap += strlen(pcMap)+1) {
			register char *pcKeep;
			if ((char *)0 != (pcKeep = strchr(pcMap, '\n'))) {
				*pcKeep = '\000';
			}
			if (fText) {
				Tag(pSIIn, pcMap);
				continue;
			}
			if ('@' == *pcMap) {
				pcKeep = pcTempl;
				pcTempl = pcMap+1;
				TextView(pSIIn, 1);
				pcTempl = pcKeep;
				continue;
			}
			Dump(pSIIn, pcMap, 1);
		}
		if (fText) {
			TextView(pSIIn, 0);
		}
	}
	if ('\000' == pcMem[0]) {
		free(pcMem);
		*ppcList = (char *)0;
		return 1;
	}
	free(*ppcList);
	*ppcList = pcMem;
	return 0;
}

/* find a host and output info about it					(ksb)
 */
static int
S_Host(pSIIn, ppcHost)
SERVER *pSIIn;
char **ppcHost;
{
	register HOSTINFO *pHIThis;
	auto char acBuf[1024];

	GetHostData(pSIIn);

	pHIThis = FindHost(pSIIn, *ppcHost);
	if ((HOSTINFO *)0 == pHIThis) {
		return 0;
	}
	MetaExpand(acBuf, pcTempl, pHIThis);
	Dump(pSIIn, acBuf, 0);
	return 1;
}

/* try to do the right thing, the argument list is not			(ksb)
 * as nice as I'd like -- but I'm emulating a kludge or two
 */
/*ARGSUSED*/
static void
list_func(argc, argv)
int argc;
char **argv;
{
	auto int iRet;
	extern time_t time();

	time(& tStart);

	iRet = 0;
	if (fTerm) {
		(void)SearchServers(S_Kill, (char **)0);
	} else if (0 != argc) {
		for (/* args*/; 0 != argc--; ++argv) {
			if (0 != SearchServers(S_Host, argv)) {
				continue;
			}
			fprintf(stderr, "%s: unknown host `%s'\n", progname, *argv);
			++iRet;
		}
	} else if (fAll) {
		auto char *pcSaw = (char *)0;
		if (0 != SearchServers(S_All, & pcSaw)) {
			fprintf(stderr, "%s: %s\n", progname, pcSaw);
			++iRet;
		}
	} else if ((char *)0 == pcMaps) {
		pcMaps = strdup("*");
	}
	if ((char *)0 != pcMaps) {
		(void)SearchServers(S_Map, & pcMaps);
		if ((char *)0 != pcMaps) {
			fprintf(stderr, "%s: unknown map%s `%s'\n", progname, (char *)0 == strchr(pcMaps, ',') ? "" : "s", pcMaps);
			iRet = 1;
		}
	}
	CloseServers();

	exit(iRet);
}
