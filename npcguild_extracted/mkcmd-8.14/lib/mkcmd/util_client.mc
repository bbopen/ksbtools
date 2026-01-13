/* $Id: util_client.mc,v 8.8 1999/07/31 21:17:42 ksb Exp $
 * meta C file for mkcmd's client socket facliity			(ksb)
 */

/* set the port for socket connection					(ksb)
 * return the fd for the new connection.
 */
static int
%%K<SockOpen>1v(pSD, pcName)
SOCK_DESCR *pSD;
char *pcName;
{
	auto struct sockaddr_in wPort;
	auto short int sPort;
	register int fd, i;
	register struct hostent *hp;
	register struct servent *sp;
	register struct protoent *pp;
	register char *pcRemote, *pcProto;
	register int iType, iProto;

	if ('\000' == (pcRemote = pSD->u_achost)[0]) {
		pcRemote = "localhost";
	}
	if ((char *)0 == (pcProto = pSD->u_pcproto) || '\000' == pcProto[0]) {
		pcProto = "tcp";
	}

	/* XXX zero the portstruct?
	 */
	if ((struct hostent *)0 != (hp = gethostbyname(pcRemote))) {
#if 0
		/* make hostname short and pretty
		 */
		(void)Whittle(acThisHost, pcRemote);
#endif
	} else {
%		fprintf(stderr, "%%s: %%s: gethostbyname: %%s: %%s\n", %b, pcName, pcRemote, hstrerror(h_errno))%;
		return -1;
	}

	if ('\000' == pSD->u_acport[0]) {
%		fprintf(stderr, "%%s: %%s: no service to contact on %%s\n", %b, pcName, pcRemote)%;
		return -1;
	} else if ((struct servent *)0 != (sp = getservbyname(pSD->u_acport, pcProto))) {
		sPort = sp->s_port;
	} else if (isdigit(pSD->u_acport[0])) {
		sPort = htons((u_short)atoi(pSD->u_acport));
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
	fd = -1;
	for (i = 0; (char *)0 != hp->h_addr_list[i]; ++i) {
		if ((fd = socket(AF_INET, iType, iProto)) < 0) {
%			fprintf(stderr, "%%s: socket: %%s\n", %b, %E)%;
			return -1;
		}
#if USE_STRINGS
		(void)bcopy((char *)hp->h_addr_list[i], (char *)&wPort.sin_addr, hp->h_length);
#else
		(void)memcpy((char *)&wPort.sin_addr, (char *)hp->h_addr_list[i], hp->h_length);
#endif
		wPort.sin_family = AF_INET;
		wPort.sin_port = sPort;
		if (0 == connect(fd, (struct sockaddr *)&wPort, sizeof(wPort))) {
			return fd;
		}
		close(fd);
	}
%	fprintf(stderr, "%%s: %%s: connect: %%s:%%d: %%s\n", %b, pcName, pcRemote, ntohs(sPort), %E)%;
	return -1;
}
