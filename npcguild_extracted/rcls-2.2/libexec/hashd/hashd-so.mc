/* $Id: hashd-so.mc,v 2.1 2000/07/27 21:42:14 ksb Exp $
 */

/* get the next hash value from the socket				(ksb)
 */
char *
NetHash(pcHash)
char *pcHash;	/* points to at least HASH_LENGTH+1 characters	*/
{
	register char *pc;
	register int iReply, cc;
	auto char acReply[1024+HASH_LENGTH+4];

	/* open the hashd port and get the Word Up
	 */
	pcHash[0] = '\000';
	if (-1 == fdGarcon) {
%@foreach hashd_ports
%		%n = %K<SockOpen>/ef%;
%@endfor
	}
	cc = read(fdGarcon, acReply, sizeof(acReply));
	close(fdGarcon);
	fdGarcon = -1;

	if (0 >= cc || 230 != (iReply = atoi(acReply))) {
		return (char *)0;
	}
	/* skip reply code, and spaces
	 */
	for (pc = acReply; isdigit(*pc); ++pc) {
	}
	while (isspace(*pc)) {
		++pc;
	}
	pc[HASH_LENGTH] = '\000';
	return strncpy(pcHash, pc, HASH_LENGTH+1);
}
