/*
 *  remote -- allows a user to login on another host
 *
 *  originally by Tom Crayner (tc); cleaned up by everyone else.
 *  Ben Jackson and Kevin Braunsdorf had quite a whack at it:
 *	xdm support
 *	tn3270 support
 *	r.e. matched hosts
 *	much cleaner code
 *
 * $Id: remote.c,v 1.23 2000/06/21 17:21:11 ksb Exp $
 */
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <signal.h>
#include <netdb.h>
#include <utmp.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>

#include "machine.h"
#include "xdm-p.h"


#if USE_TERMIOS
#include <termios.h>
#include <unistd.h>

#else
#if USE_TERMIO
#include <termio.h>
#endif
#endif

#if REGEX
#include <regex.h>
#endif

#if REGCOMP
#include <regexp.h>
#endif

#if REGCMP || NEED_STDLIB_H
#include <stdlib.h>
#endif

#if USE_LIBGEN
#include <libgen.h>
#endif

extern int errno;

static char rcsid[] =
	"$Id: remote.c,v 1.23 2000/06/21 17:21:11 ksb Exp $";

char *progname = rcsid;

static char acPerm[] = "/usr/local/lib/remote/permok";
static char acMesg[] = "/usr/local/lib/remote/motd";
static char *apcAllowed[] = {
	"/dev/console",
	"/dev/ttya",
	"/dev/ttyb",
	(char *)0
};

extern char	*ttyname();


/* if the login hangs, or the user falls asleep we don't want		(tc)
 * to tie up a port
 */
static SIGRET_T
timeout()
{
	printf("\r\nTimeout after 240 seconds, connection closed!\r\n");
	exit(1);
}


/* fork off the kid we need to run the X process wait for it		(ksb)
 */
static void
DoXFork(pcMach)
char *pcMach;
{
	register int iPid, i;
	register char *pcXServer;

	switch (iPid = fork()) {
	case -1:
		fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		fflush(stderr);
		sleep(3);
		exit(1);
	case 0:
		/* XXX we should look through all the cg*0 and bw*0
		 * devices for the one that opens...
		 */
		if (-1 != (i = open("/dev/cgsix0", O_RDWR, 0))) {
			pcXServer = PATH_GENERIC_X;
			close(i);
		} else if (-1 != (i = open("/dev/bwtwo0", O_RDWR, 0))) {
			pcXServer = PATH_MONO_X;
			close(i);
		} else {
			/* try the general one */
			pcXServer = PATH_INDIR_X;
		}
		execl(pcXServer, "X", ":0", "-once", "-query", pcMach, (char *) 0);
		fprintf(stderr, "%s: execl: %s: %s\n", progname, pcXServer, strerror(errno));
		fflush(stderr);
		sleep(3);
		exit(1);
	default:
		break;
	}
	while (-1 != (i = wait((union wait *)0)) && iPid != i) {
		;
	}

	switch (iPid = fork()) {
	case -1:
		printf("\f");
		fflush(stdout);
		return;
	case 0:
#if USE_TPUT
		execl("/usr/bin/tput", "tput", "clear", (char *)0);
		fprintf(stderr, "%s: execl: tput: %s\n", progname, strerror(errno);
#else
		execl("/usr/ucb/clear", "clear", (char *)0);
		fprintf(stderr, "%s: execl: clear: %s\n", progname, strerror(errno));
#endif
		break;
	default:
		break;
	}
	while (-1 != (i = wait((union wait *)0)) && iPid != i) {
		;
	}
}

#if REGEX
size_t
regerror(int code, const regex_t *preg, char *errbuf, size_t errbuf_size)
{
	fprintf(stderr, "%s\r", errbuf);
}
#endif

#if REGCOMP
#if !defined(regerror)
size_t
regerror(char *s)
{
	fprintf(stderr, "%s\r", s);
}
#endif
#endif

/* return 1 if /pcPat/ ~= pcStr					(ksb/aab)
 *	if (re_comp(acRE) == 0 && 0 != re_exec(pHE->h_name))
 */
int
ReMatch(pcStr, pcPat)
char *pcStr, *pcPat;
{
	register int fMatch;
#if RECOMP
	extern char *re_comp();
	char *errmsg;
#else
#if REGCMP
	/* extern char *regcmp(); from in libgen.h */
	static char *cpattern = NULL;
#else
#if REGEX
	static regex_t aRECPat[1];
	static regmatch_t aRM[10];
#else
#if REGCOMP
	static regex *pRECPat;
#else
	static char lpbuf[100];
	static char *last_pattern = NULL;
#endif
#endif
#endif
#endif

#if RECOMP
	/* (re_comp handles a null pattern internally,
	 *  so there is no need to check for a null pattern here.)
	 */
	if ((errmsg = re_comp(pcPat)) != NULL) {
		fprintf(stderr, "%s: re_comp: %s\n", progname, errmsg);
		return 0;
	}
#else
#if REGCMP
	if (pcPat == NULL || *pcPat == '\000') {
		/* A null pattern means use the previous pattern.
		 * The compiled previous pattern is in cpattern, so just use it.
		 */
		if (cpattern == NULL) {
			fprintf(stderr, "%s: regcmp: No previous regular expression\n", progname);
			return 0;
		}
	} else {
		/* Otherwise compile the given pattern.
		 */
		register char *s;
		if ((s = regcmp(pcPat, 0)) == NULL) {
			fprintf(stderr, "%s: regcmp: Invalid pattern\n", progname);
			return 0;
		}
		if (cpattern != NULL) {
			free(cpattern);
		}
		cpattern = s;
	}
#else
#if REGEX
	if (0 != regcomp(aRECPat, pcPat, REG_EXTENDED|REG_ICASE)) {
		/* see regerror() above */
		return 0;
	}
#else
#if REGCOMP
	if (0 != (pRECPat = regcomp(pcPat)) {
		/* see regerror() above */
		return 0;
	}
#else
	if (pcPat == NULL || *pcPat == '\000') {
		/* Null pattern means use the previous pattern.
		 */
		if (last_pattern == NULL) {
			fprintf(stderr, "%s: strcmp: No previous regular expression\n", progname);
			return 0;
		}
		pcPat = last_pattern;
	} else {
		(void) strcpy(lpbuf, pcPat);
		last_pattern = lpbuf;
	}
#endif
#endif
#endif
#endif

	/* check the string
	 */
#if REGCMP
	fMatch = (regex(cpattern, pcStr) != NULL);
#else
#if RECOMP
	fMatch = (re_exec(pcStr) == 1);
#else
#if REGEX
	fMatch = (0 == regexec(aRECPat, pcStr, sizeof(aRM)/sizeof(aRM[0]), aRM, 0));
#else
#if REGCOMP
	fMatch = (1 == regexec(pRECPat, pcStr));
#else
	fMatch = strcmp(pattern, pcStr);
#endif
#endif
#endif
#endif
	return fMatch;
}

/* most of the code is in main, that is the way it came			(tc)
 */
int
main(argc, argv)
int argc;
char **argv;
{
	register FILE *fpOk, *fpMesg;
	register int fPass8, fFound;
	register char *pcTty, **ppc;
	register struct hostent *pHE, *pHEUnQual;
	register char *pcWho;
	auto char acWho[BUFSIZ], acDest[MAXHOSTNAMELEN+2], acRE[256];
	auto struct stat sbuf;
#if USE_TERMIOS
	auto struct termios n_tios;
#else
#if USE_TERMIO
	auto struct termio n_tio;
#else
	auto struct sgttyb tty;
#endif
#endif
	auto int fUseX = 0, f3270 = 0;
	extern char *getenv();

	if ((char *)0 == argv[0]) {
		progname = "remote";
	} else if ((char *)0 == (progname = strrchr(argv[0], '/'))) {
		progname = argv[0];
	} else {
		++progname;
	}

	/* remove the "login shell" flag if we have one from init
	 */
	if ('-' == *progname && '\000' != progname[1]) {
		++progname;
	}

	/* follow ksb normal form for version info
	 */
	if ((char *)0 != argv[1] && '-' == argv[1][0] && 'V' == argv[1][1] && '\000' == argv[1][2]) {
		printf("%s: %s\n", progname, rcsid);
		exit(0);
	}

	if ((char *)0 == (pcWho = getenv("USER")) && (char *)0 == (pcWho = getenv("LOGNAME"))) {
		fprintf(stderr, "%s: neither USER or LOGNAME set\n", progname);
		exit(1);
	}
	if ((char *)0 == getenv("TERM")) {
#if USE_PUTENV
		(void)putenv("TERM=ansi");
#else
		(void)setenv("TERM", "ansi");
#endif
	}
	/* this prevents tn3270 connections from getting a shell with
	 * ^]!
	 */
#if USE_PUTENV
	(void)putenv("SHELL=/bin/false");
#else
	(void)setenv("SHELL", "/bin/false");
#endif
#if defined(DEF_OPENWINHOME)
#if USE_PUTENV
	(void)putenv("OPENWINHOME=" DEF_OPENWINHOME);
#else
	(void)setenv("OPENWINHOME", DEF_OPENWINHOME);
#endif
#endif
	if ((char *)0 != getenv("DISPLAY") && 0 == strcmp(pcWho, "terminal")) {
		execl("/usr/openwin/bin/x3270", "x3270", "-model", "2", "199.82.159.239 2321", (char *)0);
		execl("/usr/local/bin/x3270", "x3270", "-model", "2", "199.82.159.239 2321", (char *)0);
		write(2, "I'm sorry\n", 10);
		exit(1);
	}

	/* forbid whack and hacks
	 */
	if (1 != getppid()) {
		printf("%s: no network connections\n", progname);
		exit(1);
	}
	if (0 == strcmp(pcWho, "tn3270") || 0 == strcmp(pcWho, "e3270")) {
		f3270 = 1;
	} else if (0 == strcmp(pcWho, "xdm")) {
		fUseX = 1;
	} else if (0 == strcmp(pcWho, "terminal")) {
		fUseX = 0;
	} else {
		fprintf(stderr, "%s: login name must be one I recognize\n", progname);
		exit(1);
	}
	openlog(pcWho, 0, LOG_AUTH);

	/* timeout and ignore a quit so we will not drop core
	 */
	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGALRM, timeout);

	/* Get his terminal so we can make a few checks
	 * If he is running without a tty then get rid of him.
	 */
	if ((char *)0 == (pcTty = ttyname(1))){
		fprintf(stderr, "%s: no ttyname\n", progname);
		sleep(1);
		exit(1);
	}
	if (0 != stat(pcTty, &sbuf)) {
		fprintf(stderr, "%s: stat: %s: %s\n", progname, pcTty, strerror(errno));
		sleep(1);
		exit(1);
	}
	for (ppc = apcAllowed; (char *)0 != *ppc; ++ppc) {
		if (0 != strcmp(*ppc, pcTty)) {
			break;
		}
	}
	if ((char *)0 == *ppc) {
		printf("%s: %s: can only run on a console\n", progname, pcTty);
		(void)fflush(stdout);
		sleep(1);
		exit(2);
	}

	if (-1 == fchmod(1, 00600)) {
		fprintf(stderr, "%s: fchmod: 1: %s\n", progname, strerror(errno));
		exit(1);
	}
	/* if there's a special message of the day, print it
	 */
	if ((FILE *)0 != (fpMesg = fopen(acMesg, "r"))) {
		register int c;
		while((c = getc(fpMesg)) != EOF) {
			putchar(c);
		}
		(void)fclose(fpMesg);
	}

	/* so...where does he want to go?
	 */
#if USE_TERMIOS
#if USE_TC_CALLS
	tcgetattr(0, & n_tios);
	n_tios.c_iflag &= ~(IGNCR);
	n_tios.c_iflag |= ICRNL|IXON|IXANY;
	n_tios.c_oflag |= OPOST|ONLCR;
	n_tios.c_lflag &= ~(NOFLSH|ECHOK|ECHONL);
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
	tcsetattr(0, TCSANOW, & n_tios);
#else
	(void)ioctl(0, TCGETS, & n_tios);
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
	ioctl(0, TCSETS, & n_tios);
#endif
#else
#if USE_TERMIO
	/* XXX */
	ioctl(0, TCGETS, & n_tio);
	n_tio.c_iflag |= (INLCR|IGNCR|ICRNL|IUCLC|IXON);
	n_tio.c_oflag |= OPOST;
	n_tio.c_lflag |= (ICANON|ISIG|ECHO);
	n_tio.c_cc[VMIN] = 1;
	n_tio.c_cc[VTIME] = 0;
	ioctl(0, TCSETS, & n_tio);
#else
	ioctl(0, TIOCGETP, &tty);
	tty.sg_flags &= ~(CBREAK|DECCTQ); /* DECCTQ is -ixany */
	ioctl(0, TIOCSETP, &tty);
#endif
#endif

	/* find a machine name to goto, but don't wait too long
	 */
	(void)alarm(240);
	fPass8 = 0;
	for (;;) {
		if (f3270)
			printf("\nIBM-Gateway: ");
		else if (fUseX)
			printf("\nX-host: ");
		else
			printf("\nMachine: ");
		(void)fflush(stdout);
		if ((char *)0 == fgets(acDest, sizeof(acDest), stdin)) {
			exit(0);
		}
		acDest[strlen(acDest)-1] = '\000';
		switch (acDest[0]) {
			register char *pcKeep;
			register int c;
		default:
			/* have a machine name? */
			pHEUnQual = gethostbyname(acDest);
			if ((struct hostent *)0 == pHEUnQual) {
				fprintf(stderr, "%s: unknown host `%s\'\n", progname, acDest);
				continue;
			}
			pcKeep = malloc((pHEUnQual->h_length|7)+1);
			c = pHEUnQual->h_length;
			while (c-- > 0) {
				pcKeep[c] = ((char *)(pHEUnQual->h_addr))[c];
			}
			pHE = gethostbyaddr(pcKeep, pHEUnQual->h_length, pHEUnQual->h_addrtype);
			if ((struct hostent *)0 == pHE) {
				fprintf(stderr, "%s: reverse lookup failed `%s\'\n", progname, pHEUnQual->h_name);
				continue;
			}
			(void)strcpy(acDest, pHE->h_name);
			break;
		case '-':
			if ('\000' != acDest[2]) {
				printf("%s: too many letters in option `%s'\n", progname, acDest+1);
				break;
			}
			switch (acDest[1]) {
			case 'V':
				printf("%s: %s\n", progname, "$Id: remote.c,v 1.23 2000/06/21 17:21:11 ksb Exp $");
				continue;
			case '8':
				printf("%s: using 8-bit rlogin\n", progname);
				fPass8 = 1;
				continue;
			case '7':
				printf("%s: using 7-bit rlogin\n", progname);
				fPass8 = 0;
				continue;
			case 't':
				printf("%s: %susing tn3270\n", progname, f3270 ? "not " : "");
				f3270 = !f3270;
				continue;
			case 'x':
				printf("%s: %susing an X server\n", progname, fUseX ? "not " : "");
				fUseX = !fUseX;
				continue;
			default:
				printf("%s: unknown option `%c\'\n", progname, acDest[1]);
				/* fallthrough */
			case 'h':	/* silent help */
			case '?':
				break;
			}
			/* fallthrough */
		case '\000':
			printf("\nTerminal Options:\n");
			printf("\t-8  use an 8-bit rlogin connection\n");
			printf("\t-7  use a 7-bit rlogin connection (default)\n");
			printf("\t-t  toggle use of tn3270/rlogin\n");
			printf("\t-V  output version of remote\n");
			printf("\t-x  toggle use the X server for connection\n");
			continue;
		}
		break;
	}

	/* open the permission file
	 */
	if ((FILE *)0 == (fpOk = fopen(acPerm, "r"))) {
		syslog(LOG_ERR, "open: %s: %m", acPerm);
		sleep(1);
		exit(0);
	}

	/* run thru the file...if he's requesting a machine
	 * and it's legal then set flag
	 */
	fFound = 0;
	while ((char *)0 != fgets(acRE, sizeof(acRE), fpOk)){
		register int l;

		if (acRE[0] == '#') {
			continue;
		}
		l = strlen(acRE);
		acRE[l-1] = '\000';
		if (0 != ReMatch(acDest, acRE)) {
			fFound = 1;
			break;
		}
	}
	(void)fclose(fpOk);

	/* send him off to the machine or log him out */
	if (! fFound) {
		printf("\n%s: Cannot connect to %s\n", progname, acDest);
		(void)fflush(stdout);
		sleep(1);
		exit(1);
	}

	if (f3270) {
		(void)alarm(0);
		syslog(LOG_INFO, "tn3270 to %s", acDest);
		execl(PATH_TN3270, "tn3270", acDest, "tn3270", (char *)0);
		syslog(LOG_ERR, "execl: tn3270: %m");
		exit(1);
	}

	/* setup to connection `as you like it...'
	 */

	/* run xdm for them
	 */
	if (fUseX) {
		(void)alarm(0);
		if (!IsRunningXDM(acDest, 5)) {
			fprintf(stderr, "%s: %s: is not running XDM!\n", progname, acDest);
			exit(1);
		}
		syslog(LOG_INFO, "xdm to %s", acDest);
		fflush(stdout);
		fflush(stderr);
		DoXFork(acDest);
		/* X leave the kbd hosed 9/10 of the time -- fix it */
		execl(PATH_KBD_MODE, "kbd_mode", "-a", (char *) 0);
		fprintf(stderr, "%s: cannot find kbd_mode\n", progname);
		fflush(stderr);
		sleep(3);
		exit(1);
	}

	printf("login: ");
	(void)fflush(stdout);
	if ((char *)0 == fgets(acWho, sizeof(acWho), stdin) || '\000' == acWho[1]) {
		printf("\n%s: you must enter a login name\n", progname);
		(void)fflush(stdout);
		sleep(1);
		exit(1);
	}
	(void)alarm(0);
	acWho[strlen(acWho)-1] = '\000';

	syslog(LOG_INFO, "rlogin to %s@%s", acWho, acDest);
	if (fPass8)
		execl(PATH_RLOGIN, "rlogin", acDest, "-l", acWho, "-8", (char *)0);
	else
		execl(PATH_RLOGIN, "rlogin", acDest, "-l", acWho, (char *)0);
	fprintf(stderr, "%s: execl: %s: %s\n", progname, PATH_RLOGIN, strerror(errno));
	/* bye */
	exit(1);
	/*NOTREACHED*/
}
