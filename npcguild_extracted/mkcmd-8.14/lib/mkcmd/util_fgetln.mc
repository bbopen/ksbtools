/* $Id: util_fgetln.mc,v 8.8 1999/06/14 13:27:42 ksb Exp $
 */

/* return a pointer to the next line in the stream			(ksb)
 * set *piRet = strlen(the_line); but NO '\000' is appended
 * this is really cheap if implemented in stdio.h, but not so
 * cool when we have to emulate it.  Sigh.
 *
 * N.B. We grab RAM in 128 byte increments, because we never free it.
 * [the stdio implementation frees it when you fclose() the fp, of course]
 */
char *
fgetln(fp, piRet)
FILE *fp;
size_t *piRet;
{
	static char *pcFGBuf = (char *)0;
	static size_t iSize = 0;
	register size_t iRead;
	register int c;
	register char *pc;	/* realloc test */

	if (0 == iSize) {
%		iSize = %K<fgetln_tunes>1v%;
		pcFGBuf = calloc(iSize, sizeof(char));
	}

	/* inv. the buffer has room for the next character at least
	 */
	for (iRead = 0; EOF != (c = getc(fp)); /* nothing */) {
		if ('\n' == (pcFGBuf[iRead++] = c))
			break;
		if (iRead < iSize)
			continue;
%		iSize += %K<fgetln_tunes>2v%;
		pc = realloc(pcFGBuf, iSize * sizeof(char));
		if ((char *)0 == pc)
			return (char *)0;
		pcFGBuf = pc;
	}
	pcFGBuf[iRead] = '\000';
	if ((size_t *)0 != piRet)
		*piRet = iRead;

	return (0 == iRead) ? (char *)0 : pcFGBuf;
}
