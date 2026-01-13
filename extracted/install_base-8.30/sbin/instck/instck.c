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

/* These routines check the installation of binary (mostly) files	(ksb)
 * we look for glob expressions to match in given directories, and
 * at what modes the matched files have.  If they look OK we say
 * nothing, else we bitch about the errors.
 */
#if !defined(lint)
static char rcsid[] =
	"$Id: instck.c,v 8.32 2009/12/10 18:53:58 ksb Exp $";
#endif	/* !lint */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <syslog.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>

#include "machine.h"
#include "configure.h"
#include "install.h"
#include "main.h"
#include "syscalls.h"
#include "special.h"
#include "instck.h"
#include "magic.h"
#include "gen.h"
#include "path.h"

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

/* Needs DIR typedef to be defined first
 */
#include "old.h"

#if STRINGS
#include <strings.h>
#else
#include <string.h>
#endif



/* remove all the dirs/files that point to the the same file/dir	(ksb)
 * in a choice between two, take the full path name, or the shorter one.
 * return the new argc
 */
int
ElimDups(argc, argv)
int argc;
char **argv;
{
	auto int i, j, r;
	auto struct stat *pSTScan;

	pSTScan = (struct stat *)calloc((unsigned)argc+1, sizeof(struct stat));
	if ((struct stat *)0 == pSTScan) {
		(void)fprintf(stderr, "%s: calloc: out of memory\n", progname);
		exit(EX_OSERR);
	}

	for (i = 0; i < argc; ++i) {
		if (-1 == STAT_CALL(argv[i], & pSTScan[i])) {
			(void)fprintf(stderr, "%s: stat: %s: %s\n", progname, argv[i], strerror(errno));
			argv[i] = (char *)0;
			continue;
		}
		for (j = 0; j < i; ++j) {
			if ((char *)0 == argv[j])
				continue;
			if (pSTScan[j].st_ino != pSTScan[i].st_ino)
				continue;
			if (pSTScan[j].st_dev != pSTScan[i].st_dev)
				continue;
			if (S_IFDIR != (pSTScan[i].st_mode & S_IFMT))
				continue;
			if (strlen(argv[j]) < strlen(argv[i])) {
				r = i;
				if ('/' != argv[j][0] && '/' == argv[i][0])
					r = j;
			} else {
				r = j;
				if ('/' != argv[i][0] && '/' == argv[j][0])
					r =i;
			}
			if (fVerbose)
				(void)fprintf(stderr, "%s: `%s\': dup of `%s\' (not scanned)\n", progname, argv[r], argv[r == i ? j : i]);
			argv[r] = (char *)0;
		}
	}
	free((char *)pSTScan);
	for (j = 0, i = 0; i < argc; ++i) {
		if ((char *)0 == argv[i])
			continue;
		argv[j++] = argv[i];
	}

	return j;
}

/* Called when the glob match code didn't find any new files		(ksb)
 */
void
NoMatches(pcGlob)
char *pcGlob;
{
	if (fVerbose) {
		(void)fprintf(fpOut, "%s: no matches for `%s\'\n", progname, pcGlob);
	}
}

/* Used in the path module to init the user data field			(ksb)
 */
void
PUInit(pPD)
PATH_DATA *pPD;
{
	pPD->fseen = 0;
}


/* Find matching files, look in the real file system			(ksb)
 */
int
FileMatch(pcGlob, pcDir, pfiCall)
char *pcGlob, *pcDir;
int (*pfiCall)(/* char * */);
{
	register DIR *pDI;
	register struct DIRENT *pDE;
	register char *pcTail;
	register char *pcSlash;
	register int iSave;
	auto char *pcStarStar;
	auto int iRet;
	auto char acDir[MAXPATHLEN+1];
	auto struct stat stDir, stEnt;

	if ((DIR *)0 == (pDI = opendir(pcDir))) {
		return 0;
	}
	pcSlash = strchr(pcGlob, '/');
	if ((char *)0 != pcSlash) {
		*pcSlash++ = '\000';
	}
	(void)strcpy(acDir, pcDir);
	pcTail = strchr(acDir, '\000');
	if (pcTail[-1] != '/') {
		*pcTail++ = '/';
	}

	/* look for ** here -- ksb & wam
	 */
	for (pcStarStar = pcGlob; '\000' != *pcStarStar; ++pcStarStar) {
		if ('*' == pcStarStar[0] && '*' == pcStarStar[1])
			break;
	}
	if ('\000' == pcStarStar[0]) {
		pcStarStar = (char *)0;
	}

	iRet = 0;
	if ('\000' == *pcGlob) {
		if (-1 != STAT_CALL(acDir, & stDir)) {
			iRet = (*pfiCall)(acDir, &stDir);
		}
	} else if ((char *)0 == pcSlash || '\000' == pcSlash[0]) {
		while (0 == iRet && (struct DIRENT *)0 != (pDE = readdir(pDI))) {
			if ('.' == pDE->d_name[0] && ('\000' == pDE->d_name[1] ||
			   ('.' == pDE->d_name[1] && '\000' == pDE->d_name[2])))
				continue;
			(void)strcpy(pcTail, pDE->d_name);
			if (-1 == STAT_CALL(acDir, &stEnt)) {
				continue;
			}
			if (S_IFDIR == (stEnt.st_mode & S_IFMT) && (char *)0 != pcStarStar) {
				pcStarStar[1] = '\000';
				iSave = _SamePath(pcGlob, pDE->d_name, 1);
				pcStarStar[1] = '*';
				if ((char *)0 != pcSlash) {
					pcSlash[-1] = '/';
				}
				if (iSave)
					FileMatch(pcStarStar, acDir, pfiCall);
				if ((char *)0 != pcSlash) {
					pcSlash[-1] = '\000';
				}
			}
			/* "p/" matches dirs only */
			if ((char *)0 != pcSlash) {
				if (S_IFDIR != (stEnt.st_mode & S_IFMT))
					continue;
				(void)strcat(pcTail, "/");
			}
			if (! _SamePath(pcGlob, pDE->d_name, 1)) {
				continue;
			}
			if ((char *)0 != pcSlash) {
				pcSlash[-1] = '/';
			}
			iRet = (*pfiCall)(acDir, &stEnt);
			if ((char *)0 != pcSlash) {
				pcSlash[-1] = '\000';
			}
		}
	} else {
		while (0 == iRet && (struct DIRENT *)0 != (pDE = readdir(pDI))) {
			if ('.' == pDE->d_name[0] && ('\000' == pDE->d_name[1] ||
			   ('.' == pDE->d_name[1] && '\000' == pDE->d_name[2])))
				continue;
			(void)strcpy(pcTail, pDE->d_name);
			(void)strcat(pcTail, "/");
			if ((char *)0 != pcStarStar) {
				pcStarStar[1] = '\000';
				iSave = _SamePath(pcGlob, pDE->d_name, 1);
				pcStarStar[1] = '*';
				if (iSave) {
					if ((char *)0 != pcSlash) {
						pcSlash[-1] = '/';
					}
					iRet = FileMatch(pcStarStar, acDir, pfiCall);
					if ((char *)0 != pcSlash) {
						pcSlash[-1] = '\000';
					}
				}
			}

			if (! _SamePath(pcGlob, pDE->d_name, 1))
				continue;
			if ((char *)0 != pcSlash) {
				pcSlash[-1] = '/';
			}
			iRet = FileMatch(pcSlash, acDir, pfiCall);
			if ((char *)0 != pcSlash) {
				pcSlash[-1] = '\000';
			}
		}
	}
	closedir(pDI);
	if ((char *)0 != pcSlash) {
		*--pcSlash = '/';
	}
	return iRet;
}

#if BROKEN_CHMOD
static char aaacAdd[8][8][4] =
/* add 0   01	02    03   04    05    06     07      have	*/
  { { "", "x", "w", "wx", "r", "rx", "rw", "rwx"}, /* 00	*/
    { "",  "", "w",  "w", "r",  "r", "rw",  "rw"}, /* 01	*/
    { "", "x",  "",  "x", "r", "rx",  "r",  "rx"}, /* 02	*/
    { "",  "",  "",   "", "r",  "r",  "r",   "r"}, /* 03	*/
    { "", "x", "w", "wx",  "",  "x",  "w",  "wx"}, /* 04	*/
    { "",  "", "w",  "w",  "",   "",  "w",   "w"}, /* 05	*/
    { "", "x",  "",  "x",  "",  "x",   "",   "x"}, /* 06	*/
    { "",  "",  "",   "",  "",   "",   "",    ""}};/* 07	*/

/* Sun's chmod(1) won't take litteral 02755 to change setgid bits	(ksb)
 * so we have to convert such things into symbolic +- forms (yucko)
 *
 * If we exchange the subscripts of the Add matrix we get a subtraction
 * matrix.  See assignments to pcS{Uid,Gid,All} below vs pcA{...}.
 */
void
SunBotch(pcDo, mFrom, mTo)
char *pcDo;
int mFrom, mTo;
{
	auto char cUid, cGid, cSticky, cSep;
	auto char *pcAUid, *pcAGid, *pcAAll;
	auto char *pcSUid, *pcSGid, *pcSAll;

	cUid = (S_ISUID&mFrom) != (S_ISUID&mTo) ?
		((S_ISUID&mTo) ? '+' : '-') : '\000';
	pcAUid = aaacAdd[(mFrom&0700)>>6][(mTo&0700)>>6];
	pcSUid = aaacAdd[(mTo&0700)>>6][(mFrom&0700)>>6];
	cGid = (S_ISGID&mFrom) != (S_ISGID&mTo) ?
		((S_ISGID&mTo) ? '+' : '-') : '\000';
	pcAGid = aaacAdd[(mFrom&0070)>>3][(mTo&0070)>>3];
	pcSGid = aaacAdd[(mTo&0070)>>3][(mFrom&0070)>>3];
	cSticky = (S_ISVTX&mFrom) != (S_ISVTX&mTo) ?
		((S_ISVTX&mTo) ? '+' : '-') : '\000';
	pcAAll = aaacAdd[(mFrom&0007)>>0][(mTo&0007)>>0];
	pcSAll = aaacAdd[(mTo&0007)>>0][(mFrom&0007)>>0];
	if ('\000' != cUid) {
		*pcDo++ = 'u';
		if (cUid == cGid) {
			cGid = '\000';
			*pcDo++ = 'g';
		}
		*pcDo++ = cUid;
		*pcDo++ = 's';
		if (cUid == cSticky) {
			*pcDo++ = 't';
			cSticky = '\000';
		}
		cSep = ',';
	} else {
		cSep = '\000';
	}
	if ('\000' != cGid) {
		if ('\000' != cSep)
			*pcDo++ = cSep;
		*pcDo++ = 'g';
		*pcDo++ = cGid;
		*pcDo++ = 's';
		cSep = ',';
	}
	if ('\000' != cSticky) {
		if ('\000' != cSep)
			*pcDo++ = cSep;
		*pcDo++ = cSticky;
		*pcDo++ = 't';
		cSep = ',';
	}
	if ('\000' != pcAUid[0]) {
		if ('\000' != cSep)
			*pcDo++ = cSep;
		*pcDo++ = 'u';
		if (0 == strcmp(pcAGid, pcAUid)) {
			*pcDo++ = 'g';
			pcAGid = "";
		}
		if (0 == strcmp(pcAAll, pcAUid)) {
			*pcDo++ = 'o';
			pcAAll = "";
		}
		*pcDo++ = '+';
		(void)strcpy(pcDo, pcAUid);
		pcDo += strlen(pcDo);
		cSep = ',';
	}
	if ('\000' != pcSUid[0]) {
		if ('\000' != cSep)
			*pcDo++ = cSep;
		*pcDo++ = 'u';
		if (0 == strcmp(pcSGid, pcSUid)) {
			*pcDo++ = 'g';
			pcSGid = "";
		}
		if (0 == strcmp(pcSAll, pcSUid)) {
			*pcDo++ = 'o';
			pcSAll = "";
		}
		*pcDo++ = '-';
		(void)strcpy(pcDo, pcSUid);
		pcDo += strlen(pcDo);
		cSep = ',';
	}
	if ('\000' != pcAGid[0]) {
		if ('\000' != cSep)
			*pcDo++ = cSep;
		*pcDo++ = 'g';
		if (0 == strcmp(pcAAll, pcAGid)) {
			*pcDo++ = 'o';
			pcAAll = "";
		}
		*pcDo++ = '+';
		(void)strcpy(pcDo, pcAGid);
		pcDo += strlen(pcDo);
		cSep = ',';
	}
	if ('\000' != pcSGid[0]) {
		if ('\000' != cSep)
			*pcDo++ = cSep;
		*pcDo++ = 'g';
		if (0 == strcmp(pcSAll, pcSGid)) {
			*pcDo++ = 'o';
			pcSAll = "";
		}
		*pcDo++ = '-';
		(void)strcpy(pcDo, pcSGid);
		pcDo += strlen(pcDo);
		cSep = ',';
	}
	if ('\000' != pcAAll[0]) {
		if ('\000' != cSep)
			*pcDo++ = cSep;
		*pcDo++ = 'o';
		*pcDo++ = '+';
		(void)strcpy(pcDo, pcAAll);
		pcDo += strlen(pcDo);
		cSep = ',';
	}
	if ('\000' != pcSAll[0]) {
		if ('\000' != cSep)
			*pcDo++ = cSep;
		*pcDo++ = 'o';
		*pcDo++ = '-';
		(void)strcpy(pcDo, pcSAll);
		pcDo += strlen(pcDo);
		cSep = ',';
	}
	*pcDo = '\000';
}
#endif /* brain dead chmod(2) */

/* run a shell command and collect the result code, syslog it too	(ksb)
 */
static int
QRunCmd(pcCmd, ppcArgv, ppcEnv)
char *pcCmd, **ppcArgv, **ppcEnv;
{
	auto int iPid, iWPid;
#if HAVE_UWAIT
	auto union wait	wStatus;
#else	/* !BSD */
	auto int wStatus;
#endif	/* set up for a wait */

	(void)fflush(stdout);
	(void)fflush(stderr);
	switch ((iPid = fork())) {
	case 0:		/* child					*/
		execve(pcCmd, ppcArgv, ppcEnv);
		(void)fprintf(stderr, "%s: execve: `%s\': %s\n", progname, pcCmd, strerror(errno));
		exit(EX_UNAVAILABLE);
	case -1:	/* error					*/
		(void)fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	default:	/* parent					*/
		break;
	}


	while ((iWPid = wait(&wStatus)) != iPid) {
		if (iWPid == -1)
			break;
	}

	if (iWPid == -1) {
		/* this happens on the ETA, where did the kid go?
		 */
		if (ECHILD == errno)
			return 0;
		(void)fprintf(stderr, "%s: wait: %s\n", progname, strerror(errno));
		return 1;
	}

#if HAVE_UWAIT
	return wStatus.w_status;
#else	/* BSD */
	return wStatus;
#endif	/* return value from wait */
}

#if !defined(nil)
#define nil	((char *)0)
#endif
static char
	*apcChgrp[] =	{ BIN_chgrp,	"chgrp", nil, nil, nil, nil },
	*apcChown[] =	{ BIN_chown,	"chown", nil, nil, nil, nil },
	*apcStrip[] =	{ BIN_strip,	"strip", nil, nil, nil, nil },
	*apcRanlib[] =	{ BIN_ranlib,	"ranlib", nil, nil, nil, nil };
char
	*apcInstall[] =	{ BIN_install,	"install", nil, nil, nil, nil },
	*apcChflags[] = { BIN_chflags,	"chflags", nil, nil, nil, nil },
	*apcChmod[] =	{ BIN_chmod,	"chmod", nil, nil, nil, nil },
	*apcCmp[] =	{ BIN_cmp,	"cmp", nil, nil, nil, nil },
	*apcRm[] =	{ BIN_rm,	"rm", nil, nil, nil, nil, nil },
	*apcRmdir[] =	{ BIN_rmdir,	"rmdir", nil, nil, nil, nil, nil },
	*apcLn[] =	{ BIN_ln,	"ln", nil, nil, nil, nil },
	*apcMv[] =	{ BIN_mv,	"mv", nil, nil, nil, nil };

/* qwery the user and exec the `fix' program, if he wants to		(ksb)
 *
 * We tell RunCmd to be quiet, because we echo the command when
 * we prompt for run/not.  Return QE_SKIP for `skip the rest of this file'.
 */
int
QExec(ppcArgv, pcWas, piStatus)
char **ppcArgv, *pcWas;
int *piStatus;
{
	register char **ppc;
	register char *pcCmd, *pcFinger;
	auto char *pcTemp;
	auto char acAns[MAXANS+1];
	extern char **environ;
	static char *apcQHelp[] = {
		"f\tskip to the next file",
		"h\tprint this help message",
		"n\tdo not run this command",
		"y\trun this command",
		"q\tquit the program",
		(char *)0
	};

	if ((char *)0 == (pcFinger = pcGuilty)) {
		pcFinger = "<unknown>";
	}
	pcCmd = *ppcArgv++;
	for (;;) {
		(void)fprintf(fpOut, "%s:", progname);
		for (ppc = ppcArgv; (char *)0 != *ppc; ++ppc) {
			fprintf(fpOut, " %s", *ppc);
		}
		if ((char *)0 != pcWas)
			(void)fprintf(fpOut, " {was %s}", pcWas);

		if (fYesYes) {
			fputs(" [y] yes\n", fpOut);
			break;
		}
		fputs(" [nfhqy] ", fpOut);

		if ((char *)0 == fgets(acAns, MAXANS, stdin)) {
			(void)clearerr(stdin);
			fputs("[EOF]\n", fpOut);
			return QE_SKIP;
		}
		pcTemp = acAns;
		while (isspace(*pcTemp) && '\n' != *pcTemp)
			++pcTemp;
		if (!isatty(fileno(stdin))) {
			fputs(pcTemp, fpOut);
		}
		switch (*pcTemp) {
		case '-':	/* undocumented (set -y) option		*/
			if ('y' == pcTemp[1]) {
				fYesYes = 1;
				continue;
			}
		default:
			(void)fprintf(stderr, "%s: unknown execute key `%c\'\n", progname, *pcTemp);
		case 'h':
		case 'H':
		case '?':
			for (ppc = apcQHelp; (char *)0 != *ppc; ++ppc) {
				(void)fprintf(fpOut, "%s\n", *ppc);
			}
			continue;
		case 'y':	/* yeah, run it	*/
		case 'Y':
			break;

		case 'q':	/* quit this damn program		*/
		case 'Q':
			exit(EX_OK);

		case '\n':
		case 'n':	/* no thanks, do I have other options?	*/
		case 'N':
			return QE_NEXT;

		case 'f':	/* next file, I'm lost			*/
		case 'F':
			return QE_SKIP;
		}
		break;
	}
	if ((int *)0 != piStatus)
		*piStatus = QRunCmd(pcCmd, ppcArgv, environ);
	else
		(void)QRunCmd(pcCmd, ppcArgv, environ);
#if defined(INST_FACILITY)
	/* must have argv[1] "rm file" is shortest case
	 */
	if (bHaveRoot) {
		static char *pcLogMem;
		static int iLogMem = 0;
		register int i, iTotal;
		register char *pc;

		iTotal = 0;
		for (i = 0; (char *)0 != ppcArgv[i]; ++i) {
			iTotal += 1 + strlen(ppcArgv[i]);
		}
		if ((char *)0 != pcWas) {
			iTotal += 7 + strlen(pcWas); /* "{was "+was+ "} " */
		}
		iTotal += 3 + strlen(pcFinger);	/* "by " + guilty */
		iTotal |= 7;
		++iTotal;
		if (0 == iLogMem || (char *)0 == pcLogMem) {
			pcLogMem = calloc(iTotal, sizeof(char));
			iLogMem = iTotal;
		} else if (iTotal <= iLogMem) {
			;
		} else {
			pcLogMem = realloc(pcLogMem, iTotal);
			iLogMem = iTotal;
		}
		if ((char *)0 == pcLogMem) {
			syslog(LOG_INFO, "too long %s by %s", ppcArgv[0], pcFinger);
			return QE_RAN;
		}

		pc = pcLogMem;
		for (i = 0; (char *)0 != ppcArgv[i]; ++i) {
			(void)strcpy(pc, ppcArgv[i]);
			pc += strlen(pc);
			*pc++ = ' ';
			*pc = '\000';
		}

		if ((char *)0 != pcWas) {
			(void)strcat(pc, "{was ");
			(void)strcat(pc, pcWas);
			(void)strcat(pc, "} ");
			pc += strlen(pc);
		}
		(void)strcpy(pc, "by ");
		(void)strcat(pc, pcFinger);
		syslog(LOG_INFO, "%s", pcLogMem);
	}
#endif
	return QE_RAN;
}

/* Move a file into the OLD directory, either with install or mv.	(ksb)
 * When In is set use the OLD directory under that, rather than the
 * local one.
 */
static int
IntoOld(pcIn, pcFile)
char *pcIn, *pcFile;
{
	register char *pcSlash;
	auto int iBuf;
	auto struct stat stOld;
	auto char acTargetDir[MAXPATHLEN+4];

	if ((char *)0 == pcIn) {
		if ((char *)0 == (pcSlash = strrchr(pcFile, '/'))) {
			(void)strncpy(acTargetDir, acOLD, sizeof(acTargetDir));
		} else if (sizeof(acTargetDir)-(strlen(acOLD)+2) < (iBuf = pcSlash - pcFile)) {
			fprintf(stderr, "%s: %s: %s\n", progname, pcFile, strerror(ENAMETOOLONG));
			exit(EX_OSERR);
		} else {
			snprintf(acTargetDir, sizeof(acTargetDir), "%.*s/%s", iBuf, pcFile, acOLD);
		}
		pcIn = acTargetDir;
	}
	if (-1 == stat(pcIn, & stOld)) {
		if (ENOENT != errno) {
			fprintf(stderr, "%s: stat: %s: %s\n", progname, pcIn, strerror(errno));
			return 0;
		}
		if (!fInteract) {
			return 0;
		}
		apcInstall[2] = "-dv";
		apcInstall[3] = pcIn;
		apcInstall[4] = (char *)0;
		if (QE_RAN != QExec(apcInstall, (char *)0, & iBuf) || 0 != iBuf) {
			return 0;
		}
	} else if (S_IFDIR != (stOld.st_mode & S_IFMT)) {
		fprintf(stderr, "%s: %s: should be a directory!\n", progname, pcIn);
		return 0;
	}
	/* de install the file
	 */
	apcInstall[2] = "-RvC/dev/null";
	apcInstall[3] = pcFile;
	apcInstall[4] = (char *)0;
	if (QE_RAN != QExec(apcInstall, (char *)0, & iBuf) || 0 != iBuf) {
		return 0;
	}
	return 1;
}

CHLIST CLCheck;
static COMPONENT *pCMRoot;
int iMatches;

/* We are passed a filename that exists and matches the recorded	(ksb)
 * install.cf line our parrent is currently working on.  We should
 * look in the list of examined files to see if we have already
 * matched this file, if so we can ignore this match.  Else check to
 * see if it would pass the current line.
 * Report bugs to stdout as install(1l) would, or close.
 */
int
DoCk(pcFile, pstThis)
char *pcFile;
struct stat *pstThis;
{
	register PATH_DATA *pPD;
	auto int fdFile;
	auto int fChGroup, fChOwner, fChMode, fOther;
	auto char acOOwner[128], acOGroup[128], acOMode[128];
	auto char acNMode[128];
	auto int mMode, mNew;

	pPD = AddPath(& pCMRoot, pcFile);
	if (0 != pPD->fseen) {
		return 0;
	}
	pPD->fseen = 1;
	++iMatches;

	/* The "don't look" flag for /proc and other non-fs's (petef, ksb)
	 */
	if ('.' == CLCheck.acmode[0]) {
		return 0;
	}

	/* should be a symbolic link?
	 */
	if ((char *)0 != CLCheck.pclink && '@' == CLCheck.pclink[0]) {
#if HAVE_SYMLINKS
		register char *pcSlash, *pcTail;
		auto char acSym[MAXPATHLEN+2];
		auto int iLen, fSameDir;
		auto struct stat stTarget;

		/* is the link in the dir with the contents?
		 */
		if ((char *)0 == (pcTail = strrchr(CLCheck.pclink+1, '/'))) {
			fSameDir = 1;
			pcTail = CLCheck.pclink+1;
		} else if ((char *)0 == (pcSlash = strrchr(pcFile, '/'))) {
			fSameDir = 0; /* can't tell, really */
		} else {
			*pcTail = *pcSlash = '\000';
			fSameDir = 0 == strcmp(pcFile, CLCheck.pclink+1);
			*pcTail++ = *pcSlash = '/';
		}

		/* is the file there really a link?
		 */
		if (S_IFLNK != (pstThis->st_mode & S_IFMT)) {
			if (-1 == stat(CLCheck.pclink+1, & stTarget) || pstThis->st_dev != stTarget.st_dev || pstThis->st_ino != stTarget.st_ino) {
				fprintf(fpOut, "%s: `%s\' should be a symbolic link to `%s\'\n", progname, pcFile, CLCheck.pclink+1);
			} else {
				fprintf(fpOut, "%s: `%s\' is a hard link to `%s\', should be symbolic\n", progname, pcFile, CLCheck.pclink+1);
			}
		} else if (-1 == (iLen = readlink(pcFile, acSym, sizeof(acSym)))) {
			fprintf(stderr, "%s: readlink: %s: %s\n", progname, pcFile, strerror(errno));
			return 0;
		} else if ((acSym[iLen] = 0), 0 == strcmp(CLCheck.pclink+1, acSym)) {
			/* link is exactly correct, rare */
			return 0;
		} else if (fSameDir && (0 == strcmp(pcTail, acSym) ||
			('.' == acSym[0] && '/' == acSym[1] && 0 == strcmp(pcTail, acSym+2)))) {
			/* link (in same dir) is close enough for me	(ksb)
			 *   link@ -> file
			 *   link@ -> ./file
			 */
			return 0;
		} else if ('.' == CLCheck.pclink[1] && '/' == CLCheck.pclink[2] && 0 == strcmp(CLCheck.pclink+3, acSym)) {
			/* link is close enough, ask for ./foo got foo
			 */
			return 0;
		} else {
			fprintf(fpOut, "%s: `%s\' should be a symlink to `%s\' (not %s)\n", progname, pcFile, CLCheck.pclink+1, acSym);
		}
		if (!fInteract) {
			return 0;
		}

		/* About to change this to symlink to target,
		 * maybe we should see if we can see the target? Nope.
		 * [The target might be on visible from an NFS view.]
		 */
		if (0 == IntoOld((char *)0, pcFile)) {
			return 0;
		}
		apcLn[2] = "-s";
		apcLn[3] = CLCheck.pclink+1;
		apcLn[4] = pcFile;
		apcLn[5] = (char *)0;
		++fYesYes;
		(void)QExec(apcLn, (char *)0, & iLen);
		--fYesYes;
		return iLen;
#else
		fprintf(fpOut, "%s: `%s\' no symbolic links -- using hard to `%s\'\n", progname, pcFile, CLCheck.pclink+1);
		CLCheck.pclink[0] = ':';
#endif
	}
	/* should be a link to a certian file in the fs
	 */
	if ((char *)0 != CLCheck.pclink && ':' == CLCheck.pclink[0]) {
		auto struct stat stLink;
		register char *pcType;

		/* see if what we are a link to has the same ino/dev
		 */
		if (-1 == STAT_CALL(CLCheck.pclink+1, & stLink)) {
			if (ENOENT != errno) {
				fprintf(stderr, "%s: stat: %s: %s\n", progname, CLCheck.pclink+1, strerror(errno));
				return 0;
			}
			fprintf(fpOut, "%s: `%s\' should be a link to (currently nonexistent) `%s\'\n", progname, pcFile, CLCheck.pclink+1);
			return 0;
		}
		if (stLink.st_ino == pstThis->st_ino && stLink.st_dev == pstThis->st_dev) {
			/* a miracle occurs, they are the same file :-)
			 */
			return 0;
		}
		/* Ught-O -- they both exist and are different;
		 * but maybe they are really the same in some sense.
		 * things to try:
		 *	same contents (exactly)
		 *	one says `run the other'		(hard!)
		 *	one is stripped version of the other	(ha!)
		 *	one has the wholes filled in (dbm like)	(maybe)
		 * links to device files are noted
		 */
		pcType = NodeType(pstThis->st_mode, (char *)0);
		fprintf(fpOut, "%s: `%s\' is a %s %s.  It should be a hard link to `%s\'\n", progname, pcFile, (1 == pstThis->st_nlink ? "unique" : "stray link to a"), pcType, CLCheck.pclink+1);
		if (!fInteract) {
			return 0;
		}
		/* move the link into OLD, link to the new file
		 */
		if (0 == IntoOld((char *)0, pcFile)) {
			return 0;
		}
		apcLn[2] = CLCheck.pclink+1;
		apcLn[3] = pcFile;
		apcLn[4] = (char *)0;
		(void)QExec(apcLn, (char *)0, (int *)0);
		return 0;
	}
	if ((char *)0 != CLCheck.pclink) {
		fprintf(stderr, "%s: DoCk: internal error for file %s, checklist link field is bogus (%s)\n", progname, pcFile, CLCheck.pclink);
		return 0;
	}
#if HAVE_SYMLINKS
	/* if we are not looking at links as symbolic links look through at
	 * this point.
	 */
	if (!fLinks && S_IFLNK == (pstThis->st_mode & S_IFMT)) {
		(void)stat(pcFile, pstThis);
	}
#endif


	if ('~' == CLCheck.acmode[0]) {
		(void)fprintf(fpOut, "%s: `%s\' should not be installed\n", progname, pcFile);
	} else if ('!' == CLCheck.acmode[0]) {
		register char **ppcRun, *pcWarn;
		register DIR *pDI;

		(void)fprintf(fpOut, "%s: `%s\' should not exist\n", progname, pcFile);
		if (!fInteract) {
			return 0;
		}
		pcWarn = (char *)0 != CLCheck.pcmesg && '\000' != CLCheck.pcmesg[0] ? CLCheck.pcmesg : (char *)0;
		if (S_IFDIR != (pstThis->st_mode & S_IFMT)) {
			apcRm[2] = "-f";
			apcRm[3] = pcFile;
			apcRm[4] = (char *)0;
			ppcRun = apcRm;
			if ((char *)0 == pcWarn) {
				pcWarn = NodeType(pstThis->st_mode, (char *)0);
			}
		} else if ((DIR *)0 != (pDI = opendir(pcFile))) {
			if (IsEmpty(pDI)) {
				apcRmdir[2] = pcFile;
				apcRmdir[3] = (char *)0;
				ppcRun = apcRmdir;
				if ((char *)0 == pcWarn) {
					pcWarn = "an empty directory";
				}
			} else {
				apcRm[2] = "-ri";
				apcRm[3] = pcFile;
				apcRm[4] = (char *)0;
				ppcRun = apcRm;
				if ((char *)0 == pcWarn) {
					pcWarn = "a non-empty directory";
				}
			}
			closedir(pDI);
		} else {
			/* rmdir always fails here, I think -- ksb */
			apcRmdir[2] = pcFile;
			apcRmdir[3] = (char *)0;
			ppcRun = apcRmdir;
			if ((char *)0 == pcWarn) {
				pcWarn = "an unreadable directory";
			}
		}
		if (QE_SKIP == QExec(ppcRun, pcWarn, (int *)0)) {
			return 0;
		}
		/* If the user didn't remove it, fall-through to check it.
		 */
		if (-1 == access(pcFile, 0)) {
			return 0;
		}
	}
	if ('*' == CLCheck.acmode[0]) {
		/* nothing */;
	} else if (CLCheck.mtype != (pstThis->st_mode & S_IFMT)) {
		(void)fprintf(fpOut, "%s: `%s\' should be a %s, not a %s\n", progname, pcFile, NodeType(CLCheck.mtype, (char *)0), NodeType(pstThis->st_mode, (char *)0));
		return 0;
	}

	/* Time to check the group, owner, mode against our check list...
	 * We buffer all the info up for one big informative printf at the end
	 */
	fChOwner = fChGroup = fChMode = FALSE;

	if ('*' == CLCheck.acowner[0] || (CLCheck.fbangowner ? (fChOwner = pstThis->st_uid == CLCheck.uid) : (fChOwner = pstThis->st_uid != CLCheck.uid))) {
		register struct passwd *pwdTemp;

		pwdTemp = getpwuid(pstThis->st_uid);
		if ((struct passwd *)0 != pwdTemp) {
			(void)strcpy(acOOwner, pwdTemp->pw_name);
		} else {
			(void)sprintf(acOOwner, "%d", pstThis->st_uid);
		}
	} else {
		(void)strcpy(acOOwner, CLCheck.acowner);
	}

	if ('*' == CLCheck.acgroup[0] || (CLCheck.fbanggroup ? (fChGroup = pstThis->st_gid == CLCheck.gid) : (fChGroup = pstThis->st_gid != CLCheck.gid))) {
		register struct group *grpTemp;

		grpTemp = getgrgid(pstThis->st_gid);
		if ((struct group *)0 != grpTemp) {
			(void)strcpy(acOGroup, grpTemp->gr_name);
		} else {
			(void)sprintf(acOGroup, "%d", pstThis->st_gid);
		}
	} else {
		(void)strcpy(acOGroup, CLCheck.acgroup);
	}

	mNew = mMode = PERM_BITS(pstThis->st_mode);
	(void)sprintf(acOMode, "%04o", mMode);
	if ('*' != CLCheck.acmode[0]) {
		if (CLCheck.mmust != (mMode & CLCheck.mmust)) {
			fChMode = 1;
		} else if (0 != (mMode &~ (CLCheck.mmust|CLCheck.moptional))) {
			fChMode = 1;
		}
		mNew = CLCheck.mmust | (mMode & CLCheck.moptional);
		if (0 != (S_ISUID & mMode) ? 0 == (S_ISUID & (CLCheck.mmust|CLCheck.moptional)) : 0 != (S_ISUID & CLCheck.mmust)) {
			if (fVerbose) {
				(void)fprintf(fpOut, "%s: `%s\' setuid bit must be %s\n", progname, pcFile, apcOO[0 == (S_ISUID & mMode)]);
			}
			mNew |= (S_ISUID & (CLCheck.mmust|CLCheck.moptional));
			fChMode = 1;
		}
		if (0 != (S_ISGID & mMode) ? 0 == (S_ISGID & (CLCheck.mmust|CLCheck.moptional)) : 0 != (S_ISGID & CLCheck.mmust)) {
			if (fVerbose) {
				(void)fprintf(fpOut, "%s: `%s\' setgid bit must be %s\n", progname, pcFile, apcOO[0 == (S_ISGID & mMode)]);
			}
			mNew |= (S_ISGID & (CLCheck.mmust|CLCheck.moptional));
			fChMode = 1;
		}
		if (0 != (S_ISVTX & mMode) ? 0 == (S_ISVTX & (CLCheck.mmust|CLCheck.moptional)) : 0 != (S_ISVTX & CLCheck.mmust)) {
			if (fVerbose) {
				(void)fprintf(fpOut, "%s: `%s\' sticky bit must be %s\n", progname, pcFile, apcOO[0 == (S_ISVTX & mMode)]);
			}
			mNew |= (S_ISVTX & (CLCheck.mmust|CLCheck.moptional));
			fChMode = 1;
		}
#if BROKEN_CHMOD
		if (fChMode) {
			SunBotch(acNMode, mMode, mNew);
		} else {
			(void)sprintf(acNMode, "%04o", mNew);
		}
#else
		(void)sprintf(acNMode, "%04o", mNew);
#endif
	} else {
		(void)strcpy(acNMode, CLCheck.acmode);
	}

	/* tell the user of their sins, ask if they want to repent
	 */
	if (fChOwner || fChGroup || fChMode) {
		fprintf(fpOut, "%s: `%s\' is %s.%s(%s) should be %s%s.%s%s(%s)\n", progname, pcFile, acOOwner, acOGroup, acOMode, CLCheck.fbangowner ? "!" : "", CLCheck.acowner, CLCheck.fbanggroup ? "!" : "", CLCheck.acgroup, acNMode);
	}
	if (fInteract) {
		if (fChOwner && !CLCheck.fbangowner) {
			apcChown[2] = CLCheck.acowner;
			apcChown[3] = pcFile;
			apcChown[4] = (char *)0;
			if (QE_SKIP == QExec(apcChown, acOOwner, (int *)0))
				return 0;
		}
		if (fChGroup && !CLCheck.fbanggroup) {
			apcChgrp[2] = CLCheck.acgroup;
			apcChgrp[3] = pcFile;
			apcChgrp[4] = (char *)0;
			if (QE_SKIP == QExec(apcChgrp, acOGroup, (int *)0))
				return 0;
		}
		/* we need to know if the user did change mode
		 */
		if (fChMode) {
			auto int iChmod;
			apcChmod[2] = acNMode;
			apcChmod[3] = pcFile;
			apcChmod[4] = (char *)0;
			if (QE_SKIP == QExec(apcChmod, acOMode, & iChmod))
				return 0;
			if (0 == iChmod)
				mMode = mNew;
		} else {
			/* he *would* have...
			 */
			mMode = mNew;
		}
	}

#if HAVE_CHFLAGS
	/* N.B. chflags bits are not supported on symbolic links here
	 */
	{
	auto char *pcRef;
	auto unsigned long uFFMustSet, uFFMustClr, uFFSet, uFFClr;

	if ((char *)0 == (pcRef = CLCheck.pcflags)) {
		pcRef = "";
	}
	if (0 != strtofflags(&pcRef, &uFFMustSet, & uFFMustClr)) {
		fprintf(stderr, "%s: %s:%d: %s: unknown flag\n", progname, CLCheck.pcspecial, CLCheck.iline, pcRef);
		uFFMustSet = uFFMustClr = 0;
	}
	uFFClr = ~uFFSet;
	if (FAIL == CheckFFlags(pcFile, uFFMustSet, uFFMustClr, pstThis->st_flags, uFFMustClr & ~pstThis->st_flags) && fInteract) {
		register char *pcSet, *pcClr, *pcComma, *pcNote;

		pcSet = fflagstostr(uFFMustSet & ~pstThis->st_flags);
		pcClr = fflagstostr(pstThis->st_flags & uFFMustClr);
		if ('\000' == pcSet[0])
			pcNote = "vexillologically over-specified";
		else
			pcNote = "missing flags";
		pcSet = realloc((void *)pcSet, (15|(strlen(pcSet)+strlen(pcClr)*2))+1);
		/* clr="b,c" -> set+=",nob,noc" so *2 is at least enough
		 * This will break if a positive flag starts with "no"
		 */
		while ('\000' != *pcClr) {
			if ((char *)0 != (pcComma = strchr(pcClr, ','))) {
				*pcComma++ = '\000';
			} else {
				pcComma = "";
			}
			if ('\000' != *pcSet) {
				strcat(pcSet, ",");
			}
			if ('n' == pcClr[0] && 'o' == pcClr[1]) {
				strcat(pcSet, pcClr+2);
			} else {
				strcat(pcSet, "no");
				strcat(pcSet, pcClr);
			}
			pcClr = pcComma;
		}
		apcChflags[2] = pcSet;
		apcChflags[3] = pcFile;
		apcChflags[4] = (char *)0;

		/* We always get an empty set if the file has a flag set and
		 * we don't forbid it, but we don't allow it.  We carped about
		 * that, but we don't really want to toggle the flag.
		 */
		if ('\000' != *pcSet && QE_SKIP == QExec(apcChflags, pcNote, (int *)0)) {
			return 0;
		}
	}
	}
#endif

	/* if we don't have to check strip/ranlib, don't open the file
	 * if opening the file might activate a device, avoid it.
	 */
	if ('*' == CLCheck.chstrip && 0 == ((S_ISUID|S_ISGID)&mMode)) {
		return 0;
	}
	if (S_IFREG != (pstThis->st_mode & S_IFMT)) {
		if (!CF_IS_NONE(CLCheck)) {
			(void)fprintf(stderr, "%s: %s: will not open %s\n", progname, pcFile, NodeType(pstThis->st_mode, (char *)0));
		}
		return 0;
	}

	if (0 > (fdFile = open(pcFile, 0, 0000))) {
		if (errno != EACCES) {
			(void)fprintf(stderr, "%s: open: %s: %s\n", progname, pcFile, strerror(errno));
		}
		if (fVerbose) {
			(void)fprintf(fpOut, "%s: %s: cannot check `%c' flag\n", progname, pcFile, CLCheck.chstrip);
		}
		return 0;
	}
	if (0 != ((S_ISUID|S_ISGID)&mMode)) {
		register char *pcBad;
		if ((char *)0 != (pcBad = BadLoad(fdFile, mMode))) {
			fprintf(stderr, "%s: `%s\' is %s and loaded with `#!...\'!\n", progname, pcFile, pcBad);
		}
	}
	switch (CLCheck.chstrip) {
	case CF_ANY:	/* if is set%cid we can get here */
		break;
	default:
		fprintf(stderr, "%s: bad strip flag %c\n", progname, CLCheck.chstrip);
		break;
	case CF_STRIP:
		switch (IsStripped(fdFile, pstThis)) {
		case -1:
			(void)fprintf(fpOut, "%s: `%s\' is not even a binary, should be stripped?\n", progname, pcFile);
			break;
		case 0:
			(void)fprintf(fpOut, "%s: `%s\' is not strip\'ed\n", progname, pcFile);
			if (!fInteract)
				break;
			apcStrip[2] = pcFile;
			apcStrip[4] = pcFile;
			if (QE_SKIP == QExec(apcStrip, "not stripped", (int *)0)) {
				(void)close(fdFile);
				return 0;
			}
			break;
		case 1:
			break;
		}
		break;
	case CF_RANLIB:
		if (1 == IsRanlibbed(fdFile, pstThis)) {
			break;
		}
		(void)fprintf(fpOut, "%s: `%s\' is not ranlib\'ed", progname, pcFile);
		if (-1 != (fOther = IsStripped(fdFile, pstThis))) {
			(void)fprintf(fpOut, ", it is actually a%s binary\n", fOther ? " strip\'ed" : "");
			break;
		} else if (!fInteract) {
			(void)fputc('\n', fpOut);
			break;
		}
		(void)fputc('\n', fpOut);
		apcRanlib[2] = pcFile;
		apcRanlib[3] = (char *)0;
		if (QE_SKIP == QExec(apcRanlib, "not ranlibbed", (int *)0)) {
			(void)close(fdFile);
			return 0;
		}
		break;
	case CF_NONE:
		switch (IsStripped(fdFile, pstThis)) {
		case -1:		/* not a binary		*/
			switch (IsRanlibbed(fdFile, pstThis)) {
			case -1:	/* not a lib.a		*/
			case 0:		/* no table of contents	*/
				break;
			case 1:
				(void)fprintf(fpOut, "%s: `%s\' is ranlib\'ed\n", progname, pcFile);
				break;
			}
			break;
		case 0:			/* plain binary		*/
			break;
		case 1:			/* stripped binary	*/
			(void)fprintf(fpOut, "%s: `%s\' is strip\'ed \n", progname, pcFile);
			break;
		}
		break;
	}
	(void)close(fdFile);

	return 0;
}

/* output a login.group string for the user to ponder			(ksb)
 */
static void
LoginFmt(pwd, grp, pstThis)
register struct passwd *pwd;
register struct group *grp;
register struct stat *pstThis;
{
	/* on HP-UX should be ':' rather than '.'
	 */
#if !defined(LOGIN_GROUP_SEP)
#define LOGIN_GROUP_SEP	"."
#endif
	if ((struct passwd *)0 != pwd) {
		fprintf(fpOut, "%s", pwd->pw_name);
	} else {
		fprintf(fpOut, "%d", pstThis->st_uid);
	}
	fputs(LOGIN_GROUP_SEP, fpOut);
	if ((struct group *)0 != grp) {
		fprintf(fpOut, "%s", grp->gr_name);
	} else {
		fprintf(fpOut, "%d", pstThis->st_gid);
	}
}


/* Checks for files that were not checked:				(ksb)
 *	wide open directory
 *	setuid, setgid, sticky (and not a link)
 *	world write or group write
 *	mode zero
 *	device (c, b) owned by other than the superuser or world r/w
 *	unmapped uid or gid (complain about reverse lookup, of course)
 * group write check should be more clever, or an option because it
 * generates noise on home dir filesystems.  Maybe just group write to
 * the default group is bad?  If there are <2 members in a group then
 * group write is a no-op, so we'll not complain about it.  People blessed
 * by login-only memebership don't need to be counted.
 */
int
LeftCk(pcFile, pstThis)
char *pcFile;
struct stat *pstThis;
{
	register PATH_DATA *pPD;
	register struct group *grpTemp;
	register struct passwd *pwdTemp;
	register char *pcType;
	register int iMap;	/* bits: unmapped: group 2 | user 1 */
	register int iSingle;	/* 0020 when group write might be an issue */
	auto char acMode[100];

	pPD = AddPath(& pCMRoot, pcFile);
	if (0 != pPD->fseen) {
		return 0;
	}
	pPD->fseen = 1;

	if (fQuiet) {
		return 0;
	}

	pwdTemp = getpwuid(pstThis->st_uid);
	grpTemp = getgrgid(pstThis->st_gid);
	pcType = NodeType(pstThis->st_mode, & acMode[0]);
	iSingle = (struct group *)0 == grpTemp ? 0020 : ((char *)0 != grpTemp->gr_mem[0] && (char *)0 != grpTemp->gr_mem[1]) ? 0020 : 0000;
	if ('d' == acMode[0] && 0777 == (pstThis->st_mode & 00777) && 0 == (pstThis->st_mode & 01000)) {
		fprintf(fpOut, "%s: `%s\' wide open directory without sticky set (owner ", progname, pcFile);
		LoginFmt(pwdTemp, grpTemp, pstThis);
		fprintf(fpOut, ")\n");
	} else if ('l' != acMode[0] && (0 != (pstThis->st_mode & (iSingle|('d' == acMode[0] ? 01002 : 07002))) || 0 == (pstThis->st_mode & 07777))) {
		ModetoStr(acMode+1, (int)PERM_BITS(pstThis->st_mode), 0000);
		fprintf(fpOut, "%s: `%s\' unchecked %s, mode %s (%04o", progname, pcFile, pcType, acMode, (int)PERM_BITS(pstThis->st_mode));
		if (0 != (pstThis->st_mode & 06000) || (0 != (pstThis->st_mode & 00220) && 0 == (pstThis->st_mode & 00002))) {
			fputs(", ", fpOut);
			LoginFmt(pwdTemp, grpTemp, pstThis);
		}
		fprintf(fpOut, ")\n");
#if HAVE_CHFLAGS
	} else if (0 != pstThis->st_flags) {
		register char *pcSet;

		if ((char *)0 != (pcSet = fflagstostr(pstThis->st_flags))) {
			fprintf(fpOut, "%s: `%s\' has flag%s \"%s\"\n", progname, pcFile, (char *)0 == strchr(pcSet, ',') ? "" : "s", pcSet);
			free((void *)pcSet);
		} else {
			fprintf(fpOut, "%s: `%s\' has unknown flags set (0x%08lX)\n", progname, pcFile, (unsigned long)pstThis->st_flags);
		}
#endif
	} else if (0 != (iMap = ((struct passwd *)0 == pwdTemp) + 2 * ((struct group *)0 == grpTemp))) {
		static char *apcUnMap[4] = {
			"NPCGuild", "owner", "group", "owner and group"
		};
		fprintf(fpOut, "%s: `%s\' unmapped %s ", progname, pcFile, apcUnMap[iMap]);
		LoginFmt(pwdTemp, grpTemp, pstThis);
		if ((0700 & pstThis->st_mode) != ((0070 & pstThis->st_mode) << 3) || (0700 & pstThis->st_mode) != ((0007 & pstThis->st_mode) << 6)) {
			fprintf(fpOut, " (mode %04o)", (int)PERM_BITS(pstThis->st_mode));
		}
		fprintf(fpOut, "\n");
	} else if (('c' == acMode[0] || 'b' == acMode[0]) && 0 != pstThis->st_uid) {
		fprintf(fpOut, "%s: `%s\' device file owned by %s (uid %d)\n", progname, pcFile, pwdTemp->pw_name, pstThis->st_uid);
	} else if (('c' == acMode[0] || 'b' == acMode[0]) && (0 != (pstThis->st_mode & 00006))) {
		fprintf(fpOut, "%s: `%s\' %s is world accessible (%04o)\n", progname, pcFile, pcType, (int)PERM_BITS(pstThis->st_mode));
	} else if (fLongList) {
		fprintf(fpOut, "%s: `%s\' unchecked %s\n", progname, pcFile, pcType);
	}
	return 0;
}

/* Partition dirs from files, dirCk the dirs, fileck file files		(ksb)
 */
void
InstCk(argc, argv, pcLSpecial, pCLCheck)
int argc;
char **argv, *pcLSpecial;
CHLIST *pCLCheck;		/* the checklist buffer DoCk uses	*/
{
	register int i, j, fSave;
	auto struct stat statb_me;

	fSave = fVerbose;
	fVerbose = 0;
	for (j = i = 0; i < argc; ++i) {
		if ('\000' == argv[i]) {
			argv[i] = ".";
		}
		if (-1 == STAT_CALL(argv[i], &statb_me)) {
			fprintf(stderr, "%s: stat: %s: %s\n", progname, argv[i], strerror(errno));
			continue;
		}
		Special(argv[i], pcLSpecial, pCLCheck);
		if (S_IFDIR == (statb_me.st_mode & S_IFMT)) {
			argv[j++] = argv[i];
			if (pCLCheck->ffound)
				DoCk(argv[i], & statb_me);
			continue;
		}
		if (pCLCheck->ffound) {
			DoCk(argv[i], & statb_me);
			continue;
		}
		printf("%s: `%s\' not in checklist %s\n", progname, argv[i], pcLSpecial);
		LeftCk(argv[i], & statb_me);
	}
	fVerbose = fSave;

	if (!fDirs)
		return;
	DirCk(j, argv, pcLSpecial, pCLCheck);
	OldCk(j, argv, pCLCheck);
	for (i = 0; i < j; ++i) {
		FileMatch("*", argv[i], LeftCk);
	}
}
