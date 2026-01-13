/* mkcmd generated user interface
 * built by mkcmd version 8.14 Dist
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/param.h>
#include <pwd.h>
#include <sys/types.h>
#include "machine.h"
#include "type.h"
#include "emit.h"
#include "parser.h"
/* from getopt.m */
/* from getopt_key.m */
/* from mkcmd.m */
#line 9 "mkcmd.m"
/* $Id: main.c,v 8.14 2000/06/14 13:02:29 ksb Dist $
 */
#if !defined(MER_INV)
#define MER_INV		0x20	/* internal problem			*/
#define MER_SEMANTIC	0x10	/* strange idea, but we can't		*/
#define MER_LIMIT	0x08	/* internal limit problem		*/
#define MER_SYNTAX	0x04	/* syntax in input file			*/
#define MER_BROKEN	0x02	/* oush! what is this?			*/
#define MER_PAIN	0x01	/* we don't even know			*/
#endif

extern FILE *Search(/* char *pcFile, char *pcMode, int (**pfiClose)() */);
#line 24 "mkcmd.m"
static char rcsid[] =
	"$Id: main.c,v 8.14 2000/06/14 13:02:29 ksb Dist $";

/* mkcmd itself is build with -G so we can use the same main.c
 * file on multiple source platforms,  most people should let mkcmd
 * do the hard work.  mkcmd rembers how it was built and builds all
 * subsequent parsers that way...
 */
#if !defined(MSDOS)
extern int errno;
#endif


#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#if USE_MALLOC_H
#include <malloc.h>
#else
#if NEED_MALLOC_EXTERN
extern char *malloc(), *calloc(), *realloc();
#endif
#endif

/* from std_version.m */
/* from std_help.m */
/* from scan.m */
#line 1 "getopt.mi"
/* $Id: main.c,v 8.14 2000/06/14 13:02:29 ksb Dist $
 */

#if 1 || 1 || 0
/* IBMR2 (AIX in the real world) defines
 * optind and optarg in <stdlib.h> and confuses the heck out
 * of the C compiler.  So we use those externs.  I guess we will
 * have to stop using the old names. -- ksb
 */
static int
	u_optInd = 1;		/* index into parent argv vector	*/
static char
	*u_optarg;		/* argument associated with option	*/

#endif /* only if we use them */
#if 1
static int
	u_optopt;			/* character checked for validity	*/
#endif
char
	*progname = "$Id: main.c,v 8.14 2000/06/14 13:02:29 ksb Dist $",
	*au_terse[] = {
		" [-AGKMP] [-B boot.m] [-I directory] [-m manpage] [-n name] [files]",
		" -E expander",
		" -h",
		" -T",
		" -V",
		(char *)0
	},
	*u_help[] = {
		"A           generate ANSI function prototype headers",
		"B boot.m    our boiler plate module to find getopt and the like",
		"E expander  display expander help",
		"G           do not guess a C environment, let user code guess",
		"h           print this help message",
		"I directory search directory to search for source files",
		"K           output a table of all the keys defined for this product",
		"m manpage   build a manpage template in this file",
		"M           output transitive dependencies information",
		"n name      generate files under this alternate basename",
		"P           suppress most #line control information",
		"T           display supported type table",
		"V           show version information",
		"files       specify files containing option descriptions",
		(char *)0
	};
int
	fAnsi = 0,
	iExit = 0;
char
	*pcBootModule = "getopt.m",
	acDefTDir[] = DEFDIR;
int
	fWeGuess = 1,
	fLanguage = 0;
static char
	*pcSearchPath = (char *)0;
int
	fKeyTable = 0,
	fMaketd = 0;
char
	*pchBaseName = "prog",
	acOutMem[] = "%s: out of memory\n",
	*pcOutPlate = "default.mo";
int
	fCppLines = 1,
	fVerbose = 0;

#ifndef u_terse
#define u_terse	(au_terse[0])
#endif
/*
 * Accumulate a string, for string options that "append with a sep"
 * note: arg must start out as either "(char *)0" or a malloc'd string
 */
static char *
optaccum(pchOld, pchArg, pchSep)
char *pchOld, *pchArg, *pchSep;
{
	register unsigned len;
	register char *pchNew;
	static char acOutMem[] =  "%s: out of memory\n";

	/* Do not add null strings
	 */
	if ((char *)0 == pchArg || 0 == (len = strlen(pchArg))) {
		return pchOld;
	}

	if ((char *)0 == pchOld) {
		pchNew = malloc(len+1);
		if ((char *)0 == pchNew) {
			fprintf(stderr, acOutMem, progname);
			exit(1);
		}
		pchNew[0] = '\000';
	} else {
		len += strlen(pchOld)+strlen(pchSep)+1;
		if ((char *)0 == (pchNew = realloc(pchOld, len))) {
			fprintf(stderr, acOutMem, progname);
			exit(1);
		}
		(void)strcat(pchNew, pchSep);
	}
	pchOld = strcat(pchNew, pchArg);
	return pchOld;
}
/* from getopt.m */
/* from getopt_key.m */
/* from mkcmd.m */
#line 200 "mkcmd.m"
/* Open the file, or if we can't do that try explode(1).		(ksb)
 * You won't believe what this does to reduce the number of modules
 * in /usr/local/lib/mkcmd (cmd.m becomes 20 fewer files)
 * a trailing "-module" means pull the module from the file, if it
 * exists.
 *
 * So fopen("cmd-fred.m") -> popen("explode -o - -u fred cmd.m", "r")
 * Less obvious, "cmd-pwd,cd.m" gets both due to "fine line" action,
 * of course they are intermixed a little.
 *
 * We should prefer the .e file to the .m, so
 *	cmd.m	general interface
 *	cmd.e	the optional part we can add ("cmd-pwd.m" finds it anyway)
 *	cmd.mi	the include section
 *	cmd.ei	the optional include section ("cmd-pwd.mi" finds it)
 * same for .mh (.eh) and .mc (.ec) files.
 *
 * We also try for in the last component of a module name (kind_mod.m):
 *	kind/mod.m
 * this is for hosts with very limited file names lengths.
 */
static FILE *
Explode(pcModule, pcMode, ppfiClose)
char *pcModule, *pcMode;
int (**ppfiClose)();
{
	register FILE *fp;
	register char *pcMinus, *pcBar, *pcDot, *pcCheck, *pcExt;
	auto char acCmd[MAXPATHLEN*2+500];

	*ppfiClose = fclose;
	if ((FILE *)0 != (fp = fopen(pcModule, pcMode))) {
		if (fMaketd) {
			printf("%s.c: %s\n", pchBaseName, pcModule);
		}
		return fp;
	}

	/* We prefer kind/mod.m to kind_mod.m -- strangely
	 * note that in /usr/local_1/lib/mkcmd/util_tmp.m
	 * we will only try the last "_" once.
	 */
	if ((char *)0 == (pcBar = strrchr(pcModule, '/'))) {
		pcBar = pcModule;
	}
	if ((char *)0 != (pcBar = strrchr(pcBar, '_'))) {
		*pcBar = '/';
		fp = Explode(pcModule, pcMode, ppfiClose);
		*pcBar = '_';
		if ((FILE *)0 != fp) {
			return fp;
		}
	}

	if ((char *)0 == (pcMinus = strrchr(pcModule, '-'))) {
		return (FILE *)0;
	}

	*pcMinus++ = '\000';
	if ((char *)0 != (pcDot = strrchr(pcMinus, '.'))) {
		*pcDot++ = '\000';
	}
	(void)sprintf(acCmd, "explode -o - -u %s ", pcMinus);
	pcCheck = acCmd+strlen(acCmd);
	(void)strcat(acCmd, pcModule);
	pcExt = pcCheck+strlen(pcCheck);
	if ((char *)0 != pcDot) {
		*--pcDot = '.';
		(void)strcpy(pcExt, pcDot);
		if ('m' == pcExt[1]) {
			pcExt[1] = 'e';
			if ((FILE *)0 != (fp = fopen(pcCheck, "r"))) {
				goto good;
			}
		}
		(void)strcpy(pcExt, pcDot);
	}
	*--pcMinus = '-';

	/* no explode archive to extract from
	 */
	if ((FILE *)0 == (fp = fopen(pcCheck, "r"))) {
		return (FILE *)0;
	}
 good:
	(void)fclose(fp);
	if (fMaketd) {
		printf("%s.c: %s\n", pchBaseName, pcCheck);
	}
	if (fVerbose) {
		printf("%s: explode %s|\n", progname, acCmd);
	}
	if ((FILE *)0 == (fp = popen(acCmd, pcMode))) {
		fprintf(stderr, "%s: popen: explode: %s\n", progname, strerror(errno));
	}
	*ppfiClose = pclose;
	return fp;
}

/* Search for a file in the specified path				(ksb)
 */
FILE *
Search(pchFile, pcMode, ppfiClose)
char *pchFile, *pcMode;
int (**ppfiClose)();
{
	static char acSlash[4] = "/";
	register char *pchDir, *pchNext, *pchSlash;
	register FILE *fp;
	auto char acFName[1025];
#if TILDEDIR
	register struct passwd *pPWUser;
#endif

	if ((FILE *)0 != (fp = Explode(pchFile, pcMode, ppfiClose))) {
		return fp;
	}
	if ('/' == pchFile[0]) {
		return (FILE *)0;
	}
	for (pchDir = pcSearchPath; (char *)0 != pchDir; pchDir = pchNext) {
		pchNext = strchr(pchDir, ':');
		if ((char *)0 != pchNext) {
			*pchNext = '\000';
		}
#if TILDEDIR
		if ('~' == pchDir[0]) {
			if ((char *)0 == (pchSlash = strchr(pchDir, '/')))
				pchSlash = acSlash;
			pchSlash[0] = '\000';
			if ((struct passwd *)0 == (pPWUser = getpwnam(pchDir+1))) {
				fprintf(stderr, "%s: getpwname: %s: %s\n", progname, pchDir+1, strerror(errno));
				exit(MER_BROKEN);
			}
			(void)strcpy(acFName, pPWUser->pw_dir);
			pchSlash[0] = '/';
			(void)strcat(acFName, pchSlash);
		} else {
			(void)strcpy(acFName, pchDir);
		}
#else
		(void)strcpy(acFName, pchDir);
#endif
		if ((char *)0 != pchNext) {
			*pchNext++ = ':';
		}
		pchSlash = strrchr(acFName, '/');
		if ((char *)0 != pchSlash && '\000' == pchSlash[1]) {
			do {
				if (acFName == pchSlash)
					break;
				*pchSlash-- = '\000';
			} while ('/' == pchSlash[0]);
		}
		(void)strcat(acFName, "/");
		(void)strcat(acFName, pchFile);
		if (fVerbose) {
			fprintf(stderr, "%s: looking for `%s\'\n", progname, acFName);
		}
		if ((FILE *)0 != (fp = Explode(acFName, pcMode, ppfiClose))) {
			return fp;
		}
	}
	return (FILE *)0;
}
/* from std_version.m */
/* from std_help.m */
/* from scan.m */

#line 1 "getopt.mc"
/* $Id: main.c,v 8.14 2000/06/14 13:02:29 ksb Dist $
 * Based on Keith Bostic's getopt in comp.sources.unix volume1
 * modified for mkcmd use by ksb@fedex.com (Kevin S Braunsdorf).
 */

#if 1 || 0
/* breakargs - break a string into a string vector for execv.
 *
 * Note, when done with the vector, merely "free" the vector.
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
#if 0
static char *u_mynext(register char *u_pcScan, char *u_pcDest)
#else
static char *
u_mynext(u_pcScan, u_pcDest)
register char *u_pcScan, *u_pcDest;
#endif
{
	register int u_fQuote;

	for (u_fQuote = 0; *u_pcScan != '\000' && (u_fQuote||(*u_pcScan != ' ' && *u_pcScan != '\t')); ++u_pcScan) {
		switch (u_fQuote) {
		default:
		case 0:
			if ('"' == *u_pcScan) {
				u_fQuote = 1;
				continue;
			} else if ('\'' == *u_pcScan) {
				u_fQuote = 2;
				continue;
			}
			break;
		case 1:
			if ('"' == *u_pcScan) {
				u_fQuote = 0;
				continue;
			}
			break;
		case 2:
			if ('\'' == *u_pcScan) {
				u_fQuote = 0;
				continue;
			}
			break;
		}
		if ((char*)0 != u_pcDest) {
			*u_pcDest++ = *u_pcScan;
		}
	}
	if ((char*)0 != u_pcDest) {
		*u_pcDest = '\000';
	}
	return u_pcScan;
}

/* given an envirionment variable insert it in the option list		(ksb)
 * (exploded with the above routine)
 */
#if 0
static int u_envopt(char *cmd, int *pargc, char *(**pargv))
#else
static int
u_envopt(cmd, pargc, pargv)
char *cmd, *(**pargv);
int *pargc;
#endif
{
	register char *p;		/* tmp				*/
	register char **v;		/* vector of commands returned	*/
	register unsigned sum;		/* bytes for malloc		*/
	register int i, j;		/* number of args		*/
	register char *s;		/* save old position		*/

	while (*cmd == ' ' || *cmd == '\t')
		cmd++;
	p = cmd;			/* no leading spaces		*/
	i = 1 + *pargc;
	sum = sizeof(char *) * i;
	while (*p != '\000') {		/* space for argv[];		*/
		++i;
		s = p;
		p = u_mynext(p, (char *)0);
		sum += sizeof(char *) + 1 + (unsigned)(p - s);
		while (*p == ' ' || *p == '\t')
			p++;
	}
	++i;
	/* vector starts at v, copy of string follows nil pointer
	 * the extra 7 bytes on the end allow use to be alligned
	 */
	v = (char **)malloc(sum+sizeof(char *)+7);
	if ((char **)0 == v)
		return 0;
	p = (char *)v + i * sizeof(char *); /* after nil pointer */
	i = 0;				/* word count, vector index */
	v[i++] = (*pargv)[0];
	while (*cmd != '\000') {
		v[i++] = p;
		cmd = u_mynext(cmd, p);
		p += strlen(p)+1;
		while (*cmd == ' ' || *cmd == '\t')
			++cmd;
	}
	for (j = 1; j < *pargc; ++j)
		v[i++] = (*pargv)[j];
	v[i] = (char *)0;
	*pargv = v;
	*pargc = i;
	return i;
}
#endif /* u_envopt called */

#if 1 || 0
/*
 * return each non-option argument one at a time, EOF for end of list
 */
#if 0
static int u_getarg(int nargc, char **nargv)
#else
static int
u_getarg(nargc, nargv)
int nargc;
char **nargv;
#endif
{
	if (nargc <= u_optInd) {
		u_optarg = (char *)0;
		return EOF;
	}
	u_optarg = nargv[u_optInd++];
	return 0;
}
#endif /* u_getarg called */


#if 1 || 0
/* get option letter from argument vector, also does -number correctly
 * for nice, xargs, and stuff (these extras by ksb)
 * does +arg if you give a last argument of "+", else give (char *)0
 * alos does xargs -e if spec'd as "e::" -- not sure I like that at all,
 * but it is the most often asked for extension (rcs's -t option also).
 */
#if 0
static int u_getopt(int nargc, char **nargv, char *ostr, char *estr)
#else
static int
u_getopt(nargc, nargv, ostr, estr)
int nargc;
char **nargv, *ostr, *estr;
#endif
{
	register char	*oli;		/* option letter list index	*/
	static char	EMSG[] = "";	/* just a null place		*/
	static char	*place = EMSG;	/* option letter processing	*/

	/* when we have no optlist, reset the option scan
	 */
	if ((char *)0 == ostr) {
		u_optInd = 1;
		place = EMSG;
		return EOF;
	}
	if ('\000' == *place) {		/* update scanning pointer */
		if (u_optInd >= nargc)
			return EOF;
		if (nargv[u_optInd][0] != '-') {
			register int iLen;
			if ((char *)0 != estr && 0 == strncmp(estr, nargv[u_optInd], iLen = strlen(estr))) {
				u_optarg = nargv[u_optInd++]+iLen;
				return '+';
			}
			return EOF;
		}
		place = nargv[u_optInd];
		if ('\000' == *++place)	/* "-" (stdin)		*/
			return EOF;
		if (*place == '-' && '\000' == place[1]) {
			/* found "--"		*/
			++u_optInd;
			return EOF;
		}
	}				/* option letter okay? */
	/* if we find the letter, (not a `:')
	 * or a digit to match a # in the list
	 */
	if ((u_optopt = *place++) == ':' ||
	 ((char *)0 == (oli = strchr(ostr, u_optopt)) &&
	  (!(isdigit(u_optopt)|| '-' == u_optopt) || (char *)0 == (oli = strchr(ostr, '#'))))) {
		if(!*place) ++u_optInd;
		return '?';
	}
	if ('#' == *oli) {		/* accept as -digits */
		u_optarg = place-1;
		++u_optInd;
		place = EMSG;
		return '#';
	}
	if (':' != *++oli) {		/* don't need argument */
		u_optarg = (char *)0;
		if ('\000' == *place)
			++u_optInd;
	} else {				/* need an argument */
		if ('\000' != *place || ':' == oli[1]) {
			u_optarg = place;
		} else if (nargc <= ++u_optInd) {	/* no arg!! */
			place = EMSG;
			return '*';
		} else {
			u_optarg = nargv[u_optInd];	/* skip white space */
		}
		place = EMSG;
		++u_optInd;
	}
	return u_optopt;			/* dump back option letter */
}
#endif /* u_getopt called */
#line 1 "mkcmd_generated_main"
/*
 * parser
 */
int
main(argc, argv)
int argc;
char **argv;
{
	static char
		sbOpt[] = "AB:cE:GhI:Km:Mn:O:PTvV",
		*u_pch = (char *)0,
		*pchManPage = (char *)0;
	static int
		u_loop = 0;
	register int u_curopt;
	register char *u_pcEnv;
	extern char *getenv();
	extern int atoi();

	if ((char *)0 == argv[0]) {
		progname = "mkcmd";
	} else if ((char *)0 == (progname = strrchr(argv[0], '/'))) {
		progname = argv[0];
	} else {
		++progname;
	}
	if ((char *)0 != (u_pcEnv = getenv("MKCMD"))) {
		u_envopt(u_pcEnv, & argc, & argv);
	}
	/* "mkcmd" forces no options */
	pcSearchPath = optaccum(pcSearchPath, ".", ":");
	Init(rcsid);
	while (EOF != (u_curopt = u_getopt(argc, argv, sbOpt, (char *)0))) {
		switch (u_curopt) {
		case '*':
			fprintf(stderr, "%s: option `-%c\' needs a parameter\n", progname, u_optopt);
			exit(1);
		case '?':
			fprintf(stderr, "%s: unknown option `-%c\', use `-h\' for help\n", progname, u_optopt);
			exit(1);
		case 'A':
			fAnsi = ! 0;
			continue;
		case 'B':
			pcBootModule = u_optarg;
			continue;
		case 'c':
			fLanguage = 0;
			continue;
		case 'E':
			EmitHelp(stdout, u_optarg);
			exit(0);
		case 'G':
			fWeGuess = ! 1;
			continue;
		case 'h':
			for (u_loop = 0; (char *)0 != (u_pch = au_terse[u_loop]); ++u_loop) {
				if ('\000' == *u_pch) {
					printf("%s: with no parameters\n", progname);
					continue;
				}
				printf("%s: usage%s\n", progname, u_pch);
			}
			for (u_loop = 0; (char *)0 != (u_pch = u_help[u_loop]); ++u_loop) {
				printf("%s\n", u_pch);
			}
			exit(0);
		case 'I':
			pcSearchPath = optaccum(pcSearchPath, u_optarg, ":");
			continue;
		case 'K':
			fKeyTable = ! 0;
			continue;
		case 'm':
			pchManPage = u_optarg;
			continue;
		case 'M':
			fMaketd = ! 0;
			continue;
		case 'n':
			pchBaseName = u_optarg;
			continue;
		case 'O':
			pcOutPlate = u_optarg;
			continue;
		case 'P':
			fCppLines = ! 1;
			continue;
		case 'T':
			DumpTypes(stdout);
			exit(0);
		case 'v':
			fVerbose = ! 0;
			continue;
		case 'V':
			printf("%s: %s\n", progname, rcsid);
			printf("%s: templates from `%s:%s'\n", progname, pcSearchPath, acDefTDir);
			exit(0);
		}
		break;
	}
	pcSearchPath = optaccum(pcSearchPath, acDefTDir, ":");
	Config(pcBootModule);
	if (u_optInd == argc) {
		Config("-");
	}

	while (EOF != u_getarg(argc, argv)) {
		Config(u_optarg);
	}
	Gen(pchBaseName, pchManPage);
	Cleanup();

	exit(iExit);
	/*NOTREACHED*/
}
