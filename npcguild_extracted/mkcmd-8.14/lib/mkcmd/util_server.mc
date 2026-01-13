/* $Id: util_server.mc,v 8.7 1997/10/27 11:55:22 ksb Exp $
 * meta C file for mkcmd's server socket facliity			(ksb)
 */

/* set the port for socket listener					(ksb)
 * return the fd for the new listener.
 */
static int
%%K<SockBind>1v(pSD, pcName)
SOCK_DESCR *pSD;
char *pcName;
{
	auto struct sockaddr_in wPort;
	register int fd;
	register struct hostent *hp;
	register struct servent *sp;
	register struct protoent *pp;
	register char *pcLocal, *pcProto;
	register int iType, iProto;

	/* map the hostname "*" to "any" like shell hacks would expect
	 */
	if ('\000' == (pcLocal = pSD->u_achost)[0] || '*' == pcLocal[0] && '\000' == pcLocal[1]) {
		pcLocal = "";
	}
	if ((char *)0 == (pcProto = pSD->u_pcproto) || '\000' == pcProto[0]) {
		pcProto = "tcp";
	}

	/* XXX zero the portstruct?
	 */
	wPort.sin_family = AF_INET;
	/* no hostname means listen on any/all interfaces
	 */
	if ('\000' == pcLocal[0]) {
		wPort.sin_addr.s_addr = htonl(INADDR_ANY);
	} else if ((struct hostent *)0 != (hp = gethostbyname(pcLocal))) {
#if USE_STRINGS
		(void)bcopy((char *)hp->h_addr, (char *)&wPort.sin_addr, hp->h_length);
#else
		(void)memcpy((char *)&wPort.sin_addr, (char *)hp->h_addr, hp->h_length);
#endif
#if 0
		/* make hostname short and pretty
		 */
		(void)whittle(acThisHost, pcLocal);
#endif
	} else {
%		fprintf(stderr, "%%s: %%s: gethostbyname: %%s: %%s\n", %b, pcName, pcLocal, hstrerror(h_errno))%;
		return -1;
	}

	if ('\000' == pSD->u_acport[0]) {
%		fprintf(stderr, "%%s: %%s: no service to contact on %%s\n", %b, pcName, pcLocal)%;
		return -1;
	} else if ((struct servent *)0 != (sp = getservbyname(pSD->u_acport, pcProto))) {
		wPort.sin_port = sp->s_port;
	} else if (isdigit(pSD->u_acport[0])) {
		wPort.sin_port = htons((u_short)atoi(pSD->u_acport));
	} else {
%		fprintf(stderr, "%%s: %%s: getservbyname: %%s/%%s: no such service\n", %b, pcName, pSD->u_acport, pcProto)%;
		return -1;
	}

	iType = SOCK_STREAM;
	if ((struct protoent *)0 != (pp = getprotobyname(pcProto))) {
		iProto = pp->p_proto;
		switch (iProto) {
		case IPPROTO_IP:
		case IPPROTO_TCP:
			iType = SOCK_STREAM;
			break;
		case IPPROTO_UDP:
			iType = SOCK_DGRAM;
			break;
		case IPPROTO_RAW:
			iType = SOCK_RAW;
			break;
#if 0
			iType = SOCK_RDM;
			break;
		case IPPROTO_ICMP:
		case IPPROTO_IGMP:
		case IPPROTO_GGP:
		case IPPROTO_IPIP:
		case IPPROTO_EGP:
		case IPPROTO_PUP:
		case IPPROTO_IDP:
		case IPPROTO_TP:
		case IPPROTO_RSVP:
		case IPPROTO_EON:
		case IPPROTO_ENCAP:
#endif
		default:
			break;
		}
	} else {
		iProto = 0;
	}
	/* set up the socket to talk to the server for all load data
	 * (it will tell us who to talk to to get a real connection)
	 */
	if ((fd = socket(AF_INET, iType, iProto)) < 0) {
%		fprintf(stderr, "%%s: socket: %%s\n", %b, %E)%;
		return -1;
	}
	if (bind(fd, (struct sockaddr *)&wPort, sizeof(wPort)) < 0) {
%		fprintf(stderr, "%%s: %%s: bind: %%s:%%d: %%s\n", %b, pcName, pcLocal, ntohs(wPort.sin_port), %E)%;
		return -1;
	}
	(void)listen(fd, 20);

	return fd;
}
