/* $Id: util_socket.mc,v 8.7 1997/10/27 11:55:22 ksb Exp $
 * mkcmd meta C code for socket command line parsing			(ksb)
 */

/* record/update the new requested remote name				(ksb)
 * we cannot write on the pcValue string because gcc and other
 * ANSI C compilers put "quoted strings" in the text (readonly) segment.
 */
void
%%K<SockCvt>1v(pSD, pcName, pcValue)
SOCK_DESCR *pSD;
char *pcName, *pcValue;
{
	register char *pcColon;
	register char *pcSlash;
	register int iHostLen, iProtoLen;

	pcSlash = (char *)0;
	iHostLen = sizeof(pSD->u_achost)-1;
	iProtoLen = sizeof(pSD->u_acport)-1;
	if ((char *)0 != (pcColon = strchr(pcValue, ':'))) {
		iHostLen = pcColon++ - pcValue;
		if ((char *)0 != (pcSlash = strchr(pcColon, '/')))
			iProtoLen = pcSlash++ - pcColon;
	}
	if ('\000' != *pcValue && 0 != iHostLen) {
		(void)strncpy(pSD->u_achost, pcValue, iHostLen);
		pSD->u_achost[iHostLen] = '\000';
	}
	if ((char *)0 != pcColon && !(0 == iProtoLen || '\000' == *pcColon)) {
		(void)strncpy(pSD->u_acport, pcColon, iProtoLen);
		pSD->u_acport[iProtoLen] = '\000';
	}
	if ((char *)0 != pcSlash && '\000' != *pcSlash) {
		pSD->u_pcproto = pcSlash;
	}
}
