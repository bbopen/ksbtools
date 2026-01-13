/*
 * routine to grab a pty and put a slave process on it			(ksb)
 *
 $Compile: ${cc-cc} ${cc_debug--g} -DTEST -I/usr/include/local %f -o %F -lpucc
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <signal.h>
#include <stdio.h>

#include "machine.h"
#include "openpty.h"


/* open a pty, put a process on it return a rd/wr fd on the control pty	(ksb)
 */
int
forkpty(pcSlave, pcMaster, fBits, uid, ppcArgv)
char *pcSlave, *pcMaster;
int fBits;
uid_t uid;
char **ppcArgv;				/* command to execve		*/
{
	extern char **environ;
	auto int fdPty, fdCopy;
	auto int pid, i;
	auto long ldisc, lmode;		/* tty modes to copy		*/
	auto struct sgttyb sgttyb;
	auto struct tchars tchars;
	auto struct ltchars ltchars;
	auto struct winsize ws;
	auto int afd[2];
	auto char acSync[8];

	/* grab an avaliable pty
	 */
	if (-1 == (fdPty = openpty(pcSlave, pcMaster, fBits, uid))) {
		return -1;
	}
	if (-1 == pipe(afd)) {
		close(fdPty);
		return -1;
	}

	fflush(stdout);	/* caller should do this, sigh */
	fflush(stderr);

	switch (pid = vfork()) {
	case -1:
		/* error -- run away
		 */
		close(fdPty);
		close(afd[0]);
		close(afd[1]);
		return -1;
	default:
		/* we have the kid -- close the unneeded fds and return
		 */
		close(afd[1]);
		if (0 == (i = read(afd[0], acSync, sizeof(acSync)))) {
			close(afd[0]);
			break;
		}
		close(afd[0]);
		close(fdPty);
		return -1;
	case 0:
		/* we are the kid close the pty
		 */
		close(fdPty);
		close(afd[0]);
		fcntl(afd[1], F_SETFD, 1);
		if (2 == afd[1]) {
			afd[1] = dup(2);
		}

		/* detatch from the parrents tty
		 * make our new tty stdin, stdout, stderr
		 */
		for (i = 2; i >= 0; --i) {
			if (i == afd[1])
				continue;
			close(i);
		}
		/* find out controling terminal to copy stty setting from
		 */
		if (-1 == (fdCopy = open("/dev/tty", O_RDWR, 0600))) {
			write(afd[1], "1\n", 2);
			exit(1);
		}
		ioctl(fdCopy, TIOCGETD, (char *)&ldisc);
		ioctl(fdCopy, TIOCLGET, (char *)&lmode);
		ioctl(fdCopy, TIOCGETP, (char *)&sgttyb);
		ioctl(fdCopy, TIOCGETC, (char *)&tchars);
		ioctl(fdCopy, TIOCGLTC, (char *)&ltchars);
		ioctl(fdCopy, TIOCGWINSZ, (char *)&ws);

		ioctl(fdCopy, TIOCNOTTY, (char *)0);
		close(fdCopy);

		if (0 != open(pcSlave, O_RDWR, 0600) || 1 != dup(0) || 2 != dup(1)) {
			write(afd[1], "2\n", 2);
			exit(2);
		}
		setsid();
#if defined(TIOCSCTTY)
		ioctl(0, TIOCSCTTY, (char *)0);
#endif

		/* with the exception that we turn off echo and cbreak
		 * mode, we prop tty settings.  We should change baud too?
		 */
		sgttyb.sg_flags &= ~(CRMOD);
		ioctl(0, TIOCSETD, (char *)&ldisc);
		ioctl(0, TIOCLSET, (char *)&lmode);
		ioctl(0, TIOCSETP, (char *)&sgttyb);
		ioctl(0, TIOCSETC, (char *)&tchars);
		ioctl(0, TIOCSLTC, (char *)&ltchars);
		ioctl(0, TIOCSWINSZ, (char *)&ws);

		/* give us a proc group to work in
		 */
		ioctl(0, TIOCGPGRP, (char *)&i);
		setpgrp(0, i);
		i = getpid();
		ioctl(0, TIOCSPGRP, (char *)&i);
		setpgrp(0, i);

		execve(ppcArgv[0], ppcArgv, environ);
		write(afd[1], "3\n", 2);
		exit(3);
		break;
	}

	return fdPty;
}

#if defined(TEST)

static char *progname = "forkpty";
static int dead = 0;

/*
 * tell the user that the process died					(ksb)
 */
static void
tellem(sig)
int sig;
{
	auto int pid;
	auto int iStatus;

	pid = wait(& iStatus);
	fprintf(stderr, "child %d has exited %d (0x%04x)\n", pid, iStatus, iStatus);
	dead = 1;
}

static int s_ldisc, s_lmode;
static struct sgttyb s_sgttyb;
static struct tchars s_tchars;
static struct ltchars s_ltchars;
static struct winsize s_ws;

/*
 * save the tty state so we can exit correctly				(ksb)
 */
static void
save_tty()
{
	ioctl(0, TIOCGETD, (char *)&s_ldisc);
	ioctl(0, TIOCLGET, (char *)&s_lmode);
	ioctl(0, TIOCGETP, (char *)&s_sgttyb);
	ioctl(0, TIOCGETC, (char *)&s_tchars);
	ioctl(0, TIOCGLTC, (char *)&s_ltchars);
	ioctl(0, TIOCGWINSZ, (char *)&s_ws);
}

/*
 * make the user's tty just pass the characters through			(ksb)
 * (could we use `raw' mode?
 */
static void
hose_tty()
{
	auto struct sgttyb h_sgttyb;
	auto struct ltchars h_ltchars;
	auto struct tchars h_tchars;

	ioctl(0, TIOCGETP, (char *)&h_sgttyb);
	h_sgttyb.sg_flags &= ~ECHO;
	h_sgttyb.sg_flags |= CBREAK;
	h_sgttyb.sg_erase = -1;
	h_sgttyb.sg_kill = -1;
	ioctl(0, TIOCSETP, (char *)&h_sgttyb);

	(void) ioctl(0, TIOCGETC, (char *)&h_tchars);
	h_tchars.t_intrc = -1;
	h_tchars.t_quitc = -1;
	h_tchars.t_startc = -1;
	h_tchars.t_stopc = -1;
	h_tchars.t_eofc = -1;
	h_tchars.t_brkc = -1;
	(void) ioctl(0, TIOCSETC, (char *)&h_tchars);

	(void) ioctl(0, TIOCGLTC, (char *)&h_ltchars);
	h_ltchars.t_suspc = -1;
	h_ltchars.t_dsuspc = -1;
	h_ltchars.t_rprntc = -1;
	h_ltchars.t_flushc = -1;
	h_ltchars.t_werasc = -1;
	h_ltchars.t_lnextc = -1;
	(void) ioctl(0, TIOCSLTC, (char *)&h_ltchars);
}

/*
 * restore the tty on exit, good for the soul				(ksb)
 */
static void
restore_tty()
{
	ioctl(0, TIOCSETD, (char *)&s_ldisc);
	ioctl(0, TIOCLSET, (char *)&s_lmode);
	ioctl(0, TIOCSETP, (char *)&s_sgttyb);
	ioctl(0, TIOCSETC, (char *)&s_tchars);
	ioctl(0, TIOCSLTC, (char *)&s_ltchars);
	ioctl(0, TIOCSWINSZ, (char *)&s_ws);
}


/*
 * drive the pty for an example program, like /bin/sh			(ksb)
 */
int
main(argc, argv)
int argc;
char **argv;
{
	char acBuf[2048];
	int fd, cc;
	long int r,w,x;
	char acMaster[1024], acSlave[1024];

	if ((char *)0 == argv[1]) {
		static char *apcDef[] = { "-forkpty", "/bin/sh", "-i", (char *)0 };
		argv = apcDef;
	}

	signal(SIGCHLD, tellem);
	save_tty();
	if (-1 == (fd = forkpty(acSlave, acMaster, OPTY_UTMP, getuid(), argv+1))) {
		fprintf(stderr, "%s: forkpty: %s\n", progname, strerror(errno));
		exit(1);
	}

	r = (1 << fd) | (1 << 0);
	w = 0;
	x = 0;
	hose_tty();
	while (!dead && -1 != select(32, &r, &w, &x, (struct timeval *)0)) {
		if (r & (1 << fd)) {
			cc = read(fd, acBuf, sizeof(acBuf));
			write(1, acBuf, cc);
		}
		if (r & 1) {
			cc = read(0, acBuf, sizeof(acBuf));
			if (dead && (cc == 0 || acBuf[0] == '\004')) {
				break;
			}
			write(fd, acBuf, cc);
		}
		r = (1 << fd) | (1 << 0);
	}
	closepty(acSlave, acMaster, OPTY_UTMP, fd);

	restore_tty();

	exit(0);
}
#endif	/* TEST driver */
