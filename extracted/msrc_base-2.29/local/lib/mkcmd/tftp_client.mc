/* $Id: tftp_client.mc,v 8.30 2002/03/26 21:51:13 ksb Exp $
 * An abstraction to make use of tftp protocol for other programs	(ksb)
 */

static TFTP_CLIENT TC_Local;		/* private data saved locally	*/

/* set the run-time up							(ksb)
 */
void
tftp_init()
{
	TC_Local.rexmtval = 5;
	TC_Local.maxtimeout = 25;
	TC_Local.schat = -1;
	TC_Local.fconnected = 0;
}

/* tune timeout parameters for the application				(ksb)
 * call me as
 *	tftp_rexmt(rexmt_value);
 * before you start a tftp when you want a faster (slower) timeout.
 */
int
tftp_rexmt(iReXmit)
	int iReXmit;
{
	register int w;

	w = TC_Local.rexmtval;
	TC_Local.rexmtval = iReXmit;
	return w;
}

/* change the timeout, like umask					(ksb)
 */
int
tftp_timeout(iMaxTimeout)
	int iMaxTimeout;
{
	register int w;

	w = TC_Local.maxtimeout;
	TC_Local.maxtimeout = iMaxTimeout;
	return w;
}

static int tftp_onclock;
static jmp_buf tftp_ljbrequest;

/* handle the clock and total timeouts					(bsd)
 */
static SIGRET_T
tftp_timer(sig)
int sig;
{

	tftp_onclock += TC_Local.rexmtval;
#if !HAVE_STICKY_SIGNALS
	signal(SIGALRM, tftp_timer);
#endif
	if (tftp_onclock >= TC_Local.maxtimeout) {
		errno = ETIMEDOUT;
		longjmp(tftp_ljbrequest, 1);
	}
}

/* Send a file to a tftpd						(bsd)
 * Most tftpd's require the file to exist and be world write (o+w).
 * ksb's hack allows the file to be missing and the directory world write
 * to allow creation of new files with tftpd.
 */
int
tftp_put(fpSend, pcName, pcMode)
FILE *fpSend;
char *pcName, *pcMode;
{
	register volatile int iRet;
	register int iSize, iGot;
	register u_short usBlock, usOp, usSeq;
	register int (*pfiPack)(TFTP_PACKET *, const char *, FILE *);
	auto TFTP_PACKET TPData, TPAck;
	auto struct sockaddr_in from;
	auto int fromlen;
	auto struct sigaction saHad, saHave;

	/* reset the pack packer
	 */
	if ((FILE *)0 == fpSend) {
		pfiPack = tftp_hunk;
	} else {
		pfiPack = tftp_pack;
	}
	(void)pfiPack((TFTP_PACKET *)0, pcMode, (FILE *)0);

	iSize = tftp_request(& TPData, WRQ, pcName, pcMode);
	usBlock = 0;
	if (-1 == iSize) {
		errno = E2BIG;
		return -1;
	}

	TC_Local.SIserver.sin_port = TC_Local.wport;
#if USE_BCOPY
	bzero((char *)&saHave, sizeof(saHave));
#else
	memset((char *)&saHave, '\000', sizeof(saHave));
#endif
	saHave.sa_handler = tftp_timer;
	saHave.sa_flags = 0;	/*  SA_RESTART is a problem */
	if (-1 == sigaction(SIGALRM, & saHave, & saHad)) {
		return 4;	/* bad operation */
	}
	if (0 != (iRet = setjmp(tftp_ljbrequest))) {
		sigaction(SIGALRM, & saHad, (void *)0);
		return iRet;
	}
	tftp_onclock = 0;
	for (;;) {
		tftp_packet(& TPData, iSize, "put sent");
		if (sendto(TC_Local.schat, (void *)&TPData, iSize, 0, (struct sockaddr *)&TC_Local.SIserver, sizeof(TC_Local.SIserver)) != iSize) {
%			fprintf(stderr, "%%s: sendto: %%d: %%s\n", %b, TC_Local.schat, %E)%;
			iRet = 5;
			break;
		}

		/* same logic as a get, when we timeout we default to an
		 * error, when we get a data-gram we default to OK
		 */
		alarm(TC_Local.rexmtval);
		fromlen = sizeof(from);
		iGot = recvfrom(TC_Local.schat, (void *)&TPAck, sizeof(TPAck), 0, (struct sockaddr *)&from, &fromlen);
		alarm(0);
		if (iGot < 0) {
			iRet = 5;
			if (EINTR == errno) {
				tftp_packet((void *)0, 0, "put ack timeout");
				continue;
			}
%			fprintf(stderr, "%%s: recvfrom: %%d: %%s\n", %b, TC_Local.schat, %E)%;
			break;
		}
		tftp_packet(&TPAck, iGot, "put recieved");
		iRet = 0;

		/* redircet from 69 to data port first time through
		 * XXX: but should verify packet came from our server
		 */
		TC_Local.SIserver.sin_port = from.sin_port;

		usOp = ntohs(TPAck.usopcode);
		if (ACK != usOp) {
			if (ERROR == usOp) {
				iRet = ntohs(TPAck.u.err.uscode);
%				fprintf(stderr, "%%s: tftp_put: %%d: %%s\n", %b, iRet, TPAck.u.err.acmsg)%;
			} else {
				iRet = 4;	 /* illegal op */
			}
			break;
		}

		/* Try to re-synchronize.  When he just ack'd the
		 * packet behind what we just sent, send ours again.
		 */
		usSeq = ntohs(TPAck.u.ack.usblock);
		if (usSeq != usBlock) {
			(void)tftp_sync(TC_Local.schat);
			if (usSeq+1 == usBlock) {
				continue;
			}
			iRet = 5;	/* help */
			break;
		}

		/* if the last frame sent and ack'd wasn't block 0
		 * and it was not a full frame, we are done,
		 */
		if (iSize < sizeof(TFTP_PACKET) && usBlock > 0) {
			break;
		}

		/* build next frame, reset transaction timeout
		 */
		tftp_onclock = 0;
		iSize = pfiPack(& TPData, pcMode, fpSend);
		if (iSize < 0) {
			iRet = 3;
			iSize = tftp_nak(& TPData, errno + 100);
			tftp_packet(& TPData, iSize, "nak");
			if (sendto(TC_Local.schat, (void *)&TPData, iSize, 0, (struct sockaddr *)&TC_Local.SIserver, sizeof(TC_Local.SIserver)) != iSize) {
%				fprintf(stderr, "%%s: sendto: %%d: %%s\n", %b, TC_Local.schat, %E)%;
			}
			break;
		}
		++usBlock;
	}
	(void)sigaction(SIGALRM, & saHad, (void *)0);
	if (0 != iRet) {
		return iRet;
	}
	return 0;
}

/* Receive a file from a tftpd server					(bsd)
 * return a tftp error code and set errno, 0 means OK
 */
int
tftp_get(fpWrite, pcName, pcMode)
FILE *fpWrite;
char *pcName, *pcMode;
{
	register u_short usBlock, usSeq, usOp;
	register int iAckSize, iFrame, iRet;
	auto int fromlen;
	auto struct sockaddr_in from;
	auto TFTP_PACKET TPAck, TPData;
	auto struct sigaction saHad, saHave;

	/* reset the packet unpacker
	 */
	if ((FILE *)0 == fpWrite) {
		(void)tftp_ram((TFTP_PACKET *)0, 0, pcMode);
	} else {
		(void)tftp_unpack((TFTP_PACKET *)0, 0, pcMode, (FILE *)0);
	}

	iAckSize = tftp_request(& TPAck, RRQ, pcName, pcMode);
	if (-1 == iAckSize) {
		errno = E2BIG;
		return 4;
	}

	TC_Local.SIserver.sin_port = TC_Local.wport;
#if USE_BCOPY
	bzero((char *)&saHave, sizeof(saHave));
#else
	memset((char *)&saHave, '\000', sizeof(saHave));
#endif
	saHave.sa_handler = tftp_timer;
	saHave.sa_flags = 0;	/*  SA_RESTART is a problem */
	if (-1 == sigaction(SIGALRM, & saHave, & saHad)) {
		return 4;	/* bad operation */
	}
	if (0 != (iRet = setjmp(tftp_ljbrequest))) {
		sigaction(SIGALRM, & saHad, (void *)0);
		errno = iRet;
		return 1;
	}
	tftp_onclock = 0;
	usBlock = 1;
	iFrame = sizeof(TFTP_PACKET);
	for (;;) {
		tftp_packet(& TPAck, iAckSize, "get sent");
		if (sendto(TC_Local.schat, (void *)&TPAck, iAckSize, 0, (struct sockaddr *)&TC_Local.SIserver, sizeof(TC_Local.SIserver)) != iAckSize) {
			iRet = 5;
%			fprintf(stderr, "%%s: sendto: %%d: %%s\n", %b, TC_Local.schat, %E)%;
			break;
		}
		/* if we just ACK a partial frame we are done
		 */
		if (iFrame != sizeof(TFTP_PACKET)) {
			break;
		}

		/* Read a frame from the server, when we saw an error we
		 * remember that, if we timeout then we'll return an error
		 * when we read a good frame we set iRet back to 0 (for OK).
		 */
		alarm(TC_Local.rexmtval);
		fromlen = sizeof(from);
		iFrame = recvfrom(TC_Local.schat, (void *)&TPData, sizeof(TPData), 0, (struct sockaddr *)&from, &fromlen);
		alarm(0);
		if (iFrame < 0) {
			iRet = 5;
			if (EINTR == errno) {
				tftp_packet((void *)0, 0, "get frame timeout");
				continue;
			}
%			fprintf(stderr, "%%s: recvfrom: %%d: %%s\n", %b, TC_Local.schat, %E)%;
			break;
		}
		tftp_packet(&TPData, iFrame, "get received");
		iRet = 0;

		/* redirect first time to data port, other times redundant
		 * assignment is faster than the check... (ksb)
		 */
		TC_Local.SIserver.sin_port = from.sin_port;
		/* XXX: but we should verify server address */

		usOp = ntohs(TPData.usopcode);
		if (DATA != usOp) {
			if (ERROR == usOp) {
				iRet = ntohs(TPData.u.err.uscode);
%				fprintf(stderr, "%%s: Error code %%d: %%s\n", %b, iRet, TPData.u.err.acmsg)%;
			} else {
				iRet = 4;
			}
			break;
		}
		usSeq = ntohs(TPData.u.data.usblock);

		/* On an error, try to synchronize both sides.
		 */
		if (usSeq != usBlock) {
			(void)tftp_sync(TC_Local.schat);
			if (usSeq+1 == usBlock) {
				continue;
			}
			iRet = 2;
			break;
		}
		if ((FILE *)0 == fpWrite) {
			(void)tftp_ram(& TPData, iFrame, pcMode);
		} else {
			(void)tftp_unpack(& TPData, iFrame, pcMode, fpWrite);
		}
		iAckSize = tftp_ack(& TPAck, usBlock);
		++usBlock;
	}
	(void)sigaction(SIGALRM, & saHad, (void *)0);
	if (0 != iRet) {
		return iRet;
	}
	return 0;
}

/* shutdown the connection to a server					(ksb)
 */
int
tftp_close()
{
	if (TC_Local.fconnected && -1 != TC_Local.schat) {
		close(TC_Local.schat);
		TC_Local.schat = -1;
	}
	TC_Local.fconnected = 0;
	return 0;
}

/* set the server and port to get/put files from/to			(ksb)
 */
int
tftp_open(pHEServer, pSEPort)
	struct hostent *pHEServer;
	struct servent *pSEPort;
{
	register int sSock;

	if (TC_Local.fconnected) {
		tftp_close();
	}
#if USE_BCOPY
	bzero((char *)&TC_Local.SIserver, sizeof(TC_Local.SIserver));
	bzero((char *)&TC_Local.SIme, sizeof(TC_Local.SIme));
#else
	memset((char *)&TC_Local.SIserver, '\000', sizeof(TC_Local.SIserver));
	memset((char *)&TC_Local.SIme, '\000', sizeof(TC_Local.SIme));
#endif

	if ((struct hostent *)0 == pHEServer) {
		return 0;
	}
	if (-1 == (sSock = socket(AF_INET, SOCK_DGRAM, 0))) {
%		fprintf(stderr, "%%s: socket: %%s\n", %b, %E)%;
		return -1;
	}
	TC_Local.SIme.sin_family = AF_INET;
	if (bind(sSock, (struct sockaddr *)&TC_Local.SIme, sizeof(TC_Local.SIme)) < 0) {
%		fprintf(stderr, "%%s: bind: %%d: %%s\n", %b, sSock, %E)%;
		return -1;
	}

	TC_Local.SIserver.sin_family = pHEServer->h_addrtype;
#if USE_BCOPY
	bcopy(pHEServer->h_addr_list[0], &TC_Local.SIserver.sin_addr, pHEServer->h_length);
#else
	memcpy(&TC_Local.SIserver.sin_addr, pHEServer->h_addr_list[0], pHEServer->h_length);
#endif
	TC_Local.wport = TC_Local.SIserver.sin_port = pSEPort->s_port;

%@if "TFTP_DEBUG"
%	if (%K<TFTP_DEBUG>1v) %{
		register unsigned char *puc;

%		fprintf(stderr, "%%s: connect to %%s:%%d\n", %b, pHEServer->h_name, ntohs(pSEPort->s_port))%;
		puc = (unsigned char *)& TC_Local.SIserver.sin_addr;
%		fprintf(stderr, "%%s: host %%s IP %%d.%%d.%%d.%%d\n", %b, pHEServer->h_name, puc[0], puc[1], puc[2], puc[3])%;
%		fprintf(stderr, "%%s: socket %%d:%%d\n", %b, sSock, htons(TC_Local.SIme.sin_port))%;
	}
%@endif
	TC_Local.schat = sSock;
	TC_Local.fconnected = 1;
	return 0;
}


/* describe our status							(ksb)
 */
static TFTP_CLIENT *
tftp_status()
{
	return & TC_Local;
}
