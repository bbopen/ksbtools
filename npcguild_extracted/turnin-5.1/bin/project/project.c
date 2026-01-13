/*
 * project -- reads the turnin database to determin what directory	(ksb)
 * it should build a turnin(1l) directory tree for the given course
 * in.  Then reads a local database for a list of dXsY directories
 * to build.
 *
 *	project [-dehiorvVp] [-c course] [-G cmd] [-g cmd] [number|name]
 *
 * $Compile: ${cc-cc} ${cc_debug--O} -I/usr/include/local %f -o %F -lopt
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <limits.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

#include "machine.h"
#include "main.h"
#include "project.h"

extern int errno;
#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me)	(sys_errlist[Me])
#endif
extern int pclose(), fclose();

#if HAVE_NDIR
#include <ndir.h>
#else /* some other dir structs */

#if HAVE_DIRENT
#include <dirent.h>

#else
#include <sys/dir.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif
#endif

#if HAVE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#if USE_LOCKF || NEED_UNISTD_H
#include <unistd.h>
#endif

extern FILE *popen();
extern char *malloc(), *strchr(), *strrchr();
#define strsave(Mpch) strcpy(malloc(strlen(Mpch)+1), Mpch)

static char acTar[]= 		/* path to tar */
	TAR_PATH;

static char acCompress[]= 		/* path to compress */
	COMPRESS_PATH;

static char RCSId[] =
	"$Id: project.c,v 4.18 1997/11/11 16:28:14 ksb Exp $";

typedef enum {
	ON,
	OFF,
	DELETE
} STATUS;

static STATUS Projstatus[MAXPROJECTS];

WHAT
	What = UNKNOWN;		/* what am I doing			*/

static int
	iProjnum = -1,		/* default project			*/
	iProjcount = 0;		/* number of projects			*/

static struct passwd
	*ppwMe;			/* more deatails about the owner	*/

static char
	acMd[MAXDIVSEC+1],	/*	%d	div/sec as in (dXsX)	*/
	acMp[MAXPROJNAME+1],	/*	%p	the project number	*/
	acMu[MAXLOGINNAME+1],	/*	%u	uid of student		*/
	aacProjnames[MAXPROJNAME+1][MAXPROJECTS];
				/* names of projects			*/

static char
	acLine[MAXCHARS],	/* all these point in here		*/
	*pcUid,			/* who -> uid				*/
	*pcDir,			/* who -> where				*/
	*pcGroup,		/* who -> group				*/
	*pcSections,		/* who -> sections			*/
	acSubmit[] = "submit",	/* default turnin directory		*/
	acLock[] = LOCKFILE,	/* keep lock a file, many projects loose*/
	acMailAddr[] = MAILTO,	/* sys admin's address			*/
	acSectIgnore[] =	/* section that means ``none given''	*/
		SECTIGNORE,
	acTurnbase[] =		/* data base of project co-ordinators	*/
		TURNBASE,
	acDeadLetter[] =	/* maildrop for dead mail		*/
		"dead.letter";


/*
 * reads a configuration file of the form:			(doc/ksb)
 * course:alpha login:subdir for this course:alpha gid:sections\n
 */
int
getcourse(pcBase)
char *pcBase;
{
	register FILE *fpDB;
	register char *pc;
	register int line;
	register char *pcMaybe;
	static char acError[] = "%s: %s(%d): error in data base\n";

	if (0 == (fpDB = fopen(pcBase, "r"))) {
		fprintf(stderr, "%s: fopen: %s: %s\n", progname, pcBase, strerror(errno));
		exit(errno);
	}

	line = 0;
	while (NULL != fgets(acLine, MAXCHARS, fpDB)) {
		++line;
		pcMaybe = acLine;
		while (isspace(*pcMaybe) && '\n' != *pcMaybe)
			++pcMaybe;
		if ('#' == *pcMaybe || '\n' == *pcMaybe)
			continue;
		if (NULL == (pc = strchr(acLine, SEP))) {
			fprintf(stdout, acError, progname, pcBase, line);
			exit(16);
		}
		*pc++ = '\000';
		pcUid = pc;
		if (NULL == (pc = strchr(pc, SEP))) {
			fprintf(stdout, acError, progname, pcBase, line);
			exit(17);
		}
		*pc++ = '\000';
		pcDir = pc;
		if (NULL == (pc = strchr(pc, SEP))) {
			fprintf(stdout, acError, progname, pcBase, line);
			exit(18);
		}
		*pc++ = '\000';
		pcGroup = pc;
		if (NULL == (pc = strchr(pc, SEP))) {
			fprintf(stdout, acError, progname, pcBase, line);
			exit(19);
		}
		*pc++ = '\000';
		pcSections = pc;
		if (NULL != (pc = strchr(pc, '\n'))) {
			*pc = '\000';
		}
		if ((char *)0 != pcCourse && 0 == strcmp(pcCourse, pcMaybe)) {
			return 1;
		}
		if ((char *)0 == pcCourse && 0 == strcmp(pcUid, ppwMe->pw_name)) {
			pcCourse = pcMaybe;
			return 1;
		}
	}
	return 0;
}

/*
 * remove the trailing newline from a fgets'd input line		(ray)
 */
static void
fixup(string)
register char *string;
{
	char *newline;

	if (NULL != (newline = strchr(string, '\n'))) {
		*newline = '\000';
	}
}

static char acProjlist[] =		/* path to project list		*/
	PROJLIST;

/*
 * check for illegal chars in project name				(ksb)
 */
void
CheckChars(pcFrom, pcProj)
char *pcFrom, *pcProj;
{
	if ('\000' == *pcProj) {
		fprintf(stderr, "%s: empty project name given %s\n", progname, pcFrom);
		exit(8);
	}
	do {
		switch (*pcProj) {
		case '/':
		case '.':
			fprintf(stderr, "%s: project name given %s cannot contain `%c\'.\n", progname, pcFrom, *pcProj);
			exit(8);
		default:
			break;
		}
	} while ('\000' != *++pcProj);
}


/*
 * open projlist & read it						(ksb)
 */
int
getprojects()
{
	register FILE *fp;
	auto int status;
	auto int proj;

	if (NULL == (fp = fopen(acProjlist, "r"))) {
		if (fExec) {
			if (NULL == (fp = fopen(acProjlist, "w"))) {
				fprintf(stderr, "%s: fopen: %s: %s\n", progname, acProjlist, strerror(errno));
				exit(errno);
			}
			(void)fclose(fp);
		}
		iProjcount = 0;
		return;
	}
	proj = 0;
	while (EOF != (status = getc(fp))) {
		switch (status) {
		case '-':
			(void)fgets(aacProjnames[proj], MAXCHARS, fp);
			Projstatus[proj] = OFF;
			break;
		case '+':
			(void)fgets(aacProjnames[proj], MAXCHARS, fp);
			Projstatus[proj] = ON;
			break;
		case '=':
			(void)fgets(aacProjnames[proj], MAXCHARS, fp);
			Projstatus[proj] = ON;
			iProjnum = proj;
			break;
		default:
			fprintf(stderr, "%s: `%s\' is hosed\n", progname, acProjlist);
			exit(20);
			break;
		}
		fixup(aacProjnames[proj]);
		CheckChars("in `Projlist\'", aacProjnames[proj]);
		proj++;
	}
	iProjcount = proj;
	(void)fclose(fp);
}

/*
 * find a project in the in-core database				(ksb)
 */
int
search(name)
register char *name;
{
	int i;

	for (i = 0; i < iProjcount; ++i) {
		if (0 == strcasecmp(name, aacProjnames[i])) {
			return i;
		}
	}

	++iProjcount;
	Projstatus[i] = ON;
	(void)strcpy(aacProjnames[i], name);
	return i;
}

/*
 * update the PROJLIST file					    (doc/ksb)
 */
static void
newprojects(fWarn)
int fWarn;	/* warn of no default project. */
{
	register FILE *fp;
	register int projout;
	register int flag1 = 0;
	register int flag2 = 0;

	if (fExec) {
		if (NULL == (fp = fopen(acProjlist, "w"))) {
			fprintf(stderr, "%s: fopen: %s: %s\n", progname, acProjlist, strerror(errno));
			exit(1);
		}
	}

	for (projout = 0; projout < iProjcount; projout++) {
		if (Projstatus[projout] != DELETE) {
			if (fExec) {
				fprintf(fp, "%c%s\n",
					Projstatus[projout] == OFF ? '-' :
					iProjnum == projout ? '=' : '+',
					aacProjnames[projout]);
			}
			if (fVerbose) {
				fprintf(stdout, "echo %c%s %s %s\n",
					Projstatus[projout] == OFF ? '-' :
					iProjnum == projout ? '=' : '+',
					aacProjnames[projout],
					flag2 == 0 ? ">" : ">>",
					acProjlist);
				flag2 = 1;
			}
			if (projout == iProjnum && Projstatus[projout] == ON) {
				flag1 = 1;
			}
		}
	}
	if (fExec) {
		(void)fclose(fp);
	}
	if (flag1 == 0 && fWarn) {
		fprintf(stderr, "%s: no default project\n", progname);
	}
}

/*
 * chdir to the argument, create it if you can't find it		(ksb)
 */
static void
ceedee(pcMove)
char *pcMove;
{
	if (0 != chdir(pcMove)) {
		if (fExec) {
			if (0 != mkdir(pcMove, 0755) || 0 != chdir(pcMove)) {
				fprintf(stderr, "%s: chdir: %s: %s\n", progname, pcMove, strerror(errno));
				exit(1);
			}
		}
		if (fVerbose) {
			fprintf(stdout, "mkdir %s\n", pcMove);
		}
	}
	if (fVerbose) {
		fprintf(stdout, "cd %s\n", pcMove);
	}
}

/*
 * you don't want to look at thew next four blocks			(ksb)
 */
static char acComma[] = ",.";
static char *_pc = acComma+1;

static void
trav_init()
{
	_pc = pcSections;
}

static int
traverse(pcBuf)
char *pcBuf;
{
	extern char *strchr();
	register char *p;

	if (_pc == acComma+1)
		return 0;

	if ((char *)0 == (p = strchr(_pc, ','))) {
		p = acComma;
	}
	*p = '\000';
	(void)strcpy(pcBuf, _pc);
	*p = ',';
	_pc = p+1;

	return 1;
}

/*
 * close off traverse
 */
static void
travisty()
{
	;
}

/*
 * get the file lock for us						(ksb)
 */
int
OpenLock()
{
	auto int fdLock;

	if (-1 == (fdLock = open(acLock, O_RDWR|O_CREAT, 0600))) {
		fprintf(stderr, "%s: open: %s: %s\n", progname, acLock, strerror(errno));
		exit(1);
	}

#if USE_LOCKF
	if (lockf(fdLock, F_TLOCK, 0) == -1) {
		if (errno == EAGAIN || errno == EACCES) {
			fprintf(stderr, "%s: waiting for lock\n", progname);
			if (lockf(fdLock, F_LOCK, 0) == -1) {
				goto bad_lock;
			}
			fprintf(stderr, "%s: got lock\n", progname);
		} else {
	bad_lock:
			fprintf(stderr, "%s: lockf: %s\n", progname, strerror(errno));
			exit(2);
		}
	}
#else
	if (-1 == flock(fdLock, LOCK_NB|LOCK_EX)) {
		if (EWOULDBLOCK == errno) {
			fprintf(stderr, "%s: waiting for lock\n", progname);
			if (-1 == flock(fdLock, LOCK_EX)) {
				goto bad_lock;
			}
			fprintf(stderr, "%s: got lock\n", progname);
		} else {
	bad_lock:
			fprintf(stderr, "%s: flock: %s\n", progname, strerror(errno));
			exit(1);
		}
	}
#endif

#if defined(F_SETFD)
	if (-1 == fcntl(fdLock, F_SETFD, 1)) {
		fprintf(stderr, "%s: fcntl: %s (ignored)\n", progname, strerror(errno));
	}
#endif
	return fdLock;
}

/*
 * make the project tree (if it doesn't exist)				(ksb)
 */
static void
Make(pcProj)
char *pcProj;
{
	auto char acDivSec[MAXDIVSEC];

	ceedee(pcProj);
	if (0 != strcmp(acSectIgnore, pcSections)) {
		trav_init();
		while (traverse(acDivSec)) {
			if (fVerbose) {
				fprintf(stdout, "mkdir %s\n", acDivSec);
			}
			if (!fExec) {
				continue;
			}
			if (0 != mkdir(acDivSec, 0755) && EEXIST != errno) {
				fprintf(stderr, "%s: mkdir: %s\n", progname, strerror(errno));
				exit(2);
			}
		}
		travisty();
	}
	ceedee("..");
}

/*
 * fork off a UNIX rm to recursively remove unloved files		(ksb)
 */
static void
rm(pcFile)
char *pcFile;
{
	int iPid;
	int iRmPid;
	WAIT_T status;

	if (fVerbose) {
		fprintf(stdout, "rm -rf %s\n", pcFile);
	}
	if (fExec) {
		if (0 == (iRmPid= fork())) {
			execl("/bin/rm", "rm", "-rf", pcFile, 0);
			fprintf(stderr, "%s: execl: /bin/rm: %s\n", progname, strerror(errno));
			exit(1);
		}

		while (-1 != (iPid= wait(&status))) {
			if (iPid == iRmPid) {
				if (0 == WECODE(status))
					break;

				fprintf(stderr, "%s rm failed %d\n", progname, WECODE(status));
					break;
			}
			continue;
		}
	}
}

/*
 * remove the dir struct for a project with some rm's			(ksb)
 */
static void
Remove(pcProj)
char *pcProj;
{
	auto char acDivSec[MAXDIVSEC];

	if (0 != chdir(pcProj)) {		/* no exist, don't make */
		return;
	}
	(void)chdir("..");
	ceedee(pcProj);		/* for verbose mode	*/
	if (0 != strcmp(acSectIgnore, pcSections)) {
		trav_init();
		while (traverse(acDivSec)) {
			rm(acDivSec);
		}
		travisty();
	}
	ceedee("..");
	rm(pcProj);
}

/*
 * like strcat, but return value is end of string			(doc)
 */
static char *
stradd(pcTo, pcFrom)
register char *pcFrom, *pcTo;
{
	while (*pcTo = *pcFrom++)
		++pcTo;
	return pcTo;
}

/*
 * using the global state, produce a shell cammand an system it off	(ksb)
 *
 * escapse in command:
 *	%u	uid of student
 *	%d	division/section is (dXsX)
 *	%p	the project number
 *	%t	submissions directory
 *	%h	home of grader
 *	%s	full path %h/%t/%p/%d/%u
 *	%%	a %
 */
static void
grade(pcFrom)
register char *pcFrom;
{
	static char *pcExec = (char *)0;
	register char *pcTo;

	if ((char *)0 == pcExec) {
		pcExec = malloc(NCARGS);
		if ((char *)0 == pcExec) {
			fprintf(stderr, "%s: out of memory\n", progname);
			exit(errno);
		}
	}
	pcTo = pcExec;
	while ('\000' != pcFrom[0]) {
		if ('%' == pcFrom[0]) {
			++pcFrom;
			switch (*pcFrom++) {
			case '%':
				*pcTo++ = '%';
				continue;
			case 'u':
				pcTo = stradd(pcTo, acMu);
				break;
			case 'd':
				pcTo = stradd(pcTo, acMd);
				break;
			case 'p':
				pcTo = stradd(pcTo, acMp);
				break;
			case 't':
				pcTo = stradd(pcTo, pcDir);
				break;
			case 'h':
				pcTo = stradd(pcTo, ppwMe->pw_dir);
				break;
			case 's':
				pcTo = stradd(pcTo, ppwMe->pw_dir);
				*pcTo++ = '/';
				pcTo = stradd(pcTo, pcDir);
				*pcTo++ = '/';
				pcTo = stradd(pcTo, acMp);
				*pcTo++ = '/';
				pcTo = stradd(pcTo, acMd);
				*pcTo++ = '/';
				pcTo = stradd(pcTo, acMu);
				break;
			case '\000':
				fprintf(stderr, "%s: grade: command ends in %%?", progname);
				exit(1);
			default:
				fprintf(stderr, "%s: grade: `%c\' unknown %% escape\n", progname, pcFrom[-1]);
				exit(1);
			}
		} else {
			*pcTo++ = *pcFrom++;
		}
	}
	*pcTo = '\000';

	if (fVerbose) {
		printf("%s\n", pcExec);
	}

	if (fExec) {
		if (0 != system(pcExec)) {
			fprintf(stderr, "%s: command '%s' failed.\n", progname, pcExec);
		}
	}
}

/* scan a directory for tar files to grade				(ksb)
 */
scandiv(student)
DIR *student;
{
	register struct DIRENT *dpstudent;
	register char *pcDot;

	while (NULL != (dpstudent = readdir(student))) {
		if ('.' == dpstudent->d_name[0])	/* skip . */
			continue;
		(void)strcpy(acMu, dpstudent->d_name);
		if ((char *)0 == pcCmd) {
			/* do nothing */;
		} else if ((char *)0 != (pcDot = strrchr(acMu, '.')) && 'Z' == pcDot[1] && '\000' == pcDot[2]) {
			*pcDot = '\000';
			grade("zcat %u.Z >%u");
			grade(pcCmd);
			grade("rm -f %u");
			*pcDot = '.';
		} else {
			grade(pcCmd);
		}
	}
	closedir(student);
	if ((char *)0 != pcSum) {
		(void)strcpy(acMu, "*");
		grade(pcSum);
	}
}

/*
 * scan the project directory and system off the commands		(ksb)
 * (with grade())
 */
void
scan()
{
	register struct DIRENT *dp;
	register DIR *proj, *student;

	ceedee(acMp);
	proj = opendir(".");
	if (0 == strcmp(pcSections, acSectIgnore)) {
		(void)strcpy(acMd, ".");
		scandiv(proj);
	} else {
		while (NULL != (dp = readdir(proj))) {
			if ('.' == dp->d_name[0]) /* dot entries skipped */
				continue;
			ceedee(dp->d_name);
			(void)strcpy(acMd, dp->d_name);
			student = opendir(".");
			scandiv(student);
			ceedee("..");
		}
		closedir(proj);
	}
	ceedee("..");
}

/*
 * get the words of wisdom from the user				(ksb)
 * strip leading white space, etc
 */
char *
GetWord(pcBuf, iLen, pcDef)
char *pcBuf, *pcDef;
int iLen;
{
	register char *pcWhite, *pcCopy;
	register int l;

	(void)fflush(stdout);
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
 * is a name a poor name for a course or subdirectory			(ksb)
 */
int
BadName(pc)
char *pc;
{
	if ('\000' == *pc)
		return 1;
	for (; '\000' != *pc; ++pc) {
		if (!isalnum(*pc) && *pc != '-' && *pc != '_')
			return 1;
	}
	return 0;
}

/*
 * build the project managers data structures in the file system	(ksb)
 * also mail off a request to be added to the database
 * we must be in the users home directory here!
 * (Return the locked fd that keeps other projects blocked)
 */
int
DoInit()
{
	auto char acBuf[MAXCHARS+1], acCmd[MAXPATHLEN+300],
		acShort[MAXCOURSENAME+1], acDir[MAXSUBDIRNAME+1],
		acGroup[MAXGROUPNAME+1], acDefGroup[MAXGROUPNAME+1],
		acLong[MAXLONGNAME+1],
		acTemp[MAXDIVSEC+1], acDiv[MAXDIVSEC+1],
		acPrevExp[MAXYESNO+1], *pcComma;
	auto struct group *pgr;
	auto FILE *fpMail;
	auto int fdLock, cnt, div, sec, fProc, fFound;
	auto struct stat stDir;
	extern struct group *getgrgid(), *getgrnam();

	(void)setgrent();
	if ((struct group *)0 == (pgr = getgrgid(getegid()))) {
		fprintf(stderr, "%s: getgrgid: %d: %s\n", progname, getegid(), strerror(errno));
		exit(1);
	}
	(void)strcpy(acDefGroup, pgr->gr_name);

	printf("\nEnabling %s to accept electronic submissions\n", ppwMe->pw_name);
	printf("\n\
Project allows other users to put files in your account.  Please supply\n\
the information requested below so that your account can be enabled to\n\
accept submissions from turnin(1L).  If you have questions or need help\n\
ask a general consultant or send mail to \"%s\".\n", acMailAddr);
	printf("\n\
If you make a mistake you do not see a way to correct simply\n\
finish this run and run the program again.\n");

	for (;;) {
		printf("\nAre you sure you want to do this? [ny] ");
		if ((char *)0 == GetWord(acBuf, MAXCHARS, "no"))
			exit(0);
		switch (acBuf[0]) {
		case 'q':
		case 'Q':
		case 'N':
		case 'n':
			exit(0);
		case 'y':
		case 'Y':
			break;
		default:
			continue;
		}
		break;
	}

	if (0 != stat(".", & stDir)) {
		fprintf(stderr, "%s: stat: .: %s\n", progname, strerror(errno));
		exit(2);
	}
	if ((stDir.st_mode & 0755) != 0755) {
		printf("\nYour home directory *must* be kept world readable so that\n");
		printf("students and consultants can use turnin -v to see if they have\n");
		printf("submitted anything.\n");
		if (0 != chmod(".", (int)(stDir.st_mode | 0755))) {
			fprintf(stderr, "%s: chmod: .: %s\n", progname, strerror(errno));
			exit(2);
		}
	} else {
		printf("\nYour home directory is already world readable.  Please keep it that way\n");
		printf("so that students and consultants can use turnin -v.\n");
	}

	printf("\nHave you ever used %s to administer electronic submissions before? [ny] ", progname);
	if ((char *)0 == GetWord(acPrevExp, MAXYESNO, "none"))
		exit(0);

	do {
		printf("\nA long name for a course might be something like \n\tProgramming in Mud\n");
		printf("\nWhat is the long name of this course? ");
		if ((char *)0 == GetWord(acLong, MAXLONGNAME, ""))
			exit(0);
	} while ('\000' == acLong[0]);

	for (;;) {
		do {
			printf("\nA short name for a course might be something like \n\tcs200\n");
			printf("\nWhat is the short name of this course [%s]? ", acDefGroup);

			if ((char *)0 == GetWord(acShort, MAXCOURSENAME, acDefGroup))
				exit(0);
		} while ('\000' == acShort);
		if (strlen(acShort) > MAXCOURSENAME) {
			printf("Pick a shorter name, please.\n");
			continue;
		}
		if (BadName(acShort)) {
			printf("Course names cannot have special characters in them (except '-').\n");
			continue;
		}
		break;
	}

	pcCourse = acShort;
	pcUid = (char *)0;
	fFound = getcourse(acTurnbase);
	if (fFound) {
		printf("Course `%s' already exists owner %s.%s in %s", acShort, pcUid, pcGroup, pcDir);
		if (0 != strcmp(acSectIgnore, pcSections))
			printf("/{%s}", pcSections);
		printf(".\n");
	} else {
		pcDir = acSubmit;
		pcGroup = acDefGroup;
		pcSections = (char *)0;
	}

	for (;;) {
		printf("\nAll submitted files will be under a special subdirectory in your account.\n");
		printf("This directory will be world readable.\n");
		printf("\nWhat do you want your submission directory named? [%s] ", pcDir);
		if ((char *)0 == GetWord(acDir, MAXSUBDIRNAME, pcDir))
			exit(0);
		if (!BadName(acDir)) {
			break;
		}
		printf("The directory name must be alpha-numeric.\n");
	}

	printf("\nThe submitted files will not be group readable, but should have an\n");
	printf("appropriate group none the less.  For most people the default is fine.\n");
	for (;;) {
		printf("\nWhich group should the submitted files be in ? [%s] ", pcGroup);
		if ((char *)0 == GetWord(acGroup, MAXGROUPNAME, pcGroup)) {
			exit(0);
		}
		if ((struct group *)0 != (pgr = getgrnam(acGroup))) {
			break;
		}
		printf("getgrname: %s: not found\n", acGroup);
	}

	ceedee(acDir);
	fdLock = OpenLock();	/* closed by exit or caller */

	sprintf(acCmd, "%s -s \"%s: request for \\`%s'\" %s", MAILX_PATH, progname, acShort, acMailAddr);
	if (!fExec) {
		if ((FILE *)0 == (fpMail = fopen("/dev/null", "w"))) {
			fprintf(stderr, "%s: fopen: /dev/null: %s\n", progname, strerror(errno));
			exit(1);
		}
		fProc = 0;
	} else if ((FILE *)0 == (fpMail = popen(acCmd, "w"))) {
		fprintf(stderr, "%s: cannot mail to (%s).\n", progname, strerror(errno));
		if ((FILE *)0 == (fpMail = fopen(acDeadLetter, "a"))) {
			exit(1);
		}
		fprintf(stderr, "%s: holding letter in %s\n", progname, acDeadLetter);
		fProc = 0;
	} else {
		fProc = 1;
	}
	fprintf(fpMail, "Entry for `%s\' requested as follows:\n", acLong);
	fprintf(fpMail, "\t%s%c%s%c%s%c%s", acShort, SEP, ppwMe->pw_name, SEP, acDir, SEP, acGroup);

	printf("\n\
You can separate a large course into a few smaller parts for grading by\n\
assigning division/section identifiers.\n\n");
	printf("Three common naming conventions are listed below:\n");
	printf("\tmorning\t\tafternoon\n");
	printf("\t930scott\t230kevin\n");
	printf("\td1s1\t\td2s1\n\n");
	printf("Choose the type your students can best remember.\n");
	printf("\nEnter the sections of your course one per line.\n");
	printf("Use a dot (`.\') on a line by itself to stop.\n");
	div = 1;
	sec = 1;
	cnt = 0;
	pcComma = pcSections;
	for (;;) {
		if ((char *)0 == pcComma || '\000' == pcComma) {
			sprintf(acTemp, "d%ds%d", div, sec);
		} else {
			register int i;
			for (i = 0; i < MAXDIVSEC; ++i) {
				if ('\000' == *pcComma) {
					pcComma = (char *)0;
					break;
				}
				if (',' == *pcComma) {
					++pcComma;
					break;
				}
				acTemp[i] = *pcComma++;
			}
			acTemp[i] = '\000';
		}
		printf("section [%s] > ", acTemp);
		if ((char *)0 == GetWord(acDiv, MAXDIVSEC, acTemp)) {
			fprintf(fpMail, "\n\n!! Aborted !!\n\n");
			(fProc ? pclose : fclose)(fpMail);
			exit(0);
		}
		if ('.' == acDiv[0] && '\000' == acDiv[1]) {
			if (0 == cnt) {
				fprintf(fpMail, "%c%s", SEP,  acSectIgnore);
				++cnt;
			}
			break;
		}
		if (BadName(acDiv)) {
			printf("Divisions must have alpha-numeric names.\n");
			continue;
		}
		fprintf(fpMail, "%c%s", (0 == cnt ? SEP : ','), acDiv);
		++cnt;

		/* make d<div>s<sec> easy to do for Purdue
		 */
		if ('d' == acDiv[0] && isdigit(acDiv[1])) {
			register char *pc;
			div = atoi(acDiv+1);
			if ((char *)0 != (pc = strchr(acDiv+2, 's')) && isdigit(pc[1])) {
				sec = atoi(pc+1);
			}
		}
		if (10 == ++sec) {
			++div;
			sec = 1;
		}
	}

	printf("\nLastly, if you would like to be notified by phone when this is done\n");
	printf("Please enter a phone number now: [No, Thanks] ");
	(void) GetWord(acLong, MAXLONGNAME, "[No Phone Call Requested]");

	if (0 != strcmp(acDefGroup, acGroup)) {
		fprintf(fpMail, "\n(default group was %s)", acDefGroup);
	}
	fprintf(fpMail, "\n\nPhone: %s\n", acLong);
	fprintf(fpMail, "\nPrevious experience: %s", acPrevExp);
	fprintf(fpMail, "\nDatabase file: `%s\'", acTurnbase);
	if (fFound) {
		fprintf(fpMail, "\nPrevious database line:\n\t%s:%s:%s:%s:%s", pcCourse, pcUid, pcDir, pcGroup, pcSections);
	}

	(fProc ? pclose : fclose)(fpMail); /* works for fopen too, now */

	endgrent();

	printf("\nMail %s been sent to a superuser to finish the configuration.\n", fExec ? "has" : "would have" );
	if (fExec) {
		printf("You will be contacted via email or telephone when your account\n");
		printf("is ready for use.\n");
	}
	return fdLock;
}


/*
 * send the sysadmin mail telling her to remove us from the db		(ksb)
 */
static void
DoQuit()
{
	auto char acCmd[MAXPATHLEN+300];
	auto int fProc;
	auto FILE *fpMail;

	sprintf(acCmd, "%s -s \"%s: please delete \\`%s'\" %s", MAILX_PATH, progname, pcCourse, acMailAddr);

	if ((FILE *)0 == (fpMail = popen(acCmd, "w"))) {
		fprintf(stderr, "%s: cannot open mail cmd (%s).\n", progname, strerror(errno));
		if ((FILE *)0 == (fpMail = fopen(acDeadLetter, "a"))) {
			exit(1);
		}
		fprintf(stderr, "%s: holding letter in %s\n", progname, acDeadLetter);
		fProc = 0;
	} else {
		fProc = 1;
	}
	fprintf(fpMail, "\nPlease remove %s from the project database, `%s\'.\n", pcCourse, acTurnbase);
	fprintf(fpMail, "\nThanks.\n");
	(fProc ? pclose : fclose)(fpMail);
	printf("\nMail %s been sent to a superuser to finish the job.\n", fExec ? "has" : "would have" );
}


static char *apcEsc[] =	 {	/* an escape help message		*/
	"cmd: may include any of %[htpdus%] which will be expanded before execution",
	"%h	home directory of instructor",
	"%t	turnin directory name",
	"%p	current project number",
	"%d	division/section identifier",
	"%u	current user identifier, or `*\'",
	"%s	full path (%h/%t/%p/%d/%u) to tar file",
	"%%	a %",
	(char *)0
};

/* output advanced help info
 */
Advanced(fp)
FILE *fp;
{
	register char **ppc;

	for (ppc = apcEsc; (char *)0 != *ppc; ++ppc)
		fprintf(fp, "%s\n", *ppc);
}


/*
** pack Submit Directory			(becker)
**
** In shell syntax packSD() basically does the following:
**
**	tar -cf - proj | compress -f > proj.tar.Z
**
** where proj is the name of the current project.
**
** If the pipeline succeeds, the tar file is written and
** the submit directory (+ contents) is removed.
*/
void packSD(pcMp, iProj)
char *pcMp;
int iProj;
{
	int fdPipe[2];			/* pipe between tar and compress */
	int iTarPid;			/* pid of the tar child */
	int iZPid;				/* pid of the compress child */
	int iPid;
	WAIT_T status;
	int fdTarOut;			/* fd of the final output file */
	char acTarDotZ[MAXPROJNAME+7];	/* file name of the final output file */


	if (-1 == access(pcMp, F_OK)) {
		fprintf(stderr, "%s: %s %s\n", progname, pcMp, strerror(errno));
		exit(30);
	}

	if (OFF != Projstatus[iProj]) {
		fprintf(stderr, "%s: Project must be disabled before packing.\n", progname);
		exit(31);
	}

	if (-1 == pipe(fdPipe)) {
		fprintf(stderr, "%s: pipe %s\n", progname, strerror(errno));
		exit(32);
	}

	(void)strcpy(acTarDotZ, pcMp);
	(void)strcat(acTarDotZ, ".tar.Z");

	if (fVerbose)
		fprintf(stdout, "tar -cf %s | compress -f > %s\n", pcMp, acTarDotZ);

	if (fExec) {
		switch (iTarPid = fork()) {
		case -1:
			fprintf(stderr, "%s: fork %s\n", progname, strerror(errno));
			exit(33);
			break;

		case 0:		/* child does tar */
			if (-1 == dup2(fdPipe[1], 1)) {
				fprintf(stderr, "%s: dup2 %s\n", progname, strerror(errno));
				exit(1);
			}
			(void)close(fdPipe[0]);
			(void)close(fdPipe[1]);

			execl(acTar, "tar", "-cf", "-", pcMp, (char *)0);
			fprintf(stderr, "%s: execl %s %s\n", progname, acTar, strerror(errno));
			exit(2);
			break;

		default:
			switch (iZPid = fork()) {
			case -1:
				fprintf(stderr, "%s: fork %s\n", progname, strerror(errno));
				rm(acTarDotZ);
				exit(34);
				break;

			case 0:		/* second child does compress */
				if (-1 == (fdTarOut = open(acTarDotZ, O_CREAT|O_WRONLY, 0600))) {
					fprintf(stderr, "%s: open %s\n", progname, strerror(errno));
					exit(1);
				}

				if (-1 == dup2(fdPipe[0], 0) || -1 == dup2(fdTarOut, 1)) {
					fprintf(stderr, "%s: dup2 %s\n", progname, strerror(errno));
					exit(2);
				}

				(void)close(fdPipe[0]);
				(void)close(fdPipe[1]);
				(void)close(fdTarOut);

				execl(acCompress, "compress", "-f", "-", (char *)0);
				fprintf(stderr, "%s: execl %s %s\n", progname, acCompress, strerror(errno));
				rm(acTarDotZ);
				exit(3);
				break;

			default:
				(void)close(fdPipe[0]);
				(void)close(fdPipe[1]);
				break;
			}

			break;
		}


			/*
			** wait for children to finish
			*/
		while (-1 != (iPid = wait(&status))) {
			if (iPid == iTarPid) {
				if (0 != WECODE(status)) {
					fprintf(stderr, "%s: tar failed (%d)\n", progname, WECODE(status));
					rm(acTarDotZ);
					exit(35);
				}
				continue;
			}

			if (iPid == iZPid) {
				if (0 != WECODE(status)) {
					fprintf(stderr, "%s: compress failed (%d)\n", progname, WECODE(status));
					rm(acTarDotZ);
					exit(36);
				}
				continue;
			}
		}
	}

#if HAVE_PUTENV
	(void)putenv("ENTOMB=no");
#else
	(void)setenv("ENTOMB", "no");
#endif
	rm(pcMp);
}


/*
** unpack Submit Directory					(becker)
**
** If a submit directory (project) has been tarred and compressed
** then this routine will uncompress and "untar" it.
*/
void unpackSD(pcMp, iProj)
char *pcMp;
int iProj;
{
	int fdPipe[2];			/* pipe between zcat and tar */
	int iTarPid;			/* pid of the tar child */
	int iZCatPid;			/* pid of the zcat child */
	int iPid;
	WAIT_T status;
	char acTarDotZ[MAXPROJNAME+7];	/* file name of the input file */

	(void)strcpy(acTarDotZ, pcMp);
	(void)strcat(acTarDotZ, ".tar.Z");

	if (-1 == access(acTarDotZ, F_OK))
		return;

	if (OFF != Projstatus[iProj]) {
		fprintf(stderr, "%s: something's hosed here....\n", progname);
		exit(37);
	}

	if (-1 == pipe(fdPipe)) {
		fprintf(stderr, "%s: pipe %s\n", progname, strerror(errno));
		exit(38);
	}

	if (fVerbose)
		fprintf(stdout, "zcat %s | tar -cf -\n", acTarDotZ);

	if (fExec) {
		switch (iZCatPid = fork()) {
		case -1:
			fprintf(stderr, "%s: fork %s\n", progname, strerror(errno));
			exit(39);
			break;

		case 0:		/* child does zcat */
			if (-1 == dup2(fdPipe[1], 1)) {
				fprintf(stderr, "%s: dup2 %s\n", progname, strerror(errno));
				exit(1);
			}
			(void)close(fdPipe[0]);
			(void)close(fdPipe[1]);

			execl(ZCAT_PATH, "zcat", acTarDotZ, (char *)0);
			fprintf(stderr, "%s: execl zcat %s\n", progname, strerror(errno));
			exit(2);
			break;

		default:
			switch (iTarPid = fork()) {
			case -1:
				fprintf(stderr, "%s: fork %s\n", progname, strerror(errno));
				exit(40);
				break;

			case 0:		/* second child does tar */
				if (-1 == dup2(fdPipe[0], 0)) {
					fprintf(stderr, "%s: dup2 %s\n", progname, strerror(errno));
					exit(1);
				}
				(void)close(fdPipe[0]);
				(void)close(fdPipe[1]);

				execl(acTar, "tar", "-xf", "-", (char *)0);
				fprintf(stderr, "%s: execl %s %s\n", progname, acTar, strerror(errno));
				exit(2);
				break;

			default:
				(void)close(fdPipe[0]);
				(void)close(fdPipe[1]);
				break;
			}

			break;
		}


			/*
			** wait for children to finish
			*/
		while (-1 != (iPid = wait(&status))) {
			if (iPid == iTarPid) {
				if (0 != WECODE(status)) {
					fprintf(stderr, "%s: tar failed (%d)\n", progname, WECODE(status));
					exit(41);
				}
				continue;
			}

			if (iPid == iZCatPid) {
				if (0 != WECODE(status)) {
					fprintf(stderr, "%s: zcat failed (%d)\n", progname, WECODE(status));
					exit(42);
				}
				continue;
			}
		}
	}

#if HAVE_PUTENV
	(void)putenv("ENTOMB=no");
#else
	(void)setenv("ENTOMB", "no");
#endif
	rm(acTarDotZ);
}



/*
 * read turnin data base						(ksb)
 * make a guess at the course
 * which project, on?  default?
 * make tar file
 * maybe display it
 */
int
main(argc, argv)
int argc;
char **argv;
{
	static int fGiven = 0;
	auto char acTop[MAXPATHLEN+1];
	auto char acYes[MAXYESNO+1];
	auto int i;
	auto int iTemp, fdLock;

	(void)umask(022);

	if ((char *)0 == (progname = strrchr(argv[0], '/')))
		progname = argv[0];
	else
		++progname;

	(void) setpwent();
	if (NULL == (ppwMe = getpwuid(getuid()))) {
		fprintf(stderr, "%s: getpwuid: %d: %s\n", progname, getuid(), strerror(errno));
		exit(99);
	}

	options(argc, argv);
	if (What == INITIALIZE) {
		printf("%s: initialize user %s for a course\n", progname, ppwMe->pw_name);
		if (-1 == chdir(ppwMe->pw_dir)) {
			fprintf(stderr, "%s: chdir: %s: %s\n", progname, ppwMe->pw_dir, strerror(errno));
			exit(2);
		}
		fdLock = DoInit();
		iProjnum = 0;
		Projstatus[0] = OFF;
		newprojects(0);
		/* reply mail should include instructions for enabling projects
		 */
		(void) close(fdLock);
		exit(0);
	}

	/* we change ppwMe below to reflect root becoming the user
	 */
	if (NULL == (ppwMe = getpwuid(geteuid()))) {
		fprintf(stderr, "%s: getpwuid: %d: %s\n", progname, geteuid(), strerror(errno));
		exit(99);
	}
	if (0 == getcourse(acTurnbase)) {
		if ((char *)0 == pcCourse)
			fprintf(stderr, "%s: user %s not in database\n", progname, ppwMe->pw_name);
		else
			fprintf(stderr, "%s: course %s not in database\n", progname, pcCourse);
		exit(1);
	}
	/* if we are root, become Prof here (which might be `lroot')
	 */
	if ((struct passwd *)0 == (ppwMe = getpwnam(pcUid))) {
		fprintf(stderr, "%s: getpwname: %s: %s\n", progname, pcUid, strerror(errno));
		fprintf(stderr, "%s: no such course here\n", progname);
		exit(1);
	}
	if (-1 == setuid(ppwMe->pw_uid)) {
		fprintf(stderr, "%s: setuid: %s\n", progname, strerror(errno));
		exit(20);
	}

	if (0 == geteuid()) {
		fprintf(stderr, "%s: run as root [ny]? ", progname);
		(void)fflush(stderr);
		(void) GetWord(acYes, MAXYESNO, "no");
		if ('y' != acYes[0] && 'Y' != acYes[0]) {
			fprintf(stderr, "Aborted.\n");
			exit(0);
		}
	}
	sprintf(acTop, "%s/%s", ppwMe->pw_dir, pcDir);
	ceedee(acTop);

	fdLock = OpenLock();

	getprojects();

	if ((char *)0 != pcProject) {
		fGiven = 1;
		CheckChars("on the command line", pcProject);
		iTemp = search(pcProject);
	} else {
		iTemp = iProjnum;
	}

	if (-1 != iTemp) {
		(void)strcpy(acMp, aacProjnames[iTemp]);
	}

	switch (What) {
	case PACKSD:
		if (-1 == iTemp) {
			fprintf(stderr, "%s: no default project\n", progname);
			exit(1);
		}
		packSD(acMp, iTemp);
		break;

	case ENABLE:
		if (-1 == iTemp) {
			fprintf(stderr, "%s: no default project\n", progname);
			exit(1);
		}
		unpackSD(acMp, iTemp);
		iProjnum = iTemp;
		Projstatus[iTemp] = ON;
		Make(acMp);
		newprojects(1);
		break;

	case QUIT:
		fprintf(stdout, "Are you sure you want to stop using %s [ny]? ", progname);
		(void) GetWord(acYes, MAXYESNO, "no");
		if ('y' != acYes[0] && 'Y' != acYes[0]) {
			fprintf(stdout, "Aborted.\n");
			exit(0);
		}
		for (i = 0; i < iProjcount; ++i) {
			if (DELETE == Projstatus[i]) {
				continue;
			}
			(void)strcpy(acMp, aacProjnames[i]);
			fprintf(stdout, "%s: remove project %s [ynq] ", progname, acMp);
			(void) GetWord(acYes, MAXYESNO, "yes");
			if ('q' == acYes[0] || 'Q' == acYes[0]) {
				fprintf(stdout, "Aborted.\n");
				exit(0);
			}
			if ('y' != acYes[0] && 'Y' != acYes[0]) {
				continue;
			}
			Remove(aacProjnames[i]);
			Projstatus[i] = DELETE;
		}
		for (i = 0; i < iProjcount; ++i) {
			if (DELETE != Projstatus[i]) {
				break;
			}
		}
		if (i != iProjcount) {
			printf("%s: not all projects removed, quit.\n", progname);
			newprojects(1);
			break;
		}
		ceedee("..");
		if (fExec) {
			DoQuit();
		}
		rm(pcDir);
		break;

	case REMOVE:
		if (-1 == iTemp) {
			fprintf(stderr, "%s: no default project\n", progname);
			exit(1);
		}
		fprintf(stdout, "Are you sure you want to delete all submissions for project %s [ny]? ", acMp);
		(void) GetWord(acYes, MAXYESNO, "no");
		if ('y' != acYes[0] && 'Y' != acYes[0]) {
			fprintf(stdout, "Aborted.\n");
			exit(0);
		}
		Remove(acMp);
		Projstatus[iTemp] = DELETE;
		newprojects(1);
		break;

	case DISABLE:
		if (-1 == iTemp) {
			fprintf(stderr, "%s: no default project\n", progname);
			exit(1);
		}
		Projstatus[iTemp] = OFF;
		newprojects(1);
		break;

	case GRADE:
		if (-1 == iTemp) {
			fprintf(stderr, "%s: no default project\n", progname);
			exit(1);
		}
		unpackSD(acMp, iTemp);
#if HAVE_NICE
		nice(2);
#else /* vax */
		setpriority(PRIO_PROCESS, getpid(), 2);
#endif /* machine */
		scan();
		break;

	case OUTPUT:
		fprintf(stdout, "%s: user: %s.%s, course: %s, subdirectory: %s\n", progname, ppwMe->pw_name, pcGroup, pcCourse, pcDir);
		for (i = 0; i < iProjcount; ++i) {
			fprintf(stdout, "%s: project %-8s (%s)\n", progname, aacProjnames[i], (OFF == Projstatus[i] ? "off" : iProjnum == i ? "default" : "on"));
		}
		break;

	case LATE:
		if (-1 == iTemp) {
			fprintf(stderr, "%s: no default project\n", progname);
			exit(1);
		}
		unpackSD(acMp, iTemp);
		if (iTemp == iProjnum) {
			iProjnum = -1;
		}
		Projstatus[iTemp] = ON;
		Make(acMp);
		newprojects(1);
		break;

	case UNKNOWN:
		if (fGiven) {
			Make(acMp);
			newprojects(1);
			break;
		}
		/*fallthrough*/
	default:
		fprintf(stderr, "%s: use -h for help\n", progname);
		break;
	}

	(void) endpwent();
	(void) close(fdLock);
	exit(0);
}
