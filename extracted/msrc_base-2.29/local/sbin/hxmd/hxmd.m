#!/usr/bin/env mkcmd
# A replacement for distrib which looks more like xapply.
# $Compile(*): %b -mMkcmd -dprog %f && %b -mCompile -d%s prog.c
# $Compile: %b -mMkcmd -dprog %f && %b -mCompile prog.c
# $Mkcmd(*): ${mkcmd:-mkcmd} -n "%s" %f
comment "%c%kCompile: $%{gcc-gcc%} -g -Wall prog.c"
comment "%c%kCompile(1): $%{gcc-gcc%} -g -DDEBUG=1 -Wall prog.c"
from '<sys/types.h>'
from '<sys/param.h>'
from '<sys/socket.h>'
from '<sys/mman.h>'
from '<sys/wait.h>'
from '<fcntl.h>'
from '<signal.h>'
from '<sys/time.h>'
from '<sys/uio.h>'
from '<strings.h>'
from '<unistd.h>'
from '<errno.h>'
from '<sys/stat.h>'
from '<sysexits.h>'
from '<time.h>'

getenv ;
init 3 "(void)GtfwTrap(argc, argv);"
char* named "pcETmp" { local hidden }
init 8 'if ((char *)0 != (pcETmp = getenv("HXMD_PASS")) && \'\\000\' != *pcETmp) {u_envopt(pcETmp, & argc, & argv);} else if ((char *)0 != (pcETmp = getenv("HXMD"))) {u_envopt(pcETmp, & argc, & argv);}'


require "util_ppm.m" "util_errno.m"
require "std_version.m" "std_help.m"
require "evector.m" "hostdb.m" "slot.m"

augment action 'V' {
	user "Version();"
}

from '"machine.h"'

# for/from xapply
boolean 'n' {
	named 'fExec'
	help "passed to xapply as given"
}
boolean 'x' {
	named "fTrace"
	help "passed to xapply as given"
}
char* 'a' {
	named 'pcEscape'
	param "c"
	help "passed to xapply when given"
}
type "argv" 'e' {
	named "pPPMXapplyEnv"
	param "var=dicer"
	help "xapply environment modifier for each process"
}
boolean 'A' {
	named "fAppend"
	help "passed on to xapply when given"
}
char* 't' {
	named "pcThreads"
	param "file"
	help "passed on xapply when given"
}
char* 'R' {
	named "pcReq"
	param "req"
	help "passed on to xapply (for ptbw) when given"
}
char* 'J' {
	named "pcTasks"
	param "tasks"
	help "passed on to xapply (for ptbw) when given"
}
char* 'i' {
	named "pcXInput"
	param "input"
	help "xapply input option, when given"
}
unsigned 'P' {
	named "uParallel"
	init "6"
	help "number of tasks xapply runs in parallel, defaults to %i"
	param "[jobs]"
	noparam "if (%ypi == (%N = getenv(\"PARALLEL\")) || '\\000' == *%N) {%N = \"1\";}"
	after "if (%n < 1) {%n = 1;}"
}
char* 'S' {
	named "pcExShell"
	param "shell"
	help "xapply shell option, when given"
}
char* 'N' {
	param "else"
	named "pcExElse"
	help "xapply zero-pass command, when given"
}

# for/from xclate
char* 'L' {
	named "pcXclL"
	param "cap"
	help "passed to xclate as given"
}
char* 'H' {
	named "pcXclH"
	param "hr"
	help "passed to xclate as given"
}
char* 'T' {
	named "pcXclT"
	param "title"
	help "passed to xclate as given"
}
char* 'W' {
	named "pcXclW"
	param "widow"
	help "passed to xclate as given"
}
char* 'O' {
	named "pcXclO"
	param "output"
	help "passed to xclate as given"
}
type "argv" 'r' {
	named "pPPMReRuns"
	track "uGaver"
	param "rerun"
	help "process status from each command with STATUS and U defined"
}
type "argv" 'Q' {
	named "pPPMWards"
	track "uGaveQ"
	param "over"
	help "m4 commands included at the top of the rerun processing"
}
char* 'K' {
	named "pcKleaner"
	param "filter"
	help "cleanup hook to process rerun script"
}

# options passed though to m4, dnl
function 'D' {
	named "AddToArgs"
	update "%n(& PPMM4, %w, (isalnum(*%a) || '_' == *%a) ? %a : %a+1);"
	user "%n(& PPMAutoDef, '\\000', %a);"
	param "name=value"
	help "filter defines passed to every m4"
}
function 'U' {
	named "AddToArgs"
	update "%n(& PPMM4, %w, %a);"
	param "name"
	help "filter undefines passed to every m4"
}
function 'I' {
	named "AddToArgs"
	update "%n(& PPMM4, %w, 0 == strcmp(\"--\", %a) ? acTilde : %a);"
	param "dirname"
	help "include directory name passed to every m4"
}
type "argv" 'j' {
	named "pPPMM4Prep"
	param "m4prep"
	help "include specified files in every m4 process\'s arguments"
}
accum[""] 'd' {
	named "pcDebug"
	param "flags"
	user "if ((char *)0 != strchr(%a, '?')) {DebugHelp(stderr);}"
	after "if ((char *)0 == %n) {%n = \"\";}"
	help "debug mask, last specified is passed to m4"
}

# our options
boolean 'g' {
	named "fGtfw"
	help "assure that this instance is wrapped by gtfw"
}
action 'z' {
	from '<stdio.h>'
	update 'setenv("HXMD_PASS", "", 1);'
	help "remove HXMD_PASS from the environment"
}
number {
	named "uPreLoad"
	init "2"
	param "preload"
	help "number of preload slots to fill ahead of processing, default %qi"
}
char* 'M' {	# might be used by msrc (or the like) to change m4 defs
	hidden named "pcXM4Define"
	init '"HXMD_"'
	help "macro filename prefix for m4 cross file references"
}
boolean 's' {
	named "fSlowStart"
	init '1'
	help "turn off any slow-start logic"
}
char* 'c' {
	named "pcXCmd"
	init '"%+"'
	param "cmd"
	help "specify a custom cmd for xapply, rather than %qi"
}
integer 'F' {
	named 'iCmdLits'
	init "1"
	param 'literal'
	help "the number of parameters which are literal text (default %qi)"
}
char* named "pcCmd" {
	param "control"
	help "command expanded by our macro filter"
}
left "pcCmd" {
}
list {
	param "files"
	help "list of files to process, - for stdin"
}

%i
static char rcsid[] =
	"$Id: hxmd.m,v 1.98 2010/07/26 19:53:57 ksb Exp $";

/* mmsrc and hxmd share slot.m, we need the +x bit force-set on slot 0
 */
#define SLOT_0_ORBITS	0100

/* Size of the largest command string between Child and Hxmd
 */
#if !defined(HX_MESSAGE_LEN)
#define HX_MESSAGE_LEN	(1+MAXPATHLEN+1)	/* "%c%s\000" */
#endif

/* Number of slots to work ahead in the child, 1 or 2 is usually
 * enough to keep your machine busy.  Might be a command line option,
 * but few would know how/when to tune it.  We are a function of
 * uParallel, we might want m(Mi)+b on some system.  The code in ReDo
 * depends on this never retuning less than 2 more than iParallel.
 */
#if !defined(HX_AHEAD)
#define HX_AHEAD(Mi)	((Mi)+(uPreLoad < 2 ? 2 : uPreLoad))
#endif

/* Bytes we keep per file, MAXPATHLEN+some, the some for \000 padding
 */
#if !defined(HX_BYTES_PER_FILE)
#define	HX_BYTES_PER_FILE	(MAXPATHLEN+4)
#endif

/* XID length from xclate, we know it is <= 64 today, and we force
 * xapply -u, so it is the number of the task we started, should be
 * at most 6 digits, I'd think.
 */
#if !defined(MAX_XID_LEN)
#define	MAX_XID_LEN	64
#endif
#if MAX_XID_LEN < 10
#abort XID lenght too small to hold a task number
#endif

static char acHxmdPath[] =
	"HXMD_PATH";
static char acDefCmd[] =
	"HX_CMD";
static char acDevNull[] =
	NULL_DEVICE;
static unsigned uLeftLits, uRightLits, uRightBegin;
%%

int named "bHasRoot" {
	init dynamic "0 == geteuid();"
	hidden help "cache our effective uid's root powers"
}

init 4 "InitVectors();"

%c
static char acMySpace[HX_BYTES_PER_FILE];	/* /tmp/$RANDOM to work in*/

/* The version details we love so much					(ksb)
 */
static void
Version()
{
	register char *pcEnvSearch, *pcFrom, *pcSearch;
	register FILE *fpCheck;
	auto size_t wLen;

	printf("%s:  slot code: %s\n", progname, rcs_slot);
	printf("%s:  hostdb code: %s\n", progname, rcs_hostdb);
	printf("%s:  evector code: %s\n", progname, rcs_evector);
	if ((char *)0 != (pcEnvSearch = getenv(acHxmdPath)) && '\000' != *pcEnvSearch) {
		pcFrom = "environment";
		pcSearch = pcEnvSearch;
	} else {
		pcFrom = "default";
		pcSearch = pcHostSearch;
	}
	printf("%s: default column headers: %%%s\n", progname, pcHOST);
	printf("%s: replace an empty control with: %s\n", progname, acDefCmd);
	printf("%s: auto-define prefix, for indexed files and {B,C,U,OPT,STATUS}: %s\n", progname, pcXM4Define);
	printf("%s: parallel factor: %u, slots %u\n", progname, uParallel, HX_AHEAD(uParallel));
	printf("%s: template for private file space: %s\n", progname, acMySpace);
	printf("%s: configuration search list from $%s\n", progname, acHxmdPath);
	printf("%s: %s search list: %s\n", progname, pcFrom, pcSearch);
	printf("%s: %s double-dash directory: %s\n", progname, pcFrom, acTilde);
	printf("%s: cache recipe file: %s\n", progname, acDownRecipe);
	if ((FILE *)0 != (fpCheck = popen("xapply -V", "r")) && (char *)0 != (pcFrom = fgetln(fpCheck, & wLen)) && (char *)0 != (pcFrom = strchr(pcFrom, ','))) {
		do ++pcFrom; while ('\000' != *pcFrom && !isdigit(*pcFrom));
		if (3 > atoi(pcFrom)) {
			printf("%s: xapply major version should be 3, or better (%.2s)\n", progname, pcFrom);
		} else if (44 > atoi(strchr(pcFrom, '.')+1)) {
			printf("%s: xapply minor version must be 44, or better\n", progname);
		}
	} else {
		printf("%s: cannot check xapply version\n", progname);
	}
	if ((FILE *)0 != fpCheck) {
		pclose(fpCheck);
	}
	if ((FILE *)0 != (fpCheck = popen("xclate -V", "r")) && (char *)0 != (pcFrom = fgetln(fpCheck, & wLen)) && (char *)0 != (pcFrom = strchr(pcFrom, ','))) {
		do ++pcFrom; while ('\000' != *pcFrom && !isdigit(*pcFrom));
		if (2 > atoi(pcFrom)) {
			printf("%s: xclate major version should be 2, or better (%.2s)\n", progname, pcFrom);
		} else if (42 > atoi(strchr(pcFrom, '.')+1)) {
			printf("%s: xclate minor version must be 42, or better\n", progname);
		}
	} else {
		printf("%s: cannot check xclate version\n", progname);
	}
	if ((FILE *)0 != fpCheck) {
		pclose(fpCheck);
	}
}

/* tell the customer how to use the -d flag				(ksb)
 */
static void
DebugHelp(FILE *fpOut)
{
	fprintf(fpOut, "%s: -d [BCDGLMRTX,PS][m4-opts]\n", progname);
	fprintf(fpOut, "B\ttrace boolean exclude under -B\n");
	fprintf(fpOut, "C\tdebug cache make requsts\n");
	fprintf(fpOut, "D\tdebug m4 defines\n");
	fprintf(fpOut, "G\ttrace guards and compares under -G and -E\n");
	fprintf(fpOut, "L\tdisplay host list\n");
	fprintf(fpOut, "M\tmore debugging of configuration files (missing values)\n");
	fprintf(fpOut, "R\ttrace cleanup of temporary files\n");
	fprintf(fpOut, "T\ttrace temporary files created for literal strings\n");
	fprintf(fpOut, "X\ttrace execution of child processes\n");
	fprintf(fpOut, "PS\tused by msrc and mmsrc\n");
	fprintf(fpOut, "m4-opts\tany other letters the local m4 accepts, filtered with the letters in $HXMD_M4_DEBUG\n");
	exit(EX_OK);
}

/* Remember the original arguments in case we needed to wrap ourself.	(ksb)
 * We strdup the strings that the hostdb code might modify, if we can.
 */
static char **
GtfwTrap(int argc, char **argv)
{
	register int iOrig, iNew;
	static int iArgc = -1;
	static char **ppcArgv = (char **)0;

	if (-1 != argc) {
		iArgc = argc;
		if ((char **)0 == (ppcArgv = calloc(iArgc+4, sizeof(char *))))
			return (char **)0;
		iOrig = iNew = 0;
		ppcArgv[iNew++] = "gtfw";
		ppcArgv[iNew++] = "-m";
		while (iOrig < argc) {
			if ((char *)0 == (ppcArgv[iNew++] = strdup(argv[iOrig++]))) {
				ppcArgv[iNew-1] = argv[iOrig-1];
			}
		}
		ppcArgv[iNew] = (char *)0;
	}
	return ppcArgv;
}

/* Base vectors for the AddToArgs to manipulate, see InitVectors
 */
static PPM_BUF
	PPMM4,		/* m4 [-Uundef] [-Dvar=value] [-Idir] [-dflags]	*/
	PPMXclate,	/* xclate -m [-H..] [...] -N '>&5'		*/
	PPMXapply;	/* -- xapply -[nx]P... -mfuz ... '%+' -...	*/

/* We need to make a few argv segements, this helps.			(ksb)
 * Add a pair like {"-x", "param"} to the end of the vector and slide the
 * NULL over to the right.  N.B. pcValue can be (char *)0 for a switch w/o
 * a parameter, but this doen't work for -Pjobs (or other :: options).
 * In that case pass cLetter = '\000' and pcValue is the "-Whole".
 */
static void
AddToArgs(PPM_BUF *pPPM, int cLetter, char *pcValue)
{
	register unsigned int i;
	register char **ppc;

	if ((char **)0 == (ppc = (char **)util_ppm_size(pPPM, (unsigned)0))) {
		fprintf(stderr, "%s: ppm_size: %d: %s\n", progname, 0, strerror(errno));
		exit(EX_UNAVAILABLE);
	}
	for (i = 0; (char *)0 != ppc[i]; ++i) {
		/*nada*/
	}
	if ((char **)0 == (ppc = (char **)util_ppm_size(pPPM, i+3))) {
		fprintf(stderr, "%s: ppm_size: %d: %s\n", progname, i+3, strerror(errno));
		exit(EX_UNAVAILABLE);
	}
	if ('\000' != cLetter) {
		ppc[i] = strdup("-_");
		ppc[i][1] = cLetter;
		++i;
	}
	ppc[i] = pcValue;
	ppc[i+1] = (char *)0;
}

/* Write a \000 terminated command to our buddy, over a pipe.		(ksb)
 * ignore any pipe signals when fBlock is set.
 */
static int
WriteCmd(int fd, char *pcSend, char *pcMessage, int fBlock)
{
	register int iRet;
	auto struct iovec aIO[2];
	auto struct sigaction saBlock, saRestore;

	if (-1 == fd) {
		return -1;
	}
	aIO[0].iov_base = pcSend;
	aIO[0].iov_len = 1;
	if ((char *)0 == (aIO[1].iov_base = pcMessage)) {
		aIO[1].iov_base = "";
	}
	aIO[1].iov_len = strlen(aIO[1].iov_base)+1;
#if DEBUG
	fprintf(stderr, "%s: WriteCmd(%d, %c%s\\000)\n", progname, fd, *pcSend, aIO[1].iov_base);
#endif
	if (!fBlock) {
		return writev(fd, & aIO[0], 2);
	}
	(void)memset((void *)&saBlock, '\000', sizeof(saBlock));
	saBlock.sa_handler = SIG_IGN;
	saBlock.sa_flags = SA_RESTART;
	if (-1 == sigaction(SIGPIPE, &saBlock, &saRestore)) {
		fBlock = 0;
	}
	iRet = writev(fd, & aIO[0], 2);
	(void)sigaction(SIGPIPE, &saRestore, (struct sigaction *)0);
	return iRet;
}

/* Read a \000 terminated command from our buddy			(ksb)
 */
static int
ReadCmd(int fd, char *pcMessage)
{
	register int iCc, i, cRet;
	auto char cIn;

	cRet = -1;
	*pcMessage = '\000';
	for (i = -1; 1 == (iCc = read(fd, &cIn, 1)); ++i) {
		if (-1 == i)
			cRet = cIn;
		else if ('\000' == (pcMessage[i] = cIn))
			break;
	}
#if DEBUG
	if (-1 == cRet) {
		fprintf(stderr, "%s: EOF = Cmd(%d, %s)\n", progname, fd, pcMessage);
	} else {
		fprintf(stderr, "%s: %c = Cmd(%d, %s)\n", progname, cRet, fd, pcMessage);
	}
#endif
	return cRet;
}

static unsigned uRecSz;		/* Max size of any file list	*/
static unsigned uPrep;		/* Number of slots we use	*/

/* Filter all the files for each host recovered from			(ksb)
 * the host database information, and start the tasks under xapply.
 * We wait for Hxmd to fire up (below), then let him know that we'd
 * like to start some tasks, he tells us what to start and when to
 * cleanup a slot (then refill it).
 */
static META_HOST *
Child(int fdHxmd, char *pcFiles, char **argv, char **ppcM4, const int *piIsCache, const char *pcCacheCtl)
{
	auto char acText[HX_MESSAGE_LEN];	/* "%x,%d", or "c$path" */
	register unsigned i, fDid;
	register META_HOST *pMHNext;
	register unsigned uAble, u, uCmdLits;
	register char *pcComma;
	typedef struct SFnode {
		char *pcbase;		/* Concat of paths+'\000'	*/
		unsigned ulen;		/* Length of concat		*/
		struct SFnode *pSFnext;	/* Next slot with same state	*/
		META_HOST *pMHredo;	/* for -r to redo each host	*/
	} SLOT_FUEL;
	register SLOT_FUEL **ppSF, **ppSFTailR;
	register SLOT_FUEL *pSFTemp;
	auto SLOT_FUEL *pSFInit, *pSFMem, *pSFRun, *pSFReady;
	auto fd_set fdsCheck;
	auto struct timeval tvPoll;
	auto int fdCmd;
	auto META_HOST *pMHRet;

	/* Turn the literal commands into a files, this makes it
	 * way easier to m4 in FillSlot.
	 */
	uCmdLits = uLeftLits + uRightLits;
	for (u = 0; u < uCmdLits; ++u, ++i) {
		i = u < uLeftLits ? u : (uRightBegin+u - uLeftLits);
		snprintf(acText, sizeof(acText), "%s/cmd%u", acMySpace, u);
		if (-1 == (fdCmd = open(acText, O_RDWR|O_CREAT|O_EXCL, 0600))) {
			fprintf(stderr, "%s: open: %s: %s\n", progname, acText, strerror(errno));
			exit(EX_CANTCREAT);
		}
		if ((char *)0 != strchr(pcDebug, 'T')) {
			fprintf(stderr, "%s: echo \"%s\" >%s\n", progname, argv[i], acText);
		}
		write(fdCmd, argv[i], strlen(argv[i]));
		close(fdCmd);
		argv[i] = strdup(acText);
	}

	/* Poll at "fib22 microseconds", about 1/35th of a second
	 * this helps avoid the load spike, but we'll still start about
	 * 32 a second on a fast multiprocessor host -- ksb.
	 */
	tvPoll.tv_sec = 0;
	tvPoll.tv_usec = 28657;

	/* Make a mkstemp suffix that is wide enough, but not too wide.
	 * One character wider than the strlen of the max slot number
	 * works well.  On broken Linux one _must_ use exactly 6 X's.
	 */
#if BROKEN_MKTEMP
	snprintf(acText, sizeof(acText), "XXXXXX");
#else
	snprintf(acText, sizeof(acText), "%u", uPrep);
	for (i = 0; '\000' != acText[i]; ++i) {
		acText[i] = 'X';
	}
	acText[i++] = 'X';
	acText[i] = '\000';
#endif

	/* Build our bookkeepping queues and such
	 */
	pSFRun = (SLOT_FUEL *)0;
	pSFReady = (SLOT_FUEL *)0;
	ppSFTailR = & pSFReady;
	if ((SLOT_FUEL *)0 == (pSFMem = (SLOT_FUEL *)calloc(uPrep, sizeof(SLOT_FUEL)))) {
		fprintf(stderr, "%s: calloc(%u, %u): %s\n", progname, uPrep, (unsigned)sizeof(SLOT_FUEL), strerror(errno));
		exit(EX_OSERR);
	}
	ppSF = & pSFInit;
	for (i = 0; i < uPrep; ++i) {
		pSFMem[i].pcbase = & pcFiles[i*uRecSz];
		pSFMem[i].ulen = InitSlot(acText, pSFMem[i].pcbase, argv, acMySpace);
		pSFMem[i].pMHredo = (META_HOST *)0;
		*ppSF = & pSFMem[i];
		ppSF = & pSFMem[i].pSFnext;
	}
	*ppSF = (SLOT_FUEL *)0;
	pMHRet = pMHNext = HostInit(ppcM4);

	/* We depend on our Hxmd to send us an initial "G" command.  Which
	 * tells us to start at uParallel tasks.
	 */
	uAble = 0;
	switch (ReadCmd(fdHxmd, acText)) {
	case 'G':	/* Go! protocol		*/
		uAble = uParallel;
		break;
	case '+':	/* Slow start protocol	*/
		uAble = 1;
		break;
	case 'Q':	/* Parent failed init	*/
		exit(EX_OK);
	default:
		fprintf(stderr, "%s: Child: failed to sync with parent (%s)\n", progname, acText);
		exit(EX_SOFTWARE);
	}

	/* While we have hosts, running tasks, or ready tasks advance the
	 * state-machine
	 */
	FD_ZERO(& fdsCheck);
	while ((META_HOST *)0 != pMHNext || (SLOT_FUEL *)0 != pSFRun || (SLOT_FUEL *)0 != pSFReady) {
#if DEBUG
		fprintf(stderr, "%s: top: allows=%u, init=%p, host=%p, ready=%p, run=%p\n", progname, uAble, pSFInit, pMHNext, pSFReady, pSFRun);
#endif
		fDid = 0;
		/* Build another waiting task, if we can
		 */
		if ((SLOT_FUEL *)0 != pSFInit && (META_HOST *)0 != pMHNext) {
			auto int wExit;
			pSFTemp = pSFInit;
			pSFInit = pSFInit->pSFnext;
			pSFTemp->pMHredo = pMHNext;
			pSFTemp->pSFnext = (SLOT_FUEL *)0;
			if (0 == (wExit = FillSlot(pSFTemp->pcbase, argv, ppcM4, pMHNext, piIsCache, pcCacheCtl))) {
				*ppSFTailR = pSFTemp;
				ppSFTailR = & pSFTemp->pSFnext;
				fDid = 1;
			} else if (WIFEXITED(wExit)) {
				fprintf(stderr, "%s: %s: %s: exits code %d (host skipped)\n", progname, pMHNext->pchost, ppcM4[0], WEXITSTATUS(wExit));
				pMHNext->istatus = 1000+WEXITSTATUS(wExit);
			} else {
				fprintf(stderr, "%s: %s: %s: caught signal %d (host skipped)\n", progname, pMHNext->pchost, ppcM4[0], WTERMSIG(wExit));
				pMHNext->istatus = 2000+wExit;
			}
			pMHNext = NextHost();
		}
		/* We'll queue-for-launch as many as we can, by telling
		 * Hxmd that the slot is ready, see 'r' below
		 */
		while ((SLOT_FUEL *)0 != pSFReady && 0 != uAble) {
			pSFTemp = pSFReady;
			pSFReady = pSFReady->pSFnext;
			if ((SLOT_FUEL *)0 == pSFReady) {
				ppSFTailR = & pSFReady;
			}
			snprintf(acText, sizeof(acText), "%u", (unsigned)(pSFTemp-pSFMem));
			pSFTemp->pSFnext = pSFRun;
			pSFRun = pSFTemp;
			--uAble;
			WriteCmd(fdHxmd, "a", acText, 0);
			fDid = 1;
		}

		/* If we did nothing from the top of the loop to this point
		 * we might as well block wating for Hxmd to talk to us,
		 * else we timeout and build another task to run.
		 */
		FD_SET(fdHxmd, & fdsCheck);
		if (-1 == (i = select(fdHxmd+1, &fdsCheck, (void *)0, (void *)0, fDid ? & tvPoll : (struct timeval *)0))) {
			if (errno == EINTR)
				continue;
			break;
		}
		if (0 == i) {
			continue;
		}
		switch (ReadCmd(fdHxmd, acText)) {
		case -1:	/* Found EOF, or error		*/
			/*fallthrough*/
		case 'Q':	/* Quit now			*/
			break;
		case 'r':	/* Run slot, for real		*/
			i = atoi(acText);
			pSFTemp = pSFMem+i;
			PrintSlot(pSFTemp->pcbase, uLeftLits, uRightBegin, 1);
			continue;
		case 'd':	/* Done, move back to init [status,]xid */
			pcComma = (char *)0;
			if ((char *)0 != (pcComma = strchr(acText, ','))) {
				*pcComma++ = '\000';
				i = atoi(pcComma);
			} else {
				i = atoi(acText);
			}
			pSFTemp = pSFMem+i;
			for (ppSF = & pSFRun; (SLOT_FUEL *)0 != *ppSF; ppSF = & ppSF[0]->pSFnext) {
				if (pSFTemp != *ppSF) {
					continue;
				}
				if ((char *)0 != pcComma) {
					pSFTemp->pMHredo->istatus = atoi(acText);
				}
				*ppSF = pSFTemp->pSFnext;
				pSFTemp = (SLOT_FUEL *)0;
				break;
			}
			if ((SLOT_FUEL *)0 != pSFTemp) {
				snprintf(acText, sizeof(acText), "slot%d: was not on the run list!", i);
				WriteCmd(fdHxmd, "p", acText, 0);
				continue;
			}
			pSFMem[i].pSFnext = pSFInit;
			pSFInit = pSFMem+i;
			++uAble;
			continue;
		case '+':	/* Slow start, start next	*/
			++uAble;
			continue;
		case 'p':	/* Print something		*/
			fprintf(stderr, "%s: %s\n", progname, acText);
			continue;
		case 'V':	/* Return protocol version	*/
			WriteCmd(fdHxmd, "p", "0.3", 0);
			continue;
		}
		break;
	}
	/* Tell xclate we are done, so it will exit, then
	 * shutdown the peer connection to hxmd.
	 */
	argv[0] = "done";
	close(1);
	(void)open("/dev/null", O_WRONLY, 0666);
	close(fdHxmd);
	return pMHRet;
}

/* The parent process reads status data on from the Child and		(ksb)
 * xclate's task notifications stream for status codes.
 * Never exit here, must clean up in caller -- ksb
 */
static void
Hxmd(int fdChild, char *pcFiles, int fdNotify, int *pwElse)
{
	auto char acText[HX_MESSAGE_LEN];
	auto char acStatus[16+MAX_XID_LEN];	/* "%d,%x\000" */
	auto fd_set fdsEither, fdsLoop;
	auto struct timeval tvStart;
	auto time_t wStart, wNow;
	register int iCc, iScan;
	register int iSelMax, iLook, iSlow;
	register unsigned uXid;
	typedef struct SDnode {
		char acxid[MAX_XID_LEN];
	} SLOT_DATA;
	register SLOT_DATA *pSDMem;
	register struct timeval *ptvBlock;
	register char *pcComma;

	/* Build our slot bookkeepping, just the xid we expect from xclate
	 */
	if ((SLOT_DATA *)0 == (pSDMem = (SLOT_DATA *)calloc(uPrep, sizeof(SLOT_DATA)))) {
		fprintf(stderr, "%s: calloc(%u, %u): %s\n", progname, uParallel, (unsigned)sizeof(SLOT_DATA), strerror(errno));
		WriteCmd(fdChild, "Q", "no memory", 1);
		return;
	}
	for (iScan = 0; iScan < uPrep; ++iScan) {
		pSDMem[iScan].acxid[0] = '\000';
	}

	/* Setup select overhead
	 */
	FD_ZERO(&fdsEither);
	FD_SET(fdChild, &fdsEither);
	FD_SET(fdNotify, &fdsEither);
	iSelMax = (fdChild < fdNotify ? fdNotify : fdChild);

	/* Setup slow-start code, if uParallel is too big, or -s is set.
	 * We let the Child spin 1 task and about ~18 prep slots ahead
	 * of us (fib28/fib22), then we ramp up the tasks about 2 per
	 * second.  So we see more tasks and less prep until we have
	 * uParallel tasks and HX_AHEAD prep -- normal operation.
	 */
	time(& wStart);
	if (!fSlowStart || 1 == uParallel) {
		WriteCmd(fdChild, "G", "child", 0);
		ptvBlock = (struct timeval *)0;
		iSlow = 0;
	} else {
		WriteCmd(fdChild, "+", "slow", 0);
		tvStart.tv_sec = 0;
		tvStart.tv_usec = 514229;	/* fib28 about 0.5s */
		ptvBlock = & tvStart;
		iSlow = uParallel-1;
	}

	uXid = 0;
	for (iLook = 2; 0 != iLook; /*nada*/) {
		fdsLoop = fdsEither;
		if (-1 == (iCc = select(iSelMax+1, & fdsLoop, (void *)0, (void *)0, ptvBlock))) {
			if (errno == EINTR)
				continue;
			break;
		}
		/* When we take the slow-start timeout see if it has been half
		 * a second since the last "+" command.  If so we start one.
		 * Else we poll in 0.317811s (fib27) hoping we'll cross the
		 * second boundry.  The Child is prepping slots with lots of
		 * m4 processes while we stall the tasks, it has time to
		 * prep about 11 more per stall (fib27/fib22).
		 */
		if ((struct timeval *)0 != ptvBlock) {
			time(& wNow);
			tvStart.tv_usec = 317811;
			if (wStart < wNow) {
				WriteCmd(fdChild, "+", "ramp", 1);
				wStart += iSlow & 1;
				if (0 == --iSlow)
					ptvBlock = (struct timeval *)0;
			}
		}
		if (FD_ISSET(fdNotify, &fdsLoop)) {
			static int fOnce = 1;
			iCc = ReadCmd(fdNotify, acStatus+1);
			if (-1 == iCc) {
				FD_CLR(fdNotify, &fdsEither);
				--iLook;
				continue;
			}
			if (!isdigit(iCc)) {
				fprintf(stderr, "%s: %c%s: expected numeric xid\n", progname, iCc, acStatus+1);
				continue;
			}
			acStatus[0] = iCc;
			/* find the code, first
			 */
			if ((char *)0 != (pcComma = strchr(acStatus, ','))) {
				++pcComma;
			} else {
				pcComma = acStatus;
			}
			for (iScan = 0; iScan < uPrep; ++iScan) {
				if (0 == strcmp(pcComma, pSDMem[iScan].acxid))
					break;
			}
			if (iScan != uPrep) {
				snprintf(pcComma, sizeof(acStatus)-4, "%d", iScan);
				WriteCmd(fdChild, "d", acStatus, 0);
				pSDMem[iScan].acxid[0] = '\000';
				continue;
			}

			/* Notification for non-running task?
			 * Was is the "else clause?", the xid is "00" for
			 * that case, and -N must be on our command line.
			 */
			if ('0' == pcComma[0] && '0' == pcComma[1] && '\000' == pcComma[2]) {
				if (pcComma != acStatus) {
					pcComma[-1] = '\000';
				}
				if ((int *)0 != pwElse) {
					*pwElse = atoi(acStatus);
				}
				continue;
			}
			/* Work about xapply 3.16 bug, we hope
			 */
			if (fOnce) {
				fOnce = 0;
				WriteCmd(fdChild, "d", "0", 0);
			}
			fprintf(stderr, "%s: %s: xid not running\n", progname, pcComma);
#if DEBUG
			for (iScan = 0; iScan < uPrep; ++iScan) {
				if ('\000' == pSDMem[iScan].acxid[0])
					continue;
				fprintf(stderr, "\t%d: %s\n", iScan, pSDMem[iScan].acxid);
			}
#endif
			WriteCmd(fdChild, "d", "0", 0);
			continue;
		}
		if (!FD_ISSET(fdChild, &fdsLoop)) {
			continue;
		}
		switch (ReadCmd(fdChild, acText)) {
		case -1:	/* EOF				*/
			/*fallthrough*/
		case 'Q':	/* Child finished, closing	*/
			--iLook;
			FD_CLR(fdChild, &fdsEither);
			continue;
		case 'a':	/* Able {slot}			*/
			iScan = atoi(acText);
			snprintf(pSDMem[iScan].acxid, sizeof(pSDMem[0].acxid), "%u", uXid++);
#if DEBUG
			fprintf(stderr, "%s: got a new task %d \"%s\" -- xid %s\n", progname, iScan, acText, pSDMem[iScan].acxid);
#endif
			WriteCmd(fdChild, "r", acText, 1);
			/* When we are not running we'll not get a notify,
			 * so fake an immediate done reply here.
			 */
			if (fExec) {
				WriteCmd(fdChild, "d", acText, 1);
			}
			continue;
		case 'V':	/* Version command		*/
			WriteCmd(fdChild, "p", "0.3", 0);
			continue;
		case 'p':	/* Print data			*/
			fprintf(stderr, "%s: %s\n", progname, acText);
			continue;
		default:
			fprintf(stderr, "%s: unknown command: ?%s\n", progname, acText);
			break;
		}
		break;
	}
	close(fdNotify);
	if (-1 != fdChild) {
		close(fdChild);
	}
}

/* Build the argument vector PPM spaces, and set some fixed args	(ksb)
 */
static void
InitVectors()
{
	register char **ppc;
	register char *pcEnvTemp;

	/* Copy down debug options m4 knows about
	 */
	util_ppm_init(& PPMM4, sizeof(char *), 32);
	ppc = util_ppm_size(& PPMM4, 32);
	ppc[0] = "m4";
	ppc[1] = (char *)0;

	util_ppm_init(& PPMXclate, sizeof(char *), 32);
	ppc = util_ppm_size(& PPMXclate, 32);
	ppc[0] = "xclate";
	ppc[1] = "-m";		/* in list_func we may replace with "-ms" */
	ppc[2] = (char *)0;

	util_ppm_init(& PPMXapply, sizeof(char *), 32);
	ppc = util_ppm_size(& PPMXapply, 32);
	ppc[0] = "--";
	ppc[1] = "xapply";
	ppc[2] = "-fzmu";
	ppc[3] = (char *)0;
	if ((char *)0 == (pcEnvTemp = getenv("TMPDIR")) || '\000' == *pcEnvTemp) {
		pcEnvTemp = "/tmp";
	}
	snprintf(acMySpace, MAXPATHLEN, "%s/hxtfXXXXXX", pcEnvTemp);
}

/* Build an argv and execvp xclate					(ksb)
 * the new argv is <xclate-args> <xapply-argx>
 */
static void
RunXclate()
{
	register char **ppcArgs, **ppc;

	for (ppc = util_ppm_size(& PPMXapply, 0); (char *)0 != *ppc; ++ppc) {
		AddToArgs(& PPMXclate, '\000', *ppc);
	}
	ppcArgs = util_ppm_size(& PPMXclate, 0);
	if ((char *)0 != strchr(pcDebug, 'X')) {
		fprintf(stderr, "%s:", progname);
		for (ppc = ppcArgs; (char *)0 != *ppc; ++ppc) {
			fprintf(stderr, " %s", *ppc);
		}
		fprintf(stderr, "\n");
		fflush(stderr);
	}
	/* We go ahead and run the the xclate under -n, it won't run others
	 */
	(void)execvp("xclate", ppcArgs);
	(void)execvp("/usr/local/bin/xclate", ppcArgs);
	fprintf(stderr, "%s: execvp: xclate: %s\n", progname, strerror(errno));
	exit(EX_OSFILE);
}

/* Run a post loop command to cleanup					(ksb)
 * Build a HXMD_0..N for the orginal argv list, build a selection
 * based on -r for each host processed,
 */
int
ReDo(META_HOST *pMHList, char *pcTemp, char **argv, char **ppcM4Argv)
{
	static char acSTATUS[] = "STATUS", acU[] = "U";
	register unsigned uCount, fIsPipe;
	register FILE *fp;
	register pid_t wExits;
	register char **ppcInfo, **ppcWards, *pcReControl, *pcShell;
	auto pid_t wFilter;
	auto int wStatus;
	auto struct stat stFake;
	auto char *apcKleaner[8];

	/* If there is not a request to redo or no hosts done, just return OK
	 */
	if (!uGaver || (META_HOST *)0 == pMHList) {
		return EX_OK;
	}

	/* Put in something for -r if nothing was given, if you really
	 * want nothing give a " " (space).  Default to "host status %u".
	 * buy default -K pages the resultant status, and -Q is nonthing.
	 */
	ppcInfo = util_ppm_size(pPPMReRuns, 2);
	if ((char *)0 == *ppcInfo || ('\000' == ppcInfo[0][0] && (char *)0 == ppcInfo[1])) {
		register int iLen;
		static char acFmt[] = "%s %s%s %s%s";

		iLen = 7|(sizeof(acFmt)+strlen(pcHOST)+sizeof(acSTATUS)+sizeof(acU)+2*strlen(pcXM4Define));
		if ((char *)0 == (ppcInfo[0] = malloc(++iLen))) {
			return EX_CANTCREAT;
		}
		snprintf(ppcInfo[0], iLen, acFmt, pcHOST, pcXM4Define, acSTATUS, pcXM4Define, acU);
		ppcInfo[1] = (char *)0;
	}
	if ((char *)0 == pcKleaner || '\000' == *pcKleaner) {
		register int iLen;
		static char acFmt[] = "${PAGER:-more} %s0";

		iLen = 7|(sizeof(acFmt)+strlen(pcXM4Define));
		if ((char *)0 == (pcKleaner = malloc(++iLen))) {
			return EX_CANTCREAT;
		}
		snprintf(pcKleaner, iLen, acFmt, pcXM4Define);
	}
	if (0 != (fIsPipe = '|' == pcKleaner[0])) {
		++pcKleaner;
	}
	ppcWards = util_ppm_size(pPPMWards, 0);

	stFake.st_mode = 0700;
	stFake.st_uid = getuid();
	stFake.st_gid = getgid();
	SetCurPhase("redo");
	if ((FILE *)0 == (fp = M4Gadget(pcTemp, & stFake, &wFilter, ppcM4Argv))) {
		return EX_UNAVAILABLE;
	}
	SetCurPhase((char *)0);

	fprintf(fp, "define(`%s0',`%s')", pcXM4Define, pcTemp);
	for (uCount = 0; '\000' != argv[uCount]; ++uCount) {
		fprintf(fp, "define(`%s%u',`%s')", pcXM4Define, 1+uCount, argv[uCount]);
	}
	fprintf(fp, "define(`%sC',`%u')dnl\n", pcXM4Define, uCount);
	RecurBanner(fp);
	while ((char *)0 != *ppcWards) {
		fprintf(fp, "%s\n", *ppcWards++);
	}
	uCount = 0;	/* inv. in sync with %u */
	for (/* */; (META_HOST *)0 != pMHList; pMHList = pMHList->pMHnext) {
		fprintf(fp, "pushdef(`%s%s',%d)pushdef(`%sU',%d)dnl\n", pcXM4Define, acSTATUS, pMHList->istatus, pcXM4Define, uCount++);
		BuildM4(fp, ppcInfo, pMHList, (char *)0, (char *)0);
#if M4_POPDEF_MANY
		fprintf(fp, "popdef(`%s%s',`%sU')dnl\n", pcXM4Define, acSTATUS, pcXM4Define);
#else
		fprintf(fp, "popdef(`%s%s')popdef(`%sU')dnl\n", pcXM4Define, acSTATUS, pcXM4Define);
#endif
	}
	fclose(fp);
	while (errno = 0, -1 != (wExits = wait(&wStatus)) || EINTR == errno) {
		if (wExits == wFilter)
			break;
	}

	/* We depend on the fact that HX_AHEAD makes at least 2 slots for us
	 * we get another cmd file from the next slot -- ksb.
	 */
	pcReControl = pcTemp+uRecSz;
	SetCurPhase("filter");
	if ((FILE *)0 == (fp = M4Gadget(pcReControl, & stFake, &wFilter, ppcM4Argv))) {
		return EX_UNAVAILABLE;
	}
	SetCurPhase((char *)0);
	fprintf(fp, "define(`%s0',`%s')", pcXM4Define, pcTemp);
	for (uCount = 0; '\000' != argv[uCount]; ++uCount) {
		fprintf(fp, "define(`%s%u',`%s')", pcXM4Define, 1+uCount, argv[uCount]);
	}
	fprintf(fp, "define(`%sC',`%u')dnl\n", pcXM4Define, uCount);
	RecurBanner(fp);
	fprintf(fp, "%s\n", pcKleaner);
	fclose(fp);
	while (errno = 0, -1 != (wExits = wait(&wStatus)) || EINTR == errno) {
		if (wExits == wFilter)
			break;
	}

	/* Using -S is not really a limiter: to work around it set XCLATE to
	 * -e SHELL=what-xapply-wants, or use the HXMD_SHELL to trap it out.
	 */
	if ((char *)0 != (pcShell = getenv("HXMD_SHELL"))) {
		/* OK */
	} else if ((char *)0 != pcExShell) {
		pcShell = pcExShell;
	} else if ((char *)0 == (pcShell = getenv("SHELL"))) {
		pcShell = "/bin/sh";
	}
	if ((char *)0 != (apcKleaner[0] = strrchr(pcShell, '/'))) {
		apcKleaner[0] += 1;
	} else {
		apcKleaner[0] = pcShell;
	}
	apcKleaner[1] = ((char *)0 != strstr(pcShell, "perl")) ? "-e" : "-c";
	apcKleaner[2] = pcReControl;
	apcKleaner[3] = (char *)0;
	switch (wFilter = fork()) {
	case -1:	/* crud */
		return EX_UNAVAILABLE;
	case 0:
		if (fIsPipe) {
			close(0);
			(void)open(pcTemp, O_RDONLY, 0600);
		}
		/* Stdout is open to /dev/null now, shift that to stderr.
		 * I worry about this: we might be Wrong to use stderr as
		 * an output stream.  The theory is that any recovery we
		 * take is in response on an error of some kind. -- ksb
		 */
		close(1);
		dup(2);
		execvp(pcShell, apcKleaner);
		execvp("sh", apcKleaner);
		fprintf(stderr, "%s: execvp: %s: %s\n", progname, pcShell, strerror(errno));
		exit(EX_OSFILE);
	default:
		break;
	}
	while (errno = 0, -1 != (wExits = wait(& wStatus)) || EINTR == errno) {
		if (wExits == wFilter)
			break;
	}
	return wStatus>>8;
}

/* Form the commands and pipes to start the nuke reation by building	(ksb)
 * a cache for m4 processed files, forking the workers. Then provide
 * cleanup for the temp files and directories.
 */
static void
list_func(unsigned argc, char **argv)
{
	auto char acTemp[MAXPATHLEN+512], acRecipe[MAXPATHLEN+4];
	register char *pcEnd, **ppc;
	register char *pcM4Allows;
	register unsigned i, u;
	auto int aiXclatePipe[2], aiNotify[2], aiPeer[2], fdPages, wElse;
	auto size_t iShared, iPageSz;
	auto void *pvShared;
	auto pid_t wXclatePid, wPeerPid, wWait;
	auto int wStatus, iExit, fScriptDash, *piIsCache;
	auto char *pcHostSlot;
	auto char *apcLook[2];
	auto META_HOST *pMHReRun;
	auto const char *pcParamType, *pcCacheCtl;
	static const char
		acCntName[] = "cache control recipe",
		acPlainName[] = "marked-up file";

	/* Make sure 0, 1, 2 are open'd, to avoid corner cases below
	 */
	do {
		i = open(acDevNull, O_RDWR, 0666);
	} while (i > -1 && i < 3);
	close(i);

	/* if we are not in a gtfw and we need to be start over
	 */
	if (fGtfw && ((char *)0 == (pcParamType = getenv("gtfw_link")) || '\000' == *pcParamType)) {
		register char **ppcGtfw;
		if ((char **)0 == (ppcGtfw = GtfwTrap(-1, (char **)0))) {
			fprintf(stderr, "%s: malloc: %s\n", progname, strerror(errno));
			exit(EX_OSERR);
		}
		if ((char *)0 != strchr(pcDebug, 'X')) {
			fprintf(stderr, "%s:", progname);
			for (ppc = ppcGtfw; (char *)0 != *ppc; ++ppc) {
				fprintf(stderr, " %s", *ppc);
			}
			fprintf(stderr, "\n");
			fflush(stderr);
		}
		(void)execvp("gtfw", ppcGtfw);
		(void)execvp("/usr/local/bin/gtfw", ppcGtfw);
		fprintf(stderr, "%s: execvp: gtfw: %s\n", progname, strerror(errno));
		exit(EX_OSFILE);
	}

	/* Build the M4 phase buffer for HXMD_PHASE=(selection|%d|filter|redo)
	 */
	AddToArgs(&PPMM4, 'D', SetupPhase("PHASE", 64));

	/* Put the fixed m4 prep files in the m4 template.
	 * Then add the dash for stdin, so m4 will read our pipe.
	 */
	if ((char **)0 != (ppc = util_ppm_size(pPPMM4Prep, 0))) {
		while ((char *)0 != *ppc) {
			AddToArgs(&PPMM4, '\000', *ppc++);
		}
	}
	AddToArgs(&PPMM4, '\000', "-");

	/* Hack alert: we depend on argv extending back a slot,
	 * we know that mkcmd read the pcCmd from that slot so
	 * it is always there, or at least progname should be.
	 */
	--argv, ++argc;
	argv[0] = '\000' == *pcCmd ? acDefCmd : pcCmd;
	uRightBegin = argc;
	uLeftLits = uRightLits = 0;
	if (0 <= iCmdLits) {
		uLeftLits = iCmdLits;
	} else {
		uRightLits = -iCmdLits;
		uRightBegin -= uRightLits;
	}
	i = (uLeftLits + uRightLits);
	if (argc < i) {
		fprintf(stderr, "%s: not enough parameters for literals (%d < %u)\n", progname, argc, i);
		exit(EX_USAGE);
	}

	if ((int *)0 == (piIsCache = calloc((argc|3)+1, sizeof(int)))) {
		fprintf(stderr, "%s: calloc: %u,%d: %s\n", progname, (argc|3)+1, (int)sizeof(int), strerror(errno));
		exit(EX_TEMPFAIL);
	}
	/* Don't even try unless we can see the files in argv, we do
	 * not use access(2) because it is lame.  Don't use open on
	 * non-regular files because they might be FIFOs balanced for
	 * the host list: yes I really do that. -- ksb
	 */
	acTemp[0] = '\000';
	apcLook[1] = (char *)0;
	fScriptDash = 0;
	for (i = 0; i < argc; ++i) {
		register int iOK;
		register char *pcFound;
		auto struct stat stCheck;

		piIsCache[i] = 0;
		if (i < uLeftLits || i >= uRightBegin) {
			continue;
		}
		if ('-' == argv[i][0] && '\000' == argv[i][1]) {
			ReserveStdin(& argv[i], "files");
			fScriptDash = 0 == i;
			continue;
		}
		pcParamType = acPlainName;
		apcLook[0] = argv[i];
		if ('.' == argv[i][0] && '/' == argv[i][1]) {
			/* explicity do not search for file */
		} else if ((char *)0 == (pcFound = util_fsearch(apcLook, getenv(acHxmdPath)))) {
			fprintf(stderr, "%s: %s: cannot find file\n", progname, argv[i]);
			acTemp[0] = 'n';
			continue;
		} else {
			argv[i] = pcFound;
		}
		if (0 != stat(argv[i], & stCheck)) {
			fprintf(stderr, "%s: stat: %s: %s\n", progname, argv[i], strerror(errno));
			acTemp[0] = 'n';
			continue;
		}

		/* A request for $target.host/ accesses a cache dir via make
		 *	m4Gadget $target.host/Cache.m4 >$tfile
		 *	cd $target.host && make -f $tfile $target >$target
		 * this allows processors other than m4 for mapped files, and
		 * it creates a better host after host state machine.
		 */
		if (S_ISDIR(stCheck.st_mode)) {
			snprintf(acRecipe, sizeof(acRecipe), "%s/%s", argv[i], acDownRecipe);
			if ((char *)0 == (argv[i] = strdup(acRecipe))) {
				fprintf(stderr, "%s: strdup: %s\n", progname, strerror(errno));
				exit(EX_TEMPFAIL);
			}
			if (0 != stat(argv[i] , &stCheck)) {
				fprintf(stderr, "%s: stat: %s: %s\n", progname, argv[i], strerror(errno));
				acTemp[0] = '/';
				continue;
			}
			pcParamType = acCntName;
			piIsCache[i] = 1;
		}
		if (S_ISFIFO(stCheck.st_mode)) {
			/* I don't see any reason to disallow this, and I
			 * can see several uses for it: uniq resources per
			 * host (in a known order), synchronous execution,
			 * or just plain showing off.
			 */
		} else if (!S_ISREG(stCheck.st_mode)) {
			fprintf(stderr, "%s: %s: %s must be plain files or FIFOs\n", progname, acRecipe, pcParamType);
			acTemp[0] = 'm';
			continue;
		} else if (-1 == (iOK = open(argv[i], O_RDONLY, 0666))) {
			fprintf(stderr, "%s: open: %s: %s\n", progname, argv[i], strerror(errno));
			acTemp[0] = 'e';
			continue;
		} else {
			close(iOK);
		}
		if (0 == i && 0 == (0111 & stCheck.st_mode)) {
			fprintf(stderr, "%s: %s: must be executable\n", progname, argv[i]);
			acTemp[0] = 'x';
		}
	}
	if ('\000' != acTemp[0]) {
		exit(EX_NOINPUT);
	}

	/* We use low level I/O tell stdio to turn loose of any pending I/O,
	 * after we pick a preload zero config for any special basenames.
	 */
	if ((char *)0 == pcZeroConfig ? (0 == strcmp("-", progname) || 0 == strcmp("stdin", progname)) : (0 == strcmp("-", pcZeroConfig))) {
		pcZeroConfig = "-";
	} else if ((char *)0 == pcZeroConfig && 0 != strcmp("hxmd", progname)) {
		snprintf(acTemp, MAXPATHLEN, "%s.cf", progname);
		pcZeroConfig = strdup(acTemp);
	}
	fflush(stdout);
	fflush(stderr);

	if ((char *)0 == mkdtemp(acMySpace)) {
		fprintf(stderr, "%s: mkdtemp: %s: %s\n", progname, acMySpace, strerror(errno));
		exit(EX_CANTCREAT);
	}
	acTemp[0] = '-';
	acTemp[1] = '\000';
	pcEnd = acTemp+1;
	if (1 != uParallel && (char *)0 == pcXclH && (char *)0 == pcXclT) {
		*pcEnd++ = 's';
	}
	if (fExec) {
		*pcEnd++ = 'n';
	}
	if (fTrace) {
		*pcEnd++ = 'x';
	}
	if (fAppend) {
		*pcEnd++ = 'A';
	}
	*pcEnd++ = 'P';
	*pcEnd = '\000';
	snprintf(pcEnd, sizeof(acTemp)-strlen(acTemp), "%u", uParallel);
	AddToArgs(& PPMXapply, '\000', strdup(acTemp));
	if ((char *)0 != pcEscape) {
		AddToArgs(& PPMXapply, 'a', pcEscape);
	}
	if ((char *)0 != pcXInput) {
		AddToArgs(& PPMXapply, 'i', pcXInput);
	}
	if ((char *)0 != pcReq) {
		AddToArgs(& PPMXapply, 'R', pcReq);
	}
	if ((char *)0 != pcTasks) {
		AddToArgs(& PPMXapply, 'J', pcTasks);
	}
	if ((char *)0 != pcThreads) {
		AddToArgs(& PPMXapply, 't', pcThreads);
	}
	if ((char **)0 != (ppc = util_ppm_size(pPPMXapplyEnv, 0))) {
		while ((char *)0 != *ppc) {
			AddToArgs(& PPMXapply, 'e', *ppc++);
		}
	}
	if ((char *)0 != pcExShell) {
		AddToArgs(& PPMXapply, 'S', pcExShell);
	}
	if ((char *)0 != pcExElse) {
		AddToArgs(& PPMXapply, 'N', pcExElse);
	}
	if (argc > 1) {
		snprintf(acTemp, sizeof(acTemp), "-%u", argc);
		AddToArgs(& PPMXapply, '\000', strdup(acTemp));
	}
	/* A pcXCmd of the empty string ("") is going to trigger apply's
	 * lame catenation of the args list, I hope the first file is
	 * a script that uses the other files as useful data.		(ksb)
	 */
	AddToArgs(& PPMXapply, '\000', pcXCmd);
	for (i = 0; i < argc; ++i) {
		AddToArgs(& PPMXapply, '\000', "-");
	}

	/* Pass xclate options
	 */
	uGaver |= uGaveQ || (char *)0 != pcKleaner;
	if (uGaver || (char *)0 != pcExElse) {
		AddToArgs(& PPMXclate, 'r', (char *)0);
	}
	if ((char *)0 != pcXclH) {
		AddToArgs(& PPMXclate, 'H', pcXclH);
	}
	if ((char *)0 != pcXclL) {
		AddToArgs(& PPMXclate, 'L', pcXclL);
	}
	/* No xclate title/hr/input means -s is safe and effective,
	 * -P1 obviates the optimization totally.
	 */
	if ((char *)0 != pcXclT) {
		AddToArgs(& PPMXclate, 'T', pcXclT);
	} else if ((char *)0 == pcXclH && (char *)0 == pcXInput && 1 != uParallel) {
		ppc = util_ppm_size(& PPMXclate, 4);
		ppc[1] = "-ms";	/* replacing "-m" from InitVectors */
	}
	if ((char *)0 != pcXclO) {
		AddToArgs(& PPMXclate, 'O', pcXclO);
	}
	if ((char *)0 != pcXclW) {
		AddToArgs(& PPMXclate, 'W', pcXclW);
	}

	/* Tell m4 about any debugging we want to activate
	 */
	if ((char *)0 == (pcM4Allows = getenv("HXMD_M4_DEBUG"))) {
		pcM4Allows = M4_DEBUG_MASK;
	}
	if ('\000' != 0[pcDebug] && '\000' != pcM4Allows[0]) {
		register char *pcCursor, *pcOK, *pcMem;

		pcOK = pcMem = malloc((strlen(pcM4Allows)|3)+5);
		*pcOK++ = '-';
		*pcOK++ = 'd';
		for (pcCursor = pcM4Allows; '\000' != *pcCursor; ++pcCursor) {
			if ((char *)0 == strchr(pcDebug, *pcCursor))
				continue;
			*pcOK++ = *pcCursor;
		}
		*pcOK = '\000';
		if ('\000' != pcMem[2]) {
			ppc[1] = pcMem;
			ppc[2] = (char *)0;
		}
	}

	/* We need at least 1 cache control recipe find the tempfile we're
	 * going to use in the Child (mktemp base "make")
	 */
	if ((char *)0 == (pcHostSlot = HostSlot(acMySpace))) {
		exit(EX_DATAERR);	/* not config files */
	}
	if ((char *)0 == (pcCacheCtl = FindCache("make", pcHostSlot))) {
		exit(EX_SOFTWARE);
	}

	/* The pipes we need for xclate
	 *   Child -[pipe]- xclate in aiXclatePipe
	 *  xclate -[pipe]- Parent in aiNotify
	 */
	if (-1 == pipe(aiXclatePipe) || -1 == pipe(aiNotify)) {
		fprintf(stderr, "%s: pipe: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	switch (wXclatePid = fork()) {
	case -1:	/* Who is surprised with all the forks we need */
		fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		/* We are not commited yet, so just exit */
		exit(EX_OSERR);
	case 0:		/* Child is the xclate process */
		close(aiNotify[0]);	/* We don't read notifications	*/
		close(aiXclatePipe[1]);	/* We don't write to ourself	*/
		/* Our stdin is the xclate pipe's read side
		 */
		if (0 != aiXclatePipe[0]) {
			dup2(aiXclatePipe[0], 0);
			close(aiXclatePipe[0]);
		}
		snprintf(acTemp, sizeof(acTemp), ">&%d", aiNotify[1]);
		AddToArgs(& PPMXclate, 'N', strdup(acTemp));

		RunXclate();
		exit(EX_UNAVAILABLE);
	default:
		close(aiNotify[1]);	/* We don't write notifictation */
		close(aiXclatePipe[0]);	/* We don't read task argvs	*/
		break;
	}

	/* Peer to peer structure include a mmaped shared memory pool to
	 * store the filenames for the temp files and a socketpair:
	 *   child -[pair]- parent in aiPeer
	 * Map in the shared space for the filename buffers after we
	 * fork the xclate (no need for execve to clean it up). Compute
	 * the number of buffers we need via HX_AHEAD().
	 */
	uPrep = HX_AHEAD(uParallel);
	uRecSz = HX_BYTES_PER_FILE*argc;
	iShared = uRecSz*uPrep;
	iPageSz = getpagesize();
	if (0 != (iShared % iPageSz)) {
		iShared = (1 + iShared / iPageSz) * iPageSz;
	}
	/* If we can't map in the shared space we loose
	 */
#if defined(MAP_ANONYMOUS)	/* HPUX */
#if !defined(MAP_ANON)
#define MAP_ANON	MAP_ANONYMOUS
#endif
	fdPages = -1;
#else
#if defined(MAP_ANON)
	fdPages = -1;
#else
#define MAP_ANON	0
	if (-1 == (fdPages = open("/dev/zero", O_RDWR, 0666))) {
		fprintf(stderr, "%s: open: /dev/zero: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
#endif
#endif
	if (MAP_FAILED == (pvShared = mmap((void *)0, iShared, PROT_READ|PROT_WRITE, MAP_ANON|MAP_NOSYNC|MAP_SHARED, fdPages, (off_t)0))) {
		fprintf(stderr, "%s: mmap anonymous: %lu: %s\n", progname, (unsigned long)iShared, strerror(errno));
		exit(EX_OSERR);
	}
	if (-1 == socketpair(AF_UNIX, SOCK_STREAM, 0, aiPeer)) {
		fprintf(stderr, "%s: socketpair: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}

	/* If we took the script from stdin we set it +x -- ksb
	 * fScriptDash == argv[0] == Hostslot.stdin, but we can't see that
	 */
	if (fScriptDash && -1 == chmod(argv[0], 0700)) {
		fprintf(stderr, "%s: chmod: %s +x: %s\n", progname, argv[0], strerror(errno));
		exit(EX_OSERR);
	}
	switch (wPeerPid = fork()) {
	case -1:
		/* Now I'm really sad.
		 */
		fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	case 0:		/* We are the peer to m4 process files		*/
		close(aiNotify[0]);	/* We don't read notification	*/
		close(aiPeer[1]);	/* We are not Hxmd		*/
		if (1 != aiXclatePipe[1]) {
			dup2(aiXclatePipe[1], 1);
			close(aiXclatePipe[1]);
		}
		ppc = util_ppm_size(& PPMM4, 0);
		pMHReRun = Child(aiPeer[0], pvShared, argv, ppc, piIsCache, pcCacheCtl);
		exit(ReDo(pMHReRun, pvShared, argv, ppc));
	default:
		close(aiXclatePipe[1]);	/* We don't talk to xclate	*/
		close(aiPeer[0]);	/* We are not the Child		*/
		break;
	}
	wElse = EX_OK;
	Hxmd(aiPeer[1], pvShared, aiNotify[0], & wElse);

	/* Cleanup, processes and tempfiles
	 */
	iExit = EX_OK;
	while (errno = 0, -1 != (wWait = wait(& wStatus)) || EINTR == errno) {
		if (wPeerPid == wWait)
			iExit = wStatus >> 8;
	}
	pcEnd = (char *)pvShared;
	for (i = 0; i < uPrep; ++i) {
		CleanSlot(pcEnd);
		pcEnd += uRecSz;
	}
	CleanSlot(pcHostSlot);
	for (u = uLeftLits+uRightLits; u > 0; /* below */) {
		snprintf(acTemp, sizeof(acTemp), "%s/cmd%u", acMySpace, --u);
		if ((char *)0 != strchr(pcDebug, 'R')) {
			fprintf(stderr, "%s: rm -f %s\n", progname, acTemp);
		}
		if (-1 == unlink(acTemp)) {
			fprintf(stderr, "%s: unlink: %s: %s\n", progname, acTemp, strerror(errno));
		}
	}
	if (0 == rmdir(acMySpace) && (char *)0 != strchr(pcDebug, 'R')) {
		fprintf(stderr, "%s: rmdir %s\n", progname, acMySpace);
	}
	exit(EX_OK == wElse ? iExit : wElse);
}
