/* $Id: ls.c,v 3.4 1994/07/11 00:15:43 ksb Exp $
 * Ls -- list the current working directory
 *	the arguments to this function is the command-line arguments
 *	to give to ls.
 */
#include <stdio.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "libtomb.h"
#include "main.h"
#include "lists.h"
#include "init.h"

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

extern int errno;
#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me)	(sys_errlist[Me])
#endif


/* fork of a /bin/ls of the current directory				(ksb)
 */
int	/*ARGSUSED*/
cm_Ls(argc, argv)
int argc;
char **argv;
{
	register int iPid, iLsPid;
#if HAS_UNIONWAIT
	auto union wait status;
#else
	auto int status;
#endif

	argv[0] = "ls";		/* of course we could have been `dir' */
	switch (iLsPid = fork()) {
	case -1:
		(void)fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		exit(1);
		break;

	case 0:	/* the child */
		user();
		execvp("ls", argv);
		break;

	default: /* the parent */
		while (-1 != (iPid = wait(&status))) {
			if (iLsPid != iPid) {
				continue;
			}
#if HAS_UNIONWAIT
			if (0 != status.w_retcode) {
				(void)fprintf(stderr, "%s: ls failed (%d)\n", progname, status.w_retcode);
			}
#else
			if (0 != status) {
				(void)fprintf(stderr, "%s: ls failed (%d)\n", progname, 0xff&(status >> 8));
			}
#endif
			break;
		}
		break;
	}
}
