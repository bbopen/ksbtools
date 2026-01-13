/* $Id: purge.c,v 3.6 2008/11/17 18:59:05 ksb Exp $
 * Purge - delete files from the tomb
 */
#include <stdio.h>
#include <errno.h>
#include <sys/param.h>

#include "machine.h"
#include "libtomb.h"
#include "lists.h"
#include "init.h"
#include "purge.h"
#include "main.h"

#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me)	(sys_errlist[Me])
#endif

#if HAVE_NDIR
#include <ndir.h>
#else
#if HAVE_DIRENT
#include <dirent.h>
#else
#include <sys/dir.h>
#endif
#endif

/* remove the file from the crypt, return 0 if it went OK		(ksb)
 */
int
Purge(pLE)
LE *pLE;
{
	auto char acFile[1024];

	if ((char *)0 == real_name(acFile, pLE)) {
		return 1;
	}
	if (-1 == unlink(acFile)) {
		(void)fprintf(stderr, "%s: unlink: %s: %s\n", progname, acFile, strerror(errno));
		UnMark(pLE);
		return 1;
	}
	return 0;
}
