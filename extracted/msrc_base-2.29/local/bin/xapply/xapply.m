#!mkcmd
# $Id: xapply.m,v 3.55 2010/05/03 14:42:12 ksb Exp $
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

from '<sys/types.h>'
from '<sys/param.h>'
from '<sys/wait.h>'
from '<fcntl.h>'
from '<errno.h>'
from '<ctype.h>'
from '<signal.h>'
from '<unistd.h>'
from '<stdlib.h>'
from '<string.h>'
from '<sysexits.h>'

from '"machine.h"'
from '"fdrights.h"'

require "std_help.m" "std_version.m" "std_control.m"
require "util_pipefit.m" "util_cache.m" "util_divconnect.m"
require "dicer.c"
require "../ptbw/ptbc.m"

basename "xapply" ""

augment action 'V' {
	user "Version();"
}
%i
static char rcsid[] =
	"$Id: xapply.m,v 3.55 2010/05/03 14:42:12 ksb Exp $";
extern char **environ;
extern int errno;
static void Tasks(int, unsigned int);
static const char acEmpty[] = "";
static char acXCL_D[] = "xcl_d";
static char acPtbwLink[] = "ptbw_link";
%%

boolean 'A' {
	named "fAddEnv"
	init "0"
	help "append the tokens as shell parameters"
}
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

unsigned 'P' {
	named "iParallel"
	init "1"
	help "number of tasks to run in parallel"
	param "[jobs]"
	noparam "if (%ypi == (%N = getenv(\"PARALLEL\")) || '\\000' == *%N) {%N = \"3\";}"
}
boolean 'm' {
	track named "fManage"
	help "use xclate to manage output streams"
}
boolean 'g' {
	named "fGtfw"
	after 'if (%n) {GtfwInit(getenv("gtfw_link"));}'
	help "use gtfw for unique sequential numbers"
}
boolean 's' {
	named "fSqueeze"
	help "pass the squeeze option down to all xclate output filters"
}
boolean 'd' {
	named "fPublish"
	init "1"
	help "keep any xclate usage out of the published chain"
}
boolean 'u' {
	named "fUniq"
	help "force the xclate xid to be the expansion of %%u"
}

char* 'S' {
	named "pcShell"
	init getenv "SHELL"
	before 'if ((char *)0 == %n) {%n = "%D/sh/pv";}'
	param "shell"
	help "the shell to run tasks under"
}

type "argv" 'e' {
	named "pPPMAddEnv"
	param "var=dicer"
	help "set var to the expansion of dicer for each process"
}

char* 'N' {
	named "pcElse"
	param "else"
	help "a command to run when no tasks were otherwise launched"
}
char* variable "pcCmd" {
	param "cmd"
	help "template command to control each task"
}
int named "cEoln" {
	init "'\\n'"
	help ""
}

boolean 'f' {
	named "fpIn" "pcIn"
	help "arguments are read indirectly one per line from files"
	char* 'p' {
		named "pcPad"
		init '""'
		param "pad"
		help "fill short files with this token"
	}
	action 'z' {
		update "cEoln = '\\000';"
		help "read find-print0 output as input files"
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
# for ptbw, we can handle -R ourself worst-case
char* 't' {
	track "fGavet"
	named "pcTags"
	param "tags"
	help "list of target resources for %%t, connect to (start) a ptbw"
}
unsigned 'R' {
	track "fGaveR"
	named "uReq"
	init '1'
	param "req"
	help "number of resource allocated to each task, start a ptbw"
}
integer 'J' {
	track "fGaveJ"
	named "iCopies"
	param "tokens"
	help "number of task instances expected, start a ptbw"
}
#end ptbw interface
after {
	named "PostProc"
	update "%n(argc, argv);"
	help ""
}

left "pcCmd" {
}

list {
	named "Apply"
	param "args"
	help "list of arguments to distribute to tasks"
}

boolean 'x' {
	named "fTrace"
	help "trace actions on stderr"
}

key "PATH" 1 init {
	"/usr/local/bin" "/usr/bin"
}
key "xclate" 1 init {
	"xclate"
}
key "ptbw" 1 init {
	"ptbw"
}
char* named "pcXclate" {
	before '%n = %K<xclate>?e1qv;'
}
char* named "pcPtbw" {
	before '%n = %K<ptbw>?e1qv;'
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

#if !defined(MAX_XID_LEN)
#define MAX_XID_LEN	64		/* xclate'd xid limit, or so	*/
#endif

/* These all are nice to know						(ksb)
 */
static void
Version()
{
	printf("%s:  %s\n", progname, DicerVersion);
	printf("%s: shell: %s\n", progname, pcShell);
	printf("%s: xclate as %s\n", progname, pcXclate);
	printf("%s: xclate xid length limit %d\n", progname, MAX_XID_LEN);
	printf("%s: xclate -d environment \"%s\"\n", progname, acXCL_D);
	printf("%s: ptbw as %s, protocol %.*s\n", progname, pcPtbw, (int)strlen(PTBacVersion)-1, PTBacVersion);
	printf("%s: ptbw link environment \"%s\"\n", progname, acPtbwLink);
}

/* Keep up with -t targets -- ksb
 */
typedef struct TEnode {
	char **ppctokens;	/* uReq of them */
	unsigned int *publock;	/* uReq of these too */
	pid_t wlock;
} TARGET_ENV;
static TARGET_ENV *pTEList = (TARGET_ENV *)0;
static unsigned uTargetCursor = 0, uTargetMax = 0;

/* Take a list of tokens and put them in the local bookkeeping format	(ksb)
 * thay can be sep'd with '\n' or '\000', either.
 */
static void
LocalPool(char *pcList, unsigned int uCount, unsigned *puList, unsigned int uBlob)
{
	register TARGET_ENV *pTE;
	register unsigned int i, uFill;
	register char *pcNl, **ppcMem;

	if (0 == uBlob) {
		uBlob = 1;
	}
	ppcMem = (char **)calloc(uCount, sizeof(char *));
	pTEList = (TARGET_ENV *)calloc(iParallel, sizeof(TARGET_ENV));
	if ((TARGET_ENV *)0 == pTEList) {
		fprintf(stderr, "%s: calloc: %u,%u: %s\n", progname, uCount, (unsigned)sizeof(TARGET_ENV), strerror(errno));
		exit(EX_OSERR);
	}
	pTE = pTEList;
	for (i = 0, uFill = uBlob; i < uCount; ++i, ++uFill) {
		if ((char *)0 != (pcNl = strchr(pcList, '\n'))) {
			*pcNl = '\000';
		}
		if (uBlob == uFill) {
			pTE->ppctokens = ppcMem;
			pTE->publock = ((unsigned *)0 == puList) ? (unsigned *)0 : puList+i;
			pTE->wlock = 0;
			++pTE;
			uFill = 0;
		}
		*ppcMem++ = pcList;
		pcList += strlen(pcList)+1;
	}
	uTargetMax = pTE-pTEList;
}

static char **ppcManage, *pcSlaveXid;
/* We need an xclate in the process tree above us.			(ksb)
 * when we don't have one, we become it and make xapply our command.
 */
static void
XCLOverlay(argc, argv)
int argc;
char **argv;
{
	register char *pcXEnv = (char *)0, *pcTail;
	register char **ppcOver;
	register int i;

	if (fPublish ? ((char *)0 != getenv("xcl_link")) : (char *)0 != (pcXEnv = getenv(acXCL_D))) {
		/* 8 ~= xclate [-u $xcl_d] [-s] xid (char *)0 pad pad
		 */
		ppcManage = (char **)calloc(8, sizeof(char *));
		if ((char **)0 == ppcManage || (char *)0 == (pcSlaveXid = (char *)malloc((MAX_XID_LEN|7)+1))) {
			fprintf(stderr, "%s: building xclate argv: no memory\n", progname);
			exit(EX_OSERR);
		}
		i = 0;
		ppcManage[i++] = pcXclate;
		if (!fPublish) {
			ppcManage[i++] = "-u";
			ppcManage[i++] = strdup(pcXEnv);
			(void)unsetenv(acXCL_D);
		}
		if (fSqueeze) {
			ppcManage[i++] = "-s";
		}
		ppcManage[i++] = strcpy(pcSlaveXid, "~");
		ppcManage[i++] = (char *)0;
		return;
	}
	/* xclate -m[d] [-s] -- argv + (char *)0  =>  4+roundup(argc)
	 */
	ppcOver = (char **)calloc((argc|3)+5, sizeof(char *));
	if ((char **)0 == ppcOver) {
		fprintf(stderr, "%s: calloc: %d: cannot allocate argv for %s\n", progname, argc+4, pcXclate);
		exit(EX_OSERR);
	}
	i = 0;
	if ((char *)0 != (pcTail = strrchr(pcXclate, '/'))) {
		++pcTail;
	} else {
		pcTail = pcXclate;
	}
	ppcOver[i++] = pcTail;
	ppcOver[i++] = fPublish ? "-m" : "-md";
	if (fSqueeze) {
		ppcOver[i++] = "-s";
	}
	ppcOver[i++] = "--";
	while (argc-- > 0) {
		ppcOver[i++] = *argv++;
	}
	ppcOver[i] = (char *)0;
	(void)execvp(ppcOver[0], ppcOver);
	(void)execve(pcXclate, ppcOver, environ);
	fprintf(stderr, "%s: execve: %s: %s\n", progname, pcXclate, strerror(errno));
	exit(EX_UNAVAILABLE);
}

static int sMaster = -1;
static char *pcDyna = (char *)0;

/* Get tokens from our ptb-wrapped					(ksb)
 * send "T", get the list, ask for <iParallel*uReq>G, then
 * back off by 1's to just <uReq>
 */
static char *	/*ARGUSED*/
DynaTokens(char *pcDirect, unsigned int *puMine, unsigned int **ppuOut)
{
	register char *pcRet;
	register unsigned int i, uTry;
	auto char acSource[MAXPATHLEN+4];

	pcRet = (char *)0;
	if ((unsigned int *)0 != puMine) {
		*puMine = 0;
	}
	if ((char *)0 == pcDirect) {
		register char *pcEnv;

		if ((char *)0 == (pcEnv = getenv(acPtbwLink))) {
			fprintf(stderr, "%s: $%s: no enclosing ptbw\n", progname, acPtbwLink);
			exit(EX_DATAERR);
		}
#if HAVE_SNPRINTF
		snprintf(acSource, sizeof(acSource), "ptbw_%s", pcEnv);
#else
		sprintf(acSource, "ptbw_%s", pcEnv);
#endif
		if ((char *)0 == (pcDirect = getenv(acSource))) {
			fprintf(stderr, "%s: $%s: not found in the environment\n", progname, acSource);
			exit(EX_PROTOCOL);
		}
	}
	if (-1 == (sMaster = PTBClientPort(pcDirect))) {
		fprintf(stderr, "%s: %s: no connection: %s\n", progname, pcDirect, strerror(errno));
		exit(EX_NOINPUT);
	}
	if (-1 != PTBReqSource(sMaster, acSource, sizeof(acSource), (char **)0)) {
		pcDyna = strdup(acSource);
	}

	uTry = 0;
	for (i = iParallel; i > 0; --i) {
		uTry = uReq * i;
		if (uTry > uOurCount)
			continue;
		if ((char *)0 != (pcRet = PTBReadTokenList(sMaster, uTry, ppuOut)))
			break;
	}
	if (0 == i) {
		fprintf(stderr, "%s: ptbw cannot provide %u tokens (only %u available)\n", progname, uReq, uOurCount);
		exit(EX_DATAERR);
	}
	if ((unsigned int *)0 != puMine) {
		*puMine = uTry;
	}
	return pcRet;
}

/* If we need a ptb-wrapper around it make it so			(ksb)
 * then use it to get the targets we need, before any kids get them.
 * If we don't need a wrapper just fix up the options and return with
 * the internal target list inplace.
 * "-t -"	-> get tokens from existing ptbw
 * "-t file"	-> start a new ptbw, replace with -t -
 * -J N		-> start a new ptbw, pass down
 * -R rep	-> passed to new ptbw, only as given
 */
static void
PTBOverlay(int argc, char **argv, unsigned int uJobs)
{
	static char acMySelf[] = "XAPPLY_WRAP";
	auto char **ppcPTBArgv;
	auto char acCvt[128];
	auto unsigned uMine, *puList;
	register char *pcList, *pcBuffer;
	register unsigned int i;
	register int iCopy;
	register pid_t wCvt;

	pcList = (char *)acEmpty;
	puList = (unsigned int *)0;
	uMine = 0;
	wCvt = -1;	/* assume we can't be pid -1 */
	if ((char *)0 != (pcBuffer = getenv(acMySelf)) && '\000' != *pcBuffer) {
		wCvt = atoi(pcBuffer);
		(void)unsetenv(acMySelf);
	}

	/* We are the child of a ptbw we started, or told to look at one
	 * we didn't start then look to it for tokens.  Else we don't really
	 * need the passible transaction buster widgit [:-)].
	 */
	if (wCvt == getppid()) {
		if (0 == uReq) {
			uMine = uJobs;
			pcList = PTBIota((unsigned) uJobs, (unsigned int *)0);
		} else {
			pcTags = "-";
			pcList = DynaTokens((char *)0, & uMine, & puList);
		}
	} else if (fGavet) {
		if (PTBIsSocket(pcTags)) {
			pcList = DynaTokens(pcTags, & uMine, & puList);
		} else if ('-' == pcTags[0] && '\000' == pcTags[1]) {
			pcList = DynaTokens((char *)0, & uMine, & puList);
		} else {
			/* wrap it up, dude */
		}
	} else if (fGaveJ || (fGaveR && 0 != uReq)) {
		/* wrap me up in a default token list */
	} else {
		uMine = uJobs;
		pcList = PTBIota((unsigned) uJobs, (unsigned int *)0);
	}
	if (acEmpty == pcList) {
		/* fall into wrapper below */
	} else if ((char *)0 != pcList) {
		LocalPool(pcList, uMine, puList, uReq);
		return;
	} else {
		fprintf(stderr, "%s: cannot create resource pool\n", progname);
		exit(EX_SOFTWARE);
	}

	/* We need to wrap ourself in a ptbw, build the command line:
	 *	ptwb -m[q] [-Jjobs] [-Rreq] [-t file] -- argv (char *)0
	 *	   1+   1+       1+      1+        2+ 1+ original-argc  +1
	 */
	if ((char **)0 == (ppcPTBArgv = calloc(((1+1+2+2+2+1+argc+1)|3)+1, sizeof(char *)))) {
		fprintf(stderr, "%s: calloc(%d,%u): %s\n", progname, (1+1+1+2+1)+argc, (unsigned)sizeof(char *), strerror(errno));
		exit(EX_OSERR);
	}
	i = 0;
	/* mkcmd now searches for this, but that means it must be installed
	 * first and our build path matters.  Since msrc now depends on
	 * ptbw it is important to all.
	 */
	if ((char *)0 != (pcBuffer = strrchr(pcPtbw, '/'))) {
		++pcBuffer;
	} else {
		pcBuffer = pcPtbw;
	}
	ppcPTBArgv[i++] = pcBuffer;
	if (fGaveJ) {
		ppcPTBArgv[i++] = "-m";
	} else {
		ppcPTBArgv[i++] = "-mq";
		iCopies = iParallel;
	}
	if (0 != iCopies) {
#if HAVE_SNPRINTF
		snprintf(acCvt, sizeof(acCvt), "-J%d", iCopies);
#else
		sprintf(acCvt, "-J%d", iCopies);
#endif
		ppcPTBArgv[i++] = strdup(acCvt);
	}
	if (fGaveR && 0 != uReq) {
#if HAVE_SNPRINTF
		snprintf(acCvt, sizeof(acCvt), "-R%d", uReq);
#else
		sprintf(acCvt, "-R%u", uReq);
#endif
		ppcPTBArgv[i++] = strdup(acCvt);
	}
	if (fGavet) {
		ppcPTBArgv[i++] = "-t";
		ppcPTBArgv[i++] = pcTags;
	}
	ppcPTBArgv[i++] = "--";
	for (iCopy = 0; iCopy < argc; ++iCopy) {
		ppcPTBArgv[i++] = argv[iCopy];
	}
	ppcPTBArgv[i] = (char *)0;
#if HAVE_SNPRINTF
	snprintf(acCvt, sizeof(acCvt), "%d", (int)getpid());
#else
	sprintf(acCvt, "%d", (int)getpid());
#endif
	setenv(acMySelf, acCvt, 1);
	fflush(stdout);
	execvp(ppcPTBArgv[0], ppcPTBArgv);
	execve(pcPtbw, ppcPTBArgv, environ);
	fprintf(stderr, "%s: execve: %s: %s\n", progname, ppcPTBArgv[0], strerror(errno));
	exit(EX_UNAVAILABLE);
	/*NOTREACHED*/
}

/* Setup the -e option data we'll need in Launch			(ksb)
 */
static char **ppcAddEnv, **ppcAddDice, **ppcAddValue;
static void
SetEnvDriver(char **ppcVec, unsigned int uParams)
{
	register int i, iMax;
	register unsigned int uModuli;
	register char *pcEq;

	ppcAddValue = ppcAddDice = ppcAddEnv = (char **)0;
	if ((char **)0 == ppcVec || (char *)0 == *ppcVec) {
		return;
	}

	for (iMax = 0; (char *)0 != ppcVec[iMax]; ++iMax) {
		/* count them */
	}
	iMax = (iMax|3)+1;
	ppcAddEnv = ppcVec;
	ppcAddDice = (char **)calloc(iMax, sizeof(char *));
	ppcAddValue = (char **)calloc(iMax, sizeof(char *));
	if ((char **)0 == ppcAddValue || (char **)0 == ppcAddDice) {
		fprintf(stderr, "%s: calloc: %d: %s\n", progname, iMax, strerror(errno));
		exit(EX_OSERR);
	}
	uModuli = 0;
	for (i = 0; (char *)0 != ppcVec[i]; ++i) {
		auto char acTemp[64];
		if ((char *)0 == (pcEq = strchr(ppcVec[i], '='))) {
			++uModuli;
			sprintf(acTemp, "%c%u", cEsc, uModuli);
			if (uModuli >= uParams)
				uModuli = 0;
			ppcAddDice[i] = strdup(acTemp);
			continue;
		}
		*pcEq++ = '\000';
		ppcAddDice[i] = pcEq;
		ppcAddValue[i] = (char *)0;
	}
	ppcAddValue[i] = ppcAddDice[i] = (char *)0;
}

/* Put the values in the enviroment from the -e options			(ksb)
 */
static void
DoExportOpts()
{
	register char **ppc;
	register int i;

	if ((char **)0 == (ppc = ppcAddEnv)) {
		return;
	}
	for (i = 0; (char *)0 != ppc[i]; ++i) {
		setenv(ppc[i], ppcAddValue[i], 1);
	}
}

static pid_t *piOwn =	/* pid table we own, last is _our_pid_		*/
	(pid_t *)0;

/* build a list of processes we own, of course the list			(ksb)
 * starts out with just us in it.  We build one extra slot on the end
 * to keep our pid (and in case we get 0 tasks calloc is not NULL).
 * Also setup the environment passdowns, and the %t list.
 */
static void
Tasks(int iMax, unsigned int uParams)
{
	register int i;

	SetEnvDriver(util_ppm_size(pPPMAddEnv, 0), uParams);
	piOwn = (pid_t *)calloc(iMax+1, sizeof(pid_t));
	if ((pid_t *)0 == piOwn) {
		fprintf(stderr, "%s: calloc: pid table[%d]: %s\n", progname, iMax, strerror(errno));
		exit(EX_OSERR);
	}
	piOwn[0] = getpid();
	for (i = 0; i < iMax; ++i)
		piOwn[i+1] = piOwn[i];
}

/* remove the pid from the table, return 1 if it's in there		(ksb)
 * if fDone we also free the %t token allocated to it in the master
 */
static int
Stop(pid_t iPid, int fDone)
{
	register int i, iTok;

	for (i = 0; i < iParallel; ++i) {
		if (piOwn[i] != iPid)
			continue;
		for (iTok = 0; iTok < uTargetMax; ++iTok) {
			if (iPid != pTEList[iTok].wlock)
				continue;
			pTEList[iTok].wlock = 0;
			if (fDone && -1 != sMaster && (unsigned int *)0 != pTEList[iTok].publock) {
				(void)PTBFreeTokenList(sMaster, uReq, pTEList[iTok].publock);
				pTEList[iTok].publock = (unsigned int *)0;
			}
			break;
		}
		piOwn[i] = piOwn[iParallel];
		return 1;
	}
	return 0;
}

/* Get a token from the pool, wait for one if we must			(ksb)
 */
static int
GetToken(TARGET_ENV **ppTEFound)
{
	register int i, iRet;
	register pid_t w;
	auto int wExit;

	iRet = 0;
	*ppTEFound = (TARGET_ENV *)0;
	while (0 != uTargetMax) {
		for (i = uTargetMax; 0 < i; --i, ++uTargetCursor) {
			if (uTargetMax <= uTargetCursor)
				uTargetCursor = 0;
			if (0 != pTEList[uTargetCursor].wlock)
				continue;
			*ppTEFound = & pTEList[uTargetCursor];
			++uTargetCursor;
			return iRet;
		}

		if (0 < (w = wait3(& wExit, 0, (struct rusage *)0))) {
			if (Stop(w, 0))
				++iRet;
		}
	}
	return 0;
}

/* add a process to our list						(ksb)
 */
static void
Start(iPid, pTEThis)
pid_t iPid;
TARGET_ENV *pTEThis;
{
	static int iWrap = 0;

	while (piOwn[iWrap] != piOwn[iParallel]) {
		if (++iWrap == iParallel)
			iWrap = 0;
	}
	piOwn[iWrap] = iPid;
	pTEThis->wlock = iPid;
}

/* Send gtfw a version string so it knows we chat a like protocol	(ksb)
 */
static int
GtfwConnHook(int s, void *pv)
{
	register char *pcEos;
	auto char acReply[256];
	static const char acGtfwProto[] = "0.1";

	if (sizeof(acGtfwProto) != write(s, acGtfwProto, sizeof(acGtfwProto))) {
		fprintf(stderr, "%s: write: %d: %s\n", progname, s, strerror(errno));
		exit(EX_OSERR);
	}
	acReply[0] = '\000';
	if (-1 == read(s, acReply, sizeof(acReply))) {
		fprintf(stderr, "%s: read: %d: %s\n", progname, s, strerror(errno));
		exit(EX_PROTOCOL);
	}
	if (acReply != (pcEos = strchr(acReply, '\n'))) {
		*pcEos = '\000';
		fprintf(stderr, "%s: gtfw: %s\n", progname, acReply);
		exit(EX_PROTOCOL);
	}
	return s;
}

static int sGtfw = -1;			/* fd to chat with gtfw */
static const char *pcIota = ".";	/* %u counter source */

/* Chat up our enclosing gtfw instance for %u tokens			(ksb)
 */
static int
GtfwInit(char *pcLink)
{
	register char *pcDirect;
	auto char acSource[2*MAXPATHLEN+4];	/* host:/domain-socket */
	auto void *(*pfpvKeep)(void *);
	auto int (*pfiKeep)(int, void *);
	auto char acNotVoid[4];
	static const char acFetch[] = "U";

	if ((char *)0 == pcLink) {
		fprintf(stderr, "%s: no enclosing gtfw instance\n", progname);
		exit(EX_NOINPUT);
	}
	snprintf(acSource, sizeof(acSource), "gtfw_%s", pcLink);
	if ((char *)0 == (pcDirect = getenv(acSource))) {
		fprintf(stderr, "%s: gtfw instance mising environment link \"$%s\"\n", progname, acSource);
		exit(EX_PROTOCOL);
	}
	/* gtfw won't love us unless we send a version string
	 */
	pfiKeep = divConnHook;
	divConnHook = GtfwConnHook;
	pfpvKeep = divNextLevel;
	divNextLevel = (void *(*)(void *))0;
	sGtfw = divConnect(pcDirect, RightsWrap, acNotVoid);
	divConnHook = pfiKeep;
	divNextLevel = pfpvKeep;

	/* Ask for the token source, which could be off-host, we could
	 * remove the FQDN of our host if that's the prefix, but we don't.
	 */
	if (sizeof(acFetch)-1 != write(sGtfw, acFetch, sizeof(acFetch)-1) || (1 > read(sGtfw, acSource, sizeof(acSource)))) {
		fprintf(stderr, "%s: lost gtfw diversion\n", progname);
		exit(EX_SOFTWARE);
	}
	if ((char *)0 == (pcIota = strdup(acSource))) {
		fprintf(stderr, "%s: strdup: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	return sGtfw;
}

/* Get a unique integer from an enclosing gtfw				(ksb)
 */
static unsigned
FromGtfw()
{
	register unsigned uRet;
	auto char *pcEos;
	auto char acUniq[256];	/* "%d\n" */
	static const char acFetch[] = "u";
	static int uSeq = 0;

	if (!fGtfw) {
		return uSeq++;
	}
	if (sizeof(acFetch)-1 != write(sGtfw, acFetch, sizeof(acFetch)-1) || (1 > read(sGtfw, acUniq, sizeof(acUniq)))) {
		fprintf(stderr, "%s: lost gtfw diversion\n", progname);
		exit(EX_OSERR);
	}
	uRet = strtoul(acUniq, &pcEos, 0);
	/*  ('\n' != *pcEos) */
	return uRet;
}

static unsigned uSeqUniq = 0;	/* used to number the commands (%u)	*/
static unsigned uSeqInit = 0;	/* used to find zero -N else clause	*/
static char **ppcArgs;

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
Launch(char *pcBuilt, char *pcXid, TARGET_ENV *pTECur)
{
	register int iPid, iD1, iD2, iTmp;
	register unsigned int u;
	register char *pcRun;
	auto int afdSync[2];
	auto char acNone[8];

	fflush(stdout);
	fflush(stderr);
	afdSync[0] = afdSync[1] = -1;
	if ((fVerbose || fTrace) && -1 == pipe(afdSync)) {
		fprintf(stderr, "%s: pipe: %s\n", progname, strerror(errno));
	}
	for (iD1 = iD2 = 1; iD1 < MAX_BACKOFF; iTmp = iD1, iD1 += iD2, iD2 = iTmp) {
		switch (iPid = fork()) {
		case -1:
			if (errno == EAGAIN || errno == ENOMEM) {
				sleep(iD1);
				continue;
			}
			fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
			exit(EX_OSERR);
		case 0:
			DoExportOpts();
			ppcArgs[2] = pcBuilt;
			if (fAddEnv) {
				for (iTmp = 0; (char *)0 != ppcArgs[iTmp]; ++iTmp) {
					/* find the end */
				}
				for (u = 0; u < uReq; ++u) {
					ppcArgs[iTmp++] = pTECur->ppctokens[u];
				}
				ppcArgs[iTmp] = (char *)0;
			}
			/* wait for our parent to close a pipe to tell
			 * us he output the user notification.
			 */
			if (fVerbose || fTrace) {
				close(afdSync[1]);
				(void)read(afdSync[0], acNone, sizeof(acNone));
				close(afdSync[0]);
			}
			pcRun = ppcArgs[0];
			/* -m needs xclate to be our child, not a child of init
			 */
			if (fManage) {
				if ((char *)0 == pcXid || '\000' == *pcXid) {
#if HAVE_SNPRINTF
					snprintf(pcSlaveXid, MAX_XID_LEN, "%u", uSeqUniq);
#else
					sprintf(pcSlaveXid, "%u", uSeqUniq);
#endif
				} else {
					(void)strncpy(pcSlaveXid, pcXid, MAX_XID_LEN);
				}
				iPid = PipeFit(pcRun, ppcArgs, environ, 0);
				pcRun = pcXclate;
				ppcArgs = ppcManage;
				/* we have room for -p now, and iPid XXX */
			}
			execvp(pcRun, ppcArgs);
			execve(pcRun, ppcArgs, environ);
			fprintf(stderr, "%s: execve: %s: %s\n", progname, pcRun, strerror(errno));
			exit(EX_OSERR);
		default:
			if (fVerbose)
				printf("%s\n", pcBuilt);
			if (fTrace)
				fprintf(stderr, "%s\n", pcBuilt);
			/* make sure the customer knows what we're doing before
			 * the child process begins to execute
			 */
			if (fVerbose || fTrace) {
				close(afdSync[0]);
				fflush(stdout);
				close(afdSync[1]);
			}
			return iPid;
		}
	}
	if (fVerbose || fTrace) {
		close(afdSync[0]);
		close(afdSync[1]);
	}
	return -1;
}

/* Compute %* from a list (any of %*, %f*, or %t*)			(ksb)
 * call with ((char **)0, 0) to free space
 */
static char *
CatAlloc(char **ppcList, unsigned int iMax)
{
	register unsigned int i, uNeed;
	register char *pcCat;
	static char *pcMine = (char *)0;
	static unsigned int uMyMax = 0;

	uNeed = 0;
	if ((char **)0 == ppcList) {
		if ((char *)0 != pcMine) {
			free((void *)pcMine);
			pcMine = (char *)0;
		}
		uMyMax = 0;
		return (char *)0;
	}
	if (0 == iMax) {
		static char *apc_[2] = { "", (char *)0 };

		ppcList = apc_;
		iMax = 1;
	}
	for (i = 0; i < iMax; ++i) {
		uNeed += strlen(ppcList[i])+1;
	}
	if (uMyMax < uNeed) {
		if ((char *)0 != pcMine) {
			free((void *)pcMine);
		}
		uNeed |= 31;
		uMyMax = ++uNeed;
		pcMine = (char *)malloc(uNeed);
	}
	if ((char *)0 != pcMine) {
		pcCat = pcMine;
		for (i = 0; i < iMax; ++i) {
			if (0 != i) {
				*pcCat++ = ' ';
			}
			(void)strcpy(pcCat, ppcList[i]);
			pcCat += strlen(ppcList[i]);
		}
	}
	return pcMine;
}

/* Put a backslah quote behind any character that might get		(ksb)
 * expanded by a ksh/sh inside double quotes. viz \ " $ `
 */
static void
ReQuote(char *pcScan, unsigned *puCall, unsigned int uMax)
{
	static char *pcMem = (char *)0;
	static unsigned uMem;
	register char *pcCursor;
	register unsigned uLen;
	register int i;

	uLen = ((*puCall)|63)+1;
	if ((char *)0 == pcMem) {
		uMem = uLen;
		pcMem = (char *)malloc(uLen);
	} else if (uMem < uLen) {
		uMem = uLen;
		pcMem = (char *)realloc((void *)pcMem, uLen*sizeof(char));
	}
	if ((char *)0 == pcMem) {
		fprintf(stderr, "%s: out of memory\n", progname);
		exit(EX_OSERR);
	}

	/* We're clear, copy the source to a safe buffer and quote it back.
	 */
	pcCursor = pcMem;
	(void)memcpy(pcCursor, pcScan, *puCall);
	pcCursor[*puCall] = '\000';
	for (uLen = 0; '\000' != (i = *pcCursor++); ) {
		switch (i) {
		case '\\': case '\"': case '`': case '$':
			pcScan[uLen++] = '\\';
			/* fallthrough */
		default:
			pcScan[uLen++] = i;
			break;
		}
	}
	pcScan[uLen] = '\000';
	*puCall = uLen;
}

/* Build the command from the input data.				(ksb)
 * We know there are enough slots in argv for us to read.
 * We need to construct the command in a buffer and return it,
 * the caller passes in a buffer that is at least NCARGS big.
 * If we never see a parameter expand cat them on the end,
 * apply does this, I think it is brain dead.
 */
static char *
AFmt(pcBuffer, pcTemplate, iPass, argv, ppcNamev, pTEThis, pfTail)
char *pcBuffer, *pcTemplate;
int iPass, *pfTail;
char **argv, **ppcNamev;
TARGET_ENV *pTEThis;
{
	register char *pcScan;
	register int i, cCache, iParam, fSubPart, fGroup, iLimit, fMixer, fDQuote;
	register char **ppcExp, *pcTerm, *pcTidy;
	static unsigned uMax = NCARGS;
	auto unsigned uCall, uHold;
	auto char acNumber[64];

	pcScan = pcBuffer;
	cCache = cEsc;
	fSubPart = 0;
	uHold = 0;
	pcTidy = (char *)0;
	while ('\000' != *pcTemplate) {
		if (cCache != (*pcScan++ = *pcTemplate++)) {
			++uHold;
			continue;
		}
		if (cCache == (i = *pcTemplate++)) {
			continue;
		}
		--pcScan;
		/* %q... quote for doubles, after expanded
		 * %+ shift, %1 called as a template on %2...%$, for hxmd
		 * we used %1, so cancle the tail spell (after we call)
		 */
		if (0 != (fDQuote = ('q' == i))) {
			i = *pcTemplate++;
		}
		if ('+' == i) {
			auto unsigned uStash;
			if (0 == iPass || (char *)0 == argv[0])
				break;
			uStash = uMax;
			uMax -= uHold;
			if ((char *)0 == AFmt(pcScan, argv[0], iPass-1, argv+1, ppcNamev+1, pTEThis, pfTail))
				return (char *)0;
			uMax = uStash;
			*pfTail = 0;
			uCall = strlen(pcScan);
			if (fDQuote) {
				ReQuote(pcScan, & uCall, uMax);
			}
			uHold += uCall;
			pcScan += uCall;
			continue;
		}
		/* %f... for the filename under -f
		 * %t... for the tag-file tokens
		 * %{15} to get to bigger numbers
		 * %[5,1] for subfields (see dicer.c)
		 * %<dicer,mixer> for the mixer character snippr
		 */
		if (0 != (fMixer = MIXER_RECURSE == i)) {
			i = *pcTemplate++;
		}
		if ('f' == i) {
			i = *pcTemplate++;
			if ('t' == i || (('{' == i || '[' == i) && 't' == *pcTemplate)) /*]}*/ {
				/* allow %ft %f[t] and %f{t} w/o -f */
			} else if ('u' == i || (('{' == i || '[' == i) && 'u' == *pcTemplate)) /*]}*/ {
				/* allow %fu %f[u] and %f{u} w/o -f */
			} else if ((char **)0 == ppcNamev) {
				fprintf(stderr, "%s: %cf used without -f\n", progname, cEsc);
				exit(EX_DATAERR);
			}
			ppcExp = ppcNamev;
			iLimit = iPass;
		} else if ('t' == i) {
			static int fBugNTWarn = 1;

			if ('\000' == (i = *pcTemplate++)) {
				fprintf(stderr, "%s: %ct: requires a specification (number, f, )\n", progname, cEsc);
				exit(EX_DATAERR);
			}
			if (fExec || isatty(1)) {
				/* OK -- if they went to the trouble to put us
				 * on a tty they know "what they are doing".
				 */
			} else if (fBugNTWarn) {
				fprintf(stderr, "%s: %ct used with -n, possible locking issues\n", progname, cEsc);
				fBugNTWarn = 0;
			}
			ppcExp = pTEThis->ppctokens;
			iLimit = uReq;
		} else {
			ppcExp = argv;
			iLimit = iPass;
		}
		fGroup = 0;
		if ((fSubPart = '[' == i) || '{' == i) {
			fGroup = 1;
			i = *pcTemplate++;
		}
		/* %u  a uniq number sequence 0, 1, 2, ...
		 * %t  a lock tag, token, thread name, or target from the list
		 * %0-9 a param
		 */
		if ('u' == i || 'U' == i) {
			if (ppcExp == argv) {
				sprintf(acNumber, "%u", uSeqUniq);
				pcTerm = acNumber;
			} else if ((char *)0 == (pcTidy = strdup(pcIota))) {
				fprintf(stderr, "%s: strdup: %s\n", progname, strerror(errno));
				exit(EX_OSERR);
			} else {
				pcTerm = pcTidy;
			}
		} else if ('t' == i || 'T' == i) {
			/* %f[t...] -- provide -t's filename (ksb)
			 */
			if ((char *)0 == (pcTerm = pcDyna)) {
				fprintf(stderr, "%s: %cft: no -t option presented, filename expansion failed\n", progname, cEsc);
				exit(EX_DATAERR);
			}
		} else if (isdigit(i)) {	/* %0, %1 .. %9 */
			if (argv == ppcExp) {
				*pfTail = 0;
			}
			iParam = i - '0';
			while (fGroup) {
				i = *pcTemplate++;
				if (!isdigit(i))
					break;
				iParam *= 10;
				iParam += i - '0';
			}
			if (0 == iParam) {
				pcTerm = (char *)acEmpty;
			} else if (iParam < 1 || iParam > iLimit) {
				fprintf(stderr, "%s: %d: escape references an out of range parameter (%s limit %d)\n", progname, iParam, (argv == ppcExp ? "argument" : pTEThis->ppctokens == ppcExp ? "token" : "file"), iLimit);
				return (char *)0;
			} else {
				pcTerm = ppcExp[iParam-1];
			}
			if (fGroup) {
				--pcTemplate;
			}
		} else if ('$' == i) {	/* last param -- implied by %[1.$] */
			pcTerm = ppcExp[iPass-1];
			if (argv == ppcExp) {
				*pfTail = 0;
			}
		} else if ('*' == i) {
			pcTerm = CatAlloc(ppcExp, iLimit);
			if (argv == ppcExp) {
				*pfTail = 0;
			}
		} else {
			fprintf(stderr, "%s: %c%1.1s: unknown escape sequence\n", progname, cEsc, pcTemplate-1);
			return (char *)0;
		}
		if (fSubPart) {
			uCall = uMax-uHold;
			pcTemplate = Dicer(pcScan, &uCall, pcTemplate, pcTerm);
		} else {
			uCall = strlen(pcTerm);
			if (uHold+uCall < uMax) {
				(void)strcpy(pcScan, pcTerm);
			}
		}
		if (fMixer) {
			uCall = uMax-uHold;
			if ((char *)0 == (pcTemplate = Mixer(pcScan, &uCall, pcTemplate, MIXER_END))) {
				fprintf(stderr, "%s: mixer %c<..%s: syntax error expression\n", progname, cEsc, pcTemplate);
				return (char *)0;
			}
		}
		if (fDQuote) {
			ReQuote(pcScan, & uCall, uMax);
		}
		if ((char *)0 != pcTidy) {
			free((void *)pcTidy);
			pcTidy = (char *)0;
		}
		/* We might have already smacked the stack, but we try
		 * to catch it here --ksb
		 */
		if (uHold+uCall >= uMax) {
			fprintf(stderr, "%s: expansion is too large for NCARGS (%u)\n", progname, NCARGS);
			return (char *)0;
		}
		pcScan += uCall;
		uHold += uCall;
		if (fSubPart) {
			fSubPart = 0;
		} else if (fGroup) {
			if ('}' != *pcTemplate) {
				fprintf(stderr, "%s: %c{..%s: missing close culry (})\n", progname, cEsc, pcTemplate);
				return (char *)0;
			}
			++pcTemplate;
		}
	}
	/* implicit "%1 %2 %3 ..."
	 */
	if (0 != *pfTail) {
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
		*pfTail = 0;
	}
	*pcScan = '\000';
	return pcBuffer;
}

/* format the command and any environment insertions,			(ksb)
 * N.B. side effect globals link us to Launch.
 */
static char *
DiceControl(pcBuffer, pcTemplate, iPass, argv, ppcNamev, pTEThis)
char *pcBuffer, *pcTemplate;
int iPass;
char **argv, **ppcNamev;
TARGET_ENV *pTEThis;
{
	register char **ppc, *pcRes;
	register int i;
	auto int fTail;
	auto char acTemp[NCARGS];

	fTail = 1;
	if ((char **)0 == (ppc = ppcAddEnv)) {
		return AFmt(pcBuffer, pcTemplate, iPass, argv, ppcNamev, pTEThis, &fTail);
	}
	for (i = 0; (char *)0 != ppc[i]; ++i) {
		if ((char *)0 == ppcAddValue[i])
			break;
		free((void *)ppcAddValue[i]);
		ppcAddValue[i] = (char *)0;
	}
	for (i = 0; (char *)0 != ppc[i]; ++i) {
		fTail = 0;
		pcRes = AFmt(acTemp, ppcAddDice[i], iPass, argv, ppcNamev, pTEThis, &fTail);
		if ((char *)0 == pcRes) {
			fprintf(stderr, "%s: $%s: failed to expand\n", progname, ppcAddEnv[i]);
			exit(EX_DATAERR);
		}
		ppcAddValue[i] = strdup(acTemp);
	}
	fTail = 1;
	return AFmt(pcBuffer, pcTemplate, iPass, argv, ppcNamev, pTEThis, &fTail);
}

/* Run any else command we need and dispose of any tokens held.		(ksb)
 * The "wait loop" seems silly, becasue we didn't start any processes,
 * but our same pid might have before we were execve'd.
 */
static void
FinishElse(char *pcSpace, int argc, char **argv)
{
	register int i, iExit;
	auto int wExit;

	iExit = EX_OK;
	if (uSeqInit == uSeqUniq && (char *)0 != pcElse) {
		register char *pcBuilt;
		auto char *pcNull;
		auto TARGET_ENV *pTEElse;

		(void)GetToken(& pTEElse);
		pcNull = (char *)0;
		pcBuilt = DiceControl(pcSpace, pcElse, argc, argv, (char **)0, pTEElse);
		if ((char *)0 == pcBuilt) {
			fprintf(stderr, "%s: -N: failed to expand \"%s\"\n", progname, pcElse);
			iExit = EX_DATAERR;
		} else if (!fExec) {
			if (fVerbose)
				printf("%s\n", pcBuilt);
			if (fTrace)
				fprintf(stderr, "%s\n", pcBuilt);
		} else {
			do {
				while (0 < (i = wait3(& wExit, WNOHANG, (struct rusage *)0))) {
					Stop(i, 1);
				}
			} while (-1 == (i = Launch(pcBuilt, fGtfw ? (char *)0 : "00", pTEElse)));
			Start(i, pTEElse);
		}
	}

	/* Release free tokens if we have a ptbw to chat up
	 */
	for (i = 0; -1 != sMaster && i < uTargetMax; ++i) {
		if (0 != pTEList[i].wlock || (unsigned int *)0 == pTEList[i].publock)
			continue;
		(void)PTBFreeTokenList(sMaster, uReq, pTEList[i].publock);
		pTEList[i].publock = (unsigned int *)0;
	}

	/* just wait for any out-standing tasks
	 */
	while (-1 != (i = wait((void *)0))) {
		Stop(i, 1);
	}
	exit(iExit);
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
	auto int wExit, iRedo;
	auto TARGET_ENV *pTECur;
	auto char acCmd[NCARGS+1];
	auto char acMyXid[MAX_XID_LEN];

	if (0 != (argc % iPass)) {
		fprintf(stderr, "%s: %d arguments is not a multiple of hunk size (%d)\n", progname, argc, iPass);
		exit(EX_DATAERR);
	}
	iRedo = argc;

	/* not very useful here, but we allow it
	 */
	if ((char *)0 != pcRedir) {
		close(0);
		if (0 != open(pcRedir, O_RDONLY, 0666)) {
			fprintf(stderr, "%s: open: %s: %s\n", progname, pcRedir, strerror(errno));
			exit(EX_NOINPUT);
		}
	}

	iAdvance = 0 == iPass ? 1 : iPass;
	uSeqInit = uSeqUniq = FromGtfw();
	for (iRunning = 0; 0 != argc; uSeqUniq = FromGtfw()) {
		iRunning -= GetToken(& pTECur);
		pcBuilt = DiceControl(acCmd, pcCmd, iPass, argv, (char **)0, pTECur);
		if ((char *)0 == pcBuilt) {
			break;
		}
		(void)strncpy(acMyXid, argv[0], sizeof(acMyXid));
		acMyXid[sizeof(acMyXid)-1] = '\000';
		argc -= iAdvance, argv += iAdvance;
		if (!fExec) {
			if (fVerbose)
				printf("%s\n", pcBuilt);
			if (fTrace)
				fprintf(stderr, "%s\n", pcBuilt);
			continue;
		}
		do {
			while (0 < (i = wait3(& wExit, (iRunning < iParallel) ? WNOHANG : 0, (struct rusage *)0))) {
				if (Stop(i, 0))
					--iRunning;
			}
		} while (-1 == (i = Launch(pcBuilt, fUniq ? (char *)0 : acMyXid, pTECur)));
		Start(i, pTECur);
		++iRunning;
	}
	FinishElse(acCmd, iRedo, argv-iRedo);
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
	auto int wExit, iRedo;
	auto TARGET_ENV *pTECur;
	auto char acMyFXid[MAX_XID_LEN];

	if (0 != (argc % iPass)) {
		fprintf(stderr, "%s: %d files: not divisable by hunk size (%d)\n", progname, argc, iPass);
		exit(EX_DATAERR);
	}
	iRedo = argc;
	if ((char **)0 == (ppcSlots = (char **)calloc(iPass+1, sizeof(char *)))) {
		fprintf(stderr, "%s: out of memory\n", progname);
		exit(EX_OSERR);
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
					exit(EX_OSERR);
				}
				if ((FILE *)0 == (fpMove = fdopen(fd, "r"))) {
					fprintf(stderr, "%s: fdopen: %d: %s\n", progname, fd, strerror(errno));
					exit(EX_OSERR);
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
		(void)fcntl(fd, F_SETFD, 1);
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
			exit(EX_NOINPUT);
		}
	}

	iAdvance = 0 == iPass ? 1 : iPass;
	uSeqInit = uSeqUniq = FromGtfw();
	for (iRunning = 0; argc > 0; ) {
		iRunning -= GetToken(& pTECur);
		c = iEofs = iMem = 0;
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
				if (cEoln == c) {
					acBufs[iMem++] = '\000';
					break;
				}
				acBufs[iMem++] = c;
			}
		}
		if (i < iAdvance || (EOF != c && cEoln != c)) {
			fprintf(stderr, "%s: line length is too big for execve\n", progname);
			exit(EX_OSERR);
		}
		/* out of data on all inputs advance to next input group
		 */
		if (iAdvance == iEofs) {
			argc -= iAdvance, argv += iAdvance, pfpArgv += iAdvance;
			continue;
		}
		pcBuilt = DiceControl(acCmd, pcCmd, iPass, ppcSlots, argv, pTECur);
		if ((char *)0 == pcBuilt) {
			break;
		}
		strncpy(acMyFXid, ppcSlots[0], sizeof(acMyFXid));
		acMyFXid[sizeof(acMyFXid)-1] = '\000';
		if (!fExec) {
			if (fVerbose)
				printf("%s\n", pcBuilt);
			if (fTrace)
				fprintf(stderr, "%s\n", pcBuilt);
			uSeqUniq = FromGtfw();
			continue;
		}
		do {
			while (0 < (i = wait3(& wExit, (iRunning < iParallel) ? WNOHANG : 0, (struct rusage *)0))) {
				if (Stop(i, 0))
					--iRunning;
			}
		} while (-1 == (i = Launch(pcBuilt, fUniq ? (char *)0 : acMyFXid, pTECur)));
		Start(i, pTECur);
		uSeqUniq = FromGtfw();
		++iRunning;
	}
	FinishElse(acBufs, iRedo, argv-iRedo);
}

/* Do all the post work, we might get here three times			(ksb)
 * fix -P, -J, -R if we can, bomb on a bad -count.  Overlay any
 * wrapper processors we must have.  Build the task bookkeeping.
 */
static void
PostProc(int argc, char **argv)
{
	register char *pcTail;
	register unsigned int uJobs;

	if ((char *)0 == pcShell || '\000' == *pcShell) {
		fprintf(stderr, "%s: no shell available\n", progname);
		exit(EX_UNAVAILABLE);
	}
	if ('\000' == cEsc) {
		fprintf(stderr, "%s: -a: the NUL character forbidden\n", progname);
		exit(EX_CONFIG);
	}
	/* Bloody csh counts are differently on different hosts: in the
	 * tuning we can use    csh -c 'echo $1' 0 1 2
	 * to see if $1 is the 0 or the 1, if we get 1 we need to pad
	 * our -A parameters with a "0" or "_".
	 */
	if ((char **)0 == (ppcArgs = calloc(5+uReq, sizeof(char *)))) {
		fprintf(stderr, "%s: calloc: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	ppcArgs[0] = pcShell;
	ppcArgs[1] = "-c";
	ppcArgs[2] = ":";
	ppcArgs[3] = (char *)0;
	ppcArgs[4] = (char *)0;
	if ((char *)0 == (pcTail = strrchr(pcShell, '/'))) {
		pcTail = pcShell;
	} else {
		++pcTail;
	}
	if ((char *)0 != strstr(pcTail, "perl")) {
		ppcArgs[1] = "-e";
		/* perl programmers expect $ARGV[0] to be the first arg */
	} else if ((char *)0 != strstr(pcTail, "tcsh")) {
		/* tcsh doesn't need the pad */
	} else if ((char *)0 != strstr(pcTail, "csh")) {
#if NEED_CSH_PAD
		ppcArgs[3] = "_";
#else
		/* csh is most likely a link to tsh */
#endif
	} else {
		/* the (1 true) shell needs the pad */
		ppcArgs[3] = "_";
	}

	if (0 >= iPass) {
		fprintf(stderr, "%s: count: %d: is not a possible value\n", progname, iPass);
		exit(EX_USAGE);
	}
	if (iParallel < 1) {
		iParallel = 1;
	}
#if defined(TUNE_MAX_PROC)
	if (TUME_MAX_PROC < iParallel) {
		fprintf(stderr, "%s: -P%u: too many parallel processes (TUNE_MAX_PROC)\n", progname, iParallel);
		exit(EX_USAGE);
	}
#endif
	if (0 == uReq || 0 == iCopies) {
		uJobs = iParallel;
	} else if (!fGaveJ || iCopies < 1) {
		uJobs = iParallel * uReq;
	} else {
		uJobs = iCopies * uReq;
	}
	if (fManage) {
		XCLOverlay(argc, argv);
	}
	PTBOverlay(argc, argv, uJobs);
	Tasks(iParallel, (unsigned int)argc);
}
