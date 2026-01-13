/*
 * $Id: special.c,v 8.3 1997/10/09 14:25:23 ksb Exp $
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

/*
 * handle the configuration file code					(ksb)
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>

#include "machine.h"
#include "configure.h"
#include "install.h"
#include "syscalls.h"
#include "special.h"
#include "main.h"
#if defined(INSTCK)
#include "instck.h"
#endif

#if STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

char *apcOO[] = {	/* name of 0/1 for user			*/
	"off",
	"on"
};

struct group *grpDef;		/* default group on all files (this dir)*/
struct passwd *pwdDef;		/* default owner on all files (this dir)*/

/* set up the users values for '=' in the config file			(ksb)
 */
void
InitCfg(pcOwner, pcGroup)
char *pcOwner, *pcGroup;
{

	extern char *getenv();
	extern int _pw_stayopen;
	pwdDef = (struct passwd *)0;

#ifdef PUCC
	_pw_stayopen = 1;
#endif	

	if ((char *)0 == pcOwner) {
		if (bHaveRoot) {
			pcOwner = DEFOWNER;
		} else {
			auto char *pcUser;

			pcUser = getenv("USER");
			if ((char *)0 == pcUser) {
				pcUser = getenv("LOGNAME");
			}
			if ((char *)0 != pcUser && '\000' != pcUser[0] && (struct passwd *)0 != (pwdDef = getpwnam(pcUser)) && getuid() == pwdDef->pw_uid) {
				/* got him */;
			} else if ((struct passwd *)0 == (pwdDef = getpwuid(getuid()))) {
				fprintf(stderr, "%s: getpwuid: %d: %s\n", progname, getuid(), strerror(errno));
				exit(10);
			}
			pwdDef = savepwent(pwdDef);
			pcOwner = pwdDef->pw_name;
		}
	}
	if ((char *)0 == pcOwner || '\000' == *pcOwner) {
		pcOwner = (char *)0;
	} else if ((struct passwd *)0 == pwdDef) {
		pwdDef = getpwnam(pcOwner);
		if ((struct passwd *)0 == pwdDef) {
			fprintf(stderr, "%s: getpwname: %s: %s (ignored)\n", progname, pcOwner, strerror(errno));
		} else {
			pwdDef = savepwent(pwdDef);
		}
	}

	grpDef = (struct group *)0;
	if ((char *)0 == pcGroup) {
		if (bHaveRoot) {
			pcGroup = DEFGROUP;
		} else {
			if ((struct group *)0 == (grpDef = getgrgid(getgid()))) {
				fprintf(stderr, "%s: getgrgid: %d: %s\n", progname, getgid(), strerror(errno));
				exit(11);
			}
			grpDef = savegrent(grpDef);
			pcGroup = grpDef->gr_name;
		}
	}
	if ((char *)0 == pcGroup || '\000' == *pcGroup) {
		pcGroup = (char *)0;
	} else if ((struct group *)0 == grpDef) {
		grpDef = getgrnam(pcGroup);
		if ((struct group *)0 == grpDef) {
			fprintf(stderr, "%s: getgrname: %s: %s (ignored)\n", progname, pcGroup, strerror(errno));
		} else {
			grpDef = savegrent(grpDef);
		}
	}
}


/*
 * get the user-level name for this node `plain file' or `socket'	(ksb)
 *	return string, ref-out single char
 */
char *
NodeType(mType, pcChar)
unsigned int mType;
char *pcChar;
{
	auto char acTemp[2];
	if ((char *)0 == pcChar)
		pcChar = acTemp;

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
#if defined(S_IFLNK)
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

/*
 * convert an octal mode to an integer					(jms)
 */
static void
OctMode(pcLMode, pmMode)
char *pcLMode;		/* the string to convert (e.g., "755")		*/
int *pmMode;		/* the place to put the converted mode		*/
{
	register int mode;	/* mode of installed file		*/
	register int c;		/* next character			*/

	for (mode = 0; (c = *pcLMode) != '\000'; ++pcLMode) {
		if (c >= '0' && c <= '7') {
			mode = (mode << 3) + (c - '0');
			continue;
		}
		(void)fprintf(stderr, "%s: bad digit int mode `%c\'\n", progname, *pcLMode);
		exit(EXIT_OPT);
	}
	if ((int *)0 != pmMode) {
		*pmMode |= mode;
	}
}


/*
 * extra bits in the bit listing are handled strangely			(ksb)
 */
static void
EBit(pcBit, bSet, cSet, cNotSet)
char *pcBit;		/* pointer the the alpha bit			*/
int bSet;		/* should it set over struck			*/
int cSet;		/* overstrike with this				*/
int cNotSet;		/* it should have been this, else upper case it	*/
{
	if (0 != bSet) {
		*pcBit = (cNotSet == *pcBit) ? cSet : toupper(cSet);
	}
}


static char acOnes[] = "rwxrwxrwx";		/* mode 0777		*/
/*
 * string to octal mode mask "rwxr-s--x" -> 02751.			(ksb)
 * Note that we modify the string to do this, but we put it back.
 */
static void
StrMode(pcLMode, pmBits, pmQuests)
char *pcLMode;		/* alpha bits buffer (about 10 chars)		*/
register int *pmBits;	/* manditory mode				*/
int *pmQuests;		/* optional mode bits				*/
{
	register int iBit, i;
	register char *pcBit;

	*pmBits = 0;
	if ((int *)0 != pmQuests) {
		*pmQuests = 0;
	}

	/* if the mode is long enough to have a file type field check that
	 * out and set the file type bits too
	 */
	switch (i = strlen(pcLMode)) {
	default:
		(void)fprintf(stderr, "%s: alphabetical mode must be 9 or 10 characters `%s\' is %d\n", progname, pcLMode, i);
		exit(EXIT_OPT);
	case 9:
		*pmBits |= S_IFREG;
		break;
	case 10:
		switch (*pcLMode) {
#if HAVE_SLINKS
		case 'l':	/* symbolic link */
			*pmBits |= S_IFLNK;
			break;
#endif	/* no links to think about */
#if defined(S_IFIFO)
		case 'p':	/* fifo */
			*pmBits |= S_IFIFO;
			break;
#endif	/* no fifos */
#if defined(S_IFSOCK)
		case 's':	/* socket */
			*pmBits |= S_IFSOCK;
			break;
#endif	/* no sockets */
		case 'b':
			*pmBits |= S_IFBLK;
			break;
		case 'c':
			*pmBits |= S_IFCHR;
			break;
		case '-':
		case 'f':
			*pmBits |= S_IFREG;
			break;
		case 'd':
			*pmBits |= S_IFDIR;
			break;
		default:
			(void)fprintf(stderr, "%s: unknown file type `%c\'\n", progname, *pcLMode);
			exit(EXIT_OPT);
		}
		++pcLMode;
		break;
	}

	/* copy out and turn off set{g,u}id and stick bits
	 * (The upper case bit means no execute underneath.)
	 */
	if ((char *)0 != (pcBit = strchr("sS", pcLMode[2]))) {
		pcLMode[2] = isupper(*pcBit) ? '-' : 'x';
		*pmBits |= S_ISUID;
	}
	if ((char *)0 != (pcBit = strchr("lLsS", pcLMode[5]))) {
		pcLMode[5] = 's' == *pcBit ? 'x' : '-';
		*pmBits |= S_ISGID;
	}
	if ((char *)0 != (pcBit = strchr("tT", pcLMode[8]))) {
		pcLMode[8] = isupper(*pcBit) ? '-' : 'x';
		*pmBits |= S_ISVTX;
	}

	/* read normal mode bits
	 */
	for (i = 0, iBit = 0400; i < 9; ++i, iBit >>= 1) {
		if (pcLMode[i] == acOnes[i]) {
			*pmBits |= iBit;
		} else if ('-' == pcLMode[i]) {
			continue;
		} else if ('\?' == pcLMode[i]) {
			if ((int *)0 != pmQuests)
				*pmQuests |= iBit;
		} else if ('\000' == pcLMode[i]) {
			(void)fprintf(stderr, "%s: not enough bits in file mode\n", progname);
		} else if ((char *)0 != strchr("rwxtTsS", pcLMode[i])) {
			(void)fprintf(stderr, "%s: bit `%c\' in file mode is in the wrong column\n", progname, pcLMode[i]);
			exit(EXIT_OPT);
		} else {
			(void)fprintf(stderr, "%s: unknown bit in file mode `%c\'\n", progname, pcLMode[i]);
			exit(EXIT_OPT);
		}
	}

	/* here we put the set{u,g}id and sticky bits back
	 */
	EBit(pcLMode+2, S_ISUID & *pmBits, 's', 'x');
	EBit(pcLMode+5, S_ISGID & *pmBits, 's', 'x');
	EBit(pcLMode+8, S_ISVTX & *pmBits, 't', 'x');
}


/*
 * Convert a string that ls(1) understands into something that	     (jms&ksb)
 * chmod(2) understands (extended), or an octal mode.
 ! The string we are given may be in the text (const) segment,
 ! we have to copy it to a write segment to allow StrMode to work.
 *
 * Optional bits may be specified after a `/' as in
 *	0711/044
 * which allows any of {0711, 0751, 0715, 0755}, or write symolicly
 *	-rwx--x--x/----r--r--
 * or a symbolic mode may contain a `?' in place of a bit for optional:
 *	-rwx?-x?-x
 * (which is quite readable).
 *
 * The slash notaion is needed for optional setgid directories, at least.
 */
void
CvtMode(pcLMode, pmMode, pmQuest)
char *pcLMode;		/* the string to convert (e.g., "755")		*/
int *pmMode;		/* the place to put the converted mode		*/
int *pmQuest;		/* the place to put the converted optional mode	*/
{
	register char *pcScan;	/* copy mode so we can write on it	*/
	auto char acDown[MAXPATHLEN+1];	/* where we write		*/

	if ((int *)0 != pmMode)
		*pmMode = 0;
	if ((int *)0 != pmQuest)
		*pmQuest = 0;

	pcScan = acDown;
	while ('/' != *pcLMode && '\000' != *pcLMode) {
		*pcScan++ = *pcLMode++;
	}
	*pcScan = '\000';

	if (isdigit(*acDown)) {
		OctMode(acDown, pmMode);
	} else {
		StrMode(acDown, pmMode, pmQuest);
	}

	if ('/' == *pcLMode && (int *)0 != pmQuest) {
		++pcLMode;
		(void)strcpy(acDown, pcLMode);	/* need a writable copy */
		if (isdigit(*acDown)) {
			OctMode(acDown, pmQuest);
		} else {
			StrMode(acDown, pmQuest, pmQuest);
		}
	}
}


#if defined(CONFIG)
/*
 * convert an integer mode into a symbolic format			(ksb)
 */
void
ModetoStr(pcLMode, mMode, mOptional)
char *pcLMode;		/* alpha bits buffer (10 chars or more)		*/
int mMode;		/* maditory mode bits				*/
int mOptional;		/* optional mode bits				*/
{
	register int i, iBit;

	(void)strcpy(pcLMode, acOnes);
	for (i = 0, iBit = 0400; i < 9; ++i, iBit >>= 1) {
		if (0 == (mMode & iBit))
			pcLMode[i] = '-';
	}
	EBit(pcLMode+2, S_ISUID & mMode, 's', 'x');
	EBit(pcLMode+5, S_ISGID & mMode, 's', 'x');
	EBit(pcLMode+8, S_ISVTX & mMode, 't', 'x');
	for (i = 0, iBit = 0400; i < 9; ++i, iBit >>= 1) {
		if (0 == (mOptional & iBit))
			continue;
		if ('-' == pcLMode[i])
			pcLMode[i] = '\?';
	}
}


/*
 * CompPath()
 *	Compress a path by assuming that . and .. really point to	(ksb)
 *	what they should (good under BSD at least :-|), we also
 *	assume the all things used as a directory all directories
 *	(and exist)
 *
 * Remove extra `/', `./', `foo/..'  (in place)
 * also replace a null path with `.'
 */
static char *
CompPath(pcFile)
char *pcFile;		/* path to compress				*/
{
	register char *pcScan, *pcPut;
	register int bFirst;

	pcScan = pcPut = pcFile;
	bFirst = 1;
	for (;;) { switch (*pcPut = *pcScan) {
	case '/':
		++pcPut;
		do
			++pcScan;
		while ('/' == *pcScan);
		bFirst = 1;
		continue;
	case '.':
		if (!bFirst) {
			++pcPut, ++pcScan;
			continue;
		}
		if ('/' == pcScan[1] || '\000' == pcScan[1]) {
			do
				++pcScan;
			while ('/' == *pcScan);
			continue;
		}
		/* do foo/.., but not /.. or just ..
		 */
		if ('.' == pcScan[1] && pcPut != pcFile &&
		    ('/' == pcScan[2] || '\000' == pcScan[2]) &&
		    !('/' == pcFile[0] && pcPut == pcFile+1)) {
			--pcPut;
			do
				--pcPut;
			while ('/' != *pcPut && pcPut != pcFile);
			if ('/' == *pcPut)
				++pcPut;
			pcScan += 2;
			while ('/' == *pcScan)
				++pcScan;
			continue;
		}
	default:
		++pcPut, ++pcScan;
		bFirst = 0;
		continue;
	case '\000':
		break;
	} break; }

	/* clean up trailing / and empty path name
	 */
	if (pcPut != pcFile) {
		while (--pcPut != pcFile && '/' == *pcPut)
			*pcPut = '\000';
	}
	if ('\000' == *pcFile) {
		pcFile[0] = '.';
		pcFile[1] = '\000';
	}
	return pcFile;
}



/*
 * _SamePath()
 *	We want /bin/* to match every file in /bin OK.			(ksb)
 *	return 1 for ==, 0 for !=
 *
 * N.B. Must call CompPath on pcFile first, or know it is minimal
 */
int
_SamePath(pcGlob, pcFile, fDot)
char *pcGlob;		/* the pattern to match				*/
char *pcFile;		/* the file to match with			*/
int fDot;		/* are we at the start of pcFile, or post '/'	*/
{
	register char *pc;
	register int iLenGlob, iLenFile;
	auto int bFound, cStop;
	auto int fStarStar;

	for (;;) { switch (*pcGlob) {
	case '*':		/* match any string			*/
		fStarStar = '*' == pcGlob[1];
		if (fStarStar) {
			++pcGlob;
		}
		pc = ++pcGlob;
		iLenGlob = 0;
		while ('\\' != *pc && '?' != *pc && '[' != *pc && '*' != *pc && '\000' != *pc && '/' != *pc) {
			++pc, ++iLenGlob;
		}

		if (fStarStar) {
			iLenFile = strlen(pcFile);
		} else {
			iLenFile = 0;
			while ('/' != pcFile[iLenFile] && '\000' != pcFile[iLenFile] &&
			       (!fDot || '.' != pcFile[iLenFile])) {
				++iLenFile;
				fDot = 0;
			}
		}

		bFound = 0;
		do {
			if (iLenGlob == 0 || 0 == strncmp(pcGlob, pcFile, iLenGlob)) {
				if (_SamePath(pc, pcFile+iLenGlob, fDot)) {
					bFound = 1;
					break;
				}
			}
			--iLenFile, ++pcFile;
		} while (iLenFile >= iLenGlob);
		return bFound;
	case '[':		/* any of				*/
		++pcGlob;
		cStop = *pcFile++;
		if (cStop == '/')	/* range never match '/'	*/
			break;
		bFound = 0;
		if ('-' == *pcGlob) {
			bFound = '-' == cStop;
			++pcGlob;
		}
		while (']' != *pcGlob) {
			if ('-' == pcGlob[1]) {
				if (pcGlob[0] <= cStop && cStop <= pcGlob[2])
					bFound = 1;
				pcGlob += 2;
			} else {
				if (pcGlob[0] == cStop)
					bFound = 1;
			}
			++pcGlob;
		}
		++pcGlob;
		if (!bFound)
			break;
		continue;
	case '?':		/* and single char but '/'		*/
		if ('/' == *pcFile || (fDot && '.' == *pcFile) || '\000' == *pcFile)
			break;
		++pcGlob, ++pcFile;
		fDot = 0;
		continue;
	case '\\':		/* next char not special		*/
		++pcGlob;
		/*fall through*/
	case '/':		/* delimiter				*/
		fDot = 1;
		if (*pcGlob != *pcFile)
			break;
		++pcGlob;
		do {
			++pcFile;
		} while ('/' == *pcFile);
		continue;
	default:		/* or any other character		*/
		fDot = 0;
		if (*pcGlob != *pcFile)
			break;
		++pcGlob, ++pcFile;
		continue;
	case '\000':		/* end of pattern, end of file name	*/
		return '\000' == *pcFile;
	} break; }
	return 0;
}

/*
 * called if we have a config line that makes little sense		(ksb)
 *  (in that is says to set{u,g}id to a `*' {user,group})
 */
void
BadSet(iLine, cWhich, pcNoun, pcBadMode)
int iLine;
char cWhich, *pcNoun, *pcBadMode;
{
#if INSTALL
	if (FALSE == fVerbose)
		return;
	(void)fprintf(stderr, "%s: %s(%d): checklist specifies set%cid to a random %s with mode %s\n", progname, pcSpecial, iLine, cWhich, pcNoun, pcBadMode);
#else
	(void)fprintf(fpOut, "%s: %s(%d): checklist specifies set%cid to a random %s with mode %s\n", progname, pcSpecial, iLine, cWhich, pcNoun, pcBadMode);
#endif
}


/*
 * find an apropriate place to compare a file to a pattern		(ksb)
 *  pat == `RCS/*,v'
 *  file == /tmp/junk/RCS/main.h,v
 * return a pointer to
 *  `RCS/main.h,v'
 * pats with a leading / or a ** in them operate on the while filename
 */
static char *
RJust(pcPat, pcFile, pcLast)
char *pcPat, *pcFile, *pcLast;
{
	register char *pcRev;

	if ('/' == pcPat[0])
		return pcFile;
	for (pcRev = pcPat; '\000' != *pcRev; ++pcRev) {
		if ('*' == pcRev[0] && '*' == pcRev[1])
			return pcFile;
	}
	/* N.B. note pcRev at end of string */
	do {
		do {
			--pcRev;
		} while (pcPat != pcRev && '/' != *pcRev);
		if ('/' == *pcRev) {
			do {
				--pcRev;
			} while (pcPat != pcRev && '/' == *pcRev);
			do {
				--pcLast;
			} while (pcFile != pcLast && '/' == *pcLast);
			while (pcFile != pcLast && '/' != *pcLast)
				--pcLast;
			if (pcLast == pcFile)
				return pcFile;
			while ('/' == *pcLast)
				++pcLast;
		}
	} while (pcPat != pcRev);
	return pcLast;
}

/* emulate the bsd fgetline() kinda thing for really long cfg lines	(ksb)
 */
char *
BigLine(fpIn)
FILE *fpIn;
{
	static char *pcBuf = (char *)0;
	static unsigned long ulLen = 0;
	register unsigned long ulOffset;

	if ((char *)0 == pcBuf) {
		pcBuf = malloc(ulLen = MAXCNFLINE);
		if ((char *)0 == pcBuf) {
			return pcBuf;
		}
	}
	ulOffset = 0;
	while ((char *)0 != fgets(pcBuf+ulOffset, ulLen-ulOffset, fpIn)) {
		ulOffset += strlen(pcBuf+ulOffset);
		if ('\n' == pcBuf[ulOffset-1]) {
			return pcBuf;
		}
		if (ulOffset+1 < ulLen) {
			continue;
		}
		ulLen += MAXCNFLINE;
		pcBuf = realloc(pcBuf, ulLen);
		if ((char *)0 == pcBuf) {
			return (char *)0;
		}
	}
	if (0 == ulOffset) {
		return (char *)0;
	}
	/* add the missing \n? LLL
	 */
	return pcBuf;
}


/* build "DirCk"  or "Special"
 */
#include "special.i"

#if defined(INSTCK)
/* if we are instck build "Specail"
 */
#undef INSTCK
#define INSTALL	1
#include "special.i"
#undef INSTALL
#endif

#endif	/* check list file */
