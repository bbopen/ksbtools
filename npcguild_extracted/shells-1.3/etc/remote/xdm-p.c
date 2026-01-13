/* $Id: xdm-p.c,v 1.4 1995/09/01 20:29:08 kb207252 Exp $
 *
 * find out if there's an XDM on a given host
 *
 * Ben Jackson (ben@ben.com) Aug 2, 1993
 *
 * $Compile:${cc-cc} ${cc_debug--O} -o %F %f -I/usr/local/X11/include -L/usr/local/X11/lib -lXdmcp
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>

#include "machine.h"

#if NEED_XDMCP
#include <X11/Xos.h>
#include <X11/Xmd.h>
#include <X11/Xdmcp.h>

extern char *progname;

extern int errno;

/*
 * open a UDP port and transmit a bogus QUERY to to the target host.
 * If the host responds at all (we don't parse the reply) we assume there
 * is an XDM running (it will probably send a rejection since we send
 * bogus auth information).
 */
int
IsRunningXDM(pcHost, iTimeoutSecs)
	char *pcHost;
{
	register int fdSock, iLen, i;
	register struct hostent *pHEent;
	auto struct fd_set FDSread;
	auto struct timeval timeout;
	auto XdmcpHeader header;
	auto XdmcpBuffer buffer;
	auto ARRAYofARRAY8  AuthenticationNames;
	auto char acBuf[256];
	auto int iJunk;
	auto struct sockaddr_in SINrecv, SINsend;

	/* locate our victim
	 */
	if ((struct hostent *)0 == (pHEent = gethostbyname(pcHost)) || (char *)0 == pHEent->h_name) {
		fprintf(stderr, "%s: unknown host %s\n", progname, pcHost);
		return 0;
	}
	SINsend.sin_family = AF_INET;
	SINsend.sin_port = htons(XDM_UDP_PORT);
#if USE_STRINGS
	(void)bcopy(pHEent->h_addr, &SINsend.sin_addr.s_addr, pHEent->h_length);
#else
	(void)memcpy(&SINsend.sin_addr.s_addr, pHEent->h_addr, pHEent->h_length);
#endif

	/* create a socket and bind it to a port we can receive
	 * replies on.
	 */
	if (-1 == (fdSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) {
		fprintf(stderr, "%s: socket: %s\n", progname, strerror(errno));
		return 0;
	}
	SINrecv.sin_family = AF_INET;
	SINrecv.sin_addr.s_addr = INADDR_ANY;
	SINrecv.sin_port = htons(17700);

	if (-1 == bind(fdSock, (struct sockaddr *)&SINrecv, sizeof (struct sockaddr_in))) {
		fprintf(stderr, "%s: bind: %s\n", progname, strerror(errno));
		close(fdSock);
		return 0;
	}

	/* tickle the xdm
	 */
	buffer.size = buffer.pointer = buffer.count = 0;
	buffer.data = (BYTE *)0;
	header.version = XDM_PROTOCOL_VERSION;
	header.opcode = (CARD16) QUERY;
	header.length = 1;
	/* ZZZ XdmcpAllocARRAYofARRAY8 is b0rken.  do it ourselves, sigh */
	AuthenticationNames.length = 1;
	AuthenticationNames.data = (ARRAY8Ptr) malloc(sizeof(ARRAY8));
	XdmcpAllocARRAY8(AuthenticationNames.data, 3);
#if USE_STRINGS
	(void)bcopy("fu", AuthenticationNames.data[0].data, 2);
#else
	(void)memcpy(AuthenticationNames.data[0].data, "fu", 2);
#endif
	for (i = 0; i < (int)AuthenticationNames.length; i++)
		header.length += 2 + AuthenticationNames.data[i].length;
	XdmcpWriteHeader(&buffer, &header);
	XdmcpWriteARRAYofARRAY8 (&buffer, &AuthenticationNames);
	if (! XdmcpFlush(fdSock, &buffer, &SINsend, sizeof(struct sockaddr_in))) {
		fprintf(stderr, "%s: XdmcpFlush fiailed\n", progname);
		return 0;
	}

	/* set a timeout, and wait for the reply.
	 * if we timeout, we assume there's no XDM.
	 * might be clever to retry a few times.
	 */
	timeout.tv_sec = iTimeoutSecs;
	timeout.tv_usec = 0;
	FD_ZERO(&FDSread);
	FD_SET(fdSock, &FDSread);

 select_again:
	switch (select(fdSock + 1, &FDSread, 0, 0, &timeout)) {
	case -1:
		if (EINTR == errno)
			goto select_again;
		fprintf(stderr, "%s: select: %s\n", progname, strerror(errno));
		close(fdSock);
		return 0;
	case 0:
		/* no xdm!
		 */
		close(fdSock);
		return 0;
	default:
		/* read the reply, ignore it, return success
		 */
		iJunk = sizeof(struct sockaddr_in);
		iLen = recvfrom(fdSock, acBuf, sizeof(acBuf), 0, (struct sockaddr *)&SINrecv, &iJunk);
		close(fdSock);
		if (-1 == iLen) {
			fprintf(stderr, "%s: recvfrom: %s\n", progname, strerror(errno));
			return 0;
		}
		return 1;
	}
	/* NOT REACHED */
}

#else
int
IsRunningXDM(pcHost, iTimeoutSecs)
	char *pcHost;
{
	return 0;
}
#endif

#ifdef TEST

char *progname;

main(ac, av)
	char **av;
{
	progname = av[0];

	if (2 == ac) {
		if (IsRunningXDM(av[1], 5)) {
			printf("yes\n");
			exit(0);
		} else {
			printf("no\n");
			exit(1);
		}
	}
	exit(0);
}
#endif /* TEST */
