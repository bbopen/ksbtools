/* $Id: atoc.c,v 8.0 1997/01/29 02:41:57 ksb Exp $
 */
#include <stdio.h>
#include "atoc.h"

/* This routine converts text into C quoted strings			(ksb)
 * It reads from a FILE * and writes to a FILE *; producing code with
 * the input C text between `pcpen` and `pclose`.
 *
 * The iOpts flag provides some options that are handy:
 *	TTOC_NOP		do nothing special
 *	TTOC_PERCENT		this is a printf format string, quote %'s
 *	TTOC_NEWLINE		always break strings at newlines
 *	TTOC_BLOCK		never break the string for newlines
 *
 * The iMax argument is the max lenght of an output string, pass 0 for
 * unlimited
 *
 * pcOpen and pcClose are output whenever the a new string it started
 * or a string is closed.
 * example:	pcpen = "\tfprintf(fp, \""
 *		pclose = "\");\n"
 */
void
atoc(pFIOut, pcIn, pcOpen, pcClose, iOpts, iMax)
register FILE *pFIOut;
char *pcIn, *pcOpen, *pcClose;
register int iOpts, iMax;
{
	register int c;
	register int bQuote;
	register int iLen;

	bQuote = iLen = 0;

	if (iMax < 0) {
		iMax = 0;
	}

	while ('\000' != (c = *pcIn++)) {
		if (! bQuote) {
			fputs(pcOpen, pFIOut);
			iLen = 0;
			bQuote = 1;
		}

		switch (c) {
		case '\\':
		case '"':
			putc('\\', pFIOut);
			putc(c, pFIOut);
			iLen += 2;
			break;
		case '\t':
			fputs("\\t", pFIOut);
			iLen += 2;
			break;
		case '\n':
			fputs("\\n", pFIOut);
			if (0 != (TTOC_NEWLINE & iOpts) || (0 == (TTOC_BLOCK & iOpts) && iLen > (iMax >> 1))) {
				iLen = iMax + 2;
			} else {
				iLen += 2;
			}
			break;
		case '%':
			if (0 != (iOpts & TTOC_PERCENT)) {
				fputs("%%", pFIOut);
				iLen += 2;
				break;
			}
			/*FALLTHROUGH*/
		default:
			if (c < ' ' || c > '~') {
				fprintf(pFIOut, "\\%03o", c);
				iLen += 4;
				break;
			}
			putc(c, pFIOut);
			iLen += 1;
			break;
		}

		if (0 != iMax && iLen > iMax) {
			fputs(pcClose, pFIOut);
			bQuote = 0;
		}
	}
	if (bQuote) {
		fputs(pcClose, pFIOut);
	}
}
