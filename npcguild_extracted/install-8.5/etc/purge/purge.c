/*
 * a C version of purge(8L)						(ksb)
 *
 * Copyright 1990 Purdue Research Foundation, West Lafayette, Indiana
 * 47907.  All rights reserved.
 *
 * Written by Kevin S Braunsdorf, ksb@cc.purdue.edu, purdue!ksb
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company or the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 *
 * 1. Neither the authors nor Purdue University are responsible for any
 *    consequences of the use of this software.
 *
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Credit to the authors and Purdue
 *    University must appear in documentation and sources.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 4. This notice may not be removed or altered.
 *
 *
 * The old shell version of purge suffered from 4 problems:
 *
 * 1\ it used the -ctime option to find that is local to PUCC
 *
 * 2\ it didn't handle symbolic links correctly
 *
 * 3\ it didn't handle error conditions well (st_nlink > 1)
 *
 * 4\ it was not useful to general users
 *
 * READ THIS!  N.B. if you want to change purge:
 *
 *	Do *NOT* change purge to delete empty OLD dirs.  This will race
 *	with install and cause install to fail. (As is won't build the
 *	OLD dir, the dir will go away, install will try to build a link
 *	in it and fail.)
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>

#include "machine.h"
#include "configure.h"
#include "install.h"
#include "purge.h"
#include "filedup.h"
#include "main.h"

#if HAS_NDIR
#include <ndir.h>
#else
#if HAS_DIRENT
#include <dirent.h>
#else
#include <sys/dir.h>
#endif
#endif

#if STRINGS
#include <strings.h>
#define strchr	index
#define strrchr	rindex
#else
#include <string.h>
#endif

#if !defined(ENAMETOOLONG)
#define ENAMETOOLONG E2BIG
#endif

#if !defined(MAXPATHLEN)
#define MAXPATHLEN	1024
#endif

#ifndef lint
static char copyright[] =
"@(#) Copyright 1990 Purdue Research Foundation.\nAll rights reserved.\n";
#endif

static dev_t
	dDevid;			/* device ID */
static time_t
	tCutOff,		/* the last install time we keep	*/
	tCopyKeep;		/* last temp copy to keep		*/
static char *pcOwner =		/* could be "root" "lroot" or "0"	*/
	ODIROWNER;
static char acOld[] =		/* the name of our OLD dirs		*/
	OLDDIR;
static char acBogus[] =		/* component key for busy links		*/
	TMPBOGUS;
static char acCopy[] =		/* component key for temp copies	*/
	TMPINST;
static FILEDUPS FDLinks;	/* hard links database			*/
static int
	bHaveRoot,		/* are effective uid lets us be root	*/
	iCopy,			/* strlen of acCopy			*/
	iBogus;			/* strlen of acBogus			*/

struct {
	int size;		/* the size of the uid array itself	*/
	int count;		/* number of ids we are keeping		*/
	uid_t *puids;		/* the uid array we are keeping		*/
	char **ppclogins;	/* the login names we found		*/
} ids = {0, 0, (uid_t *)0};

/* this routine should check for all the OLD dir owners			(ksb)
 * (like `root', `news', `ingress', etc)
 */
int
isSysId(uid, uidDotDot)
uid_t uid, uidDotDot;
{
	register int j;

	if (fAnyOwner) {
		return 1;
	}
	/* the only way to get here is is ODIROWNER is (char *)0 and we
	 * are the super user.  Now a note about what a Bad Idea this is,
	 * this will get all the Normal User's directories named "OLD" and
	 * flush files out of them.  I'd never so this (w/o -a) so I'll put
	 * a warning here the first time we get here.
	 */
	if (0 == ids.count) {
		static int fKsbWarns = 0;
		if (!fKsbWarns && uid > 99) {
			fprintf(stderr, "%s: purging for non-system uids (%d)\n", progname, uid);
			fKsbWarns = 1;
		}
		return uid == uidDotDot;
	}
	for (j = 0; j < ids.count; ++j) {
		if (ids.puids[j] == uid) {
			return 1;
		}
	}
	return 0;
}

/* is a string all digits						(ksb)
 */
int
AllDigits(pcTemp)
char *pcTemp;
{
	for (; '\000' != *pcTemp; ++pcTemp) {
		if (!isdigit(*pcTemp))
			return 0;
	}
	return 1;
}

/* this routine adds a new valid OLD dir owner to the list		(ksb)
 */
int
AddHer(pcLogin)
char *pcLogin;
{
	register int j;
	register struct passwd *pwdAdd;

	if (AllDigits(pcLogin)) {
		if ((struct passwd *)0 == (pwdAdd = getpwuid(atoi(pcLogin)))) {
			fprintf(stderr, "%s: getpwuid: %s: %s\n", progname, pcLogin, strerror(errno));
			return 1;
		}
	} else if ((struct passwd *)0 == (pwdAdd = getpwnam(pcLogin))) {
		fprintf(stderr, "%s: getpwname: %s: %s\n", progname, pcLogin, strerror(errno));
		return 1;
	}
	if (ids.size < ids.count+1) {
		ids.size += 8;
		if ((uid_t *)0 == ids.puids) {
			ids.puids = (uid_t *)malloc(ids.size*sizeof(uid_t));
		} else {
			ids.puids = (uid_t *)realloc((char *)ids.puids, ids.size*sizeof(uid_t));
		}
		if ((char **)0 == ids.ppclogins) {
			ids.ppclogins = (char **)malloc(ids.size*sizeof(char *));
		} else {
			ids.ppclogins = (char **)realloc((char *)ids.ppclogins, ids.size*sizeof(char *));
		}
		if ((uid_t *)0 == ids.puids || (char **)0 == ids.ppclogins) {
			fprintf(stderr, "%s: out of memory in realloc\n", progname);
			exit(2);
		}
	}
	for (j = 0; j < ids.count; ++j) {
		if (ids.puids[j] != pwdAdd->pw_uid) {
			continue;
		}
		return 0;
	}
	ids.puids[j] = pwdAdd->pw_uid;
	ids.ppclogins[j] = pcLogin;
	++ids.count;
	return 0;
}

/* this routine added the default user (the envoker) to the OLD		(ksb)
 * owners list (set up the time flags too)
 */
void
InitAll()
{
	register char *pcUser, *pcX;
	struct passwd *pwdUser;
	extern char *getenv();

	/* remove the XXXXX from the mktemp templates
	 */
	pcX = acBogus + (sizeof(acBogus)-2);
	while ('X' == *pcX)
		*pcX-- = '\000';
	pcX = acCopy + (sizeof(acCopy)-2);
	while ('X' == *pcX)
		*pcX-- = '\000';
	iBogus = strlen(acBogus);
	iCopy = strlen(acCopy);

	FDInit(& FDLinks);
	if (0 == getuid())
		fSuperUser = 1;
	(void)time(&tCutOff);
	tCopyKeep = tCutOff - ((time_t)(60*60*24));
	tCutOff -= ((time_t)(60*60*24))*iDays;

	(void)setpwent();

	bHaveRoot = 0 == geteuid();

#if defined(INST_FACILITY)
	if (bHaveRoot && fExec) {
		openlog(progname, 0, INST_FACILITY);
	}
#endif
	if (fSuperUser) {
		if ((char *)0 != pcOwner) {
			(void)AddHer(pcOwner);
		}
		return;
	}

	pcUser = getenv("USER");
	if ((char *)0 == pcUser || '\000' == pcUser[0]) {
		pcUser = getenv("LOGNAME");
	}
	if ((char *)0 == pcUser || '\000' == pcUser[0]) {
		if ((struct passwd *)0 == (pwdUser = getpwuid(getuid()))) {
			fprintf(stderr, "%s: getpwuid: %d: %s\n", progname, getuid(), strerror(errno));
			exit(1);
		}
		pcUser = pwdUser->pw_name;
	}
	pcUser = strcpy(malloc((strlen(pcUser)|7)+1), pcUser);
	(void)AddHer(pcUser);
}


/* show the user what we are compiled to do, as best we can		(ksb)
 */
int
Version()
{
	register int i;

	printf("%s: $Id: purge.c,v 8.4 1997/02/16 21:02:05 ksb Exp $\n", progname);
	printf("%s: default backup directory owner: %s\n", progname, (char *)0 == pcOwner ? "inherited" : pcOwner);
	printf("%s: backup directory name: %s\n", progname, acOld);
	if (fAnyOwner) {
		printf("%s: purge any backup directory\n", progname);
	} else if (0 == ids.count) {
		printf("%s: backup directory owner must match parent\n", progname);
	} else {
		printf("%s: valid backup directory owner", progname);
		if (ids.count > 1)
			printf("s");
		for (i = 0; i < ids.count; ++i)
			printf("%c %s", i == 0 ? ':' : ',', ids.ppclogins[i]);
		printf("\n");
	}
#if defined(INST_FACILITY)
	printf("%s: syslog facility %d\n", progname, INST_FACILITY);
#endif
	return 0;
}


/* get the user-level name for this node `plain file' or `socket'	(ksb)
 *	return string, ref-out single char
 */
char *
NodeType(mType, pcChar)
unsigned int mType;
char *pcChar;
{
	auto char acWaste[2];

	if ((char *)0 == pcChar)
		pcChar = acWaste;

	switch (mType & S_IFMT) {
	case S_IFDIR:
		*pcChar = 'd';
		return "directory";
#if defined(S_IFSOCK)
	case S_IFSOCK:
		*pcChar = 's';
		return "socket";
#endif	/* no sockets */
#if defined(S_IFIFO)
	case S_IFIFO:
		*pcChar = 'p';
		return "fifo";
#endif
#if HAVE_SLINKS
	case S_IFLNK:
		*pcChar = 'l';
		return "symbolic link";
#endif
	case S_IFBLK:
		*pcChar = 'b';
		return "block device";
	case S_IFCHR:
		*pcChar = 'c';
		return "character device";
	case S_IFREG:
	case 0:
		*pcChar = '-';
		return "plain file";
	default:
		break;
	}
	*pcChar = '?';
	return "type unknown";
}

/* avoid putting . and .. in the dirs to check, we would loop		(ksb)
 */
int
DotSel(pDE)
struct DIRENT *pDE;
{
	register char *pcName = pDE->d_name;

	return ! ('.' == pcName[0] && ('\000' == pcName[1] || '.' == pcName[1] && '\000' == pcName[2]));
}

/* we find an OLD directory that is on the wrong device			(ksb)
 */
void
WrongDev(pcDir)
char *pcDir;
{
#if defined(INST_FACILITY)
	if (bHaveRoot) {
		syslog(LOG_WARNING, "%s is not on the same device as its parent", pcDir);
	}
#endif
	fprintf(stderr, "%s: %s is not on the same device as its parent\n", progname, pcDir);
}

/* remove the file, or fake the removal					(ksb)
 */
void
Remove(pcFile)
char *pcFile;
{
	if (fVerbose) {
		printf("%s: rm -f %s\n", progname, pcFile);
	}
	if (fExec && -1 == unlink(pcFile)) {
#if defined(INST_FACILITY)
		if (EBUSY == errno && fExec && bHaveRoot) {
			syslog(LOG_INFO, "still busy `%s\'", pcFile);
		}
#endif
		fprintf(stderr, "%s: unlink: %s: %s\n", progname, pcFile, strerror(errno));
		return;
	}
#if defined(INST_FACILITY)
	if (fExec && bHaveRoot) {
		syslog(LOG_INFO, "rm `%s\'", pcFile);
	}
#endif
}

/* if the file is a temp file used by install ignore it			(ksb)
 * for at least 1 day, then remove it.
 * else (maybe) fix the mode
 * We need the last component of the file name.
 */
int
TooOld(pcFull, pcLast, pstThis)
char *pcFull, *pcLast;
struct stat *pstThis;
{
	register int mMode;

	/* install won't leave setuid/setgid files in OLD dirs,
	 * if we find one (that is not a link to an installed
	 * file) we should drop the bits for the same reasons
	 * install does it.
	 */
	if (0 != ((S_ISGID|S_ISUID|S_ISVTX)&pstThis->st_mode)) {
		mMode = pstThis->st_mode;
		mMode &= ~(S_IFMT|S_ISUID|S_ISGID|S_ISVTX);
		if (fVerbose) {
			printf("%s: chmod %03o %s\n", progname, mMode, pcFull);
		}
		if (fExec && -1 == chmod(pcFull, mMode)) {
			fprintf(stderr, "%s: chmod: %s: %s\n", progname, pcFull, strerror(errno));
		}
	}

	if (0 == strncmp(pcLast, acCopy, iCopy)) {
		if (tCopyKeep <= pstThis->st_ctime) {
			return 0;
		}
	} else if (tCutOff <= pstThis->st_ctime) {
		return 0;
	}

	Remove(pcFull);
	return 1;
}

/* purge an OLD directory of outdated backup files			(ksb)
 * remove bogus links on any age
 * do not touch #inst$$ files unless ctime is > boot time?
 */
void
Purge(pcOld)
char *pcOld;
{
	register struct DIRENT *pDE;
	register int i;
	auto struct DIRENT **ppDEDir;
	auto char acFile[MAXPATHLEN+2], *pcLast;
	auto int iLen;
	auto struct stat stFile;

	iLen = MAXPATHLEN - strlen(pcOld);
	if (iLen <= 0) {
		fprintf(stderr, "%s: Purge: %.40s... %s\n", progname, pcOld, strerror(ENAMETOOLONG));
		return;
	}
	(void)strcpy(acFile, pcOld);
	pcLast = acFile + (MAXPATHLEN - iLen);
	if ('/' != pcLast[-1])
		*pcLast++ = '/', --iLen;
	if (-1 == (i = scandir(pcOld, &ppDEDir, DotSel, (int (*)())0))) {
		fprintf(stderr, "%s: scandir: %s: %s\n", progname, pcOld, strerror(errno));
		return;
	}
	for (; i > 0; free((char *)pDE)) {
		pDE = ppDEDir[--i];
		if (D_NAMELEN(pDE) > iLen) {
			fprintf(stderr, "%s: Purge: ...%s/%.20s... %s\n", progname, ((20 < MAXPATHLEN - iLen) ? pcOld : pcLast-20), pDE->d_name, strerror(ENAMETOOLONG));
			continue;
		}
		(void)strcpy(pcLast, pDE->d_name);
		if (-1 == STAT_CALL(acFile, & stFile)) {
			if (errno != ENOENT) {
				fprintf(stderr, "%s: stat: %s: %s\n", progname, acFile, strerror(errno));
			}
			continue;
		}
		switch (stFile.st_mode & S_IFMT) {
		default:
			fprintf(stderr, "%s: %s: is a %s?\n", progname, acFile, NodeType(stFile.st_mode, (char *)0));
			continue;
#if HAVE_SLINKS
		case S_IFLNK:
#endif
		case S_IFREG:
		case 0:
			break;
		}
		/* if the file is a bogus link to a running program,
		 * try to remove it always...
		 */
		if (0 == strncmp(pcLast, acBogus, iBogus)) {
			Remove(acFile);
			continue;
		}

		/* sticky problem -- we should not remove the evidence of a
		 * poor install (the product might still be installed in a
		 * binary directory) so we need to keep this node here...
		 * but if we find it again we can remove it, see?
		 * Later StillBad() will check the file again for links, etc.
		 *
		 * NB: hard links to OLD dirs will fry us here because we
		 * assume that (since we don't look through symlinks, the
		 * spelling of a path to a file is unique.  I guess NFS mount
		 * points hurt us too... ZZZ
		 */
		if (stFile.st_nlink > 1) {
			register char *pcLink;

			pcLink = FDAdd(&FDLinks, acFile, &stFile);
			if (0 != strcmp(acFile, pcLink)) {
				Remove(acFile);
			}
			continue;
		}

		TooOld(acFile, pcLast, &stFile);
	}
	free((char *)ppDEDir);
}

/* scan under a directory of all the OLD dirs and purge them		(ksb)
 */
void
Scan(pcDir, pstDir)
char *pcDir;
struct stat *pstDir;
{
	register struct DIRENT *pDE;
	register int i;
	auto struct DIRENT **ppDEDir;
	auto char acDown[MAXPATHLEN+2], *pcLast;
	auto struct stat stDown;
	auto int iLen;

	if ((char *)0 == pcDir || '\000' == *pcDir)
		pcDir = ".";
	iLen = MAXPATHLEN - strlen(pcDir);
	if (iLen <= 0) {
		fprintf(stderr, "%s: Scan: %.40s... %s\n", progname, pcDir, strerror(ENAMETOOLONG));
		return;
	}
	(void)strcpy(acDown, pcDir);
	pcLast = acDown + (MAXPATHLEN - iLen);
	if ('/' != pcLast[-1])
		*pcLast++ = '/', --iLen;
	if (-1 == (i = scandir(pcDir, &ppDEDir, DotSel, (int (*)())0))) {
		fprintf(stderr, "%s: scandir: %s: %s\n", progname, pcDir, strerror(errno));
		return;
	}

	/* if the entry exists, look at the other end if it is a link,
	 * if the name is `OLD' and the owner we know purge it
	 * other dirs, recurse
	 */
	for (; i > 0; free((char *)pDE)) {
		pDE = ppDEDir[--i];
		if (D_NAMELEN(pDE) > iLen) {
			fprintf(stderr, "%s: Scan: ...%s/%.20s... %s\n", progname, ((20 < MAXPATHLEN - iLen) ? pcDir : pcLast-20), pDE->d_name, strerror(ENAMETOOLONG));
			continue;
		}
		(void)strcpy(pcLast, pDE->d_name);

		if (-1 == STAT_CALL(acDown, & stDown)) {
			if (errno != ENOENT) {
				fprintf(stderr, "%s: stat: %s: %s\n", progname, acDown, strerror(errno));
			}
			continue;
		}

#if HAVE_SLINKS
		/* if we were a symbolic link, do not have to scan it,
		 * it might ELOOP us, or be a user link to /bin (so we
		 * would purge '/bin'!!
		 * (in other cases we get to it mostly w/o this name anyway)
		 */
		if (S_IFLNK == (stDown.st_mode & S_IFMT)) {
			continue;
		}
#endif

		if (0 == strcmp(acOld, pDE->d_name)) {
			if (stDown.st_dev != pstDir->st_dev) {
				WrongDev(acDown);
			} else if (isSysId((uid_t)stDown.st_uid, pstDir->st_uid)) {
				Purge(acDown);
			}
			continue;
		}
		/* this directory a mount point for a different filesystem? */
		if (dDevid != stDown.st_dev) {
			continue;
		}
		if (S_IFDIR == (stDown.st_mode & S_IFMT)) {
			Scan(acDown, & stDown);
		}
	}
	free((char *)ppDEDir);
}

/* let purge read paths to purge from find(1) output			(ksb)
 */
static void
DoStdin()
{
	static int fBeenHere = 0;
	register char *pcNl;
	auto char acIn[MAXPATHLEN+2];
	auto int c;

	if (fBeenHere++) {
		fprintf(stderr, "%s: will not recurse on stdin (skipped)\n", progname);
		return;
	}
	while ((char *)0 != fgets(acIn, MAXPATHLEN+1, stdin)) {
		if ((char *)0 == (pcNl = strchr(acIn, '\n'))) {
			fprintf(stderr, "%s: line too long on stdin\n", progname);
			while (EOF != (c = getc(stdin))) {
				if ('\n' == c)
					break;
			}
			continue;
		}
		*pcNl = '\000';
		Which(acIn);
	}
	clearerr(stdin);
	--fBeenHere;
}

/* The user may have either given us /bin or /bin/OLD to purge.		(ksb)
 * choose the correct routine and purge it.
 *
 * If an old script gives us a number of days do / with that number (yucko).
 *
 * We should check for a plain file here (before we try to scandir it) and
 * use that notation to purge for a single binary file (this from the TODO
 * file) for example:
 *	# purge /usr/bin/sort
 * Should remove old copies of "sort" from the backout directory /usr/bin/OLD.
 * We might remove "/usr/bin/OLD/sort" and "/usr/bin/OLD/sorta52710" because
 * they are (most likely) backout for "sort".  This would be used after a long
 * beta test cycle to cleanup the program tested w/o cleanup of on going
 * parallel tests.  (Of course.) -- ksb
 *
 * We might look in the binary directory to for a list of prefixes _not_ to
 * match and the one we must before we go looking.  Viz. "/usr/bin/lp" should
 * _not_ * remove backup of "lpstat".
 * [We could look for same mode or magic number as well?]
 *
 * This would limit the bad link cleanup odds a lot.  Instck -i has the code
 * to work for us for just this reason.
 */
void
Which(pcDir)
char *pcDir;
{
	register char *pcSlash;
	auto struct stat stDir, stDot;
	static char acDot[] = ".";

	if (AllDigits(pcDir) && -1 == access(pcDir, 0)) {
		fprintf(stderr, "%s: warning: old style [ndays] option\n", progname);
		iDays = atoi(pcDir);
		pcDir = "/";
	}
	if ('-' == pcDir[0] && '\000' == pcDir[1]) {
		DoStdin();
		return;
	}

	/* we can't just tack on a /.. because we might have
	 * a symbolic link for the OLD dir name, which is OK as
	 * long as the link points to something on our fs.
	 */
	if ((char *)0 == (pcSlash = strrchr(pcDir, '/'))) {
		pcSlash = pcDir;
		if (-1 == stat(acDot, & stDot)) {
			fprintf(stderr, "%s: stat: %s: %s\n", progname, acDot, strerror(errno));
			return;
		}
	} else if (pcSlash == pcDir) {
		if (-1 == stat("/", & stDot)) {
			fprintf(stderr, "%s: stat: /: %s\n", progname, strerror(errno));
			return;
		}
		++pcSlash;
	} else {
		*pcSlash = '\000';
		if (-1 == stat(pcDir, & stDot)) {
			fprintf(stderr, "%s: stat: %s: %s\n", progname, pcDir, strerror(errno));
			return;
		}
		*pcSlash++ = '/';
	}
	if (-1 == stat(pcDir, & stDir)) {
		fprintf(stderr, "%s: stat: %s: %s\n", progname, pcDir, strerror(errno));
		return;
	}
	dDevid = stDir.st_dev;
	if (0 == strcmp(acOld, pcSlash)) {
		if (stDot.st_dev != stDir.st_dev) {
			WrongDev(acOld);
		} else if (!isSysId((uid_t)stDir.st_uid, (uid_t)stDot.st_uid)) {
			fprintf(stderr, "%s: %s: %d: not a user to purge\n", progname, pcDir, stDir.st_uid);
		} else {
			Purge(pcDir);
		}
	} else {
		Scan(pcDir, & stDir);
	}
}

/* this checks to see if we recovered from the errors we found		(ksb)
 * WRT mulitple links to an OLD/backup file.
 */
static int
StillBad(pAE)
AE_ELEMENT *pAE;
{
	auto struct stat stThis;
	register char *pcSlash;

	if (-1 == STAT_CALL(pAE->pcname, &stThis) || stThis.st_nlink == 0) {
		/* some other copy of purge rm'd this for us, how nice
		 */
		return 0;
	}

	/* restore first-seen ctime so we can decide to purge or not
	 */
	stThis.st_ctime = pAE->kctime;
	pcSlash = strrchr(pAE->pcname, '/');
	if (stThis.st_nlink == 1) {
		TooOld(pAE->pcname, (char *)0 != pcSlash ? pcSlash+1: pAE->pcname, &stThis);
	} else if ((char *)0 != pcSlash) {
		*pcSlash = '\000';
		fprintf(stderr, "%s: %s/%s: link count %d, use `instck -i %s\' to repair\n", progname, pAE->pcname, pcSlash+1, stThis.st_nlink, pAE->pcname);
		*pcSlash = '/';
	} else {
		fprintf(stderr, "%s: %s: has a link count > 1, use instck to repair\n", progname, pAE->pcname);
	}
	return 0;
}

/* When done scan any file that have bad link counts and see if we 	(ksB)
 * removed the other links via purging OLD dirs
 */
void
Done()
{
	(void)FDScan(FDLinks, StillBad);

#if defined(INST_FACILITY)
	if (bHaveRoot && fExec) {
		closelog();
	}
#endif
}
