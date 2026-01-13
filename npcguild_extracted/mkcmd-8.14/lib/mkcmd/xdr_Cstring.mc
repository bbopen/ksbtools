/* $Id: xdr_Cstring.mc,v 8.12 1998/09/30 12:25:33 ksb Exp $
 * serialize and deserialize C strings as ksb would			(ksb)
 * We send the length first (as u_int)
 *	0	NULL pointer [no data send]
 *	1	empty string [no data sent]
 *	2...	data follows, including \000 on the end
 */
bool_t
%%K<Cstring>2ev(xdrs, ppcData)
register XDR *xdrs;
register char **ppcData;
{
	auto unsigned int iSize;

	switch (xdrs->x_op) {
	case XDR_ENCODE:
		if ((char *)0 == *ppcData) {
			iSize = 0;
			return xdr_u_int(xdrs, & iSize);
		}
		iSize = strlen(*ppcData)+1;
		if (FALSE == xdr_u_int(xdrs, & iSize)) {
			return FALSE;
		}
		if (1 == iSize) {
			/* don't bother sending just a '\000'
			 */
			return TRUE;
		}
		break;
	case XDR_DECODE:
		if (FALSE == xdr_u_int(xdrs, & iSize))
			return FALSE;
		if (0 == iSize) {
			*ppcData = (char *)0;
			return TRUE;
		}
		*ppcData = (char *)malloc((iSize|(sizeof(long)-1))+1);
		if ((char *)0 == *ppcData) {
			/* ZZZ how do we cleanup the XDR stream here ? */
%			fprintf(stderr, "%%s: %K<Cstring>2ev: %%d: out of memory\n", %b, iSize)%;
			return FALSE;
		}
		if (1 == iSize) {
			/* optimize out the empty string from the stream
			 */
			ppcData[0][0] = '\000';
			return TRUE;
		}
		break;
	case XDR_FREE:
		if ((char *)0 != *ppcData) {
			free((void *)*ppcData);
			*ppcData = (char *)0;
		}
		return TRUE;
	default:	/* unknown xdr direction */
		return FALSE;
	}
	return xdr_opaque(xdrs, *ppcData, iSize);
}
