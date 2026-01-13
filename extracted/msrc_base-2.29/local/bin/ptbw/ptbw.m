#!mkcmd
# $Id: ptbw.m,v 1.76 2010/05/03 13:28:13 ksb Exp $
from '<stdlib.h>'
from '<unistd.h>'
from '<fcntl.h>'
from '<sys/types.h>'
from '<sys/wait.h>'
from '<sys/socket.h>'
from '<sys/un.h>'
from '<signal.h>'
from '<string.h>'
from '<ctype.h>'
from '<netdb.h>'
from '<sys/param.h>'
from '<sys/stat.h>'
from '<sysexits.h>'

from '"fdrights.h"'
from '"machine.h"'

require "util_errno.m"
require "util_tmp.m"
require "util_cache.m"
require "util_nproc.m" "util_divstack.m" "util_divconnect.m"
require "std_help.m" "std_version.m"
require "ptbc.m"

key "divClient" 1 {
	"acPtbwName" 'rt'
}

basename "ptbw" ""
getenv "PTBW"

%i
static const char rcsid[] =
	"$Id: ptbw.m,v 1.76 2010/05/03 13:28:13 ksb Exp $",
	acPtbwName[] = "ptbw",		/* this name doesn't change argv[0] */
	acMySuffix[] = "/ptbdXXXXXX";	/* mkdtemp suffix in $TMPDIR	*/
static char
	cSourceType = '-',		/* the source type (file,func,err)*/
	acTheSource[MAXPATHLEN+4],	/* the original data source	*/
	*pcComments = "",		/* comments from token source	*/
	*pcMyTemp = (char *)0,		/* if we had to build one	*/
	*pcMySpace = (char *)0,		/* where we hide to work	*/
	*pcMySocket = (char *)0;	/* my UNIX domain socket	*/
#if HAVE_ACCEPT_FILTER && defined(SO_ACCEPTFILTER)
static const char
	acAcceptFilter[] = "dataready";
#endif

static char *BorrowTarget(char *pcDirect, unsigned int iMin, unsigned int *piCount, char *pcSource);
%%

augment action 'V' {
	user "Version();"
}

unsigned 'R' {
	named "uReq"
	track 'fGaveReq'
	init '1'
	param "req"
	help "allocate req tags for this instance"
}
unsigned named "iIotaMin" {
	hidden
	init dynamic "1+(7*util_nproc())/4"
	help "CPUS power metric, for this host"
}
char* 't' {
	named "pcTags"
	param "tags"
	help "source which contains controlled resource tags"
}
boolean 'm' {
	once track
	named "fMaster"
	help "manage output for descendant processes"
	boolean 'd' {
		named "fPublish"
		init "1"
		help "do not publish this level in the linked environment"
	}
	unsigned 'J' {
		named "iTaskMin"
		track "fGaveJ"
		init '1'
		param "jobs"
		help "number of parallel tasks expected"
	}
	char* 'N' {
		named "pcBindHere"
		param "path"
		help "force a UNIX domain name for this service"
	}
	exclude 'Vh'
	# This is hard to set and describe in shell, but here goes:	(ksb)
	#  (proc1 &  proc2 & KEEP=$! ; proc3 &  exec ptbw -p $KEEP exampled)
	integer 'p' {
		hidden named "wElected"
		user "wInferior = (pid_t)%n;"
		init "0"
		param "pid"
		help "child process elected to represent our exit status"
	}
	list {
		named "TollBooth"
		param "utility"
		help "control process for task coordination"
	}
}
boolean 'A' {
	exclude 'm'
	named "fAppend"
	init '0'
	help "append resources to the client argument list"
}
boolean 'Q' {
	named "fQuit"
	init '0'
	help "tell the enclosing persistent instance to finish"
}
boolean 'q' {
	named "fQuiet"
	init "0"
	help "be quiet, do not complain about constrained resources"
}
list {
	named "Client"
	param "client"
	help "client command"
}

%c
/* Build a temp directory for my space					(ksb)
 */
static char *
MyPlace()
{
	register char *pcRet;
	extern char *mkdtemp(char *);

	if ((char *)0 == (pcRet = malloc(((strlen(pcTmpdir)+sizeof(acMySuffix))|7)+1))) {
		fprintf(stderr, "%s: malloc: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	(void)strcat(strcpy(pcRet, pcTmpdir), acMySuffix);
	return mkdtemp(pcRet);
}

/* When we are nested inside an xclate we can use the temp directory	(ksb)
 * it built to hide our service socket.  Otherwise we must build one
 * to hide us from the cold cruel world (eval dirname \$xcl_${xcl_link}).
 */
static void
SafePlace(const char *pcInside)
{
	register char *pcEnv, *pcTail;
	static const char acXclGuess[] = "xcl_%s";
	auto char acIndex[(sizeof(acXclGuess)|7)+(32+1)];

	pcMyTemp = (char *)0;
	snprintf(acIndex, sizeof(acIndex), acXclGuess, "link");
	if ((char *)0 != (pcEnv = divCurrent((char *)0))) {
		/* try to buddy with an enclosing instance */
	} else if ((char *)0 != (pcEnv = getenv(acIndex)) && strlen(pcEnv) <= 32 && (isdigit(*pcEnv) || 'd' == *pcEnv)) {
		snprintf(acIndex, sizeof(acIndex), acXclGuess, pcEnv);
		pcEnv = getenv(acIndex);
	}
	if ((char *)0 == pcEnv || (char *)0 == (pcTail = strrchr(pcEnv, '/')) || pcEnv == pcTail) {
		pcMySpace = pcMyTemp = MyPlace();
		return;
	}
	*pcTail = '\000';
	pcMySpace = strdup(pcEnv);
	*pcTail = '/';
}

/* Make the server port for this server to listen on			(ksb)
 * stolen from ptyd, xclate, and others I've coded.
 */
static int
MakeServerPort(pcName)
char *pcName;
{
	register int sCtl, iRet, iListen;
	auto struct sockaddr_un run;
#if HAVE_ACCEPT_FILTER && defined(SO_ACCEPTFILTER)
	auto struct accept_filter_arg FA;
#endif

	if ((sCtl = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "%s: socket: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	run.sun_family = AF_UNIX;
	(void) strcpy(run.sun_path, pcName);
	if (-1 == bind(sCtl, (struct sockaddr *)&run, EMU_SOCKUN_LEN(pcName))) {
		return -1;
	}
	/* On some UNIX-like systems we need a sockopt set first -- sigh
	 */
	iRet = -1;
	for (iListen = 20*64; iListen > 5 && -1 == (iRet = listen(sCtl, iListen)) ; iListen >>= 1) {
		/* try less */
	}
	if (-1 == iRet) {
		fprintf(stderr, "%s: listen: %d: %s\n", progname, iListen, strerror(errno));
		exit(EX_OSERR);
	}
#if HAVE_ACCEPT_FILTER && defined(SO_ACCEPTFILTER)
	(void)memset(&FA, '\000', sizeof(FA));
	(void)strcpy(FA.af_name, acAcceptFilter);
	setsockopt(sCtl, SOL_SOCKET, SO_ACCEPTFILTER, &FA, sizeof(FA));
#endif
	(void)fcntl(sCtl, F_SETFD, 1);
	return sCtl;
}

/* We don't want to leave trash around the filesystem			(ksb)
 * Should be install with atexit before we call SafePlace or Bindings.
 */
static void
CleanUp(void)
{
	if ((char *)0 != pcMySocket) {
		(void)unlink(pcMySocket);
		pcMySocket = (char *)0;
	}
	if ((char *)0 != pcMyTemp) {
		(void)rmdir(pcMyTemp);
		pcMyTemp = (char *)0;
	}
}

/* Message passed as the (void *) target in calls to DiVersion		(ksb)
 */
struct _WMnode {
	int iwidth;			/* used as an int for printf width */
	const char *pcnote;
	int fsawactive;
	const char *pcdirect;
};

/* Format the into about a stacked (or diconnected) diversion		(ksb)
 * (if we get to level 0 return true).
 */
static int
DiVersion(const char *pcName, const char *pcTag, int fActive, void *pvMark)
{
	auto struct stat stSock;
	register struct _WMnode *pWM = pvMark;
	register char *pcTail;

	if ((const char *)0 == pcName) {
		pcName = "--";
	}
	if ((const char *)0 == pcTag) {
		printf("%s:%*s: missing from our environment", progname, pWM->iwidth, pcName);
	} else if (-1 != lstat(pcTag, & stSock)) {
		printf("%s:%*s: %s", progname, pWM->iwidth, pcName, pcTag);
	} else if (ENOTDIR != errno || (char *)0 == (pcTail = strrchr(pcTag, '/'))) {
		printf("%s:%*s: stat: %s: %s", progname, pWM->iwidth, pcName, pcTag, strerror(errno));
	} else {
		*pcTail = '\000';
		if (-1 == lstat(pcTag, & stSock) || S_IFSOCK != (S_IFMT & stSock.st_mode)) {
			printf("%s:%*s: lstat: %s: %s", progname, pWM->iwidth, pcName, pcTag, strerror(errno));
		} else {
			printf("%s:%*s: %s/%s", progname, pWM->iwidth, pcName, pcTag, pcTail+1);
		}
		*pcTail = '/';
	}
	if (fActive) {
		pWM->fsawactive = 1;
		printf("%s", pWM->pcnote);
	}
	if ((const char *)0 != pWM->pcdirect && 0 == strcmp(pcTag, pWM->pcdirect)) {
		pWM->pcdirect = (const char *)0;
		printf(" [-t]");
	}
	printf("\n");
	if (fActive && !isdigit(*pcName)) {
		return 1;
	}
	return '0' == pcName[0] && '\000' == pcName[1];
}

/* We output more than you'd want to know, good for debugging		(ksb)
 */
static void
Version()
{
	register const char *pcLevel;
	auto unsigned uOuter;
	auto struct _WMnode WMParam;

	(void)divNumber(& uOuter);
	printf("%s: default number of tokens: %u\n", progname, iIotaMin);
	divVersion(stdout);
	printf("%s: protocol version %.*s\n", progname, (int)strlen(PTBacVersion)-1, PTBacVersion);
#if HAVE_ACCEPT_FILTER && defined(SO_ACCEPTFILTER)
	printf("%s: accept filter: %s\n", progname, acAcceptFilter);
#endif
	printf("%s: safe directory template: %s\n", progname, acMySuffix+1);
	pcLevel = divCurrent((char *)0);
	WMParam.iwidth = (char *)0 == pcLevel ? 3 : strlen(pcLevel)+2;
	WMParam.pcnote = " [target]";
	WMParam.fsawactive = 0;
	WMParam.pcdirect = pcTags;
	if (0 == divSearch(DiVersion, (void *)& WMParam) && iDevDepth >= uOuter) {
		if ((const char *)0 != WMParam.pcdirect) {
			DiVersion("-t", WMParam.pcdirect, !WMParam.fsawactive, (void *)&WMParam);
		} else if (0 == iDevDepth) {
			printf("%s: no current diversions\n", progname);
		} else {
			printf("%s: depth -%d is too great for current stack of %u\n", progname, iDevDepth, uOuter);
		}
	} else if ((const char *)0 != WMParam.pcdirect) {
		DiVersion("-t", WMParam.pcdirect, !WMParam.fsawactive, (void *)&WMParam);
	} else if (!WMParam.fsawactive) {
		printf("%s: never saw the active diversion\n", progname);
	}
}

/* Keep up with -t targets.  We know that 127 is a big list,		(ksb)
 * so we don't have to make up clever code to search them.
 */
typedef struct TEnode {
	char *pctoken;
	pid_t wlock;
	struct TEnode *pTEnext;		/* link to next token, same pid */
} TARGET_ENV;
static TARGET_ENV *pTEList = (TARGET_ENV *)0;
static unsigned uTargetMax = 0, uTargetSize = 0;
static unsigned uTargetSpace = 0;
static unsigned uTargetCursor = 0, uTargetFree = 0;
static unsigned uTargetWidth = 0;

/* Read the targets list, or generate a default one.			(ksb)
 * The default is `-1 @ iota' {"0", "1", ..., uReq*iTaskMin-1}.
 */
static void
SetTargets(char *pcName)
{
	register char *pcBuffer, *pcStart;
	auto unsigned int uLines;
	register TARGET_ENV *pTE;
	register unsigned int i;
	register size_t iL, iC;

	/* -J must be > 0, iota == -J * -R || iota > -R
	 */
	if (iTaskMin < 1) {
		iTaskMin = 1;
	}
	if (fGaveJ && fGaveReq) {
		iIotaMin = iTaskMin * uReq;
	} else if (fGaveJ) {
		iIotaMin = iTaskMin;
	} else if (fGaveReq) {
		iIotaMin = uReq;
	}

	uLines = iIotaMin;
	pcBuffer = (char *)0;
	(void)strncpy(acTheSource, "null", sizeof(acTheSource));
	if ((char *)0 == pcName || '\000' == pcName[0]) {
		/* use iota defalt below */
	} else if ('-' == pcName[0] && '\000' == pcName[1]) {
		pcBuffer = BorrowTarget((char *)0, iIotaMin, & uLines, acTheSource);
	} else if (PTBIsSocket(pcName)) {
		pcBuffer = BorrowTarget(pcName, iIotaMin, & uLines, acTheSource);
	} else {
		register int fdTemp;
		if (-1 == (fdTemp = open(pcName, O_RDONLY, 0444))) {
			fprintf(stderr, "%s: open: %s: %s\n", progname, pcName, strerror(errno));
			exit(EX_NOINPUT);
		}
		pcBuffer = cache_file(fdTemp, & uLines);
		close(fdTemp);
		if (0 == uLines) {
			fprintf(stderr, "%s: %s: contains no lines\n", progname, pcName);
			exit(EX_DATAERR);
		}
		(void)strncpy(acTheSource, pcName, sizeof(acTheSource));
		cSourceType = '+';
	}
	if (uLines < uReq) {
		fprintf(stderr, "%s: token count of %u is less than required to run any task (%u)\n", progname, uLines, uReq);
		exit(EX_DATAERR);
	}
	if (!fQuiet && (fGaveReq || fGaveJ) && uLines < iIotaMin) {
		fprintf(stderr, "%s: token count %u less than suggested (%u)\n", progname, uLines, iIotaMin);
		fflush(stderr);
	}
	if ((char *)0 == pcBuffer && 0 < uLines) {
		pcBuffer = PTBIota(uLines, & uTargetSize);
		cSourceType = '=';
		(void)strncpy(acTheSource, "iota", sizeof(acTheSource));
	}
	if ((char *)0 == pcBuffer || 0 == uLines) {
		fprintf(stderr, "%s: no tokens generated\n", progname);
		exit(EX_DATAERR);
	}
	pcStart = pcBuffer;
	pTEList = (TARGET_ENV *)calloc(uLines, sizeof(TARGET_ENV));
	if ((TARGET_ENV *)0 == pTEList) {
		fprintf(stderr, "%s: calloc: %u,%u: %s\n", progname, uLines, (unsigned)sizeof(TARGET_ENV), strerror(errno));
		exit(EX_OSERR);
	}
	/* Put only the non-comment lines in the tableau, then means we also
	 * strip comments from up-stream data sockets.  Clever?
	 */
	uTargetWidth = 0;
	iC = 1;
	for (i = uLines, pTE = pTEList; 0 < i--; pcBuffer += iL+1) {
		iL = strlen(pcBuffer);
		if ('#' == *pcBuffer) {
			iC += iL+1;
			--uLines;
			continue;
		}
		if (iL > uTargetWidth) {
			uTargetWidth = iL;
		}
		pTE->pctoken = pcBuffer;
		pTE->wlock = 0;
		pTE->pTEnext = (TARGET_ENV *)0;
		++pTE;
	}
	if (0 == uLines) {
		fprintf(stderr, "%s: no tokens, only comments generated\n", progname);
		exit(EX_DATAERR);
	}

	/* Save the comments for the 'C' command, terminate each with '\n'
	 */
	if ((char *)0 == (pcBuffer = calloc((iC|7)+1, sizeof(char)))) {
		fprintf(stderr, "%s: calloc: %u: %s\n", progname, (unsigned)iC, strerror(errno));
		exit(EX_OSERR);
	}
	for (pcComments = pcBuffer; iC > 1; pcStart += iL+1) {
		iL = strlen(pcStart);
		if ('#' != *pcStart) {
			continue;
		}
		(void)memmove(pcBuffer, pcStart, iL);
		pcBuffer += iL;
		iC -= iL+1;
		*pcBuffer++ = '\n';
	}
	*pcBuffer = '\000';

	/* Put the token strings in a contiguous block, for the T command
	 */
	uTargetFree = uTargetMax = uLines;
	for (i = uLines, pTE = pTEList; 1 < i; --i, ++pTE) {
		iL = strlen(pTE[0].pctoken)+1;
		if (pTE[0].pctoken+iL == pTE[1].pctoken)
			continue;
		pTE[1].pctoken = (char *)memmove(pTE[0].pctoken+iL, pTE[1].pctoken, strlen(pTE[1].pctoken)+1);
	}
	uTargetSpace = pTEList[uLines-1].pctoken+strlen(pTEList[uLines-1].pctoken)+1 - pTEList[0].pctoken;
}

/* Manage client <> token interactions					(ksb)
 */
typedef struct PTnode {
#define PB_IDLE		0
#define PB_NOCRED	1
#define PB_ONLINE	2
#define PB_BLOCK	3	/* wating for a token */
#define PB_CTL		9
	unsigned int ustate;	/* from above			*/
	unsigned int uweight;	/* unimplemented fair share idea */
	unsigned int unumber;	/* partial prefix number	*/
	unsigned int iheld;	/* how many held by pid		*/
	pid_t wpid;		/* clients pid			*/
	struct PTnode *pPTnext;	/* next waiting client		*/
	char acfrom[MAXHOSTNAMELEN+4]; /* where we came from	*/
} PT_BOOK;

/* Figure out whose turn it is to be get a token set.			(ksb)
 * Service the largest weight that is > the number requested, or
 * just the largest request.  Return the one selected, and leave
 * the list of the rest complete.
 */
static PT_BOOK *
DeQueue(PT_BOOK **ppPTList)
{
	register PT_BOOK *pPTScan, *pPTMax;
	register PT_BOOK **ppPTSnip = (PT_BOOK **)0;

	if ((PT_BOOK **)0 == ppPTList || 0 == uTargetFree) {
		return (PT_BOOK *)0;
	}
	for (pPTMax = (PT_BOOK *)0; (PT_BOOK *)0 != (pPTScan = *ppPTList); ppPTList = & pPTScan->pPTnext) {
		if (pPTScan->unumber > uTargetFree) {
			continue;
		}
		pPTScan->uweight += 1;
		if ((PT_BOOK *)0 == pPTMax || pPTMax->unumber < pPTScan->unumber || (pPTMax->unumber == pPTScan->unumber && pPTMax->uweight < pPTScan->uweight)) {
			ppPTSnip = ppPTList;
			pPTMax = pPTScan;
		}
	}
	if ((PT_BOOK *)0 == pPTMax) {
		return (PT_BOOK *)0;
	}
	*ppPTSnip = pPTMax->pPTnext;
	pPTMax->pPTnext = (PT_BOOK *)0;
	return pPTMax;
}

/* Free all that a process holds					(ksb)
 */
static int
TokFree(unsigned *puTarget, PT_BOOK *pPTOwner)
{
	register int i, iRet;
	register pid_t wRelease;

	if ((unsigned *)0 != puTarget && *puTarget > uTargetMax) {
		return 0;
	}
	wRelease = pPTOwner->wpid;
	iRet = 0;
	for (i = 0; i < uTargetMax; ++i) {
		if (wRelease != pTEList[i].wlock)
			continue;
		if ((unsigned *)0 != puTarget && *puTarget != i)
			continue;
		pTEList[i].wlock = 0;
		++iRet;
	}
	pPTOwner->iheld -= iRet;
	uTargetFree += iRet;
	return iRet;
}

/* See if a token is available						(ksb)
 * return -1 for no such luck, else index
 */
static int
TokAlloc(PT_BOOK *pPT)
{
	register int i, iRet;

	iRet = 0;
	for (;;) {
		for (i = uTargetMax; 0 < i; --i, ++uTargetCursor) {
			if (uTargetMax <= uTargetCursor)
				uTargetCursor = 0;
			if (0 != pTEList[uTargetCursor].wlock)
				continue;
			pTEList[uTargetCursor].wlock = pPT->wpid;
			--uTargetFree;
			pPT->iheld += 1;
			return uTargetCursor++;
		}
	}
	return -1;
}

/* Allocate a long list of resources, fail if we can't get all		(ksb)
 */
static TARGET_ENV *
TokGroup(unsigned uNeed, PT_BOOK *pPT)
{
	register TARGET_ENV **ppTEChain;
	register int i, iTok;
	auto TARGET_ENV *pTERet;

	/* caller checked max/client for us, we just check to over max
	 */
	if (uNeed > uTargetFree) {
		return (TARGET_ENV *)0;
	}
	ppTEChain = & pTERet;
	for (i = 0; i < uNeed; ++i) {
		if (-1 == (iTok = TokAlloc(pPT))) {
			fprintf(stderr, "%s: %d: internal invarient snark\n", progname, __LINE__);
			break;
		}
		if (pPT->wpid != pTEList[iTok].wlock) {
			fprintf(stderr, "%s: %d: lock invarient snark!\n", progname, __LINE__);
		}
		*ppTEChain = & pTEList[iTok];
		ppTEChain = & pTEList[iTok].pTEnext;
	}
	*ppTEChain = (TARGET_ENV *)0;
	return pTERet;
}

static volatile pid_t wInferior = -1;
static volatile int wInfExits = EX_PROTOCOL;
static volatile int fInfOpts = WNOHANG|WUNTRACED;

/* When a signal for a child arrives we burry the dead.			(ksb)
 * If he is "our boy", then forget he's alive.
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
		if (wInferior == wReap) {
			wInferior = 0;
			wInfExits = WEXITSTATUS(wStatus);
		}
	}
}

/* signal handler to remove our socket when we are signaled		(ksb)
 */
static void
DeadMan(int _dummy)	/*ARGSUSED*/
{
	exit(0);
}

/* Build the service socket, put the mame in the environment		(ksb)
 */
static int
Bindings(unsigned uMake)
{
	static const char acEnvRead[] = "PTBW";
	register char *pcTail;
	register unsigned int i;
	register int sRet;
	auto struct stat stLook;
	auto char acLevel[(sizeof(acEnvRead)|15)+(32+1)];
	auto struct sigaction saWant;

	/* Accepting a -HUP signal to re-load the token data adds
	 * uncertianty for no gain here, just install -TERM -- ksb
	 */
	(void)memset((void *)& saWant, '\000', sizeof(saWant));
	saWant.sa_handler = (void *)DeadMan;
#if HAVE_SIGACTION
	saWant.sa_sigaction = (void *)DeadMan;
#endif
	saWant.sa_flags = SA_RESTART;
	if (-1 == sigaction(SIGINT, & saWant, (struct sigaction *)0)) {
		fprintf(stderr, "%s: sigaction: INT: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	(void)sigaction(SIGTERM, & saWant, (struct sigaction *)0);

	if ((char *)0 != pcBindHere) {
		if ('/' == *pcBindHere) {
			pcMySocket = pcBindHere;
		} else {
			register int iGuess;
			iGuess = (15|(strlen(pcMySpace)+strlen(pcBindHere)))+9;
			pcMySocket = malloc(iGuess);
			snprintf(pcMySocket, iGuess, "%s/%s", pcMySpace, pcBindHere);
		}
		if (-1 == (sRet = MakeServerPort(pcMySocket))) {
			fprintf(stderr, "%s: bind: %s: %s\n", progname, pcMySocket, strerror(errno));
			exit(EX_OSERR);
		}
	} else {
		pcMySocket = malloc((strlen(pcMySpace)|63)+65);
		pcTail = strchr(strcpy(pcMySocket, pcMySpace), '\000');
		if (pcTail > pcMySocket && '/' != pcTail[-1]) {
			*pcTail++ = '/';
		}
		for (i = 0; 1; ++i) {
			snprintf(pcTail, 64, "sta%u", i);
			if (-1 != lstat(pcMySocket, & stLook))
				continue;
			if (-1 != (sRet = MakeServerPort(pcMySocket)))
				break;

			/* We failed to get a bind in pcMySpace, if we
			 * didn't make a space we still can, once. -- ksb
			 * (Someone chmod'd our super-space ugo-w, sigh.)
			 */
			if ((char *)0 != pcMyTemp) {
				fprintf(stderr, "%s: bind: %s: %s\n", progname, pcMySocket, strerror(errno));
				exit(EX_OSERR);
			}
			pcMySpace = pcMyTemp = MyPlace();
			return Bindings(uMake);
		}
	}

	/* Notify the inferior processes of our IPC location
	 */
	if (fPublish) {
		divPush(pcMySocket);
	} else {
		divDetach(pcMySocket, (const char *)0);
	}

	/* Diddle the options variable $PTBW_$level -> $PTBW, I'm a wrapper
	 */
	(void)snprintf(acLevel, sizeof(acLevel), "%s_%u", acEnvRead, uMake);
	if ((char *)0 != (pcTail = getenv(acLevel))) {
		(void)setenv(acEnvRead, pcTail, 1);
	} else {
		(void)unsetenv(acEnvRead);
	}

	return sRet;
}

/* Send a list of token indexes sep with ',', end with '\n'		(ksb)
 */
static void
SendTokList(int sClient, int uCount, TARGET_ENV *pTENext)
{
	register TARGET_ENV *pTE;
	auto char acSend[64];

	while ((TARGET_ENV *)0 != (pTE = pTENext)) {
		pTENext = pTE->pTEnext;
		snprintf(acSend, sizeof(acSend), "%u%c", (unsigned)(pTE-pTEList), (TARGET_ENV *)0 != pTENext ? ',' : '\n');
		write(sClient, acSend, strlen(acSend));
	}
}

/* Now lead the band, from backstage, kinda.				(ksb)
 * we ignore 0, 1, 2 which we believe can't be clients
 *
 * We are a local service (only) -- since we are protected by a mode 0700
 * directory, a UNIX domain socket, and a fairly private parameter pass
 * down (in the environment).  We try to protect against an accidental
 * hang (^Z in the wrong place), but not against a malicious attack.
 * The code to do an async rights exchange is not so easy.
 * [Yes, I know ps and /proc can show the environment, mode 0700 still works.]
 */
static void
FiniteStates(sCtl)
{
	register int iClients, i, iTok;
	register PT_BOOK *pPTCur;
	register TARGET_ENV *pTEFound;
	auto PT_BOOK *pPTQueue;
	auto int iMaxPoll;
	auto fd_set fdsTemp, fdsReading;
	auto struct PTnode aPTBook[FD_SETSIZE];
	auto char acSend[8192];	/* short reply strings */
	auto struct sigaction saWant;
	auto struct timeval stStall;

	/* setup books, token table too
	 */
	for (i = 0; i < FD_SETSIZE; ++i) {
		aPTBook[i].ustate = i < 3 ? PB_CTL : PB_IDLE;
		aPTBook[i].uweight = 0;
		aPTBook[i].unumber = 0;
		aPTBook[i].iheld = 0;
		aPTBook[i].pPTnext = (PT_BOOK *)0;
	}
	pPTQueue = (PT_BOOK *)0;
	iClients = 0;
	iMaxPoll = sCtl;
	FD_ZERO(& fdsReading);
	FD_SET(sCtl, & fdsReading);
	aPTBook[sCtl].ustate = PB_CTL;
	aPTBook[sCtl].uweight = 0xf00;

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
	Death('M');

	/* We could exit the master process while we are coping the
	 * fd set, so we must timeout the select to avoid the race.
	 * As a ptbw can wait a while after the last client to exit
	 * we only poll 17 times a miniute, after the first 8 seconds.
	 */
	stStall.tv_sec = 8;
	stStall.tv_usec = 32040;
	while (0 != wInferior || 0 != iClients) {
		register int iGot;

		fdsTemp = fdsReading;
		if (-1 == (iGot = select(iMaxPoll+1, &fdsTemp, (fd_set *)0, (fd_set *)0, 0 == iClients ? & stStall : (struct timeval *)0))) {
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

		if (0 < iGot && FD_ISSET(sCtl, & fdsTemp)) {
			register int sClient;

			if (-1 == (sClient = accept(sCtl, (struct sockaddr *)0, (void *)0))) {
				if (EINTR == errno)
					continue;
				/* Someone broke our listening socket.
				 */
				FD_CLR(sCtl, & fdsReading);
				close(sCtl);
				continue;
			}
			if (sClient >= iMaxPoll) {
				iMaxPoll = sClient;
			}
			++iClients;
			FD_SET(sClient, & fdsReading);
			aPTBook[sClient].ustate = PB_NOCRED;
			aPTBook[sClient].uweight = 10;
			aPTBook[sClient].unumber = 0;
			--iGot;
		}

		/* Start at stderr+1, don't scan 0, 1, 2 every time -- ksb
		 */
		for (i = 3; 0 != iGot && i <= iMaxPoll; ++i) {
			register int iLen;
			auto char acCmd[4];

			if (!FD_ISSET(i, & fdsTemp) || sCtl == i) {
				continue;
			}
			--iGot;
			/* The first message must be a cred send --ksb
			 * A client can block us here, we should timeout,
			 * but who would do that?
			 */
			if (PB_NOCRED == aPTBook[i].ustate) {
				if (-1 == (aPTBook[i].wpid = PTBPidRecv(i, aPTBook[i].acfrom, sizeof(aPTBook[i].acfrom)))) {
					close(i);
					aPTBook[i].ustate = PB_IDLE;
					aPTBook[i].uweight = 0;
					continue;
				}
				aPTBook[i].ustate = PB_ONLINE;
				aPTBook[i].uweight = uReq;
				aPTBook[i].pPTnext = (void *)0;
				continue;
			}
			/* All commands are a single letter to avoid live locks
			 */
			iLen = read(i, acCmd, 1);
			if (1 != iLen || '.' == acCmd[0]) {
				close(i);
				aPTBook[i].ustate = PB_IDLE;
				aPTBook[i].uweight = 0;
				(void)TokFree((unsigned *)0, &aPTBook[i]);
				FD_CLR(i, & fdsReading);
				--iClients;
				continue;
			}
			/* send iTok as the reply via a break, continue doesn't
			 */
			switch (acCmd[0]) {
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				aPTBook[i].unumber *= 10;
				aPTBook[i].unumber += acCmd[0]-'0';
				continue;
			case 'C':	/* token comments */
				iTok = strlen(pcComments);
				snprintf(acSend, sizeof(acSend), "%u\n", (unsigned)iTok);
				write(i, acSend, strlen(acSend));
				write(i, pcComments, iTok);
				continue;
			case 'V':	/* version reply /[0-9]+[.][0-9]+\n/ */
				write(i, PTBacVersion, strlen(PTBacVersion));
				continue;
			case 'M':	/* reply with master process id */
				if (0 == (iTok = wInferior)) {
					write(i, "-No\n", 4);
					continue;
				}
				break;
			case 'Q':	/* is in : mode quit after this */
				if (getpid() == wInferior) {
					wInferior = 0;
				}
				if (0 == wInferior) {
					iTok = iClients;
					break;
				}
				write(i, "-No\n", 4);
				continue;
			case 'S':	/* status "Count,size\n" */
				snprintf(acSend, sizeof(acSend), "%u,%u\n", uTargetMax, uTargetSpace);
				write(i, acSend, strlen(acSend));
				continue;
			case 'U':	/* token source name\000 */
				write(i, & cSourceType, 1);
				write(i, acTheSource, strlen(acTheSource)+1);
				continue;
			case 'T':	/* token list inline */
				write(i, pTEList[0].pctoken, uTargetSpace);
				continue;
			case 'P':	/* poll for a single token */
				iTok = TokAlloc(& aPTBook[i]);
				break;
			case 'G':	/* allocate a group of N tokens */
				/* if we don't make it happen, we block them
				 */
				iTok = 0 == aPTBook[i].unumber ? 1 : (int)aPTBook[i].unumber;
				if (iTok > uTargetMax) {
					iTok = -1;
					break;
				}
				/* with -R 3 and -J 100, we stall req 40
				 * forever, I think
				 */
				if (iTok > aPTBook[i].uweight || (TARGET_ENV *)0 == (pTEFound = TokGroup(iTok, & aPTBook[i]))) {
					register PT_BOOK *pPT;
					/* Advance those in-line first
					 */
					for (pPT = pPTQueue; (PT_BOOK *)0 != pPT; pPT = pPT->pPTnext) {
						pPT->uweight += 1;
					}
					aPTBook[i].pPTnext = pPTQueue;
					pPTQueue = & aPTBook[i];
					aPTBook[i].unumber = iTok;
					aPTBook[i].ustate = PB_BLOCK;
					FD_CLR(i, & fdsReading);
					continue;
				}
				aPTBook[i].unumber = 0;
				aPTBook[i].uweight -= iTok;
				SendTokList(i, iTok, pTEFound);
				continue;
			case 'F':	/* free all we hold */
				aPTBook[i].unumber = 0;
				iTok = TokFree((unsigned *)0, &aPTBook[i]);
				break;
			case 'D':	/* release token in number buffer */
				iTok = TokFree(& aPTBook[i].unumber, &aPTBook[i]);
				aPTBook[i].unumber = 0;
				break;
			case 'N':	/* what name for this token? */
				iTok = aPTBook[i].unumber;
				aPTBook[i].unumber = 0;
				if (0 > iTok || iTok > uTargetMax) {
					write(i, "-No\n", 4);
					continue;
				}
				write(i, "+", 1);
				write(i, pTEList[iTok].pctoken, strlen(pTEList[iTok].pctoken));
				write(i, "\n", 1);
				continue;
			case 'O':	/* pid owner of this token numbner? */
				/* zero means "no lock", -1 mean no token */
				iTok = aPTBook[i].unumber;
				aPTBook[i].unumber = 0;
				if (0 > iTok || iTok > uTargetMax) {
					iTok = -1;
				} else {
					iTok = pTEList[iTok].wlock;
				}
				break;
			case 'H':	/* later, host that pid lives on */
				/* ZZZ implement by changing wlock's meaning */
				write(i, "-Not yet\n", 9);
				continue;
			case 'J':	/* justification token width	*/
				iTok = uTargetWidth;
				break;
			case 'R':	/* -R uReq I was given */
				iTok = uReq;
				break;
			default:	/* unknonw command */
				write(i, "-No\n", 4);
			case '\n':	/* silent sync */
				aPTBook[i].unumber = 0;
				continue;
			}
			snprintf(acSend, sizeof(acSend), "%d\n", iTok);
			write(i, acSend, strlen(acSend));
		}
		while (iMaxPoll > sCtl && PB_IDLE == aPTBook[iMaxPoll].ustate) {
			--iMaxPoll;
		}

		/* We have a token for some client, wake them up.
		 */
		while ((PT_BOOK *)0 != (pPTCur = DeQueue(& pPTQueue))) {
			i = pPTCur-aPTBook;
			iTok = aPTBook[i].unumber;
			if ((TARGET_ENV *)0 == (pTEFound = TokGroup(iTok, pPTCur))) {
				write(i, "-No\n", 4);
			} else {
				SendTokList(i, iTok, pTEFound);
			}
			aPTBook[i].unumber = 0;
			aPTBook[i].uweight -= iTok;
			aPTBook[i].ustate = PB_ONLINE;
			FD_SET(i, & fdsReading);
		}
	}
}

/* Setup the toll booth and run the state machine above			(ksb)
 */
static void
TollBooth(int argc, char **argv)
{
	register const char *pcArgv0, *pcOuterLevel;
	register unsigned uBoothLevel;
	auto int sCtl;
	static char *apcDefArgv[2];

	if (0 == argc || (char **)0 == argv || (char *)0 == argv[0]) {
		argc = 1;
		argv = apcDefArgv;
		if ((char *)0 == (apcDefArgv[0] = getenv("SHELL"))) {
			apcDefArgv[0] = "sh";
		}
		apcDefArgv[1] = (char *)0;
	}
	if ((char *)0 == (pcOuterLevel = divCurrent((char *)0))) {
		uBoothLevel = 1;
	} else {
		uBoothLevel = atoi(pcOuterLevel)+1;
	}

	atexit(CleanUp);
	SafePlace(pcOuterLevel);
	SetTargets(pcTags);
	sCtl = Bindings(uBoothLevel);
	if (':' == argv[0][0] && '\000' == argv[0][1] && (char *)0 == argv[1]) {
		if (-1 == wInferior)
			wInferior = getpid();
	} else switch (wInferior = fork()) {
	case -1:
		fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		(void)chdir("/");
		exit(EX_OSERR);
	case 0:
		(void)close(sCtl);
		pcArgv0 = argv[0];
		if (0 != strcmp(acPtbwName, progname)) {
			argv[0] = progname;
		}
		(void)execvp(pcArgv0, argv);
		fprintf(stderr, "%s: execvp: %s: %s\n", progname, argv[0], strerror(errno));
		exit(EX_OSERR);
	default:
		break;
	}

	/* We are not really a daemon, but we don't want to be in the way,
	 * so free us from stdin, stdout, and the current working dir.
	 * We keep stderr for messages, but shouldn't spew any.
	 */
	(void)close(0);
	(void)open("/dev/null", O_RDONLY, 0666);
	(void)close(1);
	(void)open("/dev/null", O_WRONLY, 0666);
	(void)chdir(pcMySpace);
	FiniteStates(sCtl);
	close(sCtl);
	(void)chdir("/");
	fInfOpts = 0;
	Death('T');
	exit(wInfExits);
}

/* Support for the client side, first the "status report"		(ksb)
 * This is poor form: put with scanf, and get with write.  We use the
 * "\n" on the end of the previous message to stop fscanf.  Be warned,
 * and don't do this w/o an Understanding -- ksb
 */
static int
JustStats(int sMaster)
{
	register FILE *fpFrom;
	register char *pcTokTable, *pcCursor;
	register int i, c;
	auto char acCvt[MAXPATHLEN+4];
	auto unsigned int uMasterR, uMasterL;

	if ((FILE *)0 == (fpFrom = fdopen(sMaster, "rb+"))) {
		fprintf(stderr, "%s: fdopen: %d: %s\n", progname, sMaster, strerror(errno));
		return 32;
	}
	if ((char *)0 == (pcTokTable = malloc((uOurSize|7)+1))) {
		return 35;
	}
	if (1 != write(sMaster, "T", 1)) {
		return 36;
	}
	if (uOurSize != fread((void *)pcTokTable, sizeof(char), uOurSize, fpFrom)) {
		return 38;
	}
	if (3 != write(sMaster, "RJU", 3)) {
		return 39;
	}
	fscanf(fpFrom, " %u", & uMasterR);
	if ('\n' != (c = fgetc(fpFrom))) {
		return 40;
	}
	fscanf(fpFrom, " %u", & uMasterL);
	if ('\n' != (c = fgetc(fpFrom))) {
		return 41;
	}
	printf("%s: master has %d tokens, max length %u (total space %d)\n", progname, uOurCount, uMasterL, uOurSize);
	printf(" Master tokens from ");
	c = fgetc(fpFrom);
	switch (c) {
	case '=':
		printf("the internal function: ");
		break;
	case '+':
		printf("file: ");
		break;
	case '-':
		printf("error: ");
		break;
	default:
		printf("key %c: ", c);
		break;
	}
	while ('\000' != (c = fgetc(fpFrom))) {
		putchar(c);
	}
	printf("\n Index\tLock\tTokens (request in groups of %u)\n", uMasterR);
	pcCursor = pcTokTable;
	for (i = 0; i < uOurCount; ++i) {
		snprintf(acCvt, sizeof(acCvt), "%dO", i);
		write(sMaster, acCvt, strlen(acCvt));
		printf("%6d\t", i);
		while ('\n' != (c = fgetc(fpFrom)) && EOF != c)
			putchar(c);
		printf("\t%s\n", pcCursor);
		pcCursor += strlen(pcCursor)+1;
	}
	fclose(fpFrom);
	return 0;
}

/* Add the resource to the old argv, return the new one, only		(ksb)
 * copy the strings we must (not the last, or the old argv).
 */
static char **
Extender(char **ppcOld, char *pcList)
{
	register char **ppcRet, *pcNl, *pcNext;
	register int i;

	/* Count the length of the existing argument vector;
	 * add one for the last one, add one for each \n separator,
	 * round up and allocate one for the sentinal (char *)0.
	 */
	for (i = 0; (char *)0 != ppcOld[i]; ++i) {
		/*  count'em */
	}
	++i;
	for (pcNl = pcList; (char *)0 != (pcNext = strchr(pcNl, '\n')); pcNl = pcNext+1) {
		++i;	/* add'em */
	}
	i |= 3;
	ppcRet = calloc(i+1, sizeof(char *));

	/* Copy exactly what we counted
	 */
	for (i = 0; (char *)0 != ppcOld[i]; ++i) {
		ppcRet[i] = ppcOld[i];
	}
	for (pcNl = pcList; (char *)0 != (pcNext = strchr(pcNl, '\n')); pcNl = pcNext+1) {
		*pcNext = '\000';
		ppcRet[i++] = strdup(pcNl);
		*pcNext = '\n';
	}
	ppcRet[i++] = pcNl;
	ppcRet[i] = (char *)0;
	return ppcRet;
}

/* We are the client side, like xclate we include a client wrapper.	(ksb)
 * Block for a resource and run the given command with $ptbw_list
 * set to the one we locked.
 */
static void
Client(int argc, char **argv)
{
	register int sHandle;
	register char *pcToken, *pcArgv0;
	register const char *pcOuter;

	pcOuter = divSelect();
	if (-1 == (sHandle = PTBClientPort(pcOuter))) {
		fprintf(stderr, "%s: %s: cannot establish client connection\n", progname, pcOuter);
		exit(EX_DATAERR);
	}
	if (0 == argc || ('-' == argv[0][0] && '\000' == argv[0][1])) {
		exit(JustStats(sHandle));
	}
	if (0 == uReq) {
		divResults((const char *)0);
		pcToken = (char *)0;
	} else if ((char *)0 == (pcToken = PTBReadTokenList(sHandle, uReq, (unsigned int **)0))) {
		fprintf(stderr, "%s: -R %u: cannot allocate resources: %s\n", progname, uReq, strerror(errno));
		exit(EX_CANTCREAT);
	} else {
		divResults(pcToken);
	}
	switch (wInferior = fork()) {
	case -1:
		fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	case 0:
		close(sHandle);
		if (fAppend && (char *)0 != pcToken) {
			argv = Extender(argv, pcToken);
		}
		pcArgv0 = argv[0];
		if (0 != strcmp(acPtbwName, progname)) {
			argv[0] = progname;
		}
		(void)execvp(pcArgv0, argv);
		fprintf(stderr, "%s: execvp: %s: %s\n", progname, argv[0], strerror(errno));
		exit(EX_OSERR);
	default:
		break;
	}
	/* cleanup from our inferiour process and any step-children
	 */
	fInfOpts = 0;
	Death('C');

	/* Tell the master to abandon forever (:) mode, we get back the
	 * number of client connected (ourself included) which we ignore.
	 * If we got back "-No\n", then the master was not in : mode -- ksb
	 */
	if (fQuit) {
		PTBQuit(sHandle, (char *)0, 0);
	}
	close(sHandle);
	exit(wInfExits);
}

/* Get a subset of our masters tokens to distribute to our clients	(ksb)
 * N.B. our return must be a pointer to malloc/calloc'd memory, or (char *)0
 */
static char *
BorrowTarget(char *pcDirect, unsigned int iGoal, unsigned int *puOut, char *pcSource)
{
	register int sParent;
	register unsigned int iTry;
	register char *pcRet, *pcNl;

	*puOut = 0;
	pcRet = (char *)0;
	if ((char *)0 == pcDirect) {
		pcDirect = divSelect();
	}
	if (-1 == (sParent = PTBClientPort(pcDirect))) {
		fprintf(stderr, "%s: %s: %s\n", progname, pcDirect, strerror(errno));
		exit(EX_DATAERR);
	}
	if ((char *)0 != pcSource) {
		cSourceType = PTBReqSource(sParent, pcSource, MAXPATHLEN, &pcComments);
	}
	if (uOurCount < uReq) {
		fprintf(stderr, "%s: not enough tokens from our master to make even a single %u group (only %u)\n", progname, uReq, uOurCount);
		exit(EX_DATAERR);
	}
	for (iTry = (iGoal < uReq) ? uReq : iGoal; uReq <= iTry; iTry -= uReq) {
		if (uOurCount < iTry) {
			continue;
		}
		if ((char *)0 == (pcRet = PTBReadTokenList(sParent, iTry, (unsigned int **)0))) {
			continue;
		}
		break;
	}
	if (fQuit) {
		PTBQuit(sParent, (char *)0, 0);
	}
	if ((char *)0 == pcRet) {
		return pcRet;
	}
	/* convert to internal format from protocol \n format
	 */
	for (pcNl = strchr(pcRet, '\n'); (char *)0 != pcNl; pcNl = strchr(pcNl, '\n')) {
		*pcNl++ = '\000';
	}
	*puOut = iTry;
	return pcRet;
}
