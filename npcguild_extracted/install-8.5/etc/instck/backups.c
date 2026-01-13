/* $Id: backups.c,v 8.17 1997/01/12 20:08:51 kb207252 Exp $
 *
 * These routines fix the backup directoies we hold -1 revisions	(ksb)
 * of installed files in.  The stage directory (often called "OLD")
 * contains a busted backup (links pointing all over the file system)
 * and we clean it up.  We hope.
 *
 * None of the routines in here use "clever" code.
 */

#if !defined(lint)
static char rcsid[] =
	"$Id: backups.c,v 8.17 1997/01/12 20:08:51 kb207252 Exp $";
#endif	/* !lint */

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

#if HAVE_SGSPARAM
#include <sgsparam.h>
#endif

#if HAS_NDIR
#include <ndir.h>
#else
#if HAS_DIRENT
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
#include "backups.h"

/* build a installed file name for the OLD file name in the link	(ksb)
 */
static void
SynthInst(pLI)
register LINK *pLI;
{
	if ((char *)0 == pLI->pcotail) {
		sprintf(pLI->acinst, "../%s", pLI->pcbtail+1);
		pLI->pcitail = pLI->acinst+2;
	} else {
		pLI->pcotail[0] = '\000';
		sprintf(pLI->acinst, "%s/%s", pLI->acback, pLI->pcbtail+1);
		pLI->pcitail = pLI->acinst+strlen(pLI->acback);
		pLI->pcotail[0] = '/';
	}
}

/* consult the Driver for an opinion (yes or no)			(ksb)
 * return 1 to make the caller bailout of the whole operation
 */
int
YesNo(fpIn, piAns)
FILE *fpIn;
int *piAns;
{
	auto char acAns[MAXANS];
	register char *pcTemp, **ppc;
	static char *apcYHelp[] = {
		"f  skip to the next tactic",
		"h  print this help message",
		"n  do not do this",
		"y  try this tactic",
		"q  quit the program",
		(char *)0
	};

	if (fYesYes) {
		fprintf(fpOut, " [y] yes\n");
		*piAns = 1;
		return 0;
	}

	for (;;) {
		fprintf(fpOut, " [%sfqh] ", *piAns ? "yn" : "ny");
		fflush(fpOut);

		if ((char *)0 == fgets(acAns, sizeof(acAns), fpIn)) {
			(void)clearerr(fpIn);
			fputs("[EOF]\n", fpOut);
			return 1;
		}
		pcTemp = acAns;
		while (isspace(*pcTemp) && '\n' != *pcTemp)
			++pcTemp;
		if (!isatty(fileno(fpIn))) {
			fputs(pcTemp, fpOut);
		}
		switch (*pcTemp) {
		case '-':	/* undocumented (set -y) option		*/
			if ('y' == pcTemp[1]) {
				fYesYes = 1;
				*piAns = 1;
				return 0;
			}
		default:
			(void)fprintf(stderr, "%s: unknown answer `%c\'\n", progname, *pcTemp);
		case 'h':
		case 'H':
		case '?':
			for (ppc = apcYHelp; (char *)0 != *ppc; ++ppc) {
				(void)fprintf(fpOut, "%s\n", *ppc);
			}
			fprintf(fpOut, "Your choice? ");
			fflush(fpOut);
			continue;

		case 'y':	/* yeah, run it	*/
		case 'Y':
			*piAns = 1;
			break;

		case 'q':	/* quit this damn program		*/
		case 'Q':
			exit(0);

		case 'n':	/* no thanks, do I have other options?	*/
		case 'N':
			*piAns = 0;
			/*fallthrough*/
		case '\n':
			break;

		case 'f':	/* next file, I'm lost			*/
		case 'F':
			return 1;
		}
		break;
	}
	return 0;
}

/* consult the Driver for an integer, no special checks			(ksb)
 * Well, we do check for EOF, and a bad scanf.
 *  -1   EOF
 *   0   OK, got an int (or just \n)
 *  'n'  no thanks, do I have other options?
 *  'f'  next file, I'm lost
 */
static int
GetInt(fpIn, piPick)
FILE *fpIn;
int *piPick;
{
	auto char acLine[24];	/* 24 digits is plentey for 64 bits */
	register int i;
	register char *pc, **ppc;
	static char *apcHelp[] = {
		"we need an integer or a key letter:",
		"f  let's try another file",
		"h  this help message",
		"n  do not do this at all, but do I have another choice",
		"q  quit",
		(char *)0
	};

	if ((char *)0 == fgets(acLine, sizeof(acLine), fpIn)) {
		return EOF;
	}

	for (i = 0; i < sizeof(acLine); ++i) {
		if ('\n' == acLine[i] || !isspace(acLine[i])) {
			break;
		}
	}
	if (isdigit(acLine[i]) || '+' == acLine[i] || '-' == acLine[i]) {
		if (1 != sscanf(acLine+i, "%d", piPick))
			return EOF;
		return 0;
	}
	switch (acLine[i]) {
	case 'q':
	case 'Q':
		exit(0);
	case '\n':	/* accept default value */
		return 0;
	case 'n':
	case 'N':
		return 'n';
	case 'f':
	case 'F':
		return 'f';
	default:
		fprintf(stderr, "%s: unknown integer `%s'\n", progname, acLine+i);
		/* fallthrough */
	case 'h':
	case 'H':
	case '?':
		for (ppc = apcHelp; (char *)0 != (pc = *ppc++); /* empty */) {
			fprintf(fpOut, "%s\n", pc);
		}
		break;
	}
	fprintf(fpOut, "What value [%d]? ", *piPick);
	return GetInt(fpIn, piPick);	/* tail recursion is free, right? */
}

/* is the file name a "crufty file" {from patch or the like}		(ksb)
 *	"*.FCS"		First Customer Ship from Sun
 *	"*.orig"	from patch
 *	"*~"		"
 *	"#*"		from emacs (covers '#inst$$' from install too)
 *	"*.OLD"		from ad-hoc backups
 *	"*.old"		" (*.Old too)
 *	"*.bak"		" (from DOS programmers or ispell)
 */
static int
IsCruft(pcName)
char *pcName;
{
	register char *pcDot;

	if ('#' == pcName[0]) {
		return 1;
	}
	if ((char *)0 != (pcDot = strrchr(pcName, '.'))) {
		switch (*++pcDot) {
		case 'F':
			if (0 == strcmp(pcDot, "FCS")) {
				return 1;
			}
			break;
		case 'o':
			if (0 == strcmp(pcDot, "old") || 0 == strcmp(pcDot, "orig")) {
				return 1;
			}
			break;
		case 'O':
			if (0 == strcmp(pcDot, "OLD") || 0 == strcmp(pcDot, "Old")) {
				return 1;
			}
			break;
		case 'b':
			if (0 == strcmp(pcDot, "bak")) {
				return 1;
			}
			break;
		default:
			break;
		}
	}
	pcDot = pcName + strlen(pcName);
	if (pcDot > pcName && '~' == pcDot[-1]) {
		return 1;
	}
	return 0;
}

/* -- the seven tactics of highly useful instck's -- */


/* ways to fix bad backup (-1) directories:				(ksb)
 * Each of the tactics below has a description and a tag line.
 * Then an implementation in two parts:  A guard function and an
 * implementation function.
 *
 * In the comments "OT" is an Original Target, that is the file we
 * think we might have been installing when things went awry.  Also
 * a BL is a Bad Link (one of the stray links we are moving or
 * reinstalling).
 */

/* gut's theory of operation:						(ksb)
 * it is always OK to remove an OLD link, if the file above
 * is an installed link to the same bad file [remove].
 */
static char t_gut[] =
	"remove all links from backup directories",
v_gut[] = "\
For purge to do its jobs there should be only one backup name for each\n\
file cached in the backup directory.  We can see more than one hard link\n\
to the backup file in OLD, and all the bad links are contained in OLD.\n\
\n\
This strategy removes all names to fix the problem.  This is usually\n\
safe but not very clever.\n";

/* Gut OLD of extra links:						(ksb)
 * if we have no Original Target and if all the remaining links are
 * in OLD.
 */
static int
c_gut(iFill, pLIList, pLIOrig, pstBackup)
int iFill;
LINK *pLIList, *pLIOrig;
struct stat *pstBackup;
{
	register int i, iLinksInOld;

	iLinksInOld = 0;
	for (i = 0; i < iFill; ++i) {
		if (pLIList[i].pstold == pstBackup)
			++iLinksInOld;
	}
	return pstBackup->st_nlink == iLinksInOld;
}

/* r_gut() is below purge to save an forward declaration
 */

/* link purge but with an OT we try to keep a link
 */
static char t_purge[] =
	"remove all links, except the original one, from backup directories",
v_purge[] = "\
We found all the duplicate backup links are in OLD directories.\n\
We can remove all of them, but we'll save one to be safe, which purge\n\
will delete for us after the next system dump.\n";

static int
c_purge(iFill, pLIList, pLIOrig, pstBackup)
int iFill;
LINK *pLIList, *pLIOrig;
struct stat *pstBackup;
{
	return (LINK *)0 != pLIOrig && c_gut(iFill, pLIList, pLIOrig, pstBackup);
}

/* ... just remove all but one of the links and let purge(8) eat the last
 * one later.
 *
 * N.B.: purge will do this if it can see all the OLD dirs, but it might
 * not have been run yet, or it can't see them all (NFS or hidden by a
 * mount point).
 * Return the O.T's link (or the one they kept) for more checking.
 */
static struct DIRENT *
r_purge(iFill, pLIList, pLIOrig, pstBackup, pcIn)
int iFill;
LINK *pLIList, *pLIOrig;
struct stat *pstBackup;
char *pcIn;
{
	register int i;
	register LINK *pLIThis, *pLIPass2;
	auto iRm;

	/* there is a hack in this loop to give us a chance to
	 * pick up the OT if we skipped it the first time and they
	 * kept a link (making it the real OT).
	 */
	pLIPass2 = (LINK *)0;
	for (i = 0; i <= iFill; ++i) {
		if (i < iFill) {
			pLIThis = & pLIList[i];
			if (pLIOrig == pLIThis || pLIList[i].pstold != pstBackup) {
				pLIThis->etactic = 'n';
				continue;
			}
		} else if ((LINK *)0 == pLIPass2 || 'n' != pLIPass2->etactic) {
			break;
		} else {
			pLIThis = pLIPass2;
		}
		if ((LINK *)0 != pLIOrig) {
			(void)fprintf(fpOut, "%s: unlinking %s from %s\n", progname, pLIThis->acback, pLIOrig->acback);
		} else {
			(void)fprintf(fpOut, "%s: unlinking %s\n", progname, pLIThis->acback);
		}
		apcRm[2] = "-f";
		apcRm[3] = pLIThis->acback;
		apcRm[4] = (char *)0;
		switch (QExec(apcRm, "a duplicate backup link", &iRm)) {
		case QE_RAN:
			pLIThis->etactic = 0 == iRm ? 'x' : 'f';
			if (0 == iRm && (struct DIRENT **)0 != pLIThis->ppDEold) {
				pLIThis->ppDEold[0] = (struct DIRENT *)0;
			}
			break;
		case QE_SKIP:
			return (struct DIRENT *)0;
		case QE_NEXT:
			/* left a link, if we didn't have an OT -- now we do
			 * else if the OT is still not asked we ask later
			 * else ask about the OT at the end
			 */
			if ((LINK *)0 == pLIPass2)
				pLIPass2 = pLIOrig;
			pLIOrig = pLIThis;
			break;
		}
	}

	/* if the OT is now in another OLD dir we do not have a
	 * (struct DIRENT *) handle on it to return, sigh. We'd have
	 * to malloc one and return it here (which would not get free'd
	 * -- hum. think, think, think... ksb
	 */
	if ((LINK *)0 == pLIOrig || (struct DIRENT **)0 == pLIOrig->ppDEold)
		return (struct DIRENT *)0;
	return pLIOrig->ppDEold[0];
}

/* call the above with a nil OT link for gut opperation			(ksb)
 * If we are not interactive gut defaults to leaving the first link
 * with a ppDE.
 */
static struct DIRENT *
r_gut(iFill, pLIList, pLIOrig, pstBackup, pcIn)
int iFill;
LINK *pLIList, *pLIOrig;
struct stat *pstBackup;
char *pcIn;
{
	register LINK *pLIInter;
	register int i;

	pLIInter = (LINK *)0;
	for (i = 0; fInteract && i < iFill && (LINK *)0 == pLIInter; ++i) {
		if ((struct DIRENT **)0 != pLIList[i].ppDEold && (struct DIRENT *)0 != pLIList[i].ppDEold[0])
			pLIInter = & pLIList[i];
	}
	return r_purge(iFill, pLIList, pLIInter, pstBackup, pcIn);
}


/* finish's theory of operation:					(ksb)
 *	+ we have an OT and its OLD is a BL.
 *	+ all other BLs are either
 *		OLD's with an OT ..
 *		OLD's with a BL .. 
 *	  	..'s with no OLD
 *		..'s with a a BL OLD
 *	+ there must be at least one BL in ..
 * {this means that we don't have any spurious files in the mix.}
 * [see the action side for the rest]
 */
static char t_finish[] =
	"finish the installation",
v_finish[] = "\
We have enough information to finish the installation.  The bad links\n\
in the target directories will point to the original target and who\n\
could ask for anything more?\n";

static int
c_finish(iFill, pLIList, pLIOrig, pstBackup, pcIn)
int iFill;
LINK *pLIList, *pLIOrig;
struct stat *pstBackup;
char *pcIn;
{
	register int i, iInDotDot, iInOld;
	register LINK *pLI;

	if ((LINK *)0 == pLIOrig) {	/* no OT -> can't do this */
		return 0;
	}

	/* a BL in OLD should have a OT or BL .. link, not other files
	 * in the mix, please.
	 */
	iInOld = iInDotDot = 0;
	for (i = 0; i < iFill; ++i) {
		pLI = & pLIList[i];
		if ('\000' != pLI->acback[0] && pstBackup == pLI->pstold) {
			++iInOld;
		}
		if ('\000' == pLI->acinst[0]) {
			continue;
		}
		if (pstBackup == pLI->pstup) {
			++iInDotDot;
			continue;
		}
		/* here we could use the etactic information to get more from
		 * life. XXX
		 */
		if (pLIOrig->pstup->st_dev != pLI->pstup->st_dev ||
		    pLIOrig->pstup->st_ino != pLI->pstup->st_ino) {
			return 0;
		}
	}
	return pstBackup->st_nlink == (iInOld + iInDotDot);
}

/* finish implementation						(ksb)
 *	-> unlink .. BLs and relink to OT
 *	-> unlink OLD BLs, save the OT BL [call purge's r_)
 *	=> return OT's BL for more checking [in purge]
 *	-> sanity check OT's BL with a stat call [in caller]
 */
static struct DIRENT *
r_finish(iFill, pLIList, pLIOrig, pstBackup, pcIn)
int iFill;
LINK *pLIList, *pLIOrig;
struct stat *pstBackup;
char *pcIn;
{
	register int i;
	register LINK *pLI, *pLIPass2;
	auto int iRm;

	/* relink .. links to pLIOrig->acinst
	 */
	pLIPass2 = (LINK *)0;
	for (i = 0; i < iFill; ++i) {
		pLI = & pLIList[i];
		if (pLIOrig == pLI) {
			continue;
		}
		if ('\000' == pLI->acinst[0] && pstBackup == pLI->pstold) {
			SynthInst(pLI);
			apcLn[2] = pLIOrig->acinst;
			apcLn[3] = pLI->acinst;
			apcLn[4] = (char *)0;
			switch (QExec(apcLn, "a missing link to the installed file", &iRm)) {
			case QE_RAN:
				if (0 != iRm) {
			case QE_NEXT:
					pLI->acinst[0] = '\000';
				}
				break;
			case QE_SKIP:
				return (struct DIRENT *)0;
			}
			continue;
		}
		if (pstBackup != pLI->pstup) {
			continue;
		}
		fprintf(fpOut, "%s: moving backup link %s to target %s\n", progname, pLI->acinst, pLIOrig->acinst);
		apcRm[2] = "-f";
		apcRm[3] = pLI->acinst;
		apcRm[4] = (char *)0;
		switch (QExec(apcRm, "a link to a backup file", &iRm)) {
		case QE_RAN:
			if (0 == iRm && (struct DIRENT **)0 != pLI->ppDEold) {
				pLI->ppDEold[0] = (struct DIRENT *)0;
			}
			apcLn[2] = pLIOrig->acinst;
			apcLn[3] = pLI->acinst;
			apcLn[4] = (char *)0;
			++fYesYes;
			(void)QExec(apcLn, "a missing link to the target file", &iRm);
			--fYesYes;
			break;
		case QE_SKIP:
			return (struct DIRENT *)0;
		case QE_NEXT:
			/* left a link, we can keep that one rather than
			 * one from the OLD directory
			 */
			if ((LINK *)0 == pLIPass2)
				pLIPass2 = pLI;
			break;
		}
	}
	if ((LINK *)0 == pLIPass2) {
		return r_purge(iFill, pLIList, pLIOrig, pstBackup, pcIn);
	}

	/* they kept at least one in .., try to delete all in OLD if they
	 * keep one of the links in OLD too then we can return that pDE
	 */
	return r_purge(iFill, pLIList, (LINK *)0, pstBackup, pcIn);
}


static char t_backout[] =
	"restore file, move all the bad links to the target directory",
v_backout[] = "\
We found an OLD file that looks like it got put into OLD by mistake, or\n\
by an interupted installation.\n\
\n\
We are going to try to move it back into the target directory for you,\n\
as well as all the links to that program.  This only fails when the target\n\
program has lost it's setuid (or setgid) bit due to a patch failure (so\n\
check the mode on the file when we are all done).\n";

/* Backout of the change:						(ksb)
 *	+ no OT at all
 *	+ BLs in OLD have no .., or BL ..
 *	+ at least one BL in .. for non-interactive [much better odds]
 */
static int
c_backout(iFill, pLIList, pLIOrig, pstBackup)
int iFill;
LINK *pLIList, *pLIOrig;
struct stat *pstBackup;
{
	register int iDotDot, i;
	register LINK *pLI;

	if ((LINK *)0 != pLIOrig) {
		return 0;
	}

	iDotDot = 0;
	for (i = 0; i < iFill; ++i) {
		pLI = & pLIList[i];
		if ('\000' == pLI->acinst[0] || pstBackup != pLI->pstold) {
			continue;
		}
		if (pstBackup != pLI->pstup) {
			return 0;
		}
		++iDotDot;
	}

	return fInteract || 0 != iDotDot;
}

/* [backout implementation]						(ksb)
 *	-> move all OLD BLs to .. [unlink the "dups"]
 *	=> return no further checks to do here
 */
static struct DIRENT *
r_backout(iFill, pLIList, pLIOrig, pstBackup)
int iFill;
LINK *pLIList, *pLIOrig;
struct stat *pstBackup;
{
	register int i;
	register LINK *pLI;
	auto int iRm, iYesMod;
	auto struct DIRENT *pDERet;

	pDERet = (struct DIRENT *)0;
	for (i = 0; i < iFill; ++i) {
		pLI = & pLIList[i];
		if ('\000' == pLI->acback[0] || pstBackup != pLI->pstold) {
			continue;
		}

		/* if no acinst, link it to the OLD name in the parent
		 * directory
		 */
		iYesMod = '\000' == pLI->acinst[0];
		if (iYesMod) {
			/* old in ., link to ..; else use paths
			 */
			SynthInst(pLI);
			apcLn[2] = pLI->acback;
			apcLn[3] = pLI->acinst;
			apcLn[4] = (char *)0;
			fprintf(fpOut, "%s: moving %s to %s\n", progname, pLI->acback, pLI->acinst);
			switch (QExec(apcLn, "a missing link to file we are putting back", &iRm)) {
			case QE_RAN:
				break;
			case QE_SKIP:
				return (struct DIRENT *)0;
			case QE_NEXT:
				continue;
			}
			if (0 != iRm) {
				continue;
			}
			pLI->pstup = pLI->pstold;
		}

		/* now remove the OLD one
		 */
		apcRm[2] = "-f";
		apcRm[3] = pLI->acback;
		apcRm[4] = (char *)0;
		fYesYes += iYesMod;
		switch (QExec(apcRm, "a duplicate link to the recovered file", &iRm)) {
		case QE_RAN:
			break;
		case QE_SKIP:
			return (struct DIRENT *)0;
		case QE_NEXT:
			if ((struct DIRENT *)0 == pDERet && (struct DIRENT **)0 != pLI->ppDEold)
				pDERet = pLI->ppDEold[0];
			continue;
		}
		fYesYes -= iYesMod;
		if (0 != iRm) {
			if ((struct DIRENT *)0 == pDERet && (struct DIRENT **)0 != pLI->ppDEold)
				pDERet = pLI->ppDEold[0];
			continue;
		}
		pLI->pstold = (struct stat *)0;
		pLI->acback[0] = '\000';
	}

	/* return an existing OLD link if we have one
	 */
	return pDERet;
}


/* oops's theory of operation:
 * a sanity check that we don't have all the BLs in ..
 */
static char t_oops[] = "all bad links are in a target directory",
v_oops[] = "\
All the bad links are in target directories: this means we should\n\
not even be in this code at all.\n";

/* all links are in ..							(ksb)
 * we are insane, get out.
 */
static int
c_oops(iFill, pLIList, pLIOrig, pstBackup)
int iFill;
LINK *pLIList, *pLIOrig;
struct stat *pstBackup;
{
	register int i, iCount;
	register LINK *pLI;

	iCount = 0;
	for (i = 0; i < iFill; ++i) {
		pLI = & pLIList[i];
		if ('\000' == pLI->acinst[0]) {
			continue;
		}
		if (pstBackup == pLI->pstup) {
			++iCount;
		}
	}
	return iCount == pstBackup->st_nlink;
}

/* oops tactic: repair feature						(ksb)
 */
static struct DIRENT *
r_oops(iFill, pLIList, pLIOrig, pstBackup, pcIn)
int iFill;
LINK *pLIList, *pLIOrig;
struct stat *pstBackup;
char *pcIn;
{
	fprintf(stderr, "%s: oops: %s: internal error!\n", progname, pcIn);
	return (struct DIRENT *)0;
}

static char t_punt[] = "punt",
v_punt[] = "\
We don't know how to fix this problem programatically.  We'll output all\n\
the stuff we've found out so far for you and drop you into a shell.  When\n\
you've repaired the problem (with rm, mv, cp, ln, chmod and the like) you\n\
can exit the shell and we'll have another look.\n";

/* try something else?							(ksb)
 * I'm at a loss, what else can we try?  Interactively repair some stuff
 * and go on?  Give them a shell and re-try after?! Heh!
 */
static int
c_punt(iFill, pLIList, pLIOrig, pstBackup)
int iFill;
LINK *pLIList, *pLIOrig;
struct stat *pstBackup;
{
	return 1;
}

/* see the help text for what be might do, this should _not_		(ksb)
 * be a way to break root because instck is _never_ called setuid.
 * We'll log a nice table of what we found and fail, I guess.
 */
static struct DIRENT *
r_punt(iFill, pLIList, pLIOrig, pstBackup, pcDir)
int iFill;
LINK *pLIList, *pLIOrig;
struct stat *pstBackup;
char *pcDir;
{
	register int i, l;
	register int iLeft;

	iLeft = 0;
	for (i = 0; i < iFill; ++i) {
		l = strlen(pLIList[i].acinst);
		if (l > iLeft)
			iLeft = l;
	}
	++iLeft;

	fprintf(fpOut, "%s: %s found %d pairs, including all links\n", progname, pcDir, iFill);
	for (i = 0; i < iFill; ++i) {
		fprintf(fpOut, "%*s %c %s\n", iLeft, pLIList[i].acinst, pLIOrig == & pLIList[i] ? '<' : ' ', pLIList[i].acback);
	}
	if ((LINK *)0 != pLIOrig && (struct DIRENT **)0 != pLIOrig->ppDEold) {
		return pLIOrig->ppDEold[0];
	}
	if (pLIList->ppDEold) {
		return pLIList->ppDEold[0];
	}
	return (struct DIRENT *)0;
}

/* If we can move links from OLD into .. to balance the link
 * counts we should do that.
 *
 * Try re-linking links in an updir to an Original Target:		(ksb)
 * If one link in OLD has an unrelated up file we might be able
 * to redirect all the links in all the updirs to that file
 * (the target of a broken install?)
 * Return the pointer to the Original Target's backup for
 * more checking in the caller.
 */

static LINK_STRATEGY aLSAll[] = {
	{ 0, c_finish, r_finish, t_finish, v_finish },
	{ 0, c_backout, r_backout, t_backout, v_backout },
	{ 0, c_oops, r_oops, t_oops, v_oops },
	{ 0, c_purge, r_purge, t_purge, v_purge },
	{ 0, c_gut, r_gut, t_gut, v_gut },
	/* { 0, c_swap, r_swap, t_swap, v_swap }, */
	{ 0, c_punt, r_punt, t_punt, v_punt }
};

/* Repair all the links in a binary dir to a given file in an OLD dir	(ksb)
 * note that the current working directory is important (it is an OLD dir).
 * We may reorder the ppDEBacks array if we must (w/o making scandir choke,
 * we hope).
 * Returns a pDE to the file left in OLD, if any are, else (DIRENT *)0.
 *
 * Inv.s:
 *	+ iBacks > 0
 *	+ .. is another name for pcDir
 *	+ there are at most (st_nlink+2) files that we care about
 *		= st_nlink names + a de-mktemp'd name and it's backup
 *	+ some of these are paired
 *		$link and ../$link
 *		$dir/$link and $dir/OLD/$link
 *	  in the worst case
 */
struct DIRENT *
FixBackup(pcDir, pcOld, ppDEBacks, iBacks, pstBackup)
char *pcDir, *pcOld;
struct DIRENT **ppDEBacks;
int iBacks;
struct stat *pstBackup;
{
	register LINK *pLIList;
	register int i, iFill, iCount;
	register LINK *pLI, *pLIOrig;
	auto struct DIRENT *pDERet;
	auto char acIn[MAXPATHLEN+4];
	auto int aiMap[sizeof(aLSAll)/sizeof(aLSAll[0]) + 1];
	auto int iPick;

	/* LLL we should pick the short name, or the common prefix
	 * here if we can (man rather than whatis, vi rather than view).
	 */
	pDERet = ppDEBacks[0];
	(void)sprintf(acIn, "%s/%s", pcDir, pcOld);

	/* one for each link + one for the mktemp stripped filename
	 */
	pLIList = (LINK *)calloc(pstBackup->st_nlink+1, sizeof(LINK));
	iFill = FindLinks(pcDir, pcOld, ppDEBacks, iBacks, pstBackup, pLIList);

	if (0 == iFill) {
		fprintf(fpOut, "%s: %s/%s: cannot find all %d references to inode #%ld\n", progname, acIn, pDERet->d_name, pstBackup->st_nlink, pstBackup->st_ino);
		return pDERet;
	}
	fprintf(fpOut, "%s: %s/%s: link count %d should be 1\n", progname, acIn, pDERet->d_name, pstBackup->st_nlink);

	/* Now some discorse on our inv.s:				(ksb)
	 *    + we do know all the links to the file in question, at least.
	 *
	 *    + interactive folks can specify some data, non-interactive
	 *	execution might have to give up, even after all this work.
	 *
	 *    + if we have a well installed file (in some non-OLD directory)
	 *	we call this file an Original Target, let's look for it next.
	 */

	/* When looking for an Original Target file to relink the strays to:
	 * The original file would be LINK for which (pstup != pstBackup)
	 * and (pstold == pstBackup); search non-empty acinsts for the right
	 * one.
	 *
	 * If we find two candidates we pick the `better' one:
	 * a) Hey, they might be the same file! Either:
	 *	+ links to the same inode
	 *	+ same size (and mode) and `cmp -s f1 f2' return's 0
	 *	- other cases not found useful (unlike mkcat has others, man)
	 *
	 * b) Any zero length files
	 *	- count less, we'd rather have bits I'd say
	 *
	 * c) Symbolic links
	 *	+ count only if no other choice
	 *	+ only count if we can stat(2) the other end (LLL?)
	 *
	 * d) cruft files should count less (from patches and the like):
	 *	- should count less [XXX ksb -- not yet]
	 *
	 * e) in ties the file with a ppDE should win
	 * 	since (the way we work now) the ppDE ones are first
	 *	by taking the first one we do this mostly (XXX check)
	 */
	pLIOrig = (LINK *)0;

	iCount = 0;
	for (i = 0; i < iFill; ++i) {
		pLI = & pLIList[i];
		if ('\000' == pLI->acinst[0]) {
			pLI->etactic = '!';
			continue;
		}
		if (pstBackup == pLI->pstup) {
			pLI->etactic = '=';
			continue;
		}

		/* first found, take it.
		 */
		if ((LINK *)0 == pLIOrig) {
			pLI->etactic = '<';
			pLIOrig = pLI;
			++iCount;
			continue;
		}

#if HAVE_SYMLINKS
		/* take any real file in preference to a symbolic link
		 */
		if (S_IFLNK == (pLI->pstup->st_mode & S_IFMT)) {
			pLI->etactic = '@';
			continue;
		}
#endif
		/* duplicate links to the Original Target are OK:
		 * mark them by making the stat bufs be the same.
		 */
		if (pLIOrig->pstup->st_dev == pLI->pstup->st_dev &&
			pLIOrig->pstup->st_ino == pLI->pstup->st_ino) {
			pLI->pstup = pLIOrig->pstup;
			pLI->etactic = '\"';
			continue;
		}
		/* if they are the same size, are they dups of the same data?
		 * they should both be binary (or not) and readonly (or not)!
		 */
		if (pLIOrig->pstup->st_size == pLI->pstup->st_size && (0 == (pLIOrig->pstup->st_mode & 0111)) == (0 == (pLI->pstup->st_mode & 0111)) && (0 == (pLIOrig->pstup->st_mode & 0222)) == (0 == (pLI->pstup->st_mode & 0222))) {
			auto int iCmpOut;

			apcCmp[2] = "-s";
			apcCmp[3] = pLIOrig->acinst;
			apcCmp[4] = pLI->acinst;
			apcCmp[5] = (char *)0;
			++fYesYes;
			if (QE_RAN != QExec(apcCmp, "a check for identical files", &iCmpOut)) {
				iCmpOut = 1;
			}
			--fYesYes;
			/* It is really the target file, make it a link when
			 * we fix them (below).
			 */
			if (0 == iCmpOut) {
				fprintf(fpOut, "%s: %s and %s have identical contents\n", progname, pLIOrig->acinst, pLI->acinst);
				pLI->etactic = '\"';
				continue;
			}
		}

		/* else, keep the file that is not zero bytes long
		 */
		if (0 == pLI->pstup->st_size) {
			pLI->etactic = '0';
			continue;
		}

		pLI->etactic = '<';
		if (0 == pLIOrig->pstup->st_size) {
			pLIOrig->etactic = '0';
			pLIOrig = pLI;
			continue;
		}

		/* make pLIOrig point to the newest file, if this clever?
		 * make the old one as "possible" and ++count so we'll ask
		 * or fail.
		 */
		if (pLI->pstup->st_ctime > pLIOrig->pstup->st_ctime) {
			pLIOrig = pLI;
		}
		++iCount;
	}

	/* ask the Driver which installed file is the Original Target.
	 * We can't tell (or guess) from what we see.  If we can't ask
	 * fall back to other checks.
	 */
	if (1 < iCount) {
		register int j, *piLens;
		auto int iPick;
		static char acFmt[] = "%3d\t%s\n";

		/* fInteract is set, ask which OT we should pick
		 * ask which one we should take.  We suggest that pLIOrig
		 * might be the best choice.
		 */
		piLens = (int *)calloc(iCount, sizeof(int));
		if ((int *)0 == piLens) {
			fprintf(stderr, "%s: no memory!\n", progname);
			exit(1);
		}

		iPick = 0;
		fprintf(fpOut, "%s: %s/%s: %d possible original targets:\n", progname, acIn, pDERet->d_name, iCount);
		if (fInteract) {
			fprintf(fpOut, acFmt, 0, "no target at all");
		}
		for (i = j = 0; j < iFill; ++j) {
			pLI = & pLIList[j];
			if ('<' != pLI->etactic) {
				continue;
			}
			piLens[i] = j;
			fprintf(fpOut, acFmt, ++i, pLI->acinst);
			if (pLIOrig == pLI) {
				iPick = i;
			}
		}
		if (!fInteract) {
			free((char *) pLIList);
			free((char *) piLens);
			return pDERet;
		}
		j = iPick;
		for (pLIOrig = (LINK *)0; (LINK *)0 == pLIOrig; iPick = j) {
			fprintf(fpOut, "%s: Which one should do you want to use {n, f, ?, 0-%d} [%d]? ", progname, i, iPick);
			fflush(fpOut);

			/* handles "next tactic"(1) and "skip file"(2)
			 */
			switch (GetInt(stdin, & iPick)) {
			case 0:
				break;
			case 'n':
				return pDERet;
			case 'f':
			case EOF:
				return (struct DIRENT *)0;
			default:
				fprintf(stderr, "%s: GetInt: internal error\n", progname);
				exit(1);
			}
			if (0 == iPick) {
				pLIOrig = (LINK *)0;
				break;
			}
			if (iPick > 0 && iPick <= i) {
				pLIOrig = & pLIList[piLens[iPick-1]];
				break;
			}
			fprintf(fpOut, "%s: %d is not a valid choice\n", progname, iPick);
		}
		free((char *)piLens);
	}

	/* Inv.: pLIOrig is the O.T. we want or a NULL pointer
	 */
	if ((LINK *)0 != pLIOrig) {
		fprintf(fpOut, "%s: original target for this install is `%s'\n", progname, pLIOrig->acinst);
	}

	/* Repair the mess we found with what we know.
	 * probe each strategy and display the ones that apply.
	 */
	iCount = 0;
	for (i = 0; i < sizeof(aLSAll)/sizeof(aLSAll[0]); ++i) {
		aLSAll[i].wuser = (aLSAll[i].pfiMay)(iFill, pLIList, pLIOrig, pstBackup, acIn);
		if (0 == aLSAll[i].wuser) {
			continue;
		}
		aiMap[iCount] = i;
		if (0 == iCount++) {
			fprintf(fpOut, "%s: %s: may:\n", progname, acIn);
		}
		fprintf(fpOut, "%2d) %s\n", iCount, aLSAll[i].pctext);
	}

	if (0 == iCount) {
		fprintf(fpOut, "%s: %s/%s: no tactic in my code to fix this\n", progname, acIn, pDERet->d_name);
		free((char *) pLIList);
		return pDERet;
	}

	/* we told them what they can do, now ask which one...
	 */
	while (0 != (iPick = 1)) {
		fprintf(fpOut, "Choose tactic {n, f, ?, or integer} [%d]: ", iPick);
		fflush(fpOut);
		switch (GetInt(stdin, & iPick)) {
		case 0:
			break;
		case 'n':
			return pDERet;
		case 'f':
		case EOF:
			return (struct DIRENT *)0;
		default:
			fprintf(stderr, "%s: FixBackup: internal error\n", progname);
			exit(1);
		}
		if (iPick >= 1 && iPick <= iCount)
			break;
	}
	iPick = aiMap[iPick-1];

	/* call the fix-up function and get out.  The function returns the
	 * correct (struct DIRENT *) to continue the checking, or NULL.
	 */
	pDERet = (aLSAll[iPick].pfiDo)(iFill, pLIList, pLIOrig, pstBackup, acIn);
	/* if the OT'BL has a bad link count still we complain
	 */
	if ((struct DIRENT *)0 != pDERet && -1 != STAT_CALL(pDERet->d_name, pstBackup) && 1 != pstBackup->st_nlink) {
		fprintf(fpOut, "%s: %s/%s: link count is still %d (should be 1)\n", progname, acIn, pDERet->d_name, pstBackup->st_nlink);
	}
	free((char *) pLIList);
	return pDERet;
}
