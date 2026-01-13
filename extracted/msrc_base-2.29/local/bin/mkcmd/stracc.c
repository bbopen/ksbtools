/* $Id: stracc.c,v 8.0 1997/01/29 02:41:57 ksb Exp $
 * accumulate a scanned token of any length
 *	$Compile: ${CC-cc} -c ${DEBUG--g} -I/usr/include/local %f
 */
#include <stdio.h>

#include "machine.h"

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#if USE_MALLOC_H
#include <malloc.h>
#else
#if NEED_MALLOC_EXTERN
extern char *malloc(), *calloc(), *realloc();
#endif
#endif

static char *pchBuf = (char *)0;
static unsigned long int lBufLen = 0, lCurBuf = 0;

/* allow the scanner to accumulate any number of characters for a token	(ksb)
 * n must be > 0
 */
void
FirstChar(c, n)
int c;
unsigned n;
{
	if ((char *)0 == pchBuf) {
		pchBuf = malloc(n);
		if ((char *)0 == pchBuf) {
			fprintf(stderr, "%s: out of memory.\n", "ll1");
			exit(0);
		}
	}
	lBufLen = n;
	lCurBuf = 1;
	pchBuf[0] = c;
}

/* add chars to our (maybe big buffer)					(ksb)
 */
void
AddChar(c, i)
int c;
unsigned i;
{
	if (lCurBuf == lBufLen) {
		lBufLen += i;
		pchBuf = realloc(pchBuf, lBufLen);
		if ((char *)0 == pchBuf) {
			fprintf(stderr, "%s: out of memory\n", "ll1");
			exit(1);
		}
	}
	pchBuf[lCurBuf++] = c;
}

/* Take buffer, make it ours:						(ksb)
 *  if we are using less than 1/2 the chars in the buffer,
 *  copy and save the big chunk for later use.
 */
char *
TakeChars(pu)
unsigned *pu;
{
	register char *pchRet;

	if ((unsigned *)0 != pu)
		*pu = lBufLen;

	if ((lBufLen>>1) > lCurBuf && lBufLen > 32) {
		pchRet = malloc(lBufLen);
		if ((char *)0 != pchRet) {
			bcopy(pchBuf, pchRet, lBufLen);
		} else {
			pchRet = pchBuf;
			pchBuf = (char *)0;
		}
	} else {
		pchRet = pchBuf;
		pchBuf = (char *)0;
	}
	return pchRet;
}

/* Take line, make it ours:						(ksb)
 *  same as TakeChars, but we have to have a literal newline on the
 *  end before the \000 we put on.  Or a semicolon or a close-curly
 *  because the Emit code routine fixes them.
 */
char *
TakeLine(pu)
unsigned *pu;
{
	static char acTerm[] = /*{*/ "\n};";
	register int fNull;

	fNull = lCurBuf > 0 && '\000' == pchBuf[lCurBuf-1];
	if (fNull) {
		--lCurBuf;
	}
	if (lCurBuf > 0 && (char *)0 != strchr(acTerm, pchBuf[lCurBuf-1])) {
		if (fNull) {
			AddChar('\000', 1);
		}
		return TakeChars(pu);
	}

	AddChar('\n', 2);
	if (fNull) {
		AddChar('\000', 1);
	}

	return TakeChars(pu);
}


/* use a buffer, promise not to keep it					(ksb)
 */
char *
UseChars(pu)
unsigned *pu;
{
	if ((unsigned *)0 != pu)
		*pu = lBufLen;
	return pchBuf;
}
