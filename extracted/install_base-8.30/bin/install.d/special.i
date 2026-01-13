/*
 * $Id: special.i,v 8.12 2009/12/08 22:58:14 ksb Exp $
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
 * this (long) C routine is expanded twice in instck, and only once
 * in install.  It is the one that searchs a config file.
 */

#if defined(INSTCK)
/*
 * Scan the gien dirs for pats in the check file list?  		(ksb)
 * config file format:
 *	#file	mode		owner	group	strip		[message]
 *	/unix	-r??r--r--	root	*	n[,flags]
 * `*' = any
 * `"' = last
 * `=' = the default for this col
 * `!' = not (!mode -> not exist, !group/!owner -> any but the one listed)
 */
void
DirCk(iCount, ppcDirs, pcConfig, pCL)
int iCount;		/* who many dirs to search			*/
char **ppcDirs;		/* dirs to search				*/

#else /* install */
/*
 * Is the named file in the check file list?  If so is it OK?		(ksb)
 *	FAIL is file it in list and modes are bad
 *	else SUCCESS
 * config file format:
 *	#file	mode		owner	group	strip	message
 *	/unix	-r??r--r--	root	*	n
 *	core	!r?-?--?--	*	*	n
 * `*' = any
 * `"' = last
 * `=' = the default for this col
 * `!' = not (!mode -> not exist, !group/!owner -> any but the one listed)
 */
void
Special(pcFile, pcConfig, pCL)
char *pcFile;		/* file we are searching for			*/
#endif

char *pcConfig;		/* check list file name to read			*/
CHLIST *pCL;		/* pointer to the check list to build/use	*/
{
	static FILE *fpCnf = (FILE *)0;
	static char *pcCnf = "jsmith & ksb";
	static char *pcMesg = (char *)0;
	static char acEq[] = "=", acAny[] = "*", acSame[] = "\"";
	static char acTop[32];
	static unsigned uMesgLen = 0;
	static char *pcLine = (char *)0;
	register char *pcSkip, *pcBlank;
	register int iLine;
#if defined(INSTCK)
	register int i;
	auto char *pcCurGlob;
#else
	auto char *pcLastComp;
	auto char acFPath[MAXPATHLEN+1];
#endif

	pCL->ffound = FALSE;

	if ((char *)0 == pcConfig || '\000' == pcConfig[0]) {
#if defined(INSTCK)
		(void)printf("%s: no configuration file\n", progname);
#endif
		return;
	}
	if (NULL == fpCnf || pcCnf != pcConfig) {
		if (NULL != fpCnf)
			(void)fclose(fpCnf);
		pcCnf = pcConfig;
		if (NULL == (fpCnf = fopen(pcCnf, "r"))) {
			(void)fprintf(stderr, "%s: fopen (checklist): %s: %s\n", progname, pcCnf, strerror(errno));
			exit(EX_NOINPUT);
		}
#if defined(F_SETFD)
		/* if we can we set the close-on-exec bit to be neat */
		(void)fcntl(fileno(fpCnf), F_SETFD, 1);
#endif	/* have close on exec bit to twidle	*/
	} else {
		(void)rewind(fpCnf);
	}
	iLine = 0;
	pcLine = strcpy(acTop, "* . = = =\n");
	pCL->pcspecial = pcCnf;

#if defined(INSTALL)
	if ('/' != pcFile[0]) {
		if ((char *)0 == getcwd(acFPath, sizeof(acFPath))) {
			Die("getcwd returns NULL");
		}
		(void)strcat(acFPath, "/");
		(void)strcat(acFPath, pcFile);
		pcFile = CompPath(acFPath);
	} else {
		(void)strcpy(acFPath, pcFile);	/* don't munge argument! */
		pcFile = CompPath(acFPath);
	}

	if ((char *)0 == (pcLastComp = strrchr(pcFile, '/'))) {
		Die("nil pointer");
	}
	++pcLastComp;
#endif

	/* we set the defaults:
	 * not found, plain file, mode, owner, group, no strip
	 */
	if ((char *)0 == pcMesg) {
		static char acMesg[] = "default message";

		uMesgLen = (sizeof(acMesg)|63)+1;
		if ((char *)0 == (pcMesg = malloc(uMesgLen))) {
			Die("no core");
		}
		(void)strcpy(pcMesg, acMesg);
	}
	pCL->pcmesg = pcMesg;
	pCL->pclink = (char *)0;
	pCL->acmode[0] = pCL->acmode[1] = '\000';
	pCL->pcflags = (char *)0;

	do {
		if ((char *)0 == (pcBlank = strchr(pcLine, '\n'))) {
			(void)fprintf(stderr, "%s: %s(%d) line too long\n", progname, pcCnf, iLine);
			exit(EX_DATAERR);
		}
		do {
			*pcBlank = '\000';
		} while (pcBlank != pcLine && (--pcBlank, isspace(*pcBlank)));
		for (pcSkip = pcLine; isspace(*pcSkip); ++pcSkip)
			/*empty*/;
		if ('\000' == pcSkip[0] || '#' == pcSkip[0])
			continue;

		for (pcBlank = pcSkip; '\000' != *pcBlank && ! isspace(*pcBlank); ++pcBlank)
			/*empty*/;
		while (isspace(*pcBlank))
			*pcBlank++ = '\000';
		pCL->iline = iLine;
		pCL->pcpat = pcSkip;
#if defined(INSTCK)
		pcCurGlob = pcSkip;
#else
		if (0 == iLine) {
			pCL->ffound = FALSE;
		} else if ('.' == pcSkip[0] && '\000' == pcSkip[1]) {
			/* dot matches any directory */
			auto struct stat stDotDir;
			pCL->ffound = -1 != stat(pcFile, & stDotDir) && S_IFDIR == (stDotDir.st_mode & S_IFMT);
		} else {
			pCL->ffound = SamePath(pcSkip, RJust(pcSkip, pcFile, pcLastComp));
		}
#endif

		/* mode or link */
		pcSkip = pcBlank;
		for (pcBlank = pcSkip; '\000' != *pcBlank && ! isspace(*pcBlank); ++pcBlank)
			/*empty*/;
		while (isspace(*pcBlank))
			*pcBlank++ = '\000';

		/* file is a link */
		if ('@' == pcSkip[0] || ':' == pcSkip[0]) {
			pCL->pclink = strcpy(malloc(strlen(pcSkip)+1),pcSkip);
			strcpy(pCL->acmode, "l0/7777");
			goto at_comment;
		} else if ((char *)0 != pCL->pclink) {
			free(pCL->pclink);
			pCL->pclink = (char *)0;
		}
		/* node we can't look at (mode .)
		 */
		if ('.' == pcSkip[0] && '\000' == pcSkip[1]) {
			pCL->mtype = 0;
			(void)strcpy(pCL->acmode, ".0/7777");
			pCL->mmust = 0;
			pCL->moptional = 07777;
			pCL->acowner[0] = '*';
			pCL->acowner[1] = '\000';
			pCL->acgroup[0] = '*';
			pCL->acgroup[1] = '\000';
			goto at_comment;
		}
		if ('~' == pcSkip[0] || '!' == pcSkip[0] || '*' == pcSkip[0]) {
			(void)strcpy(pCL->acmode, pcSkip);
			if ('\000' != pcSkip[1]) {
				CvtMode(pcSkip+1, &pCL->mmust, &pCL->moptional);
				pCL->mtype = (pCL->moptional|pCL->mmust) & S_IFMT;
				pCL->mmust &= 07777;
			} else {
				pCL->mtype = 0;
				pCL->mmust = 0;
				pCL->moptional = 0644;
			}
		} else if (0 == strcmp(pcSkip, acEq)) {
			if ((char *)0 == DEFMODE) {
				pCL->acmode[0] = '*';
			} else {
				CvtMode(DEFMODE, &pCL->mmust, &pCL->moptional);
				pCL->acmode[0] = '\000';
			}
		} else if ('\"' == pcSkip[0] && '\000' == pcSkip[1]) {
			/* use previous */;
		} else {
			CvtMode(pcSkip, &pCL->mmust, &pCL->moptional);
			pCL->moptional &= 07777;
			pCL->acmode[0] = '\000';
		}
		if ('\000' == pCL->acmode[0]) {
			pCL->mtype = pCL->mmust & S_IFMT;
			pCL->mmust &= 07777;
			(void)NodeType(pCL->mtype, pCL->acmode);
		}
		if (0 == pCL->mtype) {
			pCL->mtype = S_IFREG;
		}

		/* owner */
		pcSkip = pcBlank;
		for (pcBlank = pcSkip; '\000' != *pcBlank && ! isspace(*pcBlank); ++pcBlank)
			/*empty*/;
		while (isspace(*pcBlank))
			*pcBlank++ = '\000';
		pCL->fbangowner = 0;
		if ('!' == *pcSkip) {
			pCL->fbangowner = 1;
			++pcSkip;
		}
		if (0 == strcmp(pcSkip, acEq)) {
			if ((struct passwd *)0 != pwdDef) {
				(void)strcpy(pCL->acowner, pwdDef->pw_name);
			} else if ((char *)0 == DEFOWNER) {
				(void)strcpy(pCL->acowner, acAny);
			} else {
				(void)strcpy(pCL->acowner, DEFOWNER);
			}
		} else if (0 != strcmp(pcSkip, acSame)) {
			(void)strcpy(pCL->acowner, pcSkip);
		}

		/* group */
		pcSkip = pcBlank;
		for (pcBlank = pcSkip; '\000' != *pcBlank && ! isspace(*pcBlank); ++pcBlank)
			/*empty*/;
		while (isspace(*pcBlank))
			*pcBlank++ = '\000';
		pCL->fbanggroup = 0;
		if ('!' == *pcSkip) {
			pCL->fbanggroup = 1;
			++pcSkip;
		}
		if (0 == strcmp(pcSkip, acEq)) {
			if ((struct group *)0 != grpDef) {
				(void)strcpy(pCL->acgroup, grpDef->gr_name);
			} else if ((char *)0 == DEFGROUP) {
				(void)strcpy(pCL->acgroup, acAny);
			} else {
				(void)strcpy(pCL->acgroup, DEFGROUP);
			}
		} else if (0 != strcmp(pcSkip, acSame)) {
			(void)strcpy(pCL->acgroup, pcSkip);
		}

		/* strip or not */
		pcSkip = pcBlank;
		for (pcBlank = pcSkip; '\000' != *pcBlank && ! isspace(*pcBlank); ++pcBlank)
			/*empty*/;
		while (isspace(*pcBlank))
			*pcBlank++ = '\000';
		if (0 == strcmp(pcSkip, acEq)) {
			pCL->chstrip = CF_NONE;
			pCL->pcflags = (char *)0;
		} else {
			switch (*pcSkip) {
			case 's':		/* yeah we should take these too */
			case 'S':
				pCL->chstrip = CF_STRIP;
				break;
			case 'l':
			case 'L':
				pCL->chstrip = CF_RANLIB;
				break;
			default:
				(void)fprintf(stderr, "%s: unknown strip indicator `%s\'\n", progname, pcSkip);
				/* fall through to recover */
			case '-':
			case 'n':
			case 'N':
			case ',':
				pCL->chstrip = CF_NONE;
				break;
			case '*':
			case '\?':
				pCL->chstrip = CF_ANY;
				break;
			case '\"':
				break;
			}
			while (',' == *++pcSkip) {
				/* skip , */;
			}
			if (0 == strcmp(pcSkip, acSame)) {
				/* leave from the last line */
			} else if (!isspace(*pcSkip)) {
				pCL->pcflags = pcSkip;
			} else {
				pCL->pcflags = (char *)0;
			}
		}

 at_comment:
		if (0 != strcmp(pcBlank, acSame)) {
			register unsigned uNeed;

			uNeed = (strlen(pcBlank)|7)+1;
			if (uNeed > uMesgLen) {
				pcMesg = realloc(pcMesg, uNeed);
				uMesgLen = uNeed;
				pCL->pcmesg = pcMesg;
			}
			(void)strcpy(pcMesg, pcBlank);
		}

		if (0 == iLine)
			continue;

#if defined(INSTALL)
		if (!pCL->ffound) {
			continue;
		}
#endif

		if ('*' != pCL->acowner[0] || '\000' != pCL->acowner[1]) {
			struct passwd *pwd;
			auto struct stat stColon;
			if (':' == pCL->acowner[0]) {
				if (-1 == stat(pCL->acowner+1, & stColon)) {
					fprintf(stderr, "%s: stat: %s: %s\n", progname, pCL->acowner+1, strerror(errno));
					continue;
				}
				if ((struct passwd *)0 != pwdDef && pwdDef->pw_uid == stColon.st_uid) {
					pwd = pwdDef;
				} else if ((struct passwd *)0 == (pwd = getpwuid(stColon.st_uid))) {
					fprintf(stderr, "%s: getpwuid: %d (from %s): no such user?\n", progname, stColon.st_uid, pCL->acowner+1);
					continue;
				}
				(void)strcpy(pCL->acowner, pwd->pw_name);
			} else if ((struct passwd *)0 != pwdDef && 0 == strcmp(pCL->acowner, pwdDef->pw_name)) {
				pwd = pwdDef;
			} else if ((struct passwd *)0 == (pwd = getpwnam(pCL->acowner))) {
				fprintf(stderr, "%s: getpwname: %s: no such user\n", progname, pCL->acowner);
				continue;
			}
			pCL->uid = pwd->pw_uid;
		} else if (0 != (S_ISUID & pCL->mmust)) {
			BadSet(iLine, 'u', "user", pCL->acmode);
		}
		if ('*' != pCL->acgroup[0] || '\000' != pCL->acgroup[1]) {
			struct group *grp;
			auto struct stat stColon;
			if (':' == pCL->acgroup[0]) {
				if (-1 == stat(pCL->acgroup+1, & stColon)) {
					fprintf(stderr, "%s: stat: %s: %s\n", progname, pCL->acgroup+1, strerror(errno));
					continue;
				}
				if ((struct group *)0 != grpDef && grpDef->gr_gid == stColon.st_gid) {
					grp = grpDef;
				} else if ((struct group *)0 == (grp = getgrgid(stColon.st_gid))) {
					fprintf(stderr, "%s: getgrgid: %d (on %s): no such group\n", progname, stColon.st_gid, pCL->acgroup+1);
					continue;
				}
				(void)strcpy(pCL->acgroup, grp->gr_name);
			} else if ((struct group *)0 != grpDef && 0 == strcmp(pCL->acgroup, grpDef->gr_name)) {
				grp = grpDef;
			} else if ((struct group *)0 == (grp = getgrnam(pCL->acgroup))) {
				fprintf(stderr, "%s: getgrname: %s: no such group\n", progname, pCL->acgroup);
				continue;
			}
			pCL->gid = grp->gr_gid;
		} else if (0 != (S_ISGID & pCL->mmust)) {
			BadSet(iLine, 'g', "group", pCL->acmode);
		}

		ModetoStr(pCL->acmode+1, pCL->mmust, pCL->moptional);
#if defined(INSTCK)
		iMatches = 0;
		if ('.' == pCL->acmode[0]) {
			continue;
		}
		if ('/' == *pcCurGlob) {
			FileMatch(pcCurGlob+1, "/", DoCk);
			if (0 == iMatches) {
				NoMatches(pcCurGlob);
			}
		} else if ('.' == pcCurGlob[0] && '\000' == pcCurGlob[1]) {
			auto struct stat stCmdDir;
			for (i = 0; i < iCount; ++i) {
				if (-1 == stat(ppcDirs[i], & stCmdDir))
					continue;
				if (S_IFDIR != (stCmdDir.st_mode & S_IFMT))
					continue;
				DoCk(ppcDirs[i], & stCmdDir);
			}
		} else {
			/* never a no-matches check here, might not have
			 * any command line dirs to search, etc [ksb]
			 */
			for (i = 0; i < iCount; ++i) {
				FileMatch(pcCurGlob, ppcDirs[i], DoCk);
			}
		}
#else
		if (FALSE != fVerbose || FALSE != fTrace) {
			(void)printf("%s: found `%s\' in %s\n", progname, pcFile, pcConfig);
		}
		break;
#endif
	} while (++iLine, NULL != (pcLine = BigLine(fpCnf)));
}
