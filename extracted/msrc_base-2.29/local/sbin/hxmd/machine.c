/* $Id: machine.c,v 1.2 2006/01/10 14:00:45 ksb Exp $
 * HPUX9 doesn't have snprintf.
 */
#include <stdio.h>
#include "machine.h"

#if EMU_SNPRINTF
#include <varargs.h>

int	/* ARGSUSED */
snprintf(va_alist)
va_dcl
{
	va_list ap;
	int _;
	char *pc, *format;
	register int iRet;

	va_start(ap);
	pc = va_arg(ap, char *);
	_ = va_arg(ap, int);
	format = va_arg(ap, char *);
	iRet = vsprintf(pc, format, ap);
	va_end(ap);
	return iRet;
}
#endif
