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
 * install
 *	Install a <file> and back up the existing file to a standard
 *	location (normally a sub-directory named "OLD").  The name is always
 *	"OLD/<file>".  Name clashes in OLD are resolved by renaming an existing
 *	OLD/<file> to OLD/<file><pid>.  The OLD files should be cleaned
 *	up with a script that removes aged files (see purge(1l)).
 *
 * Authors:
 *	Jeff Smith (jms) & Kevin Braunsdorf (ksb)
 *	Purdue University Computing Center
 *	Math Building, Purdue, West Lafayette, IN, 47906
 *
 *	15 Nov 1985	to	01 June 1990
 *
 * Usage
 *	install [options] file [files] destination
 *	install -d [options] directory
 *	install -R [options] file
 *	install -[Vh]
 *
 * 	file(s):	an absolute or relative path.  If > 1 file to
 *		install is specified, destination must be a directory.
 *
 *	destination:	a pathname ending in either the destination
 *		directory or a filename which may be the same as or different
 *		than the tail of file(s).
 *
 * mk(1l) stuff:
 * $Compile: make all
 * $Compile: SYS=bsd make all
 * $Compile: SYS=SYSV make all
 *
 * Environment:
 *	Set the environment variable INSTALL to any of the options to get the
 *	same effect as using them on the command line (e.g., INSTALL="-c").
 *	Command line options silently override environental variables.
 *
 * N.B.
 *	+ "OLD" directories may *not* be mount points!
 *	+ the TMPINST macro file may be removed by purge after only few hours
 *
 * BUGS:
 *	+ competing installs can (still) loose data :-{!
 *	  if two users are installing the same file at the same time
 *	  we can loose one of the two updates, use the flock(1)
 *	  command on the destination directory to avoid this in the shell.
 *	  (using flock(2) we *could* avoid this, see notes in main)
 *		flock ${BIN} install -vs ${PROG} ${BIN}
 */

#if !defined(lint)
static char *rcsid = "$Id: main.c,v 8.15 2009/12/08 22:58:14 ksb Exp $";
#endif	/* !lint */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <stdio.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <sysexits.h>

#include "machine.h"
#include "configure.h"
#include "install.h"
#include "syscalls.h"
#include "special.h"
#include "file.h"
#include "dir.h"
#include "main.h"

#if STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#define ENVOPT 1
#define GETARG 0
#define GETOPT 1
/* @(#)getopt.i  -- literal text included from a tempate
 * based on Keith Bostic's getopt in comp.sources.unix volume1
 * modified for mkcmd use.... by ksb@cc.purdue.edu (Kevin Braunsdorf)
 */

#if GETOPT || GETARG
static int
	ui_optind = 1;		/* index into parent argv vector	*/
static char
	*ui_optarg;		/* argument associated with option	*/
#endif /* only if we use them */

#if ENVOPT
/* breakargs - break a string into a string vector for execv.
 *
 * Note, when done with the vector, mearly "free" the vector.
 * Written by Stephen Uitti, PUCC, Nov '85 for the new version
 * of "popen" - "nshpopen", that doesn't use a shell.
 * (used here for the as filters, a newer option).
 *
 * breakargs is copyright (C) Purdue University, 1985
 *
 * Permission is hereby given for its free reproduction and
 * modification for All purposes.
 * This notice and all embedded copyright notices be retained.
 */

/* this trys to emulate shell quoting, but I doubt it does a good job	(ksb)
 * [[ but not substitution -- that would be silly ]]
 */
static char *
mynext(pch)
register char *pch;
{
	register int fQuote;

	for (fQuote = 0; (*pch != '\000' && *pch != ' ' && *pch != '\t')||fQuote; ++pch) {
		if ('\\' == *pch) {
			continue;
		}
		switch (fQuote) {
		default:
		case 0:
			if ('"' == *pch) {
				fQuote = 1;
			} else if ('\'' == *pch) {
				fQuote = 2;
			}
			break;
		case 1:
			if ('"' == *pch)
				fQuote = 0;
			break;
		case 2:
			if ('\'' == *pch)
				fQuote = 0;
			break;
		}
	}
	return pch;
}

/* given an envirionment variable insert it in the option list		(ksb)
 * (exploded with the above routine)
 */
static int
envopt(cmd, pargc, pargv)
char *cmd, *(**pargv);
int *pargc;
{
	register char *p;		/* tmp				*/
	register char **v;		/* vector of commands returned	*/
	register unsigned sum;		/* bytes for malloc		*/
	register int i, j;		/* number of args		*/
	register char *s;		/* save old position		*/
	register char hold;		/* hold a character for a second*/

	while (*cmd == ' ' || *cmd == '\t')
		cmd++;
	p = cmd;			/* no leading spaces		*/
	i = 1 + *pargc;
	sum = sizeof(char *) * i;
	while (*p != '\000') {		/* space for argv[];		*/
		++i;
		s = p;
		p = mynext(p);
		sum += sizeof(char *) + 2 + (unsigned)(p - s);
		while (*p == ' ' || *p == '\t')
			p++;
	}
	++i;
	/* vector starts at v, copy of string follows NULL pointer
	 * the extra 7 bytes on the end allow use to be alligned
	 */
	v = (char **)malloc(sum+sizeof(char *)+7);
	if (v == NULL)
		return 0;
	p = (char *)v + i * sizeof(char *); /* after NULL pointer */
	i = 0;				/* word count, vector index */
	v[i++] = (*pargv)[0];
	while (*cmd != '\000') {
		v[i++] = p;
		s = cmd;
		cmd = mynext(cmd);
		hold = *cmd;
		*cmd = '\000';
		(void)strcpy(p, s);
		p += strlen(p)+1;
		if ('\000' != hold)
			*cmd++ = hold;
		while (*cmd == ' ' || *cmd == '\t')
			++cmd;
	}
	for (j = 1; j < *pargc; ++j)
		v[i++] = (*pargv)[j];
	v[i] = (char *)NULL;
	*pargv = v;
	*pargc = i;
	return i;
}
#endif /* envopt called */

#if GETARG
/*
 * return each non-option argument one at a time, EOF for end of list
 */
static int
getarg(nargc, nargv)
int nargc;
char **nargv;
{
	if (nargc <= ui_optind) {
		ui_optarg = (char *) 0;
		return EOF;
	}
	ui_optarg = nargv[ui_optind++];
	return 0;
}
#endif /* getarg called */


#if GETOPT
static int
	ui_optopt;			/* character checked for validity	*/

/* get option letter from argument vector, also does -number correctly
 * for nice, xargs, and stuff (these extras by ksb)
 */
static int
n_getopt(nargc, nargv, ostr)
int nargc;
char **nargv, *ostr;
{
	register char	*oli;		/* option letter list index	*/
	static char	EMSG[] = "";	/* just a null place		*/
	static char	*place = EMSG;	/* option letter processing	*/

	if ('\000' == *place) {		/* update scanning pointer */
		if (ui_optind >= nargc || nargv[ui_optind][0] != '-')
			return EOF;
		place = nargv[ui_optind];
		if ('\000' == *++place)	/* "-" (stdin)		*/
			return EOF;
		if (*place == '-' && '\000' == place[1]) {
			/* found "--"		*/
			++ui_optind;
			return EOF;
		}
	}				/* option letter okay? */
	/* if we find the letter, (not a `:')
	 * or a digit to match a # in the list
	 */
	if ((ui_optopt = *place++) == ':' ||
	 ((char *)0 == (oli = strchr(ostr,ui_optopt)) &&
	  (!(isdigit(ui_optopt)||'-'==ui_optopt) || (char *)0 == (oli = strchr(ostr, '#'))))) {
		if(!*place) ++ui_optind;
		return('?');
	}
	if ('#' == *oli) {		/* accept as -digits */
		ui_optarg = place -1;
		++ui_optind;
		place = EMSG;
		return '#';
	}
	if (*++oli != ':') {		/* don't need argument */
		ui_optarg = NULL;
		if (!*place)
			++ui_optind;
	} else {				/* need an argument */
		if (*place) {			/* no white space */
			ui_optarg = place;
		} else if (nargc <= ++ui_optind) {	/* no arg */
			place = EMSG;
			return('*');
		} else {
			ui_optarg = nargv[ui_optind];	/* white space */
		}
		place = EMSG;
		++ui_optind;
	}
	return ui_optopt;			/* dump back option letter */
}
#endif /* getopt called */
#undef ENVOPT
#undef GETARG
#undef GETOPT


/* global variables for options
 */
char *progname;			/* tail of argv[0]			*/
char *Group = (char *)0;	/* given group ownership		*/
char *Owner = (char *)0;	/* given owner				*/
char *Mode = (char *)0;		/* given mode				*/
char *HardLinks = (char *)0;	/* hard links to installed file		*/
char *SoftLinks = (char *)0;	/* symlinks to installed file		*/
char *pcFFlags = (char *)0;	/* string spec for file flags		*/
int Copy = FALSE;		/* we're copying			*/
int Destroy = FALSE;		/* user doesn't want an OLD		*/
int BuildDir = FALSE;		/* we're installing a directory		*/
int KeepTimeStamp = FALSE;	/* if preserving timestamp		*/
int fQuiet = FALSE;		/* suppress some errors, be quiet	*/
int Ranlib = FALSE;		/* if we're running ranlib(1)		*/
int Strip = FALSE;		/* if we're running strip(1)		*/
int fDelete = FALSE;		/* uninstall the given file/dir		*/
int fRecurse = FALSE;		/* Sun-like recursive dir install	*/
int fTrace = FALSE;		/* if just tracing			*/
int fVerbose = FALSE;		/* if verbose				*/
int f1Copy = FALSE;		/* only keep one copy			*/
int fOnlyNew = FALSE;		/* if file exists abort (new install)	*/
unsigned long uFFSet, uFFClr;	/* file flags set,clr under -f		*/
#if defined(CONFIG)
extern struct passwd *pwdDef;	/* aux default owner for config file	*/
extern struct group *grpDef;	/* aux default group for config file	*/
char *pcSpecial = CONFIG;	/* file contains ckecked paths		*/
#endif	/* have a -C option?			*/

/*
 * global variables, but not option flags
 */
int bHaveRoot;			/* we have root permissions		*/
char *pcGuilty;			/* the name logged as the installer	*/
static char copyright[] =
   "@(#) Copyright 1990 Purdue Research Foundation. All rights reserved.\n";


/*
 * Print detailed usage info
 */
static char *apcHelp[] = {
	"1        keep exactly one backup of the installed file",
	"c        copy file instead of renaming it",
#if defined(CONFIG)
	"C config use config to check for special files",
#endif /* no -C option to list			*/
	"d        install a directory (mkdir with specified mode/group/owner)",
	"D        destroy target (do not backup)",
#if HAVE_CHFLAGS
	"f flags  use chflags(2) to set file flags",
#endif
	"g group  install target with group",
	"h        help (print this text)",
	"H links  hard links to update (colon separated)",
	"l        run ranlib(1) on file",
	"m mode   install target with mode",
	"n        trace execution but do not do anything",
	"N        only install if target is new",
	"o owner  install target with owner",
	"p        preserve timestamp of file being installed",
	"q        if install can recover from the error be quiet",
	"r        build all intervening directories",
	"R        remove the given target",
	"s        run strip(1) on file",
#if HAVE_SLINKS
	"S links  symbolic links to update (colon separated)",
#endif	/* no symbolic links to worry about	*/
	"v        run ls(1) on installed files and backups",
	"V        explain compiled in default modes",
	(char *)0
};

#if defined(CONFIG)
char acVArgs[] =
	"[-C config]";
char acDArgs[] =
	"[-nqrv] [-C config] [-g group] [-m mode] [-o owner]";
#if HAVE_SLINKS
char acOArgs[] =
	"[-1cDlnNpqsv] [-C config] [-H links] [-S links] [-g group] [-m mode] [-o owner]";
#else
char acOArgs[] =
	"[-1cDlnNpqsv] [-C config] [-H links] [-g group] [-m mode] [-o owner]";
#endif

#else	/* change usage messgae, reflect CONFIG	*/
char acVArgs[] =
	"";
char acDArgs[] =
	"[-nqrv] [-g group] [-m mode] [-o owner]";
#if HAVE_SLINKS
char acOArgs[] =
	"[-1cDlnNpqsv] [-H links] [-S links] [-g group] [-m mode] [-o owner]";
#else
char acOArgs[] =
	"[-1cDlnNpqsv] [-H links] [-g group] [-m mode] [-o owner]";
#endif
#endif /* no -C option to list			*/

/*
 * Output a useful, std usage message. Maybe a longer one if requested	(ksb)
 */
static void
Usage(fp, bVerbose)
FILE *fp;		/* file to output to				*/
int bVerbose;		/* should we explain options more		*/
{
	register char **ppc;

	(void)fprintf(fp, "%s: usage %s files destination\n", progname, acOArgs);
	(void)fprintf(fp, "%s: usage -d %s directory\n", progname, acDArgs);
	(void)fprintf(fp, "%s: usage -h\n", progname);
	(void)fprintf(fp, "%s: usage -R [options] target\n", progname);
	(void)fprintf(fp, "%s: usage -V %s\n", progname, acVArgs);
	if (bVerbose) {
		for (ppc = apcHelp; (char *)0 != *ppc; ++ppc) {
			(void)fprintf(fp, *ppc, OLDDIR);
			(void)fputc('\n', fp);
		}
		fprintf(fp, "%s", copyright);
	}
}


/*
 * first non-nil argument						(ksb)
 */
static char *
nonnil(pc1, pc2)
char *pc1, *pc2;
{
	if ((char *)0 != pc1)
		return pc1;
	if ((char *)0 != pc2)
		return pc2;
#if defined(DEBUG)
	Die("nil pointer in nonnil");
#endif
	return (char *)0;
}

/*
 * explain to the poor user what install will use as modes here		(ksb)
 */
static void
Explain(fp)
FILE *fp;
{
	static char acDMsg[] = "inherited";
	auto struct passwd *pwd;	/* owner of this dir.		*/
	auto struct group *grp;		/* group of this directory	*/

	(void)fprintf(fp, "%s: version: %s\n", progname, "$Id: main.c,v 8.15 2009/12/08 22:58:14 ksb Exp $");
#if defined(CONFIG)
	(void)fprintf(fp, "%s: configuration file: %s, owner = %s, group = %s\n", progname, ((char *)0 == pcSpecial || '\000' == *pcSpecial) ? "(none)" : pcSpecial, (struct passwd *)0 != pwdDef ? pwdDef->pw_name : acDMsg, (struct group *)0 != grpDef ? grpDef->gr_name : acDMsg);
#endif
#if defined(INST_FACILITY)
	(void)fprintf(fp, "%s: syslog facility: %d\n", progname, INST_FACILITY);
#endif
	(void)fprintf(fp, "%s: superuser (%s) defaults:\n", progname, pcGuilty);
	(void)fprintf(fp, "%s: owner is file=%-10s dir=%-10s %s=%s\n", progname,
		nonnil(DEFOWNER, acDMsg),
		nonnil(DEFDIROWNER, acDMsg),
		OLDDIR,
		nonnil(ODIROWNER, acDMsg)
	);
	(void)fprintf(fp, "%s: group is file=%-10s dir=%-10s %s=%s\n", progname,
		nonnil(DEFGROUP, acDMsg),
		nonnil(DEFDIRGROUP, acDMsg),
		OLDDIR,
		nonnil(ODIRGROUP, acDMsg)
	);
	(void)fprintf(fp, "%s: mode is  file=%-10s dir=%-10s %s=%s\n", progname,
		nonnil(DEFMODE, acDMsg),
		nonnil(DEFDIRMODE, acDMsg),
		OLDDIR,
		nonnil(ODIRMODE, acDMsg)
	);

	if (bHaveRoot) {
		/* we assume the super user doesn't care about Joe User */
		return;
	}

	(void)setpwent();
	if ((struct passwd *)0 == (pwd = getpwuid((int) geteuid()))) {
		(void)fprintf(stderr, "%s: getpwuid: %d (effective uid) not found\n", progname, geteuid());
		exit(EX_NOUSER);
	}
	(void)endpwent();

	(void)setgrent();
	if ((struct group *)0 == (grp = getgrgid((int) getegid()))) {
		(void)fprintf(stderr, "%s: getgrgid: %d (effective gid) not found\n", progname, getegid());
		exit(EX_NOUSER);
	}
	(void)endgrent();
	(void)fprintf(fp, "%s: user defaults:\n", progname);
	(void)fprintf(fp, "%s: owner is file=%-10s dir=%-10s %s=%s\n", progname,
		pwd->pw_name,
		pwd->pw_name,
		OLDDIR,
		pwd->pw_name
	);
	(void)fprintf(fp, "%s: group is file=%-10s dir=%-10s %s=%s\n", progname,
		grp->gr_name,
		grp->gr_name,
		OLDDIR,
		grp->gr_name
	);
	(void)fprintf(fp, "%s: mode is  file=%-10s dir=%-10s %s=%s\n", progname,
		nonnil(DEFMODE, acDMsg),
		nonnil(DEFDIRMODE, acDMsg),
		OLDDIR,
		nonnil(ODIRMODE, acDMsg)
	);
}


static char acOutMem[] = "%s: out of memory\n";
/*
 * OptAccum
 * Accumulate a string, for string options that "append with a sep"	(ksb)
 * note: arg must start out as either "(char *)0" or a malloc'd string
 */
static char *
OptAccum(pcOld, pcArg, pcSep)
char *pcOld, *pcArg, *pcSep;
{
	register int len;
	register char *pcNew;

	/* Do not add null strings
	 */
	len = strlen(pcArg);
	if (0 == len) {
		return pcOld;
	}

	if ((char *)0 == pcOld) {
		pcNew = malloc(len+1);
		if ((char *)0 == pcNew) {
			(void)fprintf(stderr, acOutMem, progname);
			exit(EX_OSERR);
		}
		pcNew[0] = '\000';
	} else {
		len += strlen(pcOld)+strlen(pcSep)+1;
		if ((char *)0 == (pcNew = realloc(pcOld, len))) {
			(void)fprintf(stderr, acOutMem, progname);
			exit(EX_OSERR);
		}
		(void)strcat(pcNew, pcSep);
	}
	pcOld = strcat(pcNew, pcArg);
	return pcOld;
}


/*
 * parse options with getopt and install files
 */
int
main(argc, argv)
int argc;
char **argv;
{
	extern char *getenv();	/* we want an env var			*/
	extern char *getlogin();
	static char Opts[] =	/* valid options			*/
		"1cC:dDf:g:hH:lm:nNo:pqrRsS:u:vV";
	auto char *pcEnv;	/* options passed through env		*/
	auto char *Dest;	/* destination dir or filename		*/
	auto int iOption;	/* argument pointer			*/
	auto int iFailed;	/* installs that failed			*/
	auto int iArgs;		/* args left after option processing	*/
	auto int fExplain;	/* tell the user about our defaults	*/

	(void)umask(022);

	/* Figure out our name and fix argv[0] for getopt() if necessary.
	 */
	if (NULL == argv[0] || NULL == (progname = StrTail(argv[0])) || '\000' == *progname) {
		progname = "install";
	}
	/* here we become deinstall, or deinstallp
	 * a little gem for those who like to pick through source code...
	 */
	if ('d' == progname[0] && 'e' == progname[1]) {
		fDelete = TRUE;
	}
	iFailed = 0;

	/* Check for environtment options, if $INSTALL is a full path
	 * w/o any spaces in it ignore it. -- what a hack, ksb.
	 * This removed the bug where some (broken) makes set $INSTALL
	 * to the full path to the install program if it is a macro in
	 * the Makefile.
	 */
	if ((char *)0 != (pcEnv = getenv("INSTALL"))) {
		register char *pcHack;
		for (pcHack = pcEnv; '\000' != *pcHack; ++pcHack) {
			if (isspace(*pcHack) || '-' == *pcHack)
				break;
		} 
		if ('\000' != *pcHack) {
			envopt(pcEnv, & argc, & argv);
		}
	}

	/* See if we have root permissions, this changes default modes/owners
	 */
	bHaveRoot = 0 == geteuid();

	/* Parse command line options, set flags, etc.
	 */
	fExplain = FALSE;
	uFFSet = uFFClr = 0;
	while (EOF != (iOption = n_getopt(argc, argv, Opts))) {
		switch (iOption) {
		case '1':
			f1Copy = TRUE;
			break;
		case 'c':	/* copy		*/
			Copy = TRUE;
			break;
		case 'C':	/* check list	*/
#if defined(CONFIG)
			pcSpecial = ui_optarg;
			break;
#else /* give them an error */
			(void)fprintf(stderr, "%s: check list option not installed\n", progname);
			exit(EX_UNAVAILABLE);
#endif	/* set check flags from config file	*/
		case 'S':	/* soft links	*/
#if HAVE_SLINKS
			SoftLinks = OptAccum(SoftLinks, ui_optarg, ":");
			break;
#else	/* no symlink, warn and use hard ones	*/
			(void)fprintf(stderr, "%s: no symbolic links here, trying hard links\n", progname);
			/*fall through*/
#endif	/* handle -S based on OS		*/
		case 'H':	/* hard links	*/
			HardLinks = OptAccum(HardLinks, ui_optarg, ":");
			break;
		case 'd':	/* directory	*/
			BuildDir = TRUE;
			break;
		case 'D':	/* destroy	*/
			Destroy = TRUE;
			break;
		case 'f':	/* BSD flags to set */
#if HAVE_CHFLAGS
			pcFFlags = ui_optarg;
			if (0 != strtofflags(&ui_optarg, &uFFSet, & uFFClr)) {
				fprintf(stderr, "%s: strtofflags: \"%s\" unrecognized flag\n", progname, ui_optarg);
				exit(EX_USAGE);
			}
#else
			fprintf(stderr, "%s: no flags support found\n", progname);
#endif
			break;
		case 'g':	/* change group	*/
			Group = ui_optarg;
			break;
		case 'h':	/* help		*/
			Usage(stdout, 1);
			exit(EX_OK);
			break;
		case 'l':	/* ranlib	*/
			Ranlib = TRUE;
			break;
		case 'm':	/* change mode	*/
			Mode = ui_optarg;
			break;
		case 'n':	/* execution trace */
			fTrace = TRUE;
			break;
		case 'N':
			fOnlyNew = TRUE;
			break;
		case 'u':	/* for systemV compatibility */
		case 'o':	/* change owner	*/
			Owner = ui_optarg;
			break;
		case 'p':	/* preserve time */
			KeepTimeStamp = TRUE;
			break;
		case 'q':	/* be quiet */
			fQuiet = TRUE;
			break;
		case 'r':	/* install -d -r /a/b build /a and /a/b */
			fRecurse = TRUE;
			break;
		case 'R':	/* deintstall	*/
			fDelete = TRUE;
			break;
		case 's':	/* strip(1)	*/
			Strip = TRUE;
			break;
		case 'v':	/* run ls(1)	*/
			fVerbose = TRUE;
			break;
		case 'V':
			fExplain = TRUE;
			break;
		case '?':	/* illegal option */
			Usage(stderr, 0);
			exit(EX_USAGE);
			/*NOTREACHED*/
		case '*':
			fprintf(stderr, "%s: option `%c\' requires an argument\n", progname, ui_optopt);
			exit(EX_USAGE);
			/*NOTREACHED*/
		/* Since getopt should have caught all problems, this has
		 * to be an error
		 */
		default:
			Die("command line parsing bug");
		}
	}

	/* Usage checks.  If installing > 1 file, the destination
	 * must be a directory which already exists.
	 */
	if (FALSE != Strip && FALSE != Ranlib) {
		(void)fprintf(stderr, "%s: options `-l\' and `-s\' are incompatible\n", progname);
		exit(EX_USAGE);
	}
	if (FALSE != Ranlib && FALSE != KeepTimeStamp) {
		(void)fprintf(stderr, "%s: option `-l\' suppresses `-p\'\n", progname);
		KeepTimeStamp = FALSE;
	}
	if ((char *)0 != Owner && ((char *)0 != (pcEnv = strchr(Owner, '.')) || (char *)0 != (pcEnv = strchr(Owner, ':')))) {
		if ((char *)0 != Group) {
			fprintf(stderr, "%s: specify only one of -o owner%cgroup and -g group\n", progname, *pcEnv);
			exit(EX_USAGE);
		}
		*pcEnv++ = '\000';
		Group = pcEnv;
	}
	if ((char *)0 != Mode) {
		auto char *pcMsg; /* error, build a b,c,s,l,p message	*/
		auto int mMust;	  /* bits in mode to build		*/
		auto int m_Opt;	  /* discard optional bits		*/
		auto char chType;
		auto char *pcUse;
		auto char acShow[20];

		CvtMode(Mode, & mMust, & m_Opt);
		pcMsg = NodeType(mMust, &chType);
		switch (chType) {
		case 'd':
			BuildDir = TRUE;
			break;
		case '-':
			break;
		case 's':
			for (;;) {
				pcUse = "a unix domain service";
				break;
		case 'l':
				pcUse = "ln -s";
				break;
		case 'p':
				pcUse = "mkfifo or mknod p";
				break;
		case 'b':
		case 'c':
				(void)sprintf(acShow, "mknod %c", chType);
				pcUse = acShow;
				break;
			}
		default:
			(void)fprintf(stderr, "%s: to build a %s use `%s'\n", progname, pcMsg, pcUse);
			exit(EX_CONFIG);
		}
	}

#if defined(INST_FACILITY)
	if (bHaveRoot && FALSE == fTrace) {
		openlog(progname, 0, INST_FACILITY);
	}
#endif	/* we need to start syslog		*/
	if ((char *)0 != (pcGuilty = getlogin()) && '\000' != *pcGuilty) {
		/* found on the tty */;
	} else if ((char *)0 != (pcGuilty = getenv("USER")) && '\000' != *pcGuilty) {
		/* found in $USER */;
	} else if ((char *)0 != (pcGuilty = getenv("LOGNAME")) && '\000' != *pcGuilty) {
		/* found in $LOGNAME */;
	} else {
		/* I guess it is root -- though rcmd(3) */
		pcGuilty = "root\?";
	}

#if defined(CONFIG)
	InitCfg((char *)0, (char *)0);
#endif

	iArgs = argc - ui_optind;
	if (FALSE != fExplain) {
		Explain(stdout);
	} else if (FALSE != BuildDir) {
		auto char *pcSlash, *pcDir;

		/* this check keeps Joe Bogus from having
		 *  INSTALL=-vd
		 * in the environment break every program that uses install.
		 */
		if (1 != iArgs) {
			(void)fprintf(stderr, "%s: only a single directory name may be given with -d\n", progname);
			Usage(stderr, 0);
			exit(EX_USAGE);
		}
		if ((char *)0 != HardLinks) {
			(void)fprintf(stderr, "%s: directories cannot have multiple hard links, use -S\n", progname);
			exit(EX_USAGE);
		}
		if ((FALSE != Destroy || FALSE != f1Copy) && FALSE == fQuiet) {
			(void)fprintf(stderr, "%s: -d ignores -D and -1\n", progname);
		}
		if ((FALSE != Strip || FALSE != Ranlib) && FALSE != fQuiet) {
			(void)fprintf(stderr, "%s: -d cannot strip or ranlib a directory\n", progname);
		}

		/* StrTail cuts trailing '/'s
		 * If we are building an OLD dir do it correctly
		 */
		pcDir = argv[argc-1];
		if (0 == strcmp(OLDDIR, StrTail(pcDir))) {
			pcSlash = strrchr(pcDir, '/');
			if ((char *)0 == pcSlash) {
				/* nothing to do, in `pwd` */;
			} else if (pcSlash == pcDir) {
				/* build "/", sure... done */;
			} else if (FALSE != fRecurse) {
				*pcSlash = '\000';
				if (!fDelete && FAIL == DirInstall(pcDir, Owner, Group, Mode, DEFDIROWNER, DEFDIRGROUP, DEFDIRMODE, (char *)0, 0)) {
					exit(EX_CANTCREAT);
				}
				*pcSlash = '/';
				/* yeah, odd... but -n look right this way */
				fRecurse = FALSE;
			}
			/* letting the user symlink to an OLD dir is very
			 * bogus, we do it only because I cannot think
			 * of a good error message.... (ksb)
			 * We have to diddle here to avoid destination
			 * defaults (to the existing mode/owner/group).
			 */
			if (FALSE != bHaveRoot && (char *)0 == Owner) {
				Owner = ODIROWNER;
			}
			if (FALSE != bHaveRoot && (char *)0 == Group) {
				Group = ODIRGROUP;
			}
			if (FALSE != bHaveRoot && (char *)0 == Mode) {
				Mode = ODIRMODE;
			}
			if (FAIL == DirInstall(pcDir, Owner, Group, Mode, (char *)0, (char *)0, (char *)0, SoftLinks, fDelete)) {
				exit(EX_CANTCREAT);
			}
		} else if (FAIL == DirInstall(pcDir, Owner, Group, Mode, DEFDIROWNER, DEFDIRGROUP, DEFDIRMODE, SoftLinks, fDelete)) {
			exit(EX_CANTCREAT);
		}
	} else if (fDelete) {
		if (iArgs != 1) {
			(void)fprintf(stderr, "%s: just give one target to remove\n", progname);
			Usage(stderr, 0);
			exit(EX_USAGE);
		}
		if (FALSE != Destroy) {
			(void)fprintf(stderr, "%s: -D changed to -1 to preserve last copy of target\n", progname);
			Destroy = FALSE;
			f1Copy = TRUE;
		}
		if (FAIL == Install("-", argv[ui_optind], HardLinks, SoftLinks)) {
			++iFailed;
		}
	} else {
		if (iArgs < 2) {
			(void)fprintf(stderr, "%s: need both source and destination\n", progname);
			Usage(stderr, 0);
			exit(EX_USAGE);
		}

		Dest = argv[argc - 1];
		if (iArgs > 2 && IsDir(Dest) == FALSE) {
			(void)fprintf(stderr, "%s: `%s\' must be an existing directory to install multiple files or directories\n", progname, Dest);
			Usage(stderr, 0);
			exit(EX_USAGE);
		}

		if (iArgs > 2 && ((char *)0 != HardLinks || (char *)0 != SoftLinks)) {
			(void)fprintf(stderr, "%s: -H or -S option ambiguous with multiple source files\n", progname);
			exit(EX_USAGE);
		}

		/* install the files
		 */
		for (--argc; ui_optind < argc; ui_optind++) {
			if (FAIL == Install(argv[ui_optind], Dest, HardLinks, SoftLinks)) {
				(void)fprintf(stderr, "%s: installation of `%s\' failed\n", progname, argv[ui_optind]);
				++iFailed;
			}
		}
	}

#if defined(INST_FACILITY)
	if (bHaveRoot && FALSE == fTrace) {
		closelog();
	}
#endif	/* close-m up for now			*/

	exit(iFailed);
}
