/* $Id: xdr_Cargv.mc,v 8.11 1998/09/30 12:14:10 ksb Exp $
 * serialize and deserialize main's argv (or the like) as ksb would	(ksb)
 *	0 (char **)0
 *	1 { (char *)0 }
 *	2 { "argv0", (char *)0 }
 *
 * <number of element>
 * [ total bytes in C strings we will send
 *     <size of argv[n]> [<bytes for argv[n]>]	{for each string in argv}
 */
bool_t
%%K<Cargv>2ev(xdrs, pargv)
register XDR *xdrs;
register char **(*pargv);
{
	auto unsigned int iData;
	register int iVec, iBlob;
	register char **ppc, *pcSend;

	switch (xdrs->x_op) {
	case XDR_ENCODE:
		/* encode (char **)0 as (unsigned) 0
		 */
		if ((char **)0 == (ppc = *pargv)) {
			iData = 0;
			return xdr_u_int(xdrs, & iData);
		}
		for (iVec = iBlob = 0; (char *)0 != ppc[iVec]; ++iVec) {
			iBlob += strlen(ppc[iVec])+1;
		}
		iData = iVec+1;
		if (FALSE == xdr_u_int(xdrs, & iData)) {
			return FALSE;
		}
		if (0 == iVec) {
			return TRUE;
		}

		iData = iBlob;
		if (FALSE == xdr_u_int(xdrs, & iData)) {
			return FALSE;
		}
		while ((char *)0 != (pcSend = *ppc++)) {
			iData = iBlob = strlen(pcSend)+1;
			if (FALSE == xdr_u_int(xdrs, & iData)) {
				return FALSE;
			}
			if (1 == iBlob) {
				continue;
			}
			if (FALSE == xdr_opaque(xdrs, pcSend, iBlob)) {
				return FALSE;
			}
		}
		return TRUE;
	case XDR_DECODE:
		if (FALSE == xdr_u_int(xdrs, & iData)) {
			return FALSE;
		}
		switch (iVec = iData) {
		case 0:
			*pargv = (char **)0;
			return TRUE;
		case 1:
			ppc = (char **)malloc(sizeof(char *));
			if ((char **)0 == (*pargv = ppc)) {
%				fprintf(stderr, "%%s: malloc: %%d: out of memory\n", %b, sizeof(char *))%;
				return FALSE;
			}
			*ppc = (char *)0;
			return TRUE;
		default:
			break;
		}

		if (FALSE == xdr_u_int(xdrs, & iData)) {
			return FALSE;
		}
		iBlob = iData;

		/* allocate it all, dude
		 */
		iBlob += iVec*sizeof(char *);
		ppc = (char **)malloc(iBlob);
		if ((char **)0 == (*pargv = ppc)) {
%			fprintf(stderr, "%%s: malloc: %%d: out of memory\n", %b, iBlob)%;
			return FALSE;
		}
		pcSend = (char *)& ppc[iVec];
		while (--iVec > 0) {
			*ppc++ = pcSend;
			if (FALSE == xdr_u_int(xdrs, & iData)) {
				return FALSE;
			}
			if (1 == iData) {
				*pcSend++ = '\000';
				continue;
			}
			if (FALSE == xdr_opaque(xdrs, pcSend, iData)) {
				return FALSE;
			}
			pcSend += strlen(pcSend)+1;
		}
		*ppc = (char *)0;
		return TRUE;
	case XDR_FREE:
		if ((char **)0 != (ppc = *pargv)) {
			free((void *)ppc);
			*pargv = (char **)0;
		}
		return TRUE;
	default:	/* unknown xdr direction */
		break;
	}
	return FALSE;
}
