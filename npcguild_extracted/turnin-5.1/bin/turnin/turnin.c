/* $Id: turnin.c,v 5.1 2000/01/06 15:06:28 ksb Exp $
 * turnin files - electronic submission, the send side			(ksb)
 *
 * expects the environment varaiable
 *	<SUBPREFIX><course>=d<div>s<sec>
 * to be in the environment
 *
 * configuration
 *	apcCourse contains a list of the courses, and their submit
 *	directories, each of these directories contains a file called
 *	"Projlist" which contains the current project numbers.
 *	The students files will be filed in his division.section
 *	subdirectory under his login name.

		THIS PROGRAM RUNS SET UID ROOT

 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <ctype.h>		/* for isspace() and isdigit() macros	*/
#include <fcntl.h>

#include "../project/machine.h"
#include "main.h"

#if HAVE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

extern struct group *getgrnam(), *getgrgid();
extern struct passwd *getpwnam();
extern char *getenv();
extern char *mktemp(), **environ, *malloc(), *calloc();
#define strsave(Mpch) strcpy(malloc(strlen(Mpch)+1), Mpch)

extern int errno;
#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me)	(sys_errlist[Me])
#endif

static char acCompress[] =	/* path to compress			*/
	COMPRESS_PATH;
static char acDotZ[] =		/* compress extender			*/
	".Z";
static char acTar[] =		/* path to tar				*/
	TAR_PATH;
static char acSub[] =		/* record the SUB_ prefix		*/
	SUBPREFIX;
static char acSectIgnore[] =	/* bogus, ``no sections'' string	*/
	SECTIGNORE;
static int iCourses;		/* number of valid subscribed courses	*/

static char *apcCourse[MAXCOURSE];	/* who				*/
static char *apcUid[MAXCOURSE];		/* who -> uid			*/
static char *apcDir[MAXCOURSE];		/* who -> where			*/
static char *apcGroup[MAXCOURSE];	/* who -> group			*/
static char *apcSections[MAXCOURSE];	/* who -> sections (*)		*/

/* lines from the project file	*/
static char *apcProjects[MAXPROJECTS+1];

/*
 * reads a configuration file of the form:			(doc/ksb)
 * course:alpha uid:subdir for this course:alpha gid\n
 */
static void
ReadDB()
{
	register FILE *fpDB;
	register char *pc;
	register int i;
	static char acError[] = "%s: error in data base line %d\n";
	static char acLine[MAXCHARS];
	static char acTurnbase[] = TURNBASE;

	if (0 == (fpDB = fopen(acTurnbase, "r"))) {
		fprintf(stderr, "%s: fopen: %s: %s\n", progname, acTurnbase, strerror(errno));
		exit(2);
	}

	i = 0;
	while ((char *)0 != fgets(acLine, MAXCHARS, fpDB)) {
		pc = acLine;
		while (isspace(*pc) && '\n' != *pc)
			++pc;
		if ('#' == *pc || '\n' == *pc)	/* comments and blank lines */
			continue;
		apcCourse[i] = pc = strsave(pc);
		if ((char *)0 == (pc = strchr(pc, SEP))) {
			printf(acError, progname, i);
			exit(16);
		}
		*pc++ = '\000';
		apcUid[i] = pc;
		if ((char *)0 == (pc = strchr(pc, SEP))) {
			printf(acError, progname, i);
			exit(17);
		}
		*pc++ = '\000';
		apcDir[i] = pc;
		if ((char *)0 == (pc = strchr(pc, SEP))) {
			printf(acError, progname, i);
			exit(18);
		}
		*pc++ = '\000';
		apcGroup[i] = pc;
		if ((char *)0 == (pc = strchr(pc, SEP))) {
			printf(acError, progname, i);
			exit(19);
		}
		*pc++ = '\000';
		apcSections[i] = pc;
		if ((char *)0 != (pc = strchr(pc, '\n'))) {
			*pc = '\000';
		}
		++i;
	}
	apcCourse[i] = (char *)0;
}

/*
 * list all the courses turnin knows about				(ksb)
 */
void
ListCourses(fp)
FILE *fp;
{
	register int iWho;

	for (iWho = 0; (char *)0 != apcCourse[iWho]; ++iWho) {
		fprintf(fp, "%16s", apcCourse[iWho]);
		if (3 == iWho % 4)
			fprintf(fp, "\n");
	}
	if (0 != iWho % 4) {
		fprintf(fp, "\n");
	}
}

/*
 * list the sections in the given course				(ksb)
 * input which one the user wants and filter it for case
 */
void
FindSection(iWho, fp, pcIndex)
int iWho;
FILE *fp;
char *pcIndex;
{
	register char *pcComma, *pcLast;

	/* only one, take that one
	 */
	if ((char *)0 == strchr(apcSections[iWho], ',')) {
		/** fprintf(fp, "There is only one section for %s, using \"%s\".\n", apcCourse[iWho], apcSections[iWho]);
		 **/
		(void)strcpy(pcIndex, apcSections[iWho]);
		return;
	}
	/* if the user gave us one to try
	 */
	if ('\000' != *pcIndex) {
		goto trysec;
	}
	for (;;) {
		fprintf(fp, "The sections of %s are:\n", apcCourse[iWho]);
		pcComma = apcSections[iWho];
		while ((char *)0 != (pcLast = pcComma)) {
			pcComma = strchr(pcComma, ',');
			if ((char *)0 != pcComma)
				*pcComma = '\000';
			fprintf(fp, "\t%s\n", pcLast);
			if ((char *)0 != pcComma)
				*pcComma++ = ',';
		}

		fprintf(fp, "Enter your section: ");
		if ((char *)0  == fgets(pcIndex, MAXDIVSEC, stdin)) {
			fprintf(fp, "quit\n");
			exit(0);
		}
		if ((char *)0 == (pcLast = strchr(pcIndex, '\n'))) {
			fprintf(fp, "%s: invalid division/section identifier\n", progname);
			continue;
		}
		*pcLast = '\000';

	trysec:
		pcComma = apcSections[iWho];
		while ((char *)0 != (pcLast = pcComma)) {
			pcComma = strchr(pcComma, ',');
			if ((char *)0 != pcComma)
				*pcComma = '\000';
			if (0 == strcasecmp(pcIndex, pcLast)) {
				/* fix the case for them */
				(void)strcpy(pcIndex, pcLast);
				if ((char *)0 != pcComma)
					*pcComma++ = ',';
				return;
			}
			if ((char *)0 != pcComma)
				*pcComma++ = ',';
		}
		fprintf(fp, "%s: cannot find section `%s\' in the list for `%s\', retype it please.\n", progname, pcIndex, apcCourse[iWho]);
	}
}

/*
 * If the given course if on the course list, return its index		(aho)
 * in the apcCourse[] array.  Otherwise, return -1.
 */
static int
VerifyCourse(pcClass)
char *pcClass;
{
	register char **ppch;

	for (ppch = apcCourse; (char *)0 != *ppch; ++ppch) {
		if (0 == strcasecmp(pcClass, *ppch)) {
			return ppch - apcCourse;
		}
	}
	return -1;
}


/*
 * we need to tell the user (on stdout) what to put in his		(ksb)
 * .login/.profile.  Write to him only on stderr.
 * (run as user for no reason, btw)
 */
static void
DoInit(argc, argv, pcGuess)
int argc;
char **argv, *pcGuess;
{
	register char *pcIndex, *pcEqual;
	auto int iWho, fShell, fMore;
	auto char acBuf[MAXCOURSENAME + MAXDIVSEC];
	auto char *pcShell, acYesNo[MAXYESNO+1];
	auto char *pcKopOut = "abort";

	fprintf(stderr, "\n\
\tConfiguring your account to send electronic submissions\n\n\
This program sets up your computer account so that you may use \"%s\",\n\
the electronic submission program.\n\n", progname);

	if ((char *)0 == (pcShell = getenv("SHELL"))) {
		pcShell = "/bin/sh";
	}

	iWho = strlen(pcShell);
	if (iWho >= 3 && 'c' == pcShell[iWho-3])
		fShell = 'c';
	else
		fShell = 's';

	/* if the user used turnin -i -c cs100 we need to put cs100 in the list
	 * we know argv is (char *)0 terminated, we can use that slot.
	 */
	if ((char *)0 != pcGuess) {
		argv[argc] = pcGuess;
		++argc;
	}
	fMore = 0 == argc;
	if (fMore) {
		fprintf(stderr, "\
Below you will see a list of courses that are presently using this system.\n\
For each course that you are enrolled in, enter its name and section when\n\
prompted.  When you have entered information for all of your courses, press\n\
^D (Control-D) to quit.\n");
	} else {
		fprintf(stderr, "\
By responding the prompts below you will be enrolled in electronic\n\
submissions for");
		for (iWho = 0; iWho < argc; ++iWho) {
			fprintf(stderr, "%s%s", (0 == iWho ? " " : ", "), argv[iWho]);
		}
		fprintf(stderr, ".\n");
	}
	while (fMore || argc > 0) {
		fprintf(stderr, "\n");
		if ((char *)0 != argv[0]) {
			(void)strcpy(acBuf, argv[0]);
			pcIndex = strchr(acBuf, '\000');
			--argc, ++argv;
			goto tryit;
		}

		fprintf(stderr, "Here is a list of the courses currently available\n");
		fprintf(stderr, "(if you cannot find your course in this list type it anyway):\n");
		for (;;) {
			ListCourses(stderr);
			fprintf(stderr, "Enter a course name you are subscribed to or ^D to quit: ");
			fflush(stderr);
			if ((char *)0 == fgets(acBuf, MAXCOURSENAME, stdin) || 0 == strcmp("^D\n", acBuf) || 0 == strcmp("^d\n", acBuf)) {
				fprintf(stderr, "%s\n", pcKopOut);
				exit(0);
			}
			if ((char *)0 == (pcIndex = strchr(acBuf, '\n'))) {
				fprintf(stderr, "%s: invalid course name\n", progname);
				exit(6);
			}
			*pcIndex = '\000';
	tryit:
			if ('\000' == acBuf[0]) {
				/* skip blank lines */
				continue;
			}
			if ((char *)0 != (pcEqual = strchr(acBuf, '='))) {
				pcIndex = pcEqual;
				*pcIndex = '\000';
			}
			if (-1 != (iWho = VerifyCourse(acBuf))) {
				break;
			}
			fprintf(stderr, "\nIt is possible that you have entered a course name that will be valid later\n");
			fprintf(stderr, "If you are sure that is the name of your course I will arrange to have %s\n", progname);
			fprintf(stderr, "ask you for the section information later.\n\n");
			fprintf(stderr, "Is `%s\' is a course identifier for one of your courses? [ny] ", acBuf);
			if ((char *)0 == fgets(acYesNo, MAXYESNO, stdin)) {
				exit(1);
			}
			if ('y' == acYesNo[0] || 'Y' == acYesNo[0]) {
				if ((char *)0 == pcEqual) {
					*pcIndex++ = '\000';
					*pcIndex = '\000';
				}
				goto unknown;
			}
		}

		/*
		 * if the user is subscribed to this course, don't ask him
		 * for divsion/section
		 */
		*pcIndex++ = '\000';
		if ((char *)0 == pcEqual) {
			*pcIndex = '\000';
		}

		FindSection(iWho, stderr, pcIndex);

		(void)setpwent();
		if ((struct passwd *)0 == getpwnam(apcUid[iWho])) {
			fprintf(stderr, "Instructor \"%s\" doesn't exist on this machine, therefore, you probably\n", apcUid[iWho]);
			fprintf(stderr, "cannot use this machine for \"%s\".\n", acBuf);
		}
		(void)endpwent();

		(void)setgrent();
		if ((struct group *)0 == getgrnam(apcGroup[iWho])) {
			fprintf(stderr, "Group \"%s\" doesn't exist on this machine, report this to a consultant.\n", apcUid[iWho]);
		}
		(void)endgrent();

	unknown:
		if ('s' == fShell) {
			printf("%s%s=\'%s\'; export %s%s\n", acSub, acBuf, pcIndex, acSub, acBuf);
		} else {
			printf("setenv %s%s \'%s\'\n",acSub, acBuf, pcIndex);
		}
		pcKopOut = "done";
	}
}

/*
 * compare two strings for qsort
 */
static int
q_compare(ppch1, ppch2)
	char **ppch1, **ppch2;
{
	return strcmp(*ppch1, *ppch2);
}


/*
 * Move environment variables beginning with SUBPREFIX to the		(aho)
 * beginning of the environment (using swaps).  Sort the
 * SUBPREFIX variables. Set the global variable iCourses to
 * the number of submit variables found.
 * If we find a SUBPREFIX variable in the environment for a course we
 * don't find on the course list, we throw it out.
 *
 * for SUBPREFIX="SUB_"
 *	SUB_cs300=d1s1
 *	SUB_cs400=morning
 *	SUB_cs400=930kevin
 *
 * note: we assume we can write on what environ points to
 */
static void
FixEnvironment()
{
	register char **ppch;
	register char *pcTemp;
	register char *pcIndex;

	iCourses = 0;
	for (ppch = environ; (char *)0 != *ppch; ++ppch) {
		if (0 == strncmp(acSub, *ppch, sizeof(acSub)-1) &&
		(char *)0 != (pcIndex = strchr(sizeof(acSub)-1 + *ppch, '='))) {
			*pcIndex = '\000';
			if (-1 != VerifyCourse(sizeof(acSub)-1 + *ppch)) {
				pcTemp = environ[iCourses];
				environ[iCourses] = *ppch;
				*ppch = pcTemp;
				++iCourses;
			}
			*pcIndex = '=';
		}
	}
	if (1 < iCourses) {
		qsort((char *)environ, iCourses, sizeof(char *), q_compare);
	}
}


/*
 * Search the subscribed courses (the SUB_<course> environment 	    (ksb/aho)
 * varibles) for a single course this submission is to.  If
 * there is more than one subscribed course, use pcGuess to
 * arbitrate.  If we fail, return (char *)0.
 *
 * note: we assume that FixEnvironment() has been called before us
 */
static char *
Which(pcGuess)
char *pcGuess;
{
	register char **ppch;
	register char *pcClass;
	register int i;

	pcClass = (char *)0;
	for (ppch = environ; ppch < & environ[iCourses]; ++ppch) {
		/* two course in env, no -c, use Ask from main
		 */
		if ((char *)0 != pcClass) {
			return (char *)0;
		}
		pcClass = sizeof(acSub)-1 + *ppch;
		if ((char *)0 == pcGuess) {
			continue;
		}
		i = strlen(pcGuess);
		if (0 == strncmp(pcGuess, pcClass, i) && '=' == pcClass[i]) {
			return pcClass;
		}
		pcClass = (char *)0;
	}
	return pcClass;
}

/*
 * List the courses the user is subscribed to which are also in the
 * course list.
 */
static void
ListSubscribed(fp)
FILE *fp;
{
	register char **ppchEnv;
	register char *pc;
	register int chSep = ':';

	if (0 == iCourses) {
		fprintf(fp, "You are not subscribed to any courses.\n");
		return;
	}
	fprintf(fp, "You are subscribed to");
	for (ppchEnv = environ; ppchEnv < & environ[iCourses]; ++ppchEnv) {
		if ((char *)0 == (pc = strchr(sizeof(acSub)-1 + *ppchEnv, '='))) {
			continue;
		}
		*pc = '\000';
		fprintf(fp, "%c %s", chSep, sizeof(acSub)-1 + *ppchEnv);
		*pc = '=';
		chSep = ',';
	}
	putc('\n', fp);
}

/*
 * get the words of wisdom from the user				(ksb)
 * strip leading white space, etc
 */
static char *
GetWord(pcBuf, iLen, pcDef)
char *pcBuf, *pcDef;
int iLen;
{
	register char *pcWhite, *pcCopy;
	register int l;

	fflush(stdout);
	pcBuf[0] = '\n';
	if ((char *)0 == fgets(pcBuf, iLen, stdin))
		return (char *)0;
	pcWhite = pcBuf;
	while (isspace(*pcWhite) && '\n' != *pcWhite)
		++pcWhite;

	pcCopy = pcBuf;
	l = iLen;
	while (l-- > 0 && '\n' != (*pcCopy = *pcWhite))
		++pcCopy, ++pcWhite;

	if (pcCopy == pcBuf)
		(void)strncpy(pcBuf, pcDef, iLen);
	else
		*pcCopy = '\000';
	return pcBuf;
}

/*
 * We couldn't figure out the course, so now we ask the user		(aho)
 * interactively.  We return the course and div/sec as
 * follows: cs100=d1s1
 *
 * invariant: we do not return (char *)0
 */
static char *
AskCourse()
{
	register char **ppch;
	auto struct group *grpMe;
	static char acBuf[MAXCOURSENAME + MAXDIVSEC];
	static char acDef[MAXCOURSENAME + MAXDIVSEC];

	/* if our groupname is a valid course, use it as the default
	 * else if our groupname is the group for a course, use that
	 */
	acDef[0] = '\000';
	(void)setgrent();
	if ((struct group *)0 != (grpMe = getgrgid(getgid()))) {
		(void)strcpy(acDef, grpMe->gr_name);
		if (-1 == VerifyCourse(acDef)) {
			register int i;
			for (i = 0; (char *)0 != apcCourse[i]; ++i) {
				if (0 == strcmp(apcGroup[i], acDef))
					break;
			}
			if ((char *)0 != apcCourse[i]) {
				(void)strcpy(acDef, apcCourse[i]);
			} else {
				acDef[0] = '\000';
			}
		}
	}
	(void)endgrent();

	switch (iCourses) {
	case 0:
		printf("\
You are not subscribed to any valid course, here is a list of the courses\n\
you may use currently:\n");
		ListCourses(stdout);
		/* use the first course as the default
		 */
		if ('\000' == acDef[0] && (char *)0 != apcCourse[0]) {
			(void)strcpy(acDef, apcCourse[0]);
		}
		break;
	default:
		ListSubscribed(stdout);
		/* use the first subscribed course as the default
		 */
		if ('\000' == acDef[0]) {
			register int i;
			register char *pcIndex;

			pcIndex = environ[0] + sizeof(acSub)-1;
			for (i = 0; '=' != pcIndex[i]; ++i) {
				acDef[i] = pcIndex[i];
			}
			acDef[i] = '\000';
		}
		printf("\nIf the course you wish to submit to is not on the above list, try it anyway\n");
		break;
	}
	printf("Enter course name? [%s] ", acDef);

	if ((char *)0 == GetWord(acBuf, MAXCOURSENAME, acDef)) {
		fprintf(stderr, "%s: invalid course name\n", progname);
		exit(6);
	}
	if (-1 == VerifyCourse(acBuf)) {
		fprintf(stderr, "%s: cannot find \"%s\" in the course list\n", progname, acBuf);
		exit(7);
	}
	/*
	 * if the user is subscribed to this course, don't ask him
	 * for divsion/section, return the info here
	 */
	for (ppch = environ; ppch < & environ[iCourses]; ++ppch) {
		if (0 == strncmp(acBuf, sizeof(acSub)-1 + *ppch, strlen(acBuf))) {
			return sizeof(acSub)-1 + *ppch;
		}
	}
	return acBuf;
}

/*
 * read the given project list file and store in apcProjects		(aho)
 */
static void
ReadProjects(iUid, iGid, pcFile)
int iUid, iGid;
char *pcFile;
{
	register FILE *fp;
	register char *pc;
	register int iProject, cc;
	auto char acLine[MAXCHARS];
	auto int aiPipe[2];

	if (-1 == pipe(aiPipe)) {
		fprintf(stderr, "%s: pipe: %s\n", progname, strerror(errno));
		exit(9);
	}

	/* We don't depend on setreuid because most systems
	 * don't have it... sigh, we drop after a fork and pump the
	 * data back -- access rights are _slower_.
	 */
	switch (fork()) {
	case -1:
		fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		exit(9);
	case 0:
		if (0 != aiPipe[0]) {
			close(aiPipe[0]);
		}
		close(0);
		if (-1 == setgid(iGid) || -1 == setuid(iUid)) {
			fprintf(stderr, "%s: setuid: %d.%d: %s\n", progname, iUid, iGid, strerror(errno));
			exit(1);
		}
		if (0 != open(pcFile, O_RDONLY, 0)) {
			fprintf(stderr, "%s: open: %s: %s\n", progname, pcFile, strerror(errno));
			exit(1);
		}
		while (0 < (cc = read(0, acLine, sizeof(acLine)))) {
			(void)write(aiPipe[1], acLine, cc);
		}
		exit(0);
	defailt:
		break;
	}

	close(aiPipe[1]);
	if ((FILE *)0 == (fp = fdopen(aiPipe[0], "r"))) {
		fprintf(stderr, "%s: fdopen: %d: %s\n", progname, aiPipe[0], strerror(errno));
		exit(9);
	}
	iProject = 0;
	while ((char *)0 != fgets(acLine, MAXCHARS, fp)) {
		if ('\n' == acLine[0] || '#' == acLine[0])
			continue;
		if (iProject >= MAXPROJECTS) {
			/*ZZZ bogus */
			fprintf(stderr, "%s: too many projects in project file\n", progname);
			exit(10);
		}
		pc = acLine + strlen(acLine) - 1;
		while (isspace(*pc)) {
			*pc-- = '\000';
		}
		apcProjects[iProject++] = strsave(acLine);
	}
	apcProjects[iProject] = (char *)0;
	(void)fclose(fp);
}

/*
 * Determine the project name for this submission.  If we can't figure it out,
 * we print errors and exit.
 */
static char *
ProjName(pcGuess, pcClass)
	char *pcGuess, *pcClass;
{
	register char **ppch;

	for (ppch = apcProjects; (char *)0 != *ppch; ++ppch) {
		switch (**ppch) {
		case '=':
			if ((char *)0 == pcGuess) {
				return 1 + *ppch;
			}
			/*FALLTHROUGH*/
		case '+':
			if ((char *)0 == pcGuess) {
				continue;
			}
			if (0 == strcasecmp(1+*ppch, pcGuess)) {
				return 1 + *ppch;
			}
			break;
		default:
			if ((char *)0 == pcGuess) {
				continue;
			}
			if (0 == strcasecmp(1+*ppch, pcGuess)) {
				fprintf(stderr, "%s: submissions for ", progname);
				if (isdigit(**ppch)) {
					fprintf(stderr, "project ");
				}
				fprintf(stderr, "%s have been turned off.\n", *ppch);
				exit(11);
			}
		}
	}
	if ((char *)0 == pcGuess) {
		fprintf(stderr, "%s: no current project for %s\n", progname, pcClass);
		fprintf(stderr, "Use \"%s -l\" for a list of projects in a course.\n", progname);
		exit(12);
	}
	fprintf(stderr, "%s: ", progname);
	if (isdigit(*pcGuess)) {
		fprintf(stderr, "Project ");
	}
	fprintf(stderr, "\"%s\" is not a current project for submission in %s.  Use\n\"project -l\" for a list of projects in this course.\n", pcGuess, pcClass);
	exit(13);
	/*NOTREACHED*/
}


/*
 * list the valid projects to stdout; the argument pcClass is for	(ksb)
 * printing headers/error messages
 *
 * (assume that ReadProjects has been called)
 */
static void
ListProjects(pcClass)
char *pcClass;
{
	register char **ppch;

	if ((char *)0 == apcProjects[0]) {
		printf("There are no current projects for %s\n", pcClass);
		return;
	}
	printf("Current projects for %s:\n", pcClass);
	for (ppch = apcProjects; (char *)0 != *ppch; ++ppch) {
		printf("%-16s", 1+*ppch);
		switch (**ppch) {
		case '=':
			printf(" on (current)\n");
			break;
		case '+':
			printf(" on (alternate)\n");
			break;
		default:
			printf(" off\n");
			break;
		}
	}
}


/*
 * To prevent people from making clever paths to the turnin dirctory
 * with empty components, dots, and slashes, call this routine
 */
static void
GoodFile(pcName, pcWhat)
char *pcName, *pcWhat;
{
	if ((char *)0 == pcName || '\000' == pcName[0]) {
		fprintf(stderr, "%s: no %s specified\n", progname, pcWhat);
		exit(13);
	}
	if ((char *)0 != strchr(pcName, '.')) {
		fprintf(stderr, "%s: will not allow %ss with \'.\' in them\n", progname, pcWhat);
		exit(14);
	}
	if ((char *)0 != strchr(pcName, '/')) {
		fprintf(stderr, "%s: will not allow %ss with \'/\' in them\n", progname, pcWhat);
		exit(15);
	}
}

/*
 * show an index of the submitted files					(ksb)
 * prefer the uncompressed file
 *	tar tvf - <file
 *
 * but show compressed if no other choice
 *	compress -d <file | tar tvf -
 */
void
ShowTar(pcFile)
char *pcFile;
{
	auto char *nargv[4];
	auto int pid, n;
	auto int fds[2];

	fflush(stdout);
	fflush(stderr);
	switch (pid = fork()) {
	case 0:
		close(0);
		if (0 != open(pcFile, O_RDONLY, 0600)) {
			(void)strcat(pcFile, acDotZ);
			if (0 != open(pcFile, O_RDONLY, 0600)) {
				fprintf(stderr, "%s: open: %s: %s\n", progname, pcFile, strerror(errno));
				exit(1);
			}
			if (0 != pipe(fds)) {
				fprintf(stderr, "%s: pipe: %s\n", progname, strerror(errno));
				exit(1);
			}
			switch (fork()) {
			case -1:
				fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
				exit(1);
			case 0:		/* zcat of tar file.Z		*/
				close(1);
				dup(fds[1]);
				close(fds[1]);
				close(fds[0]);
				nargv[0] = "compress";
				nargv[1] = "-d";
				nargv[2] = (char *)0;
				execve(acCompress, nargv, environ);
				/* exec zcat */
				exit(1);
			default:	/* |tar fvf -			*/
				close(0);
				dup(fds[0]);
				close(fds[0]);
				close(fds[1]);
				break;
			}
		}
		nargv[0] = "tar";
		nargv[1] = "-tvf";
		nargv[2] = "-";
		nargv[3] = (char *)0;
		execve(acTar, nargv, environ);
		fprintf(stderr, "%s: execve: %s: %s\n", progname, acTar, strerror(errno));
		exit(21);
		break;
	case -1:
		fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		return;
	default:
		break;
	}
	while (-1 != (n = wait((WAIT_T *)0)) && pid != n)
		;
}

/*
 * make an archive and transfer to instructors account			(ksb)
 */
int
Turnin(argc, argv, eWhat)
int argc;
char **argv;
WHAT eWhat;
{
	register int i, n, pid;
	register char **nargv;
	register char *pcProj;
	register char *pcDiv;
	auto char *pcUser;
	auto struct passwd *ppw;
	auto struct group *pgr;
	static char acPath[MAXPATHLEN+1];
	static int iWho;
	static struct stat stat_buf;
	static WAIT_T wait_buf;
#if CONFIRM
	static char acYes[MAXCHARS];
#endif

	/* RUNS SET UID ROOT */

	/*
	 * Read submission information from the user's environment, from
	 * the instructors project list file, and from our data base, and
	 * check it all for consistency.
	 */
	ReadDB();

	if (INITUSER == eWhat) {
#if HAVE_SETRESUID
		if (-1 == setresuid(-1, getuid(), -1)) {
			fprintf(stderr, "%s: setresuid: %s\n", progname, strerror(errno));
			exit(20);
		}
#else
		if (-1 == seteuid(getuid())) {
			fprintf(stderr, "%s: seteuid: %s\n", progname, strerror(errno));
			exit(20);
		}
#endif
		DoInit(argc, argv, pcCourse);
		exit(0);
	}

	FixEnvironment();
	if (LISTSUB == eWhat) {
		if (0 == iCourses) {
			printf("You are not subscribed to any courses.\n");
			exit(1);
		}
		ListSubscribed(stdout);
		exit(0);
	}

	if ((char *)0 == pcCourse && iCourses > 0) {
		pcCourse = Which(pcCourse);
	}
	if ((char *)0 == pcCourse) {
		printf("\n\
For this %s, you may enter the course information interactively.\n",  LISTPROJ == eWhat ? "listing" : "submission");
		pcCourse = AskCourse();
	}
	if ((char *)0 != (pcDiv = strchr(pcCourse, '='))) {
		*pcDiv++ = '\000';
	}

	if (-1 == (iWho = VerifyCourse(pcCourse))) {
		fprintf(stderr, "%s: cannot find your course (%s) in course list\n", progname, pcCourse);
		exit(7);
	}

	(void) setpwent();
	if ((struct passwd *)0 == (ppw = getpwnam(apcUid[iWho]))) {
		printf("%s: owner \"%s\" doesn\'t exist on this machine\n", progname, apcUid[iWho]);
		exit(17);
	}
	(void) setgrent();
	if ((struct group *)0 == (pgr = getgrnam(apcGroup[iWho]))) {
		printf("%s: group \"%s\" doesn\'t exist on this machine\n", progname, apcGroup[iWho]);
		exit(17);
	}

	(void)sprintf(acPath, "%s/%s/%s", ppw->pw_dir, apcDir[iWho], PROJLIST);
	ReadProjects(ppw->pw_uid, pgr->gr_gid, acPath);
	if (LISTPROJ == eWhat) {
		ListProjects(pcCourse);
		exit(0);
	}

	pcProj = ProjName(pcLineProj, pcCourse);
	GoodFile(pcProj, "project");

	if ((char *)0 == pcDiv || '\000' == pcDiv[0]) {
		static char acDiv[MAXDIVSEC+1];
		pcDiv = acDiv;
		*pcDiv = '\000';
		FindSection(iWho, stdout, pcDiv);
	}
	GoodFile(pcDiv, "division/section");

	(void)sprintf(acPath, "%s/%s/%s", ppw->pw_dir, apcDir[iWho], pcProj);
	if (0 != strcmp(acSectIgnore, pcDiv) || 0 != strcmp(acSectIgnore, apcSections[iWho])) {
		(void)strcat(acPath, "/");
		(void)strcat(acPath, pcDiv);
	}
	/* become the instructor
	 */
#if HAVE_SETRESUID
	if (-1 == setresgid(-1, pgr->gr_gid, -1) || -1 == setresuid(-1, ppw->pw_uid, -1)) {
		fprintf(stderr, "%s: setres{u,g}id: %s\n", progname, strerror(errno));
		exit(20);
	}
#else
	if (-1 == setegid(pgr->gr_gid) || -1 == seteuid(ppw->pw_uid)) {
		fprintf(stderr, "%s: sete{u,g}id: %s\n", progname, strerror(errno));
		exit(20);
	}
#endif

	if (0 != stat(acPath, & stat_buf)) {
		fprintf(stderr, "%s: stat: %s: %s\n", progname, acPath, strerror(errno));
		exit(18);
	}
	(void)strcat(acPath, "/");

	/* preserve $USER or $LOGNAME if uid of that name matches our uid
	 * this is a gratuitous feature no one will ever use... (ksb)
	 */
	if ((char *)0 == (pcUser = getenv("USER"))) {
		pcUser = getenv("LOGNAME");
	}
	if ((char *)0 != pcUser && (struct passwd *)0 != (ppw = getpwnam(pcUser)) && getuid() == ppw->pw_uid) {
		(void)strcat(acPath, pcUser);
	} else if ((struct passwd *)0 != (ppw = getpwuid(getuid()))) {
		(void)strcat(acPath, ppw->pw_name);
	} else {
		fprintf(stderr, "%s: getpwuid(%d): %s\n", progname, getuid(), strerror(errno));
		exit(1);
	}

	if (0 != fVerbose && 0 == argc) {
		ShowTar(acPath);
		exit(0);
	}
#if CONFIRM
	if (!isatty(fileno(stdin)) || !isatty(fileno(stdout))) {
		/* do not check, we are not interactive */;
	} else if (0 == stat(acPath, & stat_buf)) {
		printf("%s: overwrite previously submitted project? [yn] ", progname);
		if ((char *)0 != GetWord(acYes, MAXCHARS, "yes") && ! ('y' == acYes[0] || 'Y' == acYes[0]))
			exit(0);
	} else {
		(void)strcat(acPath, acDotZ);
		if (0 == stat(acPath, & stat_buf)) {
			printf("%s: overwrite previously submitted project? [yn] ", progname);
			if ((char *)0 != GetWord(acYes, MAXCHARS, "yes") && ! ('y' == acYes[0] || 'Y' == acYes[0]))
				exit(0);
		}
		acPath[strlen(acPath)-sizeof(acDotZ)+1] = '\000';
	}
#endif
	(void)unlink(acPath);	/* no symlink to another file	*/

		/*
		** trash any former compressed files
		*/
	(void)strcat(acPath, ".Z");
	(void)unlink(acPath);
	acPath[strlen(acPath)-2] = '\000';

	if ((char **)0 == (nargv = (char **)calloc(argc+6, sizeof(char *)))) {
		fprintf(stderr, "%s: out of memory\n", progname);
		exit(18);
	}
	nargv[0] = "tar";
	nargv[1] = "chbf";
	nargv[2] = "1";
	nargv[3] = "-";
	for (i = 0; i < argc; ++i) {	/* copy args for tar	*/
		register char *pcParam = argv[i];

		if ('\000' == *pcParam) {
			fprintf(stderr, "%s: ignored empty parameter\n", progname);
			continue;
		}
		if ('/' == *pcParam) {
			fprintf(stderr, "%s: full pathnames not allowed on submitted files\n", progname);
			exit(19);
		}
		if (('.' == pcParam[0] && '.' == pcParam[1] && '/' == pcParam[2]) || (char *)0 != strstr(pcParam, "/../")) {
			fprintf(stderr, "%s: no component \"..\" allowed in filenames\n", progname);
			exit(19);
		}
		nargv[i+4] = pcParam;
	}

	fflush(stdout);
	fflush(stderr);
	switch (pid = fork()) {			/* child is tar		*/
	case -1:
		fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		exit(21);
	case 0:
		(void)umask(0077);
		close(1);
		if (1 != open(acPath, O_CREAT|O_TRUNC|O_WRONLY, 0600)) {
			fprintf(stderr, "%s: open: %s: %s\n", progname, acPath, strerror(errno));
			exit(22);
		}
		if (-1 == setgid(getgid()) || -1 == setuid(getuid())) {
			fprintf(stderr, "%s: set{g,u}id: %s\n", progname, strerror(errno));
			exit(1);
		}
		execve(acTar, nargv, environ);
		fprintf(stderr, "%s: exec: %s: %s\n", progname, acTar, strerror(errno));
		exit(21);
	default:
		while (-1 != (n = wait(& wait_buf)) && pid != n)
			;
		if (-1 == n) {
			fprintf(stderr, "%s: wait: %s\n", progname, strerror(errno));
			exit(22);
		}
		if (0 != WECODE(wait_buf)) {
			fprintf(stderr, "%s: tar failed (%d)\n", progname, WECODE(wait_buf));
			exit((int)WECODE(wait_buf));
		}
		break;
	}

	/*still instructor here*/
	if (0 != stat(acPath, & stat_buf)) {
		fprintf(stderr, "%s: stat: %s: %s\n", progname, acPath, strerror(errno));
		exit(18);
	}
	if (0 != fVerbose) {
		ShowTar(acPath);
	}
	if (stat_buf.st_size > (off_t) BIGTAR) {
		printf("Compressing submitted files... please wait\n");
		fflush(stdout);
		nargv[0] = "compress";
		nargv[1] = "-f";
		nargv[2] = acPath;
		nargv[3] = (char *)0;
		switch (pid = fork()) {		/* child is compress	*/
		case -1:
			fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
			exit(21);
		case 0:
			(void)umask(0077);
			execve(acCompress, nargv, environ);
			fprintf(stderr, "%s: execve: %s: %s\n", progname, acCompress, strerror(errno));
			exit(21);
		default:
			while (-1 != (n = wait(& wait_buf)) && pid != n)
				;
			if (-1 == n) {
				fprintf(stderr, "%s: wait: %s\n", progname, strerror(errno));
				exit(22);
			}
			if (0 != WECODE(wait_buf)) {
				fprintf(stderr, "%s: compress failed (%d)\n", progname, WECODE(wait_buf));
				exit((int)WECODE(wait_buf));
			}
			break;
		}
	} else {
		/* remove any old, larger file */
		(void) strcat(acPath, acDotZ);
		(void) unlink(acPath);
	}

	printf("Your files have been submitted to %s, ", pcCourse);
	if (isdigit(*pcProj)) {
		printf("project ");
	}
	printf("%s for grading.\n", pcProj);

	(void) endpwent();
	(void) endgrent();
	exit(0);
}
