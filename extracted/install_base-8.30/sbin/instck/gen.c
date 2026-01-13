/*
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
 */

/* these routines generate a file which will tell instck(1L)		(ksb)
 * what modes and owners all the listed files should have.
 */
#if !defined(lint)
static char rcsid[] =
	"$Id: gen.c,v 8.26 2009/12/29 16:37:12 ksb Exp $";
#endif	/* !lint */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sysexits.h>

#include "machine.h"
#include "configure.h"
#include "install.h"
#include "main.h"
#include "syscalls.h"
#include "special.h"
#include "instck.h"
#include "gen.h"
#include "magic.h"
#include "maxfreq.h"
#include "path.h"
#include "filedup.h"

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>

#if STRINGS
#include <strings.h>
#else
#include <string.h>
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

/* must have DIR typedef
 */
#include "old.h"


/* glob select -- used by GenMatch to scan directories			(ksb)
 * we internally use a magic star to match leading dot file, but not
 * "." and ".."
 */
static char *_pcGlTest;
static char acMagicStar[] = "*";
static int
GlSelect(pDE)
const struct DIRENT *pDE;
{
	if (acMagicStar == _pcGlTest) {
		return !('.' == pDE->d_name[0] && ('\000' == pDE->d_name[1] || ('.' == pDE->d_name[1] && '\000' == pDE->d_name[2])));
	}
	if (_SamePath(_pcGlTest, (char *)pDE->d_name, 1)) {
		return 1;
	}
	return 0;
}

/* if either of these is one of the ones in the list it wins		(ksb)
 * else look for looser values, else return the strcmp of them
 */
int
ListPref(ppcList, pcLeft, pcRight)
char **ppcList, *pcLeft, *pcRight;
{
	for (/*param*/; (char *)0 != *ppcList; ++ppcList) {
		if (0 == strcmp(*ppcList, pcLeft))
			return -1;
		if (0 == strcmp(*ppcList, pcRight))
			return 1;
	}
	for (++ppcList; (char *)0 != *ppcList; ++ppcList) {
		if (0 == strcmp(*ppcList, pcLeft))
			return 1;
		if (0 == strcmp(*ppcList, pcRight))
			return -1;
	}
	return strcmp(pcLeft, pcRight);
}

typedef struct {
	char *pcthis;		/* tail of path				*/
	struct stat stthis;	/* stat buffer on file			*/
} GRAB;

/*
 * Put the directory in a order that promotes common lines and types	(ksb)
 * sort on type, then name.
 * HACK! to move `true' and `false' up to the top of a links list for
 *	hoser vendors that use lots of links to them.
 */
static int
DirOrder(pvLeft, pvRight)
const void *pvLeft, *pvRight;
{
	register const GRAB *pGRLeft, *pGRRight;

	/* put dirs first, if both are dirs put "OLD" first,
	 * then CVS and RCS dirs
	 */
	pGRLeft = (const GRAB *)pvLeft;
	pGRRight = (const GRAB *)pvRight;
	if (S_IFDIR == (pGRLeft->stthis.st_mode & S_IFMT)) {
		static char *apcDirSpec[] = {
			acOLD,		/* install's stage area		*/
			"CVS",		/* net-RCS			*/
			"RCS",		/* local RCS			*/
			"SCCS",		/* primative RCS :-)		*/
			(char *)0,	/* separate best/worst		*/
			(char *)0	/* sentinal value		*/
		};
		if (S_IFDIR != (pGRRight->stthis.st_mode & S_IFMT)) {
			return -1;
		}
		return ListPref(apcDirSpec, pGRLeft->pcthis, pGRRight->pcthis);
	} else if (S_IFDIR == (pGRRight->stthis.st_mode & S_IFMT)) {
		return 1;
	}

	/* sort on node type and device
	 */
	if ((pGRLeft->stthis.st_mode & S_IFMT) < (pGRRight->stthis.st_mode & S_IFMT)) {
		return -1;
	}
	if ((pGRLeft->stthis.st_mode & S_IFMT) > (pGRRight->stthis.st_mode & S_IFMT)) {
		return 1;
	}
	if (pGRLeft->stthis.st_dev < pGRRight->stthis.st_dev) {
		return -1;
	}
	if (pGRLeft->stthis.st_dev > pGRRight->stthis.st_dev) {
		return 1;
	}

	/* move any links into bundles, most links first,
	 * then by inode, then `true', `false', etc.
	 */
	if (pGRLeft->stthis.st_nlink > pGRRight->stthis.st_nlink) {
		return -1;
	}
	if (pGRLeft->stthis.st_nlink < pGRRight->stthis.st_nlink) {
		return 1;
	}
	if (1 != pGRLeft->stthis.st_nlink) {
		static char *apcBest[] = {
			"console",	/* this might be a botch	   */
			"ex",		/* manual page claims this is base */
			"false",	/* lots of links in /bin on HPUX   */
			"grep",		/* egrep, fgrep are the links	   */
			"man",		/* avoid `apropos' as the basename */
			"sh",		/* called `jsh' or `bsh' too	   */
			"true",		/* lots of links in /bin on HPUX   */
			"setme",	/* ksb's setuid driver		   */
			"cpu.indir",	/* binary file NFS dispatch hack   */
			(char *)0,
			"syscon",	/* not the real console		   */
			"systty",	/* same as above, kinda		   */
			"update.src",	/* HPUX always the link side	   */
			(char *)0
		};

		if (pGRLeft->stthis.st_ino < pGRRight->stthis.st_ino) {
			return -1;
		}
		if (pGRLeft->stthis.st_ino > pGRRight->stthis.st_ino) {
			return 1;
		}
		return ListPref(apcBest, pGRLeft->pcthis, pGRRight->pcthis);
	}

	/* Collate plain files on setuid and setgid stuff
	 * (setuid then setgid then sticky).
	 * if the user wants pull the executables into a group too
	 * (this is great for /etc -- ksb).
	 */
	if (S_IFREG == (pGRLeft->stthis.st_mode & S_IFMT) &&
		S_IFREG == (pGRRight->stthis.st_mode & S_IFMT)) {
		if ((pGRLeft->stthis.st_mode & 07000) < (pGRRight->stthis.st_mode & 07000)) {
			return 1;
		}
		if ((pGRLeft->stthis.st_mode & 07000) > (pGRRight->stthis.st_mode & 07000)) {
			return -1;
		}
		if (fByExec && (pGRLeft->stthis.st_mode & 0111) < (pGRRight->stthis.st_mode & 0111)) {
			return -1;
		}
		if (fByExec && (pGRLeft->stthis.st_mode & 0111) > (pGRRight->stthis.st_mode & 0111)) {
			return 1;
		}
	}

	/* then sort on the names -- how simple
	 */
	return strcmp(pGRLeft->pcthis, pGRRight->pcthis);
}

/* find matching files, look in the real file system --			(ksb)
 * like FileMatch, but sort the list into a neato order too.
 */
int
GenMatch(pcGlob, pcDir, pfiCall)
char *pcGlob, *pcDir;
int (*pfiCall)(/* char * */);
{
	register struct DIRENT *pDE;
	register char *pcTail;
	register char *pcSlash;
	auto int iRet, n;
	auto char acDir[MAXPATHLEN+1];
	auto struct stat stDir;
	auto struct DIRENT **ppDEList;
	extern int alphasort();

	/* setup select function */
	pcSlash = strchr(pcGlob, '/');
	if ((char *)0 != pcSlash) {
		*pcSlash++ = '\000';
	}
	(void)strcpy(acDir, pcDir);
	pcTail = strchr(acDir, '\000');
	if (pcTail[-1] != '/') {
		*pcTail++ = '/';
	}

	iRet = 0;
	_pcGlTest = pcGlob;
	if ('\000' == *pcGlob) {
		if (-1 != STAT_CALL(acDir, &stDir)) {
			iRet = (*pfiCall)(acDir, &stDir, 0);
		}
	} else if (-1 == (n = scandir(pcDir, &ppDEList, GlSelect, alphasort))) {
		fprintf(stderr, "%s: scandir: %s: %s\n", progname, pcDir, strerror(errno));
		iRet = 1;
	} else if (0 == n) {
		/*  */
	} else {
		register int i, j, fNeedDir, fNotDone, iCount;
		register GRAB *pGR;
		register int iBox;

		/* "p/" matches dirs only
		 * "dir/pat" causes a recursive call (of course)
		 */
		fNeedDir = (char *)0 != pcSlash && '\000' == pcSlash[0];
		fNotDone = (char *)0 != pcSlash && '\000' != pcSlash[0];

		if ((GRAB *)0 == (pGR = (GRAB *)calloc(n, sizeof(GRAB)))) {
			fprintf(stderr, "%s: calloc: no more memory (%d, %d)?\n", progname, n, sizeof(GRAB));
			exit(EX_OSERR);
		}
		for (iBox = 0, i = 0; 0 == iRet && i < n; ++i) {
			pDE = ppDEList[i];
			pGR[iBox].pcthis = pDE->d_name;
			(void)strcpy(pcTail, pDE->d_name);
			if (-1 == STAT_CALL(acDir, &pGR[iBox].stthis)) {
				fprintf(stderr, "%s: stat: %s: %s\n", progname, acDir, strerror(errno));
			} else if (fNotDone) {
				if ((char *)0 != pcSlash) {
					pcSlash[-1] = '/';
				}
				iRet = GenMatch(pcSlash, acDir, pfiCall);
				if ((char *)0 != pcSlash) {
					pcSlash[-1] = '\000';
				}
			} else if (fNeedDir) {
				if (S_IFDIR == (pGR[iBox].stthis.st_mode & S_IFMT)) {
					++iBox;
				}
			} else {
				++iBox;
			}
		}
		qsort((char *)pGR, iBox, sizeof(GRAB), DirOrder);
		for (j = i = 0; i < iBox; ++i) {
			(void)strcpy(pcTail, pGR[i].pcthis);
			if (fNeedDir) {
				(void)strcat(pcTail, "/");
			}
			if (i < j) {
				iRet = (*pfiCall)(acDir, &pGR[i].stthis, 0);
				continue;
			}
			if (S_IFDIR == (pGR[i].stthis.st_mode & S_IFMT) || 1 == pGR[i].stthis.st_nlink) {
				iRet = (*pfiCall)(acDir, &pGR[i].stthis, 0);
				++j;
				continue;
			}
			iCount = pGR[i].stthis.st_nlink;
			while (j < iBox) {
				if (pGR[i].stthis.st_ino != pGR[j].stthis.st_ino  || pGR[i].stthis.st_dev != pGR[j].stthis.st_dev) {
					break;
				}
				--iCount, ++j;
			}
			iRet = (*pfiCall)(acDir, &pGR[i].stthis, iCount);
		}

		/* release memory we glommed
		 */
		free((char *)pGR);
		for (i = 0; i < n; ++i) {
			free((char *)ppDEList[i]);
		}
		free((char *)ppDEList);
	}
	if ((char *)0 != pcSlash) {
		*--pcSlash = '/';
	}
	return iRet;
}


FILEDUPS FDLinks;

/*
 * copy the parent directory of pcFile to pcDir, which is a buffer	(ksb)
 * given by the user. returns the `tail part'
 */
static char *
getparent(pcFile, pcDir)
char *pcFile, *pcDir;
{
	register char *pcTail;

	if ((char *)0 == (pcTail = strrchr(pcFile, '/'))) {
		if ((char *)0 != pcDir)
			(void)strcpy(pcDir, ".");
		return pcFile;
	} else if (pcFile == pcTail) {
		if ((char *)0 != pcDir)
			(void)strcpy(pcDir, "/");
		return pcFile+1;
	}
	*pcTail = '\000';
	if ((char *)0 != pcDir)
		(void)strcpy(pcDir, pcFile);
	*pcTail = '/';
	return pcTail+1;
}

static COMPONENT *pCMRoot;
static int fBeenHere = 0;
static struct passwd *pwdLast;
static struct group *grpLast;
static char acModeLast[sizeof("!-rwxrwxrwx")+1];
static dev_t devLast;

extern int stat();

/* make crass comments about things we know are funny:
 *	OLD not a directory
 *	-??s??-??- is redundant (owner root or not)
 *	-??-??s??- is redundant too
 *	-?????s??? for default group is poor form
 *	-??S??-??- or -??-??S??- or -??-??-??T
 *	-??sr?x--? or -??s???r-?		[if none above]
 *	links not shown in this report
 *	if (perm(group) == perm(other)) group should be default
 *	-????w??w? is dangerous, (d????wx?wx as well?)
 *	dr--r--r--	need +x if +r (but not the other way)
 *	b??x??xrwx	all bad bits
 *	p??x??x??x	all bad bits
 *	perm(user)<perm(world) | perm(user)<perm(group) |
 *		perm(group) < perm(world)	[if none other]
 *	?---------	why mode 0?		[when none of the above]
 *
 * We need 4 fields from the stat struct in the param list:
 *	st_uid, st_gid, st_mode, st_nlink
 * If we ever need more we'll have to fix the call in GenCk, which doesn't
 * have real (struct stat) to call with.
 */
static void
Crass(pcSlash, pcMode, pstThis, pwdThis, iCount)
char *pcSlash, *pcMode;
struct stat *pstThis;
struct passwd *pwdThis;
int iCount;
{
	register int fCom;
	static char acSCom[] = "\t#";

	fCom = 0;
	if ('d' != pcMode[0] && 0 == strcmp(acOLD, pcSlash)) {
		if (!fCom)
			(void)fputs(acSCom, stdout);
		(void)printf(" should be backup directory");
		fCom |= 1;
	}
	if ('-' == pcMode[0] && 0 != (pstThis->st_mode & 04000) && 0 == (pstThis->st_mode & 0011) && 0 != (pstThis->st_mode & 0100)) {
		if (!fCom)
			(void)fputs(acSCom, stdout);
		if (0 == pwdThis->pw_uid)
			(void)printf(" redundant setuid?");
		else
			(void)printf(" setuid only works for root");
		fCom |= 1;
	}
	if ('-' == pcMode[0] && 0 != (pstThis->st_mode & 02000) && (0 == (pstThis->st_mode & 0101) || (0 == pwdThis->pw_uid && 0 == (pstThis->st_mode & 011))) && 0 != (pstThis->st_mode & 0010)) {
		/* Mode 02010 removes our effective group in favor of a group we
		 * are a member of, that's a corner case for sure. --ksb
		 */
		if (!fCom)
			(void)fputs(acSCom, stdout);
		(void)printf(" setgid only works for root");
		fCom |= 1;
	}
	if ('-' == pcMode[0] && (struct group *)0 != grpDef && pstThis->st_gid == grpDef->gr_gid && 0 != (pstThis->st_mode & S_ISGID)) {
		if (!fCom)
			(void)fputs(acSCom, stdout);
		printf(" setgid %s?", grpDef->gr_name);
		fCom |= 1;
	}
	if ('-' == pcMode[0] && 0 == (pstThis->st_mode & 0111) && 0 != (pstThis->st_mode & 04000)) {
		if (!fCom)
			(void)fputs(acSCom, stdout);
		(void)printf(" setuid but no execute?");
		fCom |= 1;
	}
	if ('-' == pcMode[0] && 0 == (pstThis->st_mode & 0111) && 0 != (pstThis->st_mode & 02000)) {
		if (!fCom)
			(void)fputs(acSCom, stdout);
		(void)printf(" setgid but no execute?");
		fCom |= 1;
	}
	if ('-' == pcMode[0] && 0 == (pstThis->st_mode & 0111) && 0 != (pstThis->st_mode & 01000)) {
		if (!fCom)
			(void)fputs(acSCom, stdout);
		(void)printf(" sticky with no execute?");
		fCom |= 1;
	}
	if ('-' == pcMode[0] && 0 == fCom && 0 != (pstThis->st_mode & 04000) && (0 != (pstThis->st_mode & 0004) || (0 != (pstThis->st_mode & 0040) && 0 == (pstThis->st_mode & 02000)))) {
		if (!fCom)
			(void)fputs(acSCom, stdout);
		(void)printf(" read bits on setuid executables");
		fCom |= 1;
	}
	if ('-' == pcMode[0] && 0 == fCom && 0 != (pstThis->st_mode & 02000) && 0 != (pstThis->st_mode & 0004)) {
		if (!fCom)
			(void)fputs(acSCom, stdout);
		(void)printf(" world read bit on setgid executables?");
		fCom |= 1;
	}
	if (0 != (pstThis->st_mode & 0022)) {
		if (!fCom)
			(void)fputs(acSCom, stdout);
		(void)printf(" write bits?");
		fCom |= 1;
	}
	if ((struct group *)0 != grpDef && pstThis->st_gid != grpDef->gr_gid && 0 == (pstThis->st_mode & S_ISGID) && ((pstThis->st_mode & 070) >> 3) == (pstThis->st_mode & 07)) {
		if (!fCom)
			(void)fputs(acSCom, stdout);
		printf(" group %s?", grpDef->gr_name);
		fCom |= 1;
	}
	/* second half
	 */
	if (0 != iCount) {
		if (!fCom)
			(void)fputs(acSCom, stdout);
		(void)printf(" %d link%s of %ld not shown", iCount, 1 == iCount ? "" : "s", (long)pstThis->st_nlink);
		fCom |= 1;
	}
	if ('d' == pcMode[0] && (
	    ((0 != (pstThis->st_mode & 0400)) && (0 == (pstThis->st_mode & 0100))) ||
	    ((0 != (pstThis->st_mode & 0040)) && (0 == (pstThis->st_mode & 0010))) ||
	    ((0 != (pstThis->st_mode & 0004)) && (0 == (pstThis->st_mode & 0001))))) {
		if (!fCom)
			(void)fputs(acSCom, stdout);
		printf(" modes allow some read but not execute?");
		fCom |= 1;
	} else if ('d' == pcMode[0] && (
	    ((0 != (pstThis->st_mode & 0200)) && (0 == (pstThis->st_mode & 0100))) ||
	    ((0 != (pstThis->st_mode & 0020)) && (0 == (pstThis->st_mode & 0010))) ||
	    ((0 != (pstThis->st_mode & 0002)) && (0 == (pstThis->st_mode & 0001))))) {
		if (!fCom)
			(void)fputs(acSCom, stdout);
		printf(" modes allow some write but not execute?");
		fCom |= 1;
	} else if ('d' == pcMode[0] && (07 == (pstThis->st_mode & 01007))) {
		if (!fCom)
			(void)fputs(acSCom, stdout);
		printf(" make sticky?");
		fCom |= 1;
	}
	if ('b' == pcMode[0] && 0 != (00111 & pstThis->st_mode)) {
		if (!fCom)
			(void)fputs(acSCom, stdout);
		printf(" execute on block device?");
		fCom |= 1;
	}
	if ('b' == pcMode[0] && 0 != (00006 & pstThis->st_mode)) {
		if (!fCom)
			(void)fputs(acSCom, stdout);
		printf(" block device allows world access (to filesystems?)");
		fCom |= 1;
	}
#if 0
	/* +x on non-tty character devices (pty*|tty*|console)
	 * this fails on cua00 and other dial devices. Looking at the major
	 * device number is out of scope due to the limits on the synthetic
	 * stat structure we get.
	 */
	if ('c' == pcMode[0] && 0 != (00111 & pstThis->st_mode) && !(('p' == pcSlash[1] || 't' == pcSlash[1]) && 't' == pcSlash[2] && 'y' == pcSlash[3]) && 0 != strcmp("console", pcSlash+1)) {
		if (!fCom)
			(void)fputs(acSCom, stdout);
		printf(" execute on non-tty character device?");
		fCom |= 1;
	}
#endif
	if ('p' == pcMode[0] && 0 != (00111 & pstThis->st_mode)) {
		if (!fCom)
			(void)fputs(acSCom, stdout);
		printf(" execute a FIFO?");
		fCom |= 1;
	}

	/* one more stab at being rude
	 */
	if (0 == fCom && (
	    (0 == (0400 & pstThis->st_mode) && 0 != (0004 & pstThis->st_mode)) ||
	    (0 == (0200 & pstThis->st_mode) && 0 != (0002 & pstThis->st_mode)) ||
	    (0 == (0100 & pstThis->st_mode) && 0 != (0001 & pstThis->st_mode)))) {
		(void)fputs(acSCom, stdout);
		printf(" owner<world?");
		fCom |= 1;
	}
	if (0 == fCom && (
	    (0 == (0040 & pstThis->st_mode) && 0 != (0004 & pstThis->st_mode)) ||
	    (0 == (0020 & pstThis->st_mode) && 0 != (0002 & pstThis->st_mode)) ||
	    (0 == (0010 & pstThis->st_mode) && 0 != (0001 & pstThis->st_mode)))) {
		(void)fputs(acSCom, stdout);
		printf(" group<world?");
		fCom |= 1;
	}
	if (0 == fCom && (
	    (0 == (0400 & pstThis->st_mode) && 0 != (0040 & pstThis->st_mode)) ||
	    (0 == (0200 & pstThis->st_mode) && 0 != (0020 & pstThis->st_mode)) ||
	    (0 == (0100 & pstThis->st_mode) && 0 != (0010 & pstThis->st_mode)))) {
		(void)fputs(acSCom, stdout);
		printf(" owner<group?");
		fCom |= 1;
	}
	if (0 == fCom && 0 == (07777 & pstThis->st_mode)) {
		(void)fputs(acSCom, stdout);
		printf(" mode 0?");
		fCom |= 1;
	}
	(void)fputc('\n', stdout);
}

/* do the nifty output							(ksb)
 */
int
DoGen(pcFile, pstThis, iCount)
char *pcFile;
struct stat *pstThis;
int iCount;
{
	register char *pcSlash;
	register struct passwd *pwdThis;
	register struct group *grpThis;
	auto char acModeThis[sizeof("!drwxrwxrwx")+1];
	static char acFormat[] = "%-15s %-7s %-7s ";
	static char acLFormat[] = "%-23s %c%s%s";
	auto int fdFile, fBadLookUp;

	if ('.' == pcFile[0] && '/' == pcFile[1] && '\000' != pcFile[2]) {
		pcFile += 2;
	}
	if ((char *)0 == (pcSlash = strrchr(pcFile, '/'))) {
		pcSlash = pcFile;
	} else {
		++pcSlash;
	}
	(void)NodeType(pstThis->st_mode, acModeThis);
	if ('l' == acModeThis[0]) {
#if HAVE_SYMLINKS
		if (fLinks) {
			auto char acLink[MAXPATHLEN+2];
			register int i;
			register char *pcComment, *pcContent;
			if (-1 == (i = readlink(pcFile, acLink, MAXPATHLEN))) {
				fprintf(stderr, "%s: readlink: %s: %s\n", progname, pcFile, strerror(errno));
				return 0;
			}
			acLink[i] = '\000';
			pcContent = acLink;
			/* clip bogus "./" off maybe we'll get a chance to
			 * re-link it and fix is later.
			 */
			while ('.' == pcContent[0] && '/' == pcContent[1]) {
				pcContent += 2;
			}
			/* Comment about OLD -> ../anything
			 */
			if (0 != strcmp(acOLD, pcSlash) || '.' != pcContent[0] || '.' != pcContent[1] || '/' != pcContent[2]) {
				pcComment = "\n";
			} else {
				pcComment = "\t# should be a local backup directory\n";
			}
			fprintf(stdout, acLFormat, pcFile, '@', pcContent, pcComment);
			fBeenHere = 0;
			return 0;
		}
		if (-1 == stat(pcFile, pstThis)) {
			if (ENOENT != errno)
				fprintf(stderr, "%s: stat: %s: %s\n", progname, pcFile, strerror(errno));
			return 0;
		}
		(void)NodeType(pstThis->st_mode, acModeThis);
#else
		fprintf(stderr, "%s: %s: no symlinks, but link type of `l\'?\n", progname, pcFile);
		return 0;
#endif
	}
	if (pstThis->st_nlink > 1 && fLinks) {
		register char *pcLink;
		pcLink = FDAdd(& FDLinks, pcFile, pstThis);
		if (0 != strcmp(pcLink, pcFile)) {
			fprintf(stdout, acLFormat, pcFile, ':', pcLink, "\n");
			fBeenHere = 0;
			return 0;
		}
	}
	fBadLookUp = 0;
	pwdThis = getpwuid((int) pstThis->st_uid);
	if ((struct passwd *)0 == pwdThis) {
		fprintf(stderr, "%s: %s: getpwuid: %d: unknown uid\n", progname, pcFile, pstThis->st_uid);
		fBadLookUp = 1;
	}
	grpThis = getgrgid((int) pstThis->st_gid);
	if ((struct group *)0 == grpThis) {
		fprintf(stderr, "%s: %s: getgrgid: %d: unknown gid\n", progname, pcFile, pstThis->st_gid);
		fBadLookUp |= 2;
	}
	ModetoStr(acModeThis+1, (int)PERM_BITS(pstThis->st_mode), 00000);

	/* make different devices show all again
	 */
	if (acModeLast[0] != acModeThis[0] ||
	    (('b' == acModeLast[0] || 'c' == acModeLast[0]) &&
	    major(devLast) != major(pstThis->st_rdev))) {
		fBeenHere = 0;
	}
	devLast = pstThis->st_rdev;

	/* use the \" notation for multiple lines sometimes,
	 * looks, *lots* better
	 */
	if (fBadLookUp) {
		(void)fputc('#', stdout);
	}
	(void)printf("%-24s ", pcFile);
	if (fBadLookUp) {
		static char *apcBad[3] = {
			"uid", "gid", "uid and gid"
		};
		fBeenHere = 0;
		(void)printf(acFormat, acModeThis, ((struct passwd *)0 != pwdThis) ? pwdThis->pw_name : "*", ((struct group *)0 != grpThis) ? grpThis->gr_name : "*");
		(void)printf("-\t# unknown %s\n", apcBad[fBadLookUp-1]);
		return 0;
	} else if (fBeenHere && !fVerbose) {
		/* one of the most complicated C expressions I have ever coded
		 * keep dups in columns, keep last up to date
		 */
		(void)printf(acFormat,
			0 == strcmp(acModeThis, acModeLast) && 0 == (pstThis->st_mode & (S_ISVTX|S_ISUID|S_ISGID)) ? "\"" : ((void)strcpy(acModeLast, acModeThis), acModeThis),
			pwdThis->pw_uid == pwdLast->pw_uid && 0 == (pstThis->st_mode & S_ISUID) ? "\"" : (free((char *)pwdLast), pwdLast = savepwent(pwdThis), pwdThis->pw_name),
			grpThis->gr_gid == grpLast->gr_gid && 0 == (pstThis->st_mode & S_ISGID) ? "\"" : (free((char *)grpLast), grpLast = savegrent(grpThis), grpThis->gr_name)
		);
	} else {
		(void)printf(acFormat, acModeThis, pwdThis->pw_name, grpThis->gr_name);
		/* memory leak XXX */
		pwdLast = savepwent(pwdThis);
		grpLast = savegrent(grpThis);
		(void)strcpy(acModeLast, acModeThis);
		fBeenHere = 1;
	}

	if ('-' != acModeThis[0]) {
		(void)fputc('-', stdout);
	} else if (0 > (fdFile = open(pcFile, 0, 0000))) {
		if (errno != EACCES) {
			(void)fprintf(stderr, "%s: open: %s: %s\n", progname, pcFile, strerror(errno));
		}
		(void)fputc('*', stdout);
	} else {
		register char *pcDot;
		if ((char *)0 != (pcDot = strrchr(pcSlash, '.')) && 'a' == pcDot[1] && '\000' == pcDot[2]) {
			switch (IsRanlibbed(fdFile, pstThis)) {
			case -1:	/* not a library	*/
				(void)fputc('n', stdout);
				break;
			case 0:		/* library, not ranlibbed */
			case 1:		/* binary, ranlibbed */
				(void)fputc('l', stdout);
				break;
			}
		} else if (0 != (pstThis->st_mode & 0111)) {
			switch (IsStripped(fdFile, pstThis)) {
			case -1:	/* not a binary	*/
				(void)fputc('?', stdout);
				break;
			case 0:		/* binary, not stripped */
				(void)fputc('n', stdout);
				break;
			case 1:		/* binary, stripped */
				(void)fputc('s', stdout);
				break;
			}
		} else {
			(void)fputc('-', stdout);
		}
		(void)close(fdFile);
	}
#if HAVE_CHFLAGS
	{
	register char *pcMem = (char *)0;
	if (0 != pstThis->st_flags && (char *)0 != (pcMem = fflagstostr(pstThis->st_flags))) {
		fputc(',', stdout);
		fputs(pcMem, stdout);
		free((char *)pcMem);
	}
	}
#endif

	Crass(pcSlash, acModeThis, pstThis, pwdThis, iCount);
	return 0;
}

static MAXFREQ **ppMFRoot;

/* copy an ME element							(ksb)
 * configure the MF routines here
 */
int
MECopy(pMEDest, pMESource)
ME_ELEMENT *pMEDest, *pMESource;
{
	*pMEDest = *pMESource;
	return 0;
}

/* comparet two ME elements return 0 if they are equal,			(ksb)
 * non-zero if they are not
 */
int
MECompare(pME1, pME2)
ME_ELEMENT *pME1, *pME2;
{
	return pME1->chtype - pME2->chtype ||
		pME1->iuid - pME2->iuid ||
		pME1->igid - pME2->igid ||
		pME1->imode - pME2->imode;
}

/*
 * scan a directory (passed each file within) for most common		(ksb)
 * { owner, group, mode, 'd'/'-'/other, 'l'/'s'/'-' } tuple
 */
/*ARGSUSED*/
static int
DoStats(pcFile, pstThis, iCount)
char *pcFile;
struct stat *pstThis;
int iCount;
{
	auto ME_ELEMENT METhis;
	register ME_ELEMENT *pME;
	register char *pcDotx;
	static char acNew[4] = "new";
	static char acMixed[] = "mix";

	(void)NodeType(pstThis->st_mode, & METhis.chtype);
	if (METhis.chtype != '-') {
		return 0;
	}
	METhis.imode = PERM_BITS(pstThis->st_mode);
	METhis.iuid = pstThis->st_uid;
	METhis.igid = pstThis->st_gid;
	METhis.chstrip = '-';		/* XXX worng */
	METhis.pcext = acNew;
	pME = MFIncrement(ppMFRoot, & METhis);
	if ((ME_ELEMENT *)0 == pME || pME->pcext == acMixed) {
		return 0;
	}

	/* See if all these have the same extender (on the file part)
	 * More advanced would be to do the maxfreq for each extender, then
	 * join the results at the end. LLL -- ksb
	 */
	pcDotx = strrchr(pcFile, '/');
	if ((char *)0 == pcDotx) {
		pcDotx = pcFile;
	}
	pcDotx = strrchr(pcDotx, '.');
	if (pME->pcext == acNew) {
		pME->pcext = (char *)0 == pcDotx ? (char *)0 : strdup(pcDotx);
	} else if ((char *)0 == pcDotx) {
		if ((char *)0 != pME->pcext) {
			pME->pcext = acMixed;
		}
	} else if ((char *)0 == pME->pcext) {
		pME->pcext = acMixed;
	} else if (0 != strcmp(pcDotx, pME->pcext)) {
		pME->pcext = acMixed;
	}
	return 0;
}

static ME_ELEMENT MESame;

/* Output files with a different `ME' for this directory		(ksb)
 * (also output if fLinks and st_nlink > 1, or first character is a '.')
 */
static int
DoDiffs(pcFile, pstThis, iCount)
char *pcFile;
struct stat *pstThis;
int iCount;
{
	auto char ch;
	register char *pcLast;

	(void)NodeType(pstThis->st_mode, &ch);
	if (fRecurse && 'd' == ch) {
		auto char *apcFake[2];
		apcFake[0] = pcFile;
		apcFake[1] = (char *)0;
		GenCk(1, apcFake);
		return 0;
	}
	if ((char *)0 == (pcLast = strrchr(pcFile, '/'))) {
		pcLast = pcFile;
	}
	while ('/' == *pcLast) {
		++pcLast;
	}
	if ('.' != *pcLast && !fLongList && MESame.chtype == ch && MESame.iuid == pstThis->st_uid && MESame.igid == pstThis->st_gid && MESame.imode == PERM_BITS(pstThis->st_mode) && !(fLinks && 1 < pstThis->st_nlink)) {
		return 0;
	}
	(void)DoGen(pcFile, pstThis, iCount);
	return 0;
}


typedef struct PMnode {
	char s_cgrp,
	     s_cpwd;			/* keep track of malloc mem	*/
	struct passwd *s_pwd;
	struct group *s_grp;
	char s_acmode[24];		/* "d---------/rwxrwxrwx" is 21	*/
} PMODES;

/*
 * save the current dirs modes if we are doing an inherit on that prop	(ksb)
 * and take the new ones from the given file.
 * If the new one is not kosher fall back to the old one.
 */
static void
PushModes(pST, pPM)
struct stat *pST;
PMODES *pPM;
{
	/* save the current state
	 */
	pPM->s_cgrp = pPM->s_cpwd = 'n';
	pPM->s_pwd = pwdDef;
	pPM->s_grp = grpDef;
	(void)strcpy(pPM->s_acmode, pcDefMode);

	/* if we need to build new default modes
	 */
	if ((char *)0 == pcGenOwner || '\000' == pcGenOwner[0]) {
		pwdDef = getpwuid((int) pST->st_uid);
		if ((struct passwd *)0 == pwdDef) {
			fprintf(stderr, "%s: getpwuid: %d: unknown uid\n", progname, pST->st_uid);
			pwdDef = pPM->s_pwd;
		} else {
			pwdDef = savepwent(pwdDef);
			pPM->s_cpwd = 'y';
		}
	}
	if ((char *)0 == pcGenGroup || '\000' == pcGenGroup[0]) {
		grpDef = getgrgid((int) pST->st_gid);
		if ((struct group *)0 == grpDef) {
			fprintf(stderr, "%s: getgrgid: %d: unknown gid\n", progname, pST->st_gid);
			grpDef = pPM->s_grp;
			if ((struct group *)0 == grpDef)
				exit(EX_NOUSER);
		} else {
			grpDef = savegrent(grpDef);
			pPM->s_cgrp = 'y';
		}
	}
	if ((char *)0 == pcMode) {
		ModetoStr(pcDefMode+1, (int)PERM_BITS(pST->st_mode), 0);
	}
}

/*
 * restore all the saved modes for this directory			(ksb)
 */
static void
PopModes(pPM)
PMODES *pPM;
{
	register char *pc;

	if ((char *)0 == (pc = DEFOWNER) || '\000' == pc[0]) {
		free((char *)pwdDef);
		pwdDef = pPM->s_pwd;
	}
	if ((char *)0 == (pc = DEFGROUP) || '\000' == pc[0]) {
		free((char *)grpDef);
		grpDef = pPM->s_grp;
	}
	if ((char *)0 == pcMode) {
		(void)strcpy(pcDefMode, pPM->s_acmode);
	}
}

/* generate a checklist for the given directories			(ksb)
 */
int
GenCk(argc, argv)
int argc;
char **argv;
{
	register PATH_DATA *pPD;
	register int i, j, iBox;
	register char *pcFile;
	register GRAB *pGR;
	register int fBasePush;
	auto struct passwd *pwdThis;
	auto struct group *grpThis;
	auto struct stat stAbove;
	auto MAXFREQ **ppMFStats;
	auto char acMode[20];
	auto char acPat[MAXPATHLEN+3];
	auto int iRet = 0;
	auto PMODES PMFile, PMDir;

	ppMFStats = (MAXFREQ **)calloc(sizeof(MAXFREQ *), argc+1);
	if ((MAXFREQ **)0 == ppMFStats) {
		fprintf(stderr, "%s: calloc: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}

	FDInit(&FDLinks);

	if ((GRAB *)0 == (pGR = (GRAB *)calloc(argc, sizeof(GRAB)))) {
		fprintf(stderr, "%s: calloc: no memory?\n", progname);
		exit(EX_OSERR);
	}

	/* scan for plain files, collect stats on dirs
	 */
	for (iBox = i = 0; i < argc && (char *)0 != (pcFile = argv[i]); ++i) {
		register int l, fMustBeDir;
		/* /bin/ is really /bin, please.  And "" is "." (a dir).
		 */
		fMustBeDir = 0;
		if (0 == (l = strlen(pcFile))) {
			argv[i] = pcFile = ".";
			l = 1;
			fMustBeDir = 1;
		}
		while (1 != l && '/' == pcFile[l-1]) {
			pcFile[--l] = '\000';
			fMustBeDir = 1;
		}

		pPD = AddPath(& pCMRoot, pcFile);
		if (0 != pPD->fseen) {
			continue;
		}
		pPD->fseen = 1;
		if (-1 == STAT_CALL(pcFile, &pGR[iBox].stthis)) {
			fprintf(stderr, "%s: stat: %s %s\n", progname, pcFile, strerror(errno));
			continue;
		}
#if HAVE_SYMLINKS
		if (S_IFLNK == (pGR[iBox].stthis.st_mode&S_IFMT)) {
			auto char acLink[MAXPATHLEN+2];
			auto char acDup[MAXPATHLEN+2];
			register int iLen;

			if (fLinks) {
				DoGen(pcFile, &pGR[iBox].stthis, 0);
			}
			if (-1 == (iLen = readlink(pcFile, acLink, MAXPATHLEN))) {
				fprintf(stderr, "%s: readlink: %s: %s\n", progname, pcFile, strerror(errno));
				return 0;
			}
			acLink[iLen] = '\000';

			/* stat the file now
			 * mark the readlink value as seen?
			 */
			if (-1 == stat(pcFile, &pGR[iBox].stthis)) {
				fprintf(stderr, "%s: stat: %s %s\n", progname, pcFile, strerror(errno));
				continue;
			}
			if (!fDirs || S_IFDIR != (pGR[iBox].stthis.st_mode & S_IFMT)) {
				continue;
			}
			(void)getparent(pcFile, acPat);
			if ('/' == acLink[0]) {
				sprintf(acDup, "%s", acLink);
			} else {
				sprintf(acDup, "%s%s%s", acPat, "/" + ('/' == acPat[strlen(acPat+1)]), '.' == acLink[0] && '/' == acLink[1] ? acLink+2 : acLink);
			}
			DoGen(acDup, &pGR[iBox].stthis, 0);
			if ((char *)0 == (pcFile = malloc(strlen(acDup)+1))) {
				continue;
			}
			(void)strcpy(pcFile, acDup);
		}
#endif
		if (0 != fMustBeDir && S_IFDIR != (pGR[iBox].stthis.st_mode & S_IFMT)) {
			fprintf(stderr, "%s: `%s/\' is not a directory\n", progname, pcFile);
			continue;
		}
		pGR[iBox++].pcthis = pcFile;
	}

	qsort((char *)pGR, iBox, sizeof(GRAB), DirOrder);

	/* scan for base rules, output diffs
	 * In the loop below j keeps track of the last link to this file
	 * (in the multiple link case (very much like the loop at line 288).
	 */
	for (j = i = 0; i < iBox; fBasePush && (PopModes(& PMFile), 0), ++i) {
		fBasePush = 0;
		pcFile = pGR[i].pcthis;
		(void)getparent(pcFile, acPat);

		ppMFRoot = &ppMFStats[i];
		if (-1 == stat(acPat, &stAbove)) {
			fprintf(stderr, "%s: stat: %s %s\n", progname, acPat, strerror(errno));
			if (j == i) {
				++j;
			}
			continue;
		}

		/* assemble file modes
		 */
		PushModes(&stAbove, &PMFile);
		fBasePush = 1;

		/* do the links, if we saw some
		 */
		if (i < j) {
			iRet |= DoGen(pcFile, &pGR[i].stthis, 0);
			continue;
		}
		/* loop invar. broken, help!
		 */
		if (i > j) {
			fprintf(stderr, "%s: get a doctor! i > j in main generator!\n", progname);
			abort();
		}

		/* do the kids
		 */
		if (fDirs && S_IFDIR == (pGR[i].stthis.st_mode & S_IFMT)) {
			auto struct stat stFake;

			++j;
			PushModes(&pGR[i].stthis, & PMDir);
			GenMatch("*", pcFile, DoStats);
			iRet |= DoGen(pcFile, &pGR[i].stthis, 0);
			/* If no file (not dir or other node) matched, or if
			 * the `*' pat for this dir would only match a single
			 * file, or if -l; just put those files in literally.
			 * There is a bug when the only file matched has
			 * many links and is the only in the directory. LLL
			 */
			if ((MAXFREQ *)0 == ppMFStats[i] || 1 == ppMFStats[i]->icount || fLongList) {
				if (fRecurse) {
					GenMatch(acMagicStar, pcFile, DoDiffs);
				} else {
					GenMatch(acMagicStar, pcFile, DoGen);
				}
				PopModes(& PMDir);
				continue;
			}

			/* output the diffs, then the pattern
			 */
			MESame = ppMFStats[i]->ME;
			GenMatch(acMagicStar, pcFile, DoDiffs);
			PopModes(& PMDir);

			pwdThis = getpwuid((int) ppMFStats[i]->ME.iuid);
			if (0 == pwdThis) {
				fprintf(stderr, "%s: %s: getpwuid: %d: unknown uid\n", progname, pcFile, ppMFStats[i]->ME.iuid);
				continue;
			}
			grpThis = getgrgid((int) ppMFStats[i]->ME.igid);
			if (0 == grpThis) {
				fprintf(stderr, "%s: %s: getgrgid: %d: unknown gid\n", progname, pcFile, ppMFStats[i]->ME.igid);
				continue;
			}
			acMode[0] = MESame.chtype;
			/* in sync with Crass's needs */
			stFake.st_mode = ppMFStats[i]->ME.imode;
			stFake.st_uid = ppMFStats[i]->ME.iuid;
			stFake.st_gid = ppMFStats[i]->ME.igid;
			stFake.st_nlink = 1;
			ModetoStr(acMode+1, (int)PERM_BITS(stFake.st_mode), 00000);
			(void)strcpy(acPat, pcFile);
			(void)strcat(acPat, "/*" + ('/' == pcFile[strlen(pcFile)-1]));
			if ((char *)0 != ppMFStats[i]->ME.pcext && '.' == ppMFStats[i]->ME.pcext[0]) {
				(void)strcat(acPat, ppMFStats[i]->ME.pcext);
			}
			(void)printf("%-24s %-15s %-7s %-7s ?", ('.' == acPat[0] && '/' == acPat[1]) ? acPat+2 : acPat, acMode, pwdThis->pw_name, grpThis->gr_name);
			Crass(strrchr(acPat, '/'), acMode, &stFake, pwdThis, 0);

			/* we have to reset fBeenHere so we don't copy the
			 * glob modes down to the next line, because we would be
			 * comparing to the line *above* the glob line
			 */
			fBeenHere = 0;
			continue;
		}
		if (1 != pGR[i].stthis.st_nlink && S_IFDIR != (pGR[i].stthis.st_mode & S_IFMT)) {
			register int iCount;

			iCount = pGR[i].stthis.st_nlink;
			while (j < iBox) {
				if (pGR[i].stthis.st_ino != pGR[j].stthis.st_ino  || pGR[i].stthis.st_dev != pGR[j].stthis.st_dev) {
					break;
				}
				--iCount, ++j;
			}
			iRet |= DoGen(pcFile, &pGR[i].stthis, iCount);
			continue;
		}
		iRet |= DoGen(pcFile, &pGR[i].stthis, 0);
		++j;
	}
	return iRet;
}
