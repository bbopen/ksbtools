/*
 * flock a file and wait for a process to exit				(ksb)
 *
 * Copyright 1988, 1996, All Rights Reserved
 *	Kevin S Braunsdorf
 *	ksb@fedex.com
 *	CTC E370 Memphis, TN
 *
 *  `You may redistibute this code as long as you don't claim to have
 *   written it. -- kayessbee'
 *
 * $Compile: ${cc-cc} ${DEBUG--C} ${SYS--Dbsd} %f -o %F
 *
 * N.B.  flock -NB\|EX /tmp/pidfile  sh -c 'sleep 10; echo working; sleep 3'
 */
#include <sys/types.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>

#include "machine.h"

static char rcsid[] =
	"$Id: flock.c,v 2.6 2000/06/21 17:26:29 ksb Exp $";
static char *progname =
	rcsid;

static char *apcUsage[] = {	/* this is the usage line for the user	*/
	" [-cfn] [-EX|SH|UN|NB] file|fd [cmd]",
	" -h",
	" -V",
	(char *)0
};
static char *apcHelp[] = {	/* help text				*/
	"c  create file, if nonexistant",
	"f  force numeric files to be treated as files",
	"h  print this help message",
	"n  do not create file when nonexistant",
	"EX exclusive lock, default",
	"NB do not block for lock",
	"SH shared lock",
	"UN unlock",
	"V  output our version identifier",
	(char *)0
};

typedef struct LKnode {		/* a cmd line option implies a lock bit	*/
	char *pcflag;
	int iflag;
} LOCKKEY;

#if !defined(LOCK_SH)
#define   LOCK_SH	 0x01	   /* shared file lock */
#define   LOCK_EX	 0x02	   /* exclusive file lock */
#define   LOCK_NB	 0x04	   /* don't block when locking */
#define   LOCK_UN	 0x08	   /* unlock file */
#endif

static LOCKKEY aLKLocks[] = {	/* the list of the cmd lines we know	*/
	{"LOCK_EX", LOCK_EX},
	{"LOCK_SH", LOCK_SH},
	{"LOCK_UN", LOCK_UN},
	{"LOCK_NB", LOCK_NB},
	{"EX", LOCK_EX},
	{"SH", LOCK_SH},
	{"UN", LOCK_UN},
	{"NB", LOCK_NB},
	{(char *)0, -1}
};

/* determine which flag the use wants to set/clear			(ksb)
 */
int
Flag(pcCmd)
char *pcCmd;
{
	register char *pcTry;
	register LOCKKEY *pLK;
	extern char *strchr();

	for (pLK = aLKLocks; (char *)0 != (pcTry = pLK->pcflag); ++pLK) {
		if (0 == strcmp(pcTry, pcCmd)) {
			return pLK->iflag;
		}
	}
	if ((char *)0 != (pcTry = strchr(pcCmd, '|'))) {
		*pcTry++ = '\000';
		return Flag(pcCmd)|Flag(pcTry);
	}

	fprintf(stderr, "%s: `%s' is not a flock key, see -h\n", progname, pcCmd);
	exit(1);
	/*NOTREACHED*/
}

/* is this character string all digits					(ksb)
 */
int
isnumber(pc)
char *pc;
{
	while (*pc) {
		if (*pc < '0' || *pc > '9')
			break;
		++pc;
	}
	return *pc == '\000';
}

/* Get a lock on the named file and execute the rest of our arguments	(ksb)
 * while the lock is active, when this command exits exit with it's
 * extit status.
 */
int
main(argc, argv)
int argc;
char **argv;
{
	extern int atoi();
	extern char *strrchr();
	static char *apcTrue[] = {"true", (char *)0};
	auto WAIT_t wait_buf;
	auto int fd, tClose, oFlags, fCreate = 0, fFile = 0;
	auto int iLock = -1;
	auto char **ppcHelp;


	if ((char *)0 != (progname = strrchr(*argv, '/'))) {
		++progname;
	} else {
		progname = *argv;
	}
	++argv, --argc;
	while (argc > 0 && '-' == argv[0][0]) {
		switch (*++argv[0]) {
		case '\000':
			break;
		case 'n':
		case 'c':
			fCreate = argv[0][0] == 'c';
			argv[0][0] = '-';
			continue;
		case 'f':
			fFile = 1;
			argv[0][0] = '-';
			continue;
		case 'h':
			for (ppcHelp = apcUsage; (char *)0 != *ppcHelp; ++ppcHelp) {
				fprintf(stdout, "%s: usage%s\n", progname, *ppcHelp);
			}
			for (ppcHelp = apcHelp; (char *)0 != *ppcHelp; ++ppcHelp) {
				fprintf(stdout, "%s\n", *ppcHelp);
			}
			exit(0);
		case 'V':
			printf("%s: %s\n", progname, rcsid);
			exit(0);
		default:
			if (-1 == iLock)
				iLock = 0;
			iLock ^= Flag(argv[0]);
			break;
		}
		++argv, --argc;
	}

	if (-1 == iLock) {
		iLock = LOCK_EX;
	}

	if (0 == argc) {
		fprintf(stderr, "%s: must provide a file or fd, see -h\n", progname);
		exit(1);
	}

	tClose = 1;
	if (-1 == (fd = open(argv[0], O_RDONLY, 0600))) {
		oFlags = 0 != fCreate ? O_CREAT|O_WRONLY|O_APPEND : O_WRONLY|O_APPEND;
		if (-1 == (fd = open(argv[0], oFlags, 0666))) {
			if (!isnumber(argv[0]) || fFile) {
				fprintf(stderr, "%s: open: %s: %s\n", progname, argv[0], strerror(errno));
				exit(1);
			}
			fd = atoi(argv[0]);
			if (-1 == fcntl(fd, F_GETFD, 0)) {
				fprintf(stderr, "%s: fcntl: %d: %s\n", progname, fd, strerror(errno));
				exit(1);
			}
			tClose = 0;
		}
	}
	if (-1 == flock(fd, iLock)) {
		fprintf(stderr, "%s: flock: %s: %s\n", progname, argv[0], strerror(errno));
		exit(1);
	}
	++argv, --argc;

	if (0 == argc) {
		argv = apcTrue;
	}
	if (tClose != 0) {		/* save a fork */
		switch (fork()) {
		case 0:
			close(fd);
			break;
		case -1:
			fprintf(stderr, "%s: fork: %s\n", progname);
			exit(1);
			/*NOTREACHED*/
		default:
			wait(& wait_buf);
			/* exit will close fd for us */
#if HAVE_UNION_WAIT
			exit((int) wait_buf.w_retcode);
#else
			exit( wait_buf);
#endif
			/*NOTREACHED*/
		}
	}
	execvp(argv[0], argv);
	fprintf(stderr, "%s: execvp: %s: %s", progname, argv[0], strerror(errno));
	exit(1);
	/*NOTREACHED*/
}
