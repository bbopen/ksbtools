/* $Id: show.c,v 3.9 2008/11/17 18:59:05 ksb Exp $
 * do the show/page command (each command has a file)
 */
#include <stdio.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <stdlib.h>
#include <unistd.h>

#include "machine.h"
#include "libtomb.h"
#include "lists.h"
#include "init.h"
#include "show.h"
#include "main.h"

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me)	(sys_errlist[Me])
#endif

#if USE_FCNTL_H
#include <fcntl.h>
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


/* invoke the user's pager with it's standard input set to		(mjb)
 * to a entombed file
 */
int
Show(pLE)
LE *pLE;
{
	extern char *getenv();
	auto int iPid;
	register int i;
	register char *pcPager;
	auto char acRealFile[1024];

	if (NULL == (pcPager = getenv("PAGER"))) {
		pcPager = DEFPAGER;
	}

	if (NULL == real_name(acRealFile, pLE)) {
		(void)fprintf(stderr, "%s: %s: no such file\n", progname, pLE->name);
		return 1;
	}

	switch (iPid = fork()) {
	case -1:
		(void)fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		exit(1);
		break;
	case 0:
		/* the child */
		break;
	default:
		/* the parent -- wait for the kid */
		while (-1 != (i = wait((void *)0))) {
			if (iPid == i) {
				return 0;
			}
		}
		return 0;
	}

	/* child -- we had better not return!
	 */

	(void)close(0);
	if (0 != open(acRealFile, O_RDONLY, 000)) {
		(void)fprintf(stderr, "%s: freopen(%s): %s\n", progname, pLE->name, strerror(errno));
		exit(1);
	}

	drop();

	(void)execlp(pcPager, pcPager, (char *)0);
	(void)fprintf(stderr, "%s: execlp: %s\n", progname, strerror(errno));
	_exit(1);
	/*NOTREACHED*/
}
