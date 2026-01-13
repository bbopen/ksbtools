#!mkcmd
# $Id: xapply.m,v 3.4 1998/02/11 14:05:53 ksb Exp $
#
# ksb's version of apply.  The Rob Pike version was too strange for me,
# and it used that errx() stuff that I couldn't use any where else.
# This one has some nice features. See the manual page.
#
# mk will compile this if you have other ksb tools (mkcmd, mk) installed.
#	$Compile: %b %o -mMkcmd %f && %b %o prog.c && mv prog %F
#	$Mkcmd: ${mkcmd-mkcmd} %f
	comment "%c %kCompile: $%{cc-gcc%} -g %%f -o %%F"
# [yeah the line above doesn't start with a hash]

from '<sys/param.h>'
from '<sys/wait.h>'
from '<fcntl.h>'
from '<errno.h>'
from '<ctype.h>'

require "std_help.m" "std_version.m" "std_control.m"

basename "xapply" ""

augment action 'V' {
	user "Version();"
}
%i
static char rcsid[] =
	"$Id: xapply.m,v 3.4 1998/02/11 14:05:53 ksb Exp $";
extern char **environ;
extern int errno;
%%

letter 'a' {
	named "cEsc"
	init "'%'"
	param "c"
	help "change the escape character"
}

number {
	named "iPass"
	param "count"
	init "1"
	help "number of arguments passed to each command"
}

integer 'P' {
	named "iParallel"
	init "1"
	help "number of tasks to run in parallel"
	param "[jobs]"
	noparam "if (%ypi == (%N = getenv(\"PARALLEL\")) || '\\000' == *%N) {%N = \"3\";}"
	after "if (%n < 1) {%n = 1;}Tasks(%n);"
}

char* 'S' {
	named "pcShell"
	init getenv "SHELL"
	before 'if ((char *)0 == %n) {%n = "%D/sh/pv";}'
	param "shell"
	help "the shell to run tasks under"
}

char* variable "pcCmd" {
	param "cmd"
	help "template command to control each task"
}

# Params are directorys to scan with opendir/readdir/closedir.		(ksb)
# NB. not implemented due to 2 major issues:
#	what to do with leading dot file (and ".", "..")
#	should we sort the files? (like ls, or like in common first?
# This is not the place, but I think we would skip "." and ".." in _any_
# case.  Add an allow'd option '-A' like ls, and sort the files as alpha
# sort does.  Maybe.
#require "dir.m"
#boolean 'd' {
#	exclude "f"
#	list dir { ... }
#}
boolean 'f' {
	named "fpIn" "pcIn"
	help "arguments are read indirectly one per line from files"
	char* 'p' {
		named "pcPad"
		init '""'
		param "pad"
		help "fill short files with this token"
	}
	left "pcCmd" {
	}
	list file ["r"] {
		named "pfpIn" "ppcNames"
		param "files"
		user "VApply(%#, %n, %N);"
		help "files of arguments to distribute to tasks"
	}
}

char* 'i' {
	named "pcRedir"
	param "input"
	help "change stdin for the child processes"
}

left "pcCmd" {
}

list {
	named "Apply"
	param "args"
	help "list of arguments to distribute to tasks"
}

%c

/* if we do not have NCARGS in <sys/param.h> try posix or guess		(ksb)
 */
#if !defined(NCARGS)
#include <limits.h>
#if defined(ARG_MAX)
#define NCARGS	ARG_MAX
#else
#define NCARGS	4000
#endif
#endif

/* this might be nice to know						(ksb)
 */
static void
Version()
{
	printf("%s: shell: %s\n", progname, pcShell);
}

static pid_t *piOwn =	/* pid table we own, last is _our_pid_		*/
	(pid_t *)0;

/* build a list of processes we own, of course the list			(ksb)
 * starts out with just us in it.  We build one extra slot on the end
 * to keep our pid (and in case we get 0 tasks calloc is not NULL).
 */
static void
Tasks(iMax)
int iMax;
{
	register int i;

	piOwn = (pid_t *)calloc(iMax+1, sizeof(pid_t));
	if ((pid_t *)0 == piOwn) {
		fprintf(stderr, "%s: calloc: pid table[%d]: %s\n", progname, iMax, strerror(errno));
		exit(1);
	}
	piOwn[0] = getpid();
	for (i = 0; i < iMax; ++i)
		piOwn[i+1] = piOwn[i];
}

/* remove the pid from the table, return 1 if it's in there		(ksb)
 */
static int
Stop(iPid)
pid_t iPid;
{
	register int i;
	for (i = 0; i < iParallel; ++i) {
		if (piOwn[i] != iPid)
			continue;
		piOwn[i] = piOwn[iParallel];
		return 1;
	}
	return 0;
}

/* add a process to our list						(ksb)
 */
static void
Start(iPid)
pid_t iPid;
{
	static int iWrap = 0;

	while (piOwn[iWrap] != piOwn[iParallel]) {
		if (++iWrap == iParallel)
			iWrap = 0;
	}
	piOwn[iWrap] = iPid;
}


/* Launch the command the User built.					(ksb)
 * If we've done something bad to the system back off by Fibonacci numbers:
 * (viz. 1 2 3 5 8 13 21 34 55 ...) up to a max sleep time of MAX_BACKOFF.
 * If we have no fork() by then we are at MAXPROCS and the caller will
 * wait() for a slot then call us again.  {It might not be us that is at
 * MAXPROCS so we will find no done kid in the wait and spin back here,
 * repeating until the system figures out who to kill.}
 */
#if !defined(MAX_BACKOFF)
#define MAX_BACKOFF	15
#endif
static pid_t
Launch(pcBuilt)
char *pcBuilt;
{
	static char *apcTestVec[] = {
		"$SHELL", "-c", "$cmd",
		(char *)0
	};
	register int iPid, iD1, iD2, iTmp;

	fflush(stdout);
	fflush(stderr);
	for (iD1 = iD2 = 1; iD1 < MAX_BACKOFF; iTmp = iD1, iD1 += iD2, iD2 = iTmp) {
		switch (iPid = fork()) {
		case -1:
			if (errno == EAGAIN || errno == ENOMEM) {
				sleep(iD1);
				continue;
			}
			fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
			exit(1);
		case 0:
			apcTestVec[0] = pcShell;
			apcTestVec[2] = pcBuilt;
			execve(pcShell, apcTestVec, environ);
			fprintf(stderr, "%s: execve: %s: %s\n", progname, pcShell, strerror(errno));
			exit(1);
		default:
			if (fVerbose)
				printf("%s\n", pcBuilt);
			return iPid;
		}
	}
	return -1;
}

/* Select a substring from the parameter we just found			(ksb)
 * this allows us to get params from the password/group file
 * We can recurse to get subfields...
 * We can split on any single character, or "white space".
 * If you want more use an RE to convert to a single character!
 *
 * The field number can be a base 10 number, or "$" for the last
 * in the record.  The empty count is taken as 1.
 */
char *
FFmt(pcDest, pcTemplate, pcData)
char *pcDest, *pcTemplate, *pcData;
{
	register int cSep, iCount;
	auto char cKeep;
	auto char *pcEnd;
	static char acEos[] = "\000";

	switch (*pcTemplate) {
	case ']':	/* we have it */
		(void)strcpy(pcDest, pcData);
		return pcTemplate+1;
	case '\\':	
		++pcTemplate;
		cSep = *pcTemplate++;
		break;
	case ' ':
		cSep = -1;
		++pcTemplate;
		break;
	default:
		cSep = *pcTemplate++;
		break;
	}
	if ('$' == *pcTemplate) {
		iCount = -1;
		++pcTemplate;
	} else if (! isdigit(*pcTemplate)) {
		iCount = 1;
	} else {
		iCount = 0;
		do {
			iCount *= 10;
			iCount += *pcTemplate++ - '0';
		} while (isdigit(*pcTemplate));
	}
	/* locate the field they want
	 */
	while ((-1 == iCount || --iCount > 0) && '\000' != *pcData) {
		register char *pcDollar;
		pcDollar = pcData;
		if (-1 == cSep) {
			while ('\000' != *pcData && !isspace(*pcData)) {
				++pcData;
			}
			if (-1 == iCount && '\000' == *pcData) {
				pcData = pcDollar;
				break;
			}
			while (isspace(*pcData)) {
				++pcData;
			}
			continue;
		}
		pcData = strchr(pcData, cSep);
		if ((char *)0 == pcData) {
			pcData = (-1 == iCount) ? pcDollar : acEos;
			break;
		}
		++pcData;
	}
	/* find end of field
	 */
	if (-1 == cSep) {
		pcEnd = pcData;
		while ('\000' != *pcEnd && !isspace(*pcEnd))
			++pcEnd;
		if ('\000' == *pcEnd)
			pcEnd = (char *)0;
	} else {
		pcEnd = strchr(pcData, cSep);
	}
	/* put an EOS ('\000') at the end of the field and call down,
	 * then put it back and return to calleL
	 */
	if ((char *)0 != pcEnd) {
		cKeep = *pcEnd;
		*pcEnd = '\000';
	}
	pcTemplate = FFmt(pcDest, pcTemplate, pcData);
	if ((char *)0 != pcEnd) {
		*pcEnd = cKeep;
	}
	return pcTemplate;
}

/* Build the command from the input data.				(ksb)
 * We know there are enough slots in argv for us to read.
 * We need to construct the command in a buffer and return it,
 * the caller passes in a buffer that is at least NCARGS big.
 * If we never see a parameter expand cat them on the end,
 * apply does this, I think it is brain dead.
 */
static char *
AFmt(pcBuffer, pcTemplate, iPass, argv, ppcNamev)
char *pcBuffer, *pcTemplate;
int iPass;
char **argv, **ppcNamev;
{
	register char *pcScan;
	register int i, cCache, iParam, fTail;
	register int fSubPart;
	register char **ppcExp;

	pcScan = pcBuffer;
	cCache = cEsc;
	fTail = 1;
	fSubPart = 0;
	while ('\000' != *pcTemplate) {
		if (cCache != (*pcScan++ = *pcTemplate++)) {
			continue;
		}
		if (cCache == (i = *pcTemplate++)) {
			continue;
		}
		/* %f... for the filename under -f
		 * %{15} to get to bigger numbers
		 * %[5,1] for subfields (see FFmt)
		 */
		if ('f' == i) {
			if ((char **)0 == (ppcExp = ppcNamev)) {
				fprintf(stderr, "%s: %%f used without -f\n", progname);
				exit(4);
			}
			i = *pcTemplate++;
		} else {
			ppcExp = argv;
		}
		if (isdigit(i)) {	/* %0, %1 .. %9 */
			iParam = i - '0';
		} else if ('{' == i || (fSubPart = '[' == i)) {
			for (iParam = 0; i = *pcTemplate, isdigit(i); ++pcTemplate) {
				iParam *= 10;
				iParam += i - '0';
			}
			if (!fSubPart && '}' != *pcTemplate) {
				fprintf(stderr, "%s: %%{..: missing close culry (})\n", progname);
				return (char *)0;
			}
			++pcTemplate;
		} else {
			fprintf(stderr, "%s: %%%1.1s: unknown escape sequence\n", progname, pcTemplate-1);
			return (char *)0;
		}
		/* %0 means suppress all params, but do it
		 */
		fTail = 0;
		--pcScan;
		if (0 == iParam) {
			if (fSubPart) {
				fSubPart = 0;
				pcTemplate = FFmt(pcScan, pcTemplate-1, "");
			}
			continue;
		}
		--iParam;
		if (iParam < 0 || iParam >= iPass) {
			fprintf(stderr, "%s: %d: escape references an out of range parameter (limit %d)\n", progname, iParam+1, iPass);
			return (char *)0;
		}
		if (fSubPart) {
			fSubPart = 0;
			pcTemplate = FFmt(pcScan, pcTemplate-1, ppcExp[iParam]);
		} else {
			(void)strcpy(pcScan, ppcExp[iParam]);
		}
		pcScan += strlen(pcScan);
	}
	/* implicit "%1 %2 %3 ..."
	 */
	if (fTail) {
		if (pcScan > pcBuffer && !isspace(pcScan[-1])) {
			*pcScan++ = ' ';
		}
		for (i = 0; i < iPass; ++i) {
			if (0 != i) {
				*pcScan++ = ' ';
			}
			(void)strcpy(pcScan, argv[i]);
			pcScan += strlen(pcScan);
		}
	}
	*pcScan = '\000';
	return pcBuffer;
}

/* do the work								(ksb)
 *	apply -P$iParallel -S$pcShell -c "$cmd" $argc
 */
static void
Apply(argc, argv)
int argc;
char **argv;
{
	register int i, iRunning, iAdvance;
	register char *pcBuilt;
	auto int wExit;
	auto char acCmd[NCARGS+1];

	if (0 != (argc % iPass)) {
		fprintf(stderr, "%s: %d arguments is not a multiple of hunk size (%d)\n", progname, argc, iPass);
		exit(1);
	}

	/* not very useful here, but we allow it
	 */
	if ((char *)0 != pcRedir) {
		close(0);
		if (0 != open(pcRedir, O_RDONLY, 0666)) {
			fprintf(stderr, "%s: open: %s: %s\n", progname, pcRedir, strerror(errno));
			exit(3);
		}
	}

	iRunning = 0;
	iAdvance = 0 == iPass ? 1 : iPass;
	while (0 != argc) {
		pcBuilt = AFmt(acCmd, pcCmd, iPass, argv, (char **)0);
		if ((char *)0 == pcBuilt) {
			break;
		}
		argc -= iAdvance, argv += iAdvance;
		if (!fExec) {
			if (fVerbose)
				printf("%s\n", pcBuilt);
			continue;
		}
		do {
			while (0 < (i = wait3(& wExit, (iRunning < iParallel) ? WNOHANG : 0, (struct rusage *)0))) {
				if (Stop(i))
					--iRunning;
			}
		} while (-1 == (i = Launch(pcBuilt)));
		Start(i);
		++iRunning;
	}

	/* just wait for them
	 */
	while (-1 != wait((void *)0)) {
		;
	}
}

/* the User gave us a list of files on the command line,		(ksb)
 * parameters come from the files indirectly.
 */
static void
VApply(argc, pfpArgv, argv)
int argc;
FILE **pfpArgv;
char **argv;
{
	register char **ppcSlots, *pcBuilt;
	register int i, c, iMem, iEofs;
	register int iRunning, iAdvance, fd;
	register FILE *fpMove;
	auto char acBufs[NCARGS+1];
	auto char acCmd[NCARGS+1];
	auto int iOne;
	auto int wExit;

	if (0 != (argc % iPass)) {
		fprintf(stderr, "%s: %d files: not divisable by hunk size (%d)\n", progname, argc, iPass);
		exit(1);
	}
	if ((char **)0 == (ppcSlots = (char **)calloc(iPass+1, sizeof(char *)))) {
		fprintf(stderr, "%s: out of memory\n", progname);
		exit(1);
	}
	/* set close-on-exec to keep the kids from having lots of
	 * open files, if we can't set the close-on-exec we ignore it.
	 * Move a /dev/null fd on stdin if it is used as a stream of
	 * input files. (So the kids don't race for the input.)
	 * If two copies of stdin are put on the line we can read
	 * more than one token from that stream, the hack is clear. -- ksb
	 */
	fpMove = (FILE *)0;
	for (i = 0; i < argc; ++i) {
		fd = fileno(pfpArgv[i]);
		if (stdin == pfpArgv[i]) {
			if ((FILE *)0 == fpMove) {
				fd = dup(0);
				if (-1 == fd) {
					fprintf(stderr, "%s: dup: 0: %s\n", progname, strerror(errno));
					exit(2);
				}
				if ((FILE *)0 == (fpMove = fdopen(fd, "r"))) {
					fprintf(stderr, "%s: fdopen: %d: %s\n", progname, fd, strerror(errno));
					exit(2);
				}
			}
			pfpArgv[i] = fpMove;
			if (0 == fd) {
				continue;
			}
			/* fall into set close on exec for the stdin dup
			 */
		} else if (1 == fd || 2 == fd) {
			/* through /dev/fd/X I guess!, don't close them
			 */
			continue;
		}
		iOne = 1;
		(void)fcntl(fd, F_SETFD, & iOne);
	}
	/* We moved the stdin references, now close the old
	 * stdin and open the redirect one.  Or the user gave
	 * the redirect option rather than a less than symbol (why?)
	 */
	if ((FILE *)0 != fpMove || (char *)0 != pcRedir) {
		close(0);
		if ((char *)0 == pcRedir) {
			pcRedir = "/dev/null";
		}
		if (0 != open(pcRedir, O_RDONLY, 0666)) {
			fprintf(stderr, "%s: open: %s: %s\n", progname, pcRedir, strerror(errno));
			exit(3);
		}
	}

	iRunning = 0;
	iAdvance = 0 == iPass ? 1 : iPass;
	while (argc > 0) {
		iEofs = iMem = 0;
		for (i = 0; i < iAdvance && iMem < sizeof(acBufs); ++i) {
			ppcSlots[i] = acBufs+iMem;
			while (iMem < sizeof(acBufs)) {
				c = getc(pfpArgv[i]);
				if (EOF == c) {
					if (ppcSlots[i] == acBufs+iMem) {
						ppcSlots[i] = pcPad;
						++iEofs;
					}
					acBufs[iMem++] = '\000';
					break;
				}
				if ('\n' == c) {
					acBufs[iMem++] = '\000';
					break;
				}
				acBufs[iMem++] = c;
			}
		}
		if (i < iAdvance || (EOF != c && '\n' != c)) {
			fprintf(stderr, "%s: total lines is too big for execve\n", progname);
			exit(2);
		}
		/* out of data on all inputs advance to next input group
		 */
		if (iAdvance == iEofs) {
			argc -= iAdvance, argv += iAdvance, pfpArgv += iAdvance;
			continue;
		}
		pcBuilt = AFmt(acCmd, pcCmd, iPass, ppcSlots, argv);
		if ((char *)0 == pcBuilt) {
			break;
		}
		if (!fExec) {
			if (fVerbose)
				printf("%s\n", pcBuilt);
			continue;
		}
		do {
			while (0 < (i = wait3(& wExit, (iRunning < iParallel) ? WNOHANG : 0, (struct rusage *)0))) {
				if (Stop(i))
					--iRunning;
			}
		} while (-1 == (i = Launch(pcBuilt)));
		Start(i);
		++iRunning;
	}
	while (-1 != wait((void *)0)) {
		;
	}
}
