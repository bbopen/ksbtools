/* $Id: links.c,v 8.18 2009/11/04 20:50:43 ksb Exp $
 *
 * We search the file system for all the links to a given file,		(ksb)
 * not quite general purpose -- we knoe about the install -1 OLD dirs.
 *
 * But we just look harder, not more cleverly.  Some would say that we
 * should just _start_ with the `final solution', but the time to search
 * a whole file system is really long compared to the limited searches we
 * try first. (ksb)
 */

#if !VINST
#if !defined(lint)
static char rcsid[] =
	"$Id: links.c,v 8.18 2009/11/04 20:50:43 ksb Exp $";
#endif	/* !lint */
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>

#include "machine.h"
#include "configure.h"
#include "install.h"
#if VINST
#include "main.h"
#endif

#if USE_MNTENT
#include <mntent.h>
#else
#if USE_VFSENT
#include <sys/vfstab.h>
#else
#if USE_CHECKLIST
#include <checklist.h>
#define fstab		checklist
#define fs_file		fs_dir
#else
#include <fstab.h>
#endif
#endif
#endif

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

#if STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#include "main.h"
#include "syscalls.h"
#include "special.h"
#include "instck.h"
#include "old.h"
#include "gen.h"
#include "path.h"
#include "links.h"



/* We hope we have enough information in this backup filename to	(ksb)
 * re-create the original filename (if it is still in `..').
 * ../mkcmdXXXXXX, iLen = 11
 * zzzzzzzz^ = 8 = iLen - 6
 * but on filesystems with limited (14) character length
 * filenames we have to try substrings, ouch!
 */
static int
FindOrig(pcBack, pcIn, pstBack, pcOrig, pstOrig)
char *pcBack, *pcIn, *pcOrig;
struct stat *pstBack, *pstOrig;
{
	register int iLen;
	register int i;
	auto char acLoop[MAXPATHLEN];

	pcOrig[0] = '\000';
	(void)snprintf(acLoop, sizeof(acLoop), "%s/%s", pcIn, pcBack);
	iLen = strlen(acLoop);

	for (i = 0; i < 7; ++i) {
		acLoop[iLen-i] = '\000';

		if ('/' == acLoop[iLen-i-1]) {
			break;
		}
		if (-1 == STAT_CALL(acLoop, pstOrig)) {
			continue;
		}

		(void)strcpy(pcOrig, acLoop);
		if ((struct stat *)0 == pstBack) {
			return 1;
		}
		if (pstBack->st_ino == pstOrig->st_ino && pstBack->st_dev == pstOrig->st_dev) {
			return 1;
		}
	}
	return 0;
}

/* find the mount point for this file system				(ksb)
 */
int
FindMtPt(pST, pcName)
struct stat *pST;
char *pcName;
{
#if USE_MNTENT
	register FILE *fpMnt;
	auto struct mntent *pMEThis;
#else
#if USE_VFSENT
	register FILE *fpVFstab;
	auto struct vfstab VFSThis;
#else
	register struct fstab *pFS;
#endif
#endif
	auto struct stat stOf;
	register int iRet;

	iRet = 0;
#if USE_MNTENT
	if ((FILE *)0 == (fpMnt = setmntent("/etc/mtab", "r"))) {
		fprintf(stderr, "%s: setmntent: %s: %s\n", progname, "/etc/mtab", strerror(errno));
	} else {
		while ((struct mntent *)0 != (pMEThis = getmntent(fpMnt))) {
			if (hasmntopt(pMEThis, "hide"))
				continue;
			if (-1 == stat(pMEThis->mnt_dir, &stOf))
				continue;
			if (stOf.st_dev != pST->st_dev)
				continue;
			/* found it */
			(void)strcpy(pcName, pMEThis->mnt_dir);
			iRet = 1;
			break;
		}
		endmntent(fpMnt);
	}
#else
#if USE_VFSENT
	if ((FILE *)0 == (fpVFstab = fopen("/etc/vfstab", "r"))) {
		fprintf(stderr, "%s: fopen: %s: %s\n", progname, "/etc/vfstab", strerror(errno));
	} else {
		while (0 == getvfsent(fpVFstab, & VFSThis)) {
			/* swap, we guess?
			 */
			if ((char *)0 == VFSThis.vfs_mountp)
				continue;
			if ('.' == VFSThis.vfs_mountp[0] && '\000' == VFSThis.vfs_mountp[1])
				continue;	/* ignore me */
			/* don't use lstat, auto-mounts?
			 */
			if (-1 == stat(VFSThis.vfs_mountp, &stOf))
				continue;
			if (stOf.st_dev != pST->st_dev)
				continue;
			/* found it */
			(void)strcpy(pcName, VFSThis.vfs_mountp);
			iRet = 1;
			break;
		}
		(void)fclose(fpVFstab);
	}
#else
	(void)setfsent();
	while ((struct fstab *)0 != (pFS = getfsent())) {
		if ('.' == pFS->fs_file[0] && '\000' == pFS->fs_file[1])
			continue;	/* ignore me */
		/* don't use lstat, auto-mounts?
		 */
		if (-1 == stat(pFS->fs_file, &stOf))
			continue;
		if (stOf.st_dev != pST->st_dev)
			continue;
		/* found it */
		(void)strcpy(pcName, pFS->fs_file);
		iRet = 1;
		break;
	}
	(void)endfsent();
#endif
#endif
	return iRet;
}

/* locate the dir by inode in one of the given paths			(ksb)
 */
static int
FindPrefix(pcOut, pstDir, ppcList)
char *pcOut, **ppcList;
struct stat *pstDir;
{
	register char *pcSlash, *pcTry, *pcNext;
	auto struct stat stThis;
	register int r;

	*pcOut = '\000';
	while ((char *)0 != (pcTry = *ppcList++)) {
		r = 0;
		for (pcSlash = strrchr(pcTry, '/'); (char *)0 != pcSlash && pcTry != pcSlash; pcSlash = pcNext) {
			*pcSlash = '\000';
			pcNext = strrchr(pcTry, '/');
			if (0 == STAT_CALL(pcTry, & stThis)) {
				if (stThis.st_dev != pstDir->st_dev) {
					r = 1;
				} else if (stThis.st_ino == pstDir->st_ino) {
					(void)strcpy(pcOut, pcTry);
					r = 1;
				}
			}
			*pcSlash = '/';
			if (r)
				break;
		}
		if ('\000' != *pcOut)
			return 1;
	}
	return 0;
}

/* if a path is really an OLD dir we have to remap things a little	(ksb)
 * [that is is the file in acinst is really under a directory named "OLD"]
 */
static int
SpellsOld(pcOld, iOldLen, pLI)
int iOldLen;
char *pcOld;
LINK *pLI;
{
	register char *pcSlash, *pcNext;

	for (pcSlash = pLI->acinst; (char *)0 != pcSlash && '\000' != *pcSlash; pcSlash = pcNext) {
		while ('/' == *pcSlash)
			++pcSlash;
		if ('\000' == *pcSlash)
			break;
		pcNext = strchr(pcSlash, '/');
		if (0 != strncmp(pcOld, pcSlash, iOldLen) || '/' != pcSlash[iOldLen]) {
			continue;
		}
		(void)strcpy(pLI->acback, pLI->acinst);
		pLI->pcbtail = pLI->acback + (pcNext - pLI->acinst);
		pLI->pstold = pLI->pstup;
		pLI->pstup = (struct stat *)0;
		pLI->acinst[0] = '\000';
		pLI->pcitail = (char *)0;
		return 1;
	}
	return 0;
}

/* search for all the links to a given file in a diretory		(ksb)
 */
static struct stat *pSTSearch;
static int
LinkSelect(pDE)
const struct DIRENT *pDE;
{
	register const char *pc;

	pc = pDE->d_name;
	if ('.' == pc[0] && ('\000' == pc[1] || ('.' == pc[1] && '\000' == pc[2]))) {
		return 0;
	}
	return (struct stat *)0 != pSTSearch && pDE->d_ino == pSTSearch->st_ino;
}

/* return the files that are no "." or ".."				(ksb)
 */
static int
ChildSelect(pDE)
const struct DIRENT *pDE;
{
	register const char *pc;

	pc = pDE->d_name;
	return !('.' == pc[0] && ('\000' == pc[1] || ('.' == pc[1] && '\000' == pc[2])));
}

/* look through the whole bloody file system for			(ksb)
 * the inode in question.  Lucky readdir returns ino.
 * return how many we still need.
 */
int
SearchHarder(pcTop, pSTNeed, ppcList, iNeed)
struct stat *pSTNeed;
char *pcTop, **ppcList;
int iNeed;
{
	register struct DIRENT *pDE;
	register char *pc, *pcTail;
	register int i, iKeep;
	auto struct DIRENT **ppDE;
	auto struct stat stNode;

	if (-1 == (i = scandir(pcTop, &ppDE, ChildSelect, (void *)0))) {
		return iNeed;
	}

	if ('/' == pcTop[0] && '\000' == pcTop[1]) {
		pcTail = pcTop+1;
	} else {
		pcTail = pcTop+strlen(pcTop);
		if ('/' != pcTail[-1])
			*pcTail++ = '/';
	}

	iKeep = i;
	while (i > 0) {
		pDE = ppDE[--i];
		pc = pDE->d_name;
		if ('.' == pc[0] && ('\000' == pc[1] || ('.' == pc[1] && '\000' == pc[2]))) {
			continue;
		}
		if (pDE->d_ino != pSTNeed->st_ino) {
			continue;
		}
		(void)strcpy(pcTail, pc);
		if (-1 == STAT_CALL(pcTop, & stNode) || pSTNeed->st_dev != stNode.st_dev) {
			continue;
		}
		ppcList[--iNeed] = strdup(pcTop);
		ppDE[i+1] = (struct DIRENT *)0;
	}

	/* OK, give up on this level, look for directories below to scan
	 */
	for (i = iKeep; 0 != iNeed && i > 0; /* */) {
		if ((struct DIRENT *)0 == (pDE = ppDE[--i])) {
			/* found in the loop above */
			continue;
		}
		pc = pDE->d_name;
		if ('.' == pc[0] && ('\000' == pc[1] || ('.' == pc[1] && '\000' == pc[2]))) {
			continue;
		}
		(void)strcpy(pcTail, pc);
		if (-1 == STAT_CALL(pcTop, & stNode) || S_IFDIR != (stNode.st_mode & S_IFMT) || pSTNeed->st_dev != stNode.st_dev) {
			continue;
		}
		iNeed = SearchHarder(pcTop, pSTNeed, ppcList, iNeed);
	}

	/* drop memory we hold */
	for (i = iKeep; i-- > 0; /* empty */) {
		if ((struct DIRENT *)0 != ppDE[i])
			free((void *)ppDE[i]);
	}
	free((void *)ppDE);

	return iNeed;
}

#if !VINST

/* remap the path names we found such that local files are		(ksb)
 * given as relative (. and ..) and others are left alone.
 * N.B. subdirs of . are left alone as well!
 *
 * Inv: + .. is pcDotDot (length lDotDot) . is pcPwd (length lPwd)
 *	+ pcPwd is _just_ the tail (from pcDotDot down 1 level)
 *	+ the strings are malloc'd and replaced, old ones free'd
 *
 * e.g. me(ppcInfo, "/usr/local", 10, "info", 4);
 */
static void
ReMapNames(ppcList, pcDotDot, lDotDot, pcPwd, lPwd)
char **ppcList, *pcDotDot, *pcPwd;
int lDotDot, lPwd;
{
	register char *pcWas, *pcMem;
	register int iChop;

	for (/* params */; (char *)0 != (pcWas = *ppcList); ++ppcList) {
		if (0 != strncmp(pcWas, pcDotDot, lDotDot) || '/' != pcWas[lDotDot])
			continue;
		/* in right place for .., check for . now
		 */
		iChop = lDotDot+1;
		if (0 == strncmp(pcWas+iChop, pcPwd, lPwd) && '/' == pcWas[iChop+lPwd])
			iChop += lPwd+1;

		/* be sure file not is in a subdir
		 */
		if ((char *)0 != strchr(pcWas+iChop, '/'))
			continue;

		/* should pcDotDot (pcPwd) be replaced by "../" ("./")?
		 * or ".." (".")?  Let's take it w/o the slash.
		 */
		pcMem = malloc((strlen(pcWas+iChop)|3)+5);
		(void)strcpy(pcMem, (iChop != lDotDot+1) + "..");
		if ('\000' != pcWas[iChop])
			(void)strcat(pcMem, pcWas+(iChop-1));
		/* free(pcWas); */
		*ppcList = pcMem;
	}
}

/* Find all the links in a binary dir to a given file in an OLD dir	(ksb)
 * note that the current working directory is important (it is an OLD dir).
 * We may reorder the ppDEBacks array if we must (w/o making scandir choke,
 * we hope).
 * Returns the number of link pairs found.
 *
 * Inv.s:
 *	+ there are at most (st_nlink+2) files that we care about
 *		st_nlink names + a de-mktemp'd name and it's backup
 *	+ some of these are paired
 *		$link and ../$link
 *		$dir/$link and $dir/OLD/$link
 *	  in the worst case
 *
 * Caller must have done something like:
 *	pLIList = (LINK *)calloc(pstBackup->st_nlink+1, sizeof(LINK));
 */
int
FindLinks(pcDir, pcOld, ppDEBacks, iBacks, pstBackup, pLIList)
char *pcDir, *pcOld;
struct DIRENT **ppDEBacks;
int iBacks;
struct stat *pstBackup;
register LINK *pLIList;
{
	register struct stat *pstUnique;
	register int i, iUnique, iFill;
	register long int liCount;
	register LINK *pLI;

	/* set all the link buffers to a knonw idle state just in case
	 * the caller used malloc rather than calloc (like ksb would).
	 */
	liCount = pstBackup->st_nlink+1;
	for (i = 0; i < liCount; ++i) {
		pLI = & pLIList[i];
		pLI->etactic = '.';
		pLI->acback[0] = pLI->acinst[0] = '\000';
		pLI->pcitail = pLI->pcbtail = pLI->pcotail = (char *)0;
		pLI->pstup = pLI->pstold = (struct stat *)0;
		pLI->ppDEold = (struct DIRENT **)0;
	}

	/* one for each link + one for the mktemp stripped filename
	 */
	pstUnique = (struct stat *)calloc(pstBackup->st_nlink+2+iBacks, sizeof(struct stat));
	if ((LINK *)0 == pLIList || (struct stat *)0 == pstUnique) {
		Die("FindLinks: calloc: no memory for matches vectors");
	}

	/* find all the links (no matter what) load them into the LINKs db
	 * (1) start with the files in OLD
	 */
	liCount = iFill = iUnique = 0;
	for (i = 0; i < iBacks; ++i) {
		pLI = & pLIList[iFill++];
		snprintf(pLI->acback, sizeof(pLI->acback), "./%s", ppDEBacks[i]->d_name);
		pLI->pcbtail = pLI->acback+1;
		pLI->pstold = pstBackup;
		pLI->ppDEold = & ppDEBacks[i];
		++liCount;
		(void)snprintf(pLI->acinst, sizeof(pLI->acback), "../%s", pLI->pcbtail+1);
		if (-1 == STAT_CALL(pLI->acinst, & pstUnique[iUnique])) {
			pLI->acinst[0] = '\000';
			continue;
		}
		pLI->pcitail = pLI->acinst+2;
		if (pstUnique[iUnique].st_dev == pstBackup->st_dev &&
			pstUnique[iUnique].st_ino == pstBackup->st_ino) {
			pLI->pstup = pstBackup;
			++liCount;
			continue;
		}
		pLI->pstup = & pstUnique[iUnique++];
	}

	/* (2) look through the files in OLD, try the old `strip off
	 * the mktemp suffix' trick to find related links in ..
	 */
	if (liCount < pstBackup->st_nlink) {
		for (i = 0; i < iFill; ++i) {
			pLI = & pLIList[i];
			if ('\000' != pLI->acinst[0]) {
				continue;
			}
			if (FindOrig(pLI->pcbtail+1, "..", pstBackup, pLI->acinst, & pstUnique[iUnique])) {
				/* got the one we need */;
			} else if (!FindOrig(pLI->pcbtail+1, "..", (struct stat *)0, pLI->acinst, & pstUnique[iUnique])) {
				continue;
			}
			/* known "../" prefix
			 */
			pLI->pcitail = pLI->acinst+2;
			if (pstUnique[iUnique].st_dev == pstBackup->st_dev &&
				pstUnique[iUnique].st_ino == pstBackup->st_ino) {
				pLI->pstup = pstBackup;
				++liCount;
				continue;
			}
			pLI->pstup = & pstUnique[iUnique++];
		}
	}

	/* (3) look for other files in ..
	 */
	if (liCount < pstBackup->st_nlink) {
		register int iInsts;
		auto struct DIRENT **ppDEInsts;

		pSTSearch = pstBackup;
		iInsts = scandir("..", &ppDEInsts, LinkSelect, (void *)0);
		pSTSearch = (struct stat *)0;
		if (-1 == iInsts) {
			(void)fprintf(stderr, "%s: scandir: %s: %s\n", progname, pcDir, strerror(errno));
			return 0;
		}
		/* fill in a LINK node for it
		 * don't free ppDEInsts[i], we keep points below
		 */
		for (i = 0; i < iInsts; ++i) {
			register int j;
			register char *pcLink;

			/* make sure the link is not already in a pair
			 * [alphasort is not worth the order N here].
			 */
			pcLink = ppDEInsts[i]->d_name;
			for (j = 0; j < iFill; ++j) {
				if ((char *)0 != pLIList[j].pcitail && 0 == strcmp(pcLink, pLIList[j].pcitail+1))
					break;
			}
			if (j != iFill) {
				pLIList[j].ppDEup = & ppDEInsts[i];
				continue;
			}
			pLI = & pLIList[iFill++];
			(void)snprintf(pLI->acinst, sizeof(pLI->acback), "../%s", pcLink);
			pLI->pcitail = pLI->acinst+2;
			pLI->ppDEup = & ppDEInsts[i];
			pLI->pstup = pstBackup;
			++liCount;
			(void)snprintf(pLI->acback, sizeof(pLI->acback), "./%s", pcLink);
			if (-1 == STAT_CALL(pLI->acback, & pstUnique[iUnique])) {
				pLI->acback[0] = '\000';
				continue;
			}
			/* This should never happen, if it does we should exit!
			 * [or someone built the link while we are running]
			 */
			pLI->pcbtail = pLI->acback+1;
			if (pstUnique[iUnique].st_dev == pstBackup->st_dev &&
				pstUnique[iUnique].st_ino == pstBackup->st_ino) {
				fprintf(stderr, "%s: broken inv. in %s cleanup (repaired)\n", progname, pcOld);
				pLI->pstold = pstBackup;
				++liCount;
				continue;
			}
			pLI->pstold = & pstUnique[iUnique++];
		}
	}

	/* (4) if we don't (yet) have all the links to the bad backup file
	 * we should search the whole fs under the mt.pt. for the inode
	 * number with scandir.  Sigh.
	 */
	if (liCount < pstBackup->st_nlink) {
		auto char acMountPt[MAXPATHLEN+2];
		auto char acWeAre[MAXPATHLEN+2];
		auto struct stat stDotdot;
		register char **ppcAll;
		register int iOldLen, iStart;

		iStart = iFill;
		ppcAll = (char **)calloc(pstBackup->st_nlink+1, sizeof(char *));
		if (-1 == STAT_CALL("..", &stDotdot)) {
			fprintf(stderr, "%s: stat: ..: %s\n", progname, strerror(errno));
			return 0;
		}
		(void)FindMtPt(pstBackup, acMountPt);
		if (fInteract){
			fprintf(fpOut, "%s: searching %s for references to inode #%ld [this may take a while]\n", progname, acMountPt, (long)pstBackup->st_ino);
			fflush(fpOut);
		}
		i = SearchHarder(acMountPt, pstBackup, ppcAll, pstBackup->st_nlink);
		if (i != 0) {
			fprintf(stderr, "%s: cannot find all references to inode #%ld on %s\n", progname, (long)pstBackup->st_ino, acMountPt);
			return 0;
		}
		ppcAll[pstBackup->st_nlink] = (char *)0;

		/* integrate them with the more primative scans
		 * we can search for the inode number of '..' in the full
		 * paths returned in the list to figure out which of
		 * the paths is *not* in OLD or .. -- I am really pushing.
		 */
		if (!FindPrefix(acWeAre, &stDotdot, ppcAll)) {
			fprintf(stderr, "%s: can't locate pwd\n", progname);
			return 0;
		}
		iOldLen = strlen(pcOld);
		ReMapNames(ppcAll, acWeAre, strlen(acWeAre), pcOld, iOldLen);

		/* I know, by this point you think we've lost it -- but we
		 * still know what's up.  Ignore the links in OLD (.) and
		 * the ones in ..; but take the full paths and the ones
		 * below . (sure.)
		 *
		 * Yow! We even (because the SearchHarder code puts them in the
		 * correct order) can group the acinst's with the acback's.
		 */
		for (i = 0; i < pstBackup->st_nlink; ++i) {
			register char *pcLink;
			register int j;
			register int iCandLen;

			pcLink = ppcAll[i];
			pLI = & pLIList[iFill];
			/* not in . or .. */
			if ('.' != pcLink[0]) {
				snprintf(pLI->acinst, sizeof(pLI->acback), "%s", pcLink);
				pLI->pcitail = strrchr(pLI->acinst, '/');
				pLI->pstup = pstBackup;
				pLI->acback[0] = '\000';
				pLI->pstold = (struct stat *)0;
				SpellsOld(pcOld, iOldLen, pLI);
			} else if ('/' == pcLink[1]) {
				(void)strcpy(pLI->acback, pcLink);
				pLI->pcbtail = pLI->acback+1;
				pLI->pstold = pstBackup;
				pLI->acinst[0] = '\000';
				pLI->pstup = (struct stat *)0;
				/* already put in OLD, don't check */
			} else { /* must be ../ */
				snprintf(pLI->acinst, sizeof(pLI->acback), "%s", pcLink);
				pLI->pcitail = pLI->acinst+2;
				pLI->pstup = pstBackup;
				pLI->acback[0] = '\000';
				pLI->pstold = (struct stat *)0;
			}

			/* If we have an -1 link (acback) and it is not in a
			 * propper subdir (named "OLD") freak a little
			 */
			if ('\000' != pLI->acback[0] && (char *)0 != strchr(pLI->pcbtail+1, '/')) {
				pLI->pcbtail[0] = '\000';
				fprintf(stderr, "%s: %s: %s directories should never have subdirectories (%s)\n", progname, pLI->acback, pcOld, pLI->pcbtail+1);
				pLI->pcbtail[0] = '/';
				iExitCode |= 16;
			}

			/* is this a duplicate link?
			 * we can't remap backs under insts, but we can avoid
			 * adding duplicate links nodes for them.
			 * As a side effect we ignore dup info here :-].
			 */
			for (j = 0; j < iFill; ++j) {
				if ('\000' != pLI->acback[0] && 0 == strcmp(pLIList[j].acback, pLI->acback))
					break;
				if ('\000' != pLI->acinst[0] && 0 == strcmp(pLIList[j].acinst, pLI->acinst))
					break;
			}
			if (j != iFill) {
				continue;
			}

			/* unique back link we add
			 */
			if ('\000' != pLI->acback[0]) {
				++iFill;
				++liCount;
				continue;
			}

			/* If a file is an acinst we look for an acback with
			 * no acinst that we can add it to.
			 *
			 * N.B.
			 * +  SearchHarder puts the subdirectory info
			 *    first in the list of files.  I really it pushes
			 *    them on a stack last to first.
			 * +  if no OLD dir links in remote dir, then
			 *    no compactions are done.
			 *
			 * Look through empty acinsts for one with an acback
			 * in the same place with the same name as ours.
			 */
			iCandLen = pLI->pcitail - pLI->acinst;
			for (j = 0; j < iFill; ++j) {
				if ('\000' != pLIList[j].acinst[0]) {
					continue;
				}
				/* same name in OLD as current
				 */
				if (0 != strcmp(pLIList[j].pcbtail, pLI->pcitail)) {
					continue;
				}
				/* same directory as current */
				if ('.' == pLIList[j].acback[0] && pLIList[j].pcbtail == pLIList[j].acback+1) {
					/* OLD is ., dir must be ..
					 */
					if ('.' != pLI->acinst[0] || '.' != pLI->acinst[1] || pLI->pcitail != pLI->acinst+2) {
						continue;
					}
				} else if (0 != strncmp(pLIList[j].acback, pLI->acinst, iCandLen) || '/' != pLIList[j].acback[iCandLen]) {
					/* not spelled right */
					continue;
				} else if (0 != strncmp(pcOld, pLIList[j].acback+iCandLen+1, iOldLen) || pLIList[j].pcbtail != pLIList[j].acback+iCandLen+1+iOldLen) {
					/* doesn't have just "OLD/" on the end
					 */
					continue;
				}
				(void)strcpy(pLIList[j].acinst, pLI->acinst);
				pLIList[j].pcitail = pLIList[j].acinst+iCandLen;
				pLIList[j].pstup = pstBackup;
				++liCount;
				break;
			}
			if (j != iFill) {
				continue;
			}
			/* we need this one, extend the LINK db and bump the
			 * count
			 */
			++iFill;
			++liCount;
		}
		/* the added links are still 2nd class because we didn't
		 * look for acinst's in all of them
		 * LLL: check out if in /OLD here really works...
		 */
		for (i = iStart; i < iFill; ++i) {
			register int fFound;

			pLI = & pLIList[i];
			if ('\000' != pLI->acinst[0]) {
				continue;
			}
			pLI->pcbtail[0] = '\000';
			pLI->pcbtail[-iOldLen-1] = '\000';
			fFound = FindOrig(pLI->pcbtail+1, '\000' == pLI->acback[0] ? "/" : pLI->acback, (struct stat *)0, pLI->acinst, & pstUnique[iUnique]);
			pLI->pcbtail[-iOldLen-1] = '/';
			pLI->pcbtail[0] = '/';
			if (! fFound) {
				pLI->acinst[0] = '\000';
				continue;
			}
			pLI->pstup = & pstUnique[iUnique++];
			pLI->pcitail = strrchr(pLI->acinst, '/');
		}

		/* drop the memory we hold
		 */
		for (i = 0; i < pstBackup->st_nlink; /*empty*/) {
			free((void *)ppcAll[i++]);
		}
		free((void *)ppcAll);
	}

	/* sanity check because the code above is so twisted	(ksb)
	 */
	if (liCount != pstBackup->st_nlink) {
		fprintf(fpOut, "%s: %s: internal track error %ld != %ld\n", progname, ppDEBacks[0]->d_name, liCount, (long)pstBackup->st_nlink);
		iExitCode |= 32;
		return 0;
	}
	for (i = 0; i < iFill; ++i) {
		pLI = & pLIList[i];
		if ('\000' == pLI->acinst[0]) {
			pLI->pcitail = (char *)0;
			pLI->pstup = (struct stat *)0;
		} else if ((struct stat *)0 == pLI->pstup) {
			fprintf(stderr, "%s: %s: missing stat member\n", progname, pLI->acinst);
		} else if ((char *)0 == pLI->pcitail || (struct stat *)0 == pLI->pstup) {
			fprintf(stderr, "%s: %s: incomplete internal installed link structure\n", progname, pLI->acinst);
		} else if (pLI->acinst > pLI->pcitail || pLI->acinst+sizeof(pLI->acinst) <= pLI->pcitail) {
			fprintf(stderr, "%s: %s: inst-tail is out of range\n", progname, pLI->acinst);
		} else if ('/' != pLI->pcitail[0]) {
			fprintf(stderr, "%s: %s: inst-tail is off\n", progname, pLI->acinst);
		}
		if ('\000' == pLI->acback[0]) {
			pLI->pcbtail = (char *)0;
			pLI->pstold = (struct stat *)0;
		} else if ((struct stat *)0 == pLI->pstold) {
			fprintf(stderr, "%s: %s: missing stat member\n", progname, pLI->acback);
		} else if ((char *)0 == pLI->pcbtail || (struct stat *)0 == pLI->pstold) {
			fprintf(stderr, "%s: %s: incomplete internal backup link structure\n", progname, pLI->acback);
		} else if (pLI->acback > pLI->pcbtail || pLI->acback+sizeof(pLI->acback) <= pLI->pcbtail) {
			fprintf(stderr, "%s: %s: back-tail is out of range\n", progname, pLI->acback);
		} else if ('/' != pLI->pcbtail[0]) {
			fprintf(stderr, "%s: %s: back-tail is off\n", progname, pLI->acback);
		}
		if ('\000' != pLI->acback[0]) {
			pLI->pcbtail[0] = '\000';
			pLI->pcotail = strrchr(pLI->acback, '/');
			pLI->pcbtail[0] = '/';
		}
	}

	/* dump back how many we found
	 */
	return iFill;
}

#endif
