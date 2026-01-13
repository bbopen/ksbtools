/*
 * $Id: consent.c,v 8.2 1999/08/16 03:05:22 ksb Exp $
 *
 * Copyright 1992 Purdue Research Foundation, West Lafayette, Indiana
 * 47907.  All rights reserved.
 *
 * Written by Kevin S Braunsdorf, ksb@cc.purdue.edu, purdue!ksb
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company or the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 *
 * 1. Neither the authors nor Purdue University are responsible for any
 *    consequences of the use of this software.
 *
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Credit to the authors and Purdue
 *    University must appear in documentation and sources.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 4. This notice may not be removed or altered.
 */
#ifndef lint
static char copyright[] =
"@(#) Copyright 1992 Purdue Research Foundation.\nAll rights reserved.\n";
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <pwd.h>

#include "cons.h"
#include "machine.h"
#include "consent.h"
#include "client.h"
#include "main.h"

#if USE_TERMIO
#include <termio.h>

#else
#if USE_TERMIOS
#include <termios.h>
#include <unistd.h>

#else	/* use ioctl stuff */
#include <sgtty.h>
#include <sys/ioctl.h>
#endif
#endif

#if USE_STREAMS
#include <stropts.h>
#endif

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#if HAVE_SELECT_H
#include <sys/select.h>
#endif


BAUD baud [] = {
	{"9600", B9600},	/* we pick a nice std baud for default */
#if defined(EXTA)
	{"a", EXTA},
	{"A", EXTA},
#endif
#if defined(EXTB)
	{"b", EXTB},
	{"B", EXTB},
#endif
#if defined(B230400)
	{"230400",B230400},
#endif
#if defined(B115200)
	{"115200",B115200},
#endif
#if defined(B153600)
	{"153600", B153600},
#endif
#if defined(B230400)
	{"230400", B230400},
#endif
#if defined(B307200)
	{"307200", B307200},
#endif
#if defined(B460800)
	{"460800", B460800},
#endif
#if defined(B76800)
	{"76800", B76800},
#endif
#if defined(B57600)
	{"57600", B57600},
#endif
#if defined(B38400)
	{"38400", B38400},
#endif
#if defined(B28800)
	{"28800", B28800},
#endif
#if defined(B19200)
	{"19200", B19200},
#endif
#if defined(B14400)
	{"14400", B14400},
#endif
#if defined(B7200)
	{"7200",  B7200},
#endif
#if defined(B4800)
	{"4800", B4800},
#endif
#if defined(B2400)
	{"2400", B2400},
#endif
#if defined(B1800)
	{"1800", B1800},
#endif
#if defined(B1200)
	{"1200", B1200},
#endif
#if defined(B600)
	{"600", B600},
#endif
#if defined(B300)
	{"300", B300},
#endif
#if defined(B200)
	{"200", B200},
#endif
#if defined(B150)
	{"150", B150},
#endif
#if defined(B134)
	{"134", B134},
#endif
#if defined(B110)
	{"110", B110},
#endif
#if defined(B75)
	{"75", B75},
#endif
#if defined(B50)
	{"50", B50},
#endif
#if defined(B0)
	{"0", B0},
#endif
};


/* find a baud rate for the string "9600x" -> B9600			(ksb)
 */
static BAUD *
FindBaud(pcMode)
char *pcMode;
{
	register int i;

	for (i = 0; i < sizeof(baud)/sizeof(struct baud); ++i) {
		if (0 != strncmp(pcMode, baud[i].acrate, strlen(baud[i].acrate)))
			continue;
		return baud+i;
	}
	return baud;
}

PARITY parity[] = {
#if USE_TERMIOS
#if !defined(PAREXT)
#define PAREXT	0
#endif
	{'e', PARENB|CS7, 0},			/* even			*/
	{'m', PARENB|CS7|PARODD|PAREXT, 0},	/* mark			*/
	{'o', PARENB|CS7|PARODD, 0},		/* odd			*/
	{'p', CS8, 0},				/* pass 8 bits, no parity */
	{'s', PARENB|CS7|PAREXT, 0},		/* space		*/
#else
	{'e', EVENP, ODDP},	/* even					*/
	{'m', EVENP|ODDP, 0},	/* mark					*/
	{'o', ODDP, EVENP},	/* odd					*/
#if defined(PASS8)
	{'p', PASS8,EVENP|ODDP},/* pass 8 bits, no parity		*/
#endif
	{'s', 0, EVENP|ODDP}	/* space				*/
#endif
};

/* find a parity on the end of a baud "9600even" -> EVEN		(ksb)
 */
static PARITY *
FindParity(pcMode)
char *pcMode;
{
	register int i;
	auto char cFirst;

	cFirst = *pcMode;
	if ('\000' == cFirst) {
		return parity;
	}

	if (isupper(cFirst)) {
		cFirst = tolower(cFirst);
	}
	for (i = 0; i < sizeof(parity)/sizeof(struct parity); ++i) {
		if (cFirst != parity[i].ckey)
			continue;
		return parity+i;
	}
	fprintf(stderr, "%s: unknown parity `%s', using `%c'\n", progname, pcMode, parity->ckey);
	return parity;
}

/* convert "9600p" to a useful internal format				(ksb)
 * return 1 for ^S/^Q 0 for tandem flow control
 */
int
XlateSpeed(pcMode, ppBaud, ppParity)
char *pcMode;
BAUD **ppBaud;
PARITY **ppParity;
{
	register int i;

	*ppBaud = FindBaud(pcMode);
	i = strlen(ppBaud[0]->acrate);
	if (strlen(pcMode) >= i) {
		pcMode += i;
	}
	switch (*pcMode) {
	case 'x':	/* xon/xoff inline flow control */
		i = 0;
		++pcMode;
		break;
	case 't':	/* tandem mode */
		++pcMode;
		/* fall into default (tandem) mode */
	default:
		i = 1;
		break;
	}
	*ppParity = FindParity(pcMode);
	return i;
}

/* can be called from version to display compiled baud rates		(ksb)
 */
void
BaudRates(fp)
FILE *fp;
{
	register int i, iWidth, fFirst, iLen, iProgLen;
	static char acAvail[] = "available baud rates:";

	if (0 == sizeof(baud)/sizeof(struct baud)) {
		fprintf(fp, "%s: no baud rates in table, that is not good\n", progname);
		exit(1);
	}

	iProgLen = strlen(progname);
	fprintf(fp, "%s: %s", progname, acAvail);
	iWidth = iProgLen + 2 + sizeof(acAvail)-1;
	if (0 != sizeof(parity)/sizeof(struct parity)) {
		fprintf(fp, " [xt][");
		for (i = 0; i < sizeof(parity)/sizeof(struct parity); ++i) {
			fputc(parity[i].ckey, fp);
			++iWidth;
		}
		fprintf(fp, "] at");
		iWidth += 6 + 4;
	}
	fFirst = 0;
	for (i = 0; i < sizeof(baud)/sizeof(struct baud); ++i) {
		iLen = strlen(baud[i].acrate);
		if (iWidth + iLen >= 78) {
			fputc('\n', fp);
			fFirst = 1;
		}
		if (fFirst) {
			iWidth = iProgLen + 2 + iLen;
			fprintf(fp, "%s: %s", progname, baud[i].acrate);
		} else {
			iWidth += 1 + iLen;
			fprintf(fp, " %s", baud[i].acrate);
		}
		fFirst = 0;
	}
	fputc('\n', fp);
}


#if USE_TERMIOS
/* setup a tty device							(ksb)
 */
static int
TtyDev(pCE)
CONSENT *pCE;
{
	struct termios termp;
	auto struct stat stPerm;

	/* here we should fstat for `read-only' checks
	 */
	if (-1 == fstat(pCE->fdtty, & stPerm)) {
		fprintf(stderr, "%s: fstat: %s: %s\n", progname, pCE->dfile, strerror(errno));
	} else if (0 == (stPerm.st_mode & 0222)) {
		/* any device that is read-only we won't write to
		 */
		pCE->fronly = 1;
	}

	/*
	 * Get terminal attributes
	 */
	if (-1 == tcgetattr(pCE->fdtty, &termp)) {
		fprintf(stderr, "%s: tcgetattr: %s(%d): %s\n", progname, pCE->dfile, pCE->fdtty, strerror(errno));
		return -1;
	}

	/*
	 * Turn off:	echo
	 *		icrnl
	 *		opost	No post processing
	 *		icanon	No line editing
	 *		isig	No signal generation
	 * Turn on:	ixoff
	 */
	termp.c_iflag = IXON|IXOFF|BRKINT;
	termp.c_oflag = 0;
	termp.c_cflag = CREAD;
	termp.c_cflag |= pCE->pparity->iset;
	termp.c_lflag = 0;
	/*
	 * Set the VMIN == 128
	 * Set the VTIME == 1 (0.1 sec)
	 * Don't bother with the control characters as they are not used
	 */
	termp.c_cc[VMIN] = 128;
	termp.c_cc[VTIME] = 1;

	if (-1 == cfsetospeed(&termp,pCE->pbaud->irate)) {
		fprintf(stderr, "%s: cfsetospeed: %s(%d): %s\n", progname, pCE->dfile, pCE->fdtty, strerror(errno));
		return -1;
	}
	if (-1 == cfsetispeed(&termp,pCE->pbaud->irate)) {
		fprintf(stderr, "%s: cfsetispeed: %s(%d): %s\n", progname, pCE->dfile, pCE->fdtty, strerror(errno));
		return -1;
	}

	/*
	 * Set terminal attributes
	 */
	if (-1 == tcsetattr(pCE->fdtty, TCSADRAIN, &termp)) {
		fprintf(stderr, "%s: tcsetattr: %s(%d): %s\n", progname, pCE->dfile, pCE->fdtty, strerror(errno));
		return -1;
	}

#if USE_STREAMS
	/* eat all the streams modules upto and including ttcompat
	 */
        while (ioctl(pCE->fdtty, I_FIND, "ttcompat") == 1) {
		(void)ioctl(pCE->fdtty, I_POP, 0);
        }
#endif
	pCE->fup = 1;
	return 0;
}
#else

/* setup a tty device							(ksb)
 */
static int
TtyDev(pCE)
CONSENT *pCE;
{
	struct sgttyb sty;
	struct tchars m_tchars;
	struct ltchars m_ltchars;
	auto struct stat stPerm;
#if NEED_THREE_WIRE
	auto int iModFlags;
#endif

	/* here we should fstat for `read-only' checks
	 */
	if (-1 == fstat(pCE->fdtty, & stPerm)) {
		fprintf(stderr, "%s: fstat: %s: %s\n", progname, pCE->dfile, strerror(errno));
	} else if (0 == (stPerm.st_mode & 0222)) {
		/* any device that is read-only we won't write to
		 */
		pCE->fronly = 1;
	}

#if USE_POSIX_TTY
	/* AIX wants to use the posix tty code -- from mwm@fnal.gov
	 */
	if (-1 == ioctl(pCE->fdtty, TIOCSETD, "posix")) {\
		fprintf(stderr, "%s: tiocsetd: %d: %s\n", progname, pCE->fdtty, strerror(errno));
	}
#endif

#if USE_SOFTCAR
#if defined(TIOCSSOFTCAR)
	if (-1 == ioctl(pCE->fdtty, TIOCSSOFTCAR, &fSoftcar)) {
		fprintf(stderr, "%s: softcar: %d: %s\n", progname, pCE->fdtty, strerror(errno));
		return -1;
	}
#endif
#endif

#if NEED_THREE_WIRE
	if (-1 == ioctl(pCE->fdtty, TIOCMODG, &iModFlags)) {
		fprintf(stderr, "%s: ioctl7: %d: %s\n", progname, pCE->fdtty, strerror(errno));
		return -1;
	}
	iModFlags |= TIOCM_CTS | TIOCM_CAR | TIOCM_DTR;
	if (-1 == ioctl(pCE->fdtty, TIOCMODS, &iModFlags)) {
		fprintf(stderr, "%s: ioctl8: %d: %s\n", progname, pCE->fdtty, strerror(errno));
		return -1;
	}
#endif

	/* stty 9600 raw cs7
	 */
	if (-1 == ioctl(pCE->fdtty, TIOCGETP, (char *)&sty)) {
		fprintf(stderr, "%s: ioctl1: %s(%d): %s\n", progname, pCE->dfile, pCE->fdtty, strerror(errno));
		return -1;
	}
	sty.sg_flags &= ~(ECHO|CRMOD|TANDEM|pCE->pparity->iclr);
	sty.sg_flags |= (CBREAK|pCE->pparity->iset);
	if (! pCE->fuseflow) {
		sty.sg_flags |= TANDEM;
	}
	sty.sg_erase = -1;
	sty.sg_kill = -1;
	sty.sg_ispeed = pCE->pbaud->irate;
	sty.sg_ospeed = pCE->pbaud->irate;
	if (-1 == ioctl(pCE->fdtty, TIOCSETP, (char *)&sty)) {
		fprintf(stderr, "%s: ioctl2: %d: %s\n", progname, pCE->fdtty, strerror(errno));
		return -1;
	}

	/* stty undef all tty chars
	 * (in cbreak mode we may not need to this... but we do)
	 */
	if (-1 == ioctl(pCE->fdtty, TIOCGETC, (char *)&m_tchars)) {
		fprintf(stderr, "%s: ioctl3: %d: %s\n", progname, pCE->fdtty, strerror(errno));
		return -1;
	}
	m_tchars.t_intrc = -1;
	m_tchars.t_quitc = -1;
	if (pCE->fuseflow) {
		m_tchars.t_startc = '\021';	/* ^Q */
		m_tchars.t_stopc = '\023';	/* ^S */
	} else {
		m_tchars.t_startc = -1;
		m_tchars.t_stopc = -1;
	}
	m_tchars.t_eofc = -1;
	m_tchars.t_brkc = -1;
	if (-1 == ioctl(pCE->fdtty, TIOCSETC, (char *)&m_tchars)) {
		fprintf(stderr, "%s: ioctl4: %d: %s\n", progname, pCE->fdtty, strerror(errno));
		return -1;
	}
	if (-1 == ioctl(pCE->fdtty, TIOCGLTC, (char *)&m_ltchars)) {
		fprintf(stderr, "%s: ioctl5: %d: %s\n", progname, pCE->fdtty, strerror(errno));
		return -1;
	}
	m_ltchars.t_werasc = -1;
	m_ltchars.t_flushc = -1;
	m_ltchars.t_lnextc = -1;
	m_ltchars.t_suspc = -1;
	m_ltchars.t_dsuspc = -1;
	if (-1 == ioctl(pCE->fdtty, TIOCSLTC, (char *)&m_ltchars)) {
		fprintf(stderr, "%s: ioctl6: %d: %s\n", progname, pCE->fdtty, strerror(errno));
		return -1;
	}
#if USE_STREAMS
	/* pop off the un-needed streams modules (on a sun3 machine esp.)
	 * (Idea by jrs@ecn.purdue.edu)
	 */
        while (ioctl(pCE->fdtty, I_POP, 0) == 0) {
		/* eat all the streams modules */;
        }
#endif
	pCE->fup = 1;
	return 0;
}
#endif

#if DO_VIRTUAL
/* setup a virtual device						(ksb)
 */
static int
VirtDev(pCE)
CONSENT *pCE;
{
#if USE_TERMIOS
	static struct termios n_tios;
#else
	auto struct sgttyb sty;
	auto struct tchars m_tchars;
	auto struct ltchars m_ltchars;
#endif
#if HAVE_RLIMIT
	auto struct rlimit rl;
#endif
	auto int i, iNewGrp;
	auto int fd;
	extern char **environ;
	register char *pcShell, **ppcArgv;

	(void)fflush(stdout);
	(void)fflush(stderr);

	switch (pCE->ipid = fork()) {
	case -1:
		return -1;
	case 0:
		break;
	default:
		pCE->fup = 1;
		sleep(2);	/* chance to open line */
		return 0;
	}

	/* put the signals back that we trap
	 */
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGTSTP, SIG_DFL);

	/* setup new process with clean filew descriptors
	 */
#if HAVE_RLIMIT
	getrlimit(RLIMIT_NOFILE, &rl);
	i = rl.rlim_cur;
#else
	i = getdtablesize();
#endif
	for (/* i above */; i-- > 2; ) {
		close(i);
	}
	/* leave 2 until we *have to close it*
	 */
	close(1);
	close(0);
#if defined(TIOCNOTTY)
	if (-1 != (i = open("/dev/tty", 2, 0))) {
		ioctl(i, TIOCNOTTY, (char *)0);
		close(i);
	}
#endif

#if HAVE_SETSID
	iNewGrp = setsid();
	if (-1 == iNewGrp) {
		fprintf(stderr, "%s: %s: setsid: %s\n", progname, pCE->server, strerror(errno));
		iNewGrp = getpid();
	}
#else
	iNewGrp = getpid();
#endif

	if (0 != open(pCE->acslave, 2, 0) || 1 != dup(0)) {
		fprintf(stderr, "%s: %s: fd sync error\n", progname, pCE->server);
		exit(1);
	}
#if HAVE_PTSNAME || HAVE_LDTERM
	/* Eat all the modules down through "ldterm".
	 */
	while (ioctl(0, I_FIND, "ldterm") == 1) {
		(void)ioctl(0, I_POP, 0);
	}
#endif
#if HAVE_PTSNAME
	/* SYSVr4 semantics for opening stream ptys			(gregf)
	 * under PTX (others?) we have to push the compatibility
	 * streams modules `ptem' and `ld'
	 */
	if (ioctl(0, I_FIND, "ptem") == 0) {
		(void)ioctl(0, I_PUSH, "ptem");
	}
	if (ioctl(0, I_FIND, "ld") == 0) {
		(void)ioctl(0, I_PUSH, "ld");
	}
#endif
#if HAVE_LDTERM
	if (ioctl(0, I_FIND, "ptem") == 0) {
		(void)ioctl(0, I_PUSH, "ptem");
	}
	if (ioctl(0, I_FIND, "ldterm") == 0) {
		(void)ioctl(0, I_PUSH, "ldterm");
	}
#endif
#if HAVE_STTY_LD
	if (ioctl(0, I_FIND, "stty_ld") == 0) {
		(void)ioctl(0, I_PUSH, "stty_ld");
	}
#endif
#if HAVE_TTCOMPAT
	if (ioctl(0, I_FIND, "ttcompat") == 0) {
		(void)ioctl(0, I_PUSH, "ttcompat");
	}
#endif

#if USE_TERMIOS
#if USE_OLD_TCSETS
	if (0 != ioctl(0, TCGETS, & n_tio)) {
		fprintf(stderr, "%s: iotcl: getsw: %s\n", progname, strerror(errno));
		exit(1);
	}
#else
	if (-1 == tcgetattr(0, & n_tios)) {
		fprintf(stderr, "%s: tcgetattr: stdin: %s\n", progname, strerror(errno));
		exit(1);
	}
#endif
	n_tios.c_iflag &= ~(IGNCR|IUCLC);
	n_tios.c_iflag |= ICRNL|IXON|IXANY;
	n_tios.c_oflag &= ~(OLCUC|ONOCR|ONLRET|OFILL|NLDLY|CRDLY|TABDLY|BSDLY);
	n_tios.c_oflag |= OPOST|ONLCR;
	n_tios.c_lflag &= ~(XCASE|NOFLSH|ECHOK|ECHONL);
	n_tios.c_lflag |= ISIG|ICANON|ECHO;
	n_tios.c_cc[VEOF] = '\004';
	n_tios.c_cc[VEOL] = '\000';
	n_tios.c_cc[VERASE] = '\010';
	n_tios.c_cc[VINTR] = '\003';
	n_tios.c_cc[VKILL] = '@';
	/* MIN */
	n_tios.c_cc[VQUIT] = '\034';
	n_tios.c_cc[VSTART] = '\021';
	n_tios.c_cc[VSTOP] = '\023';
	n_tios.c_cc[VSUSP] = '\032';
#if USE_OLD_TCSETS
	if (0 != ioctl(0, TCSETS, & n_tios)) {
		fprintf(stderr, "%s: getarrt: %s\n", progname, strerror(errno));
		exit(1);
	}
#else
	if (-1 == tcsetattr(0, TCSANOW, & n_tios)) {
		fprintf(stderr, "%s: tcsetattr: stdin: %s\n", progname, strerror(errno));
		exit(1);
	}
#endif

	(void)tcsetpgrp(0, iNewGrp);
#else
	/* stty 9600 raw cs7
	 */
	if (-1 == ioctl(0, TIOCGETP, (char *)&sty)) {
		fprintf(stderr, "%s: ioctl1: %s: %s\n", progname, pCE->fdtty, strerror(errno));
		exit(1);
	}
	sty.sg_flags &= ~(CBREAK|TANDEM|pCE->pparity->iclr);
	sty.sg_flags |= (ECHO|CRMOD|pCE->pparity->iset);
	sty.sg_erase = '\b';
	sty.sg_kill = '\025';
	sty.sg_ispeed = pCE->pbaud->irate;
	sty.sg_ospeed = pCE->pbaud->irate;
	if (-1 == ioctl(0, TIOCSETP, (char *)&sty)) {
		fprintf(stderr, "%s: ioctl2: %s\n", progname, strerror(errno));
		exit(1);
	}

	/* stty undef all tty chars
	 * (in cbreak mode we may not need to this... but we do)
	 */
	if (-1 == ioctl(0, TIOCGETC, (char *)&m_tchars)) {
		fprintf(stderr, "%s: ioctl3: %s\n", progname, strerror(errno));
		exit(1);
	}
	m_tchars.t_intrc = '\003';
	m_tchars.t_quitc = '\034';
	m_tchars.t_startc = '\021';
	m_tchars.t_stopc = '\023';
	m_tchars.t_eofc = '\004';
	m_tchars.t_brkc = '\033';
	if (-1 == ioctl(0, TIOCSETC, (char *)&m_tchars)) {
		fprintf(stderr, "%s: ioctl4: %s\n", progname, strerror(errno));
		exit(1);
	}
	if (-1 == ioctl(0, TIOCGLTC, (char *)&m_ltchars)) {
		fprintf(stderr, "%s: ioctl5: %s\n", progname, strerror(errno));
		exit(1);
	}
	m_ltchars.t_werasc = '\027';
	m_ltchars.t_flushc = '\017';
	m_ltchars.t_lnextc = '\026';
	m_ltchars.t_suspc = '\032';
	m_ltchars.t_dsuspc = '\031';
	if (-1 == ioctl(0, TIOCSLTC, (char *)&m_ltchars)) {
		fprintf(stderr, "%s: ioctl6: %s\n", progname, strerror(errno));
		exit(1);
	}

	/* give us a process group to work in
	 */
	ioctl(0, TIOCGPGRP, (char *)&i);
	setpgrp(0, i);
	ioctl(0, TIOCSPGRP, (char *)&iNewGrp);
	setpgrp(0, iNewGrp);
#endif

	close(2);
	(void)dup(1);		/* better be 2, but it is too late now */

	/* if the command is null we should run root's shell, directly
	 * if we can't find root's shell run /bin/sh
	 */
	pcShell = "/bin/sh";
	if ('\000' == pCE->pccmd[0]) {
		static char *apcArgv[] = {
			"-shell", "-i", (char *)0
		};
		register struct passwd *pwd;

		if ((struct passwd *)0 != (pwd = getpwuid(0)) && '\000' != pwd->pw_shell[0]) {
			pcShell = pwd->pw_shell;
		}
		ppcArgv = apcArgv;
	} else {
		static char *apcArgv[] = {
			"/bin/sh", "-ce", (char *)0, (char *)0
		};

		apcArgv[2] = pCE->pccmd;
		ppcArgv = apcArgv;
	}
	execve(pcShell, ppcArgv, environ);
	fprintf(stderr, "execve: %s\n", strerror(errno));
	exit(1);
	/*NOTREACHED*/
}
#endif

#if DO_ANNEX
/* connect to an Annex terminal server and get the port		(ksb&olson)
 * Thanks to Bob Olson <olson@mcs.anl.gov> for the ideas in this code.
 */
static int
AnnexConnect(pCE)
CONSENT *pCE;
{
	auto struct sockaddr_in annex_port;
	auto int iFlag;
	register struct hostent *pHE;
	register int fd;

#if USE_STRINGS
	(void)bzero((char *)&annex_port, sizeof(annex_port));
#else
        (void)memset((void *)&annex_port, 0, sizeof(annex_port));
#endif
	if ((struct hostent *)0 == (pHE = gethostbyname(pCE->pcannex))) {
		fprintf(stderr, "%s: gethostbyname: %s: %s\n", progname, pCE->pcannex, hstrerror(h_errno));
		return -1;
	}
#if USE_STRINGS
	bcopy(pHE->h_addr, &annex_port.sin_addr, pHE->h_length);
#else
	memcpy(&annex_port.sin_addr, pHE->h_addr, pHE->h_length);
#endif
	annex_port.sin_family = pHE->h_addrtype;
	annex_port.sin_port = htons((short)pCE->haxport);

	if (-1 == (fd = socket(AF_INET, SOCK_STREAM, 0))) {
		fprintf(stderr, "%s: socket: %s\n", progname, strerror(errno));
		return -1;
	}
	iFlag = 1;
	(void)setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&iFlag, sizeof(iFlag));
	if (-1 == connect(fd, (struct sockaddr *)&annex_port, sizeof(annex_port))) {
		fprintf(stderr, "%s: connect: %s:%d: %s\n", progname, pCE->pcannex, ntohs(annex_port.sin_port), strerror(errno));
		close(fd);
		return -1;
	}
	return fd;
}

/* finsh the setup for the annex connected device			(ksb)
 * we should do the minimal telnet negotiation here, not just bomb out.
 * XXX: telnet neg. work here (char-at-a-time, 80x24)
 */
static void
AnnexDev(pCE)
CONSENT *pCE;
{
	pCE->fup = 1;
	CSTROUT(pCE->fdtty, "\r\n");
}
#endif

/* down a console, virtual or real					(ksb)
 */
void
ConsDown(pCE, pfdSet)
CONSENT *pCE;
fd_set *pfdSet;
{
#if DO_VIRTUAL
	if (-1 != pCE->ipid) {
		if (-1 != kill(pCE->ipid, SIGHUP))
			sleep(1);
		pCE->ipid = -1;
	}
#endif
	if (-1 != pCE->fdtty) {
		FD_CLR(pCE->fdtty, pfdSet);
#if DO_VIRTUAL
		if (0 == pCE->fvirtual)  {
			(void)close(pCE->fdtty);
			pCE->fdtty = -1;
		}
#else
		(void)close(pCE->fdtty);
		pCE->fdtty = -1;
#endif
	}
	if (-1 != pCE->fdlog) {
		(void)close(pCE->fdlog);
		pCE->fdlog = -1;
	}
	pCE->fup = 0;
}

/* set up a console the way it should be for use to work with it	(ksb)
 * also, recover from silo over runs by dropping the line and re-opening
 * We also maintian the select set for the caller.
 */
void
ConsInit(pCE, pfdSet)
CONSENT *pCE;
fd_set *pfdSet;
{
	/* clean up old stuff
	 */
	ConsDown(pCE, pfdSet);

	pCE->fronly = 0;
	(void)strcpy(pCE->acline, pCE->server);
	pCE->inamelen = strlen(pCE->server);
	pCE->acline[pCE->inamelen++] = ':';
	pCE->acline[pCE->inamelen++] = ' ';
	pCE->iend = pCE->inamelen;


	/* try to open it again
	 */
	if (-1 == (pCE->fdlog = open(pCE->lfile, O_RDWR|O_CREAT|O_APPEND, 0644))) {
		fprintf(stderr, "%s: open: %s: %s\n", progname, pCE->lfile, strerror(errno));
		return;
	}

#if DO_VIRTUAL
	if (pCE->fvirtual) {
		/* still open, never ever close it, but set the bit */
	} else
#endif
#if DO_ANNEX
	if (pCE->fannexed) {
		pCE->fdtty = AnnexConnect(pCE);
		if (-1 == pCE->fdtty) {
			(void)close(pCE->fdlog);
			pCE->fdlog = -1;
			return;
		}
	} else
#endif
	if (-1 == (pCE->fdtty = open(pCE->dfile, O_RDWR|O_NDELAY, 0600))) {
		fprintf(stderr, "%s: open: %s: %s\n", progname, pCE->dfile, strerror(errno));
		(void)close(pCE->fdlog);
		pCE->fdlog = -1;
		return;
	}
	FD_SET(pCE->fdtty, pfdSet);

	/* ok, now setup the device
	 */
#if DO_VIRTUAL
	if (pCE->fvirtual) {
		VirtDev(pCE);
	} else
#endif
#if DO_ANNEX
	if (pCE->fannexed) {
		AnnexDev(pCE);
	} else
#endif
	{
		TtyDev(pCE);
	}
}
