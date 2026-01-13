#!mkcmd

from '<sys/types.h>'
from '<sys/param.h>'
from '<sys/file.h>'
from '<sys/stat.h>'
from '<sys/socket.h>'
from '<sys/time.h>'
from '<netinet/in.h>'
from '<signal.h>'
from '<string.h>'
from '<errno.h>'
from '<netdb.h>'
from '<stdio.h>'
from '<ctype.h>'
from '<pwd.h>'

require "std_help.m" "std_version.m" "cmd.m" "cmd_help.m" "cmd_version.m"

augment action 'V' {
	user "VersionInfo();"
}

from '"machine.h"'
from '"main.h"'
from '"stagger.h"'

%h
extern char acServer[256];
%%

%i
static char *rcsid =
	"$Id: labc.m,v 2.12 1996/12/28 22:48:17 kb207252 Exp $";

#if HAVE_SELECT_H
#include <sys/select.h>
#endif

#if !defined(IBMR2)
extern char *malloc();
#endif

char *pcGuilty = (char *)0;
%%

basename "labc" ""

init "GetUsername();"
init "cmd_init(& CSLabc, aCMLabc, sizeof(aCMLabc)/sizeof(CMD), NULL, 3);"

long 'A' {
	hidden named "iAckPort"
	param "port"
	init "29047"
	help "labd uses this port to receive acks"
}

boolean 'd' {
	named "fDebug"
	init "0"
	help "run in debug mode"
}

char* 'm' {
	named "pcLabdHost"
	param "hostname"
	help "labd to control run on this host"
}

after {
}

every {
	named "cmd_from_string"
	param "commands"
	update "%n(& CSLabc, %N, (int *)0);"
	help "commands to control labd"
}

zero {
	named "cmd_from_file"
	update "%n(& CSLabc, stdin);"
}

%c
/* some debug and version clues we might need				(ksb)
 */
static void
VersionInfo()
{
	printf("%s: connect to labd on port %d/udp\n", progname, iAckPort);
	printf("%s: messages come from \"%s\"\n", progname, pcGuilty);
}


/* look for a host in the domain of the labd we are controling		(ksb)
 */
struct hostent *
SearchHostName(pcFind, pcWRT)
char *pcFind, *pcWRT;
{
	register struct hostent *pHE;
	auto char acTry[256];
	register char *pcTail, *pcTry;

	if ((struct hostent *)0 != (pHE = gethostbyname(pcFind))) {
		return pHE;
	}

	pcTry = acTry;
	pcTail = strchr(pcWRT, '.');
	while ((char *)0 != pcTail && '\000' != pcFind[0]) {
		if ('.' == (*pcTry++ = *pcFind++)) {
			pcTail = strchr(pcTail+1, '.');
		}
	}
	if ((char *)0 == pcTail) {
		return (struct hostent *)0;
	}
	(void)strcpy(pcTry, pcTail);
	return gethostbyname(acTry);
}

/* find the login name of the person running labc (usually root)	(ksb)
 */
static void
GetUsername()
{
	extern char *getenv(), *getlogin();
	register char *pcUser;
	register struct passwd *pwd;

	pcUser = getlogin();
	if ((char *)0 == pcUser || '\000' == *pcUser) {
		pcUser = getenv("USER");
	}
	if ((char *)0 == pcUser || '\000' == *pcUser) {
		pcUser = getenv("LOGNAME");
	}
	setpwent();
	if ((char *)0 != pcUser && (struct passwd *)0 != (pwd = getpwnam(pcUser)) && (0 == getuid() || pwd->pw_uid == getuid())) {
		pcGuilty = strdup(pcUser);
	} else if ((struct passwd *)0 != (pwd = getpwuid(getuid()))) {
		pcGuilty = strdup(pwd->pw_name);
	} else {
		fprintf(stderr, "%s: getpwuid: %d: cannot locate your passwd entry\n", progname, getuid());
		exit(2);
	}
	endpwent();
}

/* build a port we can send/recv mesgs				   (bj/ksb)
 * bind to a secure port so labd will trust us.
 */
static int
GetPriPort()
{
	register int iSecurePort, iRes, s;
	auto struct sockaddr_in SINrecv;

	if (-1 == (s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) {
		fprintf(stderr, "%s: socket: %s\n", progname, strerror(errno));
		exit(1);
	}
	SINrecv.sin_family = AF_INET;
	SINrecv.sin_addr.s_addr = htonl(INADDR_ANY);
	for (iSecurePort = 1023; iSecurePort > 0; --iSecurePort) {
		SINrecv.sin_port = htons(iSecurePort);
		if (-1 != (iRes = bind(s, (struct sockaddr *)&SINrecv, sizeof (SINrecv))))
			break;
	}
	if (-1 == iRes) {
		fprintf(stderr, "%s: bind: %s\n", progname, strerror(errno));
		exit(1);
	}
	return s;
}

static int fdSock;
static struct sockaddr_in SINlabd;
static char acControl[256];

/* send a command to the local labd and wait for an ACK			(ksb)
 * needs a list of machines, and an `OK to re-send flag'
 * should async xmit and recv the hosts with select, calling
 * function on each return pkt
 */
int
Tell(pcMesg, pcBuf, fResend)
char *pcMesg, *pcBuf;
int fResend;
{
	auto int iBuf, iTrys;
	auto char ac_[MAXUDPSIZE];
	auto struct sockaddr_in SINfrom;
	auto struct fd_set fdR;
	struct timeval timeout;

	if ((char *)0 == pcBuf) {
		pcBuf = ac_;
	}

	if ('\000' == acControl[0]) {
		fprintf(stderr, "%s: no connection\n", progname);
		return 1;
	}
	iTrys = 5;
resend:
	if (0 == --iTrys) {
		return 1;
	}
	FD_ZERO(&fdR);
	FD_SET(fdSock, &fdR);
	timeout.tv_sec = 1;
	timeout.tv_usec = 300;
	(void)sendto(fdSock, pcMesg, strlen(pcMesg)+1, 0, (struct sockaddr *)&SINlabd, sizeof(struct sockaddr_in));

redo:
	switch (select(fdSock+1, &fdR, 0, 0, &timeout)) {
	case -1:
		if (EINTR == errno) {
			goto redo;
		}
		exit(1);
	case 0:
		/* timeout */
		if (fResend) {
			goto resend;
		}
		return 1;
	default:
		break;
	}
	iBuf = sizeof(SINfrom);
	while (-1 == recvfrom(fdSock, pcBuf, MAXUDPSIZE, 0, (struct sockaddr *)&SINfrom, &iBuf) && EINTR == errno)
		/* */;
	return 0;
}


char acTickets[32];

/* disable tickets for labd -> labwatch					(ksb)
 */
static int
cmDisable(argc, argv)
int argc;
char **argv;
{
	return Tell("N", acTickets, 1);
}

/* enable tickets for labd -> labwatch					(ksb)
 */
static int
cmEnable(argc, argv)
int argc;
char **argv;
{
	return Tell("Y", acTickets, 1);
}

/* tell the running labd to execute fresh binary, might work		(ksb)
 */
static int
cmExec(argc, argv)
int argc;
char **argv;
{
	return Tell("x", (char *)0, 0);
}

/* get interval information from running labd				(ksb)
 * expect back %x*%x (interval * seconds)
 */
static int
cmInterval(argc, argv)
int argc;
char **argv;
{
	auto char acInterval[MAXUDPSIZE];
	auto char acHost[MAXUDPSIZE];
	auto int iInter, iSeconds, iHisPort;
	register char *pcColon;

	acHost[0] = '\000';
	(void)Tell("@", acHost, 1);
	if ((char *)0 != (pcColon = strchr(acHost, ':'))) {
		*pcColon++ = '\000';
		sscanf(pcColon, "%x", & iHisPort);
	}
	acInterval[0] = '\000';
	(void)Tell("t", acInterval, 1);
	if ('\000' == acHost[0] || 2 != sscanf(acInterval, "%x*%x", & iInter, & iSeconds)) {
		fprintf(stderr, "%s: %s: protocol botch\n", progname, acControl);
		return;
	}
	printf("%s reports to %s", acControl, acHost);
	if ((char *)0 != pcColon) {
		printf(" on port %d/udp", iHisPort);
	}
	printf("\nevery %d second%s", iSeconds, iSeconds == 1 ? "" : "s");
	if (1 != iInter)
		printf(", reporting at least 1 in every %d intervals", iInter);
	printf("\n");
}

/* output a packet from labd						(ksb)
 */
static int
cmPing(argc, argv)
int argc;
char **argv;
{
	auto char acStats[MAXUDPSIZE];

	acStats[0] = '\000';
	(void)Tell("!", acStats, 1);
	if ('\000' == acStats[0]) {
		return 1;
	}
	acTickets[0] = acStats[0];
	acTickets[1] = '\000';
	/* LLL format the packet we got back?
	 */
	printf("%s", acStats);
}

/* kill the labd on the target machines					(ksb)
 */
static int
cmShoot(argc, argv)
int argc;
char **argv;
{
	return Tell("q", (char *)0, 0);
}

char acServer[256];
static int iWatchPort;
static struct sockaddr_in SINlabwatch;

/* find our local labd on our loopback at iAckPort.			(ksb)
 * then find the server he is attached to, clever.
 */
static int
FindLab(pcHost)
char *pcHost;
{
	register struct hostent *pHE;
	register char *pcColon;
	auto char acBuf[MAXUDPSIZE];	/* server:port */

	acServer[0] = '\000';
	acControl[0] = '\000';

	SINlabd.sin_family = AF_INET;
	SINlabd.sin_port = htons(iAckPort);
	if ((char *)0 == pcHost) {
		pcHost = "localhost";
		SINlabd.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	} else if ((struct hostent *)0 == (pHE = gethostbyname(pcHost)) || (char *)0 == pHE->h_name) {
		fprintf(stderr, "%s: gethostbyname: %s: %s\n", progname, pcHost, hstrerror(h_errno));
		return 0;
	} else {
		pcHost = pHE->h_name;
#if USE_STRINGS
		bcopy(pHE->h_addr, &SINlabd.sin_addr.s_addr, pHE->h_length);
#else
		memcpy(&SINlabd.sin_addr.s_addr, pHE->h_addr, pHE->h_length);
#endif
	}
	(void)strcpy(acControl, pcHost);

	acBuf[0] = '\000';
	(void)Tell("@", acBuf, 1);
	if ((char *)0 == (pcColon = strchr(acBuf, ':'))) {
		fprintf(stderr, "%s: %s: can't find labd\n", progname, acControl);
		acControl[0] = '\000';
		return 0;
	}
	*pcColon++ = '\000';
	if ((struct hostent *)0 == (pHE = SearchHostName(acBuf, acControl))) {
		fprintf(stderr, "%s: gethostbyname: %s: %s\n", progname, acBuf, hstrerror(h_errno));
		acControl[0] = '\000';
		return 0;
	}
	(void)strcpy(acServer, pHE->h_name);
	(void)sscanf(pcColon, "%x", & iWatchPort);
	*--pcColon = ':';

	SINlabwatch.sin_family = AF_INET;
	SINlabwatch.sin_port = htons(iWatchPort);
#if USE_STRINGS
	bcopy(pHE->h_addr, &SINlabwatch.sin_addr.s_addr, pHE->h_length);
#else
	memcpy(&SINlabwatch.sin_addr.s_addr, pHE->h_addr, pHE->h_length);
#endif
	if (fDebug) {
		printf("%s: %s: found labwatch on %s:%d\n", progname, acControl, acServer, iWatchPort);
	}
	return 1;
}


/* reach out an touch someone						(ksb)
 * with no arguments grab the local machine
 */
static int
cmControl(argc, argv)
int argc;
char **argv;
{
	acTickets[0] = '\000';
	if (argc > 2) {
		fprintf(stderr, "%s: multiple hosts not supported (yet)\n", progname);
	}
	return ! FindLab(argv[1]);
}

/* let labwatch know what we are up to					(ksb)
 * if we're called bounce use labs reflection facility, which
 * only works to localhost, BTW.
 */
static int
cmMessage(argc, argv)
int argc;
char **argv;
{
	auto char acMess[MAXUDPSIZE];
	register int l, i, iLen;
	register struct sockaddr *pSA;

	sprintf(acMess, "#%s:", pcGuilty);
	iLen = strlen(acMess);
	for (i = 1; i < argc; ++i) {
		acMess[iLen++] = ' ';
		l = strlen(argv[i]);
		if (l + iLen < sizeof(acMess)) {
			strcpy(acMess+iLen, argv[i]);
			iLen += l;
			continue;
		}
		if (1 == i) {
			l = sizeof(acMess)-(iLen+1);
			strncpy(acMess+iLen, argv[i], l);
			argv[i] += l;
			iLen = sizeof(acMess)-1;
		}
		break;
	}
	pSA = 'b' == argv[0][0] ? (struct sockaddr *)&SINlabd : (struct sockaddr *)&SINlabwatch;
	(void)sendto(fdSock, acMess, iLen+1, 0, pSA, sizeof(struct sockaddr_in));
#if 0
	/* we don't really want to spam the Ops monitor with >1k of data
	 */
	if (i < argc) {
		--i;
		argv[i] = argv[0];
		cmMessage(argc-i, argv+i);
	}
#endif
}

/* tell the labd to notifiy labwatch of an impending reboot		(ksb)
 */
static int
cmReboot(argc, argv)
int argc;
char **argv;
{
	auto int iStatus, iMinutes;
	auto char acShutdown[MAXUDPSIZE], acReply[MAXUDPSIZE];

	switch (argc) {
	case 0:
	case 1:
	default:
		fprintf(stderr, "%s: %s: usage [minutes]\n", progname, argv[0]);
		return 1;
	case 2:
		iMinutes = atoi(argv[1]);
		if (0 == iMinutes) {
			fprintf(stderr, "%s: %s: %s: illegal number of minutes\n", progname, argv[0], argv[1]);
			return 1;
		}
		break;
	}
	(void)sprintf(acShutdown, "S%x", iMinutes /* *60? LLL */);
	iStatus = Tell(acShutdown, acReply, 1);
	if ('\000' != acReply[0]) {
		printf("%s: %s\n", acControl, acReply);
	}
	return iStatus;
}

/* tell the labd we control to swith to another labwatch		(ksb)
 */
static int
cmRedir(argc, argv)
int argc;
char **argv;
{
	auto int iStatus;
	auto char acRedir[MAXUDPSIZE], acReply[MAXUDPSIZE];

	switch (argc) {
	case 0:
	case 1:
	default:
		fprintf(stderr, "%s: redirect: usage host [port]\n", progname);
		return 1;
	case 2:
		sprintf(acRedir, ">%s", argv[1]);
		break;
	case 3:
		sprintf(acRedir, ">%s:%s", argv[1], argv[2]);
		break;
	}
	iStatus = Tell(acRedir, acReply, 1);
	if ('#' != acReply[0]) {
		fprintf(stderr, "%s: %s: protocol botch in redirection (%s)\n", progname, argv[0], acReply);
		exit(1);
	}
	switch (acReply[1]) {
		register char *pcColon;
		register struct hostent *pHE;
	case 'f':	/* failed */
		fprintf(stderr, "%s: redirection %s\n", progname, acReply+1);
		return 1;
	case 'o':	/* ok host:port */
		if ((char *)0 == (pcColon = strchr(acReply+4, ':'))) {
			fprintf(stderr, "%s: %s: no colon in host reply\n", progname, argv[0]);
			return 1;
		}
		*pcColon++ = '\000';
		sscanf(pcColon, "%x", & iWatchPort);
		if ((struct hostent *)0 == (pHE = SearchHostName(acReply+4, acControl))) {
			fprintf(stderr, "%s: gethostbyname: %s: %s\n", progname, acReply+4, hstrerror(h_errno));
			return 1;
		}
		(void)strcpy(acServer, pHE->h_name);
		break;
	default:
		fprintf(stderr, "%s: %s: unknown reply (%s)\n", progname, argv[0], acReply+1);
		return 1;
	}
	return iStatus;
}

/* where does the labd we are talking to send reports			(ksb)
 */
static int
cmStatus(argc, argv)
int argc;
char **argv;
{
	register int iWhich;
	register char *pcTick;

	if (1 == argc) {
		iWhich = 'a';
	} else if ((char *)0 != strchr("awhpt?", argv[1][0])) {
		iWhich = argv[1][0];
	} else {
		fprintf(stderr, "%s: unknown status request `%s' (try \"status ?\")\n", progname, argv[1]);
		return 1;
	}
	if ('?' != iWhich && '\000' == acControl[0]) {
		fprintf(stderr, "%s: not connected\n", progname);
		return 1;
	}
	pcTick = '-' == acTickets[0] ? "no" : '+' == acTickets[0] ? "yes" : "unknown";
	switch (iWhich) {
	case 'a':
		printf("%s, %s:%d, %s\n", acControl, acServer, iWatchPort, pcTick);
		break;
	case 'p':
		printf("%d\n", iWatchPort);
		break;
	case 'h':
		printf("%s\n", acControl);
		break;
	case 't':
		printf("%s\n", pcTick);
		break;
	case 'w':
		printf("%s\n", acServer);
		break;
	case '?':
	default:
		printf("all	control, host:port, tickets\n");
		printf("host	machine out labd run on\n");
		printf("port	labwatch port\n");
		printf("tickets	does the machine grant tickets?\n");
		printf("where	labwatch server our labd reports to\n");
		break;
	}
	return 0;
}

CMD aCMLabc[] = {
	{"bye",		(char *)0, (int (*)())0, CMD_RET},
	{"bounce",	"send a message to labwatch through labd", cmMessage, CMD_NULL, "[text]"},
	{"control",	"change which host(s) we control", cmControl, 0, "[host]"},
	{"disable",	"do not grant tickets for this machine", cmDisable},
	{"enable",	"grant tickets for this machine", cmEnable},
	{"execute",	"execute a fresh binary in place", cmExec},
	CMD_DEF_HELP,
	{"interval",	"report labd's sleep interval", cmInterval, CMD_NULL},
	{"message",	"send a message to labwatch from this host", cmMessage, CMD_NULL, "[text]"},
	{"ping",	"poll labd for current info", cmPing},
	CMD_DEF_QUIT,
	{"redirect",	"move labd to another server", cmRedir, CMD_NULL, "host [port]"},
	{"shoot",	"terminate labd on this host", cmShoot},
	{"shutdown",	"let labwatch know we are going down soon", cmReboot, CMD_NULL, "[minutes]"},
	{"stagger",	"spread out clients over the labd report interval", cmStagger},
	{"status",	"output useful status information", cmStatus},
	CMD_DEF_VERSION,
	{"?", 		(char *)0, cmd_help},
};

CMDSET CSLabc;


/* setup to do all the nifty cm commands				(ksb)
 */
int
after_func()
{
	if (0 != geteuid()) {
		fprintf(stderr, "%s: %s, su to run me\n", progname, pcGuilty);
		exit(1);
	}

        (void)signal(SIGPIPE, SIG_IGN);
	fdSock = GetPriPort();
	if (!FindLab(pcLabdHost)) {
		exit(1);
	}
}
%%
