/* $Id: debug.c,v 3.0 1992/04/22 13:23:26 ksb Stab $
 */
#include <sys/types.h>
#include <sys/param.h>

#include "libtomb.h"
#include "debug.h"

#if defined DEBUG
/*
 * stuff for debugging
 *
 * Matthew Bradburn, Purdue University Computing Center
 */

#include <stdio.h>
#include <strings.h>
#include <varargs.h>

static FILE *pfTrace;


/*
 * DebugInit -- do the dumb stuff I gotta do.  Called by the other functions
 *	in this file, if it hasn't been called already.
 */
void
DebugInit()
{
	static char acFile[100];	/* the trace file's name	*/

	/* create the trace file's name: "TRACE_PATH/entomb.$$"	*/
	(void)sprintf(acFile, "%s/%s.%d", TRACE_PATH, "entomb", getpid());

	/* open the trace file		*/
	if (NULL == (pfTrace = fopen(acFile, "w"))) {
		(void)fprintf(stderr, "%s: fopen: ", acFile);
		perror(acFile);
		exit(NOT_ENTOMBED);
	}
	setbuf(pfTrace, (char *)0);
}


/*
 * Trace -- put output into the trace file, followed by a newline.
 */
void
Trace(va_alist)
va_dcl
{
	va_list args;
	static int tInit = 0;
	char *pcFmt;

	if (!tInit) {
		DebugInit();
		tInit = 1;
	}

	va_start(args);
	pcFmt = va_arg(args, char *);
	vfprintf(pfTrace, pcFmt, args);
	va_end(args);
	(void)putc('\n', pfTrace);
}
#endif
