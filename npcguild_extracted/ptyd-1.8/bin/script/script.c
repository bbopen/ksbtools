/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif

#ifndef lint
static char *sccsid = "@(#)script.c	5.4 (Berkeley) 11/13/85";
static char *rcsid  = "$Id: script.c,v 4.3.1.10 1997/11/29 23:48:54 ksb Exp $";
#endif

/*
 * script
 */
#include <sys/param.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#if !defined(sun)
#include <sys/ioctl.h>
#endif

#if defined(TERMIO)
# if defined(sun)
#  include <termios.h>
#  define  termio	termios
#  define  TCSETA	TCSETS
#  define  TCSETAW	TCSETSW
#  define  TCGETA	TCGETS
#  if defined(sparc)
#   define ADD		1
#   define REMOVE	(-1)
extern void utent();
#  endif
# else
#  include <unistd.h>
#  include <termio.h>
# endif
#else
# include <sgtty.h>
#endif

#include <sys/time.h>
#include <sys/file.h>

#if !defined(O_RDWR)
# include <sys/fcntl.h>
#endif

#if defined(_iPSC386)
# include <sys/stream.h>
# include <sys/ptem.h>
#endif

#if defined(OPENPTY)
# include "openpty.h"
#endif

#if !defined(SIGCHLD) && defined(SIGCLD)
# define  SIGCHLD SIGCLD
#endif

extern int errno;
extern char	*ctime(),
		*getenv();
extern void	perror();
extern long	time();
char	*shell;
FILE	*fscript;
int	master;
int	slave;
int	child;
int	subchild;
char	*fname = "typescript";
int	finish();

#if defined(SIGWINCH)
int		SigWinch();
#endif

#if !defined(TERMIO)
struct	sgttyb b;
struct	tchars tc;
struct	ltchars lc;
#else
struct	termio b;
#endif

#if defined(TIOCGWINSZ) && defined(TIOCSWINSZ)
struct	winsize win;
#endif

int	lb;
int	l;
int	aflg;
int	uflg;

#if defined(OPENPTY)
char	line[MAXPATHLEN+1], sline[MAXPATHLEN+1];
#else
# if defined(eta10) || defined(_iPSC386)
char	*line = "/dev/ptypXX";
# else
char	*line = "/dev/ptyXX";
# endif
#endif

main(argc, argv)
	int argc;
	char *argv[];
{

	shell = getenv("SHELL");
	if (shell == 0)
		shell = "/bin/sh";
	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {
		switch (argv[0][1]) {

		case 'a':
			aflg++;
			break;

		case 'u':
			uflg++;
			break;

		default:
			(void) fprintf(stderr, "usage: script [-au] [typescript]\n");
			exit(1);
		}
		argc--, argv++;
	}
	if (argc > 0)
		fname = argv[0];
	if ((fscript = fopen(fname, aflg ? "a" : "w")) == NULL) {
		perror(fname);
		fail();
		/* NOTREACHED */
	}
	getmaster();
	(void) printf("Script started, file is %s\n", fname);
	fixtty();

	(void) signal(SIGCHLD, finish);
	child = fork();
	if (child < 0) {
		perror("fork");
		fail();
		/* NOTREACHED */
	}
	if (child == 0) {
#if defined(eta10) || defined(_iPSC386)
/*
 * Get the slave side before the fork.  On some platforms (see #if), the
 * read(master, ...) executes too soon and returns a premature EOF if
 * the slave side ain't there yet.  This in turn causes an immediate
 * termination of "script".
 */
		getslave();
#endif
		subchild = child = fork();
		if (child < 0) {
			perror("fork");
			fail();
			/* NOTREACHED */
		}
		if (child)
			dooutput();
		else
			doshell();
		/* NOTREACHED */
	}
	doinput();
	/* NOTREACHED */
}

doinput()
{
	char ibuf[BUFSIZ];
	int cc;

	(void) fclose(fscript);
#if defined(SIGWINCH) && defined(TIOCGWINSZ) && defined(TIOCSWINSZ)
	(void) signal(SIGWINCH, SigWinch);
#endif
	while ((cc = read(0, ibuf, BUFSIZ)) > 0)
		(void) write(master, ibuf, cc);
	done();
	/* NOTREACHED */
}

#if defined(SIGWINCH) && defined(TIOCGWINSZ) && defined(TIOCSWINSZ)
int
SigWinch()
{
	struct winsize	winch;

	if (ioctl(0, TIOCGWINSZ, (char *) &winch) == 0)
		(void) ioctl(master, TIOCSWINSZ, (char *) &winch);
}
#endif

#include <sys/wait.h>
#include <sys/resource.h>

finish()
{
#if !defined(_AIX)
	union wait status;
	register int pid;
#else
	int		status;
	register pid_t	pid;
#endif
	register int die = 0;

	while ((pid = wait3(&status, WNOHANG, (struct rusage *)0)) > 0)
		if (pid == child)
			die = 1;

	if (die)
	{
		done();
		/* NOTREACHED */
	}
}

dooutput()
{
	long tvec;
	char obuf[BUFSIZ];
	int cc;

#if defined(eta10) || defined(_iPSC386)
	(void) close(slave);
#endif
	(void) close(0);
	tvec = time((long *)0);
	(void) fprintf(fscript, "Script started on %s", ctime(&tvec));
#if defined(_AIX)
	(void) fcntl(master, F_SETFL, 0);
#endif
	for (;;) {
		cc = read(master, obuf, sizeof (obuf));
		if (cc <= 0)
			break;
		(void) write(1, obuf, cc);
		(void) fwrite(obuf, 1, cc, fscript);
	}
	done();
	/* NOTREACHED */
}

doshell()
{
	int t;

#if defined(TERMIO)
	setpgrp();
#endif
#if !defined(eta10) && !defined(_iPSC386)
	t = open("/dev/tty", O_RDWR, 0);
	if (t >= 0) {
		(void) ioctl(t, TIOCNOTTY, (char *)0);
		(void) close(t);
	}
	getslave();
#endif
	(void) close(master);
	(void) fclose(fscript);
	(void) dup2(slave, 0);
	(void) dup2(slave, 1);
	(void) dup2(slave, 2);
	if (slave > 2) {
		(void) close(slave);
	}
#if defined(sparc) && !defined(OPENPTY)
	utent(ADD);
#endif
	(void) execl(shell, "sh", "-i", 0);
	perror(shell);
	fail();
	/* NOTREACHED */
}

fixtty()
{
#if !defined(TERMIO)
	struct ltchars lbuf;
	struct sgttyb sbuf;
	struct tchars tbuf;
#else
	struct termio sbuf;
#endif

	sbuf = b;
#if !defined(TERMIO)
	sbuf.sg_flags |= CBREAK;	/* CBREAK handles parity */
	sbuf.sg_flags &= ~(ECHO|TBDELAY|CRMOD);
	(void) ioctl(0, TIOCSETP, (char *)&sbuf);
	tbuf.t_intrc = -1;
	tbuf.t_quitc = -1;
	tbuf.t_startc = -1;
	tbuf.t_stopc = -1;
	tbuf.t_eofc = -1;
	tbuf.t_brkc = -1;
	(void) ioctl(0, TIOCSETC, (char *)&tbuf);
	lbuf.t_suspc = -1;
	lbuf.t_dsuspc = -1;
	lbuf.t_rprntc = -1;
	lbuf.t_flushc = -1;
	lbuf.t_werasc = -1;
	lbuf.t_lnextc = -1;
	(void) ioctl(0, TIOCSLTC, (char *)&lbuf);
#else
        sbuf.c_iflag &= ~(INLCR|IGNCR|ICRNL|IUCLC|IXON);
	sbuf.c_oflag &= ~(OPOST);
	sbuf.c_lflag &= ~(ISIG|ICANON|ECHO|ECHOE|ECHOK|ECHONL);
	sbuf.c_cc[VMIN]   = 1;
	sbuf.c_cc[VTIME]  = 0;
	(void) ioctl(0, TCSETAW, (char *)&sbuf);
#endif
}

fail()
{
	(void) kill(0, SIGTERM);
	done();
	/* NOTREACHED */
}

done()
{
	long tvec;

	if (subchild) {
		tvec = time((long *)0);
		(void) fprintf(fscript,"\nscript done on %s", ctime(&tvec));
		(void) fclose(fscript);
		(void) close(master);
#if defined(sparc) && !defined(OPENPTY)
		utent(REMOVE);
#endif
	} else {
#if !defined(TERMIO)
		(void) ioctl(0, TIOCSETP, (char *)&b);
		(void) ioctl(0, TIOCSETC, (char *)&tc);
		(void) ioctl(0, TIOCSLTC, (char *)&lc);
#else
		(void) ioctl(0, TCSETAW, (char *)&b);
#endif
#if defined(OPENPTY)
		(void) closepty(sline, line, uflg ? OPTY_UTMP : OPTY_NOP, master);
#endif
		(void) printf("Script done, file is %s\n", fname);
	}
	exit(0);
}

getmaster()
{
#if defined(OPENPTY)
	master = openpty(sline, line, uflg ? OPTY_UTMP : OPTY_NOP, getuid());
	if (-1 != master) {
# if !defined(TERMIO)
	    (void) ioctl(0, TIOCGETP, (char *)&b);
	    (void) ioctl(0, TIOCGETC, (char *)&tc);
	    (void) ioctl(0, TIOCGETD, (char *)&l);
	    (void) ioctl(0, TIOCGLTC, (char *)&lc);
	    (void) ioctl(0, TIOCLGET, (char *)&lb);
# else
	    (void) ioctl(0, TCGETA, (char *)&b);
# endif
# if defined(TIOCGWINSZ) && defined(TIOCSWINSZ)
	    (void) ioctl(0, TIOCGWINSZ, (char *)&win);
# endif
	    return;
	}
#else
	char *pty, *bank, *cp;
	struct stat stb;

# if defined(eta10) || defined(_iPSC386)
#  define PTY	"/dev/ptyp0"
#  define PTYm1 "/dev/ptyp"
# else
#  define PTY	"/dev/ptyp"
#  define PTYm1	"/dev/pty"
# endif

	pty = &line[strlen(PTY)];
# if defined(eta10) || defined(_iPSC386)
	for (bank = "0123"; *bank; bank++)
# else
	for (bank = "pqrs"; *bank; bank++)
# endif
	{
		line[strlen(PTYm1)] = *bank;
		*pty = '0';
		if (stat(line, &stb) < 0)
			break;
		for (cp = "0123456789abcdef"; *cp; cp++) {
			*pty = *cp;
			master = open(line, O_RDWR, 0);
			if (master >= 0) {
				char *tp = &line[strlen("/dev/")];
				int ok;

				/* verify slave side is usable */
				*tp = 't';
				ok = access(line, R_OK|W_OK) == 0;
				*tp = 'p';
				if (ok) {
# if !defined(TERMIO)
				    (void) ioctl(0, TIOCGETP, (char *)&b);
				    (void) ioctl(0, TIOCGETC, (char *)&tc);
				    (void) ioctl(0, TIOCGETD, (char *)&l);
				    (void) ioctl(0, TIOCGLTC, (char *)&lc);
				    (void) ioctl(0, TIOCLGET, (char *)&lb);
# else
				    (void) ioctl(0, TCGETA, (char *)&b);
# endif
# if defined(TIOCGWINSZ) && defined(TIOCSWINSZ)
				    (void) ioctl(0, TIOCGWINSZ, (char *)&win);
# endif
					return;
				}
				(void) close(master);
			}
		}
	}
#endif
	(void) fprintf(stderr, "Out of pty's\n");
	fail();
	/* NOTREACHED */
}

getslave()
{

#if defined(OPENPTY)
	slave = open(sline, O_RDWR, 0);
#else
	line[strlen("/dev/")] = 't';
	slave = open(line, O_RDWR, 0);
#endif
	if (slave < 0) {
		perror(line);
		fail();
		/* NOTREACHED */
	}
#if !defined(TERMIO)
	(void) ioctl(slave, TIOCSETP, (char *)&b);
	(void) ioctl(slave, TIOCSETC, (char *)&tc);
	(void) ioctl(slave, TIOCSLTC, (char *)&lc);
	(void) ioctl(slave, TIOCLSET, (char *)&lb);
	(void) ioctl(slave, TIOCSETD, (char *)&l);
#else
	(void) ioctl(slave, TCSETAW, (char *)&b);
#endif
#if defined(TIOCGWINSZ) && defined(TIOCSWINSZ)
	(void) ioctl(slave, TIOCSWINSZ, (char *)&win);
#endif
	setsid();
	if (-1 == ioctl(slave, TIOCSCTTY, (char *)0)) {
		fprintf(stderr, "script: set controling tty: %s\n", strerror(errno));
	}
}

#if defined(sparc) && !defined(OPENPTY)
# include <utmp.h>
# include <pwd.h>

void
utent(mode)
int mode;
{
	struct utmp utmp;
	struct passwd *pw;
	char *tp = &line[strlen("/dev/")];
	int slot = 0;
	int f, i;

	if ((f = open("/etc/utmp", O_RDWR)) < 0) {
		fprintf(stderr, "warning: could not update utmp entry\r\n");
		return;
	}

	*tp = 't';
	if (mode == ADD) {
		slot = ttyslot();
		pw = getpwuid(getuid());
		strncpy(utmp.ut_name, pw ? pw->pw_name : "?", sizeof (utmp.ut_name));
	} else {
		for(i=0; read(f, &utmp, sizeof (utmp)) == sizeof (utmp); i++) {
			if ( strncmp(utmp.ut_line, tp, sizeof (utmp.ut_line)) )
				continue;
			slot = i;
			break;
		}
		*utmp.ut_name = 0;
	}
	strncpy(utmp.ut_line, tp, sizeof(utmp.ut_line));
	time(&utmp.ut_time);
	*utmp.ut_host = 0;
	if (slot) {
		lseek(f, (long)(slot*sizeof (utmp)), 0);
		write(f, (char *)&utmp, sizeof(utmp));
	}
	close(f);
}
#endif
