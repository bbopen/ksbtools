#!mkcmd
# $Id: labd.m,v 3.28 1997/11/14 22:37:09 ksb Exp $

from '<sys/types.h>'
from '<sys/param.h>'
from '<sys/mman.h>'
from '<sys/file.h>'
from '<sys/stat.h>'
from '<sys/socket.h>'
from '<sys/time.h>'
from '<netinet/in.h>'
from '<signal.h>'
from '<string.h>'
from '<errno.h>'
from '<netdb.h>'
from '<utmp.h>'
from '<nlist.h>'
from '<stdio.h>'
from '<ctype.h>'
from '<pwd.h>'

require "std_help.m" "std_version.m"

augment action 'V' {
	user "VersionInfo();"
}

from '"machine.h"'
from '"daemon.h"'
from '"main.h"'

basename "labd" ""

boolean 'd' {
	named "fDebug"
	init "0"
	help "run in debug mode, implies -n"
	user "%rnn = !%rni;"
}

boolean 'N' {
	named "fForSale"
	init '1'
	help "never grant rlogin service tickets"
}

long 's' {
	named "iSleepSeconds"
	param "secs"
	init "140"
	help "time to sleep between checking load/users/mounts"
}

long 'm' {
	named "iMaxQuietSleeps"
	param "nsleeps"
	init "3"
	help "maximum number of sleeps between reports"
}

long 't' {
	named "iLoadThreshholdPct"
	param "percent"
	init "10"
	help "percent change in load average before reporting"
}

long 'i' {
	named "iIgnoreIdle"
	param "mins"
	init "600"
	help "ignore users more than 'mins' minutes idle"
	user "%n *= 60;"
}

boolean 'u' {
	named "fAllUsers"
	init '0'
	help "show all users instead of just console user"
}

boolean 'n' {
	named "fExec"
	init "1"
	help "don't daemonize on startup"
}

char* 'X' {
	named "pcXdmHackFile"
	init '(char *)0'
	param "path"
	help "with XDM: name of a file given by GiveConsole"
}

string[256] variable "acServerHost" {
	param "server-host"
	init '"feinfo.cc.purdue.edu"'
	help "machine running the labwatch"
}

int variable "iServerPort" {
	param "server-port"
	after "%n = LookupServPort();"
	help "port for load reports"
}

left "acServerHost" [ "iServerPort" ] {
}

list {
	named "Loadmon"
	update "%n(argc, argv);"
	param ""
	help ""
}

# some options for debugging
char* 'T' {
	hidden named "pcUtmp"
	init 'UTMP_FILE'
	help "name of utmp file"
}

char* 'K' {
	hidden named "pcKmem"
	init '"/dev/kmem"'
	help "name of kmem device"
}

char* 'U' {
	hidden named "pcUnix"
	init 'VMUNIX'
	help "name of the running kernel"
}

long 'A' {
	hidden named "iAckPort"
	param "port"
	init "29047"
	help "receive acks on port"
}

%h
typedef struct utmp UTMP;

#define LEAP		5		/* round value for fall forward */
%%

%i
static char *rcsid =
	"$Id: labd.m,v 3.28 1997/11/14 22:37:09 ksb Exp $";

#if USE_AVERUNNABLE
#include <sys/resource.h>
#endif

#if HAVE_SELECT_H
#include <sys/select.h>
#endif

#if USE_LINUX_SYSINFO
#include <linux/kernel.h>
#include <linux/sys.h>
#endif

#if USE_SYSINFO
#include <sys/sysinfo.h>
#endif

#if !defined(MAX_UNIQUE_USERS)
#define MAX_UNIQUE_USERS	25
#endif

struct nlist nl[] = {
#if USE_SYSINFO
	{ "sysinfo" },
#define SYSINFO		0
#else

#if defined(NAME_AVENRUN)
	{ NAME_AVENRUN },
#else
#if USE_ELF || !NEED_UNDERBAR
	{ "avenrun" },
#else
	{ "_avenrun" },
#endif
#endif
#define AVENRUN		0

#endif

#if HAS_KBOOTTIME
#if USE_ELF || !NEED_UNDERBAR
	{ "boottime" },
#else
	{ "_boottime" },
#endif
#define BOOTTIME	1
#endif
	NLIST_SENTINAL_VALUE
};

#if HAS_GETTTYENT
#include <ttyent.h>
#endif
#if !HAVE_GETDTABLESIZE
#include <sys/resource.h>
#endif

#if MACH_HEADERS
#include <mach/mach.h>
#define N_NAME	n_un.n_name
#else
#define N_NAME	n_name
#endif
%%

%c
static char acService[] = LABLOG_SERVICE;

static void VersionInfo()
{
	register struct servent *pSE;

	if ((struct servent *)0 == (pSE = getservbyname(acService, "udp"))) {
		printf("%s: service \"%s\" not installed\n", progname, acService);
	} else {
		printf("%s: report to labwatch on \"%s\"", progname, pSE->s_name);
		if (0 != strcmp(acService, pSE->s_name)) {
			printf(" (we call it \"%s\")", acService);
		}
		printf(" on port %d/udp\n", ntohs(pSE->s_port));
	}
	printf("%s: acks on port %d/udp\n", progname, iAckPort);
}

#if ! HAS_KBOOTTIME
static void GetWBoot(time_t *ptBuf)
{
	register FILE *fpWTmp;
	auto struct utmp utThis;

	if ((FILE *)0 == (fpWTmp = fopen(BOOTTIME_FILE, "r"))) {
		time(ptBuf);
	}
	while (1 == fread(& utThis, sizeof(utThis), 1, fpWTmp)) {
		if (BOOT_TIME != utThis.ut_type)
			continue;
		*ptBuf = utThis.ut_time;
	}
	fclose(fpWTmp);
}
#endif

static short int LookupServPort()
{
	register struct servent *pSE;

	if ((struct servent *)0 == (pSE = getservbyname(acService, "udp"))) {
		fprintf(stderr, "%s: getservbyname: %s: %s\n", progname, acService, strerror(errno));
		exit(1);
	}
	return ntohs(pSE->s_port);
}

int fdSock;  /* SigFunc must be able to close our listener */

static SIGRET_T SigFunc(int iSig)
{
	if (fDebug) {
		printf("%s: going down on signal %d...\n", progname, iSig);
	}
	close(fdSock);
	exit(1);
}

int fForce = 0;

SIGRET_T Force(int iSig)
{
	fForce = 1;
#if !HAVE_STICKY_SIGNALS
	signal(SIGALRM, Force);
#endif
}

static void Loadmon(int argc, char **argv)
{
	register int fdKmem, fdUtmp;
	register int iQuietSleeps, fFirst, iLen, iMissingAcks;
	register int usRand, iDelta, i, k, fForSaleOrig, fSawConsole;
	register char *pcTemp;
	register int iUtmpEntries, iMaxUtmpEntries, iUniqs;
	register UTMP *pUutmp;
	register struct sockaddr *pSINout;
	char aacUniqUsers[MAX_UNIQUE_USERS][8];
	struct stat statbuf;
#if USE_SYSINFO || USE_LINUX_SYSINFO
	auto struct sysinfo wSysinfo;
#endif
	AVENRUN_T avenrun[3], oldavenrun[3];
	char acReport[MAXUDPSIZE], *pcReport;
	struct sockaddr_in SINsend, SINrecv;
	struct hostent *pHEent;
	struct timeval timeout, savetime;
#if TIME_T_BOOTTIME
	auto time_t boottime;
#else
	auto struct timeval boottime;
#endif
	auto struct fd_set FDSread;
	auto time_t tBoottime, tNow, tShutDown;
	time_t tLastReport, tConsoleLogin, tOldUtmpMtime;
	auto int iJunk;

	if (fDebug) {
		printf("%s: starting up in debug mode, pid %d\n", progname, getpid());
	}

	if ((struct hostent *)0 == (pHEent = gethostbyname(acServerHost)) || (char *)0 == pHEent->h_name) {
		fprintf(stderr, "%s: gethostbyname: %s: %s\n", progname, acServerHost, hstrerror(h_errno));
		exit(1);
	}
	if (fDebug) {
		printf("%s: sending to host %s, port %hd\n", progname, pHEent->h_name, iServerPort);
	}

	/* chdir to / to avoid locking any automounted filesystems that
	 * someone was so foolish as to start us from
	 */
	if (-1 == chdir("/")) {
		fprintf(stderr, "%s: chdir /: %s  (you're hosed!)\n", progname, strerror(errno));
		exit(1);
	}

	if (-1 == (fdSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) {
		fprintf(stderr, "%s: socket: %s\n", progname, strerror(errno));
		exit(1);
	}
	SINsend.sin_family = AF_INET;
	SINsend.sin_port = htons(iServerPort);
#if USE_STRINGS
	bcopy(pHEent->h_addr, &SINsend.sin_addr.s_addr, pHEent->h_length);
#else
	memcpy(&SINsend.sin_addr.s_addr, pHEent->h_addr, pHEent->h_length);
#endif

	SINrecv.sin_family = AF_INET;
	SINrecv.sin_addr.s_addr = htonl(INADDR_ANY);
	SINrecv.sin_port = htons(iAckPort);
	if (-1 == bind(fdSock, (struct sockaddr *)&SINrecv, sizeof (SINrecv))) {
		fprintf(stderr, "%s: bind: %s\n", progname, strerror(errno));
		exit(1);
	}

	if (-1 == (fdKmem = open(pcKmem, 0, 0))) {
		fprintf(stderr, "%s: open: %s: %s\n", progname, pcKmem, strerror(errno));
		exit(1);
	}


#if USE_LINUX_SYSINFO
	if (-1 == sysinfo(& wSysinfo)) {
		fprintf(stderr, "%s: sysinfo: %s\n", progname, strerror(errno));
		exit(1);
	}

	/* now - the seconds up is the boot time, right?
	 */
	time(& tBoottime);
	tBoottime -= wSysinfo.uptime;
#else
#if HAVE_KNLIST
	if (-1 == knlist(nl, sizeof(nl)/sizeof(struct nlist), sizeof(struct nlist))) {
		fprintf(stderr, "%s: %s: knlist failed\n", progname, pcUnix);
		exit(1);
	}
#else
	if (-1 == nlist(pcUnix, nl)) {
		fprintf(stderr, "%s: %s: nlist failed\n", progname, pcUnix);
		exit(1);
	}
#endif
	if (0 == nl[0].n_value) {
		fprintf(stderr, "%s: %s: %s: no name list\n", progname, pcUnix, nl[0].N_NAME);
		exit(1);
	}

	/* find the boot time (it's constant, you know)
	 */
#if HAS_KBOOTTIME
	lseek(fdKmem, (long)nl[BOOTTIME].n_value, SEEK_SET);
	read(fdKmem, &boottime, sizeof(boottime));
#if TIME_T_BOOTTIME
	tBoottime = boottime;
#else
	tBoottime = (time_t) boottime.tv_sec;
#endif
#else
	GetWBoot(& tBoottime);
#endif

#endif /* sysinfo or read syms from kernel */

	if (-1 == (fdUtmp = open(pcUtmp, 0, 0))) {
		fprintf(stderr, "%s: open: %s: %s\n", progname, pcUtmp, strerror(errno));
		exit(1);
	}
#if HAS_GETTTYENT
	setttyent();
	iMaxUtmpEntries = 0;
	while ((struct ttyent *)0 != getttyent())
		++iMaxUtmpEntries;
	endttyent();
#else
	iMaxUtmpEntries = 50;
#endif
#if USE_MMAP
# if !defined(MAP_FILE)
# define MAP_FILE	0
# endif
	if ((UTMP *)-1 == (pUutmp = (UTMP *)mmap(0, iMaxUtmpEntries * sizeof(UTMP), PROT_READ, MAP_FILE|MAP_SHARED, fdUtmp, 0))) {
		fprintf(stderr, "%s: mmap: %s (%d entries): %s\n", progname, pcUtmp, iMaxUtmpEntries, strerror(errno));
		exit(1);
	}
#else
	if ((UTMP *)0 == (pUutmp = (UTMP *)malloc(iMaxUtmpEntries * sizeof(UTMP)))) {
		fprintf(stderr, "%s: malloc(%d): Out of memory\n", progname, iMaxUtmpEntries * sizeof(UTMP));
		exit(1);
	}
#endif

	FD_ZERO(&FDSread);
	FD_SET(fdSock, &FDSread);
	FD_SET(fdKmem, &FDSread);
	FD_SET(fdUtmp, &FDSread);
        if (!fExec) {
                FD_SET(0, &FDSread);
                FD_SET(1, &FDSread);
                FD_SET(2, &FDSread);
        }
	signal(SIGALRM, Force);
        signal(SIGPIPE, SIG_IGN);
	daemon(SigFunc, &FDSread, fExec);

	/* init our auto variables
	 */

	{
		auto struct timeval tv;
#if GETTIMEOFDAY_BROKEN
		(void)gettimeofday(& tv, 0);
#else
		(void)gettimeofday(& tv);
#endif
		usRand = tv.tv_usec & ((1<<21) - 1);
	}

	iQuietSleeps = 0;
	iDelta = iMissingAcks = 0;
	fForce = 0;
	for (i = 0; i < 3; ++i)
		oldavenrun[i] = 0;
	timeout.tv_sec = 0;
	timeout.tv_usec = usRand;

	fForSaleOrig = fForSale;
	tShutDown = 0;

	/* go into our report loop
	 */
	for (;;) {
		if (fDebug) {
			fflush(stdout);
		}

		FD_ZERO(&FDSread);
		FD_SET(fdSock, &FDSread);

select_again:
		switch (select(fdSock+1, &FDSread, (fd_set *)0, (fd_set *)0, &timeout)) {
		case -1:
			if (EINTR == errno) {
				if (fForce) {
					pSINout = (struct sockaddr *)&SINsend;
					break;
				}
				goto select_again;
			}
			fprintf(stderr, "%s: select: %d: %s\n", progname, fdSock, strerror(errno));
			exit(1);
			/* NOT REACHED */

		default:
			/* momma is calling!
			 */
			iJunk = sizeof(SINrecv);

			while (-1 == (iLen = recvfrom(fdSock, acReport, sizeof(acReport), 0, (struct sockaddr *)&SINrecv, &iJunk)) && EINTR == errno)
				/* NO STATEMENT */;
			if (-1 == iLen) {
				if (! fExec) {
					fprintf(stderr, "%s: recvfrom: %s\n", progname, strerror(errno));
				}
				exit(1);
			}

			if (fDebug) {
				printf("%s: receceived an%s ack: %s\n", progname, (ntohs(SINrecv.sin_port) < 1024) ? " authoritative" : "", acReport);
			}

			/* to prevent tampering, we ignore stuff from
			 * non-reserved ports
			 */
			if (!fDebug && ntohs(SINrecv.sin_port) >= 1024) {
				/* syslog this? */
				continue;
			}

			pSINout = (struct sockaddr *)&SINrecv;
			iMissingAcks = 0;
			iDelta /= 2;

			/* reset our timeout to help spread us out (since
			 * momma is ack'ing us as fast as she's able to handle
			 * input)
			 */
			savetime = timeout;
			timeout.tv_sec = iSleepSeconds - iDelta;
			timeout.tv_usec = usRand;

			switch (acReport[0]) {
			case 'a':
				/* just saying hi! */
				continue;
			case '!':
				/* send a packet but don't change the timeout
				 */
				timeout = savetime;
				/* FALL THROUGH */
			case '?':
				/* force a packet */
				fForce = 1;
				{	struct timeval tm_i_need_usleep;
					tm_i_need_usleep.tv_sec = 0;
					tm_i_need_usleep.tv_usec = usRand;
					select(0, 0, 0, 0, &tm_i_need_usleep);
				}
				break;
			case 'q':
				SigFunc(1);
				/* NOT REACHED */
			case 'x': /* we are obsolete. */
				/* this causes coredumps over NFS
				 * :( -- bj  XXX
				 */
				close(fdSock);
				execvp(argv[0], argv);
				/* NOT REACHED */
			case '=':
				iDelta = 0;
				iSleepSeconds = atoi(acReport + 1);
				timeout.tv_sec = iSleepSeconds;
				continue;
			case '+':
				/* momma gives us a push (labc stagger)
				 * don't wander off station once set
				 */
				timeout.tv_sec += atoi(acReport + 1);
				usRand = 0;
				continue;
			case '-':
				iDelta = atoi(acReport + 1);
				if (iDelta < 0) {
					if (iDelta < - iSleepSeconds / 2) {
						iDelta = - iSleepSeconds / 2;
					}
				} else if (iDelta > iSleepSeconds / 2) {
					iDelta = iSleepSeconds / 2;
				}
				timeout.tv_sec = iSleepSeconds - iDelta;
				continue;
			case '>':
				fForce = 1;
				/* redirect to some other host
				 * default is to host that called us
				 * we can tell if it took by the forced report
				 */
				if ('\000' == acReport[1]) {
					register unsigned char *puc;

					SINsend.sin_addr = SINrecv.sin_addr;
					SINsend.sin_port = SINrecv.sin_port;
					iServerPort = ntohs(SINsend.sin_port);
					puc = (unsigned char *)& SINrecv.sin_addr;
					(void)sprintf(acServerHost, "%d.%d.%d.%d", puc[0], puc[1], puc[2], puc[3]);
					break;
				}
				if ((char *)0 != (pcTemp = strchr(acReport + 1, ':'))) {
					*pcTemp = '\000';
					/* LLL decimal! */
					iServerPort = atoi(pcTemp + 1);
				}
				if ((struct hostent *)0 == (pHEent = gethostbyname(acReport + 1)) || (char *)0 == pHEent->h_name) {
					auto char acBroken[MAXUDPSIZE];
					sprintf(acBroken, "#failed %s: %s", acReport + 1, hstrerror(h_errno));
					iLen = strlen(acBroken) + 1;
					(void) sendto(fdSock, acBroken, iLen, 0, (struct sockaddr *)&SINrecv, sizeof(struct sockaddr_in));
					fForce = 0;
					continue;
				}
				SINsend.sin_port = htons(iServerPort);
#if USE_STRINGS
				bcopy(pHEent->h_addr, &SINsend.sin_addr.s_addr, pHEent->h_length);
#else
				memcpy(&SINsend.sin_addr.s_addr, pHEent->h_addr, pHEent->h_length);
#endif
				(void)strcpy(acServerHost, pHEent->h_name);
				/* third party mode, ack to the labc
				 */
				sprintf(acReport, "#ok %s:%x", acServerHost, iServerPort);
				iLen = strlen(acReport) + 1;
				(void) sendto(fdSock, acReport, iLen, 0, (struct sockaddr *)&SINrecv, sizeof(struct sockaddr_in));

				/* and say hi to out new mother
				 */
				pSINout = (struct sockaddr *)&SINsend;
				break;
			case 'N':
			case 'Y':
				fForce = 1;
				fForSale = 'N' == acReport[0] ? 0 : fForSaleOrig;
				pSINout = (struct sockaddr *)&SINsend;
				if (*(unsigned int *)&SINsend.sin_addr == *(unsigned int *)&SINrecv.sin_addr && SINsend.sin_port == SINrecv.sin_port)
					break;
				acReport[0] = fForSale ? '+' : '-';
				acReport[1] = '\000';
				(void) sendto(fdSock, acReport, 2, 0, (struct sockaddr *)&SINrecv, sizeof(struct sockaddr_in));
				break;
			case 'S':
				/* this machine is shutting down --
				 * start marking packets to indicate
				 * this.  only accept such acks from
				 * localhost. (?)
				 * syslog this? LLL
				 */
				if (INADDR_LOOPBACK != ntohl(SINrecv.sin_addr.s_addr)) {
					static char acNo[] = "#refused shutdown";
					(void)sendto(fdSock, acNo, sizeof(acNo), 0, (struct sockaddr *)&SINrecv, sizeof(struct sockaddr_in));
					continue;
				}
				/* we don't want to let people start jobs
				 * so we turn off tickets too
				 */
				time(& tShutDown);
				tShutDown += 60 * atoi(acReport + 1);
				fForSale = 0;

				/* for an update to mother, but ack labc
				 */
				fForce = 1;
				pSINout = (struct sockaddr *)&SINsend;
				if (*(unsigned int *)&SINsend.sin_addr != *(unsigned int *)&SINrecv.sin_addr || SINsend.sin_port != SINrecv.sin_port) {
					static char acOk[] = "#ok";
					(void)sendto(fdSock, acOk, sizeof(acOk), 0, (struct sockaddr *)&SINrecv, sizeof(struct sockaddr_in));
				}
				break;
			case '#':	/* labc doesn't need this */
				/* this should be from a client program running
				 * on localhost asking us to forward a
				 * comment line to the labwatch.
				 */
				if (INADDR_LOOPBACK != ntohl(SINrecv.sin_addr.s_addr)) {
					/* syslog this? */
					continue;
				}
				if (fDebug) {
					printf("%s: broadcasting a comment:\n%s", progname, acReport);
				}
				iLen = strlen(acReport) + 1;
				(void) sendto(fdSock, acReport, iLen, 0, (struct sockaddr *)&SINsend, sizeof(struct sockaddr_in));
				continue;
			case '@':
				/* report `who we report to'
				 */
				sprintf(acReport, "%s:%x", acServerHost, iServerPort);
				iLen = strlen(acReport) + 1;
				(void) sendto(fdSock, acReport, iLen, 0, (struct sockaddr *)&SINrecv, sizeof(struct sockaddr_in));
				continue;
			case 't':
				/* report sleep time and max quiet sleeps
				 * and idle thresh
				 */
				sprintf(acReport, "%x*%x", iMaxQuietSleeps, iSleepSeconds);
				iLen = strlen(acReport) + 1;
				(void) sendto(fdSock, acReport, iLen, 0, (struct sockaddr *)&SINrecv, sizeof(struct sockaddr_in));
				continue;
			default:
				/* LLL should be one to fetch iIgnoreIdle
				 */
				continue;
			}
			/* break here for the '?' option above
			 */
			break;
		case 0:
			/* we timed out, so do our thang
			 */
			timeout.tv_sec = iSleepSeconds - iDelta;
			timeout.tv_usec = usRand;
			pSINout = (struct sockaddr *)&SINsend;
			break;
		}
		++iQuietSleeps;

		/* read the load average
		 */
#if USE_LINUX_SYSINFO
		if (-1 == sysinfo(& wSysinfo)) {
			fprintf(stderr, "%s: sysinfo: %s\n", progname, strerror(errno));
			exit(1);
		}
		avenrun[0] = wSysinfo.loads[0];
		avenrun[1] = wSysinfo.loads[1];
		avenrun[2] = wSysinfo.loads[2];
#else
#if USE_AVERUNNABLE
		lseek(fdKmem, (long)nl[AVENRUN].n_value, SEEK_SET);
		{
			auto struct loadavg averunnable;
			read(fdKmem, &averunnable, sizeof(averunnable));
			for (i = 0; i < 3; ++i)
				avenrun[i] = averunnable.ldavg[i];
		}
#else
		lseek(fdKmem, (caddr_t)nl[AVENRUN].n_value, SEEK_SET);
		read(fdKmem, avenrun, sizeof(avenrun));
#endif
#endif
#if defined(ADJ_FSCALE)
/* this is silly, FSCALE is wrong! NEXTS, S81 */
		for (i = 0; i < 3; i++)
			avenrun[i] /= ADJ_FSCALE;
#endif
		if (fDebug) {
			double fLoads[3];

			for (i = 0; i < 3; i++)
				fLoads[i] = (double)avenrun[i] / FSCALE;
			printf("%s: load is %4.2lf %4.2lf %4.2lf\n", progname, fLoads[0], fLoads[1], fLoads[2]);
		}
		/* stat the utmp file to see if users have logged in/out.
		 * (we only scan the thing if we fall into 'generate report')
		 */
		if (-1 == fstat(fdUtmp, &statbuf)) {
			fprintf(stderr, "%s: fstat: %s: %s\n", progname, pcUtmp, strerror(errno));
			exit(1);
		}
		iUtmpEntries = statbuf.st_size / sizeof(UTMP);

		/* We report if:
		 * we're forced to or
		 * we've missed the last ACK or utmp changed or
		 * we *have* to be marked alive, or the load is
		 * interesting (someone raised it).
		 * otherwise don't even look at the other info...
		 */
		if (fForce) {
			fForce = 0;
		} else if (0 == iMissingAcks && tOldUtmpMtime == statbuf.st_mtime && iQuietSleeps < iMaxQuietSleeps && (0 != oldavenrun[0] ? ((abs(avenrun[0] - oldavenrun[0]) * 100) / oldavenrun[0] < iLoadThreshholdPct) : 0 == avenrun[0])) {
			continue;
		}
		tOldUtmpMtime = statbuf.st_mtime;

		/* build a report
		 */
		++iMissingAcks;  /* we haven't gotten an ack for this one */
		if ((iDelta += LEAP) > iSleepSeconds / 2) {
			iDelta = iSleepSeconds / 2;
		}

		/* timestamp the report with now and our boot time.
		 */
		tLastReport = time(&tNow);
		sprintf(acReport, "%c%x:%x\n", fForSale ? '+' : '-', tNow, iMissingAcks - 1);
		pcReport = acReport + strlen(acReport);
		sprintf(pcReport, "b:%x\n", tBoottime);
		pcReport += strlen(pcReport);

		/* are we on the way down?
		 * xmit number of seconds left before we croak
		 */
		if (0 != tShutDown) {
			if (tNow > tShutDown) {
				tShutDown = 0;
			} else {
				sprintf(pcReport, "d:%x\n", tShutDown - tNow);
				pcReport += strlen(pcReport);
			}
		}
		/* record the loadavg in the broadcast packet
		 */
		sprintf(pcReport, "l:%x %x %x\n", (int)(avenrun[0]*XMIT_SCALE/FSCALE), (int)(avenrun[1]*XMIT_SCALE/FSCALE), (int)(avenrun[2]*XMIT_SCALE/FSCALE));
		pcReport += strlen(pcReport);

		/* find out who is connected (utmp)
		 * we recompued iUtmpEntries above (while testing mtime)
		 */
#if !USE_MMAP
		if (-1 == lseek(fdUtmp, 0, SEEK_SET)) {
			fprintf(stderr, "%s: lseek: %s: %s\n", progname, pcUtmp, strerror(errno));
			exit(1);
		}
		if (-1 == read(fdUtmp, pUutmp, iUtmpEntries * sizeof(UTMP))) {
			fprintf(stderr, "%s: read: %s: %s\n", progname, pcUtmp, strerror(errno));
			exit(1);
		}
#endif
		for (fFirst = 1, fSawConsole = iUniqs = iLen = i = 0; i < iUtmpEntries; ++i) {
			register int fConsole, fIdle;
			auto char acTtyFile[20];

			if ('\000' == pUutmp[i].ut_name[0] || nonuser(pUutmp[i]))
				continue;

			sprintf(acTtyFile, "/dev/%.8s", pUutmp[i].ut_line); /* XXX size */
			if (-1 == stat(acTtyFile, &statbuf)) {
				continue;
			}
			fConsole = !strncmp(pUutmp[i].ut_line, NAME_CONSOLE, 8);
			/* we assume that the console is the first real
			 * tty.  if it's not the report will be fried.  XXX
			 */
			if (fConsole) {
				sprintf(pcReport, "c:%.8s:%x\n", pUutmp[i].ut_name, pUutmp[i].ut_time);
				pcReport += strlen(pcReport);
				fSawConsole = 1;
			}
			for (k = 0; k < iUniqs; ++k) {
				if (0 == strncmp(aacUniqUsers[k], pUutmp[i].ut_name, 8)) {
					break;
				}
			}
			/* we should keep the *count* correctly ZZZ
			 */
			if (k == iUniqs && iUniqs < MAX_UNIQUE_USERS) {
				strncpy(aacUniqUsers[iUniqs++], pUutmp[i].ut_name, 8);
			}
			++iLen;
			if (!fAllUsers) {
				continue;
			}
			if (fFirst) {
				strcat(pcReport, "w:");
				pcReport += strlen(pcReport);
				fFirst = 0;
			}
			fIdle = tNow - statbuf.st_atime > iIgnoreIdle;
			sprintf(pcReport, fIdle ? "-%.8s ":"%.8s ",pUutmp[i].ut_name);
			pcReport += strlen(pcReport);
		}
		/* replace trailing space with NL */
		if (!fFirst) {
			pcReport[-1] = '\n';
		}
		sprintf(pcReport, "u:%x:%x\n", iLen, iUniqs);
		pcReport += strlen(pcReport);

		/* if we run xdm, there might not be a user on "/dev/console"
		 * even if someone is logged on to the console of the machine.
		 * we check some device (presumably chown'd by GiveConsole)
		 * and use the owner as the console user and the ctime as the
		 * login time (the ctime will reflect the last chown time).
		 * if it doesn't exist or root is the owner, we ignore it.(XXX?)
		 */
		if (!fSawConsole && (char *)0 != pcXdmHackFile && -1 != stat(pcXdmHackFile, &statbuf) && 0 != statbuf.st_uid) {
			static int iLastUid;
			static char acLogName[33];	/* XXX size */

			if (iLastUid != statbuf.st_uid) {
				struct passwd *pwd;

				iLastUid = statbuf.st_uid;
				if ((struct passwd *)0 != (pwd = getpwuid(iLastUid))) {
					strcpy(acLogName, pwd->pw_name);
				} else {
					sprintf(acLogName, "uid%d", iLastUid);
				}
			}
			sprintf(pcReport, "c:%.8s:%x\n", acLogName, statbuf.st_ctime);
			pcReport += strlen(pcReport);
		}

		/* this is more than likely the most expensive
		 * thing we run but it helps check to see that the
		 * homedirs of the students are balanced in lab
		 */
		GetMntFs(pcReport);
		pcReport += strlen(pcReport);
		*pcReport++ = '\n';
		*pcReport++ = '\000';

		/* broadcast the status report
		 */
		if (fDebug) {
			printf("%s: broadcasting a status report:\n%s", progname, acReport);
		}
		iLen = strlen(acReport) + 1;
		if (iLen != sendto(fdSock, acReport, iLen, 0, pSINout, sizeof(struct sockaddr_in))) {
			if (! fExec) {
				fprintf(stderr, "%s: sendto: %s\n", progname, strerror(errno));
			}
			exit(1);
		}

		/* get ready for next time
		 */
		iQuietSleeps = 0;
		for (i = 0; i < 3; ++i)
			oldavenrun[i] = avenrun[i];
	}
}
%%
