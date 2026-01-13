/* $Id: util_rr.mc,v 8.10 2008/11/15 14:18:14 ksb Exp $
 * The internal structure rrpoll uses is 100% opaque -- ksb
 */
typedef struct DWnode {
	struct hostent HE;		/* fake hostent structure	*/
	char *apcalias[2];		/* pseudo alias list		*/
	char achost[MAXHOSTNAMELEN+4];	/* keep the name we asked about	*/
	time_t tlast;			/* last time we looked		*/
	PPM_BUF PMindex;		/* index of addresses		*/
	PPM_BUF PMaddrs;		/* list of addresses		*/
} DNS_WATCHER;

/* Call me with a DNS name to watch, I'll give you a handle.		(ksb)
 */
void *
util_rrwatch(char *pcRR)
{
	register DNS_WATCHER *pDWRet;

	if (0 == (_res.options & RES_INIT)) {
		res_init();
	}
	if ((char *)0 == pcRR || '\000' == *pcRR || MAXHOSTNAMELEN <= strlen(pcRR) || (DNS_WATCHER *)0 == (pDWRet = malloc(sizeof(DNS_WATCHER)))) {
		return (void *)0;
	}
	util_ppm_init(& pDWRet->PMindex, sizeof(char *), 0);
	util_ppm_init(& pDWRet->PMaddrs, MAXKEEPADDR, 0);
	pDWRet->tlast = time((void *)0)-2;
	pDWRet->HE.h_name = strncpy(pDWRet->achost, pcRR, sizeof(pDWRet->achost));
	pDWRet->HE.h_aliases = pDWRet->apcalias;
	pDWRet->apcalias[0] = pDWRet->achost;
	pDWRet->apcalias[1] = (char *)0;
	pDWRet->HE.h_addrtype = AF_INET;
	pDWRet->HE.h_length = 0;
	pDWRet->HE.h_addr_list = util_ppm_size(& pDWRet->PMindex, 24);
	pDWRet->HE.h_addr_list[0] = (char *)0;

	return (void *)pDWRet;
}

/* Give me the handle and I'll poll for updates to the target RR,	(ksb)
 * then return a fake hostent for your enjoyment.
 */
struct hostent *
util_rrpoll(void *pvHandle)
{
	register DNS_WATCHER *pDWCur;
	register u_char *pucCursor, *pucEnd;
	register u_char (*paucMath)[MAXKEEPADDR];
	auto int iSize, iAnswers, iSkip, iType, iAnswerSize, i;
	auto long lTTL, lTTLMin;
	auto time_t tNow;
	auto u_char aucAnswer[PACKETSZ];
	auto HEADER *hHeader;

	if ((DNS_WATCHER *)0 == (pDWCur = pvHandle) || '\000' == pDWCur->achost[0]) {
		return (struct hostent *)0;
	}
	tNow = time((void *)0);
	if (tNow < pDWCur->tlast) {
		return & pDWCur->HE;
	}

	iSize = res_search(pDWCur->achost, C_IN, T_A, aucAnswer, sizeof(aucAnswer));
	if (-1 == iSize) {
%		fprintf(stderr, "%%s: res_search: %%s: %%s\n", %b, pDWCur->achost, hstrerror(h_errno))%;
		return (struct hostent *)0;
	}

	if (iSize > sizeof(aucAnswer)) {
%		fprintf(stderr, "%%s: res_search: fixed size too small (%%d > %%d)\n", %b, iSize, sizeof(aucAnswer))%;
		iSize = sizeof(aucAnswer);
	}

	hHeader = (HEADER *)aucAnswer;
	pucCursor = aucAnswer + sizeof(*hHeader);
	pucEnd = aucAnswer + iSize;

	/* skip questions section
	 */
	for (i = ntohs((unsigned short)hHeader->qdcount); i--; pucCursor += iSkip + QFIXEDSZ) {
		if (-1 == (iSkip = dn_skipname(pucCursor, pucEnd))) {
			break;
		}
	}

	i = 0;
	lTTLMin = LONGKEEPTTL;	/* we look every hour at least */
	for (iAnswers = ntohs((unsigned short)hHeader->ancount); iAnswers > 0; --iAnswers) {
		if (-1 == (iSkip = dn_skipname(pucCursor, pucEnd))) {
%			fprintf(stderr, "%%s: dn_skipname: skipname failed\n", %b)%;
			break;
		}
		pucCursor += iSkip;
		GETSHORT(iType, pucCursor);
		pucCursor += INT16SZ;
		GETLONG(lTTL, pucCursor);
		GETSHORT(iAnswerSize, pucCursor);

		if (iType == T_A) {
			pDWCur->HE.h_addrtype = iType;
			pDWCur->HE.h_length = iAnswerSize;
			paucMath = util_ppm_size(& pDWCur->PMaddrs, (i|15)+1);
			memcpy((void *)(paucMath+i), pucCursor, iAnswerSize);
			if (lTTL < lTTLMin)
				lTTLMin = lTTL;
			++i;
		}
		pucCursor += iAnswerSize;
		if (pucCursor >= pucEnd) {
			break;
		}
	}
	pDWCur->HE.h_addr_list = util_ppm_size(& pDWCur->PMindex, (i|7)+1);
	paucMath = util_ppm_size(& pDWCur->PMaddrs, 0);
	for (iAnswers = 0; iAnswers < i; ++iAnswers) {
		pDWCur->HE.h_addr_list[iAnswers] = (char *)& paucMath[iAnswers];
	}
	pDWCur->HE.h_addr_list[i] = (char *)0;
	pDWCur->tlast = tNow + lTTLMin;
	return & pDWCur->HE;
}

/* Release the node we are watching					(ksb)
 */
void
util_rrrelease(void *pvHeld)
{
	register DNS_WATCHER *pDWHeld;

	if ((DNS_WATCHER *)0 == (pDWHeld = pvHeld))
		return;
	util_ppm_free(& pDWHeld->PMindex);
	util_ppm_free(& pDWHeld->PMaddrs);
	free(pvHeld);
}
