# $Id: slot.m,v 1.15 2010/08/04 23:08:06 ksb Exp $
# The "slot" abstraction is a vector of catenated strings		(ksb)
# each of which represents the path to a temp file.  The list is
# terminated by an empty string.  Some number of the slots file
# are taken as "literals", which means when we print the slot we
# output the contents of the file, rather than the name of the file.
# The number of literals is the same for every slot and is only needed
# by "PrintSlot".
from '<sys/types.h>'
from '<sys/signal.h>'

# We live with "hostdb.m" as a prerequisite, I think, for RecurBanner
# and maybe some others.

%i
static char rcs_slot[] =
	"$Id: slot.m,v 1.15 2010/08/04 23:08:06 ksb Exp $";
static const char acDownRecipe[] =
	"Cache.m4";

static int InitSlot(char *, char *, char **, const char *);
static void PrintSlot(char *pcSlot, unsigned uLeft, unsigned uRight, int fdOut);
static int FillSlot(char *pcSlot, char **argv, char **ppcM4Argv, META_HOST *pMHThis, const int *piIsCache, const char *pcCacheCtl);
static void SlotDefs(FILE *fpOut, char *pcSlot);
static void CleanSlot(char *pcSlot);
static char *SetupPhase(const char *pcMnemonic, size_t wPad);
static const char *SetCurPhase(const char *pcNow);

static FILE *M4Gadget(const char *pcOut, struct stat *pstLike, pid_t *pwM4, char **ppcM4Argv);
static int MakeGadget(const char *pcOut, struct stat *pstLike, const char *pcRecipe, char *pcTarget, const char *pcDir);

static size_t
	wPhaseLen = 0;
static char
	*pcM4Phase = (char *)0;	/* wPhaseLen character for the m4 pass info */
%%

%c
/* Construct a new slot, return the length				(ksb)
 * build the directory and the empty files to over-write.
 * We know the called allocated space for all the files we need to make.
 * Build "temp[1]\000temp[2]\000...\000\000", we don't need a "0"
 * return the length up to (not including) the last literal \000
 * We can write this to xapply -fz (via xclate) to run this program.
 */
static int
InitSlot(char *pcMkTmp, char *pcSlot, char **argv, const char *pcTmpSpace)
{
	register unsigned i, fd;
	register char *pc, *pcTail, *pcCursor;
	auto char acEmpty[MAXPATHLEN+1];

#if DEBUG
	fprintf(stderr, "%s: initSlot %p\n", progname, pcSlot);
#endif
	pcCursor = pcSlot;
	*pcCursor = '\000';
	acEmpty[0] = '\000';
	acEmpty[MAXPATHLEN] = '\000';
	for (i = 0; (char *)0 != (pc = argv[i]); /*nada*/) {
		if ('\000' == acEmpty[0]) {
			snprintf(acEmpty, MAXPATHLEN, "%s/%s", pcTmpSpace, pcMkTmp);
			if ((char *)0 == mkdtemp(acEmpty)) {
				fprintf(stderr, "%s: mkdtemp: %s: %s\n", progname, acEmpty, strerror(errno));
				exit(EX_CANTCREAT);
			}
		}
		if ((char *)0 == (pcTail = strrchr(pc, '/'))) {
			pcTail = pc;
		} else {
			++pcTail;
		}
		snprintf(pcCursor, MAXPATHLEN, "%s/%s", acEmpty, pcTail);
		fd = open(pcCursor, O_WRONLY|O_CREAT|O_EXCL, 0600);
		/* Two params with same tail, build another direcory
		 * for the next one
		 */
		if (-1 == fd && EEXIST == errno) {
			acEmpty[0] = '\000';
			continue;
		}
		close(fd);
		pcCursor += strlen(pcCursor);
		*++pcCursor = '\000';
		++i;
	}
	return pcCursor - pcSlot;
}

/* Remove the junk we built for the temporary files for a slot		(ksb)
 */
static void
CleanSlot(char *pcSlot)
{
	register char *pcSlash;

	if ((char *)0 == pcSlot) {
		return;
	}
#if DEBUG
	fprintf(stderr, "%s: clean slot %s\n", progname, pcSlot);
#endif
	while ('\000' != *pcSlot) {
		if ((char *)0 == (pcSlash = strrchr(pcSlot, '/')))
			break;
		if ((char *)0 != strchr(pcDebug, 'R')) {
			fprintf(stderr, "%s: rm -f %s\n", progname, pcSlot);
		}
		(void)unlink(pcSlot);
		*pcSlash = '\000';
		if (0 == rmdir(pcSlot) && (char *)0 != strchr(pcDebug, 'R')) {
			fprintf(stderr, "%s: rmdir %s\n", progname, pcSlot);
		}
		*pcSlash = '/';
		pcSlot = pcSlash+strlen(pcSlash)+1;
	}
}

/* Let each file know where the other files are (or will be).		(ksb)
 * This lets Distfiles know where any processed *.host is, for example.
 * It does depend on presented order in argv, like any other program.
 */
static void
SlotDefs(FILE *fpOut, char *pcSlot)
{
	register unsigned uCount;

	for (uCount = 0; '\000' != *pcSlot; ++uCount) {
		fprintf(fpOut, "define(`%s%u',`%s')", pcXM4Define, uCount, pcSlot);
		pcSlot += strlen(pcSlot)+1;
	}
	fprintf(fpOut, "define(`%sC',`%u')dnl\n", pcXM4Define, uCount);
	RecurBanner(fpOut);
}


/* Build the -D spec for the m4 line, hold the pointer to the value	(ksb)
 */
static char *
SetupPhase(const char *pcMnemonic, size_t wPad)
{
	register char *pcRet;
	register int i;

	if (64 < wPad) {
		wPad = 64;
	}
	/* round(len("HXMD_") +      len("PHASE") + '='   )  )+ 64c */
	i = ((strlen(pcXM4Define)+strlen(pcMnemonic)+1)|63)+1 + wPad;
	if ((char *)0 == (pcRet = malloc(i))) {
		fprintf(stderr, "%s: malloc: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	snprintf(pcRet, i, "%s%s=", pcXM4Define, pcMnemonic);
	pcM4Phase = pcRet+strlen(pcRet);
	wPhaseLen = wPad;
	return pcRet;
}

/* Tell the -j files where the output is bound for the gadgets,		(ksb)
 * used outside of this file to force a phase name, we set the numbers.
 */
static const char *
SetCurPhase(const char *pcNow)
{
	if ((char *)0 == pcM4Phase)
		return (char *)0;
	if ((char *)0 == pcNow)
		pcNow = "";
	return strncpy(pcM4Phase, pcNow, wPhaseLen);
}

/* start an M4 filter eval $M4Argv > $Out &   *pwM4 = $!		(ksb)
 */
static FILE *
M4Gadget(const char *pcOut, struct stat *pstLike, pid_t *pwM4, char **ppcM4Argv)
{
	register int iChmodErr, iTry;
	register int fdTemp;
	register FILE *fpRet;
	auto int aiM4[2];

	if (-1 == chmod(pcOut, 0600)) {
		/* If someone unlinked the file, can we put it back?
		 */
		iChmodErr = errno;
		if (ENOENT == errno && -1 != (fdTemp = open(pcOut, O_WRONLY|O_CREAT|O_EXCL, 0600))) {
			/* recovered, we hope */
		} else {
			fprintf(stderr, "%s: chmod: %s: %s\n", progname, pcOut, strerror(iChmodErr));
		}
	}
	if (-1 == (fdTemp = open(pcOut, O_WRONLY|O_TRUNC, 0600))) {
		return (FILE *)0;
	}
	if ((struct stat *)0 != pstLike) {
		(void)fchmod(fdTemp, pstLike->st_mode & 07777);
		(void)fchown(fdTemp, bHasRoot ? pstLike->st_uid : -1, pstLike->st_gid);
	}
	if (-1 == pipe(aiM4)) {
		fprintf(stderr, "%s: pipe: %s\n", progname, strerror(errno));
		close(fdTemp);
		return (FILE *)0;
	}
	for (iTry = 0; iTry < 5; ++iTry) { switch (*pwM4 = fork()) {
	case -1:
		/* we need to backoff and retry at least once
		 */
		if (iTry == 4) {
			fprintf(stderr, "%s: fork m4: %s\n", progname, strerror(errno));
			exit(EX_OSERR);
		}
		sleep(iTry+1);
		continue;
		/*NOTREACHED*/
	case 0:
		close(aiM4[1]);		/* We don't write to ourself */
		if (0 != aiM4[0]) {
			dup2(aiM4[0], 0);
			close(aiM4[0]);
		}
		if (1 != fdTemp) {
			dup2(fdTemp, 1);
			close(fdTemp);
		}
#if defined(M4_SYS_PATH)
		execvp(M4_SYS_PATH, ppcM4Argv);
#endif
		execvp("/usr/bin/m4", ppcM4Argv);
		execvp("/usr/local/bin/m4", ppcM4Argv);
		fprintf(stderr, "%s: execvp: m4: %s\n", progname, strerror(errno));
		exit(EX_OSFILE);
	default:
		close(fdTemp);
		close(aiM4[0]);		/* We don't read m4 input */
		break;
	} break; }
	if ((FILE *)0 == (fpRet = fdopen(aiM4[1], "w"))) {
		fprintf(stderr, "%s: fdopen: %d: %s\n", progname, aiM4[1], strerror(errno));
		close(aiM4[1]);
	}
	return fpRet;
}

/* We have a cache access form, we've built the recipe now run make	(ksb)
 * First make a name for make to help trace errors.
 */
static int
MakeGadget(const char *pcOut, struct stat *pstLike, const char *pcRecipe, char *pcTarget, const char *pcDir)
{
	register int iTry;
	static char *pcMakeName = (char *)0;
	auto pid_t wMake, wWait;
	auto int wStatus;
	auto char *apcMake[8];	/* "make", "-[s]f", $recipe, $target, NULL */
	static const char acTempl[] = "%s: make";

	if ((char *)0 != pcMakeName) {
		/* did it last time, keep it */
	} else if ((char *)0 == (pcMakeName = calloc(wStatus = ((strlen(progname)+sizeof(acTempl))|7)+1, sizeof(char)))) {
		fprintf(stderr, "%s: calloc: %d: %s\n", progname, wStatus,strerror(errno));
		exit(EX_TEMPFAIL);
	} else {
		snprintf(pcMakeName, wStatus, acTempl, progname);
	}

	for (iTry = 0; iTry < 5; ++iTry) { switch (wMake = fork()) {
	case -1:
		/* we need to backoff and retry at least once
		 */
		if (iTry == 4) {
			fprintf(stderr, "%s: fork make: %s\n", progname, strerror(errno));
			exit(EX_OSERR);
		}
		sleep(iTry+1);
		continue;
		/*NOTREACHED*/
	case 0:
		close(1);
		if (-1 == open(pcOut, O_WRONLY, 0666)) {
			exit(EX_OSERR);
		}
		if ((struct stat *)0 != pstLike) {
			(void)fchmod(1, pstLike->st_mode & 07777);
			(void)fchown(1, bHasRoot ? pstLike->st_uid : -1, pstLike->st_gid);
		}
		if ((char *)0 == pcDir || ('.' == pcDir[0] && '\000' == pcDir[1])) {
			/* no chdir */
		} else if (-1 == chdir(pcDir)) {
			fprintf(stderr, "%s: chdir: %s: %s\n", progname, pcDir, strerror(errno));
			exit(EX_OSERR);
		} else if ((char *)0 != strchr(pcDebug, 'C')) {
			fprintf(stderr, "%s: chdir %s\n", progname, pcDir);
		}
		/* building: make -f $recipe $target
		 */
		apcMake[0] = pcMakeName;
		apcMake[1] = "-sf";
		apcMake[2] = (char *)pcRecipe;
		apcMake[3] = pcTarget;
		apcMake[4] = (char *)0;
		if ((char *)0 != strchr(pcDebug, 'C')) {
			register int i;

			fprintf(stderr, "%s:", progname);
			for (i = 0; (char *)0 != apcMake[i]; ++i) {
				fprintf(stderr, " %s", apcMake[i]);
			}
			fprintf(stderr, " >%s\n", pcOut);
			fflush(stderr);
		}
#if defined(MAKE_SYS_PATH)
		execvp(MAKE_SYS_PATH, apcMake);
#endif
		execvp("/usr/bin/make", apcMake);
		execvp("/usr/local/bin/make", apcMake);
		fprintf(stderr, "%s: execvp: make: %s\n", progname, strerror(errno));
		_exit(0);
		_exit(126);
	default:
		break;
	} break; }
	wStatus = 126;
	while (-1 != (wWait = wait(&wStatus)) || EINTR == errno) {
		if (wWait != wMake) {
			continue;
		}
		if (WIFSTOPPED(wStatus)) {
			kill(wMake, SIGCONT);
			continue;
		}
		break;
	}
	return wStatus;
}

/* Run m4 for each file in the slot, return 0 for error,		(ksb)
 * or non-zero for OK.  Some of these might be cache access if we
 * are passed a non-NULL piIsCache && 0 != IsCache[i].
 */
static int
FillSlot(char *pcSlot, char **argv, char **ppcM4Argv, META_HOST *pMHThis, const int *piIsCache, const char *pcCacheCtl)
{
	register int i, cSave;
	auto struct stat stArg;
	auto pid_t wM4, wWait;
	auto int wStatus, wRet;
	register FILE *fpChatter;
	register char *pcAll;
	register const char *pcInto;
	register char *pcTarget, *pcChdir;
	static const char acMarkup[] = "CACHE_";

#if DEBUG
	fprintf(stderr, "%s: FillSlot: %p:\n", progname, pcSlot);
#endif
	pcAll = pcSlot;
	wRet = 0;
	pcChdir = (char *)0;
	for (i = 0; '\000' != *pcSlot && 0 == wRet; ++i) {
		/* If we can't copy the mode/group we still try to process
		 */
		if ((char *)0 == argv[i] || -1 == stat(argv[i], & stArg)) {
			stArg.st_mode = 0700;
			stArg.st_uid = getuid();
			stArg.st_gid = getgid();
		}
#if defined(SLOT_0_ORBITS)
		if (0 == i) {
			stArg.st_mode |= SLOT_0_ORBITS;
		}
#endif
		/* For make path/[.]target[.ext]/recipe.m4 becomes just target
		 * the empty string (from "./recipe.m4" ) becomes the key
		 */
		if ((const int *)0 != piIsCache && 0 != piIsCache[i]) {
			register char *pcSlash, *pcDot, *pcLook;

			pcInto = pcCacheCtl;
			if ((char *)0 != (pcTarget = argv[i])) {
				if ((char *)0 != (pcLook = strrchr(pcTarget, '/'))) {
					*pcLook = '\000';
					pcDot = pcTarget;
					while ('.' == pcDot[0] && '/' == pcDot[1]) {
						while ('/' == *++pcDot)
							;
					}
					pcChdir = strdup(pcDot);
				}
				if ((char *)0 != (pcSlash = strrchr(pcTarget, '/')))
					pcTarget = pcSlash+1;
				while ('.' == *pcTarget)
					++pcTarget;
				if ((char *)0 == (pcDot = strrchr(pcTarget, '.')))
					pcDot = (char *)0;
				else
					*pcDot = '\000';

				if ('\000' == *pcTarget) {
					pcTarget = HostTarget(pMHThis);
				} else if ((char *)0 == (pcTarget = strdup(pcTarget))) {
					fprintf(stderr, "%s: strdup: out of memeory\n", progname);
					exit(EX_TEMPFAIL);
				}
				if ((char *)0 != pcDot)
					*pcDot = '.';
				if ((char *)0 != pcLook)
					*pcLook = '/';
			}
		} else {
			pcChdir = (char *)0;
			pcTarget = (char *)0;
			pcInto = pcSlot;
		}
		if ((char *)0 != pcM4Phase && '\000' == (cSave = *pcM4Phase)) {
			snprintf(pcM4Phase, 64, "%d", i);
		} else {
			cSave = '\000';
		}
		if ((FILE *)0 == (fpChatter = M4Gadget(pcInto, & stArg, & wM4, ppcM4Argv))) {
			exit(EX_OSERR);
		}
		if ((char *)0 != pcTarget) {
			fprintf(fpChatter, "define(`%s%sRECIPE',`%s')define(`%s%sTARGET',`%s')define(`%s%sDIR',`%s')dnl\n", pcXM4Define, acMarkup, pcCacheCtl, pcXM4Define, acMarkup, pcTarget, pcXM4Define, acMarkup, pcChdir);
		}
		SlotDefs(fpChatter, pcAll);
		HostDefs(fpChatter, pMHThis);
		if ((char *)0 != argv[i])
			fprintf(fpChatter, "include(`%s')`'dnl\n", argv[i]);
		fclose(fpChatter);
		if ((char *)0 != strchr(pcDebug, 'D')) {
			register char **ppc;

			fprintf(stderr, "%s:", progname);
			for (ppc = ppcM4Argv; (char *)0 != *ppc; ++ppc) {
				fprintf(stderr, " %s", *ppc);
			}
			fprintf(stderr, "<<\\! >%s\n", pcInto);
			if ((char *)0 != pcTarget) {
				fprintf(stderr, "define(`%s%sRECIPE',`%s')define(`%s%sTARGET',`%s')", pcXM4Define, acMarkup, pcCacheCtl, pcXM4Define, acMarkup, pcTarget);
			}
			SlotDefs(stderr, pcSlot);
			HostDefs(stderr, pMHThis);
			if (-1 == i) {
				fprintf(stderr, "%s\n", argv[i]);
			} else if ((char *)0 != argv[i]) {
				fprintf(stderr, "include(`%s')dnl\n", argv[i]);
			}
			fprintf(stderr, "!\n");
			fflush(stderr);
		}
		if ((char *)0 != pcM4Phase && '\000' == cSave) {
			*pcM4Phase = cSave;
		}
		/* When wait fails we want a known error, nice uses 126
		 */
		wStatus = 126;
		while (-1 != (wWait = wait(&wStatus)) || EINTR == errno) {
			if (wWait != wM4) {
				continue;
			}
			if (WIFSTOPPED(wStatus)) {
				kill(wM4, SIGCONT);
				continue;
			}
			break;
		}
		wRet = wStatus;
		/* Run what ever processor they really wanted, via make.
		 */
		if (0 == wStatus && (char *)0 != pcTarget) {
			wRet = MakeGadget(pcSlot, & stArg, pcCacheCtl, pcTarget, pcChdir);
		}
		if ((char *)0 != pcChdir) {
			free(pcChdir);
			pcChdir = (char *)0;
		}
		pcSlot += strlen(pcSlot)+1;
	}
	return wRet;
}

/* Output the slot, this know that the first position is		(ksb)
 * the literal command, don't do anything for empty slots.
 * We are pretty sure that the user command doesn't have and \000's
 * in it after m4 chewed on it.  We have to stop at the first
 * \000 character, or we have to change the \000 in to spaces.
 */
static void
PrintSlot(char *pcSlot, unsigned uLeft, unsigned uRight, int fdOut)
{
	register unsigned u;
	register int fdCat, iCc;

	if ((char *)0 == pcSlot || '\000' == *pcSlot)
		return;
	for (u = 0; '\000' != *pcSlot; pcSlot += iCc, ++u) {
		iCc = strlen(pcSlot)+1;
		if (u >= uLeft && u < uRight) {
			if (-1 == write(fdOut, pcSlot, iCc)) {
				fprintf(stderr, "%s: write: %d: %s\n", progname, fdOut, strerror(errno));
				return;
			}
			continue;
		}
		if (-1 == (fdCat = open(pcSlot, O_RDONLY, 0))) {
			fprintf(stderr, "%s: open: %s: %s\n", progname, pcSlot, strerror(errno));
			return;
		}
		FdCopy(fdCat, fdOut, pcSlot);
		close(fdCat);
		(void)write(fdOut, "\000", 1);
	}
	errno = 0;
}
%%
