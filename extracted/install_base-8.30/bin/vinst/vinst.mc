/* $Id: vinst.mc,v 1.19 2009/10/15 17:53:16 ksb Exp $
 * implement the vipw-like visual-install product			(ksb)
 */

/* turn the CPP macros into static variables
 */
static char acPathShell[] =
	BIN_shell;
%@foreach "vinst_paths"
%static char acPath%D/%a/e1Uv%D/%a/e2-$v[] =
%	BIN_%a%;
%@endfor
%static char acPathInstall[] =
%	BIN_install%;
%static char acPathInstallus[] =
%	BIN_installus%;
%static char acPathInstck[] =
%	BIN_instck%;
#define CFUDGE	10	/* worst case extra argv[] slots for install opts */

static void
Version()
{
%	printf("%%s: default shell %%s\n", %b, acPathShell)%;
%@foreach "vinst_paths"
%	printf("%%s: %a path %%s\n", %b, acPath%D/%a/e1Uv%D/%a/e2-$v)%;
%@endfor
%	printf("%%s: install path %%s\n", %b, acPathInstall)%;
%	printf("%%s: installus path %%s\n", %b, acPathInstallus)%;
%	printf("%%s: instck path %%s\n", %b, acPathInstck)%;
}

/* quit the program or keep going					(ksb)
 */
static void
QuitOrNot()
{
	register int i, iEdit;

	i = 'a';
	for (;;) { switch (i) {
	case 'a':
		printf("%s: Quit or continue? [q, c, ?] ", progname);
		i = '\000';
		do {
			iEdit = getchar();
			if (!isspace(iEdit) && '\000' == i) {
				i = isupper(iEdit) ? tolower(iEdit) : iEdit;
			}
		} while ('\n' != iEdit && '\r' != iEdit);
		if ('\000' == i) {
			i = 'q';
		}
		continue;
	case 'q':
	case 'n':
		exit(0);
	case 'c':
	case 'y':
		return;
	case '?':
	default:
		printf("q  quit the program\nc  continue anyway\n");
		printf("?  this help message\n");
		i = 'a';
		continue;
	} break; }
}

/* read the target file into the temp file				(ksb)
 */
int
GetFile(fpOut, pcRead, pstTarget)
FILE *fpOut;
char *pcRead;
struct stat *pstTarget;
{
	register char *pcShell;
	register FILE *fpOrig;
	register ssize_t cc, oc, iSize;
	auto char acCopy[16384];	/* big blocks */
	auto struct stat stCopy;

	/* no file to read so we make a template for the Customer
	 */
	if ((char *)0 == pcRead) {
		if ((char *)0 == (pcShell = getenv("SHELL"))) {
			pcShell = acPathShell;
		}
		fprintf(fpOut, "#!%s\nexit 0\n", pcShell);
		return 0 != ferror(fpOut);
	}

	if ((FILE *)0 == (fpOrig = fopen(pcTarget, "rb"))) {
		fprintf(stderr, "%s: fopen: %s: %s\n", progname, pcTarget, strerror(errno));
		exit(3);
	}
	iSize = 0;
	while (0 < (cc = fread(acCopy, 1, sizeof(acCopy), fpOrig))) {
		iSize += cc;
		do {
			oc = fwrite(acCopy, 1, cc, fpOut);
			if (0 == oc) {
				fprintf(stderr, "%s: fwrite: %s\n", progname, strerror(errno));
				return 1;
			}
			cc -= oc;
		} while (0 < cc);
	}
	(void)fclose(fpOrig);
	(void)fflush(fpOut);
	if (ferror(fpOut)) {
		fprintf(stderr, "%s: ferror: %s\n", progname, strerror(errno));
		return 1;
	}
	if (-1 == fstat(fileno(fpOut), &stCopy)) {
		fprintf(stderr, "%s: fstat: %d: %s\n", progname, fileno(fpOut), strerror(errno));
		return 1;
	}
	if (pstTarget->st_size != stCopy.st_size) {
		fprintf(stderr, "%s: copy is not the same size (%ld != %ld)\n", progname, (long)pstTarget->st_size, (long)stCopy.st_size);
		return 1;
	}
	return 0;
}

/* we do not try to emulate install's options too closely		(ksb)
 * just look to see if it looks like there _might_ be a -H in the
 * command line... it might be stashed in the $INSTALL variable...
 */
static int
NoOpt(ppcArgv, iWhich)
char **ppcArgv;
int iWhich;
{
	register int i;

	for (i = 0; (char *)0 != ppcArgv[i]; ++i) {
		if ('-' != ppcArgv[i][0])
			continue;
		if ('-' == ppcArgv[i][1] && '\000' == ppcArgv[i][2])
			break;
		if ((char *)0 != strchr(ppcArgv[i], iWhich)) {
			return 0;
		}
	}
	return 1;
}

/* show be what the changes look like					(ksb)
 */
static void
ShowDiffs(pcF1, pcF2, iStyle)
char *pcF1, *pcF2;
int iStyle;
{
	register char *pcDiffOptions;
	auto int iExit;
	auto char acCmd[MAXPATHLEN*3+200];

	switch (iStyle) {
	case 'c':
		pcDiffOptions = "-C3";
		break;
	case 'u':
		pcDiffOptions = "-U3";
		break;
	default:
		pcDiffOptions = "";
		break;
	}
	(void)sprintf(acCmd, "%s %s \"%s\" \"%s\" | ${PAGER-${more-more}}", acPathDiff, pcDiffOptions, pcF1, pcF2);
	if (0 != (iExit = system(acCmd))) {
		if (WIFSIGNALED(iExit)) {
			fprintf(stderr, "%s: received signale %d\n", progname, WTERMSIG(iExit));
		} else {
			fprintf(stderr, "%s: command failed with %d\n", progname, WEXITSTATUS(iExit));
		}
	}
}

/* we hit a symbolic link, rebuild the target to map to the direct	(ksb)
 * file, as best we can.
 * change "./foo" -> "foo" and "./" -> "." from the link text
 * if link sez "l @-> ls" and target was "/bin/l" report "/bin/ls".
 * one of those hack "tftp @-> ." links remove the last comp.
 * We don't even try to do the "../" thing becasue of the case where
 * the directory comp. prev to the "../" was itself a symbolic link.
 */
static char *
ReTarget(pcOut, pcOrig, pcLink)
char pcOut[MAXPATHLEN], *pcOrig, *pcLink;
{
	register char *pcSlash;
	register int cSave;

	while ('.' == pcLink[0] && '/' == pcLink[1]) {
		pcLink += 2;
		while ('/' == *pcLink)
			++pcLink;
	}
	if ('\000' == *pcLink) {
		pcLink = ".";
	}

	if ('/' == pcLink[0]) { /* absolute link */
		return strcpy(pcOut, pcLink);
	}
	if ((char *)0 == (pcSlash = strrchr(pcOrig, '/'))) {
		/* "orig" -> link */
		if (0 == strcmp(pcLink, pcOrig)) {
			fprintf(stderr, "%s: %s: %s\n", progname, pcOrig, strerror(ELOOP));
			exit(10);
		}
		return strcpy(pcOut, pcLink);
	}
	cSave = *++pcSlash;
	*pcSlash = '\000';
	if (strlen(pcOrig)+strlen(pcLink)+1 > MAXPATHLEN) {
		*pcSlash = cSave;
		fprintf(stderr, "%s: %s: %s\n", progname, pcOrig, strerror(ENAMETOOLONG));
		exit(10);
	}
	if (pcOut != pcOrig) {
		(void)strcpy(pcOut, pcOrig);
	}
	*pcSlash = cSave;
	(void)strcat(pcOut, pcLink);
	return pcOut;
}

/* We found a Bad thing.  The Customer is trying to edit a file		(ksb)
 * that is linked into an OLD directory.  Fix it and make them re-run.
 */
static void
InstckRecover(pcOldDir)
char *pcOldDir;
{
	extern char **environ;
	auto char acMasq[MAXPATHLEN+4];
	static char *apcInstckArgs[] = {
		"bugs@ksb.NPCGuild.Org",
		"-viC/dev/null",
		(char *)0,	/* 2 */
		(char *)0
	};

	printf(" We must run instck to repair the damage.  After instck repairs the links\n");
	printf(" start this program over again.  If you cannot repair the damage\n");
	printf(" seek help: the `purge\' program might remove your work.\n");
	QuitOrNot();

	(void)sprintf(acMasq, "%s: instck", progname);
	apcInstckArgs[0] = acMasq;
	apcInstckArgs[2] = pcOldDir;
	(void)execve(acPathInstck, apcInstckArgs, environ);
	(void)execve(acPathInstck, apcInstckArgs, (char **)0);
	fprintf(stderr, "%s: execve: %s: %s\n", progname, acPathInstck, strerror(errno));
	printf("%s: I can't find `instck' to help you.  Sorry.\n", progname);
}

/* find the -H option the Customer needs				(ksb)
 * The file is the target file if it is in the same directory (stat dirs)
 * and it has the same tail component (strcmp).  Return the filename in
 * a "-H" option format'd string.
 * If a link name has "/OLD/" in it we should complain that the Customer
 * should run instck on the OLD directory.
 */
static char *
FindHards(pcTarget, pstTarget)
char *pcTarget;
struct stat *pstTarget;
{
	register char **ppcAll, *pcMem;
	register int n, iMem;
	register char *pcTailTarget, *pcTailCheck, *pcSep;
	auto char acMountPt[MAXPATHLEN+4];
	auto struct stat stDir, stCheck;

	if ((char **)0 == (ppcAll = (char **)calloc(pstTarget->st_nlink+1, sizeof(char *)))) {
		return (char *)0;
	}
	(void)FindMtPt(pstTarget, acMountPt);
	printf("%s: searching %s for references to %s (inode #%ld),\n\tthis may take a while\n", progname, acMountPt, pcTarget, pstTarget->st_ino);
	fflush(stdout);
	n = SearchHarder(acMountPt, pstTarget, ppcAll, pstTarget->st_nlink);
	if (0 != n) {
		printf("%s: missing %d of %d links\n", progname, n, pstTarget->st_nlink);
		return (char *)0;
	}

	/* concatenate all but the "pimary" into a -H parameter
	 * check for "/OLD/"
	 */
	iMem = 1; /* null at the end */
	for (n = 0; n < pstTarget->st_nlink; ++n) {
		register char *pcOldPath;
		if ((char *)0 == (pcOldPath = strstr(ppcAll[n], "/OLD/"))) {
			iMem += strlen(ppcAll[n])+2;
			continue;
		}
		printf("%s: Link `%s' is in an install backup directory.\n", progname, ppcAll[n]);
		pcOldPath[4] = '\000';
		InstckRecover(ppcAll[n]);
		pcOldPath[4] = '/';
	}
	ppcAll[n] = (char *)0;
	iMem |= 7;
	pcMem = (char *)calloc(iMem+1, sizeof(char));

	pcTailTarget = strrchr(pcTarget, '/');
	if ((char *)0 == pcTailTarget) {	/* sh */
		n = stat(".", & stDir);
		pcTailTarget = pcTarget;
	} else if (pcTailTarget == pcTarget) {	/* /kernel */
		n = stat("/", & stDir);
		pcTailTarget = pcTarget+1;
	} else {				/* /usr/bin/sort */
		*pcTailTarget = '\000';
		n = stat(pcTarget, & stDir);
		*pcTailTarget++ = '/';
	}
	if (-1 == n) {
		fprintf(stderr, "%s: stat: destination directory: %s\n", progname, strerror(errno));
		exit(1);
	}

	/* we know these have at least one slash in them
	 */
	iMem = 0;
	pcSep = "";
	for (n = 0; n < pstTarget->st_nlink; ++n) {
		pcTailCheck = strrchr(ppcAll[n], '/');
		*pcTailCheck = '\000';
		(void)stat(pcTailCheck == ppcAll[n] ? "/" : ppcAll[n], & stCheck);
		*pcTailCheck = '/';
		if (stDir.st_ino == stCheck.st_ino && 0 == strcmp(pcTailCheck+1, pcTailTarget)) {
			/* the "primary link" -- don't copy it */
			continue;
		}
		(void)strcat(pcMem, pcSep);
		(void)strcat(pcMem, ppcAll[n]);
		pcSep = ":";
		++iMem;
	}
	if (iMem+1 != pstTarget->st_nlink) {
		fprintf(stderr, "%s: could not deduce the primary link in the list\n", progname);
		return (char *)0;
	}
	return pcMem;
}

/* All the obvious revision control I could think of -- ksb
 */
static char *apcRevs[] = {
	"RCS",			/* revision control system	*/
	"CVS",			/* Concurrent Versions System	*/
	"SCCS",			/* Source Code Control System	*/
	".svn",			/* Subversion			*/
	"%s,v",			/* RCS				*/
	"s.%s",			/* SCCS				*/
	(char *)0
};
/* We take the same options as install, except we _just_ look at the	(ksb)
 * left-most to see that it is a plain-file.
 *	vinst /tmp/outback -m 644
 * will cp the existing /tmp/outback to a temp place (/tmp/OLD/#ve$$)
 * fork an editor on the file (${VISUAL-${EDITOR-vi}})
 * wait for the editor
 * ask to install the file, or not
 */
void
list_func(argc, ppcInstOpts)
int argc;
char **ppcInstOpts;
{
	register char *pcSlash, *pcEnd;
	register int i;
	register FILE *fpTemp;
	auto struct stat stTarget, stOld;
	auto char acOld[MAXPATHLEN+4], acTemp[MAXPATHLEN+4];
	auto char acIndir[MAXPATHLEN+4];
	auto int fTarget, iEdit, fMakeTarget;
	auto int fdCopy, wUmask;
	auto char *pcHards;
	auto char **ppcNewArgv, *pcInstall, **ppcRevCntl;
	extern char **environ;

	/* We need to do a umask since mkstemp is lame
	 */
	wUmask = umask(0777);
	(void)umask(wUmask);

	if (!isatty(0) || !isatty(1)) {
		fprintf(stderr, "%s: is an interactive program, run from a tty device only\n", progname);
		exit(ENOTTY);
	}
	for (;;) {
		if (-1 == (fTarget = lstat(pcTarget, & stTarget))) {
			if (errno == ENOENT)
				break;
			fprintf(stderr, "%s: lstat: %s: %s\n", progname, pcTarget, strerror(errno));
			exit(1);
		}
		if (S_IFLNK != (stTarget.st_mode & S_IFMT)) {
			break;
		}
		if (-1 == (i = readlink(pcTarget, acOld, sizeof(acOld)))) {
			fprintf(stderr, "%s: readlink: %s: %s\n", progname, pcTarget, strerror(errno));
			exit(1);
		}
		acOld[i] = '\000';
		ReTarget(acIndir, pcTarget, acOld);
		printf("%s: %s is a symbolic link\n", progname, pcTarget);
		printf("%s: change target to \"%s\"\n", progname, acIndir);
		QuitOrNot();
		pcTarget = acIndir;
	}

	/* confirm if file doesn't exist (?)
	 * check plain file, link count > 1 and no options?
	 */
	pcHards = (char *)0;
	if (-1 == fTarget) {
		printf("%s: %s doesn't exist.\n", progname, pcTarget);
		QuitOrNot();
	} else {
		/* I know about continguous files (S_IFCTG), but what to do?
		 * what about whiteout files (S_IFWHT)
		 */
		if (S_IFREG != (stTarget.st_mode & S_IFMT)) {
			printf("%s: %s not a plain file\n", progname, pcTarget);
			QuitOrNot();
		}
		if (stTarget.st_nlink == 1) {
			/* nothing */
		} else if (NoOpt(ppcInstOpts, 'H')) {
			printf("%s: %s has %d hard links -- we need to find them all.\n", progname, pcTarget, stTarget.st_nlink);
			pcHards = FindHards(pcTarget, & stTarget);
			if ((char *)0 == pcHards) {
				printf("%s: sorry, I can't find all the hard links\n", progname);
			}
			QuitOrNot();
		}
	}

	ppcNewArgv = (char **)calloc(argc+CFUDGE, sizeof(char *));
	if ((char **)0 == ppcNewArgv) {
		fprintf(stderr, "%s: calloc: %d*%d: %s\n", progname, argc+CFUDGE, sizeof(char *), strerror(errno));
		exit(1);
	}
	pcInstall = fUserSupport ? acPathInstallus : acPathInstall;
	if ((char *)0 == (pcSlash = strrchr(pcInstall, '/'))) {
		ppcNewArgv[0] = pcInstall;
	} else {
		ppcNewArgv[0] = ++pcSlash;
	}

	/* find the target's directory path
	 */
	if ((char *)0 == (pcSlash = strrchr(pcTarget, '/'))) {
		acOld[0] = '.';
		acOld[1] = '\000';
		pcSlash = pcTarget;
	} else if (pcSlash == pcTarget) {
		acOld[0] = '/';
		acOld[1] = '\000';
		++pcSlash;
	} else {
		*pcSlash = '\000';
		(void)strcpy(acOld, pcTarget);
		*pcSlash++ = '/';
	}

	/* build target directory if we can
	 */
	fMakeTarget = -1 == stat(acOld, & stOld) && ENOENT == errno;
	if (fMakeTarget && ! fUserSupport) {
		fprintf(stderr, "%s: %s: directory doesn't exist, please construct the directory first\n", progname, acOld);
		exit(2);
	}

	/* Does this directory have another Conf Mgmnt type?
	 */
	pcEnd = acOld + strlen(acOld);
	if (pcEnd[-1] != '/') {
		*pcEnd++ = '/';
	}
	/* If we find a revision control directory we should offer to
	 * lock the file and run vi on it directly, then diff and commit
	 * the change.  I'm not ready to code all that today.
	 */
	for (ppcRevCntl = apcRevs; (char *)0 != *ppcRevCntl; ++ppcRevCntl) {
		(void)sprintf(pcEnd, *ppcRevCntl, pcSlash);
		if (-1 == stat(acOld, & stOld)) {
			continue;
		}
		fprintf(stderr, "%s: %s: is a revision control artifact\n", progname, acOld);
		fprintf(stderr, "%s: We should use that revision control system, rather than install.\n", progname);
		QuitOrNot();
		break;
	}

	/* Build OLD directory, if we must
	 */
	(void)strcpy(pcEnd, "OLD");
	if (-1 == stat(acOld, & stOld)) {
		auto pid_t wPid;
		auto int iStatus;

		if (ENOENT != errno) {
			fprintf(stderr, "%s: stat: %s: %s\n", progname, acOld, strerror(errno));
			exit(2);
		}

		/* try to build it with install
		 */
		ppcNewArgv[1] = "-drq";
		ppcNewArgv[2] = acOld;
		ppcNewArgv[3] = (char *)0;
		switch (wPid = fork()) {
		case -1:
			/* ignore it -- installus might build it */
			break;
		case 0:
			execve(pcInstall, ppcNewArgv, environ);
			execve(pcInstall, ppcNewArgv, (char **)0);
			fprintf(stderr, "%s: execve: %s: %s\n", progname, pcInstall, strerror(errno));
			exit(8);
		default:
			(void)waitpid(wPid, & iStatus, 0);
			break;
		}
		/* If it failed it might just not be owned by us in
		 * the installus configuration, which is not fatal. --ksb
		 */
	} else if (S_IFDIR != (stOld.st_mode & S_IFMT)) {
		fprintf(stderr, "%s: %s: is not a directory\n", progname, acOld);
		exit(2);
	}

	/* be sure the target directory is in place
	 */
	*--pcEnd = '\000';
	if (fMakeTarget && -1 == stat(acOld, & stOld) && ENOENT == errno) {
		fprintf(stderr, "%s: %s: %s: failed to construct target directory\n", progname, pcInstall, acOld);
		exit(4);
	}
	*pcEnd++ = '/';
	(void)sprintf(acTemp, "%s/#vnXXXXXX", acOld);

	/* make a copy of the file, or an empty with the correct mode
	 * how smart should we be here.  I pick to show off, a little.
	 */
#if HAVE_MKSTEMP
	if (-1 == (fdCopy = mkstemp(acTemp))) {
		(void)sprintf(acTemp, "%s/vinstXXXXXX", pcTmpdir);
		fdCopy = mkstemp(acTemp);
	}
	if (-1 == fdCopy) {
		fprintf(stderr, "%s: mkstemp: %s: %s\n", progname, acTemp, strerror(errno));
		exit(2);
	}
	if (-1 == fchmod(fdCopy, (~wUmask) & (-1 == fTarget ? 0775 : (0600|(stTarget.st_mode & 0777))))) {
		fprintf(stderr, "%s: fchmod: %d: %s\n", progname, fdCopy, strerror(errno));
		exit(2);
	}
#else
	if ((char *)0 != mktemp(acTemp) && -1 != (fdCopy = open(acTemp, O_CREAT|O_WRONLY, -1 == fTarget ? 0775 : (0600|(stTarget.st_mode & 0777))))) {
		/* OK */
	} else {
		(void)sprintf(acTemp, "%s/vinstXXXXXX", pcTmpdir);
		if ((char *)0 != mktemp(acTemp) && -1 == (fdCopy = open(acTemp, O_CREAT|O_WRONLY, -1 == fTarget ? 0775 : (0600|(stTarget.st_mode & 0777))))) {
			fprintf(stderr, "%s: open: %s: %s\n", progname, acTemp, strerror(errno));
			exit(2);
		}
	}
#endif
	if ((FILE *)0 == (fpTemp = fdopen(fdCopy, "wb"))) {
		fprintf(stderr, "%s: fopen: %s: %s\n", progname, acTemp, strerror(errno));
		exit(2);
	}
	if (GetFile(fpTemp, -1 == fTarget ? (char *)0 : pcTarget, &stTarget)) {
		fclose(fpTemp);
		(void)unlink(acTemp);
		exit(3);
	}
	fclose(fpTemp);

	/* build the argument vector to install the file
	 */
	for (i = 0; i < argc; ++i) {
		ppcNewArgv[i+1] = ppcInstOpts[i];
	}
	if ((char *)0 != pcHards) {
		ppcNewArgv[++argc] = "-H";
		ppcNewArgv[++argc] = pcHards;
	}
	ppcNewArgv[++argc] = acTemp;
	ppcNewArgv[++argc] = pcTarget;
	ppcNewArgv[++argc] = (char *)0;

	/* edit the temp file and offer to install it
	 *	r restart (get another copy to edit)
	 *	e edit it again
	 *	d show me the diffs ([c] context changes [u] unified output)
	 *	h help [?] help
	 *	i install [y] yes
	 *	n no delete the file [q] quit
	 */
	i = 'e';
	for (;;) { switch (i) {
	case 'r':
		if ((FILE *)0 == (fpTemp = fopen(acTemp, "wb"))) {
			fprintf(stderr, "%s: fopen: %s: %s\n", progname, acTemp, strerror(errno));
			exit(5);
		}
		i = GetFile(fpTemp, -1 == fTarget ? (char *)0 : pcTarget, &stTarget);
		fclose(fpTemp);
		if (0 != i) {
			unlink(acTemp);
			exit(9);
		}
		i = 'e';
		continue;
	case 'e':
%		iEdit = %K<util_editor>1v(acTemp)%;
		if (0 != iEdit) {
			fprintf(stderr, "%s: %s: editor failed with %d\n", progname, acTemp, iEdit);
		}
		/* fall through */
	case 'a':
		fprintf(stderr, "%s: What next? [?, d/c/u, e, i, q, r] ", progname);
		i = '\000';
		do {
			iEdit = getchar();
			if (!isspace(iEdit) && '\000' == i) {
				i = isupper(iEdit) ? tolower(iEdit) : iEdit;
			}
		} while ('\n' != iEdit && '\r' != iEdit);
		if ('\000' == i) {
			i = '?';
		}
		continue;
	case 'q':
	case 'n':
		(void)unlink(acTemp);
		exit(0);
	case 'i':
	case 'y':
		break;
	case 'd':
	case 'c':
	case 'u':
		ShowDiffs(pcTarget, acTemp, i);
		i = 'a';
		continue;
	case '?':
	case 'h':
	default:
		printf("%s:", progname);
		for (i = 0; (char *)0 != ppcNewArgv[i]; ++i) {
			printf(" %s", ppcNewArgv[i]);
		}
		printf("\nq  quit, do not %s the target\n", ppcNewArgv[0]);
		printf("d  show differences between copy and target [c context] [u unified]\n");
		printf("e  edit the file again\n");
		printf("i  update %s\n", pcTarget);
		printf("r  reload %s, edit a fresh copy\n", acTemp);
		printf("?  this help message\n");
		i = 'a';
		continue;
	} break; }
	(void)execve(pcInstall, ppcNewArgv, environ);
	(void)execve(pcInstall, ppcNewArgv, (char **)0);
	fprintf(stderr, "%s: execve: %s: %s\n", progname, pcInstall, strerror(errno));
	exit(8);
}
