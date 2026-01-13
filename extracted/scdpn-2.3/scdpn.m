#!mkcmd
# $Id: scdpn.m,v 2.3 2012/01/31 21:21:47 ksb Exp $
# sniff cdp packets and come up with an interface.cl line	(petef,ksb,csg)
# ref: http://www.cisco.com/univercd/cc/td/doc/product/lan/trsrb/frames.htm
# 	$Compile(*): %b %o -mMkcmd %f && %b %o -d%s prog.c && mv -f prog %F
comment	"%kCompile(*): $%{cc-cc%} -Wall -o %%F %%f -lpcap -g -D%%s"
# 	$Mkcmd(*): ${mkcmd-mkcmd} %f

from '<stdio.h>'
from '<stdlib.h>'
from '<unistd.h>'
from '<errno.h>'
from '<signal.h>'
from '<setjmp.h>'
from '<string.h>'
from '<ctype.h>'
from '<sys/types.h>'
from '<sys/param.h>'
from '<sys/socket.h>'
from '<sys/wait.h>'
from '<sys/ioctl.h>'
from '<sysexits.h>'
from '"machine.h"'

basename "scdpn" ""
require "std_help.m" "std_version.m"

augment action 'V' {
	user "Version();"
}

%i
static char rcsid[] =
	"$Id: scdpn.m,v 2.3 2012/01/31 21:21:47 ksb Exp $";
#if defined(HPUX11)
#include "pcap-hpux.h"
#endif
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netdb.h>
#include <pcap.h>
%%

init 4 "Init();"

boolean 'v' {
	named "fVerbose"
	help "give verbose information about each cdp packet"
}
int 't' {
	named "iTimeout"
	init "600"
	param "seconds"
	help "time to wait for a CDP packet"
}

string["MAXHOSTNAMELEN"] 'N' {
	named "acHostname"
	help "hostname to report as"
	before "InitHost(%n);"
	param "name"
}

list {
	named "LinuxBug"
	update "%n(%#, %@);"
	hidden param "hack"
	help "this works around a Linux kernel bug"
}
every {
	named "SniffCDP"
	param "interfaces"
	help "list of interfaces to sniff CDP packets on"
}

zero {
	update "AcrossIfs(SniffCDP);"
}

%c
#define EXTRACT_16BITS(p) \
	((u_int16_t)*((const u_int8_t *)(p) + 0) << 8 | \
	(u_int16_t)*((const u_int8_t *)(p) + 1))

#define EXTRACT_32BITS(p) \
	((u_int32_t)((u_int32_t)*((const u_int8_t *)(p) + 0) << 24 | \
		(u_int32_t)*((const u_int8_t *)(p) + 1) << 16 | \
		(u_int32_t)*((const u_int8_t *)(p) + 2) << 8 | \
		(u_int32_t)*((const u_int8_t *)(p) + 3)))

#if !defined(CDP_EXPR)
#define CDP_EXPR "ether dst 01:00:0c:cc:cc:cc and ether[14:4] = 0xaaaa0300 and ether[18:4] = 0x000c2000"
#endif

static char *pcCdpExpr = CDP_EXPR,
	acErrbuf[PCAP_ERRBUF_SIZE],
	acSwitch[MAXHOSTNAMELEN],
	acPort[128],
	acVLAN[(sizeof("vlan65536")|7)+1];
pcap_if_t *ppiInt;

/* handle an alarm if we don't get a cdp packet in time			(petef)
 */
static jmp_buf jmpbuf;
static void
alarmhandler()
{
	if (fVerbose)
		fprintf(stderr, "# cdp sniff timed out\n");
	longjmp(jmpbuf, 1);
}

/* initialize our hostname						(petef)
 * take common endings off the hostname
 */
char *
InitHost(pcThis)
char *pcThis;
{
	char *pcCursor;

	if (-1 == gethostname(pcThis, MAXHOSTNAMELEN)) {
		fprintf(stderr, "%s: gethostname: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}

	pcCursor = strstr(pcThis, ".fedex.com");
	if ((char *)0 != pcCursor && '\000' == pcCursor[10])
		*pcCursor = '\000';
	pcCursor = strstr(pcThis, ".sac");
	if ((char *)0 != pcCursor && '\000' == pcCursor[4])
		*pcCursor = '\000';

	return pcThis;
}

/* initialize global data we reuse for each interface			(petef)
 */
static void
Init()
{
	if (-1 == pcap_findalldevs(&ppiInt, acErrbuf)) {
		fprintf(stderr, "%s: pcap_findalldevs: %s\n", progname, acErrbuf);
		exit(EX_UNAVAILABLE);
	}
}

/* Run some function over every interface not in the ignore list	(ksb)
 */
static char acIgnore[] = "any,lo,lo0,";
static int
AcrossIfs(int (*pfiRun)(char *))
{
	register pcap_if_t *ppiScan;
	register char *pcFound;
	register int iRet;

	iRet = 0;
	for (ppiScan = ppiInt; (pcap_if_t *)0 != ppiScan; ppiScan = ppiScan->next) {
		if ((char *)0 != (pcFound = strstr(acIgnore, ppiScan->name)) && ',' == pcFound[strlen(ppiScan->name)] && (acIgnore == pcFound || ',' == pcFound[-1])) {
			continue;
		}
		iRet += pfiRun(ppiScan->name);
	}
	return iRet;
}

/* output the list of interfaces we are going to scan			(ksb)
 */
static int
IfList(char *pcIf)
{
	static char acFirst[] = "interfaces found: ";
	static char acOther[] = ",";
	static char *pcSep = acFirst;

	if ((char *)0 == pcIf) {
		pcSep = acFirst;
		return 0;
	}
	printf("%s%s", pcSep, pcIf);
	pcSep = acOther;
	return 1;
}

/* dump out tunes for std version output checks				(ksb)
 */
static void
Version()
{
	printf("%s: ignore list: %.*s\n", progname, sizeof(acIgnore)-2, acIgnore);
	printf("%s: ", progname);
	if (0 == AcrossIfs(IfList)) {
		printf("no interfaces found");
	}
	printf("\n");
	IfList((char *)0);	/* reset the interface list machine */
}

/* IP u_long -> string in malloc'd space				(petef)
 */
static char *
iptos(ulIP)
u_long ulIP;
{
	char *pcRet;
	u_char *pucIP;

	pucIP = (u_char *)&ulIP;
	pcRet = (char *)calloc(32, sizeof(char));
	sprintf(pcRet, "%d.%d.%d.%d", pucIP[0], pucIP[1], pucIP[2], pucIP[3]);
	return pcRet;
}


/* given a device, get the primary IP address				(petef)
 */
static char *
GetIP(pcDev)
char *pcDev;
{
	char *pcRet = "?";
	pcap_addr_t *ppaAddr;
	struct sockaddr_in *s;
	pcap_if_t *ppiCursor;

	ppiCursor = ppiInt;
	for(; ppiCursor; ppiCursor = ppiCursor->next) {
		if (0 != strcmp(ppiCursor->name, pcDev))
			continue;
		for(ppaAddr = ppiCursor->addresses; ppaAddr; ppaAddr = ppaAddr->next) {
			switch(ppaAddr->addr->sa_family) {
			case AF_INET:
				s = (struct sockaddr_in *)ppaAddr->addr;
				pcRet = iptos(s->sin_addr.s_addr);
				break;
			}
			if (0 != strcmp(pcRet, "?"))
				break;
		}
	}
	return pcRet;
}

/* print addresses from a CDP packet					(petef)
 */
static void
PrintAddr(pucAddr, iAddrLen)
int iAddrLen;
u_char *pucAddr;
{
	int iLength, iProtoType, iProtoLength;
	u_char *pucCursor;

	pucCursor = pucAddr;
	iLength = EXTRACT_32BITS(pucCursor);
	pucCursor += 4;

	while(pucCursor < (pucAddr + iAddrLen)) {
		iProtoType = pucCursor[0];
		iProtoLength = pucCursor[1];
		pucCursor += 2;
		iLength = EXTRACT_16BITS(&pucCursor[iProtoLength]);
		/* IPv4 address? */
		if (1 == iProtoType && 1 == iProtoLength && *pucCursor == 0xcc && 4 == iLength) {
			pucCursor += 3;
			printf("# CDP Device Address: %u.%u.%u.%u\n", pucCursor[0], pucCursor[1], pucCursor[2], pucCursor[3]);
			pucCursor += 34;
		} else {
			pucCursor += 2 + iLength;
		}
	}
}


/* callback function for pcap_loop when we get a CDP packet		(petef)
 */
static void
ParseCDP(pucUseless, pphHeader, pucPacket)
u_char *pucUseless;
const struct pcap_pkthdr *pphHeader;
const u_char *pucPacket;
{
	register int iLength, iType, iIt, iCh;
	register u_char *pucCursor;

	pucCursor = (u_char *)pucPacket;
	pucCursor += 22; /* ethernet header */
	if (fVerbose) {
		printf("# CDP Packet Version: %u\n", (int)*pucCursor);
		printf("# CDP Packet TTL: %us\n", (int)*(pucCursor+1));
	}
	pucCursor += 4; /* cdp header */

	while (pucCursor < (pucPacket + pphHeader->caplen)) {
		iType = EXTRACT_16BITS(pucCursor);
		iLength = EXTRACT_16BITS(pucCursor+2) - 4;
		pucCursor += 4;

		if (0x0001 == iType) {
			sprintf(acSwitch, "%.*s", iLength, pucCursor);
			if (fVerbose)
				printf("# CDP Device Name: %.*s\n", iLength, pucCursor);
		} else if (0x0002 == iType && fVerbose) {
			PrintAddr(pucCursor, iLength);
		} else if (0x0003 == iType) {
			if (fVerbose)
				printf("# CDP Port Name: %.*s\n", iLength, pucCursor);
			sprintf(acPort, "%.*s", iLength, pucCursor);
		} else if (0x0005 == iType && fVerbose) {
			printf("# CDP Device Version: ");
			for (iIt = 0; iIt < iLength; iIt++) {
				iCh = *(pucCursor + iIt);
				putchar(iCh);
				if ('\n' == iCh)
					printf("#\t");
			}
			putchar('\n');
		} else if (0x0006 == iType && fVerbose) {
			printf("# CDP Platform: %.*s\n", iLength, pucCursor);
		} else if (0x000a == iType) {
			register int iVLAN;
			iVLAN = EXTRACT_16BITS(pucCursor);
			if (fVerbose)
				printf("# CDP Native VLAN ID: %d\n", iVLAN);
			sprintf(acVLAN, "vlan%d", iVLAN);
		} else if (0x000b == iType && fVerbose) {
			printf("# CDP Duplex: %s\n", *(pucCursor) ? "full" : "half");
		}
		pucCursor += iLength;
	}
}

/* Given a device and ip address, get the MAC address -- hard to be	(petef)
 * both clean and portable here.  Don't free our result w/o a compate,
 * as sometimes we return a constant string (acDont).
 */
static char acDont[] = "????.????.????\000";
static char *
GetMac(pcDev, pcIP)
char *pcDev, *pcIP;
{
	register char *pcRet;

	if ((char *)0 == (pcRet = (char *)calloc(sizeof(acDont), sizeof(char))))
		return acDont;
	(void)strcpy(pcRet, acDont);

#if defined(LINUX)
	{
	auto struct ifreq ifr;
	auto int iSockFd;
	register u_char *pucAddr;

	iSockFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	(void)strncpy(ifr.ifr_name, pcDev, sizeof(ifr.ifr_name));
	if (ioctl(iSockFd, SIOCGIFHWADDR, &ifr) < 0) {
		fprintf(stderr, "%s: ioctl: %s\n", progname, strerror(errno));
		return acDont;
	}
	pucAddr = (unsigned char *)ifr.ifr_addr.sa_data;
	(void)sprintf(pcRet, "%.2x%.2x.%.2x%.2x.%.2x%.2x", pucAddr[0], pucAddr[1], pucAddr[2], pucAddr[3], pucAddr[4], pucAddr[5]);
	close(iSockFd);
	}
#else
#if defined(HPUX11)
	{
	register FILE *pfLanScan;
	auto char acInt[16], acDummy[64], acAddress[14];

	if ((FILE *)0 == (pfLanScan = popen("lanscan -a -i", "r"))) {
		fprintf(stderr, "%s: popen: lanscan -a -i: %s\n", progname, strerror(errno));
		return acDont;
	}
	while (0 == feof(pfLanScan)) {
		fscanf(pfLanScan, "%s %s %s", acAddress, acInt, acDummy);
		if (0 != strcmp((char *)acInt, pcDev))
			continue;
		sprintf(pcRet, "%c%c%c%c.%c%c%c%c.%c%c%c%c", tolower(acAddress[2]), tolower(acAddress[3]), tolower(acAddress[4]), tolower(acAddress[5]), tolower(acAddress[6]), tolower(acAddress[7]), tolower(acAddress[8]), tolower(acAddress[9]), tolower(acAddress[10]), tolower(acAddress[11]), tolower(acAddress[12]), tolower(acAddress[13]));
		break;
	}
	fclose(pfLanScan);
	}
#else
#if defined(SUN5) || defined(FREEBSD)
	{
	register FILE *pfArp;
	register struct ether_addr *eaAddr;
	auto char acDummy[64], acMacAddr[sizeof(acDont)], acCmd[1024+256+8];
	static char acEther[] = "ether ";

	sprintf(acCmd, "/usr/sbin/arp %s", pcIP);
	if ((FILE *)0 == (pfArp = popen(acCmd, "r"))) {
		fprintf(stderr, "%s: %s: popen: %s: %s\n", progname, pcDev, acCmd, strerror(errno));
		return acDont;
	}
	fscanf(pfArp, "%s %s %s %s", acDummy, acDummy, acDummy, acMacAddr);
	fclose(pfArp);
	if (0 == strcmp("no", acMacAddr)) {
		register char *pcLine;
		auto size_t wLen;

		sprintf(acCmd, "ifconfig %s", pcDev);
		if ((FILE *)0 == (pfArp = popen(acCmd, "r"))) {
			fprintf(stderr, "%s: %s: no MAC in the ARP table\n", progname, pcIP);
			fprintf(stderr, "%s: %s: cannot run %s\n", progname, pcDev, acCmd);
			return acDont;
		}
		while ((char *)0 != (pcLine = fgetln(pfArp, & wLen))) {
			if (0 == wLen || '\n' != pcLine[wLen-1])
				break;
			pcLine[--wLen] = '\000';
			while (isspace(*pcLine))
				++pcLine;
			if (0 != strncmp(pcLine, acEther, sizeof(acEther)-1))
				continue;
			pcLine += sizeof(acEther)-1;
			strncpy(acMacAddr, pcLine, sizeof(acMacAddr));
			break;
		}
		fclose(pfArp);
	}
	eaAddr = (struct ether_addr *)ether_aton(acMacAddr);
	sprintf(pcRet, "%.2x%.2x.%.2x%.2x.%.2x%.2x", eaAddr->ether_addr_octet[0], eaAddr->ether_addr_octet[1], eaAddr->ether_addr_octet[2], eaAddr->ether_addr_octet[3], eaAddr->ether_addr_octet[4], eaAddr->ether_addr_octet[5]);
	}
#else
	fprintf(stderr, "%s: no way to get MAC address\n", progname);
#endif
#endif
#endif
	return pcRet;
}

/* take a long interface, FastEthernet0/3, and shorten it: F0/3 */
static char *
ShortenInterface(pcLongDev)
register char *pcLongDev;
{
	register char *pcRet, *pcCursor, *pcRetCursor;
	register int iLen, iFlag = 0;

	if (0 == strcmp(pcLongDev, "none")) {
		return "none";
	}
	pcRet = (char *)calloc(64, sizeof(char));
	pcRetCursor = pcRet;
	iLen = strlen(pcLongDev);
	for(pcCursor = pcLongDev; pcCursor <= (pcLongDev + iLen); ++pcCursor) {
		if (1 == iFlag && isdigit((int)*pcCursor))
			++iFlag;
		if (1 != iFlag) {
			*pcRetCursor = *pcCursor;
			pcRetCursor++;
		}
		if (0 == iFlag)
			++iFlag;
	}
	return pcRet;
}

/* sniff a CDP (Cisco Discovery Protocol) packet on pcDev and		(petef)
 * print info about it in peg's interface.cl format.
 */
static int
SniffCDP(char *pcDev)
{
	auto struct bpf_program bpCDP;
	auto bpf_u_int32 uiMaskp, uiNetp;
	register pcap_t *p;
	register char *pcIP;

	if (fVerbose) {
		printf("Sniffing CDP on interface %s", pcDev);
		if (0 != iTimeout) {
			printf(" (timeout=%d)", iTimeout);
		}
		printf("...\n");
	}
	if ((pcap_t *)0 == (p = pcap_open_live(pcDev, ETHERMTU, 1, 1000, acErrbuf))) {
		fprintf(stderr, "%s: pcap_open_live: %s\n", progname, acErrbuf);
		return 1;
	}

	if (-1 == pcap_compile(p, &bpCDP, pcCdpExpr, 0, 0)) {
		fprintf(stderr, "%s: pcap_compile: %s\n", progname, pcap_geterr(p));
		return 1;
	}

	if (-1 == pcap_setfilter(p, &bpCDP)) {
		fprintf(stderr, "%s: pcap_setfilter: %s\n", progname, pcap_geterr(p));
		return 1;
	}

	/* By default, we don't know anything
	 */
	strcpy(acSwitch, "?");
	strcpy(acPort, "none");
	strcpy(acVLAN, "");
	alarm(0);
	signal(SIGALRM, alarmhandler);
	alarm(iTimeout);
	if (0 == setjmp(jmpbuf) && -1 == pcap_loop(p, 1, ParseCDP, (u_char *)0)) {
		alarm(0);
		fprintf(stderr, "%s: pcap_loop: %s\n", progname, pcap_geterr(p));
		return 1;
	}
	alarm(0);

	pcIP = GetIP(pcDev);
	printf("%s:%s:%s:%s:%s:%s:%s\n", acHostname, pcDev, pcIP, GetMac(pcDev, pcIP), acSwitch, ShortenInterface(acPort), acVLAN);

	(void)pcap_close(p);
	return 0;
}

/* There is a bug in the interaction between signals and "pause"	(ksb)
 * on Linux, we fork to avoid it.  When there is only 1 requested
 * interface we won't see it.
 */
static void
LinuxBug(int argc, char **argv)
{
#if defined(LINUX)
	register pid_t wPid, w;
	register int iRet;
	auto int iCode;

	if (1 == argc) {
		return;
	}
	for (iRet = EX_OK; argc > 0; --argc, ++argv) {
		switch (wPid = fork()) {
		case -1:
			fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
			break;
		case 0: /* we are the kid */
			exit(SniffCDP(*argv));
			break;
		default:
			while (-1 != (w = wait(& iCode)) && wPid != w) {
				/* nada */
			}
			if (EX_OK != iRet) {
				iRet = iCode;
			}
			continue;
		}
		break;
	}
	exit(iRet);
#endif
}
%%
