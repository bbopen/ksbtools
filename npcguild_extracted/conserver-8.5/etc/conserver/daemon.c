/* $Id: daemon.c,v 8.2 2000/07/25 23:14:10 ksb Exp $
 * Joe daemon								(ksb)
 * (define INETD if Joe is to be started by inetd.)
 *
 * $Compile: ${cc-cc} ${cc_debug--O} -DTEST %f -o %F
 *	-DDYNIX		for dynix systems
 *	-DBSD		for dynix3 and other BSD systems
 *	-DPOSIX		for POSIX (half-hearted)
 *	-DINETD		if required to run under inetd
 *
 *	the default is some sort of SYSV
 */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>

#include "machine.h"

#ifdef BSD
#include <sys/ioctl.h>
#include <strings.h>
#else
#include <string.h>
#endif

#ifdef POSIX
#include <limits.h>
#include <unistd.h>
#endif

#if NEED_GNU_TYPES
#include <gnu/types.h>
#endif

#if HAVE_SELECT_H
#include <sys/select.h>
#endif

extern char *progname;
extern int errno;

/*
 * begin to act like a daemon; call func on signals 			(ksb)
 */
void
Daemon(func, fd_ignore, fDetatch)
SIGRET_T (*func)();
fd_set *fd_ignore;
int fDetatch;
{
	register int tt, lim, iHold;
	register SIGRET_T (*oldfunc)();

#ifdef DYNIX
	lim = getdtablesize();
#else
#ifdef POSIX
	lim = OPEN_MAX;		/* from <limits.h>	*/
#else
	lim = NOFILE;
#endif /* POSIX */
#endif /* DYNIX */

	if (fDetatch) {
		switch (fork()) {
		case -1:
			fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
			exit(1);
		default:
			exit(0);
		case 0:
			break;
		}
	}
	for (tt = 0; tt < lim; ++tt) {
		if (! FD_ISSET(tt, fd_ignore))
			(void) close(tt);
	}

	iHold = -1;
	for (tt = 0; tt < 3; ++tt) {
		auto int iJunk;
		if (-1 == fcntl(tt, F_GETFD, & iJunk)) {
			if (-1 == iHold)
				iHold = open("/dev/null", 2, 0666);
			else
				dup(iHold);
		}
	}

	if (fDetatch) {
#if defined TIOCNOTTY
		tt = open("/dev/tty", 2);
		if (tt >= 0) {
			ioctl(tt, TIOCNOTTY, 0);
			close(tt);
		}
#else
		setsid();
#endif
	}
	if (SIG_DFL == func) {
		return;
	}
	if (SIG_DFL != (oldfunc = signal(SIGHUP, func))) {
		signal(SIGHUP, oldfunc);
	}
	if (SIG_DFL != (oldfunc = signal(SIGINT, func))) {
		signal(SIGINT, oldfunc);
	}
	if (SIG_DFL != (oldfunc = signal(SIGQUIT, func))) {
		signal(SIGQUIT, oldfunc);
	}
	if (SIG_DFL != (oldfunc = signal(SIGALRM, func))) {
		signal(SIGALRM, oldfunc);
	}
	if (SIG_DFL != (oldfunc = signal(SIGTERM, func))) {
		signal(SIGTERM, oldfunc);
	}
	if (SIG_DFL != (oldfunc = signal(SIGPIPE, func))) {
		signal(SIGPIPE, oldfunc);
	}
/*
 * SYSV doesn't have either SIGXCPU or SIGURG
 */
#if defined SIGXCPU
	if (SIG_DFL != (oldfunc = signal(SIGXCPU, func))) {
		signal(SIGXCPU, oldfunc);
	}
#endif
#if defined SIGURG
	if (SIG_DFL != (oldfunc = signal(SIGURG, func))) {
		signal(SIGURG, oldfunc);
	}
#endif
}
