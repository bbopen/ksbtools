#!/usr/local/bin/mkcmd
# $Id: xclate.m,v 2.64 2010/01/08 14:29:12 ksb Exp $
# xclate works with xapply (-m) to collate output from many child
# processes into a coherent stream.
#
# We use the environment prefix "xcl_" for our environment variable.
#
# $Compile: ${mkcmd-mkcmd} xclate.m && ${cc-gcc} prog.c
from '<sys/types.h>'
from '<sys/wait.h>'
from '<stdlib.h>'
from '<unistd.h>'
from '<limits.h>'
from '<fcntl.h>'
from '<signal.h>'
from '<sys/socket.h>'
from '<sys/un.h>'
from '<sys/uio.h>'
from '<sys/stat.h>'
from '<sys/ioctl.h>'
from '<ctype.h>'
from '<time.h>'
from '<assert.h>'
from '<sysexits.h>'

from '"machine.h"'
from '"fdrights.h"'
from '"mkdtemp.h"'

basename "xclate" ""
getenv "XCLATE"

require "std_help.m" "std_version.m" "util_divstack.m" "util_divconnect.m"
require "util_errno.m" "util_sigret.m" "util_ppm.m" "util_tmp.m"
require "util_pipefit.m"

# Tell divClient our direct option is -u, not -t like every other wrapper
# (mkcmd 'ru' could be written as '<respect>u' just the same, see -E option).
key "divClient" 1 {
	'"xcl"' 'ru'
}

%i
static char rcsid[] =
	"$Id: xclate.m,v 2.64 2010/01/08 14:29:12 ksb Exp $";
static const char acProto[] =
	"0.8";		/* the protocol version chat		*/
#if HAVE_SYS_PIPE_H && ! defined(FREEBSD)
#include <sys/pipe.h>
#endif

#if !defined(PIPE_SIZE)
#define PIPE_SIZE	16384
#endif

#if !defined(MAX_XID_LEN)
#define MAX_XID_LEN	64
#endif

#if !defined(MAX_PID_LEN)
#define MAX_PID_LEN	32
#endif

/* Broken pump of the master stdin -> widows we used to do, -I obviated this
 * For everyones safety never enable this (it breaks sshw and sapply). -- ksb
 */
static char
	acMySpace[] = "xclXXXXXX",
	acDevNull[] = "/dev/null",
	acEnvCmd[] = "XCLATE";
static FILE
	*fpVerbose;		/* -v output hook, std_control.m */
%%

augment action 'V' {
	user "Version();"
}
augment boolean 'v' {
	key 2 { "fpVerbose" }
}
init 1 "if ((FILE *)0 == (fpVerbose = fdopen(dup(2), \"w\"))) {fpVerbose = stderr;}(void)fcntl(fileno(fpVerbose), F_SETFD, 1);setvbuf(fpVerbose, (char *)0, _IOLBF, (size_t)512);"

# These options are common to client and master diversions, if the client
# doesn't set on it takes the one given to the controlling diversion.
boolean 's' {
	named "fSqueeze"
	init "0"
	help "squeeze out header+footer from tasks with no other output"
}
char* 'H' {
	named "pcHorizRule"
	param "hr"
	help "add a horizontal rule to end each output stream"
}
char* 'T' {
	named "pcTitle"
	param "title"
	help "a title for each stream, replace %%x with xid, %%s with sequence"
}
boolean 'q' {
	named "fAllowQuick"
	init '0'
	help "allow quicker processing of empty clients"
}
# Allowed in both, but not really passed to clients.  Could act like a poison
# pill in that the first client to connect would terminate the diversion
boolean 'Q' {
	named "fTerminate"
	help "tell the enclosing persistent instance to finish"
}
# in master-mode we build it, in client mode we use it as the direct option
char* 'u' {
	param "unix"
	named "pcMaster"
	init "(char *)0"
	help "specify a forced unix domain end-point name"
}
boolean 'm' {
	once
	named "fMaster"
	help "manage output for descendant processes"
	# no -W, or -W -
	#	our widows to our parent xclate
	#	no parent xclate builds a last hunk, -T "widows $PID"
	# -W "|proc" our widows to a process
	# -W ">&fd" our widows to an open fd (by number)
	# -W file (or ">>file") our widows (appened) to a file
	char* 'W' {
		param "widow"
		named "pcWidows"
		help "redirect widowed output to this file, process, or fd"
	}
	char* 'O' {
		param "output"
		named "pcPriv"
		help "redirect collated output to this file, process, or fd"
	}
	char* 'N' {
		param "notify"
		named "pcNotify"
		help "message this file, process, or fd for each finished section (with each xid)"
	}
	boolean 'A' {
		named "fStartup"
		help "add startup banner to nofity stream, for sshw"
	}
	char* 'i' {
		named "pcForcedIn"
		track "fReplaceOrigIn"
		param "input"
		help "replace the original stdin for the utility process"
	}
	boolean 'r' {
		named "fExit2"
		help "report exit codes in the notify stream (as code,xid)"
	}
	list {
		named "Master"
		param "utility"
		help "a command to produce output"
	}
	boolean 'd' {
		named "fPublish"
		init "1"
		help "do not publish this level in the linked environment"
	}
	# This is hard to set and describe in shell, but here goes:	(ksb)
	#  (proc1 &  proc2 & KEEP=$! ; proc3 &  exec xclate -p $KEEP exampled)
	integer 'p' {
		hidden named "wElected"
		user "wInferior = (pid_t)%n;"
		init "0"
		param "pid"
		help "child process elected to represent our exit status"
	}
	exit {
		update "exit(EX_OK);"
	}
	exclude "EIDmwYS"
}
# client only
boolean 'w' {
	named "fWeeping"
	help "redirect our output to the widow section, not the collated section"
}
boolean 'E' {
	named "fAskStderr"
	help "use the stderr of the selected instance"
}
boolean 'I' {
	named "fAskStdin"
	help "use the stdin of the selected instance"
}
boolean 'D' {
	named "fAskPWD"
	help "use the current working directory of the selected instance"
}
boolean 'Y' {
	named "fNewTTY"
	help "change the controlling terminal to the new stdin, stdout, or stderr"
}
type "bytes" 'L' {
	named "uiLimit" "pcLimit"
	track "fGaveL"
	init '"64k"'
	param "cap"
	help "cap on memory based output buffer"
}
char* named "pcXid" {
	local param "xid"
	help "the identification for this stream, usually xapply's %%1"
}
left ["pcXid"] {
}
list {
	named "Stream"
	update "%n(pcXid, %#, %@);"
	param "client"
	help ""
	abort "exit(EX_OK);"
}
# we don't encourage people to use perl as the shell, like xapply did.
char* 'S' {
	hidden named "pcInterp"
	init getenv "SHELL"
	before 'if ((char *)0 == %n) {%n = "/bin/sh";}'
}

# I suppose we don't need to document the hack here.
boolean 'Z' {
	hidden
	after "if (%n) {WidowsWalk();}"
	help "trap for nested recursive calls under a widow process"
}
letter 'a' {
	hidden
	named "cEsc"
	init "'%'"
	help "change the escape character (from %qi)"
}

%c
/* Prevent the deadlock that version 2.8 had with widows		(ksb)
 */
static void
WidowsWalk()
{
	if (fMaster) {
		return;
	}
	fprintf(stderr, "%s: recursive execution from the widow process leads to deadlock, becoming a cat\n", progname);
	execlp("cat", "cat", "-", (char *)0);
	execlp("/bin/cat", "cat", "-", (char *)0);
	exit(EX_USAGE);
}

/* Find a safe place to make a /tmp (or $TMPDIR) directory		(ksb)
 */
static char *
LocalTmp(const char *pcBase)
{
	register char *pcMem;
	register int iLen;

	iLen = ((strlen(pcTmpdir)+1+strlen(pcBase)+2)|7)+1;
	if ((char *)0 == (pcMem = malloc(iLen))) {
		fprintf(stderr, "%s: malloc: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	snprintf(pcMem, iLen, "%s/%s/", pcTmpdir, pcBase);
	return pcMem;
}

struct _WXnode {	/* type for the pvBlob we expect	*/
	int fsawactive;
	const char *pcinfo;
	const char *pcdirect;
};
/* Show some version information for a socket in the stack		(ksb)
 */
static int
DiVersion(const char *pcId, const char *pcTag, int fActive, void *pvBlob)
{
	register int fdCheck;
	register struct _WXnode *pWX;
	register char *pcTail;
	auto struct stat stSock;
	auto char acVersion[1024];	/* realy about 6 chars */

	if ((const char *)0 == pcId) {
		pcId = "--";
	}

	/* When wrapw is playing tricks we see her socket as if it
	 * were a directory, don't paninc and nobody gets hurt.
	 */
	stSock.st_mode = 0;
	if ((char *)0 != (pcTail = strrchr(pcTag, '/'))) {
		*pcTail = '\000';
		(void)lstat(pcTag, & stSock);
		*pcTail = '/';
	}
	if ((const char *)0 == pcTag) {
		printf("%s: %2s: missing from our environment", progname, pcId);
	} else if (S_IFSOCK != (S_IFMT & stSock.st_mode) && -1 == lstat(pcTag, &stSock)) {
		printf("%s: %2s: stat: %s: %s", progname, pcId, pcTag, strerror(errno));
	} else if (!fVerbose) {
		printf("%s: %2s %s", progname, pcId, pcTag);
	} else if (-1 == (fdCheck = divConnect(pcTag, RightsWrap, (void *)0)) || 0 >= read(fdCheck, acVersion, sizeof(acVersion)-1)) {
		printf("%s: %2s %s: unresponsive", progname, pcId, pcTag);
	} else {
		close(fdCheck);
		printf("%s: %2s %s: version %s", progname, pcId, pcTag, acVersion);
	}
	if ((struct _WXnode *)0 != (pWX = pvBlob)) {
		if (fActive) {
			pWX->fsawactive = 1;
			printf("%s", pWX->pcinfo);
		}
		if ((const char *)0 != pcTag && (const char *)0 != pWX->pcdirect && 0 == strcmp(pWX->pcdirect, pcTag)) {
			pWX->pcdirect = (const char *)0;
			printf(" [-u]");
		}
	}
	printf("\n");
	if (fActive && !isdigit(*pcId)) {
		return 1;
	}
	return '0' == pcId[0] && '\000' == pcId[1];
}

/* output the standard ksb version details				(ksb)
*/
static void
Version()
{
	auto unsigned uOuter;
	auto struct _WXnode WXThis;

	(void)divNumber(&uOuter);
	divVersion(stdout);
	printf("%s: protocol version %s\n", progname, acProto);
	printf("%s: safe directory template: %s\n", progname, acMySpace);
	WXThis.pcinfo = " [target]";
	WXThis.fsawactive = 0;
	WXThis.pcdirect = pcMaster;
	if (0 == divSearch(DiVersion, (void *)&WXThis) && iDevDepth >= uOuter) {
		if ((const char *)0 != WXThis.pcdirect) {
			DiVersion("-u", WXThis.pcdirect, !WXThis.fsawactive, (void *)&WXThis);
		} else if (0 == iDevDepth) {
			printf("%s: no current diversions\n", progname);
		} else {
			printf("%s: depth -%d is too great for current stack of %d\n", progname, iDevDepth, uOuter);
		}
	} else if ((const char *)0 != WXThis.pcdirect) {
		DiVersion("-u", WXThis.pcdirect, !WXThis.fsawactive, (void *)&WXThis);
	} else if (!WXThis.fsawactive) {
		printf("%s: never saw the active diversion\n", progname);
	}
}

/* Make the server port for this server to listen on			(ksb)
 * stolen from ptyd
 */
static int
MakeServerPort(pcName)
char *pcName;
{
	register int sCtl, iListen, iRet;
	auto struct sockaddr_un run;

	if ((sCtl = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "%s: socket: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	run.sun_family = AF_UNIX;
	(void) strncpy(run.sun_path, pcName, sizeof(run.sun_path));
	if (-1 == bind(sCtl, (struct sockaddr *)&run, EMU_SOCKUN_LEN(pcName))) {
		return -1;
	}
	iRet = -1;
	for (iListen = 20*64; iListen > 5 && -1 == (iRet = listen(sCtl, iListen)) ; iListen >>= 1) {
		/* try less */
	}
	if (-1 == iRet) {
		fprintf(stderr, "%s: listen: %d: %s\n", progname, iListen, strerror(errno));
		exit(EX_OSERR);
	}
	return sCtl;
}

/* The sort for xid's is strange in that we look for a "number" in	(ksb)
 * the string to compare, as well as a lexical key.
 *	app10 <=> app2 (return -1)
 *	100 <=> 101 (return 1)
 *	10a <=> 10b (return 1)
 *	foo <=> foo1 (return 1)
 * N.B. the strings cannot be (const char), sigh.
 */
static int
XidCmp(pcLeft, pcRight)
char *pcLeft, *pcRight;
{
	register char *pcLNum, *pcRNum;
	auto char *pcLRem, *pcRRem;
	register int iRet;

	for (pcLNum = pcLeft; '\000' != *pcLNum; ++pcLNum) {
		if (isdigit(*pcLNum))
			break;
	}
	for (pcRNum = pcRight; '\000' != *pcRNum; ++pcRNum) {
		if (isdigit(*pcRNum))
			break;
	}
	iRet = (pcRNum - pcRight) - (pcLNum - pcLeft);
	if (0 != iRet) {
		return iRet;
	}
	iRet = strncmp(pcLeft, pcRight, pcRNum-pcRight);
	if (0 != iRet) {
		return iRet;
	}
	pcLRem = pcRRem = "";
	if (isdigit(*pcRNum)) {
		if (isdigit(*pcLNum)) {
			iRet = strtol(pcRight, &pcRRem, 10);
			iRet -= strtol(pcLeft, &pcLRem, 10);
		} else {
			return 1;
		}
	} else if (isdigit(*pcLNum)) {
		return -1;
	} else {
		iRet = 0;
	}
	if (0 != iRet) {
		return iRet;
	}
	return strcmp(pcLRem, pcRRem);
}

/* Repeatedly call write(2) to flush large data out a small pipe	(ksb)
 * most modern unix systems don't need this, except HPUX
 */
ssize_t
ReWrite(int fd, const char *pc, size_t nb)
{
	register ssize_t iRet, cc;

	iRet = 0;
	do {
		cc = write(fd, pc, nb);
		if (-1 == cc)
			return cc;
		if (0 == cc)
			break;
		pc += cc;
		iRet += cc;
		nb -= cc;
	} while (nb > 0);
	return iRet;
}

/* Let the mkcmd pipefit utility show our details better		(ksb)
 */
static pid_t
CredPipeFit(const char *pcCred, const char *pcInterp, char **ppcFilter, char **ppcEnviron, int fdPush)
{
	register pid_t wRet;
	register size_t iLen;
	auto char *pcSave;
	register void *pvMem;

	pcSave = progname;
	pvMem = (void *)0;
	if ((char *)0 != pcCred) {
		iLen = (3|(strlen(progname)+2+strlen(pcCred)))+1;
		if ((char *)0 == (progname = pvMem = malloc(iLen))) {
			progname = pcSave;
		} else {
			snprintf(progname, iLen, "%s: %s", pcSave, pcCred);
		}
	}
	wRet = PipeFit(pcInterp, ppcFilter, ppcEnviron, fdPush);
	progname = pcSave;
	if ((void *)0 != pvMem) {
		free(pvMem);
	}
	return wRet;
}
%%
require "util_fsearch.m"
%c
/* use mkcmd's path search function to find programs for PipeFit	(ksb)
 * unless the program is spec'd as "/full/path" or "./prog" or "../prog"
 */
static char *
FindBin(char *pcProg)
{
	static char *apcList[2];
	register char *pcRet;

	if ('/' == pcProg[0] || ('.' == pcProg[0] && ('/' == pcProg[1] || ('.' == pcProg[1] && '/' == pcProg[2])))) {
		return pcProg;
	}
	apcList[1] = (char *)0;
	if ((char *)0 == (apcList[0] = pcProg)) {
		apcList[0] = pcInterp;
	}
	if ((char *)0 == apcList[0]) {
		apcList[0] = "/bin/sh";
	}
	if ((char *)0 != (pcRet = util_fsearch(apcList, getenv("PATH")))) {
		return pcRet;
	}
	fprintf(stderr, "%s: %s: not found in PATH\n", progname, apcList[0]);
	exit(EX_NOINPUT);
}

/* A file descriptor information entry for my use			(ksb)
 * N.B. now a bit set
 */
#define ACT_DEAD	0x0100		/* free slot			*/
#define ACT_UP		0x0200		/* special in,out,accept,widow,err*/
#define ACT_XID		0x0400		/* open, no xid yet		*/
#define ACT_QUICK	0x0800		/* done -> quick buffer		*/
#define ACT_OPEN	0x0000		/* open, no command pending	*/
#define ACT_INPUT	0x0001		/* asking for stdin		*/
#define ACT_OUTPUT	0x0002		/* asking for stdout		*/
#define ACT_ERROR	0x0004		/* asking for stderr		*/
#define ACT_WIDOW	0x0008		/* asking for widows (anyone)	*/
#define ACT_EXEC	0x0010		/* wants to fork unmanaged kid	*/
#define ACT_PEND	0x0020		/* claims to have output	*/
#define ACT_BMAX	0x0040		/* claims a full output buffer	*/
#define ACT_DONE	0x0080		/* claims quick, ready to flush	*/
#define ACT_MASK_PRIO	(ACT_EXEC|ACT_PEND|ACT_BMAX|ACT_DONE)
typedef struct PLMnode {
	int factive;			/* fd status from ACT_ above	*/
	int icode;			/* exit code from client	*/
	char acxid[MAX_XID_LEN];	/* name of stream		*/
} PUMPER;

/* Find a client stream to output					(ksb)
 * (the King is dead, long live the King)
 * We'll sort the xid's of all the eq eligables, we'll take a finished
 * stream over an active, we'll take a data stream over a pending one.
 */
static int
Elect(pPLM, iCount)
PUMPER *pPLM;
int iCount;
{
	register int i, iRet, fActive;
	register char *pcFound;

	fActive = 0;
	pcFound = (char *)0;
	iRet = -1;
	for (i = 0; i < iCount; ++i, ++pPLM) {
		if (0 == (ACT_OUTPUT & pPLM->factive)) {
			continue;
		}
		if (0 == (ACT_MASK_PRIO & pPLM->factive)) {
			continue;
		}
		if ((char *)0 == pcFound) {
			/* found one */;
		} else if ((ACT_MASK_PRIO&fActive) < (ACT_MASK_PRIO&pPLM->factive)) {
			/* better stream status */;
		} else if (XidCmp(pcFound, pPLM->acxid) < 0) {
			/* better order */;
		} else {
			/* stick with the one you know */
			continue;
		}
		/* take the new one */
		iRet = i;
		fActive = pPLM->factive;
		pcFound = pPLM->acxid;
	}
	return 0 == fActive ? -1 : iRet;
}

static volatile pid_t wInferior = -1;
static volatile int wInfExits = EX_PROTOCOL;
static volatile int fInfOpts = WNOHANG|WUNTRACED;
#if defined(DEBUG)
static volatile int wPidCount = 0;	/* only a gdb aid */
#endif

/* When a signal for a child arrives we burry the dead.			(ksb)
 * If he is "our boy", then forget he's alive.
 * N.B. Don't set the signal action up until you've set wInferior!
 */
static void
Death(int _dummy)	/*ARGSUSED*/
{
	register pid_t wReap;
	auto int wStatus;

	while (0 < (wReap = wait3(& wStatus, fInfOpts, (struct rusage *)0))) {
		if (WIFSTOPPED(wStatus)) {
			(void)kill(wReap, SIGCONT);
			continue;
		}
		if (-1 == wInferior || wInferior == wReap) {
			if (wInferior == wReap)
				wInferior = 0;
#if defined(DEBUG)
			++wPidCount;
#endif
			wInfExits = WEXITSTATUS(wStatus);
		}
	}
}

/* send an unsigned interger to the peer				(ksb)
 */
static void
TossWord(int fd, size_t wWord, int cSep)
{
	auto char acCvt[64];

	snprintf(acCvt, sizeof(acCvt), "%ld%c", (long)wWord, cSep);
	ReWrite(fd, acCvt, strlen(acCvt));
}

/* decode the word we just sent in TossWord				(ksb)
 * or "." for no value.
 */
static int
CatchWord(int fd, size_t *pwFound)
{
	auto char *pcEnd, acDecode[64];
	register int i;

	for (i = 0; i < sizeof(acDecode); ++i) {
		if (1 != read(fd, acDecode+i, 1)) {
			fprintf(stderr, "%s: read: %d: %s\n", progname, fd, strerror(errno));
			exit(EX_PROTOCOL);
		}
		if (isdigit(acDecode[i]))
			continue;
		if ('.' == acDecode[i]) {
			return 0;
		}
		/* must be a separator like : or \n
		 */
		acDecode[i] = '\000';
		break;
	}
	if (sizeof(acDecode) == i) {
		fprintf(stderr, "%s: string length too long\n", progname);
		exit(EX_PROTOCOL);
	}
	pcEnd = (char *)0;
	*pwFound = strtol(acDecode, &pcEnd, 10);
	if ((char *)0 != pcEnd && '\000' != *pcEnd) {
		fprintf(stderr, "%s: %s: length conversion failed\n", progname, acDecode);
		exit(EX_PROTOCOL);
	}
	return 1;
}

/* send a (potentially long) string over the diversion socket		(ksb)
 */
static void
TossString(int fd, char *pcDatum)
{
	register size_t uLen;
	if ((char *)0 == pcDatum) {
		ReWrite(fd, ".", 1);
		return;
	}
	uLen = ((unsigned long)strlen(pcDatum))+1;
	TossWord(fd, uLen, ':');
	ReWrite(fd, pcDatum, uLen);
}

/* decode what TossString sent us
 */
static char *
CatchString(int fd)
{
	auto char *pcRet;
	auto size_t wSize;

	if (0 == CatchWord(fd, &wSize)) {
		return (char *)0;
	}
	if ((char *)0 == (pcRet = malloc(wSize))) {
		fprintf(stderr, "%s: malloc: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	if (wSize == read(fd, pcRet, wSize)) {
		return pcRet;
	}
	fprintf(stderr, "%s: out of sync with peer\n", progname);
	exit(EX_SOFTWARE);
}

/* Tell any notification agent about this tasks exit status		(ksb)
 * (the iTty flag saves a lot of duplicate fcntl calls, that's all).
 */
static void
Notify(int fdTo, PUMPER *pPLM, int iTty)
{
	register int iLen;
	auto char acCvt[8];

	/* XXX do 'and' and 'or' options here from the todo
	 */
	if (-1 == fdTo) {
		return ;
	}
	iLen = strlen(pPLM->acxid);
	if (iTty) {
		pPLM->acxid[iLen] = '\n';
	}
	if (fExit2) {
		snprintf(acCvt, sizeof(acCvt), "%d,", pPLM->icode);
		ReWrite(fdTo, acCvt, strlen(acCvt));
	}
	ReWrite(fdTo, pPLM->acxid, iLen+1);
}

/* A client connects to our socket and give us an xid and some status	(ksb)
 * We send him back someplace to put his data, and a sequence number.
 * We copies his stream there, then closes the data channel, and the
 * control connection to tell us he's done.
 * We can notify an unrelated process of the termination of a client,
 * for hxmd's use mostly.
 *	while we still have an open connection (widow or client)
 *		select on the fd's we have active
 *		close any done clients (or the widow stream)
 *		look for new clients
 *			add control connection
 *		read commands from clients
 *			update status
 *		as needed elect a new output stream, input, and stderr
 */
static void
PumpLikeMad(fdSource, fdAccept, fdOut, fdWidows, fdNotify, fColonMode)
int fdSource, fdAccept, fdOut, fdWidows, fdNotify, fColonMode;
{
	register int cc;
	auto PUMPER aPLM[FD_SETSIZE];
	auto int iLeaseOut, iServing, iMaxPoll;
	auto char *pcQuick;
	auto fd_set fdsTemp, fdsReading;
	auto char acCounter[(MAX_XID_LEN|31)+1];
	auto int iConnections, iTty, fdSend, fdDrain, fdQuick;
	auto unsigned int uiCapture;
	auto struct sigaction saWant;
	auto struct timeval stStall;

	/* Build a (huge) list of clients to buffer.  We know that we
	 * need 2 fd's for each client, so allocated buffers to max_fd/2
	 * clients only (we know fdAccept takes 1 pair off as well).
	 */
	for (cc = 0; cc < FD_SETSIZE; ++cc) {
		aPLM[cc].factive = ACT_DEAD;
	}
	fdDrain = 0;
	aPLM[fdDrain].factive = ACT_UP;
	(void)strcpy(aPLM[0].acxid, "drain");
	aPLM[2].factive = ACT_UP;
	(void)strcpy(aPLM[2].acxid, "err");
	aPLM[fdWidows].factive = ACT_UP;
	(void)strcpy(aPLM[fdWidows].acxid, "widows");
	aPLM[fdOut].factive = ACT_UP;
	(void)strcpy(aPLM[fdOut].acxid, "out");
	aPLM[fdAccept].factive = ACT_UP;
	(void)strcpy(aPLM[fdAccept].acxid, "accept");
	if (-1 != fdNotify) {
		aPLM[fdNotify].factive = ACT_UP;
		(void)strcpy(aPLM[fdNotify].acxid, "notify");
		iTty = isatty(fdNotify);
	}
	if (-1 != fdSource) {
		aPLM[fdSource].factive = ACT_UP;
		(void)strcpy(aPLM[fdSource].acxid, "in");
	}
	(void)memset((void *)&saWant, '\000', sizeof(saWant));
	saWant.sa_handler = SIG_IGN;
	saWant.sa_flags = SA_RESTART;
	if (-1 == sigaction(SIGPIPE, &saWant, (struct sigaction *)0)) {
		fprintf(stderr, "%s: sigaction: restart: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	(void)memset((void *)& saWant, '\000', sizeof(saWant));
	saWant.sa_handler = (void *)Death;
#if HAVE_SIGACTION
	saWant.sa_sigaction = (void *)Death;
#endif
	saWant.sa_flags = SA_RESTART;
	if (-1 == sigaction(SIGCHLD, & saWant, (struct sigaction *)0)) {
		fprintf(stderr, "%s: sigaction: CHLD: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	Death('P');

	uiLimit |= 1023;
	++uiLimit;
	while (512 < uiLimit && (char *)0 == (pcQuick = malloc(uiLimit))) {
		uiLimit >>= 1;
	}
	if (512 > uiLimit) {
		fprintf(stderr, "%s: malloc: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	uiCapture = 0;
	fdQuick = iLeaseOut = -1;
	iMaxPoll = fdAccept;
	FD_ZERO(&fdsReading);
	FD_SET(fdAccept, &fdsReading);
	FD_SET(fdDrain, &fdsReading);
	iServing = 1;
	iConnections = 0;
	/* While we have a client or a process to manage we process events
	 * We used to race between the 0 != Inferior and the select setup, now
	 * we timeout ~17 times a minute to check again.  Cheap solution.
	 */
	stStall.tv_sec = 2;
	stStall.tv_usec = 178309;
	while (0 != iServing || 0 != wInferior) {
		register int iGot;

		fdsTemp = fdsReading;
		if (-1 == (iGot = select(iMaxPoll+1, &fdsTemp, (fd_set *)0, (fd_set *)0, 0 == iServing ? & stStall : (struct timeval *)0))) {
			if (EINTR == errno)
				continue;
			fprintf(stderr, "%s: select: %s\n", progname, strerror(errno));
			break;
		}
		if (0 == iGot) {
			stStall.tv_sec = 3;
			stStall.tv_usec = 524578;
			continue;
		}

		if (0 < iGot && FD_ISSET(fdAccept, &fdsTemp)) {
			register int fdClient;
			if (-1 == (fdClient = accept(fdAccept, (struct sockaddr *)0, (socklen_t *)0))) {
				if (EINTR == errno)
					continue;
				/* Someone broke our listening socket??
				 */
				FD_CLR(fdAccept, &fdsReading);
				close(fdAccept);
				continue;
			}
			aPLM[fdClient].factive = ACT_XID;
			aPLM[fdClient].icode = 0;
			(void)memset((void *)aPLM[fdClient].acxid, '\000', sizeof(aPLM[0].acxid));
			--iGot;
			FD_SET(fdClient, &fdsReading);
			if (fdClient > iMaxPoll) {
				iMaxPoll = fdClient;
			}
			++iServing;
			ReWrite(fdClient, acProto, sizeof(acProto));
			/* If we continue here we are a little more "fair"
			 */
			if (-1 == iLeaseOut) {
				continue;
			}
		}

		/* pump widows data if available
		 */
		if (-1 != fdDrain && FD_ISSET(fdDrain, &fdsTemp)) {
			register int iLen;
			auto char acW[PIPE_SIZE];

			iLen = read(fdDrain, acW, sizeof(acW));
			if (0 < iLen) {
				ReWrite(fdWidows, acW, iLen);
			} else {
				FD_CLR(fdDrain, &fdsReading);
				close(fdDrain);
				(void)open(acDevNull, O_RDONLY, 0666);
				aPLM[fdDrain].factive = ACT_DEAD;
				fdDrain = -1;
				--iServing;
				/* We close fdWidows in the caller, a client
				 * can still ask to write to that. --ksb
				 */
				aPLM[fdWidows].factive = ACT_DEAD;
			}
			--iGot;
		}

		/* Look for a client command and act on it.
		 * N.B. all commands are at least 4 chatacters on the socket
		 *	assert(sizeof(acCmd)>=MAX_XID_LEN&&MAX_XID_LEN>=4);
		 * the client must wait for the reply before sending cmds
		 */
		for (cc = 1; 0 != iGot && cc <= iMaxPoll; ++cc) {
			register int iLen;
			auto char acCmd[(MAX_XID_LEN|7)+1];

			if (!FD_ISSET(cc, &fdsTemp) || fdAccept == cc) {
				continue;
			}
			--iGot;
			if (0 != (ACT_QUICK & aPLM[cc].factive)) {
				iLen = read(cc, pcQuick+uiCapture, uiLimit-uiCapture);
				if (0 < iLen) {
					uiCapture += iLen;
					if (uiCapture == uiLimit) {
						FD_CLR(cc, &fdsReading);
					}
					continue;
				}
				goto drop;
			}
			/* In this version the client must send xid first
			 *  xid\n  ||  xid\r  ||  xid\000
			 * we send back a uniq sequence number
			 */
			if (ACT_XID == aPLM[cc].factive) {
				register int iCursor, c;
				register char *pcEos, *pcMax;

				iLen = read(cc, acCmd, MAX_XID_LEN+1);
				if (0 >= iLen) {
					goto drop;
				}
				pcEos = aPLM[cc].acxid;
				pcMax = pcEos+sizeof(aPLM[0].acxid)-1;
				pcEos += strlen(pcEos);
				for (iCursor = 0; iCursor < iLen; ++iCursor) {
					if ('\n' == (c = acCmd[iCursor]) || '\r' == c)
						c = '\000';
					if (pcEos < pcMax)
						*pcEos++ = c;
					if ('\000' == c) {
						aPLM[cc].factive = ACT_OPEN;
						snprintf(acCounter, sizeof(acCounter), "%d\n", iConnections++);
						ReWrite(cc, acCounter, strlen(acCounter));
						break;
					}
				}
				*pcEos = '\000';
				continue;
			}
			if (0 >= (iLen = read(cc, acCmd, 4))) {
				goto drop;
			}
			/* [0-9][0-9][0-9]\000 -- report exit code
			 */
			if (4 == iLen && isdigit(acCmd[0]) && isdigit(acCmd[1]) && isdigit(acCmd[2])) {
				aPLM[cc].icode = atoi(acCmd);
				continue;
			}
			/* cq \000	- channel abort (or just close socket)
			 */
			if (4 != iLen || ' ' != acCmd[2] || ('c' == acCmd[0] && 'q' == acCmd[1])) {
 drop:
				FD_CLR(cc, &fdsReading);
				close(cc);
				aPLM[cc].factive = ACT_DEAD;
				Notify(fdNotify, & aPLM[cc], iTty);
				--iServing;
				if (cc == fdQuick) {
					fdQuick = -1;
				}
				if (cc == iLeaseOut) {
					if (-1 != fdDrain && 0 != (aPLM[iLeaseOut].factive & ACT_INPUT))
						FD_SET(0, &fdsReading);
					iLeaseOut = -1;
				}
				continue;
			}
			/* -V \000	- show version of protocol
			 */
			if ('-' == acCmd[0] && 'V' == acCmd[1]) {
				(void)ReWrite(cc, acProto, sizeof(acProto));
				continue;
			}
			/* Q! \000	- channel wants us to exit when done
			 * (We'll exit after present clients finish.)
			 */
			if ('Q' == acCmd[0] && '!' == acCmd[1]) {
				iServing -= fColonMode;
				fColonMode = wInferior = 0;
				continue;
			}
			/* ex \000	- channel wants to execute
			 */
			if ('e' == acCmd[0] && 'x' == acCmd[1]) {
				if (0 == (ACT_DONE & aPLM[cc].factive))
					aPLM[cc].factive |= ACT_EXEC;
				continue;
			}
			/* hd \000	- channel has data (not complete)
			 */
			if ('h' == acCmd[0] && 'd' == acCmd[1]) {
				if (0 == (ACT_DONE & aPLM[cc].factive))
					aPLM[cc].factive |= ACT_PEND;
				continue;
			}
			/* bx \000	- channel buffer full
			 */
			if ('b' == acCmd[0] && 'x' == acCmd[1]) {
				if (0 == (ACT_DONE & aPLM[cc].factive))
					aPLM[cc].factive |= ACT_BMAX;
				continue;
			}
			/* rf \000	- channel complete (ready to flush)
			 */
			if ('r' == acCmd[0] && 'f' == acCmd[1]) {
				if (0 == (ACT_DONE & aPLM[cc].factive))
					aPLM[cc].factive |= ACT_BMAX;
				continue;
			}
			/* qm \000	- channel complete, quick mode
			 */
			if ('q' == acCmd[0] && 'm' == acCmd[1]) {
				aPLM[cc].factive |= ACT_DONE;
				continue;
			}
			/* od \000 open my current working directory, anyone
			 * can.  If I can't open it I'll return /.
			 */
			if ('o' == acCmd[0] && 'd' == acCmd[1]) {
				register char *pcDir = ".\n";

				if (-1 == (fdSend = open(".", O_RDONLY, 0))) {
					fdSend = open("/", O_RDONLY, 0);
					pcDir = "/\n";
				}
				if (-1 == RightsSendFd(cc, &fdSend, 1, pcDir)) {
					/* ouch, I don't know */
				}
				close(fdSend);
				continue;
			}

			/* o0 \000	- channel request for (widows|std)-in
			 * o1 \000	- channel request for stdout
			 * o2 \000	- channel request for stderr
			 * ow \000	- open widows out directly (no lock)
			 */
			if ('o' == acCmd[0] && '0' == acCmd[1]) {
				aPLM[cc].factive |= ACT_INPUT;
				continue;
			}
			if ('o' == acCmd[0] && '1' == acCmd[1]) {
				aPLM[cc].factive |= ACT_OUTPUT;
				continue;
			}
			if ('o' == acCmd[0] && '2' == acCmd[1]) {
				aPLM[cc].factive |= ACT_ERROR;
				continue;
			}
			if ('o' == acCmd[0] && 'w' == acCmd[1]) {
				if (-1 == RightsSendFd(cc, &fdWidows, 1, "widow\n")) {
					/* ouch, I don't know, maybe we can
					 * send /dev/null as consolation prize?
					 */
				}
				continue;
			}
			/* Return some options we might want client to know,
			 * all of these return a flag character and a '\000'.
			 */
			if ('?' == acCmd[0] && 'q' == acCmd[1]) {
				ReWrite(cc, fAllowQuick||fSqueeze ? "q" : "-", 2);
				continue;
			}
			if ('?' == acCmd[0] && 's' == acCmd[1]) {
				ReWrite(cc, fSqueeze ? "s" : "-", 2);
				continue;
			}
			if ('?' == acCmd[0] && 'd' == acCmd[1]) {
				ReWrite(cc, fPublish ? "d" : "-", 2);
				continue;
			}
			if ('?' == acCmd[0] && ':' == acCmd[1]) {
				ReWrite(cc, 0 == wInferior ? "Q" : fColonMode ? ":" : "-", 2);
				continue;
			}
			if ('?' == acCmd[0] && 'T' == acCmd[1]) {
				TossString(cc, pcTitle);
				continue;
			}
			if ('?' == acCmd[0] && 'H' == acCmd[1]) {
				TossString(cc, pcHorizRule);
				continue;
			}
			if ('?' == acCmd[0] && 'L' == acCmd[1]) {
				if (fGaveL)
					TossWord(cc, uiLimit, '\n');
				else
					ReWrite(cc, ".", 1);
				continue;
			}
			fprintf(stderr, "%s: %c%c: unknown command\n", progname, acCmd[0], acCmd[1]);
		}
		/* drop the quick data, if we have a break in the output
		 */
		if (-1 == iLeaseOut && 0 != uiCapture) {
			register int iLen;

			ReWrite(fdOut, pcQuick, uiCapture);
			uiCapture = 0;
			while (-1 != fdQuick) {
				iLen = read(fdQuick, pcQuick, uiLimit);
				if (0 <= iLen)
					break;
				ReWrite(fdOut, pcQuick, iLen);
			}
			if (-1 != fdQuick) {
				FD_CLR(fdQuick, &fdsReading);
				close(fdQuick);
				aPLM[fdQuick].factive = ACT_DEAD;
				Notify(fdNotify, & aPLM[fdQuick], iTty);
				--iServing;
				fdQuick = -1;
			}
		}
		/* When nobody is using the channel, find a worthy client.
		 * Tell the new shining one he's up, or we need another.
		 */
		while (-1 == iLeaseOut) {
			if (-1 == (iLeaseOut = Elect(aPLM, iMaxPoll+1))) {
				break;
			}
			/* We had to catch sigpipe here, the other end could
			 * have been killed before we serviced them.
			 */
			if (-1 != RightsSendFd(iLeaseOut, &fdOut, 1, "out")) {
				aPLM[iLeaseOut].factive	&= ~ACT_OUTPUT;
				break;
			}
			FD_CLR(iLeaseOut, &fdsReading);
			close(iLeaseOut);
			aPLM[iLeaseOut].factive = ACT_DEAD;
			--iServing;
		}
		/* Only the holder of stdout can get stdin/err (avoid deadlock).
		 */
		if (-1 == iLeaseOut) {
			continue;
		}
		if (0 != (aPLM[iLeaseOut].factive & ACT_ERROR)) {
			fdSend = 2;
			if (-1 != RightsSendFd(iLeaseOut, &fdSend, 1, "err\n"))
				aPLM[iLeaseOut].factive &= ~ACT_ERROR;
		}
		if (0 != (aPLM[iLeaseOut].factive & ACT_INPUT)) {
			fdSend = fdSource;
			if (-1 != RightsSendFd(iLeaseOut, &fdSend, 1, "in\n"))
				aPLM[iLeaseOut].factive &= ~ACT_INPUT;
		}
		/* If we have a client that's done, allocate the quick buffer
		 */
		if (fAllowQuick && -1 == fdQuick && uiCapture < uiLimit && -1 != (fdQuick = Elect(aPLM, iMaxPoll+1))) {
			fdSend = fdWidows;
			if (0 == (ACT_DONE & aPLM[fdQuick].factive) || -1 == RightsSendFd(fdQuick, &fdSend, 1, "quick\n")) {
				fdQuick = -1;
			} else {
				aPLM[fdQuick].factive &= ~ACT_MASK_PRIO;
				aPLM[fdQuick].factive |= ACT_QUICK;
			}
		}
	}
}

/* Install the option to prevent recursion under -W.			(ksb)
 * Build (and return) a new environ with the option added to acEnvCmd.
 */
char **
PreventRecursion(char **ppcEnv, char *pcOpt)
{
	register char **ppcRet, *pcOrig, **ppcFound;
	register int iNewLen, i;

	if ((char **)0 == ppcEnv) {
		auto char *apcNada[1];
		ppcEnv = apcNada;
		apcNada[0] = (char *)0;
	}
	for (i = 0; (char *)0 != ppcEnv[i]; ++i) {
		/* nada */
	}
	/* Allocate an extra slot incase we add XCLATE, and round up
	 * to a multiple of 4 for the memory allocator.  We never free
	 * this space, which is OK in xclate's work-flow. -- ksb
	 */
	iNewLen = ((i + 1)|3)+1;
	if ((char **)0 == (ppcRet = calloc(iNewLen, sizeof(char *)))) {
		return ppcEnv;
	}
	/* copy the environment, remember where $XCLATE lives
	 */
	ppcFound = (char **)0;
	for (i = 0; (char *)0 != ppcEnv[i]; ++i) {
		ppcRet[i] = ppcEnv[i];
		if (0 != strncmp(ppcRet[i], acEnvCmd, sizeof(acEnvCmd)-1) || '=' != ppcRet[i][sizeof(acEnvCmd)])
			continue;
		ppcFound = & ppcRet[i];
	}
	if ((char **)0 == ppcFound) {
		ppcFound = & ppcRet[i++];
		if (' ' == *pcOpt) {
			++pcOpt;	/* remove the whitespace	*/
		}
		pcOrig = "";
	} else {
		pcOrig = & ppcFound[0][sizeof(acEnvCmd)+1];
	}
	iNewLen = ((sizeof(acEnvCmd)+1+strlen(pcOrig)+strlen(pcOpt))|15)+1;
	if ((char *)0 == (ppcFound[0] = calloc(iNewLen, sizeof(char)))) {
		return ppcEnv;
	}
	snprintf(ppcFound[0], iNewLen, "%s=%s%s", acEnvCmd, pcOrig, pcOpt);
	ppcRet[i] = (char *)0;
	return ppcRet;
}

/* open a widow, or new output channel					(ksb)
 */
int
NewChannel(char *pcWhich, int fdDefault, char **ppcEnviron, const char *pcCredit)
{
	register int fdRet;
	auto char *pcEnd;
	auto char *apcFilter[8];
	auto struct sockaddr_un run;

	fdRet = fdDefault;
	if ((char *)0 == pcWhich || ('-' == pcWhich[0] && '\000' == pcWhich[1])) {
		return fdRet;
	}
	if ('|' == pcWhich[0]) {
		if ((char *)0 == (apcFilter[0] = strrchr(pcInterp, '/'))) {
			apcFilter[0] = pcInterp;
		} else {
			++(apcFilter[0]);
		}
		apcFilter[1] = 0 == strcmp(apcFilter[0], "perl") ? "-e" : "-c";
		if ('-' != pcWhich[1]) {
			apcFilter[2] = pcWhich+1;
			apcFilter[3] = (char *)0;
		} else {
			apcFilter[2] = "--";
			apcFilter[3] = pcWhich+1;
			apcFilter[4] = (char *)0;
		}
		CredPipeFit(pcCredit, pcInterp, apcFilter, ppcEnviron, fdRet);
		(void)fcntl(fdRet, F_SETFD, 1);
	} else if ('>' == pcWhich[0] && '&' == pcWhich[1]) {
		/* LLL we could recognize >&- to return -1 */
		pcEnd = (char *)0;
		fdRet = strtol(pcWhich+2, &pcEnd, 10);
		if ((char *)0 != pcEnd && '\000' != *pcEnd) {
			fprintf(stderr, "%s: unknown diversion expression `%s'?\n", progname, pcEnd);
			exit(EX_DATAERR);
		}
		if (2 < fdRet) {
			(void)fcntl(fdRet, F_SETFD, 1);
		}
		if (fVerbose && (const char *)0 != pcCredit) {
			fprintf(fpVerbose, "%s: %s: exec >&%d\n", progname, pcCredit, fdRet);
		}
	} else if ('>' == pcWhich[0] && '>' == pcWhich[1]) {
		pcWhich += 2;
		fdRet = open(pcWhich, O_WRONLY|O_CREAT|O_APPEND, 0666);
		if (-1 == fdRet) {
			fprintf(stderr, "%s: open: %s: %s\n", progname, pcWhich, strerror(errno));
			exit(EX_NOINPUT);
		}
		if (2 < fdRet) {
			(void)fcntl(fdRet, F_SETFD, 1);
		}
		if (fVerbose && (const char *)0 != pcCredit) {
			fprintf(fpVerbose, "%s: %s: exec %d>>%s\n", progname, pcCredit, fdRet, pcWhich);
		}
	} else {
		register int iOpenOpt;

		/* I wish we could set O_SHLOCK or O_EXLOCK here, at least
		 * we can connect to a socket, like dispd. -- ksb
		 */
		iOpenOpt = O_WRONLY|O_CREAT;
		if ('>' == pcWhich[0]) {
			iOpenOpt |= O_TRUNC;
			++pcWhich;
		}
		if (-1 == (fdRet = open(pcWhich, iOpenOpt, 0666))) {
			register int e;

			e = errno;
			run.sun_family = AF_UNIX;
			(void)strncpy(run.sun_path, pcWhich, sizeof(run.sun_path));
			if (-1 == (fdRet = socket(AF_UNIX, SOCK_STREAM, 0) || -1 == connect(fdRet, (struct sockaddr *)&run, EMU_SOCKUN_LEN(pcWhich)))) {
				if (ENOTSOCK == errno)
					fprintf(stderr, "%s: open: %s: %s\n", progname, pcWhich, strerror(e));
				else
					fprintf(stderr, "%s: connect: %s: %s\n", progname, pcWhich, strerror(errno));
				exit(EX_NOINPUT);
			}
			/* we don't chat anything, the utility must */
		}
		if (2 < fdRet) {
			(void)fcntl(fdRet, F_SETFD, 1);
		}
		if (fVerbose && (const char *)0 != pcCredit) {
			fprintf(fpVerbose, "%s: %s: exec %d>%s\n", progname, pcCredit, fdRet, pcWhich);
		}
	}
	if (-1 != fdRet) {
		return fdRet;
	}
	if (-1 == (fdRet = open(acDevNull, O_WRONLY, 0666))) {
		fprintf(stderr, "%s: open: %s: %s\n", progname, acDevNull, strerror(errno));
		exit(EX_NOINPUT);
	}
	if (2 < fdRet) {
		(void)fcntl(fdRet, F_SETFD, 1);
	}
	if (fVerbose && (const char *)0 != pcCredit) {
		fprintf(fpVerbose, "%s: %s: exec %d>%s\n", progname, pcCredit, fdRet, acDevNull);
	}
	return fdRet;
}

/* Like the above, but for the input express for stdin			(ksb)
 * Return non-zero for an OS error, or exit(EX_?).  Take '-' as /dev/null.
 */
static int
ResetInput(char *pcWhich, int fdDest, char **ppcEnviron, char *pcCredit)
{
	auto int fdTmp;
	auto char *pcEnd;
	auto char *apcFilter[8];
	auto struct sockaddr_un run;

	if ((char *)0 == pcWhich || '\000' == *pcWhich) {
		return 0;
	}
	if ('-' == pcWhich[0] && '\000' == pcWhich[1]) {
		pcWhich = acDevNull;
	}
	if ('|' == pcWhich[0]) {
		if ((char *)0 == (apcFilter[0] = strrchr(pcInterp, '/'))) {
			apcFilter[0] = pcInterp;
		} else {
			++(apcFilter[0]);
		}
		apcFilter[1] = 0 == strcmp(apcFilter[0], "perl") ? "-e" : "-c";
		if ('-' != pcWhich[1]) {
			apcFilter[2] = pcWhich+1;
			apcFilter[3] = (char *)0;
		} else {
			apcFilter[2] = "--";
			apcFilter[3] = pcWhich+1;
			apcFilter[4] = (char *)0;
		}
		if (-1 == CredPipeFit(pcCredit, pcInterp, apcFilter, ppcEnviron, 0)) {
			return 1;
		}
	} else if ('<' == pcWhich[0] && '&' == pcWhich[1]) {
		/* We recognize <&- for close stdin, why would you want to?
		 */
		if ('-' == pcWhich[2] && '\000' == pcWhich[3]) {
			close(0);
			return 0;
		}
		pcEnd = (char *)0;
		fdTmp = strtol(pcWhich+2, &pcEnd, 10);
		if ((char *)0 != pcEnd && '\000' != *pcEnd) {
			fprintf(stderr, "%s: unknown input expression `%s'?\n", progname, pcEnd);
			exit(EX_DATAERR);
		}
		if (fVerbose && (const char *)0 != pcCredit) {
			fprintf(fpVerbose, "%s: %s: exec 0<&%d\n", progname, pcCredit, fdTmp);
		}
		if (0 != fdTmp && -1 == dup2(fdTmp, 0)) {
			fprintf(stderr, "%s: dup2: %d, 0: %s\n", progname, fdTmp, strerror(errno));
			exit(EX_OSERR);
		}
	} else if ('<' == pcWhich[0] && '<' == pcWhich[1]) {
		fprintf(stderr, "%s: <<: here documents not supported (yet)\n", progname);
		exit(EX_UNAVAILABLE);
	} else if ('<' == pcWhich[0] && '>' == pcWhich[1]) {
		(void)close(0);
		if (0 != open(pcWhich+2, O_RDWR, 0666)) {
			fprintf(stderr, "%s: open: %s: %s\n", progname, pcWhich+2, strerror(errno));
			exit(EX_NOINPUT);
		}
	} else {
		if ('<' == *pcWhich)
			++pcWhich;
		(void)close(0);
		if (0 != open(pcWhich, O_RDONLY, 0666)) {
			register int e;

			e = errno;
			run.sun_family = AF_UNIX;
			(void)strncpy(run.sun_path, pcWhich, sizeof(run.sun_path));
			if (0 != socket(AF_UNIX, SOCK_STREAM, 0) || -1 == connect(0, (struct sockaddr *)&run, EMU_SOCKUN_LEN(pcWhich))) {
				if (ENOTSOCK == errno)
					fprintf(stderr, "%s: open: %s: %s\n", progname, pcWhich, strerror(e));
				else
					fprintf(stderr, "%s: connect: %s: %s\n", progname, pcWhich, strerror(errno));
				exit(EX_NOINPUT);
			}
			/* we don't chat anything, the utility must */
		}
	}
	return 0;
}

/* We are controlling an instance of xclate				(ksb)
 *	build a unix domain socket and bind to it
 *	set environment for kids (after we read it, as needed)
 *	setup widows fd (to our parent, by default)
 *	+ fork a child and execve her default "/bin/sh" {"-sh", (char *)0}
 *	PumpLikeMad
 *	wait for child (to cleanup zombie)
 *	exit with the above exit code
 */
static void
Master(argc, argv)
int argc;
char **argv;
{
	extern char **environ;
	register char *pcExec, *pc;
	register int i;
	auto unsigned uTop;
	auto char *apcDefShell[2], **ppcNoRecurse, *pcEncl;
	auto int fdListen, fdWidow, fdPriv, fdNotify, fdFeed, fdOrigIn;
	auto char acEnvName[512];
	auto char *pcWorkSpace, *pcSocket, *pcParent;

	/* Don't mess with my stdin, stdout, stderr.  If they are
	 * closed I'll just open /dev/null as I see fit.  Save the
	 * original stdin (or /dev/null) away for PumpLikeMad.
	 */
	if (-1 == fcntl(0, F_GETFD, 1) && 0 != open(acDevNull, O_RDONLY, 0666)) {
		fprintf(stderr, "%s: open: %s: %s\n", progname, acDevNull, strerror(errno));
		exit(EX_OSERR);
	}
	if (-1 == fcntl(1, F_GETFD, 1) && 1 != open(acDevNull, O_WRONLY, 0666)) {
		fprintf(stderr, "%s: open: %s: %s\n", progname, acDevNull, strerror(errno));
		exit(EX_OSERR);
	}
	if (-1 == fcntl(2, F_GETFD, 1) && 2 != open(acDevNull, O_WRONLY, 0666)) {
		fprintf(stderr, "%s: open: %s: %s\n", progname, acDevNull, strerror(errno));
		exit(EX_OSERR);
	}
	if (-1 == (fdOrigIn = dup(0))) {
		fprintf(stderr, "%s: dup: 0: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	(void)fcntl(fdOrigIn, F_SETFD, 1);

	if (argc == 0 || (char *)0 == (pcExec = argv[0])) {
		if ((char *)0 == (pcExec = pcInterp)) {
			pcExec = "/bin/sh";
		}
		if ((char *)0 != (pc = strrchr(pcExec, '/'))) {
			++pc;
		} else {
			pc = pcExec;
		}
		apcDefShell[0] = pc;
		apcDefShell[1] = (char *)0;
		argv = apcDefShell, argc = 1;
	}
	if (1 == argc && ':' == argv[0][0] && '\000' == argv[0][1]) {
		pcExec = (char *)0;
	} else {
		pcExec = FindBin(pcExec);
	}
	if (-1 == divNumber(& uTop )) {
		fprintf(stderr, "%s: divStack: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	++uTop;
	/* Allow xcl_0 to be set by the parent to force a wrapper home on us.
	 */
	pcParent = (0 == uTop) ? (char *)0 : divIndex(uTop-1);

	/* -O - open an enclosing diversion, find it before we publish ours
	 */
	pcEncl = (char *)0 != pcPriv && ('-' == pcPriv[0] && '\000' == pcPriv[1]) ? divSelect() : (char *)0;

	/* Build the socket if we have fixed name use that,
	 * else if we have a parent use his directory, or make our own.
	 */
	if ((char *)0 == pcMaster) {
		register int iScan;
		auto struct stat stDup;

		pcWorkSpace = (char *)0 != pcParent ? strdup(pcParent) : LocalTmp(acMySpace);
		if ((char *)0 == pcWorkSpace || (char *)0 == (pc = strrchr(pcWorkSpace, '/'))) {
			fprintf(stderr, "%s: workspace botch (%s)\n", progname, (char *)0 == pcWorkSpace ? "a NULL pointer" : "missing slash");
			exit(EX_SOFTWARE);
		}
		*pc = '\000';
		if ((char *)0 == pcParent) {
			mkdtemp(pcWorkSpace);
		}
		i = (7|strlen(pcWorkSpace))+MAX_XID_LEN+4+16;
		if ((char *)0 == (pcSocket = calloc(i, sizeof(char)))) {
			fprintf(stderr, "%s: calloc: %d*%u: %s\n", progname, i, (unsigned)sizeof(char), strerror(errno));
			exit(EX_TEMPFAIL);
		}

		/* We try to make a pattern that someone could follow,
		 * but the ice is too thin here to bet on any races -- ksb
		 */
		for (iScan = (fPublish ? uTop : 0); 1; ++iScan) {
			if (fPublish) {
				snprintf(pcSocket, i, "%s/%d", pcWorkSpace, iScan);
			} else {
				snprintf(pcSocket, i, "%s/%u_%d", pcWorkSpace, uTop, iScan);
			}
			if (-1 != stat(pcSocket, &stDup))
				continue;
			if (-1 != (fdListen = MakeServerPort(pcSocket)))
				break;
		}
	} else if (-1 == (fdListen = MakeServerPort(pcMaster))) {
		fprintf(stderr, "%s: bind: %s: %s\n", progname, pcMaster, strerror(errno));
		exit(EX_OSERR);
	} else {
		pcWorkSpace = (char *)0;
		pcSocket = pcMaster;
	}
	(void)fcntl(fdListen, F_SETFD, 1);
	if (fPublish) {
		i = divPush(pcSocket);
	} else {
		i = divDetach(pcSocket, (const char *)0);
	}
	if (0 != i) {
		fprintf(stderr, "%s: environment failure: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}

	/* If we can find XCLATE_$TOP in the environment rename it to XCLATE,
	 * else (we can't find a new one) at least unset the old one.
	 * Thus the xclate below us gets a different set of forced options.
	 */
	snprintf(acEnvName, sizeof(acEnvName), "%s_%u", acEnvCmd, uTop);
	if ((char *)0 != (pc = getenv(acEnvName))) {
		(void)setenv(acEnvCmd, pc, 1);
		if (fPublish) {
			(void)unsetenv(acEnvName);
		}
	} else if ((char *)0 != getenv(acEnvCmd)) {
		(void)unsetenv(acEnvCmd);
	}

	/* Accept the traffic from our child process(es)
	 */
	fdFeed = -1;

	/* We need to call NewChannel before the fork to set close-on-exec
	 * for any fd's the client process passed us, viz. -W ">&4"	(ksb)
	 */
	ppcNoRecurse = PreventRecursion(environ, " -Z");
	if ((char *)0 == pcNotify) {
		fdNotify = -1;
	} else if ('-' == pcNotify[0] && '\000' == pcNotify[1]) {
		fdNotify = open("/dev/tty", O_WRONLY, 0666);
		(void)fcntl(fdNotify, F_SETFD, 1);
	} else {
		register int fdTry;

		fdTry = dup(2);
		fdNotify = NewChannel(pcNotify, fdTry, ppcNoRecurse, "-N");
		if (fdNotify != fdTry)
			close(fdTry);
	}
	fdWidow = NewChannel(pcWidows, 2, ppcNoRecurse, "-W");
	if ((char *)0 == pcPriv) {
		fdPriv = 1;
	} else if ('-' == pcPriv[0] && '\000' == pcPriv[1]) {
		auto int iLen;

		/* -O - open an enclosing diversion, or fail, even support -Q
		 */
		if ((char *)0 == pcEncl) {
			fprintf(stderr, "%s: -O: -: no such diversion\n", progname);
			exit(EX_CONFIG);
		}
		if (-1 == (fdFeed = divConnect(pcEncl, RightsWrap, (void *)0)) || 1 > read(fdFeed, acEnvName, sizeof(acEnvName))) {
			fprintf(stderr, "%s: -O: %s: connect: %s\n", progname, pcEncl, strerror(errno));
			exit(EX_UNAVAILABLE);
		}
		(void)ReWrite(fdFeed, "o1 \000ex ", 8);
		iLen = sizeof(acEnvName);
		if (-1 == RightsRecvFd(fdFeed, &fdPriv, 1, acEnvName, &iLen)) {
			fprintf(stderr, "%s: -O: %s: failed to send rights\n", progname, pcEncl);
			exit(EX_TEMPFAIL);
		}
		if (fTerminate) {
			(void)ReWrite(fdFeed, "Q! ", 4);
		}
	} else {
		fdPriv = NewChannel(pcPriv, 1, environ, "-O");
	}
	/* Under (-r) and -v at least we carp here if there is no notify
	 * stream.  Might we should in any case, but for "be liberal".
	 */
	if (-1 == fdNotify && (fExit2 || fStartup)) {
		if (-1 == (fdNotify = open(acDevNull, O_WRONLY, 0666))) {
			fprintf(stderr, "%s: -N: open: %s: %s\n", progname, acDevNull, strerror(errno));
			exit(EX_USAGE);
		}
		(void)fcntl(fdNotify, F_SETFD, 1);
		if (fVerbose) {
			fprintf(fpVerbose, "%s: -r or -A set without -N, nofications sent to %s\n", progname, acDevNull);
		}
	} else if (fStartup) {
		i = strlen(pcSocket);
		if (isatty(fdNotify))
			pcSocket[i] = '\n';
		if (-1 == write(fdNotify, pcSocket, i+1))
			fprintf(stderr, "%s: write: %d: %s\n", progname, fdNotify, strerror(errno));
		pcSocket[i] = '\000';
	}
	if ((char *)0 == pcExec) {
		wInfExits = EX_OK;
		if (-1 == wInferior)
			wInferior = getpid();
		PumpLikeMad(fdOrigIn, fdListen, fdPriv, fdWidow, fdNotify, 1);
	} else if (fReplaceOrigIn && ResetInput(pcForcedIn, 0, environ, "-i")) {
		fprintf(stderr, "%s: -i: %s: %s\n", progname, pcForcedIn, strerror(errno));
		exit(EX_OSERR);
	} else if (-1 == (wInferior = CredPipeFit("cmd", pcExec, argv, environ, 0))) {
		exit(EX_OSERR);
	} else {
		PumpLikeMad(fdOrigIn, fdListen, fdPriv, fdWidow, fdNotify, 0);
	}
	if (-1 != fdNotify && -1 == close(fdNotify)) {
		fprintf(stderr, "%s: -N: close: %d: %s\n", progname, fdNotify, strerror(errno));
	}
	if (-1 == close(fdWidow)) {
		fprintf(stderr, "%s: -W close: %d: %s\n", progname, fdWidow, strerror(errno));
	}
	if (-1 == close(fdPriv)) {
		fprintf(stderr, "%s: -O: close: %d: %s\n", progname, fdPriv, strerror(errno));
	}
	if (-1 != fdFeed && -1 == close(fdFeed)) {
		fprintf(stderr, "%s: -O %s: close: %d: %s\n", progname, pcEncl, fdPriv, strerror(errno));
	}

	/* Cleanup, we must wait for all our kids, this does assume that any
	 * adopted children (fork'd before we execve'd) are really part of
	 * our same task.  That's not a big stretch, given my indiscretions.
	 */
	(void)unlink(pcSocket);
	fInfOpts = 0;		/* wait for all kids now */
	Death('M');
	if ((char *)0 == pcParent && (char *)0 != pcWorkSpace) {
		(void)rmdir(pcWorkSpace);
	}
	exit(wInfExits);
}

/* Output a nifty header above (rule below) the output stream		(ksb)
 * We could allow the command line passed to the client as %1, %2...
 * and %# for the count of arguments.  Then we'd have to add the mixer and
 * dicer and it would spin out of control.  Use xapply -e var=dicer to send
 * what you want in an environment variable, get that with %{var}.
 */
static int
Banner(fdOut, pcTemplate, pcWho, pcCount)
int fdOut;
char *pcTemplate, *pcWho, *pcCount;
{
	register char *pcMem, *pcEnv;
	register int iMax, i, iEscape, fNl, iRet;

	if ((char *)0 == pcTemplate || '\000' == *pcTemplate) {
		return 0;
	}
	if ((char *)0 == pcWho || '\000' == *pcWho) {
		pcWho = "?";
	}
	if ((char *)0 == pcCount || '\000' == *pcCount) {
		pcCount = "00";
	}

	/* The max expansion is max(len($Who), len($Count)) * number-of-%
	 * plus the lenght of the template string itself.
	 */
	i = strlen(pcWho);
	iMax = strlen(pcCount);
	if (iMax < i) {
		iMax = i;
	}
	for (iEscape = i = 0; '\000' != pcTemplate[i]; ++i) {
		if (cEsc == pcTemplate[i])
			++iEscape;
	}
	iMax *= iEscape;
	iMax += strlen(pcTemplate)+1; /* +1 for the '\n' we add */
	iMax |= 15;
	pcMem = calloc(iMax+1, sizeof(char));

	fNl = 1;
	iRet = 0;
	for (i = 0; '\000' != *pcTemplate; ++pcTemplate) {
		if (cEsc != (pcMem[i] = *pcTemplate)) {
			++i;
			continue;
		}
		if (cEsc == *++pcTemplate) {
			pcMem[i++] = cEsc;
			continue;
		}
		switch (*pcTemplate) {
		case 'x':	/* our xid */
			(void)strcpy(pcMem+i, pcWho);
			i += strlen(pcWho);
			continue;
		case 'u':	/* undocumented alias for %s		*/
		case 's':	/* our position in the sequence		*/
			(void)strcpy(pcMem+i, pcCount);
			i += strlen(pcCount);
			continue;
		case '{':	/*} %{ENV} adds $ENV to banner		*/
			iRet += ReWrite(fdOut, pcMem, i);
			i = 0; /*{*/
			while ('}' != pcTemplate[1] && '\000' != pcTemplate[1]) {
				pcMem[i++] = *++pcTemplate;
			} /*{*/
			if ('}' == pcTemplate[1]) {
				++pcTemplate;
			}
			pcMem[i] = '\000';
			if ((char *)0 == (pcEnv = getenv(pcMem))) {
				fprintf(stderr, "%s: %s: $%s: not in the environment\n", progname, pcWho, pcMem);
			} else {
				iRet += ReWrite(fdOut, pcEnv, strlen(pcEnv));
			}
			i = 0;
			continue;
		case '\000':	/* suppress trailing newline */
			fNl = 0;
			break;
		case '0':	/* "" (all expanders in xapply's chain do) */
			continue;
		default:
			++i;
			break;
		}
		break;
	}
	if (fNl) {
		pcMem[i++] = '\n';
	}
	return iRet + ReWrite(fdOut, pcMem, i);
}

/* Our stdin is a stream we want to send to a parent xclate's		(ksb)
 * managed stream.
 *	select a mode,  input-pump or no-pump
 *		input pump we buffer stdin for the process (1)
 *		no-pump we can't (2)
 *	find the master's divsrsion service and
 *	connect to the approriate xclate unix domain socket
 *	request a list of fd's from the master (honor -w, -E, -I)
 *	select for message from master or stdin
 *		read from stdin if we can and have buffer space
 *		read on the master fd to get our name, and new stdout
 *	pump buffered stdin to the locked stdout
 *	close the locked stdout
 *	report our child's exit code
 *	close the socket to the parrent to let the next stream go
 *	exit clean
 */
static void
Stream(pcWho, argc, argv)
char *pcWho;
int argc;
char **argv;
{
#define MODE_FILTER	1 /* inferior is pushed on stdin		 */
#define MODE_POPEN	2 /* push inferior on stdout pipe		 */
#define MODE_PROCESS	3 /* only parent the utility child process	 */
#define MODE_SQUEEZE	4 /* change to FITLER after req. fds aquired	 */
	extern char **environ;
	static char acUtility[] = "utility";
	register int cc;
	register char *pcBuffer, *pcNl;
	register unsigned int uiMax, uiHolding, uMode;
	auto char *pcUnix, *pcExec;
	auto size_t iNew;
	auto int fdFeed, fdMax;
	auto int fdNewIn, fdNewOut, fdNewErr, fdNewPWD;
	auto char acOurXid[MAX_XID_LEN+4];
	auto char acMessage[512 > MAX_XID_LEN+4 ? 512 : MAX_XID_LEN+4];
	auto char acServProt[32];	/* n.m\000 */
	auto PPM_BUF PPM;
	auto fd_set fdsTemp, fdsReading;
	auto pid_t wTemp;

	/* Make sure 0, 1, 2 are open :-(, don't mess with me, kid.
	 */
	while (-1 != (fdMax = open(acDevNull, O_RDWR, 0666))) {
		if (fdMax > 2)
			break;
	}
	if (-1 == fdMax) {
		fprintf(stderr, "%s: open: %s: %s\n", progname, acDevNull, strerror(errno));
		exit(EX_OSERR);
	}
	close(fdMax);

	pcExec = (char *)0;
	if (0 != argc && (char *)0 == (pcExec = FindBin(argv[0]))) {
		fprintf(stderr, "%s: %s: cannot locate in $PATH\n", progname, argv[0]);
		exit(EX_CONFIG);
	}

	uiHolding = 0;
	uiMax = PIPE_SIZE*2;
	(void)util_ppm_init(&PPM, sizeof(char), uiMax);
	if ((char *)0 == (pcUnix = pcMaster)) {
		pcUnix = divSelect();
	}
	if ((char *)0 == pcUnix) {
		if (0 != argc) {
			exit(EX_DATAERR);
		}
		pcBuffer = (char *)util_ppm_size(&PPM, uiMax);
		while (0 < (cc = read(0, pcBuffer, uiMax))) {
			ReWrite(1, pcBuffer, cc);
		}
		exit(EX_DATAERR);
	}

	if ((char *)0 == pcWho) {
		if ((char *)0 == (pcWho = calloc(MAX_PID_LEN, sizeof(char)))) {
			fprintf(stderr, "%s: calloc: %d: %s\n", progname, MAX_PID_LEN, strerror(errno));
			exit(EX_OSERR);
		}
		snprintf(pcWho, MAX_PID_LEN, "%ld", (long int)getpid());
	}
	if (MAX_XID_LEN <= strlen(pcWho)) {
		fprintf(stderr, "%s: xid: %s: too long, max length %d\n", progname, pcWho, MAX_XID_LEN-1);
		exit(EX_CANTCREAT);
	}
	if (-1 == (fdFeed = divConnect(pcUnix, RightsWrap, (void *)0))) {
		fprintf(stderr, "%s: %s: client: %s\n", progname, pcUnix, strerror(errno));
		exit(EX_NOINPUT);
	}
	if (1 > read(fdFeed, acServProt, sizeof(acServProt))) {
		fprintf(stderr, "%s: %s: dropped us\n", progname, pcUnix);
		exit(EX_NOINPUT);
	}
	if (0 != strcmp(acServProt, acProto) && fVerbose) {
		fprintf(fpVerbose, "%s: %s: protocol mismatched (local \"%s\", remote \"%s\")\n", progname, pcUnix, acProto, acServProt);
	}
	ReWrite(fdFeed, pcWho, strlen(pcWho)+1);
	if (0 >= read(fdFeed, acOurXid, sizeof(acOurXid))) {
		fprintf(stderr, "%s: %s: lost enclosing diversion (%s)\n", progname, pcUnix, strerror(errno));
		exit(EX_PROTOCOL);
	}
	if ((char *)0 != (pcNl = strchr(acOurXid, '\n'))) {
		*pcNl = '\000';
	}

	/* If we don't have -H, -T, or -L set ask the master diversion for them,
	 * but never overrule an explict empty string from our command-line.
	 */
	if ((char *)0 == pcTitle) {
		ReWrite(fdFeed, "?T ", 4);
		pcTitle = CatchString(fdFeed);
	}
	if ((char *)0 == pcHorizRule) {
		ReWrite(fdFeed, "?H ", 4);
		pcHorizRule = CatchString(fdFeed);
	}
	if (!fGaveL) {
		auto size_t wTmp;
		ReWrite(fdFeed, "?L ", 4);
		if (0 != CatchWord(fdFeed, &wTmp) && 0 < wTmp) {
			uiLimit = wTmp;
		}
	}
	/* If we don't have -s set, ask the master believe her if she does.
	 * Under -q & ~-s, set -s when no title or hr specified, as is common.
	 */
	if (!fSqueeze) {
		ReWrite(fdFeed, "?s ", 4);
		if (2 != read(fdFeed, acMessage, 2)) {
			fprintf(stderr, "%s: ?s: %s: short reply\n", progname, pcUnix);
			exit(EX_PROTOCOL);
		}
		fSqueeze = ('s' == acMessage[0]);
	}
	if (!fAllowQuick) {
		ReWrite(fdFeed, "?q ", 4);
		if (2 != read(fdFeed, acMessage, 2)) {
			fprintf(stderr, "%s: ?q: %s: short reply\n", progname, pcUnix);
			exit(EX_PROTOCOL);
		}
		fAllowQuick = ('q' == acMessage[0]);
	}
	if (fAllowQuick && !fSqueeze && ((char *)0 == pcHorizRule || '\000' == pcHorizRule[0]) && ((char *)0 == pcTitle || '\000' == pcTitle[0])) {
		fSqueeze = 1;
	} else {
		fAllowQuick = 0;
	}
	/* -s (SQUEEZE) forces a FILTER mode, as soon as we safely can {we
	 * may have to aquire a new stdin first, if we were to be a PROCESS}.
	 *
	 * With no utility:	utility | $progname [-sw] $xid
	 * tactic FILTER (in -> collated-output)
	 *	utility | $progname -> collated-output
	 *
	 * With -s:	source | $progname [-{I|E|D}][wY] $xid utility
	 * tactic SQUEEZE (locked-in -> child-utility -> collated-output)
	 *	utility | $progname -> collated-output
	 * with -IED we must wait for the lock to push the utility on stdin
	 *
	 * With no -IED:	source | $progname -{I|E|D}[wY] $xid utility
	 * tactic POPEN (in -> util -> collated-output)
	 *	* -> $progname | utility -> collated-output
	 *
	 * With -IED:		$progname $xid -{I|E|D}[wY] utility
	 * tactic PROCESS (aquire and fork)
	 *	$progname hold collated-output
	 *	locked-in -> child-utility -> collated-output
	 */
	if (0 == argc) {
		uMode = MODE_FILTER;
		if (fAskStdin || fAskStderr || fAskPWD) {
			fprintf(stderr, "%s: no %s given, turn off -I, -E, -D\n", progname, acUtility);
		}
		fAskStdin = fAskStderr = fAskPWD = 0;
	} else if (fSqueeze) {
		uMode = MODE_SQUEEZE;
	} else {
		uMode = fAskStdin ? MODE_PROCESS : MODE_POPEN;
	}

	/* negotiate for the rights we need
	 */
	if (fAskStdin) {
		ReWrite(fdFeed, "o0 ", 4);
	}
	if (fAskStderr) {
		ReWrite(fdFeed, "o2 ", 4);
	}
	if (fAskStderr || fAskStdin || !fWeeping) {
		ReWrite(fdFeed, "o1 ", 4);
	}
	if (fAskPWD) {
		auto int fdRecv;

		ReWrite(fdFeed, "od ", 4);
		iNew = sizeof(acMessage)-1;
		if (-1 == RightsRecvFd(fdFeed, &fdRecv, 1, acMessage, &iNew)) {
			exit(EX_OSERR);
		}
		fdNewPWD = fdRecv;
		(void)fcntl(fdNewPWD, F_SETFD, 1);
	} else {
		fdNewPWD = -1;
	}

	/* If we are in squeeze mode we might be able to _never_ block
	 * on the exclusive out (xapply does this a lot).
	 */
	FD_ZERO(& fdsReading);
	FD_SET(fdFeed, &fdsReading);
	if (MODE_SQUEEZE == uMode && !fAskStderr && !fAskStdin) {
		register int iGot;
		auto int fdBack = -1;

		if (fAskPWD) {
			if (-1 == (fdBack = open(".", O_RDONLY, 0))) {
				(void)fcntl(fdBack, F_SETFD, 1);
			}
			if (-1 == fchdir(fdNewPWD)) {
				fprintf(stderr, "%s: fchdir: %s\n", progname, strerror(errno));
				exit(EX_OSERR);
			}
			(void)close(fdNewPWD);
			fAskPWD = 0;
		}
		wInferior = CredPipeFit(acUtility, pcExec, argv, environ, 0);
		argc = 0;
		uMode = MODE_FILTER;
		if (-1 != fdBack) {
			(void)fchdir(fdBack);
			/* EACCESS is bad: we could have been started from
			 * a directory we can't get back to, but we keep on.
			 */
			(void)close(fdBack);
		}
		pcBuffer = (char *)util_ppm_size(&PPM, PIPE_SIZE);
		iGot = read(0, pcBuffer, PIPE_SIZE);
		/* If she exited w/o any output, just report her status,
		 * but wait for all our kids first.
		 */
		if (1 > iGot) {
			fInfOpts = 0;
			Death('S');
			goto squeezed;
		}
		uiHolding = iGot;
		(void)ReWrite(fdFeed, "hd ", 4);
		fSqueeze = 0;
	}
	switch (uMode) {
	case MODE_PROCESS:
	case MODE_SQUEEZE:
		(void)ReWrite(fdFeed, "ex ", 4);
		break;
	case MODE_FILTER:
	case MODE_POPEN:
		/* We'll the diversion more tell in the loop below
		 */
		if (uiHolding < uiLimit) {
			FD_SET(0, &fdsReading);
		}
		break;
	default:
		break;
	}
	fdMax = fdFeed;
	fdNewIn = fdNewErr = -1;
	for (fdNewOut = -1; (!fWeeping && -1 == fdNewOut) || (fAskStdin && -1 == fdNewIn) || (fAskStderr && -1 == fdNewErr); /*nada*/) {
		register int iGot;
		auto int fdRecv;

		fdsTemp = fdsReading;
		if (-1 == (iGot = select(fdMax+1, &fdsTemp, (fd_set *)0, (fd_set *)0, (struct timeval *)0))) {
			if (EINTR == errno)
				continue;
			fprintf(stderr, "%s: select: %s\n", progname, strerror(errno));
			break;
		}

		/* Unblock our data-source, if we can read from her. If she
		 * finishes first we might change things... if she never
		 * gives data and never closes we just wait forever.
		 */
		if (FD_ISSET(0, &fdsTemp)) {
			if (uiHolding+PIPE_SIZE > uiMax) {
				uiMax += PIPE_SIZE;
			}
			pcBuffer = (char *)util_ppm_size(&PPM, uiMax);
			iGot = read(0, pcBuffer+uiHolding, PIPE_SIZE);

			/* A filter ended with no output, don't get a lock,
			 * just fall out all the way to the wait.
			 */
			if (fSqueeze && MODE_FILTER == uMode && 0 >= iGot && 0 == uiHolding) {
				fWeeping = 0;
				fdNewPWD = fdNewOut = fdNewErr = -1;
				FD_CLR(0, &fdsReading);
				break;
			}
			/* A filter ended with some data, optimize if we can
			 * if we can find exit code go for the "quick" buffer
			 * in the parent process.
			 */
			if (0 >= iGot) {
				fInfOpts = 0;
				Death('?');
				if (0 == wInferior) {
					snprintf(acMessage, sizeof(acMessage), "%03d", (int)wInfExits);
					(void)ReWrite(fdFeed, acMessage, 4);
					(void)ReWrite(fdFeed, "qm ", 4);
				} else if (MODE_FILTER == uMode) {
					(void)ReWrite(fdFeed, "rf ", 4);
				} else if (MODE_POPEN == uMode) {
					(void)ReWrite(fdFeed, "ex ", 4);
				}
				FD_CLR(0, &fdsReading);
				if (MODE_FILTER == uMode) {
					fSqueeze = 0;
				}
				continue;
			}
			if (0 == uiHolding && MODE_FILTER == uMode) {
				fSqueeze = 0;
				(void)ReWrite(fdFeed, "hd ", 4);
			}
			uiHolding += iGot;
			/* When we fill the buffer, stop polling stdin
			 */
			if (uiHolding >= uiLimit) {
				FD_CLR(0, &fdsReading);
				(void)ReWrite(fdFeed, "bx ", 4);
			}
		}

		/* We have new mail, could be any of those we requested
		 */
		if (-1 == fdFeed || !FD_ISSET(fdFeed, &fdsTemp)) {
			continue;
		}
		iNew = sizeof(acMessage)-1;
		if (-1 == RightsRecvFd(fdFeed, &fdRecv, 1, acMessage, &iNew)) {
			/* We might recover -- output to our original
			 * stdout?  I think this is NOT good idea.
			 */
			exit(EX_PROTOCOL);
		}
		acMessage[sizeof(acMessage)-1] = '\000';
		if ((char *)0 != (pcNl = strchr(acMessage, '\n'))) {
			*pcNl = '\000';
		}
		/* When we want stderr, stdin, or widows we need to
		 * have more logic here;
		 *   /out/	collated stdout, our sequence # or
		 *   /in/	manager's stdin
		 *   /err/	manager's stderr
		 *   /quick/	send your output to the master
		 */
		if ('o' == acMessage[0]) {
			fdNewOut = fdRecv;
			continue;
		}
		if ('e' == acMessage[0]) {
			fdNewErr = fdRecv;
			continue;
		}
		if ('i' == acMessage[0]) {
			fdNewIn = fdRecv;
			continue;
		}
		if ('q' == acMessage[0]) {
			pcBuffer = (char *)util_ppm_size(&PPM, uiMax);
			Banner(fdFeed, pcTitle, pcWho, acOurXid);
			ReWrite(fdFeed, pcBuffer, uiHolding);
			Banner(fdFeed, pcHorizRule, pcWho, acOurXid);
			close(fdFeed);
			exit(EX_OK);
		}
		/* how did we get here?
		 */
		fprintf(stderr, "%s: %s: protocol error, v%s\n", progname, acMessage, acProto);
		exit(EX_PROTOCOL);
	}
	/* We don't need the collated stream, get the widows
	 */
	if (fWeeping) {
		if (-1 != fdNewOut) {
			close(fdNewOut);
			fdNewOut = -1;
		}
		ReWrite(fdFeed, "ow ", 4);
		iNew = sizeof(acMessage)-1;
		if (-1 == RightsRecvFd(fdFeed, &fdNewOut, 1, acMessage, &iNew)) {
			exit(EX_OSERR);
		}
		/* assert('w' != acMessage[0]); */
	}
	if (fTerminate) {
		(void)ReWrite(fdFeed, "Q! ", 4);
	}

	/* move the desciptors requests into place
	 */
	if (-1 != fdNewErr && 2 != fdNewErr) {
		fflush(stderr);
		close(2);
		dup(fdNewErr);
		close(fdNewErr);
		fdNewErr = -1;
	}
	if (-1 != fdNewOut && 1 != fdNewOut) {
		fflush(stdout);
		close(1);
		dup(fdNewOut);
		(void)fcntl(fdNewOut, F_SETFD, 1);
	}
	if (-1 != fdNewIn && 0 != fdNewIn) {
		close(0);
		dup(fdNewIn);
		close(fdNewIn);
		fdNewIn = -1;
	}
	if (-1 != fdNewPWD) {
		if (-1 == fchdir(fdNewPWD)) {
			fprintf(stderr, "%s: fchdir: %d: %s\n", progname, fdNewPWD, strerror(errno));
			exit(EX_OSERR);
		}
		(void)close(fdNewPWD);
		fdNewPWD = -1;
	}
	/* sshw wants this, I'm sure
	 */
	if (fNewTTY) {
		register int fdTty;
#if defined(TIOCNOTTY)
		register char *pcTty;
#endif

		fdTty = (-1 != fdNewIn && isatty(fdNewIn)) ? fdNewIn : (-1 != fdNewOut && isatty(fdNewOut)) ? fdNewOut : isatty(fdNewErr) ? fdNewErr : -1;
		if (-1 == fdTty) {
			/* nada, we don't have a new controlling tty */
		} else if (-1 == setsid()) {
			/* we can't get free of the old one, sigh */
#if defined(TIOCSCTTY)
		/* try the "new way" if we have it */
		} else if (-1 != ioctl(fdTty, TIOCSCTTY, (char *)0)) {
			/* we win, I guess */
#endif
#if defined(TIOCNOTTY)
		} else if ((char *)0 != (pcTty = ttyname(fdTty))) {
			register int i;

			if (-1 != (i = open("/dev/tty", O_RDWR, 0666))) {
				(void)ioctl(i, TIOCNOTTY, (char *)0);
				(void)close(i);
			}
			/* try to pick it up from an open, old school */
			(void)close(open(pcTty, O_RDWR, 0666));
#else
		} else {
			/* I don't know what else to try -- ksb */
#endif
		}
	}
	if (!fSqueeze) {
		Banner(fdNewOut, pcTitle, pcWho, acOurXid);
		pcTitle = (char *)0;
	}
	wTemp = -1;
	switch (uMode) {
	case MODE_FILTER:
		/* already running and humming, or exited */
		break;
	case MODE_PROCESS:
		if (fVerbose) {
			fprintf(fpVerbose, "%s: %s", progname, acUtility);
			for (cc = 0; (char *)0 != argv[cc]; ++cc) {
				fprintf(fpVerbose, " %s", argv[cc]);
			}
			fprintf(fpVerbose, "\n");
			fflush(fpVerbose);
		}
		for (;;) { switch (wTemp = fork()) {
		case -1:
			/* sadly we are in major trouble here, but
			 * we might continue here someday for fork again
			 */
			fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
			exit(EX_OSERR);
		case 0:
			execvp(pcExec, argv);
			execve(pcExec, argv, environ);
			fprintf(stderr, "%s: execve: %s: %s\n", progname, pcExec, strerror(errno));
			exit(EX_CONFIG);
		default:
			break;
		} break; }
		break;
	case MODE_SQUEEZE:
		wTemp = CredPipeFit(acUtility, pcExec, argv, environ, 0);
		uMode = MODE_FILTER;
		break;
	case MODE_POPEN:
		wTemp = CredPipeFit(acUtility, pcExec, argv, environ, 1);
		break;
	}
	if ((pid_t)-1 != wInferior && (pid_t)-1 != wTemp) {
		wInferior = wTemp;
	}

	/* If we can't read at least 1 charactrer from stdin, squeeze it out
	 * (we are certian that we are in filter mode since fSqueeze is set).
	 */
	if (fSqueeze && 1 > uiHolding) {
		register int iGot;

		pcBuffer = (char *)util_ppm_size(&PPM, PIPE_SIZE);
		iGot = read(0, pcBuffer, PIPE_SIZE);
		if (0 < iGot) {
			uiHolding = iGot;
		}
		if (0 == uiHolding) {
			fInfOpts = 0;
			Death('s');
			goto squeezed;
		}
		Banner(fdNewOut, pcTitle, pcWho, acOurXid);
		pcTitle = (char *)0;
		fSqueeze = 0;
	}

	/* Resume stdin pump, even if we filled the buffer capacity.
	 */
	if (uiHolding > 0 || FD_ISSET(0, &fdsReading)) {
		pcBuffer = (char *)util_ppm_size(&PPM, PIPE_SIZE);
		cc = ReWrite(1, pcBuffer, uiHolding);
		if (-1 == cc) {
			exit(EX_OSERR);
		}
		/* I've seen us get hung here, how?
		 */
		while (0 < (cc = read(0, pcBuffer, uiMax))) {
			ReWrite(1, pcBuffer, cc);
		}
		close(0);
		(void)open(acDevNull, O_RDONLY, 0666);
	}
	close(1);
	(void)open(acDevNull, O_RDWR, 0666);
	fInfOpts = 0;		/* wait for all kids now */
	Death('b');
	if (!fSqueeze) {
		Banner(fdNewOut, pcHorizRule, pcWho, acOurXid);
		pcHorizRule = (char *)0;
	}
	close(fdNewOut);

	/* After the code (three digits and a \000) we might send
	 * "cq \000" here, but we don't need it, the close is enough.
	 */
 squeezed:
	if (0 != wInfExits) {
		snprintf(acMessage, sizeof(acMessage), "%03d", (int)wInfExits);
		(void)ReWrite(fdFeed, acMessage, 4);
	}
	close(fdFeed);
	exit(EX_OK);
}
%%
