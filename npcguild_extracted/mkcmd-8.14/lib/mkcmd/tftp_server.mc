/* $Id: tftp_server.mc,v 8.28 1999/08/24 00:40:57 ksb Exp $
 * implementation for products that want to serve tftp clients		(ksb)
 * Ideas and code copied from tftpd from the BSD distribution
 * Jeeze: http://info.internet.isi.edu:80/in-notes/rfc/files/rfc1123.txt [44]
 * the Sorcerer's Apprentice Syndrome
 */
static TFTP_SERVER TS_Local = { 5, 25};

static int tserv_onclock;
static jmp_buf tserv_ljbrequest;

/* set the server retransmission timeout				(ksb)
 */
int
tserv_rexmt(wNew)
int wNew;
{
	register int wWas;

	wWas = TS_Local.irexmt;
	TS_Local.irexmt = wNew;
	return wWas;
}

/* set the server maximum transmission time				(ksb)
 */
int
tserv_timeout(wNew)
int wNew;
{
	register int wWas;

	wWas = TS_Local.imaxtime;
	TS_Local.imaxtime = wNew;
	return wWas;
}

/* handle server clock operations					(ksb)
 */
static SIGRET_T
tserv_timer(iSig)
int iSig;	/*ARGSUSED*/
{

	tserv_onclock += TS_Local.irexmt;
#if !HAVE_STICKY_SIGNALS
	signal(SIGALRM, tserv_timer);
#endif
	if (tserv_onclock >= TS_Local.imaxtime) {
		longjmp(tserv_ljbrequest, ETIMEDOUT);
	}
}

/* Serve the requested file to a customer				(ksb)
 * Build the first datagram, send it with timeout care,
 * sync with the ACK we read, resend this or build the next;
 * honor the mode to pack the data (tftp_pack does that).
 */
int
tserv_serve(fdClient, fpData, pcMode)
int fdClient;
FILE *fpData;
const char *pcMode;
{
	register volatile int iRet, iError;	/* tftp, system  error */
	register volatile int iSize, iLen;
	register volatile u_short usBlock, usSeq, usOp;
	auto TFTP_PACKET TPData, TPAck;
	auto struct sigaction saHad, saHave;

	/* reset the packing machine
	 */
	(void)tftp_pack((TFTP_PACKET *)0, pcMode, (FILE *)0);

	usBlock = 1;
	iSize = tftp_pack(& TPData, pcMode, fpData);
	if (iSize < 0) {
		iSize = tftp_nak(&TPAck, errno + 100);
		(void)send(fdClient, (void *)&TPAck, iSize, 0);
		tftp_packet(&TPAck, iSize, "nak serve");
		errno = ENAMETOOLONG;
		return 4;	/* bad operation */
	}

	tserv_onclock = 0;
#if USE_BCOPY
	bzero((char *)&saHave, sizeof(saHave));
#else
	memset((char *)&saHave, '\000', sizeof(saHave));
#endif
	saHave.sa_handler = tserv_timer;
	saHave.sa_flags = 0;	/*  SA_RESTART is a problem */
	if (-1 == sigaction(SIGALRM, & saHave, & saHad)) {
		return 4;	/* bad operation */
	}
	iError = 0;
	if (0 != (iRet = setjmp(tserv_ljbrequest))) {
		sigaction(SIGALRM, & saHad, (void *)0);
		errno = iError;
		return iRet;
	}
	for (;;) {
		if (send(fdClient, (void *)&TPData, iSize, 0) != iSize) {
			iError = errno;
			iRet = 5;	/* bad xfer id */
			syslog(LOG_ERR, "serve: send: %d: %m", fdClient);
			break;
		}
		tftp_packet(&TPData, iSize, "serve data");
		alarm(TS_Local.irexmt);        /* read the ack */
		iLen = recv(fdClient, (void *)&TPAck, sizeof(TPAck), 0);
		alarm(0);
		if (iLen < 0) {
			if (EINTR == errno) {
				tftp_packet((void *)0, 0, "serve ack timeout");
				continue;
			}
			iError = errno;
			iRet = 5;	/* bad xfer id */
			syslog(LOG_ERR, "serve: recv: %d: %m", fdClient);
			break;
		}
		tftp_packet(&TPAck, iLen, "serve ack");

		/* got a packet, look at the operation
		 * LLL: make sure it is from the client?
		 */
		usOp = ntohs(TPAck.usopcode);
		if (ACK != usOp) {
			iError = 0;
			if (ERROR == usOp) {
				iRet = ntohs(TPAck.u.err.uscode);
				syslog(LOG_ERR, "client: %d: %s", iRet, TPAck.u.err.acmsg);
			} else {
				iRet = 4;
			}
			break;
		}

		usSeq = ntohs(TPAck.u.ack.usblock);
		if (usSeq != usBlock) {
			(void)tftp_sync(fdClient);
			if (usSeq+1 == usBlock) {
				continue;
			}
			iError = 0;
			iRet = 2;
			break;
		}

		/* When sent and ack's a packet that was not full,
		 * we are done.
		 */
		if (iSize < SEGSIZE) {
			break;
		}
		iSize = tftp_pack(& TPData, pcMode, fpData);
		if (iSize < 0) {
			iError = errno;
			iRet = 2;
			iLen = tftp_nak(& TPAck, errno + 100);
			send(fdClient, (void *)&TPAck, iLen, 0);
			tftp_packet(&TPAck, iLen, "serve nak");
			break;
		}
		++usBlock;
		tserv_onclock = 0;
	}
	(void)sigaction(SIGALRM, & saHad, (void *)0);
	errno = iError;
	return iRet;
}

/* Receive a file from a network customer				(ksb)
 */
int
tserv_accept(fdPeer, fpWrite, pcMode)
int fdPeer;
FILE *fpWrite;
const char *pcMode;
{
	register volatile int iAckSize, iSize;
	register volatile u_short usBlock, usSeq, usOp;
	register volatile int iRet, iError;
	auto TFTP_PACKET TPAck, TPData;
	auto struct sigaction saHad, saHave;

	/* reset the unpack machine
	 */
	(void)tftp_unpack((TFTP_PACKET *)0, 0, pcMode, (FILE *)0);

	usBlock = 0;
	iSize = sizeof(TFTP_PACKET);
	iAckSize = tftp_ack(& TPAck, usBlock);

	tserv_onclock = 0;
#if USE_BCOPY
	bzero((char *)&saHave, sizeof(saHave));
#else
	memset((char *)&saHave, '\000', sizeof(saHave));
#endif
	saHave.sa_handler = tserv_timer;
	saHave.sa_flags = 0;	/*  SA_RESTART is a problem */
	if (-1 == sigaction(SIGALRM, & saHave, & saHad)) {
		return 4;	/* bad operation */
	}
	iError = 0;
	if (0 != (iRet = setjmp(tserv_ljbrequest))) {
		sigaction(SIGALRM, & saHad, (void *)0);
		errno = iError;
		return iRet;
	}
	for (;;) {
		if (send(fdPeer, (void *)&TPAck, iAckSize, 0) != iAckSize) {
			iError = errno;
			iRet = 5; /* bad xfer id */
			syslog(LOG_ERR, "send: %s: %m", fdPeer);
			break;
		}
		tftp_packet(&TPAck, iAckSize, "accept send ack");
		/* when we ack a data packet < full size we are done
		 */
		if (iSize < sizeof(TFTP_PACKET)) {
			break;
		}
		alarm(TS_Local.irexmt);
		iSize = recv(fdPeer, (void *)&TPData, sizeof(TPData), 0);
		alarm(0);
		if (iSize < 0) {
			if (EINTR == errno) {
				tftp_packet((void *)0, 0, "accept frame timeout");
				continue;
			}
			iRet = 5;	/* bad xfer id */
			iError = errno;
			syslog(LOG_ERR, "recv: %d: %m", fdPeer);
			break;
		}
		tftp_packet(&TPData, iSize, "accept recv data");
		usOp = ntohs(TPData.usopcode);
		if (DATA != usOp) {
			iError = 0;
			if (ERROR == usOp) {
				iRet = ntohs(TPAck.u.err.uscode);
				syslog(LOG_ERR, "client: %d: %s", iRet, TPAck.u.err.acmsg);
			} else {
				iRet = 2;
			}
			break;
		}
		++usBlock;
		usSeq = ntohs(TPData.u.ack.usblock);
		if (usSeq != usBlock) {
			(void)tftp_sync(fdPeer);
			if (usSeq+1 == usBlock) {
				continue;
			}
			iError = 0;
			iRet = 4;
			break;
		}
		(void)tftp_unpack(& TPData, iSize, pcMode, fpWrite);
		iAckSize = tftp_ack(& TPAck, usBlock);
		tserv_onclock = 0;
	}
	(void)sigaction(SIGALRM, & saHad, (void *)0);
	errno = iError;
	return iRet;
}

/* setup the tftp server data structures				(ksb)
 * or reset them.
 */
void
tserv_init()
{
	TS_Local.irexmt = 5;
	TS_Local.imaxtime = 25;
}

/* let the Implemetor see our public parameters				(ksb)
 */
static TFTP_SERVER *
tserv_status()
{
	return & TS_Local;
}
