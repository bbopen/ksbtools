#!mkcmd
# $Id: generic.m,v 1.11 2010/03/03 14:46:30 ksb Exp $
# The generic wrapper template -- ksb
# Find the 31 comments marked with double-percent (%%) below, do
# what is implies you should then delete the comment.  You should
# end up with a wrapper.
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
from '<sys/param.h>'
from '<sys/stat.h>'
from '<sysexits.h>'

# %% add any mkcmd from directives or require directives near here %%
from '"machine.h"'

require "util_errno.m"
require "util_tmp.m" "util_divstack.m" "util_divconnect.m"
require "std_help.m" "std_version.m"

# %% below with "<respect> to option t" change "t" to your direct option %%
# %% access option leter %%
key "divClient" 1 {
	"acMyDefName" '<respect>t'
}

# %% change your default basename, and option environment vaiable %%
basename "%%prog%%" ""
getenv "%%PROG%%"

%i
/* %% include anything you need here with a normal #include %% */

static char rcsid[] =
	"$Id: generic.m,v 1.11 2010/03/03 14:46:30 ksb Exp $",
	/* %% the name the doesn't change argv[0] in the inferior %% */
	acMyDefName[] = "%% myself %%",
	/* %% change the mktemp afix "prog" to your favorite %% */
	acMySuffix[] = "/%%prog%%XXXXXX";	/* mkdtemp suffix in $TMPDIR	*/
static char
	*pcMyTemp = (char *)0,		/* if we had to build one	*/
	*pcMySpace = (char *)0,		/* where we hide to work	*/
	*pcMySocket = (char *)0;	/* my UNIX domain socket	*/
#if HAVE_ACCEPT_FILTER && defined(SO_ACCEPTFILTER)
static const char
	acAcceptFilter[] = "dataready";
#endif

/* %% any required forward declaration, ignore the double-percent below %% */
%%

augment action 'V' {
	user "Version();"
}

# %% this is the "direct" option from ptbw, pick your letter %%
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
	char* 'N' {
		named "pcBindHere"
		param "path"
		help "force a UNIX domain name for this service"
	}
	exclude 'Vh'
	list {
		named "Master"
		param "utility"
		help "control process for task coordination"
	}
}
boolean 'Q' {
	named "fQuit"
	init '0'
	help "tell the enclosing persistent instance to finish"
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
	/* %% you might pick another friend, rather than xcl_ below %% */
	static const char acXclGuess[] = "xcl_%s";
	auto char acIndex[(sizeof(acXclGuess)|7)+(32+1)];

	pcMyTemp = (char *)0;
#if HAVE_SNPRINTF
	snprintf(acIndex, sizeof(acIndex), acXclGuess, "link");
#else
	sprintf(acIndex, acXclGuess, "link");
#endif
	if ((char *)0 != (pcEnv = divCurrent((char *)0))) {
		/* try to buddy with an enclosing instance */
	} else if ((char *)0 != (pcEnv = getenv(acIndex)) && strlen(pcEnv) <= 32 && (isdigit(*pcEnv) || 'd' == *pcEnv)) {
#if HAVE_SNPRINTF
		snprintf(acIndex, sizeof(acIndex), acXclGuess, pcEnv);
#else
		sprintf(acIndex, acXclGuess, pcEnv);
#endif
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
	/* %% you might need to prep the unix domain socket %% */
	/* PidPrep(sCtl); */
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

/* Pass pcDirect = /tmp-dir/socket-name to contact
 * a ptbw-like service.
 */
int
MakeClientPort(const char *pcDirect)
{
	register int s, iFailed, iSlow;
	auto struct sockaddr_un run;

	for (iSlow = 0; ; iSlow ^= 1) {
		if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
			return -1;
		}
		memset((void *)& run, '\000', sizeof(run));
		run.sun_family = AF_UNIX;
		if (strlen(pcDirect) > sizeof(run.sun_path)-1) {
			errno = ENAMETOOLONG;
			goto failed;
		}
		(void)strcpy(run.sun_path, pcDirect);
		if (-1 != connect(s, (struct sockaddr *)&run, strlen(pcDirect)+2)) {
			(void)fcntl(s, F_SETFD, 1);
			break;
		}
		if (ECONNREFUSED != errno) {
			goto failed;
		}
		close(s);
		s = -1;
		/* try again */
		sleep(iSlow);
	}
	/* %% any setup to get started here %% */
	if (iFailed) {
		register int e;
		errno = EPFNOSUPPORT;
failed:
		e = errno;
		(void)close(s);
		errno = e;
		return -1;
	}
	return s;
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
	int iwidth;
	const char *pcnote;
	int fsawactive;
	/* %% you'll need to customize this, more than likely, see below %% */
	const char *pcdirect;
};

/* Format the info about a stacked (or diconnected) diversion		(ksb)
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
	/* %% if you don't have a pcTags direct option delete this if %% */
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
	/* %% output any tunes or versions the Customer needs to see here %% */
	divVersion(stdout);
#if HAVE_ACCEPT_FILTER && defined(SO_ACCEPTFILTER)
	printf("%s: accept filter: %s\n", progname, acAcceptFilter);
#endif
	printf("%s: safe directory template: %s\n", progname, acMySuffix+1);
	pcLevel = divCurrent((char *)0);
	WMParam.iwidth = (char *)0 == pcLevel ? 3 : strlen(pcLevel)+2;
	WMParam.pcnote = " [target]";
	WMParam.fsawactive = 0;
	/* %% you might need more stuff here to setupt DiVersion %%
	 * %% replace -t with your option letter below %%
	 */
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

/* %% put your support code here %%
 */
static void
MyMaster(int fdCtl)
{
}

static volatile pid_t wInferior = 0;
static volatile int wInfExits = EX_SOFTWARE;
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
			wInfExits = wStatus;
		}
	}
}

/* Signal handler to remove our socket when we are signaled;		(ksb)
 * we use atexit(3) to do that, by the way.
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
	static const char acEnvRead[] = "PTBW";/* %% change this %% */
	register char *pcTail;
	register unsigned int i;
	register int sRet;
	auto struct sigaction saWant;
	auto struct stat stLook;
	auto char acLevel[(sizeof(acEnvRead)|15)+(32+1)];

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

	/* Accepting a -HUP signal to re-load the tableau data adds
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
#if HAVE_SNPRINTF
			snprintf(pcMySocket, iGuess, "%s/%s", pcMySpace, pcBindHere);
#else
			sprintf(pcMySocket, "%s/%s", pcMySpace, pcBindHere);
#endif
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
			/* %% socket mnemonic "dIv" can be any short word %% */
			snprintf(pcTail, 64, "dIv%u", i);
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

	/* Diddle the options variable $US_$level -> $US, I'm a wrapper
	 */
	(void)snprintf(acLevel, sizeof(acLevel), "%s_%u", acEnvRead, uMake);
	if ((char *)0 != (pcTail = getenv(acLevel))) {
		(void)setenv(acEnvRead, pcTail, 1);
	} else {
		(void)unsetenv(acEnvRead);
	}

	return sRet;
}


/* We are the client side, like xclate we include a client wrapper.	(ksb)
 * %% fill in your client code %%
 */
static void
Client(int argc, char **argv)
{
	register int sHandle;
	register char *pcArgv0;
	register const char *pcOuter;
	register pid_t wReap;
	auto int wStatus;
	auto struct sigaction saWant;

	(void)memset((void *)& saWant, '\000', sizeof(saWant));
	saWant.sa_handler = (void *)Death;
#if HAVE_SIGACTION
	saWant.sa_sigaction = (void *)Death;
#endif
	saWant.sa_flags = SA_RESTART;
	if (-1 == sigaction(SIGCHLD, & saWant, (struct sigaction *)0)) {
		fprintf(stderr, "%s: sigaction: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}

	pcOuter = divSelect();
	if (-1 == (sHandle = MakeClientPort(pcOuter))) {
		fprintf(stderr, "%s: %s: cannot establish client connection\n", progname, pcOuter);
		exit(EX_DATAERR);
	}
	/* %% handle command line options %% */

	switch (wInferior = fork()) {
	case -1:
		fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	case 0:
		close(sHandle);
		/* %% we are about to become the inferior process %%
		 * %% what do you need to setup here? %%
		 */
		if (0 != strcmp(acMyDefName, progname)) {
			argv[0] = progname;
		}
		(void)execvp(pcArgv0, argv);
		fprintf(stderr, "%s: execvp: %s: %s\n", progname, argv[0], strerror(errno));
		exit(EX_OSERR);
	default:
		break;
	}

	fInfOpts = 0;
	Death('C');

	/* Tell the master to abandon forever (:) mode, we get back the
	 * number of client connected (ourself included) which we ignore.
	 * If we got back "-No\n", then the master was not in : mode -- ksb
	 */
	if (fQuit) {
		/* %% tell the master to quit on sHandle %% */
	}
	close(sHandle);
	exit(wInfExits);
}

/* Setup the master process and turn it loose				(ksb)
 */
static void
Master(int argc, char **argv)
{
	register const char *pcArgv0, *pcOuterLevel;
	register unsigned uMasterLevel;
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
		uMasterLevel = 1;
	} else {
		uMasterLevel = atoi(pcOuterLevel)+1;
	}

	atexit(CleanUp);
	SafePlace(pcOuterLevel);
	/* %% anyting we need to setup the Master's state machine %% */
	sCtl = Bindings(uMasterLevel);
	if (':' == argv[0][0] && '\000' == argv[0][1] && (char *)0 == argv[1]) {
		wInferior = getpid();
	} else switch (wInferior = fork()) {
	case -1:
		fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		(void)chdir("/");
		exit(EX_OSERR);
	case 0:
		(void)close(sCtl);
		pcArgv0 = argv[0];
		if (0 != strcmp(acMyDefName, progname)) {
			argv[0] = progname;
		}
		(void)execvp(pcArgv0, argv);
		fprintf(stderr, "%s: execvp: %s: %s\n", progname, argv[0], strerror(errno));
		exit(EX_OSERR);
	default:
		break;
	}

	/* %% setup to the master process %% */
	/* We are not really a daemon, but we don't want to be in the way,
	 * so free us from stdin, stdout, and the current working dir.
	 * We keep stderr for messages, but shouldn't spew any.
	 */
	(void)close(0);
	(void)open("/dev/null", O_RDONLY, 0666);
	(void)close(1);
	(void)open("/dev/null", O_WRONLY, 0666);
	(void)chdir(pcMySpace);
	MyMaster(sCtl /* other args */);

	/* %% if you have to tidy stuff (sCtl socket, files) do that here %% */
	(void)chdir("/");
	close(sCtl);

	/* wait for the wrapped process, if it hasn't finished yet
	 */
	fInfOpts = 0;
	Death('E');
	exit(wInfExits);
}
