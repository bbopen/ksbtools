/*
 * Copyright 1990 Purdue Research Foundation, West Lafayette, Indiana
 * 47907.  All rights reserved.
 *
 * Written by Kevin S Braunsdorf, ksb@cc.purdue.edu, purdue!ksb
 *	      Jeff Smith, jsmith@cc.purdue.edu, purdue!jsmith
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
 * install a file
 */
#if !defined(lint)
static char *rcsid = "$Id: file.c,v 8.20 2009/12/08 22:58:14 ksb Exp $";
#endif	/* !lint */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <syslog.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <sysexits.h>

#include "machine.h"
#include "configure.h"
#include "install.h"
#include "main.h"
#include "dir.h"
#include "syscalls.h"
#include "special.h"

#if STRINGS
#include <strings.h>
#else
#include <string.h>
#endif


/*
 * paths, names and options of tools we need to fork
 */
#if !defined(BINSTRIP)
#define BINSTRIP	"/bin/strip"
#endif

static char acStrip[] =	BINSTRIP;

#if HAVE_RANLIB
#if !defined(BINRANLIB)
#define BINRANLIB	"/usr/bin/ranlib"
#endif

static char acRanlib[] =	BINRANLIB;
#endif	/* sysV site do not have to run ranlib	*/

#if !defined(LSARGS)
#if defined(BSD)
#if HAVE_CHFLAGS
#define LSARGS		"-lgo"
#else
#define LSARGS		"-lg"
#endif
#else	/* bsd needs a -g option to show group	*/
#define LSARGS		"-l"
#endif	/* how does ls(1) show both owner&group	*/
#endif

static char acLsArgs[] =	LSARGS;


/* If the backup directory doesn't exist, create it.  We didn't
 * check for this until now because we don't know the name of the
 * backup directory till now, and if there is really a file to install.
 * lop off last component of pcBackPath...
 */
void
MkOld(pcBackPath)
char *pcBackPath;		/* full backup path			*/
{
	register char *pcTemp;
	auto char acDD[MAXPATHLEN+1];
	auto struct stat statb;		/* stat of OLD directory	 */
	auto struct stat statb_dd;	/* stat of OLD/.. directory	 */

	pcTemp = strrchr(pcBackPath, '/');
	if (pcTemp == (char *)0) {
		Die("MkOld");
	}

	/* This is a bit of a kludge.  mkdir(1) under 2.9bsd can't
	 * stand trailing '/' chars in pathnames.
	 */
	while ('/' == *pcTemp && pcTemp > pcBackPath) {
		--pcTemp;
	}

	if ('/' != *pcTemp)
		++pcTemp;
	*pcTemp = '\000';
	if ('\000' == pcBackPath[0]) {
		/* slash must exist! */;
	} else if (-1 == STAT_CALL(pcBackPath, &statb)) {
		/* we don't want to use the mode specified with -m for
		 * the OLD directory since that probably wasn't what they
		 * wanted, so use a reasonable compile-time default
		 */
		if (FAIL == DirInstall(pcBackPath, FALSE == bHaveRoot ? (char *)0 : ODIROWNER, FALSE == bHaveRoot ? (char *)0 : ODIRGROUP, ODIRMODE, (char *)0, (char *)0, (char *)0, (char *)0, 0)) {
			(void)fprintf(stderr, "%s: can\'t create `%s\'\n", progname, pcBackPath);
			exit(EX_OSERR);
		}
		if (fVerbose != FALSE) {
			(void)fprintf(stderr, "%s: had to create `%s\'\n", progname, pcBackPath);
		}
	} else if (S_IFDIR != (statb.st_mode & S_IFMT)) {
		(void)fprintf(stderr, "%s: %s must be a directory\n", progname, pcBackPath);
		exit(EX_USAGE);
	} else {
		(void)strcpy(acDD, pcBackPath);
		(void)strcat(acDD, "/..");
		if (-1 == STAT_CALL(acDD, &statb_dd)) {
			(void)fprintf(stderr, "%s: stat: %s: %s\n", progname, acDD, strerror(errno));
			exit(EX_OSERR);
		}
		if (statb.st_dev != statb_dd.st_dev) {
			(void)fprintf(stderr, "%s: `%s\' is a mount point!\n", progname, pcBackPath);
			exit(EX_USAGE);
		}
	}
	*pcTemp = '/';
}



/*
 * DoBackup()
 *	Actually backs up the file by renaming it
 */
/*ARGSUSED*/
static int
DoBackup(bWasLink, pcDestPath, pcBackPath, pcNewBack, pcTellNew)
int bWasLink;		/* file was a symlink				*/
char *pcDestPath;	/* file that will be clobbered			*/
char *pcBackPath;	/* filename to try as backup name		*/
char *pcNewBack;	/* if we moved $DEST/OLD/foo, new name		*/
char *pcTellNew;	/* tell the user the new backup name for OLD/foo*/
{
	auto struct stat statb_backup;	/* stat of OLD/name		 */

	pcNewBack[0] = '\000';		/* didn't have to move it	*/

	MkOld(pcBackPath);

	if (FALSE != bWasLink) {
#if HAVE_SLINKS
		if (CopySLink(pcDestPath, pcBackPath)) {
			return SUCCEED;
		}
#endif	/* can copy what we do not have?	*/
		return FAIL;
	}

	/* backup target already exists, mv it
	 */
	if (-1 != stat(pcBackPath, &statb_backup)) {
		(void)strcpy(pcNewBack, pcBackPath);
		MungName(pcNewBack);
		if (FAIL == Rename(pcBackPath, pcNewBack, pcTellNew)) {
			/* rename output an error message for us */
			return FAIL;
		}
	}

	/* Just link the backup file to the current file to avoid the window
	 * that results from renaming the backup before the new file is
	 * installed.  Resolve collisions if necessary.  If we can't link, copy.
	 */
	if (FALSE != fTrace) {
		(void)printf("%s: ln %s %s\n", progname, pcDestPath, pcBackPath);
	} else if (-1 == link(pcDestPath, pcBackPath)) {
		if (FAIL == DoCopy(pcDestPath, pcBackPath)) {
			(void)fprintf(stderr, "%s: can\'t link or copy `%s\' to `%s\'\n", progname, pcBackPath, pcDestPath);
			return FAIL;
		}
	}

	return SUCCEED;
}


/*
 * MakeNames()
 *	Given the file to install and the destination, return the full path of
 *	the destination and the full path of the backup file.  This is somewhat
 *	complicated since the destination may be a directory or a full path, and
 *	the full path may end in a filename that exists or not.
 */
void
MakeNames(fStdin, pcFTI, pcDest, pcFull, pcBack)
int fStdin;		/* we are doing stdin here	(in )		*/
char *pcFTI;		/* File To Install		(in )		*/
char *pcDest;		/* Destination of pcFTI		(in )		*/
char *pcFull;		/* full pathname of destination (out)		*/
char *pcBack;		/* full pathname of the backup	(out)		*/
{
	register char *pcTailFTI;	/* tail of pcFTI		*/
	register char *pcDestDir;	/* destination directory	*/
	register char *pcTailDest;	/* tail of pcDest		*/
	register int fIsDir;		/* target is a directory	*/
	auto struct stat stDest;

	/* Get tail of file to install
	 */
	if ((pcTailFTI = strrchr(pcFTI, '/')) == (char *)0) {
		pcTailFTI = pcFTI;
	} else {
		++pcTailFTI;
	}

	/* Get the name of the destination directory and the name the file
	 * to install will have in that directory.  If the destination we were
	 * passed is a directory then the destination dir is just pcDest
	 * and the filename is the tail of the file to install.  If the
	 * destination isn't a directory, then either they gave a pathname
	 * (something with '/' in it) for the file to install, in which case
	 * the destination directory is the head of that path and the filename
	 * is the tail, or they're installing the file in the current directory
	 * with a different name.  Note that this can't be the case where they
	 * say "install foo bar" and bar is a directory because we already
	 * took care of that in the first case.
	 * (trailing slashes used to choke us, now we remove them first)
	 */
	while ((char *)0 != (pcTailDest = strrchr(pcDest, '/')) && pcDest != pcTailDest && '\000' == pcTailDest[1]) {
		*pcTailDest = '\000';
	}

	/* we can't use "IsDir" here because symbolic links to dirs
	 * in the delete case broke, links to dirs in the install case
	 * do the "right" thing, we think. {Since /bin is a sym link
	 * on some systems and we want "install -v ls /bin" to work.}
	 */
	if (-1 == STAT_CALL(pcDest, &stDest)
#if HAVE_SLINKS
	 || (fDelete && S_IFLNK == (stDest.st_mode & S_IFMT))
#endif
	) {
		fIsDir = 0;
	} else {
		fIsDir = IsDir(pcDest);
	}

	if (fIsDir) {
		pcDestDir = pcDest;
		pcTailDest = pcTailFTI;
		if (fStdin) {
			if (fDelete) {
				(void)fprintf(stderr, "%s: use \"%s -R -d%s %s\" to remove a directory\n", progname, progname, fVerbose ? "v" : "", pcDest);
			} else {
				(void)fprintf(stderr, "%s: cannot intuit destination name for `-\' (stdin)\n", progname);
			}
			exit(EX_USAGE);
		}
	} else if ((char *)0 != pcTailDest) {
		pcDestDir = pcDest;
		*pcTailDest++ = '\000';
	} else {
		pcDestDir = ".";
		pcTailDest = pcDest;
	}

	/* make pathname of file to back up.
	 */
	(void)sprintf(pcBack, "%s/%s/%s", pcDestDir, OLDDIR, pcTailDest);

	/* Make the pathname the file to install will have
	 */
	(void)sprintf(pcFull, "%s/%s", pcDestDir, pcTailDest);
}

/* pull an item out of a PATH-like list
 *  "ksb", "PQD", "foo:bar:ksb:wham"  -> "foo:bar:PQD:wham"
 * (don't modify the args forever)
 */
char *
Xtract(pcFind, pcInsert, pcLink)
char *pcFind;			/* element to find			*/
char *pcInsert;			/* put this in it's place or (char *)0	*/
char *pcLink;			/* in/out list to replace		*/
{
	register char *pcSlash, *pcColon, *pcOrig, *pcMem;
	auto char cKeep;
	static char acColon[] = ":";

	if ((char *)0 == (pcSlash = strrchr(pcFind, '/'))) {
		pcSlash = pcFind;
	} else {
		register int len;

		/* are we inserting into the same dir we were in ?
		 */
		*pcSlash = '\000';
		len = strlen(pcFind);
		if ((char *)0 != pcInsert && 0 == strncmp(pcFind, pcInsert, len) && '/' == pcInsert[len] && (char *)0 == strchr(pcInsert+len+1, '/')) {
			pcInsert += len+1;
		}
		*pcSlash++ = '/';
	}

	/* find the link in the list (correct list)
	 */
	pcColon = (char *)0;	/* shutup gcc -Wall */
	for (pcOrig = pcLink; (char *)0 != pcLink && '\000' != *pcLink; *pcColon = ':', pcLink = pcColon+1) {
		if ((char *)0 == (pcColon = strchr(pcLink, ':'))) {
			pcColon = acColon;
		}
		*pcColon = '\000';
		if ('/' == *pcLink) {
			if (0 == strcmp(pcLink, pcFind))
				break;
		} else if (0 == strcmp(pcLink, pcSlash)) {
			break;
		}
	}
	/* not found, fail
	 */
	if ((char *)0 == pcLink) {
		*pcColon = ':';
		return (char *)0;
	}

	/* remove the current link from the list
	 */
	if (pcOrig != pcLink) {
		--pcLink;
	}
	cKeep = *pcLink;
	*pcLink = '\000';

	pcMem = malloc(( (((char *)0 == pcInsert ? 0 : (1+strlen(pcInsert))) + strlen(pcOrig)+1 + strlen(pcColon+1)+1)|3)+1);
	if ((char *)0 != pcMem) {
		if ('\000' != *pcOrig) {
			(void)strcpy(pcMem, pcOrig);
		} else {
			pcMem[0] = '\000';
		}
		if ((char *)0 != pcInsert) {
			if ('\000' != pcMem[0]) {
				(void)strcat(pcMem, ":");
			}
			(void)strcat(pcMem, pcInsert);
		}
		if ('\000' != pcColon[1]) {
			if ('\000' != pcMem[0]) {
				(void)strcat(pcMem, ":");
			}
			(void)strcat(pcMem, pcColon+1);
		}
	}
	*pcLink = cKeep;
	*pcColon = ':';
	return pcMem;
}

/*
 * re-think a -H..., -S..., file /dest so it looks like the config	(ksb)
 * file says it should.  Then we can try again.  Or not.
 * return FAIL if the recursive install failed (else SUCCEED)
 * NB: we don't really fix this, we just bitch and die because the
 * invarients say that the config file traps errors *WITH OUT* fix'n
 * them.  The user should fix the Makefile/makefile/command line.  Amen.
 */
int
ReThink(pcShould, pcSource, pcDest, pcHLinks, pcSLinks)
char *pcShould;			/* what we know it should be, : or @	*/
char *pcSource;			/* the source file to install		*/
char *pcDest;			/* where we though it went		*/
char *pcHLinks;			/* the list of hard links is might be	*/
char *pcSLinks;			/* the list of soft links is might be	*/
{
	register char *pcNewList;
	auto char *pcType;

	if (':' == pcShould[0]) {
		pcType = "hard";
		pcNewList = Xtract(pcShould+1, pcDest, pcHLinks);
	} else {
		pcType = "symbolic";
		pcNewList = Xtract(pcShould+1, pcDest, pcSLinks);
	}

	/* we look in the wrong link list, if it is here
	 * we can kill two problems with one run, kinda
	 */
	if ((char *)0 == pcNewList) {
		register char *pcBungle, **ppcFrom, **ppcTo, *pcMem;

		if (':' == pcShould[0]) {
			pcBungle = "symbolic";
			ppcFrom = & pcSLinks;
			ppcTo = & pcHLinks;
		} else {
			pcBungle = "hard";
			ppcFrom = & pcHLinks;
			ppcTo = & pcSLinks;
		}
		pcNewList = Xtract(pcShould+1, (char *)0, *ppcFrom);
		if ((char *)0 != pcNewList) {
			fprintf(stderr, "%s: command line lists `%s\' as a %s link to `%s\'\n", progname, pcShould+1, pcBungle, pcDest);
		}
		*ppcFrom = pcNewList;
		pcMem = malloc(strlen(pcShould)+((char *)0 == *ppcTo ? 0 : strlen(*ppcTo)+7)+1);
		if ((char *)0 != pcMem) {
			if ((char *)0 != *ppcTo) {
				(void)strcpy(pcMem, *ppcTo);
				(void)strcat(pcMem, ":");
			} else {
				pcMem[0] = '\000';
			}
			(void)strcat(pcMem, pcDest);
		}
		pcNewList = pcMem;
	}

	/* suggest the correct command line
	 */
	if ((char *)0 != pcNewList) {
		fprintf(stderr, "%s: configuration specifies a prefered destination of `%s\' for `%s\'\n", progname, pcShould+1, pcDest);
		fprintf(stderr, "%s: suggest `install ...", progname);
		if (':' == pcShould[0]) {
			fprintf(stderr, " -H %s", pcNewList);
			if ((char *)0 != pcSLinks)
				fprintf(stderr, " -S %s", pcSLinks);
		} else {
			if ((char *)0 != pcHLinks)
				fprintf(stderr, " -S %s", pcHLinks);
			fprintf(stderr, " -S %s", pcNewList);
		}
		fprintf(stderr, " %s %s'\n", pcSource, pcShould+1);
		return FAIL;
	}
	fprintf(stderr, "%s: `%s\' should be a %s link to `%s\'\n", progname, pcDest, pcType, pcShould+1);
	return FAIL;
}


#define HARD	0		/* type of link to make			*/
#define SOFT	1
static char *apcLType[] = {
	"hard",
	"symbolic"
};
static char *apcLText[] = {
	"ln",
	"ln -s"
};

/*
 * build the links, no errors allowed!					(ksb)
 */
int
DoLinks(pST, pcFullFile, pcLinks, eType, pwd, grp)
struct stat *pST;		/* stat of file we were linked to	*/
char *pcFullFile;		/* name of file we were linked to	*/
char *pcLinks;			/* links to build			*/
int eType;			/* type of link to build		*/
struct passwd *pwd;		/* owner for file			*/
struct group *grp;		/* group for file			*/
{
	static char acColon[] = ":";	/* last time through loop	*/
	register char *pcColon;		/* skip through the ':' list	*/
	register char *pcLink;		/* skip through the ':' list	*/
	auto char *pcBusy;		/* used to cut up OLD name	*/
	auto char *pcBase;		/* just the base of the target	*/
	auto struct stat statb_link;	/* statbuf for stat'ing links	*/
	auto struct stat statb_p2;	/* statbuf for what link -> to	*/
	auto int fRet;			/* value to return		*/
	auto int iLen;			/* lenght of link text		*/
	auto int bBackup;		/* backup the older link	*/
	auto int bSymbolic;		/* existing link it symlink	*/
	auto char *pcMsg;		/* error message flags		*/
	auto char acDest[MAXPATHLEN+1];	/* full path to link		*/
	auto char acLink[MAXPATHLEN+1];	/* link text from readlink	*/
	auto char acOld[MAXPATHLEN+1];	/* backup of a link		*/
	auto char acBusy[MAXPATHLEN+1];	/* temp name for last link case	*/
#if defined(CONFIG)
	auto CHLIST Check;
#endif	/* we need a check list entry		*/

	fRet = SUCCEED;

	for (pcLink = pcLinks; '\000' != pcLink[0]; *pcColon = ':', pcLink = pcColon+1) {
		/* if we are building a soft link with no slashes
		 * in it we can just use a basename change -- else
		 * use the full path.  We used to stat to see if they
		 * pointed to the same directory, but that might change.
		 * (( after we make the link... *sigh* ))
		 */
		if (SOFT == eType && (char *)0 == strchr(pcLink, '/')) {
			pcBase = strrchr(pcFullFile, '/');
			if ((char *)0 == pcBase)
				Die("nil pointer");
			++pcBase;
		} else if (HARD == eType && (char *)0 != (pcBase = strrchr(pcFullFile, '/'))) {
			++pcBase;
		} else {
			pcBase = pcFullFile;
		}
		if ((char *)0 == (pcColon = strchr(pcLink, ':'))) {
			pcColon = acColon;
		}
		*pcColon = '\000';

		if (0 == strcmp(pcBase, pcLink)) {
			fprintf(stderr, "%s: will not link destination `%s\' to itself\n", progname, pcFullFile);
			continue;
		}
		MakeNames(FALSE, pcLink, ".", acDest, acOld);
#if defined(CONFIG)
		/* If the file is one we recognize trap link type
		 */
		Special(acDest, pcSpecial, & Check);
		if (Check.ffound && '.' != Check.acmode[0]) {
			if ((char *)0 == Check.pclink) {
				fprintf(stderr, "%s: %s: `%s\' should not be a link!\n", progname, pcSpecial, acDest);
				if ('\000' != Check.pcmesg[0]) {
					(void)printf("%s: %s: %s\n", progname, acDest, Check.pcmesg);
				}
				continue;
			}
			if ((':' == Check.pclink[0]) != (HARD == eType)) {
				fprintf(stderr, "%s: %s: says `%s\' should be a %s link (to %s)\n", progname, pcSpecial, acDest, apcLType[1 - eType], Check.pclink+1);
				continue;
			}
		}
#endif	/* set check list flags from config file */

		if (-1 != STAT_CALL(pcLink, &statb_link)) {
			pcMsg = NodeType(statb_link.st_mode, (char *)0);
			bBackup = FALSE;
			switch (statb_link.st_mode & S_IFMT) {
#if HAVE_SLINKS
			case S_IFLNK:	/* symbolic link */
				bSymbolic = TRUE;
				if (HARD == eType) {
					if (FALSE == fQuiet)
						(void)fprintf(stderr, "%s: existing symbolic link %s becomes a link hard\n", progname, pcLink);
					bBackup = TRUE;
				}
				iLen = readlink(pcLink, acLink, MAXPATHLEN);
				if (-1 == iLen) {
					(void)fprintf(stderr, "%s: readlink: %s: %s\n", progname, pcLink, strerror(errno));
					continue;
				}
				acLink[iLen] = '\000';
				if (-1 == stat(acLink, & statb_p2)) {
					if ((struct stat *)0 != pST || ENOENT != errno) {
						(void)fprintf(stderr, "%s: stat: %s: %s\n", progname, acLink, strerror(errno));
					}
					bBackup = TRUE;
				} else if (0 != strcmp(pcBase, acLink)) {
					if (FALSE == fQuiet)
						(void)fprintf(stderr, "%s: link `%s\' spelled `%s\', not `%s\'\n", progname, pcLink, acLink, pcBase);
					bBackup = TRUE;
				} else if (HARD != eType) {
					/* if delete is set dink it */
					if (fDelete) {
						break;
					}
					if (grp->gr_gid != statb_link.st_gid || (bHaveRoot && pwd->pw_uid != statb_link.st_uid)) {
						goto fix_symlink;
					}
					/* OK, looks good, leave it be */
					continue;
				}
				break;
#endif	/* no links to think about	*/
#if defined(S_IFIFO)
			case S_IFIFO:	/* fifo */
#endif	/* no fifos */
#if defined(S_IFSOCK)
			case S_IFSOCK:	/* socket */
#endif	/* no sockets */
#if defined(S_IFDOOR)
			case S_IFDOOR:	/* Solaris door */
#endif	/* no doors */
			case S_IFDIR:	/* directory */
			case S_IFCHR:	/* character special */
			case S_IFBLK:	/* block special */
				(void)fprintf(stderr, "%s: %s link %s is a %s, fail\n", progname, apcLType[eType], pcLink, pcMsg);
				fRet = FAIL;
				continue;

			case 0:
			case S_IFREG:	/* regular */
				bSymbolic = FALSE;
				if ((struct stat *)0 != pST && (pST->st_dev != statb_link.st_dev || pST->st_ino != statb_link.st_ino)) {
					(void)fprintf(stderr, "%s: existing hard link %s doesn\'t point to old file\n", progname, pcLink);
					bBackup = TRUE;
				}
#if HAVE_SLINKS
				if (SOFT == eType) {
					if (FALSE == fQuiet)
						(void)fprintf(stderr, "%s: supposed symbolic link %s was a hard link\n", progname, pcLink);
					/* go on and remove it */
					bBackup = TRUE;
				}
#endif
				break;

			default:
				(void)fprintf(stderr, "%s: unrecognized file type on %s\n", progname, pcLink);
				exit(EX_UNAVAILABLE);
			}
			if (bBackup && FAIL == DoBackup(bSymbolic, pcLink, acOld, acDest, (char *)0)) {
				(void)fprintf(stderr, "%s: backup failed for link `%s\', skipped\n", progname, pcLink);
				continue;
			}
			if (fTrace) {
				(void)printf("%s: rm -f %s\n", progname, pcLink);
			} else if (-1 != unlink(pcLink)) {
				/* OK */;
			} else if (ETXTBSY != errno) {
				(void)fprintf(stderr, "%s: unlink: %s: %s\n", progname, pcLink, strerror(errno));
				continue;
			} else {
				/* might be the last link to a running binary
				 * link to a bogus name and try again...
				 */
				pcBusy = strrchr(acOld, '/');
				if ((char *)0 == pcBusy) {
					Die("nil pointer");
				}
				*pcBusy = '\000';
				(void)sprintf(acBusy, "%s/%s", acOld, TMPBOGUS);
				*pcBusy = '/';
				MkOld(acBusy);
				Mytemp(acBusy);
				if (-1 == Rename(pcLink, acBusy, fVerbose ? "moving busy link" : (char *)0)) {
					continue;
				}
			}
		} else if (ENOENT != errno) {
			(void)fprintf(stderr, "%s: stat: %s: %s\n", progname, pcLink, strerror(errno));
			fRet = FAIL;
			continue;
		}
		if (FALSE != fDelete) {
			continue;
		}
		if (fTrace) {
			(void)printf("%s: %s %s %s\n", progname, apcLText[eType], pcBase, pcLink);
		} else if (HARD == eType) {
			if (-1 == link(pcBase, pcLink)) {
				(void)fprintf(stderr, "%s: link: %s to %s: %s\n", progname, pcBase, pcLink, strerror(errno));
				fRet = FAIL;
			}
		} else
#if HAVE_SLINKS
		{
			if (-1 == symlink(pcBase, pcLink)) {
				(void)fprintf(stderr, "%s: symlink: %s to %s: %s\n", progname, pcBase, pcLink, strerror(errno));
				fRet = FAIL;
			}
fix_symlink:

#if HAVE_LCHOWN
			/* lchown -- modes on a symbolic link don't matter so
			 * we are going to ignore this (not a nonzero exit),
			 * unless it shows something other than a perm error.
			 */
			if (-1 == lchown(pcLink, (FALSE != bHaveRoot ? pwd->pw_uid : -1), grp->gr_gid) && errno != EPERM) {
				fprintf(stderr, "%s: lchown: %s: %s\n", progname, pcLink, strerror(errno));
				/* still don't fail the install */
			}
#else
			/* goto target statement */;
#endif /* lchown or not */
		}
#else /* no symbolc links from command-line (under -S) */
		{
fix_symlink:
			(void)fprintf(stderr, "%s: no support for symbolic links included\n", progname);
			fRet = FAIL;
		}
#endif /* include symbolic link support or not */
		if (FALSE != fVerbose) {
			(void)RunCmd(acLs, acLsArgs, pcLink);
		}
	}
	return fRet;
}

/*
 * start a links process for the -S or -H options			(ksb)
 */
int
LaunchLinks(pST, pcDestPath, pcHLink, pcSLink, mMode, pwd, grp)
char *pcDestPath;
struct stat *pST;		/* old file, if one exists	*/
char *pcHLink;
char *pcSLink;
int mMode;
struct group *grp;
struct passwd *pwd;
{
	static char acDot[] = ".";
	static char acSlash[] = "/";
	register int iChild;	/* child we forked, for wait	*/
	register char *pcDir;	/* change to dir		*/
	register char *pcLash;	/* last slash in file path	*/

	/* We fork a child to do the linking because we have to
	 * chdir and umask and stuff.  We may not be able to chdir
	 * back to where we are (worst case).
	 */
	(void)fflush(stdout);
	(void)fflush(stderr);
	switch (iChild = fork()) {
	case -1:	/* system error				*/
		(void)fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	default:	/* parrent, wait around a while		*/
		return 0 != MyWait(iChild);
	case 0:
		break;
	}

	/* Child, go do linking, by
	 * cee dee to the destination directory,
	 * build the links
	 */
	pcDir = pcDestPath;
	if ((char *)0 == (pcLash = strrchr(pcDir, '/'))) {
		pcDir = acDot;
	} else if (pcDestPath != pcLash) {
		*pcLash = '\000';
	} else {
		pcDir = acSlash;
	}
	if (fTrace) {
		(void)printf("%s: ( cd %s\n", progname, pcDir);
	}
	/* We really want to cd, even while just tracing
	 */
	if (-1 == chdir(pcDir)) {
		(void)fprintf(stderr, "%s: chdir: %s: %s\n", progname, pcDir, strerror(errno));
		exit(EX_DATAERR);
	}
	if (pcLash == acDot) {
		pcLash = pcDestPath;
	} else if (pcLash == acSlash) {
		pcLash = pcDestPath+1;
	} else {
		*pcLash++ = '/';
	}

	/* we are in the correct directory,
	 * now we must build the links
	 */
	if ((char *)0 != pcHLink) {
		if (FAIL == DoLinks(pST, pcDestPath, pcHLink, HARD, pwd, grp))
			exit(EX_NOPERM);
	}
#if HAVE_SLINKS
	/* symbolic links must be made with the correct umask
	 * we cannot chmod them
	 */
	if ((char *)0 != pcSLink) {
		if (fTrace) {
			(void)printf("%s: umask %03o\n", progname, (~mMode) & 0777);
		}
		(void)umask((~mMode) & 0777);

		if (FAIL == DoLinks(pST, pcDestPath, pcSLink, SOFT, pwd, grp))
			exit(EX_NOPERM);
	}
#endif	/* symbolic links */
	if (fTrace) {
		(void)printf("%s: )\n", progname);
	}
	exit(EX_OK);
}


#if defined(INST_FACILITY)
static char acFCreated[] = "file `%s\' created %s.%s(%04o) by %s\n";
static char acFUpdated[] = "file `%s\' updated %s.%s(%04o) by %s\n";
static char acFRemoved[] = "file `%s\' removed %s.%s(%04o) by %s\n";
#endif

static char *apcFrom[] = {		/* where did we get data from	*/
#define FROM_CMD	0
	"given on the command line",
#define FROM_OLD	1
	"from the previously installed file",
#define FROM_DEF	2
	"from the compiled in defaults",
#define FROM_SRC	3
	"from the source file",
#define FROM_DIR	4
	"from the destination directory"
};

/*
 * backs up the current file and installs a new one			(ksb)
 *
 * if fDelete is set we install a message and remove it,
 * in this case our caller passes "-" as the file to install.
 */
int
Install(pcFilePath, pcDestPath, pcHLink, pcSLink)
char *pcFilePath; 	/* the file to be installed			*/
char *pcDestPath;	/* the destination directory or file		*/
char *pcHLink;		/* hard links to make				*/
char *pcSLink;		/* symlinks to make				*/
{
	auto int iOFrom;		/* owner from which source	*/
	auto int iGFrom;		/* group from which source	*/
	auto int iMFrom;		/* mode from which source	*/
	auto int mMode, mOptMode;	/* mode to install with		*/
	auto int bLocalCopy;		/* should we copy or rename	*/
	auto int bDestExists;		/* destination file exists?	*/
	auto int bWasLink;		/* was the file a symlink?	*/
	auto int bBackup;		/* we're backing up this file	*/
	auto int bCollide;		/* cd OLD; install foo .. (fix)	*/
	auto int bStdin;		/* copy ``-'' (stdin) to dest	*/
	auto char *pcSlash;		/* last slash in $DEST/OLD/foo	*/
	auto struct passwd *pwd;	/* /etc/passwd info		*/
	auto struct group *grp;		/* /etc/group info		*/
	auto struct stat statb_file;	/* stat(2) the file to install	*/
	auto struct stat statb_dest;	/* stat(2) the destination	*/
	auto char *pcMsg;		/* flags for error message	*/
	auto char acDestPath[MAXPATHLEN+1];/* pathname of file to install*/
	auto char acBackPath[MAXPATHLEN+1];/* pathname of backup file	*/
	auto char acNewBack[MAXPATHLEN+1];/* where DoBackup put OLD/foo	*/
	auto char acTempPath[MAXPATHLEN+1];/* pathname for copy & link	*/
	auto char acBusyPath[MAXPATHLEN+1];/* pathname for busy link	*/
#if defined(CONFIG)
	auto CHLIST Check;
#endif	/* we need a check list entry		*/
#if defined(INST_FACILITY)
	auto char *pcLogStat;
	auto char acLogBuf[MAXLOGLINE];
#endif	/* we should syslog changes		*/

	if ((char *)0 == pcFilePath) {
		Die("nil filepath");
	}

	/* We always assume we'll back up the file unless we were
	 * told not to.  We may find out later there isn't anything
	 * to back up...
	 */
	acNewBack[0] = '\000';
	bBackup = (Destroy == FALSE) ? TRUE : FALSE;
	bStdin = '-' == pcFilePath[0] && '\000' == pcFilePath[1];

	/* Figure out the final destination name of the file to install
	 */
	MakeNames(bStdin, pcFilePath, pcDestPath, acDestPath, acBackPath);
	if ((char *)0 == (pcSlash = strrchr(acBackPath, '/'))) {
		Die("nil from strrchr");
	}

	/* if we are trying to deinstall a file, built a bogus one to
	 * install in it's place, then remove the bogus one.  This temp
	 * file is treated as `stdin' to the real install process.
	 */
	if (fDelete) {
		register FILE *fpRm;
		auto int afd[2];

		if (-1 == pipe(afd)) {
			fprintf(stderr, "%s: pipe: %s\n", progname, strerror(errno));
			return FAIL;
		}
		fflush(stdout);
		fflush(stderr);
		switch (fork()) {
		case -1:
			fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
			return FAIL;
		default:		/* parent: install rm script	*/
			close(0);
			dup(afd[0]);
			close(afd[0]);
			close(afd[1]);
			break;
		case 0:			/* child: output message	*/
			close(afd[0]);
			if ((FILE *)0 == (fpRm = fdopen(afd[1], "w"))) {
				Die("fdopen");
			}
			(void)fprintf(fpRm, "#!/bin/cat\nThis file, %s, is being removed (by %s)\n", acDestPath, pcGuilty);
			(void)fflush(fpRm);
			(void)fclose(fpRm);
			exit(EX_OK);
		}
	}
	if (bStdin) {
		if (-1 == fstat(fileno(stdin), &statb_file)) {
			(void)fprintf(stderr, "%s: fstat: stdin: %s\n", progname, strerror(errno));
			return FAIL;
		}
	} else if (-1 == STAT_CALL(pcFilePath, &statb_file)) {
		(void)fprintf(stderr, "%s: stat: %s: %s\n", progname, pcFilePath, strerror(errno));
		return FAIL;
	}

	bWasLink = FALSE;
	bLocalCopy = Copy || bStdin;
	pcMsg = NodeType(statb_file.st_mode, (char *)0);
	if (!bStdin) switch (statb_file.st_mode & S_IFMT) {
#if HAVE_SLINKS
	case S_IFLNK:	/* symbolic link */
#if SLINKOK
		if (FALSE == fQuiet)
			(void)fprintf(stderr, "%s: source `%s\' is a symbolic link, copying as plain file\n", progname, pcFilePath);
		break;
#endif	/* think about links */
#endif	/* no links to think about */
#if defined(S_IFIFO)
	case S_IFIFO:	/* fifo */
#endif	/* no fifos */
#if defined(S_IFSOCK)
	case S_IFSOCK:	/* socket */
#endif	/* no sockets */
#if defined(S_IFDOOR)
	case S_IFDOOR:	/* Solaris door */
#endif	/* no doors */
	case S_IFDIR:	/* directory */
		(void)fprintf(stderr, "%s: source `%s\' is a %s, fail\n", progname, pcFilePath, pcMsg);
		return FAIL;

	case S_IFCHR:	/* character special */
	case S_IFBLK:	/* block special */
		/* always copy special files, don't remove them.
		 */
		if (FALSE != fVerbose && FALSE == bLocalCopy && FALSE == fQuiet) {
			(void)printf("%s: copying special file %s\n", progname, pcFilePath);
		}
		bLocalCopy = TRUE;
		break;

	case 0:
	case S_IFREG:	/* regular/plain file */
		break;

	default:
		(void)fprintf(stderr, "%s: unrecognized file type on %s\n", progname, pcFilePath);
		return FAIL;
	}

	/* There is a bad case here, the user is in $DEST/OLD trying to
	 *	$ install foo ..
	 * (`casue $DEST/foo is hosed).  We should move
	 * $DEST/OLD/foo to a foo$$ to make room for the backup, then
	 * link $DEST/foo to $DEST/OLD/foo and copy the broken one back
	 * into place!  Eak.  We warn the user about this case and fix it.
	 */
	if (-1 != stat(acBackPath, &statb_dest) &&
	    statb_file.st_dev == statb_dest.st_dev &&
	    statb_file.st_ino == statb_dest.st_ino) {
		if (FALSE == fQuiet)
			(void)fprintf(stderr, "%s: source `%s\' will be renamed before installation\n", progname, acBackPath);
		bCollide = TRUE;
	} else {
		bCollide = FALSE;
	}

#if defined(CONFIG)
	/* If the file is one we recognize trap bogus modes/strip
	 */
	Special(acDestPath, pcSpecial, & Check);
#endif	/* set check list flags from config file */

	/* Make sure we don't install a file on top of itself.  If the stat()
	 * fails then it doesn't exist and we're ok.  Otherwise, if the device
	 * and inode numbers are the same we gripe and die.
	 *
	 * See if there's anything to back up (i.e., an existing file with
	 * the same name as acDestPath).  If this is an initial installation
	 * there won't be anything to back up, but that isn't an error, so we
	 * just note it.
	 *
	 * If they said "-D" (destroy current binary) there's no sense in
	 * complaining that there's nothing to back up...
	 */
	if (-1 != STAT_CALL(acDestPath, &statb_dest)) {
		bDestExists = TRUE;
		/* Check and see if source and dest are the
		 * same file, or there are extra links to the
		 * existing file and report that (after file type)
		 */
		if (statb_file.st_dev == statb_dest.st_dev
			&& statb_file.st_ino == statb_dest.st_ino) {
			(void)fprintf(stderr, "%s: will not move %s onto itself (%s)\n", progname, pcFilePath, acDestPath);
			exit(EX_DATAERR);
		}

		/* Here we take note of funny file types, we don't want the
		 * destination to be a funny file (because people shouldn't
		 * live like that -- ksb)
		 */
		pcMsg = NodeType(statb_dest.st_mode, (char *)0);
		switch (statb_dest.st_mode & S_IFMT) {
#if HAVE_SLINKS
		case S_IFLNK:	/* symbolic link */
			if (fDelete) {
				/* we don't complain here, just mv it to OLD
				 */
				bWasLink = TRUE;
				bBackup = TRUE;
				break;
			}
#if DLINKOK
			if (FALSE == fQuiet)
				(void)fprintf(stderr, "%s: destination %s was a symbolic link\n", progname, acDestPath);
			bWasLink = TRUE;
			/* for backup of all links, we might be wrong
			 * in even installing it!
			 */
			bBackup = TRUE;
			break;
#endif	/* think about links */
#endif	/* no links to think about */
#if defined(S_IFIFO)
		case S_IFIFO:	/* fifo */
#endif	/* no fifos */
#if defined(S_IFSOCK)
		case S_IFSOCK:	/* socket */
#endif	/* no sockets */
#if defined(S_IFDOOR)
		case S_IFDOOR:	/* Solaris door */
#endif	/* no doors */
		case S_IFDIR:	/* directory */
		case S_IFCHR:	/* character special */
		case S_IFBLK:	/* block special */
			(void)fprintf(stderr, "%s: destination `%s\' is a %s, fail\n", progname, acDestPath, pcMsg);
			return FAIL;

		case 0:
		case S_IFREG:	/* regular */
			break;

		default:
			(void)fprintf(stderr, "%s: unrecognized file type on `%s\'\n", progname, acDestPath);
			return FAIL;
		}
#if defined(INST_FACILITY)
		pcLogStat = acFUpdated;
#endif	/* we should syslog changes		*/
		if (fOnlyNew) {
			(void)fprintf(stderr, "%s: target `%s\' exists, should be a new installation (-N specified)\n", progname, acDestPath);
			return FAIL;
		}
	} else if (ENOENT != errno) {
		(void)fprintf(stderr, "%s: stat: %s: %s\n", progname, acDestPath, strerror(errno));
		exit(EX_OSERR);
	} else if (fDelete) {
		(void)fprintf(stderr, "%s: no `%s\' to remove\n", progname, acDestPath);
		return FAIL;
	} else {
		bDestExists = FALSE;
		bBackup = FALSE;
		if (FALSE == fQuiet) {
			(void)fprintf(stderr, "%s: no `%s\' to %s\n", progname, acDestPath, Destroy != FALSE ? "overwrite" : "back up");
		}
		/* we need to set statb_dest.st_dev so we know what
		 * device the destination will be on, so we can force
		 * a copy rather than a rename
		 * (we might also copy its mode/group/owner if no defaults)
		 */
		*pcSlash = '\000';
		if (-1 == stat(acBackPath, &statb_dest)) {
			register char *pcHelp, *pcCheck;
			if ((char *)0 == (pcHelp = strrchr(acDestPath, '/'))) {
				Die("nil pointer");
			}
			pcCheck = acDestPath;
			if (acDestPath == pcHelp) {
				pcCheck = "/";
			}
			*pcHelp = '\000';
			if (-1 == stat(pcCheck, &statb_dest)) {
				fprintf(stderr, "%s: stat: %s: %s\n", progname, pcCheck, strerror(errno));
				exit(EX_USAGE);
			}
			*pcHelp = '/';
		}
		*pcSlash = '/';
#if defined(INST_FACILITY)
		pcLogStat = acFCreated;
#endif	/* we should syslog changes		*/
	}
#if defined(INST_FACILITY)
	if (FALSE != fDelete) {
		pcLogStat = acFRemoved;
	}
#endif	/* we should syslog changes		*/

	/* if they've specified an owner, group or mode we take it,
	 * if the destination file already exists use that,
	 * otherwise, use a reasonable default.
	 */
	(void)setpwent();
	if ((char *)0 != Owner) {
		if ((struct passwd *)0 == (pwd = getpwnam(Owner))) {
			(void)fprintf(stderr, "%s: no passwd entry for %s\n", progname, Owner);
			exit(EX_USAGE);
		}
		if (FALSE == bHaveRoot && pwd->pw_uid != geteuid()) {
			(void)fprintf(stderr, "%s: effective uid cannot make destination owned by %s (%d != %d)\n", progname, Owner, geteuid(), pwd->pw_uid);
			exit(EX_CANTCREAT);
		}
		iOFrom = FROM_CMD;
	} else if (bDestExists != FALSE) {
		if ((struct passwd *)0 == (pwd = getpwuid((int) statb_dest.st_uid))) {
			(void)fprintf(stderr, "%s: destination owner %d doesn\'t exist\n", progname, statb_dest.st_uid);
			exit(EX_NOUSER);
		}
		iOFrom = FROM_OLD;
	} else if (bHaveRoot) {
		if ((char *)0 != DEFOWNER) {
			if ((struct passwd *)0 == (pwd = getpwnam(DEFOWNER))) {
				(void)fprintf(stderr, "%s: default owner `%s\' doesn\'t exist\n", progname, DEFOWNER);
				exit(EX_NOUSER);
			}
			iOFrom = FROM_DEF;
		} else {
			if ((struct passwd *)0 == (pwd = getpwuid((int) statb_dest.st_uid))) {
				(void)fprintf(stderr, "%s: destination directory owner %d doesn\'t exist\n", progname, statb_dest.st_uid);
				exit(EX_NOUSER);
			}
			iOFrom = FROM_DIR;
		}
	} else if ((struct passwd *)0 == (pwd = getpwuid((int) statb_file.st_uid))) {
		(void)fprintf(stderr, "%s: no passwd entry for source owner %d\n", progname, statb_file.st_uid);
		exit(EX_NOUSER);
		/*NOTREACHED*/
	} else {
		iOFrom = FROM_SRC;
	}
	pwd = savepwent(pwd);
	(void)endpwent();

	/* take specified group or existing destination file's group
	 * if destination exists, duplicate its group
	 * else leave group the same as the file to install
	 */
	(void)setgrent();
	if ((char *)0 != Group) {
		grp = getgrnam(Group);
		if ((struct group *)0 == grp) {
			(void)fprintf(stderr, "%s: no group entry for %s\n", progname, Group);
			exit(EX_NOUSER);
		}
		iGFrom = FROM_CMD;
	} else if (bDestExists != FALSE) {
		grp = getgrgid((int) statb_dest.st_gid);
		if ((struct group *)0 == grp) {
			(void)fprintf(stderr, "%s: no group entry for destination group %d\n", progname, statb_dest.st_gid);
			exit(EX_NOUSER);
		}
		iGFrom = FROM_OLD;
	} else if (bHaveRoot) {
		if ((char *)0 != DEFGROUP) {
			if ((struct group *)0 == (grp = getgrnam(DEFGROUP))) {
				(void)fprintf(stderr, "%s: no group entry for default group %s\n", progname, DEFGROUP);
				exit(EX_NOUSER);
			}
			iGFrom = FROM_DEF;
		} else {
			grp = getgrgid((int) statb_dest.st_gid);
			if ((struct group *)0 == grp) {
				(void)fprintf(stderr, "%s: no group entry for destination directory group %d\n", progname, statb_dest.st_gid);
				exit(EX_NOUSER);
			}
			iGFrom = FROM_DIR;
		}
	} else if ((struct group *)0 == (grp = getgrgid((int) getegid()))) {
		(void)fprintf(stderr, "%s: no group entry effective group %d\n", progname, getegid());
		exit(EX_NOUSER);
		/*NOTREACHED*/
	} else {
		iGFrom = FROM_SRC;
	}
	grp = savegrent(grp);
	(void)endgrent();

	/* take specified mode, use destination mode, or default
	 * (we never take the source modes, user uses root defaults)
	 */
	if ((char *)0 != Mode) {
		CvtMode(Mode, & mMode, & mOptMode);
		iMFrom = FROM_CMD;
		/* here is some magic, if the user is a real install
		 * wiz. he may have left us to tune the final modes...
		 */
		if (0 != mOptMode) {
			mMode |= statb_dest.st_mode & mOptMode;
		}
	} else if (FALSE != bDestExists) {
#if DLINKOK
		if (FALSE != bWasLink && FALSE == fDelete) {
			(void)fprintf(stderr, "%s: mode on existing symbolic link `%s\' cannot be used as install mode\n", progname, acDestPath);
			return FAIL;
		}
#endif
		mMode = statb_dest.st_mode &~ S_IFMT;
		iMFrom = FROM_OLD;
	} else if ((char *)0 != DEFMODE) {
		CvtMode(DEFMODE, & mMode, & mOptMode);
		iMFrom = FROM_DEF;
		if (0 != mOptMode) {
			mMode |= statb_dest.st_mode & mOptMode;
		}
	} else {
		mMode = statb_dest.st_mode &~ (S_IFMT|S_ISUID|S_ISGID|S_ISVTX);
		iMFrom = FROM_DIR;
	}

	/* if a file is already installed warn of potential differences in
	 * mode bits, owner, or group
	 */
	if (bDestExists != FALSE) {
		if (pwd->pw_uid != statb_dest.st_uid && FALSE == fQuiet) {
			(void)fprintf(stderr, "%s: `%s\' owner mismatch (%d != %d)\n", progname, acDestPath, pwd->pw_uid, statb_dest.st_uid);
		}

		if (grp->gr_gid != statb_dest.st_gid && FALSE == fQuiet) {
			(void)fprintf(stderr, "%s: `%s\' group mismatch (%d != %d)\n", progname, acDestPath, grp->gr_gid, statb_dest.st_gid);
		}

		if (PERM_RWX(mMode) != PERM_RWX(statb_dest.st_mode) && FALSE == fQuiet) {
			(void)fprintf(stderr, "%s: `%s\' mode mismatch (%04o != %04o)\n", progname, acDestPath, mMode, statb_dest.st_mode &~ S_IFMT);
		}
		if ((S_ISUID & mMode) != (S_ISUID & statb_dest.st_mode)) {
			(void)fprintf(stderr, "%s: `%s\' setuid bit changed, now %s\n", progname, acDestPath, apcOO[0 != (S_ISUID & mMode)]);
		}
		if ((S_ISGID & mMode) != (S_ISGID & statb_dest.st_mode)) {
			(void)fprintf(stderr, "%s: `%s\' setgid bit changed, now %s\n", progname, acDestPath, apcOO[0 != (S_ISGID & mMode)]);
		}
		if ((S_ISVTX & mMode) != (S_ISVTX & statb_dest.st_mode)) {
			(void)fprintf(stderr, "%s: `%s\' sticky bit changed, now %s\n", progname, acDestPath, apcOO[0 != (S_ISVTX & mMode)]);
		}
	}

	/* here we really want to check against the default stuff if the
	 * modes (owner, group, mode) of the file are not what the default
	 * would give, choke a little (and the modes are not from the cmd line)
	 * but this would be too much work; let instck -G save us
	 * in the broken cases.
	 */

#if defined(CONFIG)
	/* time to check the group and owner against our check list
	 */
	if (Check.ffound && '.' != Check.acmode[0]) {
		register int bFail = FALSE;

		if (!fDelete && (char *)0 != Check.pclink) {
			static int fStopIt = 0;

			if (fStopIt) {
				return FAIL;
			}
			++fStopIt;
			/* Like:  ":/bin/passwd", "a.out",
			 * "/bin/chfn", "chfn:passwd", ""
			 */
			bFail = ReThink(Check.pclink, pcFilePath, acDestPath, pcHLink , pcSLink);
			--fStopIt;
			return bFail;
		}
		if ('*' == Check.acowner[0]) {
			/* OK no check */;
		} else if (Check.fbangowner) {
			if (Check.uid == pwd->pw_uid) {
				(void)fprintf(stderr, "%s: `%s\' should not have owner %s\n", progname, acDestPath, Check.acowner);
				bFail = TRUE;
			}
		} else if (Check.uid != pwd->pw_uid) {
			(void)fprintf(stderr, "%s: `%s\' should have owner %s (not %s)\n", progname, acDestPath, Check.acowner, pwd->pw_name);
			bFail = TRUE;
		}

		if ('*' == Check.acgroup[0]) {
			/*OK */;
		} else if (Check.fbanggroup) {
			if (Check.gid == grp->gr_gid) {
				(void)fprintf(stderr, "%s: `%s\' should not have group %s\n", progname, acDestPath, Check.acgroup);
				bFail = TRUE;
			}
		} else if (Check.gid != grp->gr_gid) {
			(void)fprintf(stderr, "%s: `%s\' should have group %s (not %s)\n", progname, acDestPath, Check.acgroup, grp->gr_name);
			bFail = TRUE;
		}

		switch (Check.acmode[0]) {
		case '?':
		case '*':
			/*OK*/;
		case '-':
			if (Check.mmust != (mMode & Check.mmust)) {
				(void)fprintf(stderr, "%s: `%s\' mode %04o doesn\'t have bits to match %s (%04o)\n", progname, acDestPath, mMode, Check.acmode, Check.mmust);
				bFail = TRUE;
			} else if (0 != (mMode &~ (Check.mmust|Check.moptional))) {
				(void)fprintf(stderr, "%s: `%s\' mode %04o has too many bits to match %s (%04o)\n", progname, acDestPath, mMode, Check.acmode, Check.mmust|Check.moptional);
				bFail = TRUE;
			}
			if (0 != (S_ISUID & mMode) ? 0 == (S_ISUID & (Check.mmust|Check.moptional)) : 0 != (S_ISUID & Check.mmust)) {
				(void)fprintf(stderr, "%s: `%s\' setuid bit must be %s\n", progname, acDestPath, apcOO[0 == (S_ISUID & mMode)]);
				bFail = TRUE;
			}
			if (0 != (S_ISGID & mMode) ? 0 == (S_ISGID & (Check.mmust|Check.moptional)) : 0 != (S_ISGID & Check.mmust)) {
				(void)fprintf(stderr, "%s: `%s\' setgid bit must be %s\n", progname, acDestPath, apcOO[0 == (S_ISGID & mMode)]);
				bFail = TRUE;
			}
			if (0 != (S_ISVTX & mMode) ? 0 == (S_ISVTX & (Check.mmust|Check.moptional)) : 0 != (S_ISVTX & Check.mmust)) {
				(void)fprintf(stderr, "%s: `%s\' sticky bit must be %s\n", progname, acDestPath, apcOO[0 == (S_ISVTX & mMode)]);
				bFail = TRUE;
			}
			break;
		default:
			(void)fprintf(stderr, "%s: `%s\' must be a %s\n", progname, acDestPath, NodeType(Check.mtype, (char *)0));
			bFail = TRUE;
			break;
		case '!':
			if (fDelete) {
				break;
			}
			/* fall through */
		case '~':
			(void)fprintf(stderr, "%s: `%s\' should not be %s", progname, acDestPath, fDelete ? "removed" : "installed");
			if ((char *)0 != Check.pcmesg && '\000' != Check.pcmesg[0])
				(void)fprintf(stderr, ", %s", Check.pcmesg);
			(void)fputc('\n', stderr);
			bFail = TRUE;
			break;
		}

		switch (Check.chstrip) {
		case CF_ANY:
			break;
		case CF_STRIP:
			if (!fDelete && !Strip) {
				(void)fprintf(stderr, "%s: `%s\' must be strip\'ed (use -s)\n", progname, acDestPath);
				bFail = TRUE;
			}
			break;
		case CF_RANLIB:
			if (!fDelete && !Ranlib) {
				(void)fprintf(stderr, "%s: `%s\' must be ranlib\'ed (use -l)\n", progname, acDestPath);
				bFail = TRUE;
			}
			break;
		case CF_NONE:
			if (!fDelete && (Strip || Ranlib)) {
				(void)fprintf(stderr, "%s: `%s\' must not be strip\'ed or ranlib\'ed (do not use -l or -s)\n", progname, acDestPath);
				bFail = TRUE;
			}
			break;
		}
#if HAVE_CHFLAGS
		{
		auto unsigned long uFFMustSet, uFFMustClr;
		auto char *pcRef;

		if ((char *)0 == (pcRef = Check.pcflags)) {
			/* OK */
		} else if (0 != strtofflags(&pcRef, &uFFMustSet, & uFFMustClr)) {
			fprintf(stderr, "%s: %s:%d: strtofflags: %s: unknown flag\n", progname, Check.pcspecial, Check.iline, pcRef);
			return FAIL;
		} else if (FAIL == CheckFFlags(acDestPath, uFFMustSet, uFFMustClr, uFFSet, uFFClr)) {
			bFail = TRUE;
		}
		}
#endif
		if (FALSE != bFail) {
			(void)fprintf(stderr, "%s: `%s\' failed check in `%s\'\n", progname, acDestPath, pcSpecial);
			return FAIL;
		}
		if (fDelete) {
			(void)printf("%s: %s(%d) %s pattern `%s\'\n", progname, Check.pcspecial, Check.iline, 0 == strcmp(Check.pcpat, acDestPath) ? "remove" : "check", Check.pcpat);
		}
#if defined(PARANOID)
	} else if (bHaveRoot && 0 != (mMode & (S_ISUID|S_ISGID)) && (char *)0 != pcSpecial) {
		fprintf(stderr, "%s: set%cid `%s\' ", progname, (S_ISUID & mMode) ? 'u' : 'g', acDestPath);
		if (FROM_OLD != iMFrom) {
			if ('\000' == *pcSpecial) {
				fprintf(stderr, "needs check list file\n");
			} else {
				fprintf(stderr, "not found in check list %s\n", pcSpecial);
			}
			return FAIL;
		}
		if ('\000' == *pcSpecial) {
			fprintf(stderr, "needs to be added to a check list file\n");
		} else {
			fprintf(stderr, "needs to be added to the check list %s\n", pcSpecial);
		}
#endif	/* all setuid programs must be in check	*/
	}
#endif	/* have check list entry to compare	*/

	/* if we are removing the file, don't strip/ranlib it
	 * (and drop any setuid/setgid/sticky bits for security)
	 */
	if (fDelete) {
		Ranlib = Strip = FALSE;
		mMode = PERM_RWX(mMode);
	}

	/* Mixing modes from one source with group/owner of another
	 * might be a bad idea, in fact bad enough we stop the install.
	 */
	if (0 != (S_ISUID & mMode) && iMFrom != iOFrom) {
		(void)fprintf(stderr, "%s: `%s\' will not setuid based on mode %s and owner %s\n", progname, acDestPath, apcFrom[iMFrom], apcFrom[iOFrom]);
		if (FROM_CMD == iMFrom) {
			fprintf(stderr, "%s: check mode and use `-o owner\' on the command line to verify owner\n", progname);
		} else if (FROM_CMD == iOFrom) {
			fprintf(stderr, "%s: check owner and use `-m mode\' on the command line to verify mode\n", progname);
		} else {
			fprintf(stderr, "%s: check owner and mode, use `-m mode -o owner\' on the command line to verify mode\n", progname);
		}
		return FAIL;
	}
	if (0 != (S_ISGID & mMode) && iMFrom != iGFrom) {
		(void)fprintf(stderr, "%s: `%s\' will not setgid based on mode %s and group %s\n", progname, acDestPath, apcFrom[iMFrom], apcFrom[iOFrom]);
		if (FROM_CMD == iMFrom) {
			fprintf(stderr, "%s: check mode and use `-g group\' on the command line to verify group\n", progname);
		} else if (FROM_CMD == iGFrom) {
			fprintf(stderr, "%s: check group and use `-m mode\' on the command line to verify mode\n", progname);
		} else {
			fprintf(stderr, "%s: check group and mode, use `-m mode -g group\' on the command line to verify mode\n", progname);
		}
		return FAIL;
	}

	/* Now we back up the old file if we're supposed to
	 * there might be nothing to backup because of -D
	 * but we've warned them about links and checked for funny file types
	 */
	if (bBackup != FALSE) {
		if (FAIL == DoBackup(bWasLink, acDestPath, acBackPath, acNewBack, FALSE == f1Copy && (FALSE != fVerbose || (Check.ffound && '\000' != Check.pcmesg[0])) ? "rename" : (char *)0)) {
			return FAIL;
		}
		/* see comment where bCollide is set above
		 */
		if (bCollide) {
			pcFilePath = acNewBack;
		}
	}

	/* Copy the file to the fs the dest is on, so we can just rename it.
	 */
	if (bLocalCopy != FALSE || statb_dest.st_dev != statb_file.st_dev) {
		/* cut backup at last slash, copy to buffer,
		 * concat on a Mytemp (mktemp) template
		 */
		*pcSlash = '\000';
		(void)sprintf(acTempPath, "%s/%s", acBackPath, TMPINST);
		*pcSlash = '/';
		MkOld(acTempPath);
		(void)Mytemp(acTempPath);
		if (FAIL == DoCopy(pcFilePath, acTempPath)) {
			(void)fprintf(stderr, "%s: copy of %s to %s failed\n", progname, pcFilePath, acDestPath);
			if ('\000' != acNewBack[0]) {
				(void)Rename(acNewBack, acBackPath, "return");
			}
			return FAIL;
		}
		if (FALSE == bLocalCopy) {
			if (FALSE != fTrace) {
				(void)printf("%s: rm -f %s\n", progname, pcFilePath);
			} else if (-1 != unlink(pcFilePath)) {
				/* nada */;
			} else if (ETXTBSY == errno) {
				fprintf(stderr, "%s: %s is running, not removed\n", progname, pcFilePath);
				/* carp, but succeed */
			} else {
				(void)fprintf(stderr, "%s: unlink: %s: %s\n", progname, pcFilePath, strerror(errno));
				if ('\000' != acNewBack[0]) {
					(void)Rename(acNewBack, acBackPath, "return");
				}
				return FAIL;
			}
		}
		pcFilePath = acTempPath;
	}

	/* If a file is set{u,g}id check for shell script magic number!
	 * This gives away a shell with the symbolic link setuid bug,
	 * but we don't stop the install, and I think we should.
	 * Maybe if the target user is root?
	 */
	if (0 != ((S_ISUID|S_ISGID) & mMode) && 0 != ((S_IXUSR|S_IXGRP|S_IXOTH) & mMode)) {
		register char chSet;		/* 'g' or 'u', set?id	*/
		register FILE *fpCheck;		/* file we install	*/

		chSet = (S_ISUID & mMode) ? 'u' : 'g';
		if ((FILE *)0 != (fpCheck = fopen(pcFilePath, "r"))) {
			if ('#' == getc(fpCheck) && '!' == getc(fpCheck)) {
				(void)fprintf(stderr, "%s: `%s\' is set%cid and loaded with a `#!\'\n", progname, acDestPath, chSet);
			}
			(void)fclose(fpCheck);
		} else {
			(void)fprintf(stderr, "%s: cannot check magic number on set%cid `%s\'\n", progname, chSet, acDestPath);
		}
	}

	/* Unlink the current file (which DoBackup() linked to the backup
	 * file) but only if it exists.
	 */
	if (FALSE != bDestExists) {
#if HAVE_CHFLAGS
		{
		register unsigned long ulBits;

		/* If we plan to set or have immutable, remove from previous
		 */
		ulBits = (uFFClr|UF_NOUNLINK|UF_IMMUTABLE|SF_IMMUTABLE|SF_NOUNLINK);
		/* ulBits |= no bits we _must_ remove here - ask if you like */
		ulBits &= statb_dest.st_flags;
		if ((char *)0 != pcFFlags && 0 != ulBits) {
			if (FALSE != fTrace) {
				register char *pc, *pcMem;

				(void)printf("%s: chflags no" , progname);
				for (pcMem = pc = fflagstostr(ulBits); '\000' != *pc; ++pc) {
					putchar(*pc);
					if (',' == *pc) {
						printf("no");
					}
				}
				(void)printf(" %s\n", acDestPath);
				free((void *)pcMem);
			} else if (0 != chflags(acDestPath, statb_dest.st_flags & ~ulBits)) {
				fprintf(stderr, "%s: chflags: %s: %s\n", progname, acDestPath, strerror(errno));
				return FAIL;
			}
		}
		}
#endif
		if (FALSE != fTrace) {
			(void)printf("%s: rm -f %s\n", progname, acDestPath);
		} else if (-1 != unlink(acDestPath)) {
			/* OK */;
		} else if (ETXTBSY != errno) {
			(void)fprintf(stderr, "%s: unlink: %s: %s\n", progname, acDestPath, strerror(errno));
			if ('\000' != acNewBack[0]) {
				(void)Rename(acNewBack, acBackPath, "return");
			}
			return FAIL;
		} else {
			/* might be the last link to a running binary under
			 * sys5, sigh, link to a bogus name and try again...
			 */
			*pcSlash = '\000';
			(void)sprintf(acBusyPath, "%s/%s", acBackPath, TMPBOGUS);
			*pcSlash = '/';
			MkOld(acBusyPath);
			(void)Mytemp(acBusyPath);
			if (-1 == Rename(acDestPath, acBusyPath, fVerbose ? "moving busy file" : (char *)0)) {
				if ('\000' != acNewBack[0]) {
					(void)Rename(acNewBack, acBackPath, "return");
				}
				return FAIL;
			}
		}
	}


	/* If requested, strip the new file or ranlib it.  Have to do this
	 * before ChOwnGrp and ChGroup or 2.9BSD drops the setuid bits...
	 */
	if (FALSE != Strip) {
		if (FALSE != fVerbose) {
			(void)printf("%s: %s %s\n", progname, acStrip, pcFilePath);
		}
		if (0 != RunCmd(acStrip, pcFilePath, (char *)0)) {
			(void)fprintf(stderr, "%s: `%s %s\' failed, run strip by hand\?\n", progname, acStrip, pcFilePath);
		}
	}

	if (FALSE != Ranlib) {
#if HAVE_RANLIB
		/* on the pdp11/70 this kludge made the ranlib command work
		 * I don't think we ever knew why... now I don't have an 11
		 * to find out if we still need it.  Maybe chmod changed
		 * the times on the file?
		 */
#if defined(pdp11)
		(void)ChMode(pcFilePath, mMode);
#endif
		if (FALSE != fVerbose) {
			(void)printf("%s: %s %s\n", progname, acRanlib, pcFilePath);
		}
		if (0 != RunCmd(acRanlib, pcFilePath, (char *)0)) {
			(void)fprintf(stderr, "%s: `%s %s\' failed, run ranlib by hand\?\n", progname, acRanlib, pcFilePath);
		}
#else	/* on sysV fake it, same cmd that way	*/
		if (FALSE != fVerbose) {
			(void)printf("%s: ranlib for `%s\' done by ar(1)\n", progname, pcFilePath);
		}
#endif	/* do we really run ranlib(1)		*/
	}

	/* Change ownership, group, timestamp, and mode of installed file
	 * (chmod must be done last or setuid bits are dropped under 2.9bsd)
	 */
	if (FALSE != bHaveRoot) {
		if (0 != ChOwnGrp(pcFilePath, pwd, grp)) {
			exit(EX_CANTCREAT);
		}
	} else if (0 != ChGroup(pcFilePath, grp)) {
		exit(EX_CANTCREAT);
	}
	if (FALSE != KeepTimeStamp) {
		ChTimeStamp(pcFilePath, & statb_file);
	}
	if (0 != ChMode(pcFilePath, mMode)) {
		exit(EX_CANTCREAT);
	}

	BlockSigs();

	if (FAIL == Rename(pcFilePath, acDestPath, (char *)0)) {
		/*
		 * OUCH! why can't we rename this puppy? Here we are in
		 * big trouble: can't go back in some cases, can't go
		 * forward in others.  Let the user figure it out.
		 */
		(void)fprintf(stderr, "%s: \ano currently installed file, aborting\a\n", progname);
		exit(EX_NOINPUT);
	}

	/* We have moved the file in, we are commited.
	 * We *must* complete the installation at all costs now.
	 * There is no turning back from here on; no more return FAIL.
	 */
	if ((char *)0 != pcHLink || (char *)0 != pcSLink) {
		if (LaunchLinks(bDestExists ? & statb_dest : (struct stat *)0, acDestPath, pcHLink, pcSLink, mMode, pwd, grp)) {
			(void)fprintf(stderr, "%s: links failed, finishing installation anyway\n", progname);
		}
	}

	if (FALSE != f1Copy && pcFilePath != acNewBack && '\000' != acNewBack[0]) {
		if (FALSE != fTrace) {
			(void)printf("%s: rm -f %s\n", progname, acNewBack);
		} else if (-1 != unlink(acNewBack)) {
			/* OK */;
		} else if (ETXTBSY != errno) {
			(void)fprintf(stderr, "%s: unlink: %s: %s\n", progname, acNewBack, strerror(errno));
		} else {
			/* the last link to a running binary (SYSV)
			 * move it to a bogus name
			 */
			*pcSlash = '\000';
			(void)sprintf(acBusyPath, "%s/%s", acBackPath, TMPBOGUS);
			*pcSlash = '/';
			/* the MkOld below is unneeded
			 * (we know of at least one file in there)
			 */
			MkOld(acBusyPath);
			(void)Mytemp(acBusyPath);
			if (-1 == Rename(acNewBack, acBusyPath, "moving busy backup file")) {
				(void)fprintf(stderr, "%s: rename %s to %s: %s\n", progname, acNewBack, acBusyPath, strerror(errno));
			}
		}
	}
#if HAVE_CHFLAGS
	if ((char *)0 != pcFFlags) {
		register char *pcMem;

		if ((char *)0 == (pcMem = fflagstostr(uFFSet))) {
			pcMem = strdup(pcFFlags);
		}
		if (FALSE != fTrace) {
			(void)printf("%s: chflags %s %s\n", progname, pcMem, acDestPath);
		} else if (0 != chflags(acDestPath, uFFSet)) {
			fprintf(stderr, "%s: chflags: %s: %s\n", progname, acDestPath, strerror(errno));
			return FAIL;
		}
		free((void *)pcMem);
	}
#endif
	UnBlockSigs();

	/* re-stat, we may have changed link count
	 */
	if (-1 != STAT_CALL(acBackPath, & statb_dest) && 1 != statb_dest.st_nlink) {
		if ((char *)0 != pcHLink) {
			(void)printf("%s: -H option may not have listed all the links to %s, still %u left\n", progname, acDestPath, (unsigned)(statb_dest.st_nlink-1));
		} else {
			(void)printf("%s: %s may still be installed someplace, link count is too big (%u)\n", progname, acBackPath, (unsigned)statb_dest.st_nlink);
		}
		*pcSlash = '\000';
		(void)printf("%s: use `instck -i %s' to repair link counts\n", progname, acBackPath);
		*pcSlash = '/';
	}

	/* and turn off special bits on backup -- in case installation
	 * was security related
	 */
	if (FALSE == bWasLink && FALSE != bDestExists && FALSE != bBackup && 0 != (statb_dest.st_mode & ~SAFEMASK)) {
		auto char acMode[16];	/* "rwxrwxrwx" */

		if (0 != ChMode(acBackPath, (int)statb_dest.st_mode & SAFEMASK)) {
			ModetoStr(acMode, (int)statb_dest.st_mode, 0);
			fprintf(stderr, "%s: %s: old file remains with unsafe permission bits (%s)\n", progname, acBackPath, acMode);
		}
	}

#if defined(INST_FACILITY)
	/*
	 * syslog out change if we are the superuser and we really changed
	 * something
	 */
	if (bHaveRoot && FALSE == fTrace) {
		(void)sprintf(acLogBuf, pcLogStat, acDestPath, pwd->pw_name, grp->gr_name, mMode, pcGuilty);
		syslog(LOG_INFO, acLogBuf);
	}
#endif	/* we should syslog changes		*/

#if defined(CONFIG)
	/* if the file is in a check list report installation message
	 */
	if (Check.ffound && '\000' != Check.pcmesg[0]) {
		(void)printf("%s: %s: %s\n", progname, acDestPath, Check.pcmesg);
	}
#endif	/* have check list to output a comment	*/

	/* if we are supposed to dink it, do it now, else
	 * if requested, show the results of the installation with ls(1)
	 */
	if (fDelete) {
		if (fTrace) {
			(void)printf("%s: rm -f %s\n", progname, acDestPath);
		} else if (-1 == unlink(acDestPath)) {
			fprintf(stderr, "%s: unlink: %s: %s\n", progname, acDestPath, strerror(errno));
			return FAIL;
		}
		(void)RunCmd(acLs, acLsArgs, acBackPath);
	} else if (FALSE != fVerbose) {
		if (FALSE != bBackup) {
			(void)RunCmd(acLs, acLsArgs, acBackPath);
		}
		(void)RunCmd(acLs, acLsArgs, acDestPath);
	}

	return SUCCEED;
}
