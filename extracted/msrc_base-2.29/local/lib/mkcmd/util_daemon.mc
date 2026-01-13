/* $Id: util_daemon.mc,v 8.10 2008/11/15 20:15:48 ksb Exp $
 */

/* begin to act like a daemon; call func on signals 			(ksb)
 */
%#if %K-1v
void Daemon(SIGRET_T (*func)(), fd_set *fd_ignore, int fDetatch)
#else
void
Daemon(func, fd_ignore, fDetatch)
SIGRET_T (*func)();
fd_set *fd_ignore;
int fDetatch;
#endif
{
	register int tt, lim, iHold;
	register SIGRET_T (*oldfunc)();

#if defined(OPEN_MAX)
	lim = OPEN_MAX;		/* from <limits.h>	*/
#if defined(NOFILE)
	lim = NOFILE;
#else
	lim = getdtablesize();
#endif
#endif
	if (fDetatch) {
		switch (fork()) {
		case -1:
%			fprintf(stderr, "%%s: fork: %%s\n", %b, %E)%;
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
