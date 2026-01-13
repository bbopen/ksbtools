/*@Header@*/
/* $Id: util_tftp.mc,v 8.24 2001/10/18 01:17:37 ksb Exp $
 */

/*@Explode sync@*/
/* When an error has occurred, it is possible that the two sides	(bsd)
 * are out of synch.  i.e.: that what I think is the other side's
 * response to packet N is really their response to packet N-1.
 *
 * So, to try to prevent that, we flush all the input queued up
 * for us on the network connection on our host.
 *
 * We return the number of packets we flushed (mostly for reporting
 * when TFTP_DEBUG is active). [modified by ksb]
 */
static int
tftp_sync(sBad)
int sBad;
{
	register int j;
	auto int i, fromlen;
	auto TFTP_PACKET TPJunk;
	auto struct sockaddr_in from;

	for (j = 0; ioctl(sBad, FIONREAD, &i), 0 != i; ++j) {
		fromlen = sizeof from;
		(void)recvfrom(sBad, (void*)&TPJunk, sizeof(TPJunk), 0, (struct sockaddr *)&from, &fromlen);
	}
	return j;
}

/*@Explode packet@*/
/* debug output to stderr						(bsd)
 */
%@if "TFTP_DEBUG"
static void
tftp_packet(pTP, iLen, pcWho)
TFTP_PACKET *pTP;
int iLen;
const char *pcWho;
{
	static char *opcodes[] = {
		"#0", "RRQ", "WRQ", "DATA", "ACK", "ERROR"
	};
	register char *cp, *file;
	register u_short op;

%	if (! %K<TFTP_DEBUG>1v) %{
		return;
	}
	if ((TFTP_PACKET *)0 == pTP || 0 == iLen) {
%		fprintf(stderr, "%%s: %%s <no data>\n", %b, pcWho)%;
		return;
	}

	op = ntohs(pTP->usopcode);
	if (op < RRQ || op > ERROR) {
%		fprintf(stderr, "%%s: %%s opcode=%%x ", %b, pcWho, op)%;
	} else {
%		fprintf(stderr, "%%s: %%s %%s ", %b, pcWho, opcodes[op])%;
	}

	switch (op) {
	case RRQ:
	case WRQ:
		file = pTP->u.req.acname;
		cp = strchr(file, '\000');
%		fprintf(stderr, "<file=%%s, mode=%%s>\n", file, cp + 1)%;
		break;

	case DATA:
		iLen -= 4;
%		fprintf(stderr, "<block=%%d, %%d byte%%s>\n", ntohs(pTP->u.data.usblock), iLen, 1 == iLen ? "" : "s")%;
		break;

	case ACK:
%		fprintf(stderr, "<block=%%d>\n", ntohs(pTP->u.ack.usblock))%;
		break;

	case ERROR:
%		fprintf(stderr, "<code=%%d, msg=%%s>\n", ntohs(pTP->u.err.uscode), pTP->u.err.acmsg)%;
		break;
	}
}
%@endif

/*@Explode request@*/
/* Start the conversation with a request packet built here.		(ksb)
 * Some UNIX filenames are too long to request (MAXPATHLEN>505 bytes),
 * {yes we subtracted strlen("octet")+1}.
 * Request is RRQ or WRQ, return the size of the packet constructed.
 */
int
tftp_request(pTPMake, usRequest, pcName, pcMode)
TFTP_PACKET *pTPMake;
u_short usRequest;
const char *pcName, *pcMode;
{
	register char *pcCursor;
	register int iSize;

	iSize = strlen(pcName)+1 + strlen(pcMode)+1;
	if (iSize > sizeof(pTPMake->u.req.acname)) {
		return -1;
	}
	pTPMake->usopcode = htons(usRequest);
	pcCursor = pTPMake->u.req.acname;
	(void)strcpy(pcCursor, pcName);
	pcCursor += strlen(pcCursor)+1;
	(void)strcpy(pcCursor, pcMode);
	pcCursor += strlen(pcCursor)+1;
	return iSize + sizeof(pTPMake->usopcode);
}

/*@Explode nak@*/
/* Build a nak packet (error message).					(ksb)
 * Error code passed in is one of the standard TFTP codes, or a UNIX errno
 * offset by 100. Return the size of the packet.
 */
int
tftp_nak(pTPMake, iError)
register TFTP_PACKET *pTPMake;
int iError;
{
	register char *pcText;

	pcText = 0 == iError ? "OK" : tftp_error(iError);
	if ((char *)0 == pcText) {
		if ((char *)0 == (pcText = strerror(iError - 100))) {
			pcText = "unknown";
		}
		iError = EUNDEF;
	}

	pTPMake->usopcode = htons((u_short)ERROR);
	pTPMake->u.err.uscode = htons((u_short)iError);
	(void)strcpy(pTPMake->u.err.acmsg, pcText);

	/* header + string + '\000' for string terminator
	 */
	return strlen(pcText) + (sizeof(pTPMake->usopcode)+sizeof(pTPMake->u.err.uscode) + 1);
}

/*@Explode ack@*/
/* Fill in the ACK packet.						(ksb)
 * Return the sizeof the packet.
 */
int
tftp_ack(pTPMake, usBlock)
register TFTP_PACKET *pTPMake;
u_short usBlock;
{
	pTPMake->usopcode = htons((u_short)ACK);
	pTPMake->u.ack.usblock = htons(usBlock);
	return sizeof(pTPMake->usopcode)+sizeof(pTPMake->u.ack.usblock);
}

/*@Explode pack@*/
/* Pack local data from the FILE* for network transport			(ksb)
 * call with (FILE *)0 to reset the block number, return the packet size.
 */
int
tftp_pack(pTPMake, pcMode, fpFrom)
register TFTP_PACKET *pTPMake;
const char *pcMode;
FILE *fpFrom;
{
	register char *pcBuf;
	register int iCursor, cCur;
	static u_short usSeq = 0;
	static int cPending = -1;

	if ((FILE *)0 == fpFrom) {
		usSeq = 0;
		cPending = -1;
		return 0;
	}

	++usSeq;
	pTPMake->usopcode = htons((u_short)DATA);
	pTPMake->u.data.usblock = htons(usSeq);
	if ('o' == *pcMode) {
		iCursor = fread((void *)pTPMake->u.data.acbuf, 1, sizeof(pTPMake->u.data.acbuf), fpFrom);
	} else {
		/* convert from UNIX to "netascii" (rfc764) stream
		 * \n -> \r\n
		 * \r -> \r\000
		 */
		pcBuf = pTPMake->u.data.acbuf;
		for (iCursor = 0; iCursor < sizeof(pTPMake->u.data.acbuf); ++iCursor) {
			if (-1 != cPending) {
				cCur = cPending;
				cPending = -1;
			} else if (EOF == (cCur = getc(fpFrom))) {
				break;
			} else if ('\n' == cCur || '\r' == cCur) {
				cPending = '\r' == cCur ? '\000' : '\n';
				cCur = '\r';
			}
			*pcBuf++ = cCur;
		}
	}
	return iCursor + sizeof(pTPMake->usopcode)+sizeof(pTPMake->u.data.usblock);
}

/*@Explode unpack@*/
/* Unpack network data from packet to FILE* for local delivery.		(ksb)
 * We let the caller check the sequence (we don't care).  Return the
 * number of characters output. iLen is the length of the packet,
 * not the data.
 */
int
tftp_unpack(pTPMade, iLen, pcMode, fpTo)
register TFTP_PACKET *pTPMade;
register int iLen;
const char *pcMode;
FILE *fpTo;
{
	static u_short usSeq = 0;
	static int cPending = -1;
	register char *pcBuf;
	register int iCursor, iWrote;

	if ((FILE *)0 == fpTo) {
		usSeq = 0;
		cPending = -1;
		return 0;
	}

	/* remove the header overhead from the packet length
	 */
	iLen -= sizeof(pTPMade->usopcode)+sizeof(pTPMade->u.data.usblock);
	if ('o' == *pcMode) {
		return fwrite((void*)pTPMade->u.data.acbuf, 1, iLen, fpTo);
	}
	pcBuf = pTPMade->u.data.acbuf;
	iWrote = 0;
	for (iCursor = 0; iCursor < iLen; ++iCursor, ++pcBuf) {
		/* \r\n -> \n, \r\000 -> \r
		 * \r[^\n\000] -> \r
		 */
		if (-1 != cPending) {
			if ('\n' == *pcBuf)
				putc('\n', fpTo);
			else /*( if ('\000' == *pcBuf) */
				putc('\r', fpTo);
			cPending = -1;
			++iWrote;
			continue;
		}
		if ('\r' == *pcBuf) {
			cPending = '\r';
			continue;
		}
		putc(*pcBuf, fpTo);
		++iWrote;
	}
	return iWrote;
}

/*@Explode hunk@*/
/* To transmit a slab of memory to a client/peer.			(ksb)
 * Setup the control data with:
 *	pcNewMode = tftp_output(pcMode, pvData, iLen);
 *
 * Then (connect to a client and) call
 *	tftp_put((FILE *), pcName, pcNewMode);
 * that will send the buffer to the remote client with the name you want.
 * (That put can be repeated many times.)
 *
 * When you are done, to cleanup, you must call:
 *	tftp_forget(pcNewMode);
 * we don't care how you allocate or free the (void *)'s memory.
 */
typedef struct TOnode {
	void *pvmagic;
	struct TOnode *pTOnext;
	char *pcslab;
	int islen;
	int cpending;
	int isent;
	u_short uiseq;
} TFTP_OSLAB;

static TFTP_OSLAB *pTOGlobal =	/* this is used to clean them all	*/
	(TFTP_OSLAB *)0;	/* XXX would be nifty in tftp client	*/

/* Setup a hunk of memory to be transmitted to a client/peer		(ksb)
 * by calling this function with the mode, (void *) and the length.
 * We return a _new_mode_  you _must_ use that mode as the tftp_put mode,
 * or (char *)0 for failure.
 */
static char *
tftp_output(pcMode, pvData, iLen)
register char *pcMode;
register void *pvData;
register int iLen;
{
	register TFTP_OSLAB *pTONew;
	register int iModelen;
	register char *pcKey;

	if ((char *)0 == pcMode) {
		pcMode = "octal";
	}
	iModelen = (strlen(pcMode)|7)+1;
	pTONew = (TFTP_OSLAB *)malloc(sizeof(TFTP_OSLAB)+iModelen);
	if ((TFTP_OSLAB *)0 == pTONew) {
		return (char *)0;
	}
	pTONew->islen = iLen;
	pTONew->pcslab = (char *)pvData;
	pTONew->uiseq = 0;
	pTONew->cpending = -1;
	pTONew->pTOnext = pTOGlobal;
	pTOGlobal = pTONew;
	pcKey = (char *)(pTONew+1);
	pTONew->pvmagic = (void *)pcKey;
	return strcpy(pcKey, pcMode);
}

/* copy data from a buffer (recorded above) into a network packet	(ksb)
 */
static int
tftp_hunk(pTPMake, pcMode, fpFrom)
register TFTP_PACKET *pTPMake;
const char *pcMode;
FILE *fpFrom;
{
	register TFTP_OSLAB *pTOCur;
	register char *pcBuf, *pcSrc;
	register int iCursor, cCur;

	if ((char *)0 == pcMode) {
		return -1;
	}
	pTOCur = ((TFTP_OSLAB *)pcMode)-1;
	if (pTOCur->pvmagic != (void *)pcMode) {
		return -1;
	}
	if ((TFTP_PACKET *)0 == pTPMake) {
		pTOCur->uiseq = 0;
		pTOCur->cpending = -1;
		pTOCur->isent = 0;
		return 0;
	}

	++pTOCur->uiseq;
	pTPMake->usopcode = htons((u_short)DATA);
	pTPMake->u.data.usblock = htons(pTOCur->uiseq);
	pcSrc = pTOCur->pcslab+pTOCur->isent;
	if ('o' == *pcMode) {
		iCursor = pTOCur->islen - pTOCur->isent;
		if (iCursor > sizeof(pTPMake->u.data.acbuf))
			iCursor = sizeof(pTPMake->u.data.acbuf);
		bcopy((void *)pcSrc, (void *)pTPMake->u.data.acbuf, iCursor);
		pTOCur->isent += iCursor;
	} else {
		/* convert from UNIX to "netascii" (rfc764) stream
		 * \n -> \r\n
		 * \r -> \r\000
		 */
		pcBuf = pTPMake->u.data.acbuf;
		for (iCursor = 0; iCursor < sizeof(pTPMake->u.data.acbuf); ++iCursor) {
			if (-1 != pTOCur->cpending) {
				cCur = pTOCur->cpending;
				pTOCur->cpending = -1;
			} else if (pTOCur->isent == pTOCur->islen) {
				break;
			} else {
				cCur = *pcSrc++, ++pTOCur->isent;
				if ('\n' == cCur || '\r' == cCur) {
					pTOCur->cpending = '\r' == cCur ? '\000' : '\n';
					cCur = '\r';
				}
			}
			*pcBuf++ = cCur;
		}
	}
	return iCursor + sizeof(pTPMake->usopcode)+sizeof(pTPMake->u.data.usblock);
}

/* Forget that we new this buffer					(ksb)
 * This was a slab of data we sent out via tftp_put, now we are done --
 * so free the control block.  If you pick (char *)0 we forget them all.
 */
static void
tftp_forget(pcNewMode)
char *pcNewMode;
{
	register TFTP_OSLAB *pTOOld, **ppTOList, *pTOScan;
	register void *pvMagic;

	if ((char *)0 == pcNewMode) {
		while ((TFTP_OSLAB *)0 != pTOGlobal) {
			pTOScan = pTOGlobal->pTOnext;
			free((void *)pTOGlobal);
			pTOGlobal = pTOScan;
		}
		return;
	}

	if ((void *)0 == (pvMagic = (void *)pcNewMode)) {
		return;
	}
	pTOOld = ((TFTP_OSLAB *)pvMagic)-1;
	if (pvMagic != pTOOld->pvmagic) {
		return;
	}
	ppTOList = & pTOGlobal;
	while ((TFTP_OSLAB *)0 != (pTOScan = *ppTOList)) {
		if (pTOOld != pTOScan) {
			ppTOList = & pTOScan->pTOnext;
		}
		*ppTOList = pTOScan->pTOnext;
		break;
	}
	free((void *)pTOOld);
}

/*@Explode ram@*/
/* Unpack network data from packet to a local buffer.			(ksb)
 * Call tftp_get((FILE *), pcName, pcMode) to use a RAM buffer.
 *
 * Then		pcBuf = tftp_ram((TFTP_PACKET *)0, 1, (char *)0);
 * if you want to keep the buffer, s/1/0/ if you don't.
 * When you keep the buffer we won't reuse it and you HAVE TO FREE it.
 */
char *
tftp_ram(pTPMade, iLen, pcMode)
register TFTP_PACKET *pTPMade;
register int iLen;
const char *pcMode;
{
	static u_short usSeq = 0;
	static int cPending = -1;
	register char *pcBuf;
	register int iCursor ;
	static int iMark = 0, iMax = 0;
	static char *pcRAM = (char *)0;
	register char *pcMem;

	/* When we have data, terminate the buffer with a '\000'
	 * and return a pointer to the first character. Else NULL.
	 */
	if ((TFTP_PACKET *)0 == pTPMade) {
		usSeq = 0;
		cPending = -1;
		if ((char *)0 != pcRAM) {
			pcRAM[iMark] = '\000';
		}
		iMark = 0;
		pcMem = pcRAM;
		if (0 != iLen) {
			pcRAM = (char *)0, iMax = 0;
		}
		return pcMem;
	}

	/* remove the header overhead from the packet length
	 */
	iLen -= sizeof(pTPMade->usopcode)+sizeof(pTPMade->u.data.usblock);
	iCursor = iMax;
	if ((char *)0 == (pcMem = pcRAM)) {
		iMax = 10240;		/* average tftp file size? */
		pcMem = calloc(iMax, sizeof(char));
	} else if (iMax < iLen + iMark + 2) {
		iMax += iLen + 3 * sizeof(pTPMade->u.data.acbuf);
		pcMem = realloc(pcRAM, iMax * sizeof(char));
	}

	/* Out of core, sigh -- free stuff, dude;
	 * else we are clear to process - ksb.
	 */
	if ((char *)0 == pcMem) {
		iMax = iCursor;
		return (char *)0;
	}
	pcRAM = pcMem;

	if ('o' == *pcMode) {
#if USE_BCOPY
		bcopy((void*)pTPMade->u.data.acbuf, pcRAM+iMark, iLen);
#else
		memcpy(pcRAM+iMark, (void*)pTPMade->u.data.acbuf, iLen);
#endif
		iMark += iLen;
		return pcRAM;
	}
	pcBuf = pTPMade->u.data.acbuf;
	for (iCursor = 0; iCursor < iLen; ++iCursor, ++pcBuf) {
		/* \r\n -> \n, \r\000 -> \r
		 * \r[^\n\000] -> \r
		 */
		if (-1 != cPending) {
			if ('\n' == *pcBuf)
				pcRAM[iMark++] = '\n';
			else /*( if ('\000' == *pcBuf) */
				pcRAM[iMark++] = '\r';
			cPending = -1;
			continue;
		}
		if ('\r' == *pcBuf) {
			cPending = '\r';
			continue;
		}
		pcRAM[iMark++] = *pcBuf;
	}
	return pcRAM;
}
