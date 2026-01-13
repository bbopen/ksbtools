#!mkcmd
# $Id: make.m,v 1.41 2010/08/14 18:20:21 ksb Exp $
from '<sys/types.h>'
from '<sys/wait.h>'
from '<dirent.h>'
from '<fcntl.h>'
from '<stdio.h>'
from '<unistd.h>'

require "util_ppm.m" "util_tmp.m"

from '"machine.h"'

char* 'f' {
	named "pcMakefile"
	init dynamic "(char *)FindMakefile()"
	param "makefile"
	help "search this master source makefile for macros"
}
char* named "pcMakeClean" {
	init "(char *)0"
}
char* 'm' {
	named "pcMakeHook"
	key 1 init {
		'""'		# default to __progname
	}
	before "MakeSetHooks(& %n, & pcMakeClean, %ZK1ev);"
	track "uGaveReq"
	update "MakeSetHooks(& %n, & pcMakeClean, %a);"
	param "prereq"
	help "the prerequisite make target to gather source files"
}
integer named "fShowMake" {
	hidden init "0"
	help "show the shell code to parser the makefile on stderr"
}
type "argv" 'y' {
	named "pPPMMakeOpt"
	# a yoke holds moving parts together, like INTO pairing --ksb
	param "yoke"
	help "force make macro assignments or command-line options"
}

%hi
/* The generic pull macros from a makefile interface
 */
typedef struct MVnode {
	char *pcname;			/* ${name} from the Makefile	*/
	unsigned int flocks;		/* never/always add to this	*/
	unsigned int fsynth;		/* we build from debris left	*/
	unsigned int ucur;		/* number of values found	*/
	/* opaque part */
	PPM_BUF PPMlist;		/* accumulated names		*/
	int afdpipe[2];			/* the result pipe we use	*/
} MAKE_VAR;

/* The specific interface for msrc and mmsrc (makeme2), this is the
 * booty we plunder from the makefile.
 */
typedef struct BOnode {
	char *pcinto;			/* INTO from the -y|makefile	*/
	char *pcmode;			/* MODE from the -y|makefile	*/
	char **ppcsubdir;		/* subdirs from SUBDIR		*/
	char **ppcmap;			/* target names from MAP	*/
	char **ppcorig;			/* source files	from MAP	*/
	char **ppcsend;			/* source files from SEND	*/
	char **ppctarg;			/* target names from SEND	*/
	char **ppcignore;		/* IGNORE + MAP's		*/
	char **ppchxinclude;		/* options to hxmd, viz. db	*/
	unsigned usubdir, umap, uorig, usend, utarg,
		uignore, uhxinclude;	/* counts of the above		*/
	unsigned usplay;		/* count of send != targ elems	*/
} BOOTY;
%%

%i
static char rcs_make[] =
	"$Id: make.m,v 1.41 2010/08/14 18:20:21 ksb Exp $";

static void MakeSetHooks(char **ppcPre, char **ppcPost, char *pcParam);
static char *strRstr(const char *pcBig, const char *pcSmall);
static int InList(char **ppcList, const char *pcValue);
static void AddMVValue(MAKE_VAR *, char *);
static unsigned GetMVCount(MAKE_VAR *);
static char **GetMVValue(MAKE_VAR *);
static void ExecMake(char **ppcMake);
extern int GatherMake(const char *, MAKE_VAR *);
static char *TempMake(const char *);
static void TempNuke(void);
extern int fDebug;
static const char acMakeTempPat[] = "mtfcXXXXXX";
static const char *FindMakefile();
static int Plunder(MAKE_VAR *pMVList, unsigned uList, const char *pcRecipes);
static unsigned Booty(BOOTY *pBO, const char *pcRecipes, FILE *fpShow);

static char
	acMapSuf[] = MAP_SUFFIX,
	acInclSuf[] = INCLUDE_SUFFIX;
static MAKE_VAR aMVNeed[] = {
	{"SEND"},			/* copy these from this directory */
#define DX_SEND		0
	{"INTO"},			/* platform source directory name */
#define DX_INTO		1
	{"IGNORE"},			/* but don't send these, */
#define DX_IGNORE	2
	{"MAP"},			/* s/.host$// and m4 process these */
#define DX_MAP		3
	{"SUBDIR"},			/* mkdir, for user processing */
#define DX_SUBDIR	4
	{"HXINCLUDE"},			/* options to hxmd, viz. -C/-X/-Z */
#define DX_HXINCLUDE	5
	{"MODE"},			/* local|remote|auto (-l local) */
#define DX_MODE		6
	{(char *)0}
#define DX__END		7
};
static const char *apcSkipDir[] = {
	"RCS", "CVS", "SCCS", "SVN",
	(const char *)0
};
#if !defined(MAKE_AUTO_PLUS)
#define MAKE_AUTO_PLUS	"+++"
#endif
static const char acAutoPlus[] =
	MAKE_AUTO_PLUS;			/* explicit add here meta token */
#if !defined(MAKE_AUTO_LOCK)
#define MAKE_AUTO_LOCK	"."
#endif
static const char acNoAuto[] =
	MAKE_AUTO_LOCK;			/* explicit never add meta token */
%%

%c

/* as strrchr is to strchr, this function is to strstr			(ksb)
 */
static char *
strRstr(const char *pcBig, const char *pcSmall)
{
	register const char *pcCur;

	for (pcBig = strstr(pcBig, pcSmall); (char *)0 != pcBig; pcBig = pcCur) {
		if ((char *)0 == (pcCur = strstr(pcBig+1, pcSmall)))
			break;
	}
	return (char *)pcBig;
}

/* The normal C idiom for is this string in the (short) list		(ksb)
 */
static int
InList(char **ppcList, const char *pcValue)
{
	register int cFirst;
	register const char *pc;

	if ((char **)0 == ppcList) {
		return 0;
	}
	for (cFirst = *pcValue; (const char *)0 != (pc = (const char *)*ppcList); ++ppcList) {
		if (cFirst == *pc && 0 == strcmp(pcValue, pc))
			return 1;
	}
	return 0;
}

/* Under -m we can take a "prereq:clean" set of targets, or just	(ksb)
 * the older "prereq" without a clean target (the default).  We are
 * always called before option parsing with "" to set a run-time
 * default value (__+progname).
 * Map the empty strings to the correct values for -m "", -m ":"
 * and -m ":__clearCache" (N.B. if the progname is "clean" you get dups).
 * Easter Egg: we allow "::" to reset the -m option back to defaults.
 */
static void
MakeSetHooks(char **ppcPre, char **ppcPost, char *pcParam)
{
	register char *pcIPre, *pcIPost;

	if ((char *)0 != pcParam && 0 == strcmp("::", pcParam)) {
		pcParam = (char *)0;
	}
	if ((char *)0 == pcParam) {
		uGaveReq = 0;
		*ppcPre = *ppcPost = (char *)0;
	}
	if ((char *)0 == (pcIPre = pcParam)) {
		pcIPost = (char *)0;
	} else if ((char *)0 != (pcIPost = strchr(pcParam, ':'))) {
		*pcIPost++ = '\000';
	}

	/* If we are a reset, or we didnt' have one then form a default
	 * from the current progname, else take what was given.
	 */
	if ((char *)0 == pcIPre || ('\000' == *pcIPre && (char *)0 == *ppcPre)) {
		if ((char *)0 == (*ppcPre = malloc(((strlen(progname)|3)+5)))) {
			fprintf(stderr, "%s: out of memory\n", progname);
			exit(EX_OSERR);
		}
		snprintf(*ppcPre, strlen(progname)+3, "__%s", progname);
	} else if ('\000' != *pcIPre) {
		*ppcPre = pcIPre;
	}

	/* If you didn't give a colon spec, or the one you gave was empty and
	 * we already have a post target, then we are done.
	 * Think about -m ":_cache_clear" -m "_open_bank:", which are in the
	 * wrong order, but should work.
	 */
	if ((char *)0 == pcIPost || ((char *)0 != *ppcPost && '\000' == *pcIPost))
		return;
	*ppcPost = '\000' == *pcIPost ? "__clean" : pcIPost;
}

/* Find the make(1) description file we need to make this work:		(ksb)
 * the first one that we can access(2), or the first that exists.
 * N.B. We are assuming that we are not running setuid/setgid here.
 */
static const char *
FindMakefile()
{
	register unsigned u;
	register const char *pc, *pcAlt;
	auto struct stat stThis;
	static const char *apcLook[] = {
		"Msrc.mk", "msrc.mk", "makefile", "Makefile", (char *)0
	};

	pcAlt = (char *)0;
	for (u = 0; (char *)0 != (pc = apcLook[u]); ++u) {
		if (0 != stat(pc, &stThis) || !S_ISREG(stThis.st_mode)) {
			continue;
		}
		if (0 != access(pc, R_OK)) {
			pcAlt = pc;
			continue;
		}
		return pc;
	}
	return pcAlt;
}

/* Build a temp file in cooperation with $TMPDIR, pcTmpdir util_tmp;	(ksb)
 * reset with (char *)0; return our mkdtemp dir with "".
 */
static char *
TempMake(const char *pcBase)
{
	register int iLen;
	register char *pcRet;
	static char *pcMySpace = (char *)0;

	if ((char *)0 == pcBase) {
		if ((char *)0 != pcMySpace) {
			pcMySpace = (char *)0;
		}
		return (char *)0;
	}
	if ('\000' == pcBase[0]) {
		return pcMySpace;
	}
	if ((char *)0 == pcMySpace) {
		iLen = strlen(pcTmpdir)+1+sizeof(acMakeTempPat)+1;
		iLen |= 15;
		if ((char *)0 == (pcRet = malloc(iLen+1))) {
			return (char *)0;
		}
#if HAVE_SNPRINTF
		snprintf(pcRet, iLen, "%s/%s", pcTmpdir, acMakeTempPat);
#else
		sprintf(pcRet, "%s/%s", pcTmpdir, acMakeTempPat);
#endif
		pcMySpace = mkdtemp(pcRet);
		atexit(TempNuke);
	}
	if ((char *)0 == pcMySpace) {
		return (char *)0;
	}
	iLen = strlen(pcMySpace)+1+strlen(pcBase)+1;
	iLen |= 15;
	if ((char *)0 == (pcRet = malloc(iLen+1))) {
		return (char *)0;
	}
#if HAVE_SNPRINTF
	snprintf(pcRet, iLen, "%s/%s", pcMySpace, pcBase);
#else
	sprintf(pcRet, "%s/%s", pcMySpace, pcBase);
#endif
	return pcRet;
}

/* Get rid of all our /tmp space, please (atexit)			(ksb)
 */
static void
TempNuke()
{
	register char *pcMySpace;
	register DIR *pDR;
	register struct dirent *pDI;
	register int fCarp;
	auto struct stat stDir;
	auto char acRm[MAXPATHLEN+4];

	if ((char *)0 == (pcMySpace = TempMake(""))) {
		return;
	}
	fCarp = 1;
	if ((DIR *)0 != (pDR = opendir(pcMySpace))) {
		while ((struct dirent *)0 != (pDI = readdir(pDR))) {
			if ('.' == pDI->d_name[0] && ('\000' == pDI->d_name[1] || ('.' == pDI->d_name[1] && '\000' == pDI->d_name[2]))) {
				continue;
			}
#if HAVE_SNPRINTF
			snprintf(acRm, sizeof(acRm), "%s/%s", pcMySpace, pDI->d_name);
#else
			sprintf(acRm, "%s/%s", pcMySpace, pDI->d_name);
#endif
			if (-1 == stat(acRm, & stDir) || !S_ISDIR(stDir.st_mode)) {
				if (-1 == unlink(acRm) && fCarp) {
					fprintf(stderr, "%s: unlink: %s: %s\n", progname, acRm, strerror(errno));
					fCarp = 0;
				}
				continue;
			}
			if (-1 == rmdir(acRm)) {
				fprintf(stderr, "%s: rmdir: %s: %s\n", progname, acRm, strerror(errno));
				fCarp = 0;
			}
		}
		closedir(pDR);
	}
	if (fCarp) {
		(void)rmdir(pcMySpace);
	}
	(void)TempMake((char *)0);
}

/* Add the value to the macro's list of values				(ksb)
 */
static void
AddMVValue(MAKE_VAR *pMVThis, char *pcValue)
{
	register char **ppcList;
	register unsigned u;

	u = pMVThis->ucur++;
	if ((char **)0 == (ppcList = (char **)util_ppm_size(& pMVThis->PPMlist, pMVThis->ucur+1))) {
		fprintf(stderr, "%s: malloc: out of memory\n", progname);
		exit(EX_OSERR);
	}
	ppcList[u] = pcValue;
	ppcList[u+1] = (char *)0;
}

/* Read this single macro's values into the PPM bound to it		(ksb)
 * We can use stdio here because we're going to close (fclose) the fd.
 */
static int
AquireMacro(MAKE_VAR *pMVThis)
{
	register FILE *fp;
	register char *pcLine, *pcToken;
	register int iLast;
	auto size_t iSize;

	if ((FILE *)0 == (fp = fdopen(pMVThis->afdpipe[0], "rb"))) {
		return -1;
	}
	pMVThis->ucur = 0;
	iLast = 0;
	while ((char *)0 != (pcLine = fgetln(fp, & iSize))) {
		for (pcToken = (char *)0; iSize > 0; --iSize, ++pcLine) {
			if (isspace(*pcLine)) {
				if ((char *)0 != pcToken) {
					*pcLine = '\000';
					AddMVValue(pMVThis, strdup(pcToken));
					pcToken = (char *)0;
				}
				continue;
			}
			if ((char *)0 != pcToken) {
				continue;
			}
			pcToken = pcLine;
			iLast = iSize;
		}
		if ((char *)0 == pcToken) {
			continue;
		}
		pcLine = malloc((iLast|7)+1);
		strncpy(pcLine, pcToken, iLast);
		pcLine[iLast] = '\000';
		AddMVValue(pMVThis, pcLine);
	}
	return fclose(fp);
}

/* "Make it happen"							(ksb)
 * When make is not in our search path ($PATH) try some internal paths.
 */
static void
ExecMake(char **ppcMake)
{
	register char **ppcCast;
	register int i;

	ppcCast = (char **)ppcMake;
	if (fDebug) {
		fprintf(stderr, "%s:", progname);
		for (i = 0; (char *)0 != ppcCast[i]; ++i) {
			fprintf(stderr, " %s", ppcCast[i]);
		}
		fprintf(stderr, "\n");
		fflush(stderr);
	}
	execvp(PATH_MAKE, ppcCast);
	execvp("/usr/bin/make", ppcCast);
	execvp("/usr/ccs/bin/make", ppcCast);
	execvp("/usr/local/bin/make", ppcCast);
	fprintf(stderr, "%s: execvp: make: %s\n", progname, strerror(errno));
	exit(EX_OSFILE);
}

/* Output the new stanza in the make(1) makefile			(ksb)
 * We allow the makefile to force a depend on the hook for prep work.
 */
static void
MakeServo(int fdOut, MAKE_VAR *pMV, int iCount, const char *pcHook)
{
	register int i;
	auto char acCvt[32];	/* 32 character is a long number here */
	static char acEcho[] = "\techo ${";

	write(fdOut, "\n", 1);
	write(fdOut, pcHook, strlen(pcHook));
	write(fdOut, ":\n", 2);
	for (i = 0; i < iCount; ++i) {
		write(fdOut, acEcho, sizeof(acEcho)-1);
#if HAVE_SNPRINTF
		snprintf(acCvt, sizeof(acCvt), "} 1>&%d\n", pMV[i].afdpipe[1]);
#else
		sprintf(acCvt, "} 1>&%d\n", pMV[i].afdpipe[1]);
#endif
		write(fdOut, pMV[i].pcname, strlen(pMV[i].pcname));
		write(fdOut, acCvt, strlen(acCvt));
	}
	fsync(fdOut);
}

/* Move the read side (only) up above 10 so we can fit more macros	(ksb)
 * in each itteraion of the make process.  Reset the machine with a NULL.
 */
static int
MoveReadUp(int *piFd)
{
	static char iLow = 11;
	register int fdWas;
	auto int iDummy;

	if ((int *)0 == piFd) {
		iLow = 11;
		return 0;
	}
	if (10 < (fdWas = *piFd)) {
		return 0;
	}
	while (-1 != fcntl(iLow, F_GETFD, (void *)&iDummy)) {
		++iLow;
	}
	if (errno == EBADF && -1 != dup2(fdWas, iLow)) {
#if DEBUG
		if (fDebug)
			fprintf(stderr, "moved %d to %d\n", fdWas, iLow);
#endif
		*piFd = iLow;
		++iLow;
		return close(fdWas);
	}
	/* we can't, sigh */
	return 0;
}

/* Get a make(1) rolling to plunder the given make variables.		(ksb)
 * Then read the results and return a success(0) or failure(-1).
 * Called in cleanup mode (when pcFile = (char *)0) we remove our tempfile.
 */
static int
LaunchMake(const char *pcFile, MAKE_VAR *pMV, int iCount)
{
	static char *pcTemp = (char *)0;
	static int fdTemp = -1;
	auto char acBuffer[16*1024];
	register int iCc, iChunk, iVar;
	register char *pcOut, **ppcYoke;
	auto pid_t wMake, wStatus;
	auto int iStatus, fdSource;
	auto char **ppcMake;

	if ((char *)0 == pcFile) {
		if (-1 != fdTemp) {
			close(fdTemp);
			fdTemp = -1;
		}
		if ((char *)0 != pcTemp) {
			(void)unlink(pcTemp);
			free((void *)pcTemp);
			pcTemp = (char *)0;
		}
		return EX_OK;
	}

	/* Copy the file to out temp file, we could even use mmap here -- ksb
	 */
	if (-1 == (fdSource = open(pcFile, O_RDONLY, 0444))) {
		fprintf(stderr, "%s: open: %s: %s\n", progname, pcFile, strerror(errno));
		exit(EX_NOINPUT);
	}
	if (-1 == MoveReadUp(& fdSource)) {
		fprintf(stderr, "%s: dup: %d: %s\n", progname, fdSource, strerror(errno));
		return EX_OSERR;
	}

	if ((char *)0 == pcTemp && (char *)0 == (pcTemp = TempMake("makefile"))) {
		fprintf(stderr, "%s: tempfile: %s\n", progname, strerror(errno));
		return EX_OSERR;
	}
	if (-1 == fdTemp && -1 == (fdTemp = open(pcTemp, O_RDWR|O_CREAT, 0666))) {
		fprintf(stderr, "%s: open: %s: %s\n", progname, pcTemp, strerror(errno));
		return EX_OSERR;
	}
	if (-1 == MoveReadUp(& fdTemp)) {
		fprintf(stderr, "%s: dup: %d: %s\n", progname, fdTemp, strerror(errno));
		return EX_OSERR;
	}

	if ((off_t)0 != lseek(fdTemp, (off_t)0, SEEK_SET)) {
		fprintf(stderr, "%s: lseek: %d: %s\n", progname, fdTemp, strerror(errno));
		exit(EX_OSERR);
	}
	if (-1 == ftruncate(fdTemp, (off_t) 0)) {
		fprintf(stderr, "%s: ftruncate: %d: %s\n", progname, fdTemp, strerror(errno));
		exit(EX_OSERR);
	}
	iCc = 0;
	while (0 < (iChunk = read(fdSource, acBuffer, sizeof(acBuffer)))) {
		pcOut = acBuffer;
		while (0 < iChunk && 0 <= (iCc = write(fdTemp, pcOut, iChunk)))
			pcOut += iCc, iChunk -= iCc;
	}
	if (-1 == iCc) {
		fprintf(stderr, "%s: write: %s(%d): %s\n", progname, pcTemp, fdSource, strerror(errno));
		(void)close(fdSource);
		return EX_OSERR;
	}
	(void)close(fdSource);

	/* generate the pipe connection target and shell code
	 */
	MakeServo(fdTemp, pMV, iCount, pcMakeHook);
	if (fShowMake) {
		fprintf(stderr, "%s: cat %s - <<\\! >%s\n", progname, pcFile, pcTemp);
		fflush(stderr);
		MakeServo(2, pMV, iCount, pcMakeHook);
		fprintf(stderr, "!\n");
	}

	/* Fork the make -f $pcTemp __hook
	 */
	fflush(stdout);
	fflush(stderr);
	switch (wMake = fork()) {
	case -1:
		fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
		/*NOTREACHED*/
	case 0:
		close(fdTemp);
		for (iVar = 0; iVar < iCount; ++iVar) {
			close(pMV[iVar].afdpipe[0]);	/* we don't read */
		}
		iVar = 0;
		if ((char **)0 == (ppcYoke = (char **)util_ppm_size(pPPMMakeOpt, 0))) {
			while ((char *)0 != ppcYoke[iVar])
				++iVar;
		}
		/* make -s -f makefile <yoke> __hook (char *)0 */
		if ((char **)0 == (ppcMake = (char **)calloc(((4+iVar+2)|7)+1, sizeof(char *)))) {
			fprintf(stderr, "%s: calloc: %s\n", progname, strerror(errno));
			exit(EX_OSERR);
		}
		iCc = 0;
		ppcMake[iCc++] = "make";
		ppcMake[iCc++] = "-s";		/* should be able to defeat */
		ppcMake[iCc++] = "-f";
		ppcMake[iCc++] = pcTemp;
		for (iVar = 0; (char **)0 != ppcYoke && (char *)0 != ppcYoke[iVar]; ++iVar) {
			ppcMake[iCc++] = ppcYoke[iVar];
		}
		ppcMake[iCc++] = pcMakeHook;
		ppcMake[iCc++] = (char *)0;

		ExecMake(ppcMake);
		/*NOTREACHED*/
	default:
		for (iVar = 0; iVar < iCount; ++iVar) {
			close(pMV[iVar].afdpipe[1]);	/* we don't write */
		}
		break;
	}
	/* Read his data from each pipe into the buffers provided then close,
	 * and wait for the make process
	 */
	for (iVar = 0; iVar < iCount; ++iVar) {
		(void)AquireMacro(& pMV[iVar]);
	}
	iStatus = EX_UNAVAILABLE << 8;
	while (-1 != (wStatus = wait((void *)& iStatus)) || EINTR == errno) {
		if (wStatus == wMake)
			break;
	}
	return WEXITSTATUS(iStatus);
}


/* Fetch the value of the given variables from the Makefile		(ksb)
 * We assume they are lists of space separated words without quotes.
 * We are going to give them to rdist or rsync or cp, so they should be
 * well-behaved (else those will crash/fail).
 * A (char *) sentinal marks the end of the list, by convention the
 * given make(1) file should not use targets that match m/^__[a-z]*$/ for
 * normal stuff, as we use those.
 *
 * We can only use (at most) fd's 3-9 (7 of them) at a time, as the shell
 * won't dup any two digit descriptors.  Sigh.  Well just fork more make
 * processes until they are all satisfied.
 */
int
GatherMake(const char *pcFrom, MAKE_VAR *pMV)
{
	register unsigned int u, uBase;
	register int iRet;

	iRet = EX_OK;
	if ((MAKE_VAR *)0 == pMV || (char *)0 == pMV->pcname) {
		return iRet;
	}
	(void)MoveReadUp((int *)0);
	for (uBase = u = 0; (char *)0 != pMV[u].pcname; /* humm */) {
		if (-1 == pipe(pMV[u].afdpipe)) {
			/* no more pipes, try to flush to free fds */
		} else if (10 <= pMV[u].afdpipe[1] && 10 <= pMV[u].afdpipe[0]) {
			/* oops both sides >= 10, free some lower numbers */
			close(pMV[u].afdpipe[0]);
			close(pMV[u].afdpipe[1]);
		} else if (-1 == MoveReadUp(& pMV[u].afdpipe[0])) {
			/* fatal error on close of read-side */
			close(pMV[u].afdpipe[1]);
			close(pMV[u].afdpipe[0]);
		} else {
			/* move the write down if we must */
			if (10 <= pMV[u].afdpipe[1]) {
				register int iT;
				iT = pMV[u].afdpipe[1];
				pMV[u].afdpipe[1] = dup(iT);
				close(iT);
			}
			/* we queue'd it, yeah */
			if (-1 != pMV[u].afdpipe[1]) {
				util_ppm_init(& pMV[u].PPMlist, sizeof(char *), 0);
				++u;
				continue;
			}
			close(pMV[u].afdpipe[0]);
		}

		/* When we can't even get one going, use stdout?
		 * I don't think we can even use stdout, at least no with 100%
		 * certainty that the Customer's tools chain doesn't use it
		 * as a resource to get the file list (viz. prompts). --ksb
		 */
		if (uBase == u) {
			fprintf(stderr, "%s: make -f %s: pipe: no fd < 10 available\n", progname, pcFrom);
			return -1;
		}
		if (EX_OK != (iRet = LaunchMake(pcFrom, pMV+uBase, u-uBase))) {
			return iRet;
		}
		uBase = u;
	}
	if (uBase < u && EX_OK != (iRet = LaunchMake(pcFrom, pMV+uBase, u-uBase))) {
		return iRet;
	}

	/* cleanup and return our victory
	 */
	return LaunchMake((char *)0, pMV, u);
}

/* Accessor method for the list we got					(ksb)
 */
static char **
GetMVValue(MAKE_VAR *pMVThis)
{
	return (char **)util_ppm_size(& pMVThis->PPMlist, 0);
}

/* If token "+++" is part of the make variable allow adds to it		(ksb)
 * (and remove the token).  Use "./+++" to send a file name "+++".
 * The opposite is "." which means don't add any (more) to this macro.
 * Return bit-0 for open ended, bit-1 for closed, no bits for unspecified.
 */
static int
OpenEnded(MAKE_VAR *pMVThis)
{
	register char *pc, **ppcIn, **ppcOut;
	register int iRet;

	iRet = 0;
	if ((char **)0 == (ppcOut = GetMVValue(pMVThis))) {
		return iRet;
	}
	for (ppcIn = ppcOut; (char *)0 != (pc = *ppcIn); ++ppcIn) {
		if (0 == strcmp(acAutoPlus, pc))
			--pMVThis->ucur, iRet |= 1;
		else if (0 == strcmp(acNoAuto, pc))
			--pMVThis->ucur, iRet |= 2;
		else
			*ppcOut++ = pc;
	}
	*ppcOut = (char *)0;
	return iRet;
}

/* Accessor method for the number of words we saw in the value		(ksb)
 */
static unsigned
GetMVCount(MAKE_VAR *pMVThis)
{
	return pMVThis->ucur;
}

/* Look for the variables we need from the Makefile given		(ksb)
 * The files must be in ".", we are $PWD context sensitive like make(1)
 */
static int
Plunder(MAKE_VAR *pMVList, unsigned uList, const char *pcRecipes)
{
	register unsigned i;
	register struct dirent *pDE;
	register DIR *pDRDot;
	register const char *pcTail;
	register int iCode;
	register char **ppcTrim, *pcSnip;
	auto struct stat stThis;

	if ((MAKE_VAR *)0 == pMVList) {
		pMVList = aMVNeed;
		uList = (sizeof(aMVNeed)/sizeof(aMVNeed[0]))-1;
	}
	/* Sigh, make exits "1" for any nonzero exit from a recipe, which
	 * is not a useful exit code for me -- ksb
	 */
	if (EX_OK != (iCode = GatherMake(pcRecipes, pMVList))) {
		fprintf(stderr, "%s: %s: make failed with exit code %d\n", progname, pcRecipes, iCode);
		return EX_NOINPUT;
	}
	/* If you set SUBDIR (MAP, HXINCLUDE) to "." we take that to mean
	 * you have none, all the dirs in here are the opposite kind dirs.
	 * Empty macros imply auto selection, or macros with the open-ended
	 * token "+++".  This could remove a lone dot from a messaged INTO,
	 * that's why we can put an underscore before it.
	 */
	for (i = 0; i < uList; ++i) {
		pMVList[i].flocks = OpenEnded(& pMVList[i]);
		if (2 & pMVList[i].flocks)
			pMVList[i].fsynth = 0;
		else if ((1 & pMVList[i].flocks) || 0 == GetMVCount(& pMVList[i]))
			pMVList[i].fsynth = 1;
	}
	/* The MAP macro can have directory entries now -- hxmd does it,
	 * so we must remove any training "/" from the names of dirs in MAP
	 * or the SUBDIR code will add them redundantly.	(ksb)
	 */
	ppcTrim = GetMVValue(& pMVList[DX_MAP]);
	for (i = GetMVCount(& pMVList[DX_MAP]); i-- > 0; ) {
		while ((char *)0 != (pcSnip = strrchr(ppcTrim[i], '/')) && '\000' == pcSnip[1])
			*pcSnip = '\000';
	}
	if ((DIR *)0 == (pDRDot = opendir("."))) {
		fprintf(stderr, "%s: opendir: .: %s\n", progname, strerror(errno));
		return EX_NOPERM;
	}

	/* Disposition of each file made easy here -- ksb
	 * skip . and .., of course
	 * if the node is in one of (MAP, SEND, IGNORE) skip it
	 * if -d node
	 *	Iff named RCS, SCCS, SVN, CVS skip it
	 *	Iff no SUBDIR, then [-d] (dirs) -> SUBDIR
	 *	continue
	 * if ! -f put
	 *	-> IGNORE
	 *	continue
	 * if none in MAP, then *.host -> MAP (else fall into SEND)
	 * if none in SEND and -f || -L (files or symlink) -> SEND
	 */
	while ((struct dirent *)0 != (pDE = readdir(pDRDot))) {
		if ('.' == pDE->d_name[0] && ('\000' == pDE->d_name[1] || ('.' == pDE->d_name[1] && '\000' == pDE->d_name[2]))) {
			continue;
		}
		if (-1 == stat(pDE->d_name, & stThis)) {
			fprintf(stderr, "%s: stat: %s: %s\n", progname, pDE->d_name, strerror(errno));
			return EX_NOPERM;
		}
		if (InList(GetMVValue(& pMVList[DX_MAP]), pDE->d_name) || InList(GetMVValue(& pMVList[DX_SEND]), pDE->d_name) || InList(GetMVValue(& pMVList[DX_IGNORE]), pDE->d_name)) {
			continue;
		}

		/* Any directory we find that is not a revision control one
		 * we try to add to SUBDIR or MAP, if we are building each.
		 * MAP gets it if it ends in ".host", just like a file.
		 */
		if (S_ISDIR(stThis.st_mode)) {
			register char *pcCheck;
			for (i = 0; (char *)0 != apcSkipDir[i]; ++i)
				if (0 == strcmp(apcSkipDir[i], pDE->d_name))
					break;
			if ((char *)0 != apcSkipDir[i])
				continue;
			if (InList(GetMVValue(& pMVList[DX_SUBDIR]), pDE->d_name))
				continue;

			if (0 != pMVList[DX_MAP].fsynth && (char *)0 != (pcCheck = strRstr(pDE->d_name, acMapSuf)) && pDE->d_name != pcCheck && '\000' == pcCheck[sizeof(acMapSuf)-1])
				AddMVValue(& pMVList[DX_MAP], strdup(pDE->d_name));
			else if (0 != pMVList[DX_SUBDIR].fsynth)
				AddMVValue(& pMVList[DX_SUBDIR], strdup(pDE->d_name));
			else if (0 != pMVList[DX_IGNORE].fsynth)
				AddMVValue(& pMVList[DX_IGNORE], strdup(pDE->d_name));
			continue;
		}
		if (!S_ISREG(stThis.st_mode)) {
			AddMVValue(& pMVList[DX_IGNORE], strdup(pDE->d_name));
			continue;
		}

		if (0 != pMVList[DX_MAP].fsynth && (const char *)0 != (pcTail = strRstr(pDE->d_name, acMapSuf)) && pDE->d_name != pcTail && '\000' == pcTail[sizeof(acMapSuf)-1]) {
			AddMVValue(& pMVList[DX_MAP], strdup(pDE->d_name));
			continue;
		}
		if (0 != pMVList[DX_HXINCLUDE].fsynth && (const char *)0 != (pcTail = strRstr(pDE->d_name, acInclSuf)) && '\000' == pcTail[sizeof(acInclSuf)-1]) {
			AddMVValue(& pMVList[DX_HXINCLUDE], strdup(pDE->d_name));
			continue;
		}
		if (0 != pMVList[DX_SEND].fsynth) {
			AddMVValue(& pMVList[DX_SEND], strdup(pDE->d_name));
			continue;
		}
		if (0 != pMVList[DX_IGNORE].fsynth) {
			AddMVValue(& pMVList[DX_IGNORE], strdup(pDE->d_name));
			continue;
		}
		/* If you locked SEND, INCLUDE, and IGNORE we have to map it
		 * (so do not lock IGNORE).
		 */
		if (0 != pMVList[DX_MAP].fsynth) {
			AddMVValue(& pMVList[DX_MAP], strdup(pDE->d_name));
			continue;
		}
	}
	closedir(pDRDot);

	/* Iff no INTO, then try to match $PWD =~ s!/msrc/!/src/!o
	 */
	if (0 != pMVList[DX_INTO].fsynth) {
		register char *pcMem, *pcCut;

		if ((char *)0 == (pcMem = malloc(MAXPATHLEN+4))) {
			fprintf(stderr, "%s: malloc: %d: %s\n", progname, MAXPATHLEN+3, strerror(errno));
			return EX_OSERR;
		}
		if ((char *)0 == (pcMem = getcwd(pcMem, MAXPATHLEN+4))) {
			fprintf(stderr, "%s: getcwd: %s\n", progname, strerror(errno));
			return EX_OSERR;
		}
		if ((char *)0 == (pcCut = strstr(pcMem, "/msrc/"))) {
			fprintf(stderr, "%s: %s: cannot deduce \"INTO\" not under \"msrc\": %s\n", progname, pcRecipes, pcMem);
			return EX_DATAERR;
		}
		memmove(pcCut+1, pcCut+2, strlen(pcCut+1));
		AddMVValue(& pMVList[DX_INTO], pcMem);
	}
	/* When into has more than one value we are baked --ksb
	 * Easter eggs are us, when INTO="_reason _for _botch..."
	 */
	if (1 != (i = GetMVCount(& pMVList[DX_INTO]))) {
		register char **ppcEgg, *pcText;

		if (1 > i || (char **)0 == (ppcEgg = GetMVValue(& pMVList[DX_INTO])) || '_' != ppcEgg[0][0]) {
			fprintf(stderr, "%s: %s: \"INTO\" should have exactly one value, not %d\n", progname, pcRecipes, i);
		} else {
			fprintf(stderr, "%s: %s:", progname, pcRecipes);
			while ((char *)0 != (pcText = *ppcEgg++)) {
				if ('_' == *pcText)
					++pcText;
				fprintf(stderr, " %s", pcText);
			}
			fprintf(stderr, "\n");
		}
		return EX_DATAERR;
	}
	return 0;
}

/* Get the data form the makefile and condition it a little:		(ksb)
 * we add the MAP files mapped name to the IGNORE list.  Both makeme
 * and msrc itself need this common data.
 */
static unsigned
Booty(BOOTY *pBO, const char *pcRecipes, FILE *fpShow)
{
	static char acRsyncHack[] = "/./";
	register unsigned i;
	register char **ppc;
	register char *pcTail, *pcOrig;

	if (EX_OK != (i = Plunder(aMVNeed, (sizeof(aMVNeed)/sizeof(aMVNeed[0]))-1, pcRecipes))) {
		exit(i);
	}
	if ((char **)0 == (ppc = GetMVValue(& aMVNeed[DX_INTO])) || (char *)0 == *ppc) {
		fprintf(stderr, "%s: %s: INTO: no value found\n", progname, pcRecipes);
		exit(EX_CONFIG);
	}
	pBO->pcinto = *ppc;
	if ((char **)0 == (ppc = GetMVValue(& aMVNeed[DX_MODE])) || (char *)0 == *ppc || 0 == (i = GetMVCount(& aMVNeed[DX_MODE]))) {
		AddMVValue(& aMVNeed[DX_MODE], "auto");
		aMVNeed[DX_MODE].fsynth = 1;
		ppc = GetMVValue(& aMVNeed[DX_MODE]);
		i = GetMVCount(& aMVNeed[DX_MODE]);
	}
	if (1 != i) {
		fprintf(stderr, "%s: %s: MODE: too many values (%u)\n", progname, pcRecipes, i);
		exit(EX_CONFIG);
	} else {
		pBO->pcmode = *ppc;
	}
	pBO->ppcorig = GetMVValue(& aMVNeed[DX_MAP]);
	pBO->umap = pBO->uorig = GetMVCount(& aMVNeed[DX_MAP]);
	pBO->ppcsubdir = GetMVValue(& aMVNeed[DX_SUBDIR]);
	pBO->usubdir = GetMVCount(& aMVNeed[DX_SUBDIR]);
	if ((char **)0 == (pBO->ppcmap = (char **)calloc((pBO->umap|7)+1, sizeof(char *)))) {
		fprintf(stderr, "%s: calloc: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	/* Build a target/map path vector from orig/send, we honor the rsync
	 * "/./" markup _or_ remove any "../" prefix from the source path. (ksb)
	 */
	for (i = 0; i < pBO->umap; ++i) {
		pcOrig = pBO->ppcorig[i];
		if ((char *)0 != (pcTail = strRstr(pcOrig, acRsyncHack))) {
			pcOrig = pcTail + (sizeof(acRsyncHack)-1);
			while ('/' == *pcOrig)
				++pcOrig;
		} else {
			while ('.' == pcOrig[0] && '.' == pcOrig[1] && '/' == pcOrig[2]) {
				pcOrig += 2;
				while ('/' == *pcOrig)
					++pcOrig;
			}
		}
		pBO->ppcmap[i] = strdup(pcOrig);
		if ((char *)0 != (pcTail = strRstr(pBO->ppcmap[i], acMapSuf)) && '\000' == pcTail[sizeof(acMapSuf)-1])
			*pcTail = '\000';
		if (InList(GetMVValue(& aMVNeed[DX_IGNORE]), pBO->ppcmap[i]))
			continue;
		AddMVValue(& aMVNeed[DX_IGNORE], pBO->ppcmap[i]);
	}
	pBO->ppcsend = GetMVValue(& aMVNeed[DX_SEND]);
	pBO->utarg = pBO->usend = GetMVCount(& aMVNeed[DX_SEND]);
	if ((char **)0 == (pBO->ppctarg = (char **)calloc((pBO->usend|7)+1, sizeof(char *)))) {
		fprintf(stderr, "%s: calloc: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	pBO->usplay = 0;
	for (i = 0; i < pBO->usend; ++i) {
		pcOrig = pBO->ppcsend[i];
		if ((char *)0 != (pcTail = strRstr(pcOrig, acRsyncHack))) {
			++pBO->usplay;
			pcOrig = pcTail + (sizeof(acRsyncHack)-1);
			while ('/' == *pcOrig)
				++pcOrig;
		} else {
			while ('.' == pcOrig[0] && '.' == pcOrig[1] && '/' == pcOrig[2]) {
				pcOrig += 2;
				while ('/' == *pcOrig)
					++pcOrig;
			}
		}
		pBO->ppctarg[i] = pcOrig;
	}
	pBO->ppcignore = GetMVValue(& aMVNeed[DX_IGNORE]);
	pBO->uignore = GetMVCount(& aMVNeed[DX_IGNORE]);
	pBO->ppchxinclude = GetMVValue(& aMVNeed[DX_HXINCLUDE]);
	pBO->uhxinclude = GetMVCount(& aMVNeed[DX_HXINCLUDE]);

	for (i = 0; (FILE *)0 != fpShow && i < (sizeof(aMVNeed)/sizeof(aMVNeed[0]))-1; ++i) {
		if ((char **)0 == (ppc = GetMVValue(& aMVNeed[i]))) {
			fprintf(fpShow, "%s: [empty]\n", aMVNeed[i].pcname);
			continue;
		}
		fprintf(fpShow, "%s: [%s]\n", aMVNeed[i].pcname, (2 & aMVNeed[i].flocks) ? "make locked" : (1 & aMVNeed[i].flocks) ? "make open" : 0 == aMVNeed[i].fsynth ? "make" : "generated");
		for (/* ppc */; (char *)0 != *ppc; ++ppc) {
			fprintf(fpShow, "\t%s\n", *ppc);
		}
	}
	return 0;
}
%%
