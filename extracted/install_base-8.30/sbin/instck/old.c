/*
 * Copyright 1994 Purdue Research Foundation, West Lafayette, Indiana
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
 */

/* These routines check the backup directoies we hold -1 revisions	(ksb)
 * of installed files in.  The stage directory (often called `OLD')
 * has some nice features:
 *	+ OLD is on the same device as it's ..
 *	+ no node under OLD is anything but a plain (or contiguous) file
 *	+ no file under OLD is setuid (setgid) or sticky,
 *	+ all backup files have a link count of one
 *	+ no file under OLD is so precious that we can't remove it if
 *	  all the above are true
 */
#if !defined(lint)
static char rcsid[] =
	"$Id: old.c,v 8.35 2009/12/29 16:35:55 ksb Exp $";
#endif	/* !lint */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>
#include <unistd.h>

#include "machine.h"
#include "configure.h"
#include "install.h"
#include "main.h"
#include "syscalls.h"
#include "special.h"
#include "instck.h"
#include "gen.h"
#include "path.h"

#if HAVE_SGSPARAM
#include <sgsparam.h>
#endif

#if HAVE_NDIR
#include <ndir.h>
#else
#if HAVE_DIRENT
#if USE_SYS_DIRENT_H
#include <sys/dirent.h>
#else
#include <dirent.h>
#endif
#else
#include <sys/dir.h>
#endif
#endif

#include "backups.h"
#include "old.h"

#if STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#if !defined(nil)
#define nil	((char *)0)
#endif

/* We have to find a non-existant name for the file we are moving.	(ksb)
 * This is important so we try lots of names:
 *	$COMP		${COMP}XXXXXX		${COMP%%??????}XXXXXX
 *	#instXXXXXX	#upXXXXXX
 * N.B. we are in the destination directory.
 */
static char *
GuessName(pcOut, pcComp)
char *pcOut, *pcComp;
{
	static char acX[] = "XXXXXX";
	register int iLen;
	auto struct stat stOut;

	(void)strcpy(pcOut, pcComp);
	if (-1 == STAT_CALL(pcOut, & stOut) && ENOENT == errno) {
		return pcOut;
	}
	(void)strcat(pcOut, acX);
	if ((char *)0 != mktemp(pcOut))
		return pcOut;

	/* back off a little and tack on the X's */
	(void)strcpy(pcOut, pcComp);
	iLen = strlen(pcOut);
	if (iLen > sizeof(acX)-1) {
		iLen -= sizeof(acX)-1;
		(void)strcpy(pcOut+iLen, acX);
		if ((char *)0 != mktemp(pcOut))
			return pcOut;
	}

	(void)strcpy(pcOut, "#instck");
	(void)strcat(pcOut, acX);
	if ((char *)0 != mktemp(pcOut))
		return pcOut;

	(void)strcpy(pcOut, "#up");
	(void)strcat(pcOut, acX);
	if ((char *)0 != mktemp(pcOut))
		return pcOut;
	return (char *)0;
}

/* return 1 if the directory is empty, else 0				(ksb)
 */
int
IsEmpty(pDI)
DIR *pDI;
{
	register struct DIRENT *pDE;

	while ((struct DIRENT *)0 != (pDE = readdir(pDI))) {
		if ('.' == pDE->d_name[0] && ('\000' == pDE->d_name[1] ||
		   ('.' == pDE->d_name[1] && '\000' == pDE->d_name[2])))
			continue;
		break;
	}
	return (struct DIRENT *)0 == pDE;
}

/* We found a directory under an OLD directory, install won't		(ksb)
 * do this -- but novice super users and install scripts might.
 * cwd is pcIn, pcDir is the name in . for the directory and pstDir
 * is the usual stat structure (for pcDir).
 *
 * If the directory is empty we have little regret about it's loss.
 * Otherwise we should count the nodes in the directory (hey, compared to
 * the 1200 line of code we run for hard link repairs, this is sane) by
 * type (f,-, l, c/b, d, n,s) and take action based on the sums.
 *
 * If we find only plain (| contig) files (and symbolic links?) --
 *	then offer to move them up.
 * If we find non-empty subdirs, char/block specials, sockets or the like --
 *	then we'd better offer to cpio up the directory
 *	and rm -rf it (or do nothing at all).
 *
 * Clever inv.s:
 *	+ pstDir->st_nlink is >2 if we have subdirs (we don't have to
 *	  scandir to know this)
 *	+ pcDir could be "foo/bar" here as long as "foo" is going to be
 *	  checked after us (recursive calls)
 * return 1 for forced rescan of pcIn because we renamed or added files.
 * return 0 for no change or removed pcDir only
 */
static int
ClobberDir(pcIn, pcDir, pstDir, fDown)
char *pcIn, *pcDir;
struct stat *pstDir;
int fDown;	/* 'n' (no), '?' (ask user), 'y' (yes) */
{
	static char *apcPlural[2] = { "s", "" };
	auto int iRmdir, fWhack;
	auto char acArchive[MAXPATHLEN+2], acBackFile[MAXPATHLEN+2];
	auto struct stat stIn;
	register DIR *pDI;
	register struct DIRENT *pDE;
	register char *pcTail;
	register int iNot, iPlain, iSpecial, iFifo;
	register long int liDir;
#if HAVE_SYMLINKS
	register int iSlink;
#endif

	/* if we can't open it we should not rmdir it, I think.
	 */
	if ((DIR *)0 == (pDI = opendir(pcDir))) {
		fprintf(stderr, "%s: opendir: %s/%s: %s\n", progname, pcIn, pcDir, strerror(errno));
		return 0;
	}

	if (2 == pstDir->st_nlink && IsEmpty(pDI)) {
		closedir(pDI);
		apcRmdir[2] = pcDir;
		apcRmdir[3] = nil;
		(void)QExec(apcRmdir, "an empty subdirectory of a backup directory", &iRmdir);
		return 0;
	}
	/* the only tactic we'll take non=interactive is "empty" removed
	 */
	if (!fInteract) {
		closedir(pDI);
		return 0;
	}

	if (2 < pstDir->st_nlink && '?' == fDown) {
		auto int iAns, fKeepYesYes;

		fprintf(fpOut, "%s: backup subdirectory %s/%s has %ld descendent director%s\n", progname, pcIn, pcDir, (long)pstDir->st_nlink-2, 3 == pstDir->st_nlink ? "y" : "ies");
		fprintf(fpOut, "%s: should we descend to purge all files from this directory?", progname);
		fKeepYesYes = fYesYes;
		iAns = fYesYes;
		fYesYes = 0;
		if (0 != YesNo(stdin, & iAns)) {	/* 'f' */
			closedir(pDI);
			fYesYes |= fKeepYesYes;
			return 0;
		}
		fDown = iAns ? 'y' : 'n';
		fYesYes |= fKeepYesYes;
	}

	/* stat the participants for a herd (head?) count
	 */
	fWhack = 0;
	(void)rewinddir(pDI);
	pcTail = strcpy(acArchive, pcDir);
	pcTail += strlen(pcTail);
	liDir = iNot = iPlain = iSpecial = iFifo = 0;
#if HAVE_SYMLINKS
	iSlink = 0;
#endif
	*pcTail++ = '/';
	while ((struct DIRENT *)0 != (pDE = readdir(pDI))) {
		if ('.' == pDE->d_name[0] && ('\000' == pDE->d_name[1] ||
		   ('.' == pDE->d_name[1] && '\000' == pDE->d_name[2])))
			continue;
		(void)strcpy(pcTail, pDE->d_name);
		if (-1 == STAT_CALL(acArchive, & stIn)) {
			++iNot;
			continue;
		}
		switch (stIn.st_mode & S_IFMT) {
		/* stuff we can handle */
		case S_IFREG:
#if defined(S_IFCTG)
		case S_IFCTG:
#endif
			++iPlain;
			continue;
#if HAVE_SYMLINKS
		case S_IFLNK:
			++iSlink;
			continue;
#endif
		/* trash files */
#if defined(S_IFNWK)
		case S_IFNWK:
#endif
#if defined(S_IFDOOR)
		case S_IFDOOR:
#endif
		case S_IFIFO:
		case S_IFSOCK:
			++iFifo;
			continue;
		/* bad things */
		case S_IFBLK:
		case S_IFCHR:
			++iSpecial;
			continue;
		case S_IFDIR:
			if ('n' != fDown) {
				fWhack |= ClobberDir(pcIn, acArchive, &stIn, fDown);
			}
			if (-1 != STAT_CALL(acArchive, & stIn) || ENOENT != errno) {
				++liDir;
			}
			continue;
		default:
			break;
		}
		fprintf(stderr, "%s: %s/%s: unknown file type\n", progname, pcIn, acArchive);
		++iNot;
	}
	*--pcTail = '\000';
	/* if we can't see it all at this level, quit
	 */
	if (0 != iNot || -1 == STAT_CALL(pcDir, pstDir)) {
		return fWhack;
	}
	if (pstDir->st_nlink != (liDir + 2)) {
		fprintf(stderr, "%s: %s/%s failed internal link count sanity check (%ld != %ld)\n", progname, pcIn, pcDir, (long)pstDir->st_nlink, (long)(liDir + 2));
		return fWhack;
	}
	/* if all any specials or dirs we just bail
	 */
	if (0 != iSpecial || 0 != liDir) {
		fprintf(fpOut, "%s: clean %s/%s by hand, please.\n", progname, pcIn, pcDir);
		return fWhack;
	}

	fprintf(fpOut, "%s: backup subdirectory %s/%s has %d file%s, %d fifo%s", progname, pcIn, pcDir, iPlain, apcPlural[1 == iPlain], iFifo, apcPlural[1 == iFifo]);
#if HAVE_SYMLINKS
	fprintf(fpOut, ", %d symlink%s", iSlink, apcPlural[1 == iSlink]);
#endif
	fprintf(fpOut, "\n");

	/* loop through to cleanup:
	 *	remove symlinks, or move into OLD
	 *	move files (and contigs) into OLD, or unlink them
	 *	remove fifo, socket and "network files" (cuwi bono?)
	 */
	rewinddir(pDI);
	*pcTail++ = '/';
	iNot = 0;
	while ((struct DIRENT *)0 != (pDE = readdir(pDI))) {
		static char acWas[] = "a misplaced backup file";
		register char fPref;	/* D-elete or M-ove */
		register char **ppc1, **ppc2;
		auto int iRan;

		if ('.' == pDE->d_name[0] && ('\000' == pDE->d_name[1] ||
		   ('.' == pDE->d_name[1] && '\000' == pDE->d_name[2])))
			continue;
		(void)strcpy(pcTail, pDE->d_name);
		if (-1 == STAT_CALL(acArchive, & stIn)) {
			++iNot;
			break;
		}

		switch (stIn.st_mode & S_IFMT) {
		/* stuff we can handle */
		case S_IFREG:
#if defined(S_IFCTG)
		case S_IFCTG:
#endif
			fPref = 'm';
			break;
#if HAVE_SYMLINKS
		case S_IFLNK:
			fPref = 'm';
			break;
#endif
		/* trash files */
#if defined(S_IFNWK)
		case S_IFNWK:
#endif
		case S_IFIFO:
		case S_IFSOCK:
			fPref = 'd';
			break;
		/* bad things */
#if defined(S_IFDOOR)
		case S_IFDOOR:
#endif
		case S_IFBLK:
		case S_IFCHR:
		case S_IFDIR:
		default:
			fprintf(stderr, "%s: %s/%s: broken invarient file type\n", progname, pcIn, acArchive);
			++iNot;
			continue;
		}
		if ((char *)0 == GuessName(acBackFile, pDE->d_name)) {
			fprintf(stderr, "%s: no temp name for %s/%s\n", progname, pcIn, pDE->d_name);
			++iNot;
			continue;
		}
		apcRm[2] = "-f";
		apcRm[3] = acArchive;
		apcRm[4] = (char *)0;
		ppc1 = ppc2 = apcRm;
		apcMv[2] = acArchive;
		apcMv[3] = acBackFile;
		apcMv[4] = (char *)0;
		if ('m' == fPref) {
			ppc1 = apcMv;
		} else {
			ppc2 = apcMv;
		}
		switch (QExec(ppc1, acWas, &iRan)) {
		case QE_RAN:
			if (0 == iRan)
				break;
			/*fallthough*/
		case QE_SKIP:
			++iNot;
			break;
		case QE_NEXT:
			if (QE_RAN != QExec(ppc2, acWas, &iRan) || 0 != iRan)
				++iNot;
			break;
		}
	}
	*pcTail-- = '\000';
	if (0 != iNot) {
		return fWhack;
	}
	rewinddir(pDI);
	iRmdir = IsEmpty(pDI);
	closedir(pDI);
	if (iRmdir) {
		apcRmdir[2] = pcDir;
		apcRmdir[3] = nil;
		(void)QExec(apcRmdir, "a reformed subdirectory of a backup directory", &iRmdir);
	}
	return fWhack;
}

/* look in .. for a node, via a scandir for the t/maj/min		(ksb)
 */
static int
NodeLives(pcIn, pcName, pstNode)
char *pcIn, *pcName;
struct stat *pstNode;
{
	register DIR *pDI;
	register struct DIRENT *pDE;
	register char *pcTail;
	auto char acLook[MAXPATHLEN+1];
	auto struct stat stLook;

	if ((DIR *)0 == (pDI = opendir(pcIn))) {
		fprintf(stderr, "%s: opendir: %s: %s\n", progname, pcIn, strerror(errno));
		return 0;
	}
	if ('.' == pcIn[0] && ('\000' == pcIn[1] || ('/' == pcIn[1] && '\000' == pcIn[2]))) {
		acLook[0] = '\000';
		pcTail = acLook;
	} else {
		(void)strcpy(acLook, pcIn);
		pcTail = acLook+strlen(acLook);
	}
	if (pcTail == acLook) {
		/* nothing */
	} else if ('/' != pcTail[-1]) {
		*pcTail++ = '/';
	}
	while ((struct DIRENT *)0 != (pDE = readdir(pDI))) {
		if ('.' == pDE->d_name[0] && ('\000' == pDE->d_name[1] ||
		   ('.' == pDE->d_name[1] && '\000' == pDE->d_name[2])))
			continue;
		(void)strcpy(pcTail, pDE->d_name);
		if (0 != STAT_CALL(acLook, & stLook)) {
			continue;
		}
		if ((stLook.st_mode & S_IFMT) != (pstNode->st_mode & S_IFMT) ||
			stLook.st_dev != pstNode->st_dev)
			continue;
		break;
	}
	(void)closedir(pDI);
	return (struct DIRENT *)0 != pDE;
}


/* We found a node in a backup directory that is not a plain file	(ksb)
 * can we unlink it?  We are *in* pcCurOld, so pDEBad->d_name
 */
static void
NotAFile(pcCurOld, pDEBad, pstBad)
char *pcCurOld;
struct DIRENT *pDEBad;
struct stat *pstBad;
{
	register char *pcWas = "a bad backup node", *pcCursor;
	register int iLen;
	auto char acLink[MAXPATHLEN+200+1];

	(void)fprintf(fpOut, "%s: `%s/%s\' should be a plain file, is a %s\n", progname, pcCurOld, pDEBad->d_name, NodeType(pstBad->st_mode, (char *)0));

	switch (pstBad->st_mode & S_IFMT) {
	default:
	case S_IFREG:
#if defined(S_IFCTG)
	case S_IFCTG:
#endif
		/* how did we get here! */
		return;
#if HAVE_SYMLINKS
	case S_IFLNK:
		/* we should log the readlink(2) output
		 * rm -f node {was a symbolic link to "..."}
		 */
		(void)strcpy(acLink, "was a symbolic link to \"");
		pcCursor = acLink + strlen(acLink);
		if (-1 != (iLen = readlink(pDEBad->d_name, pcCursor, MAXPATHLEN))) {
			pcCursor[iLen++] = '"';
			pcCursor[iLen] = '\000';
			pcWas = acLink;
		}
		break;
#endif
#if defined(S_IFNWK)
	case S_IFNWK:
		fprintf(stderr, "%s: %s/%s: what is a network file anyway?\n", progname, pcCurOld, pDEBad->d_name);
		return;
#endif
#if defined(S_IFDOOR)
	case S_IFDOOR:
#endif
	case S_IFIFO:
	case S_IFSOCK:
		/* I think (if rm -f will take it) this is it OK
		 * if it is busy leave it (how do we get errno?)
		 */
		break;
	case S_IFBLK: /* block special */
	case S_IFCHR: /* character special */
		/* When can we remove a special file (b/c)? (ignore Yesyes?)
		 *	if link count is > 1
		 *	if we can see the same type/maj/min in ..
		 * and we are interactive
		 */
		if (!fInteract) {
			return;
		}
		if ((pstBad->st_nlink > 1) || NodeLives("..", pDEBad->d_name, pstBad)) {
			break;
		}
		return;
	case S_IFDIR:
		fprintf(stderr, "%s: %s: internal flow error!\n", progname, pDEBad->d_name);
		/* ClobberDir(pcCurOld, pDEBad->d_name, pstBad, '?'); */
		return;
	}

	apcRm[2] = pDEBad->d_name;
	apcRm[3] = nil;
	(void)QExec(apcRm, pcWas, (int *)0);
}

static char
	*pcOldOwner = ODIROWNER,
	*pcOldGroup = ODIRGROUP,
	*pcOldMode = ODIRMODE;

/* select the file in OLD which might be interesting			(ksb)
 */
static int
OldSelect(pDE)
const struct DIRENT *pDE;
{
	register const char *pc;

	pc = pDE->d_name;
	if ('.' == pc[0] && ('\000' == pc[1] || ('.' == pc[1] && '\000' == pc[2]))) {
		return 0;
	}
	return 1;
}

/* sort the files in OLD that we might have trouble with		(ksb)
 * files with external links, setuid, setgid, sticky, or the same file
 * more than once.  Or WRONG type!
 */
static int
OldSort(ppDELeft, ppDERight)
const struct DIRENT **ppDELeft, **ppDERight;
{
	auto struct stat stLeft, stRight;
	register int fBadLeft, fBadRight;

	if (ppDELeft[0]->d_ino == ppDERight[0]->d_ino) {
		return 0;
	}

	if (-1 == STAT_CALL(ppDELeft[0]->d_name, & stLeft)) {
		return -1;
	}
	if (-1 == STAT_CALL(ppDERight[0]->d_name, & stRight)) {
		return -1;
	}

	/* should be redundent with d_ino...
	 */
	if (stLeft.st_ino == stRight.st_ino)
		return 0;

	if (stLeft.st_nlink > stRight.st_nlink)
		return -1;
	if (stLeft.st_nlink < stRight.st_nlink)
		return 1;
	fBadLeft = 0 != (07000 & stLeft.st_mode) || S_IFREG != (stLeft.st_mode & S_IFMT);
	fBadRight = 0 != (07000 & stRight.st_mode) || S_IFREG != (stRight.st_mode & S_IFMT);
	if (fBadLeft == fBadRight) {
		return strcmp(ppDELeft[0]->d_name, ppDERight[0]->d_name);
	}
	/* must be bad left, or right */
	return  fBadLeft ? -1 : 1;
}

static char *apcBadEnglish[8] = {
	(char *)0,
	"sticky",			/* Yes, it occured to me that we */
	"setgid",			/* could save a few bytes with   */
	"setgid and sticky",		/* some common tail hacks, or a  */
	"setuid",			/* complex routine to build these */
	"setuid and sticky",		/* from the three words.  This is */
	"setuid and setgid",		/* too simple to break.	-- ksb	*/
	"setuid, setgid and sticky"
};

char
	acOLD[] = OLDDIR;

/* check the OLD dirs for pooly installed files				(ksb)
 * a poorly installed file is one that:
 *	- has more than one link
 *	- is not a plain (or contiguous) file [only MASSCOMP has S_IFCTG]
 *	- mode is setuid [or gid] or sticky
 *	- has flags scgh or the like, if you want those add in -C explicitly
 */
void
OldCk(iCount, ppcDirs, pCLCheck)
int iCount;
char **ppcDirs;
CHLIST *pCLCheck;		/* the checklist buffer DoCk uses	*/
{
	register struct DIRENT *pDE;
	auto struct DIRENT **ppDEList;
	register int i, iFailed;
	auto char acMode[64], acPwd[MAXPATHLEN+1];
	auto char *pcKeep;
	auto struct stat stBackup, stOld, stDir;
	auto struct passwd *pwdOldDef;
	auto struct group *grpOldDef;
	auto int mOldMust = 0, mOldQuest = 0;
	auto int fdDot;

	if (-1 == (fdDot = open(".", O_RDONLY, 000))) {
		fprintf(stderr, "%s: open: .: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	if ((char *)0 == getcwd(acPwd, sizeof(acPwd))) {
		(void)fprintf(stderr, "%s: getcwd: %s\n", progname, strerror(errno));
		exit(EX_NOINPUT);
	}

	if (!bHaveRoot) {
		pwdOldDef = getpwuid(geteuid());
		if ((struct passwd *)0 == pwdOldDef) {
			fprintf(stderr, "%s: getpwuid: %d: %s\n", progname, geteuid(), strerror(errno));
			exit(EX_NOUSER);
		}
		pwdOldDef = savepwent(pwdOldDef);
		pcOldOwner = pwdOldDef->pw_name;
	} else if ((char *)0 == pcOldOwner || '\000' == pcOldOwner[0]) {
		pcOldOwner = (char *)0;
		pwdOldDef = (struct passwd *)0;
	} else {
		pwdOldDef = getpwnam(pcOldOwner);
		if ((struct passwd *)0 == pwdOldDef) {
			fprintf(stderr, "%s: getpwname: %s: %s\n", progname, pcOldOwner, strerror(errno));
			exit(EX_NOUSER);
		}
		pwdOldDef = savepwent(pwdOldDef);
	}
	if (!bHaveRoot) {
		grpOldDef = getgrgid(getegid());
		if ((struct group *)0 == grpOldDef) {
			fprintf(stderr, "%s: getgrgid: %d: %s\n", progname, getegid(), strerror(errno));
			exit(EX_NOUSER);
		}
		grpOldDef = savegrent(grpOldDef);
		pcOldGroup = grpOldDef->gr_name;
	} else if ((char *)0 == pcOldGroup || '\000' == pcOldGroup[0]) {
		pcOldGroup = (char *)0;
		grpOldDef = (struct group *)0;
	} else {
		grpOldDef = getgrnam(pcOldGroup);
		if ((struct group *)0 == grpOldDef) {
			fprintf(stderr, "%s: getgrname: %s: %s\n", progname, pcOldGroup, strerror(errno));
			exit(EX_NOUSER);
		}
		grpOldDef = savegrent(grpOldDef);
	}
	if ((char *)0 == pcOldMode || '\000' == pcOldMode[0]) {
		pcOldMode = (char *)0;
	} else {
		CvtMode(pcOldMode, &mOldMust, &mOldQuest);
	}

	pcKeep = pCLCheck->pcflags;
	pCLCheck->pcflags = (char *)0;
	pCLCheck->pclink = (char *)0;
	pCLCheck->pcmesg = "backup directory";
	for (iFailed = i = 0; i < iCount; ++i) {
		auto char acCurOld[MAXPATHLEN+2];
		register int j, iConsumed, n;

		/* full path to it, relative path or constructed from soil
		 */
		if ('/' == ppcDirs[i][0]) {
			(void)sprintf(acCurOld, "%s/", ppcDirs[i]);
		} else if ('.' == ppcDirs[i][0] && '\000' == ppcDirs[i][1]) {
			(void)sprintf(acCurOld, "%s/", acPwd);
		} else {
			(void)sprintf(acCurOld, "%s/%s/", acPwd, ppcDirs[i]);
		}
		/* If the dir is a symlink we need to modes on the remote
		 * end to clone, not the modes on the link itself	(ksb)
		 */
		if (-1 == stat(acCurOld, & stDir)) {
			(void)fprintf(fpOut, "%s: stat: %s: %s\n", progname, ppcDirs[i], strerror(errno));
			continue;
		}
		(void)strcat(acCurOld, acOLD);

		/* I don't really want OLD dirs to be symbolic links:
		 * if they are and point off-device we should remove
		 * the link (and install -d the directory?), or if the
		 * symbolic link resolves to a non-directory [ouch!]
		 */
		if (-1 == STAT_CALL(acCurOld, & stOld)) {
			if (ENOENT != errno)
				(void)fprintf(stderr, "%s: stat: %s: %s\n", progname, acCurOld, strerror(errno));
			continue;
		}
#if HAVE_SYMLINKS
		/* check off-device symbolic link case, or symlink points
		 * to something other than a directory (ouch)
		 */
		if (S_IFLNK == (stOld.st_mode & S_IFMT)) {
			auto struct stat stRemote;
			auto int iUnlink, iInstall;

			if (-1 == stat(acCurOld, & stRemote)) {
				if (ENOENT != errno) {
					(void)fprintf(stderr, "%s: stat: %s: %s\n", progname, acCurOld, strerror(errno));
					continue;
				}
				(void)fprintf(fpOut, "%s: `%s' points to nothing\n", progname, acCurOld);
			} else if (stOld.st_dev == stRemote.st_dev && S_IFDIR == (stRemote.st_mode & S_IFMT)) {
				/* we check link contents in gen.c
				 */
				continue;
			} else {
				(void)fprintf(fpOut, "%s: `%s' points to %s %s\n", progname, acCurOld, stRemote.st_dev != stOld.st_dev ? "an off-device" : "a", NodeType(stRemote.st_mode, (char *)0));
			}
			if (!fInteract) {
				continue;
			}

			apcRm[2] = "-f";
			apcRm[3] = acCurOld;
			apcRm[4] = nil;
			switch (QExec(apcRm, "a bad symbolic link", &iUnlink)) {
			case QE_RAN:
				/* We are interactive, we might ask
				 * if they want a clean OLD dir built?
				 */
				if (0 == iUnlink) {
					break;
				}
				/*fallthough*/
			case QE_SKIP:
				continue;
			case QE_NEXT:
				/* what else can we do here?
				 */
				continue;
			}
			/* install -dv $acCurOld
			 */
			apcInstall[2] = fVerbose ? "-dv" : "-d";
			apcInstall[3] = acCurOld;
			apcInstall[4] = nil;
			(void)QExec(apcInstall, "missing backup directory", &iInstall);
			continue;
		}
#endif
		/* fix exit code? Make them umount it -- HA!
		 */
		if (stOld.st_dev != stDir.st_dev) {
			(void)fprintf(fpOut, "%s: `%s' is on another device, device of parent directory differs (%ld != %ld)\n", progname, acCurOld, (long)stOld.st_dev, (long)stDir.st_dev);
			/* ask if they'd like us to umount it :-) */
			continue;
		}

		/* check the modes on the OLD directory itself, build a CLnode
		 */
		pCLCheck->mtype = S_IFDIR;
		if (stOld.st_uid == stDir.st_uid) {
			pCLCheck->uid = stDir.st_uid;
		} else if ((char *)0 == pcOldOwner) {
			register struct passwd *pwdTemp;

			pCLCheck->uid = stDir.st_uid;
			pwdTemp = getpwuid(stDir.st_uid);
			if ((struct passwd *)0 != pwdTemp) {
				(void)strcpy(pCLCheck->acowner, pwdTemp->pw_name);
			} else {
				(void)sprintf(pCLCheck->acowner, "%d", stDir.st_uid);
			}
		} else {	/* yeah, this is an invarient */
			pCLCheck->uid = pwdOldDef->pw_uid;
			(void)strcpy(pCLCheck->acowner, pwdOldDef->pw_name);
		}
		if (stOld.st_gid == stDir.st_gid) {
			pCLCheck->gid = stDir.st_gid;
		} else if ((char *)0 == pcOldGroup) {
			register struct group *grpTemp;

			pCLCheck->gid = stDir.st_gid;
			grpTemp = getgrgid(stDir.st_gid);
			if ((struct group *)0 != grpTemp) {
				(void)strcpy(pCLCheck->acgroup, grpTemp->gr_name);
			} else {
				(void)sprintf(pCLCheck->acgroup, "%d", stDir.st_gid);
			}
		} else {
			pCLCheck->gid = grpOldDef->gr_gid;
			(void)strcpy(pCLCheck->acgroup, grpOldDef->gr_name);
		}
		if ((char *)0 == pcOldMode || PERM_BITS(stOld.st_mode) == PERM_BITS(stDir.st_mode)) {
			pCLCheck->mmust = PERM_BITS(stDir.st_mode);
			pCLCheck->moptional = S_ISGID;
		} else {
			pCLCheck->mmust = mOldMust;
			pCLCheck->moptional = mOldQuest;
		}
		pCLCheck->chstrip = CF_NONE;
		(void)NodeType(pCLCheck->mtype, pCLCheck->acmode);
		ModetoStr(pCLCheck->acmode+1, pCLCheck->mmust, pCLCheck->moptional);
		(void)DoCk(acCurOld, &stOld);

		/* goto the directory and seach for poor backups (NO continue)
		 */
		if (-1 == chdir(acCurOld)) {
			if (ENOTDIR != errno)
				(void)fprintf(stderr, "%s: chdir: %s: %s\n", progname, acCurOld, strerror(errno));
			continue;
		}
		if (-1 == (n = scandir(".", &ppDEList, OldSelect, OldSort))) {
			(void)fprintf(stderr, "%s: scandir: %s/.: %s\n", progname, acCurOld, strerror(errno));
		} else for (j = 0; iConsumed = 1, j < n; j += iConsumed) {
			register char *pcChMode;

			if ((struct DIRENT *)0 == (pDE = ppDEList[j])) {
				continue;
			}
			if (-1 == STAT_CALL(pDE->d_name, &stBackup)) {
				(void)fprintf(stderr, "%s: stat: %s/%s: %s\n", progname, acCurOld, pDE->d_name, strerror(errno));
				continue;
			}
#if defined(S_IFCTG)
			if (S_IFCTG == (stBackup.st_mode & S_IFMT)) {
				/* treat as a plain file */
			} else
#endif
			if (S_IFDIR == (stBackup.st_mode & S_IFMT)) {
				auto int fRescan; /*ZZZ*/
				fRescan = ClobberDir(acCurOld, pDE->d_name, &stBackup, '?');
				continue;
			} else if (S_IFREG != (stBackup.st_mode & S_IFMT)) {
				NotAFile(acCurOld, pDE, & stBackup);
				continue;
			}
			if (1 != stBackup.st_nlink) {
				while (j+iConsumed < n && ppDEList[j+iConsumed]->d_ino == pDE->d_ino)
					++iConsumed;
				pDE = FixBackup(ppcDirs[i], acOLD, ppDEList+j, iConsumed, & stBackup);
				if ((struct DIRENT *)0 == pDE)
					continue;
			}
			pcChMode = apcBadEnglish[(stBackup.st_mode & (S_ISUID|S_ISGID|S_ISVTX)) >> 9];
			if ((char *)0 == pcChMode || '\000' == pcChMode[0]) {
				continue;
			}

			(void)fprintf(fpOut, "%s: `%s/%s\' bad backup file, is a %s file in a backup directory\n", progname, acCurOld, pDE->d_name, pcChMode);
			if (!fInteract) {
				continue;
			}
#if BROKEN_CHMOD
			/* We know this is a file and files allow
			 * us to drop the set?id bit with an octal
			 * change, but they (Sun) might break this...
			 */
			SunBotch(acMode, PERM_BITS(stBackup.st_mode), PERM_RWX(stBackup.st_mode));
#else
			(void)sprintf(acMode, "%04o", (unsigned int)PERM_RWX(stBackup.st_mode));
#endif
			apcChmod[2] = acMode;
			apcChmod[3] = pDE->d_name;
			apcChmod[4] = nil;
			(void)QExec(apcChmod, pcChMode, (int *)0);
		}
		if (0 != fchdir(fdDot)) {
			fprintf(stderr, "%s: fchdir: %d: %s\n", progname, fdDot, strerror(errno));
			exit(EX_OSERR);
		}
	}
	pCLCheck->pcflags = pcKeep;
	(void)close(fdDot);
}

/* if we are given an OLD dir, just run the OldCk on it			(ksb)
 */
int
FilterOld(iCount, ppcDirs, pCLCheck)
int iCount;
char **ppcDirs;
CHLIST *pCLCheck;		/* the checklist buffer DoCk uses	*/
{
	register char *pcTail;
	auto int i, j, fCmp, k;
	auto char *pcKeep, **ppcOlds;

	pcKeep = pCLCheck->pcflags;
	pCLCheck->pcflags = (char *)0;
	ppcOlds = (char **)calloc(iCount+1, sizeof(char *));
	if ((char **)0 == ppcOlds) {
		Die("calloc: no memory");
	}
	i = j = 0;
	for (k = 0; k < iCount; ++k) {
		ppcDirs[i] = ppcDirs[k];
		if ((char *)0 == (pcTail = strrchr(ppcDirs[i], '/'))) {
			fCmp = strcmp(ppcDirs[i], acOLD);
		} else {
			fCmp = strcmp(pcTail+1, acOLD);
		}
		if (0 != fCmp) {
			++i;
			continue;
		}
		if ((char *)0 == pcTail) {
			ppcOlds[j] = ".";
		} else if (pcTail == ppcDirs[i]) {
			ppcOlds[j] = "/";
		} else {
			*pcTail = '\000';
			ppcOlds[j] = malloc(strlen(ppcDirs[i])+1);
			if ((char *)0 == ppcOlds[j]) {
				Die("malloc: no memory");
			}
			(void)strcpy(ppcOlds[j], ppcDirs[i]);
			*pcTail = '/';
		}
		++j;
	}
	if (0 != j) {
		OldCk(j, ppcOlds, pCLCheck);
	}
	pCLCheck->pcflags = pcKeep;
	free((void *)ppcOlds);
	return i;
}
