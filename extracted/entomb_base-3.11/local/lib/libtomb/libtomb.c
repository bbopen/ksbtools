/* $Id: libtomb.c,v 3.11 2008/01/07 01:19:03 ksb Exp $
 *
 * These functions get called instead of the standard library functions.
 *
 * Copyright 1988, 1992 Purdue Research Foundation
 */
char libtomb_version[] =
	"$Id: libtomb.c,v 3.11 2008/01/07 01:19:03 ksb Exp $";

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/param.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>

#include "libtomb.h"

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif


#if GLOB_ENTOMB
/*
 * this macro could be in glob.h, or not...
 */
#define SamePath(Mglob, Mfile)	_SamePath(Mglob, Mfile, 1)

/*
 * _SamePath()
 *	We want /bin/* to match every file in /bin OK.			(ksb)
 *	return 1 for ==, 0 for !=
 *	
 * N.B. Must call CompPath on pcFile first, or know it is minimal
 */
static int
_SamePath(pcGlob, pcFile, fDot)
char *pcGlob;		/* the pattern to match				*/
char *pcFile;		/* the file to match with			*/
int fDot;		/* are we at the start of pcFile, or post '/'	*/
{
	register char *pc;
	register int iLenGlob, iLenFile;
	auto int bFound, cStop;

	for (;;) { switch (*pcGlob) {
	case '*':		/* match any string			*/
		pc = ++pcGlob;
		iLenGlob = 0;
		while ('\\' != *pc && '?' != *pc && '[' != *pc && '*' != *pc && '\000' != *pc && '/' != *pc) {
			++pc, ++iLenGlob;
		}

			
		iLenFile = 0;
		while ('/' != pcFile[iLenFile] && '\000' != pcFile[iLenFile] &&
		       (!fDot || '.' != pcFile[iLenFile])) {
			++iLenFile;
			fDot = 0;
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
#endif /* glob ENTOMB variable */


/*
 * _env_set - checks to see if the environment variable is set, and 
 *	returns yes or no depending on whether we should entomb or not.
 * ENTOMB=no				# same as no:*
 * ENTOMB=no:*.o			# do not save *.o
 * ENTOMB=yes:*.[chyl]:Makefile:*,v	# save only source files and RCS files
 * ENTOMB=no,copy			# no, copy to tomb
 * ENTOMB=yes,copy			# yes, copy to tomb
 * ENTOMB=cmd(,cmd).*(:pattern).*	# in general..
 */
static int
_env_set(pcFile, piWhat2do)
char *pcFile;
int *piWhat2do;
{
	extern char *getenv();
	register char *pcEnVar;	/* set below: default is to entomb. */
	register int tRet = 1;
	register char *pcComma;
#if GLOB_ENTOMB
	register char *pcColon, *pcSlash;
#endif

	if ((char *)0 == (pcEnVar = getenv("ENTOMB"))) {
		static char _[] = DEFAULT_EV;
		pcEnVar = _;
	}

	if (NULL != pcEnVar) {
#if GLOB_ENTOMB
		if (NULL != (pcColon = strchr(pcEnVar, ':'))) {
			*pcColon = '\000';
		}
		if (NULL != (pcSlash = strrchr(pcFile, '/'))) {
			pcFile = pcSlash+1;
		}
#endif
		/* Parse the comma separated list from left to right.
		 * Items closer to the left are superceded by conflicting
		 * items to the right.
		 */
		do {
			while (' ' == *pcEnVar || '\t' == *pcEnVar) {
				++pcEnVar;
			}
			if (NULL != (pcComma = strchr(pcEnVar, ','))) {
				*pcComma = '\000';
			}
			if (0 == strcasecmp(pcEnVar, "no")) {
				tRet = 0;
			} else if (0 == strcasecmp(pcEnVar, "false")) {
				tRet = 0;
			} else if (0 == strcasecmp(pcEnVar, "yes")) {
				tRet = 1;
			} else if (0 == strcasecmp(pcEnVar, "true")) {
				tRet = 1;
			} else if (0 == strcasecmp(pcEnVar, "copy")) {
				if (COPY != *piWhat2do) {
					*piWhat2do = COPY_UNLINK;
				}
			}
			if (NULL == pcComma) {
				break;
			}
			pcEnVar = pcComma+1;
			*pcComma = ',';
		} while ('\000' != *pcEnVar);
#if GLOB_ENTOMB
		if (NULL != pcColon) {
			register int tFound;
			register char *pcNext;
			static char acTerm[] = ":";

			tFound = 0;
			while (*pcColon++ = ':', !tFound && '\000' != *pcColon) {
				/* handle yes:\:rofix:Makefile
				 * a quoted : is OK for SamePath
				 */
				pcNext = pcColon;
				while (NULL != (pcNext = strchr(pcNext, ':'))) {
					if ('\\' != pcNext[-1]) {
						break;
					}
					++pcNext;
				}
				if (NULL == pcNext) {
					pcNext = acTerm;
				}
				*pcNext = '\000';
				if (SamePath(pcColon, pcFile)) {
					tFound = 1;
				}
				pcColon = pcNext;
			}
			if (! tFound) {
				tRet = !tRet;
			}
		}
#endif /* glob ENTOMB variable */
	}

	return tRet;
}


/* entomb -- try to decide if we should entomb this file, and		(ksb)
 * 	invoke the "entomb" program if we should.  Return:
 *
 *	NOT_ENTOMBED	if we tried to entomb and there was an error
 *	0		if we entombed and there was no error
 *	NO_TOMB		there was no tomb on the given filesystem
 *
 * These return codes may sometimes be ignored.
 */
int
_entomb(iWhat2do, pcPath)
int iWhat2do;
char *pcPath;
{
	static dev_t dev_tLast;		/* last device we checked	*/
	static int iRet = 0;		/* result of last entomb	*/
	static time_t wWhen;		/* when we saw no tomb		*/
	static char *apcEntomb[] = {
		/* path  argv0    [-C|-c]     --        $target     NULL */
		ENTOMB, "entomb", (char *)0, (char *)0, (char *)0, (char *)0
	};
	auto struct stat statBuf;
	register int iPid, i;
#if HAVE_UNIONWAIT
	auto union wait w;
#else
	auto int w;
#endif

	/* if the file is in the env eception list don't even stat it
	 */
	if (! _env_set(pcPath, &iWhat2do)) {
		return NOT_ENTOMBED;
	}

	/* if the file doesn't exist, or something of this nature,
	 * we don't need to fork.  We'll say it was not entomb'd.
	 */
	if (-1 == STAT_CALL(pcPath, &statBuf)) {
		return NOT_ENTOMBED;
	}

	/* if the file is zero size or not a plain file, we don't entomb it
	 */
	if (0 == statBuf.st_size || S_IFREG != (statBuf.st_mode&S_IFMT)) {
		return NOT_ENTOMBED;
	}


	/* We save the device number and entomb result of the last file we
	 * entombed.  If entomb told us that there wasn't a tomb on the
	 * given device, then we can safely say there is still not a tomb
	 * on the same device.  This is not true forever, but most procs
	 * that entomb don't live very long (rm, mv, cp, purge and cpio).
	 *
	 * Every hour (60*60) we check for a new tomb or copy of "entomb"
	 * that might have a better policy.  That way daemons learn about
	 * new tombs w/o a restart.  NT would requite us to reboot the host
	 * to add a tomb, we are better than that.  -- ksb
	 */
	if (statBuf.st_dev == dev_tLast && NO_TOMB == iRet) {
		if (wWhen + (60*60) > time((time_t *)0))
			return NO_TOMB;
	}
	dev_tLast = statBuf.st_dev;
	iRet = NOT_ENTOMBED;
	(void)time(& wWhen);

	/* fork off an entomb process and ask charon to ferry the file
	 * to a crypt in the tomb for this fs.
	 */
	switch (iPid = fork()) {
		register char **ppcArgs;
	case 0:
		/* ENTOMB is a set-uid program whose name is passed via
		 * command-line option to the C compiler.  It usually
		 * live in /usr/local/libexec/entomb or lib/entomb.
		 *
		 * We eliminate the environment so that people can't screw
		 * things up with a HOSTALIASES file.
		 */
		ppcArgs = apcEntomb+2;
		if (COPY == iWhat2do) {
			*ppcArgs++ = "-c";
		} else if (COPY_UNLINK == iWhat2do) {
			*ppcArgs++ = "-C";
		}
		*ppcArgs++ = "--";	/* allow file to start with dash */
		*ppcArgs++ = pcPath;
		*ppcArgs = (char *)0;
		(void)execve(apcEntomb[0], apcEntomb+1, (char **)0);

		/* I argue that if we can't run entomb, then NO_TOMB
		 * is more accurate than NOT_ENTOMBED -- ksb
		 */
		_exit(NO_TOMB);
		/*NOTREACHED*/
	case -1:
		return NOT_ENTOMBED;
	default:
		break;
	}

#if HAVE_WAITPID
	/* Andrew J. Korty <ajk@purdue.edu>				(ajk)
	 * use waitpid to prevent messing up nfsd and KDE apps.
	 */
	i = waitpid(iPid, (int *)&w, 0);
#else /* fall back to clunky wait call */
	/* If this process has a signal handler for SIGCHLD we might
	 * mess it up a little. --ksb
	 */
	while (-1 != (i = wait(&w)) && iPid != i) {
		;
	}
#endif
	if (i != iPid) {
		return NOT_ENTOMBED;
	}
#if HAVE_UNIONWAIT
	iRet = w.w_retcode;
#else
	iRet = (w >> 8) & 0xff;
#endif
	return iRet;
}
