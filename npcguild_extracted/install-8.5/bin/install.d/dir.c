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

/*
 * install a directory						(ksb)
 */
#if !defined(lint)
static char *rcsid = "$Id: dir.c,v 8.2 1997/07/30 14:33:59 ksb Exp $";
#endif	/* !lint */

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
#include "main.h"
#include "dir.h"
#include "file.h"
#include "syscalls.h"
#include "special.h"

#if STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#if !defined(F_OK)
#define F_OK	0
#endif

#if !defined(BINLS)
#define BINLS		"/bin/ls"
#endif

char acLs[] =		BINLS;


#if !defined(LSDIRARGS)
#if defined(SYSV) || defined(HPUX7)
#define LSDIRARGS	"-ld"
#else	/* bsd needs a -g option to show group	*/
#define LSDIRARGS	"-lgd"
#endif	/* how does ls(1) show both owner&group	*/
#endif

char acLsDirArgs[] =	LSDIRARGS;


#if !HAVE_MKDIR
#if !defined(BINMKDIR)
#define BINMKDIR	"/bin/mkdir"
#endif
char acMkdir[] =	BINMKDIR;
#endif


static char acDCreated[] = "directory `%s\' created %s.%s(%04o) by %s\n";
static char acDUpdated[] = "directory `%s\' updated %s.%s(%04o) by %s\n";
static char acDRemoved[] = "directory `%s\' removed %s.%s(%04o) by %s\n";

static int fLast = 0;
static struct stat stLast;

/*
 * DirInstall()
 *	Create a directory with given modes and owners
 *
 * Side effects
 *	Creates a directory somewhere
 */
int
DirInstall(pcDir, pcOwner, pcGroup, pcMode, pcDefOwner, pcDefGroup, pcDefMode, pcSLinks, fRemove)
char *pcDir;		/* the directory to create			*/
char *pcOwner;		/* owner to give it				*/
char *pcGroup;		/* group ownership				*/
char *pcMode;		/* explicit -m option				*/
char *pcDefOwner;	/* default owner to give it			*/
char *pcDefGroup;	/* default group ownership			*/
char *pcDefMode;	/* default if no -m option and doesn't exist	*/
char *pcSLinks;		/* symbolic links to the directory		*/
int fRemove;		/* remove the directory, do not install		*/
{
	register char *pcSlash;
	auto int bSame;			/* exist dir is the same as req.*/
	auto struct passwd *pwd;	/* owner of this dir.		*/
	auto struct group *grp;		/* group of this directory	*/
	auto int mMode, mOptMode;	/* mode of this directory	*/
	auto struct stat statb_dir;	/* for this dir or parrent	*/
	auto int bDestExists;		/* destination file exists?	*/
	auto char acDir[MAXPATHLEN+1];	/* copy of our name to munge	*/
	auto char *pcMsg;		/* error message		*/
#if defined(CONFIG)
	auto CHLIST Check;		/* must match this record	*/
#endif	/* set check flags from config file	*/
#if defined(INST_FACILITY)
	auto char *pcLogStat;
	auto char acLogBuf[MAXLOGLINE];
#endif	/* we should syslog changes		*/

	if (strlen(pcDir) > MAXPATHLEN) {
#if defined(ENAMETOOLONG)
		fprintf(stderr, "%s: directory name: %s\n", progname, strerror(ENAMETOOLONG));
#else
		fprintf(stderr, "%s: directory name: name too long\n", progname);
#endif
		return FAIL;
	}

	/*	install -d /bin/
	 * used to choke us because we tried to make `/bin' and `/bin/'
	 *	install -d /usr/local//bin
	 * was a funny case too
	 */
	/* the New Castle Conection would hate us here ... */
	for (bSame = '\000', pcSlash = acDir; '\000' != *pcDir; ++pcDir) {
		if ('/' == bSame && '/' == *pcDir)
			continue;
		*pcSlash++ = bSame = *pcDir;
	}
	if (pcSlash == acDir) {
		(void)strcpy(acDir, ".");
	} else if (pcSlash != acDir+1 && '/' == pcSlash[-1]) {
		pcSlash[-1] = '\000';
	} else {
		pcSlash[0] = '\000';
	}

#if defined(CONFIG)
	Special(acDir, pcSpecial, & Check);
#endif	/* set check flags from config file	*/

	if (-1 != stat(acDir, & statb_dir)) {
		bDestExists = TRUE;
		/* LLL check fOnlyNew? -- ksb */
	} else {

		/* we need the parrent dirs modes to copy here...
		 */
		if ((char *)0 != (pcSlash = strrchr(acDir, '/'))) {
			while (pcSlash != acDir && '/' == pcSlash[-1])
				pcSlash--;
			*pcSlash = '\000';
			if (pcSlash == acDir) {
				if (-1 == stat("/", & statb_dir)) {
					fprintf(stderr, "%s: stat: /: %s\n", progname, strerror(errno));
					return FAIL;
				}
			} else if (-1 == stat(acDir, & statb_dir)) {
				/* we may use code like this to get (-rd)
				 * Sun-like recursive dir install behavior
				 */
				if (errno != ENOENT) {
					fprintf(stderr, "%s: stat: %s: %s\n", progname, acDir, strerror(errno));
					return FAIL;
				}
				if (FALSE == fRecurse) {
					if (!fLast) {
						fprintf(stderr, "%s: stat: %s: %s\n", progname, acDir, strerror(errno));
						return FAIL;
					}
					statb_dir = stLast;
				} else if (FAIL == DirInstall(acDir, pcOwner, pcGroup, pcMode, pcDefOwner, pcDefGroup, pcDefMode, (char *)0, 0)) {
					return FAIL;
				} else if (FALSE == fTrace && -1 == stat(acDir, & statb_dir)) {
					fprintf(stderr, "%s: stat: %s: %s\n", progname, acDir, strerror(errno));
					return FAIL;
				}
			}
			*pcSlash = '/';
		} else if (-1 == stat(".", & statb_dir)) {
			fprintf(stderr, "%s: stat: .: %s\n", progname, strerror(errno));
			return FAIL;
		}
		bDestExists = FALSE;
	}

	(void)setpwent();
	if ((char *)0 != pcOwner) {
		if ((struct passwd *)0 == (pwd = getpwnam(pcOwner))) {
			(void)fprintf(stderr, "%s: getpwname: %s not found\n", progname, pcOwner);
			exit(EXIT_OPT);
		}
		if (FALSE == bHaveRoot && pwd->pw_uid != geteuid()) {
			(void)fprintf(stderr, "%s: effective uid cannot make directory owned by %s (%d != %d)\n", progname, pcOwner, geteuid(), pwd->pw_uid);
			exit(EXIT_OPT);
		}
	} else if (bDestExists != FALSE) {
		if ((struct passwd *)0 == (pwd = getpwuid((int) statb_dir.st_uid))) {
			(void)fprintf(stderr, "%s: destination owner %d doesn\'t exist\n", progname, statb_dir.st_uid);
			exit(EXIT_OPT);
		}
	} else if (bHaveRoot) {
		if ((char *)0 != pcDefOwner) {
			pcOwner = pcDefOwner;
			if ((struct passwd *)0 == (pwd = getpwnam(pcOwner))) {
				(void)fprintf(stderr, "%s: getpwname: %s not found\n", progname, pcOwner);
				exit(EXIT_OPT);
			}
		} else {
			if ((struct passwd *)0 == (pwd = getpwuid((int) statb_dir.st_uid))) {
				(void)fprintf(stderr, "%s: destination directory owner %d doesn\'t exist\n", progname, statb_dir.st_uid);
				exit(EXIT_OPT);
			}
		}
	} else if ((struct passwd *)0 == (pwd = getpwuid((int) geteuid()))) {
		(void)fprintf(stderr, "%s: getpwuid: %d (effective uid) not found\n", progname, geteuid());
		exit(EXIT_OPT);
	}
	pwd = savepwent(pwd);
	(void)endpwent();

	(void)setgrent();
	if ((char *)0 != pcGroup) {
		if ((struct group *)0 == (grp = getgrnam(pcGroup))) {
			(void)fprintf(stderr, "%s: getgrname: %s not found\n", progname, pcGroup);
			exit(EXIT_OPT);
		}
	} else if (bDestExists != FALSE) {
		grp = getgrgid((int) statb_dir.st_gid);
		if ((struct group *)0 == grp) {
			(void)fprintf(stderr, "%s: no group entry for destination group %d\n", progname, statb_dir.st_gid);
			exit(EXIT_OPT);
		}
	} else if (bHaveRoot) {
		if ((char *)0 != pcDefGroup) {
			pcGroup = pcDefGroup;
			if ((struct group *)0 == (grp = getgrnam(pcGroup))) {
				(void)fprintf(stderr, "%s: getgrname: %s not found\n", progname, pcGroup);
				exit(EXIT_OPT);
			}
		} else {
			grp = getgrgid((int) statb_dir.st_gid);
			if ((struct group *)0 == grp) {
				(void)fprintf(stderr, "%s: no group entry for destination directory group %d\n", progname, statb_dir.st_gid);
				exit(EXIT_OPT);
			}
		}
	} else if ((struct group *)0 == (grp = getgrgid((int) getegid()))) {
		(void)fprintf(stderr, "%s: getgrgid: %d (effective gid) not found\n", progname, getegid());
		exit(EXIT_OPT);
	}
	grp = savegrent(grp);
	(void)endgrent();

	/* take specified mode, use destination mode, or the default
	 * or inherit it from the parrent directory
	 */
	if ((char *)0 != pcMode) {
		CvtMode(pcMode, & mMode, & mOptMode);
		if (0 != mOptMode) {
			mMode |= statb_dir.st_mode & mOptMode;
		}
	} else if (bDestExists != FALSE) {
		mMode = statb_dir.st_mode &~ S_IFMT;
	} else if ((char *)0 != pcDefMode) {
		CvtMode(pcDefMode, & mMode, & mOptMode);
		if (0 != mOptMode) {
			mMode |= statb_dir.st_mode & mOptMode;
		}
	} else {
		mMode = statb_dir.st_mode &~ S_IFMT;
	}

#if defined(CONFIG)
	if (Check.ffound) {
		register int mChk;
		register int fFail;

		fFail = FALSE;
		if ((char *)0 != Check.pclink) {
			(void)fprintf(stderr, "%s: `%s\' be a %s link to `%s\'\n", progname, acDir, ':' == Check.pclink[0] ? "hard" : "symbolic", Check.pclink+1);
			fFail = TRUE;
			goto quit;
		}
		if ('*' == Check.acowner[0]) {
			/* OK no check */;
		} else if (Check.fbangowner) {
			if (Check.uid == pwd->pw_uid) {
				(void)fprintf(stderr, "%s: `%s\' should not have owner %s\n", progname, acDir, Check.acowner);
				fFail = TRUE;
			}
		} else if (Check.uid != pwd->pw_uid) {
			(void)fprintf(stderr, "%s: `%s\' owner %s should be %s\n", progname, acDir, pwd->pw_name, Check.acowner);
			fFail = TRUE;
		}

		if ('*' == Check.acgroup[0]) {
			/*OK */;
		} else if (Check.fbangowner) {
			if (Check.uid == pwd->pw_uid) {
				(void)fprintf(stderr, "%s: `%s\' should not have group %s\n", progname, acDir, Check.acgroup);
				fFail = TRUE;
			}
		} else if (Check.gid != grp->gr_gid) {
			(void)fprintf(stderr, "%s: `%s\' group %s should be %s\n", progname, acDir, grp->gr_name, Check.acgroup);
			fFail = TRUE;
		}

		switch (Check.acmode[0]) {
		case '?':
		case '*':
			/*OK*/;
			break;
		case 'd':
			mChk = PERM_RWX(Check.mmust|Check.moptional);
			if (PERM_RWX(Check.mmust) != PERM_RWX(mMode&Check.mmust)) {
				(void)fprintf(stderr, "%s: `%s\' mode %04o doesn\'t have bits to match %s (%04o)\n", progname, acDir, PERM_RWX(mMode), Check.acmode, mChk);
				fFail = TRUE;
			} else if (0 != (PERM_RWX(mMode) &~ mChk)) {
				(void)fprintf(stderr, "%s: `%s\' mode %04o has too many bits to match %s (%04o)\n", progname, acDir, PERM_RWX(mMode), Check.acmode, mChk);
				fFail = TRUE;
			}
			if (0 != (S_ISUID & mMode) ? 0 == (S_ISUID & (Check.mmust|Check.moptional)) : 0 != (S_ISUID & Check.mmust)) {
				(void)fprintf(stderr, "%s: `%s\' setuid bit must be %s\n", progname, acDir, apcOO[0 == (S_ISUID & mMode)]);
				fFail = TRUE;
			}
			if (0 != (S_ISGID & mMode) ? 0 == (S_ISGID & (Check.mmust|Check.moptional)) : 0 != (S_ISGID & Check.mmust)) {
				(void)fprintf(stderr, "%s: `%s\' setgid bit must be %s\n", progname, acDir, apcOO[0 == (S_ISGID & mMode)]);
				fFail = TRUE;
			}
			if (0 != (S_ISVTX & mMode) ? 0 == (S_ISVTX & (Check.mmust|Check.moptional)) : 0 != (S_ISVTX & Check.mmust)) {
				(void)fprintf(stderr, "%s: `%s\' sticky bit must be %s\n", progname, acDir, apcOO[0 == (S_ISVTX & mMode)]);
				fFail = TRUE;
			}
			break;
		default:
			(void)fprintf(stderr, "%s: `%s\' must be a %s\n", progname, acDir, NodeType(Check.mtype, (char *)0));
			fFail = TRUE;
			break;
		case '!':
		case '~':
			(void)fprintf(stderr, "%s: `%s\' should not be installed", progname, acDir);
			if ((char *)0 != Check.pcmesg && '\000' != Check.pcmesg[0])
				(void)fprintf(stderr, ", %s", Check.pcmesg);
			(void)fputc('\n', stderr);
			fFail = TRUE;
			break;
		}
	quit:
		if (FALSE != fFail) {
			return FAIL;
		}
	}
#endif	/* have check list to compare with	*/

	if (fRemove) {
		register char *pcEnd;

		if (!bDestExists) {
			(void)fprintf(stderr, "%s: `%s\' doesn't exist to remove\n", progname, acDir);
			return FAIL;
		}

		/* strangely this will remove /bin/OLD/OLD too,
		 * but I think this is what we want in that case...
		 */
		if ((char *)0 == (pcEnd = strrchr(acDir, '\000')))
			Die("nil eos pointer");
		pcEnd[0] = '/';
		(void)strcpy(pcEnd+1, OLDDIR);
		if (-1 != access(acDir, F_OK) &&
		    FAIL == DirInstall(acDir, FALSE == bHaveRoot ? (char *)0 : ODIROWNER, FALSE == bHaveRoot ? (char *)0 : ODIRGROUP, ODIRMODE, (char *)0, (char *)0, (char *)0, (char *)0, 1)) {
			return FAIL;
		}
		*pcEnd = '\000';

#if defined(CONFIG)
		if (Check.ffound) {
			(void)printf("%s: %s(%d) %s %s, remove it?\n", progname, Check.pcspecial, Check.iline, 0 == strcmp(Check.pcpat, acDir) ? "is" : "matches", acDir);
		}
#endif	/* config list to modify		*/

		if (FALSE != fTrace) {
			(void)printf("%s: rmdir %s\n", progname, acDir);
		} else if (-1 == rmdir(acDir)) {
			(void)fprintf(stderr, "%s: rmdir: %s: %s\n", progname, acDir, strerror(errno));
			return FAIL;
		}

#if defined(INST_FACILITY)
		/*
		 * do not syslog any user built directories
		 */
		if (bHaveRoot && FALSE == fTrace) {
			(void)sprintf(acLogBuf, acDRemoved, acDir, pwd->pw_name, grp->gr_name, mMode, pcGuilty);
			syslog(LOG_INFO, acLogBuf);
		}
#endif	/* we should syslog changes		*/

		return SUCCEED;
	} else if (bDestExists) {
		pcMsg = NodeType(statb_dir.st_mode, (char *)0);
		switch (statb_dir.st_mode & S_IFMT) {
#if HAVE_SLINKS
		case S_IFLNK:	/* symbolic link */
#endif	/* no links to think about */
#if defined(S_IFIFO)
		case S_IFIFO:	/* fifo */
#endif	/* no fifos */
#if defined(S_IFSOCK)
		case S_IFSOCK:	/* socket */
#endif	/* no sockets */
		case S_IFCHR:	/* character special */
		case S_IFBLK:	/* block special */
		case 0:
		case S_IFREG:
			(void)fprintf(stderr, "%s: directory `%s\' is already a %s\n", progname, acDir, pcMsg);
			return FAIL;

		case S_IFDIR:	/* directory */
			break;

		default:
			(void)fprintf(stderr, "%s: unrecognized file type on `%s\'\n", progname, acDir);
			return FAIL;
		}
		bSame = TRUE;
		if (statb_dir.st_uid != pwd->pw_uid) {
			if (FALSE == fQuiet)
				(void)fprintf(stderr, "%s: `%s\' owner mismatch (%d != %d)\n", progname, acDir, pwd->pw_uid, statb_dir.st_uid);
			bSame = FALSE;
		}
		if (statb_dir.st_gid != grp->gr_gid) {
			if (FALSE == fQuiet)
				(void)fprintf(stderr, "%s: `%s\' group mismatch (%d != %d)\n", progname, acDir, grp->gr_gid, statb_dir.st_gid);
			bSame = FALSE;
		}
		if (PERM_BITS(statb_dir.st_mode) != PERM_BITS(mMode)) {
			if (FALSE == fQuiet)
				(void)fprintf(stderr, "%s: `%s\' mode mismatch (%04o != %04o)\n", progname, acDir, mMode, statb_dir.st_mode &~ S_IFMT);
			bSame = FALSE;
		}
		if ((S_ISUID & mMode) != (S_ISUID & statb_dir.st_mode)) {
			(void)fprintf(stderr, "%s: `%s\' setuid bit changed, was %s\n", progname, acDir, apcOO[0 != (S_ISUID & mMode)]);
			bSame = FALSE;
		}
		if ((S_ISGID & mMode) != (S_ISGID & statb_dir.st_mode)) {
			(void)fprintf(stderr, "%s: `%s\' setgid bit changed, was %s\n", progname, acDir, apcOO[0 != (S_ISGID & mMode)]);
			bSame = FALSE;
		}
		if ((S_ISVTX & mMode) != (S_ISVTX & statb_dir.st_mode)) {
			(void)fprintf(stderr, "%s: `%s\' sticky bit changed, was %s\n", progname, acDir, apcOO[0 != (S_ISVTX & mMode)]);
			bSame = FALSE;
		}
		if (FALSE != bSame) {
			goto show_dir;
		}
#if defined(INST_FACILITY)
		pcLogStat = acDUpdated;
#endif	/* we should syslog changes		*/
	} else {
#if HAVE_MKDIR
		if (FALSE != fTrace) {
			(void)printf("%s: mkdir %s\n", progname, acDir);
		} else if (-1 == mkdir(acDir, mMode)) {
			(void)fprintf(stderr, "%s: mkdir: %s: %s\n", progname, acDir, strerror(errno));
			return FAIL;
		}
#else	/* BSD */
		if (RunCmd(acMkdir, acDir, (char *)0) != 0) {
			/* mkdir reported fail for us */
			return FAIL;
		}
#endif	/* make a directory */
#if defined(INST_FACILITY)
		pcLogStat = acDCreated;
#endif	/* we should syslog changes		*/
	}

	if (FALSE != bHaveRoot) {
		ChOwnGrp(acDir, pwd, grp);
	} else {
		ChGroup(acDir, grp);
	}
	ChMode(acDir, mMode);
	stLast.st_uid = pwd->pw_uid;
	stLast.st_gid = grp->gr_gid;
	stLast.st_mode = mMode | S_IFDIR;
	fLast = 1;

#if defined(INST_FACILITY)
	/*
	 * do not syslog any user built directories
	 */
	if (bHaveRoot && FALSE == fTrace) {
		(void)sprintf(acLogBuf, pcLogStat, acDir, pwd->pw_name, grp->gr_name, mMode, pcGuilty);
		syslog(LOG_INFO, acLogBuf);
	}
#endif	/* we should syslog changes		*/

show_dir:
	/* here we could make symbolic links	LLL
	 */
	if ((char *)0 != pcSLinks) {
		if (LaunchLinks(&statb_dir, acDir, (char *)0, pcSLinks, mMode, pwd, grp)) {
			(void)fprintf(stderr, "%s: %s: symbolic links failed\n", progname, acDir);
		}
	}

#if defined(CONFIG)
	/* if the file is in a check list report installation message
	 */
	if (Check.ffound && '\000' != Check.pcmesg[0]) {
		(void)printf("%s: %s: %s\n", progname, acDir, Check.pcmesg);
	}
#endif	/* have check list to output a comment	*/

	/*
	 * show what we built with ls(1)
	 */
	if (FALSE != fVerbose) {
		(void)RunCmd(acLs, acLsDirArgs, acDir);
	}

	return SUCCEED;
}
