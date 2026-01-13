#!mkcmd
# $Compile(*):	%b %o -mMkcmd %f && %b %o -dHAVE_RLIMIT prog.c && mv -i prog %F
# $Mkcmd(*):	${mkcmd-mkcmd} std_help.m std_version.m %f
from '<sys/types.h>'
from '<sys/param.h>'
from '<errno.h>'
from '<fcntl.h>'
from '<signal.h>'
from '<ctype.h>'
from '"machine.h"'

require "std_help.m" "std_version.m"
basename "Tee" ""

%i
#if HAVE_RLIMIT
#include <sys/resource.h>
#endif

#if USE_SYS_INODE_H
#include <sys/inode.h>
#endif

#if !defined(PIPSIZ)
#if defined(PIPE_BUF)
#define PIPSIZ	PIPE_BUF
#else
#define PIPSIZ	5120
#endif
#endif

static int iMaxFd;

static char *rcsid =
	"$Id: Tee.m,v 2.5 1997/03/08 20:46:16 ksb Exp $";
%%

before {
}

escape integer {
	named "iMaxSpig"
	init "8"
	param "spigots"
	help "number of tees per process"
}

boolean 'a' {
	named "fAppend"
	help "append to the files in the list"
}

# from the HPUX tee manual page...
action 'i' {
	update '(void)signal(SIGINT, SIG_IGN);'
	help "ignore interrupts"
}

boolean 'v' {
	named "fVerbose"
	help "trace progress on stderr"
}

# XXX should take "20b" and the like -- ksb
integer 'b' {
	named "iChunk"
	param "blocksize"
	init "-1"
	help "set the blocksize for the transfer"
}

boolean 'x' {
	hidden named "fTrace"
	init "0"
	help "trace creation of data network"
}
after {
}

list {
	param "files"
	named "Tee"
	help "files to tee to, or shell processes with a leading pipe"
}

augment action 'V' {
	user 'after_func();printf("%%s: spigots %%d, max fd %%d, blocksize %%d\\n", %b, iMaxSpig, iMaxFd, %rbn);'
}
%%

/* insert runtime defaults and system params				(ksb)
 */
static void
before_func()
{
#if HAVE_RLIMIT
	auto struct rlimit rl;

	getrlimit(RLIMIT_NOFILE, &rl);
	iMaxFd = rl.rlim_cur;
#else
#if NEED_GETDTABLESIZE
	iMaxFd = NOFILE;
#else
	iMaxFd = getdtablesize();
#endif
#endif
	(void)signal(SIGCHLD, SIG_IGN);
	(void)signal(SIGPIPE, SIG_IGN);
}

/* figure out the best course of action					(ksb)
 */
static void
after_func()
{
	if (iMaxFd < 5) {
		fprintf(stderr, "%s: not enough system file descriptors (%d < 5)\n", progname, iMaxFd);
		exit(1);
	}
	/* we count how many we can procees:
	 *  count 0, 1, 2 which are already open
	 *  count extra one needed if last in list is |cmd
	 */
	if (0 >= iMaxSpig || iMaxSpig > (iMaxFd - 4)) {
		iMaxSpig = iMaxFd - 4;
	}

	if (0 >= iChunk) {
		iChunk = PIPSIZ;
	}
}

/* Couple a command to the tee system.					(ksb)
 */
static int
MkProc(pcCmd)
char *pcCmd;
{
	register int i;
	auto int iKid;
	auto int aiPipe[2];
	static char *pcShell = "/bin/sh";

	if (-1 == pipe(aiPipe)) {
		fprintf(stderr, "%s: pipe: %s\n", progname, strerror(errno));
		return -1;
	}
	switch (iKid = fork()) {
	case 0:
		break;
	case -1:
		fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		close(aiPipe[0]), close(aiPipe[1]);
		return -1;
	default:
		if (fTrace) {
			fprintf(stderr, "%s: kid %d fork'd\n", progname, iKid);
		}
		close(aiPipe[0]);
		return aiPipe[1];
	}
	close(0);
	dup(aiPipe[0]);
	close(aiPipe[0]);
	close(aiPipe[1]);
	
	/* close the other streams we have open, we just pipe'd
	 * aiPipe[0] so it should be a good upper bound on fd's
	 */
	for (i = 3; i < aiPipe[0]; ++i) {
		(void)close(i);
	}
	execl(pcShell, "sh", "-c", pcCmd, (char *)0);
	fprintf(stderr, "%s: execl: %s: %s\n", progname, pcShell, strerror(errno));
	exit(1);
	/*NOTREACHED*/
}

#if !defined(O_ACCMODE)
#define O_ACCMODE	(O_RDONLY|O_WRONLY|O_RDWR)
#endif

/* Couple an existing file descriptor to the tee system.		(ksb)
 * [/dev/fd/10 would be cool, if we had that available, but we don't.]
 */
static int
FdOut(pcFd)
char *pcFd;
{
	register int iCvt, iFlag;

	iCvt = atoi(pcFd);
	if (-1 == (iFlag = fcntl(iCvt, F_GETFL, (void *)0))) {
		return -1;
	}
	if (O_RDONLY == (iFlag & O_ACCMODE)) {
		fprintf(stderr, "%s: %s: file descriptor %d is read only\n", progname, pcFd, iCvt);
		return -1;
	}
	return iCvt;
}

/* tee to all the files given, if more are given than			(ksb)
 * we can write to fork another tee to do the rest
 * If we can only fit 3 then
 *    tee a1 a2 a3 a4 a5 a6 a7 a8
 * would become
 *    tee a1 a2 a3 | tee a4 a5 a6 | tee a7 a8
 *
 * There is a subtle bug in this program, it assumes that we do not
 * inherit a lot of open file descriptors.  If we do we can run out
 * unexpectedly and not do our mission. -- ksb (XXX)
 */
static void
Tee(argc, argv)
int argc;
char **argv;
{
	register int i, cc;
	register char *pcBuf;
	register int iMade;
	auto int *piSpigots;
	auto int iKid;
	auto int iCnt;
	register int iReap;

	cc = 1;
	if (argc > iMaxSpig) {
		auto int aiPipe[2];

		if (-1 == pipe(aiPipe)) {
			fprintf(stderr, "%s: pipe: %s\n", progname, strerror(errno));
			exit(1);
		}
		switch (iKid = fork()) {
		default:
			close(0);
			dup(aiPipe[0]);
			close(aiPipe[0]);
			close(aiPipe[1]);
			Tee(argc-iMaxSpig, argv+iMaxSpig);
			exit(0);
		case -1:
			fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
			exit(1);
		case 0:
			break;
		}
		close(aiPipe[0]);
		cc = aiPipe[1];
		argc = iMaxSpig;
		/* only parent notes blocks as they pass */
		fVerbose = 0;
	}
	piSpigots = (int *)calloc(argc+2, sizeof(int));
	iMade = 0;
	piSpigots[iMade++] = cc;
	for (i = 0; i < argc; ++i) {
		if ('-' == argv[i][0] && '\000' == argv[i][1]) {
			piSpigots[iMade++] = 1;
			continue;
		}
		if ('|' == argv[i][0]) {
			piSpigots[iMade] = MkProc(argv[i]+1);
		} else if ('>' == argv[i][0] && '&' == argv[i][1] && isdigit(argv[i][2])) {
			piSpigots[iMade] = FdOut(argv[i]+2);
		} else {
			piSpigots[iMade] = open(argv[i], fAppend ? O_WRONLY|O_APPEND : O_WRONLY|O_CREAT|O_TRUNC, 0666);
		}
		if (-1 != piSpigots[iMade]) {
			++iMade;
			continue;
		}
		fprintf(stderr, "%s: open: %s: %s\n", progname, argv[i], strerror(errno));
		/* UNIX tee ignores this, sigh */
	}

	pcBuf = (char *)calloc(iChunk, sizeof(char));
	iCnt = 0;
	while (0 < (cc = read(0, pcBuf, iChunk))) {
		register int wc, oc;
		register char *pc;

		for (i = 0; i < iMade; ++i) {
			oc = cc;
			pc = pcBuf;
			while (0 < oc) {
				wc = write(piSpigots[i], pc, oc);
				if (-1 == wc) {
					fprintf(stderr, "%s: write: %d: %s\n", progname, piSpigots[i], strerror(errno));
					break;
				}
				oc -= wc;
				pc += wc;
			}
		}
		if (fVerbose) {
			(void)write(2, ".", 1);
			if (70 == ++iCnt) {
				(void)write(2, "\n", 1);
				iCnt = 0;
			}
		}
	}
	if (iCnt) {
		write(2, "\n", 1);
	}
	if (-1 == cc) {
		fprintf(stderr, "%s: read: stdin: %s\n", progname, strerror(errno));
	}
	for (i = 0; i < iMade; ++i) {
		(void)close(piSpigots[i]);
	}
	while (-1 != (iReap = wait((void *)0)) && EINTR != errno) {
		if (fTrace) {
			fprintf(stderr, "%s: reap %d\n", progname, iReap);
		}
	}
	exit(0);
}
