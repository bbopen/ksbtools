#!mkcmd
# $Id: msrc.m,v 1.83 2010/08/12 16:07:10 ksb Exp $
# Use hxmd to push master source files and commands
# see the msrc.html file here for details about how it works -- ksb
comment " * %kCompile: $%{cc:-gcc%} -Wall -g %%f -o %%F%k%k"

from '<sys/param.h>'
from '<sys/types.h>'
from '<sys/stat.h>'
from '<dirent.h>'
from '<unistd.h>'
from '<string.h>'
from '<stdlib.h>'
from '<sysexits.h>'

from '"machine.h"'

basename "msrc" ""
getenv "MSRC"

require "util_errno.m"
require "std_help.m" "std_version.m"
require "util_ppm.m"
require "envfrom.m"
require "make.m"

%i
static char rcsid[] =
	"$Id: msrc.m,v 1.83 2010/08/12 16:07:10 ksb Exp $";
static const char acHxmd[] = PATH_HXMD,
	acPtbw[] = PATH_PTBW,
	acMyTemp[] = "mtfcXXXXXX";
static int uZConf = 0;
static int iFirstDiv = 1;
static char *pcDefineMode = (char *)0;
static char *apcDefCmd[] = {
	"make", (char *)0
};
%%

init "MsrcStart();"

function 'aBcCDeEGHiIjJkKLMNOoQrRtTUWXYZ' {
	hidden named "AddSwitch"
	param "[hxmd-opts]"
	help "pass an option down to hxmd"
}
number {
	named "uPreLoad"
	init "2"
	param "preload"
	user "{auto char acCvt[128];snprintf(acCvt, sizeof(acCvt), \"%%u\", uPreLoad);AddSwitch('#', acCvt);}"
	help "number of preload slots to fill ahead of processing, default %qi"
}
function 'd' {
	named "AddSwitch"
	param "flags"
	help "debug flags we pass on to m4 ('?' for help)"
}
action 'Angsx' {
	hidden from "<stdio.h>"
	update "AddSwitch(%w, %ypi);"
	help "pass switch down to hxmd"
}
unsigned 'P' {
	named "iParallel"
	track "fGavePara"
	init "6"
	help "number of tasks to run in parallel, default %qi"
	param "[jobs]"
	noparam "if (%ypi == (%N = getenv(\"PARALLEL\")) || '\\000' == *%N) {%N = \"6\";}"
	after "if (%n < 1) {%n = 1;}"
}
integer 'F' {
	param "literal"
	help "used by msrc to tune hxmd, forbidden on our command line"
	user "fprintf(stderr, \"%%s: do not specify %qL on the command line\\n\", %b);"
	abort "exit(EX_USAGE);"
}
boolean 'l' {
	named "fLocal"
	help "shift the environment to a temporary directory on the local host"
}
augment action "h" {
	user 'printf("%%s: also any option to hxmd\\n", %b);'
}
augment action 'V' {
	user 'Version();'
}
char* 'u' {
	named "pcEntryLogin"
	track "fGaveLogin"
	param 'login'
	help "shorthand to define ENTRY_LOGIN"
}
boolean 'z' {
	named "fHXFlags"
	init "1"
	help "ignore any HXINCLUDE macro from makefile"
}
list {
	named "Msrc"
	param "utility"
	help "processed through hxmd, then executed on the target host"
}

%c

/* Help us understand the magic here					(ksb)
 */
static void
Version()
{
	register int i;

	printf("%s:  makefile code: %s\n", progname, rcs_make);
	printf("%s: hxmd as: %s\n", progname, acHxmd);
	printf("%s: auto MAP suffix: %s\n", progname, acMapSuf);
	if ((char *)0 != pcMakefile) {
		printf("%s: make recipe file: %s\n", progname, pcMakefile);
	} else {
		printf("%s: no make recipe file\n", progname);
	}
	printf("%s: make hook: %s", progname, pcMakeHook);
	if ((char *)0 != pcMakeClean)
		printf(":%s", pcMakeClean);
	printf("\n%s: default utility:", progname);
	for (i = 0; (char *)0 != apcDefCmd[i]; ++i) {
		printf(" %s", apcDefCmd[i]);
	}
	printf("\n%s: temporary directory template: %s\n", progname, acMakeTempPat);
}


static PPM_BUF PPMHxmd;		/* accumulated options for hxmd		*/
static unsigned uHxmd;		/* number of options we've seen		*/
static int
	fShowProvision = 0,	/* debug the provision file, -d P	*/
	fTrace = 0;		/* trace our call to hxmd, set as -d S	*/
int
	fDebug = 0;		/* debug our magics, set as -d X	*/


/* Pass a single letter switch or switch with a param down to hxmd.	(ksb)
 * N.B. won't do -P, which we take separately.
 */
static void
AddSwitch(int iOpt, char *pcArg)
{
	register char **ppc;
	/* static char acInto[] = "INTO="; */

	ppc = util_ppm_size(& PPMHxmd, (uHxmd|7)+9);
	if ('#' == iOpt) {
		register char *pcDup;

		if ((char *)0 == pcArg) {
			/* nada */
		} else if ((char *)0 == (pcDup = malloc((strlen(pcArg)|3)+5))) {
			fprintf(stderr, "%s: out of memory\n", progname);
			exit(EX_OSERR);
		} else  {
			pcDup[0] = '-';
			(void)strcpy(pcDup+1, pcArg);
			ppc[uHxmd++] = pcDup;
		}
		return;
	} else if ('\000' != iOpt) {
		ppc[uHxmd] = strdup("-_");
		ppc[uHxmd++][1] = iOpt;
	}
	if ((char *)0 != pcArg) {
		ppc[uHxmd++] = pcArg;
	}

	/* Also tap option stream for some clues we need ... -- ksb
	 */
	if ('Z' == iOpt) {
		uZConf |= 1;
	}
	if ('d' == iOpt && (char *)0 != pcArg && (char *)0 != strchr(pcArg, 'S')) {
		++fTrace;
	}
	if ('d' == iOpt && (char *)0 != pcArg && (char *)0 != strchr(pcArg, 'P')) {
		++fShowProvision;
		++fShowMake;
	}
	if ('d' == iOpt && (char *)0 != pcArg && (char *)0 != strchr(pcArg, 'X')) {
		++fDebug;
	}
	if ((char *)0 != pcArg  && (char *)0 != strchr(pcArg, '?')) {
		printf("%s: -d [PSX?]*[aceflqtxV]*\nP\ttrace makefile manipulations\nS\ttrace shell and rdist provisions\nX\ttrace execution of subcommands\n?\tdisplay these options and exit\n", progname);
		exit(EX_OK);
	}
}

/* Setup any data structures we need					(ksb)
 * Make room for the ptbw params, even if we don't use them.  We will
 * fill-in the two (char *)0's later (fLocal) or skip them.
 */
static void
MsrcStart()
{
	register char **ppc;
	register size_t wLen;
	register char *pcCur;

	util_ppm_init(& PPMHxmd, sizeof(char *), 256);
	ppc = util_ppm_size(& PPMHxmd, 256);
	uHxmd = 0;
	ppc[uHxmd++] = (char *)acPtbw;
	ppc[uHxmd++] = "-mdt";
	ppc[uHxmd++] = (char *)0;
	ppc[uHxmd++] = "-N";
	ppc[uHxmd++] = (char *)0;
	ppc[uHxmd++] = (char *)acHxmd;

	/* make hook:  __ |progname|
	 * define mode:  -D     msrc     _MODE=local|remote	\000
	 *		  2  |progname|  5   +1 +6		+1
	 */
	wLen = ((2+strlen(progname)+5+1+(6|5))|3)+5;
	pcDefineMode = malloc(wLen);
	if ((char *)0 == pcDefineMode) {
		fprintf(stderr, "%s: out of memory\n", progname);
		exit(EX_OSERR);
	}
	snprintf(pcDefineMode, wLen-1, "-D%s_MODE=", progname);
	for (pcCur = pcDefineMode; '\000' != *pcCur; ++pcCur) {
		if (islower(*pcCur))
			*pcCur = toupper(*pcCur);
	}
	/* later we'll strcat on the local or remote designation
	 */
}

/* Quote the dots and stars to prevent RE expanstion of execpt_pat's	(ksb)
 */
static void
REQuote(FILE *fpOut, const char *pcName, int fHow)
{
	register int c;
	static char acDoubleBack[] = "\\\\";
	register char *pcEsc;

	pcEsc = 0 == fHow ? "" : acDoubleBack;
	while ('\000' != (c = *pcName++)) {
		if ('\\' == c) {
			fputs(acDoubleBack, fpOut);
			continue;
		}
		if ((char *)0 != strchr("^.[$()|*+?{ \t" /* } */, c)) {
			fputs(pcEsc, fpOut);
		}
		putc(c, fpOut);
	}
}

/* Build the Provision file based on the macros from the makefile	(ksb)
 * in rdist mode (default)
 *	`myself: ( . ) -> ( '[user@]HOST` )
 *		install -onodescend ${INTO};'
 *	`dirs: ( ${SUBDIR} ) -> ( '[user@]HOST` )
 *		install -owhole,nodescend ${INTO};'
 * for files mapped to their original name:
 *	`files: ( ${SEND} ) -> ( '[user@]HOST` )
 *		except_pat ( ${IGNORE} );
 *		install -owhole ${INTO} ;'
 * for each file mapped to another name via /./ infix or ../ prefix:
 *	`files: ( $source_file -> ( '[user@]HOST` )
 *		install -b ${INTO}/$destination_name;
 * for each MAP file
 *	`files: ( HXMD_$N ) -> ( '[user@]HOST` )
 *		install -b ${INTO}/NAME[$N] ;'
 */
static void
RemoteProvision(FILE *fpOut, BOOTY *pBO, const char *pcDir)
{
	register unsigned u, uEmpty, i;
	register const char **ppcCur, **ppcCmp, *pcThis;
	register int fNone;
	auto struct stat stThis;

	ppcCmp = (const char **)pBO->ppcignore;

	fprintf(fpOut, "dnl rdist distfile to update a remote host\n");
	fprintf(fpOut, "`myself: ( . ) -> ( 'ifdef(`ENTRY_LOGIN',`ENTRY_LOGIN@')HOST` )\n\tinstall -onodescend %s;'\n", pBO->pcinto);
	fprintf(fpOut, "`dirs: ( ");
	uEmpty = 1;
	for (i = 0; i < pBO->usubdir; ++i) {
		pcThis = pBO->ppcsubdir[i];
		if (InList((char **)ppcCmp, pcThis))
			continue;
		fprintf(fpOut, " ");
		REQuote(fpOut, pcThis, 0);
		uEmpty = 0;
	}
	if (0 != uEmpty) {
		fprintf(fpOut, " .");
	}
	fprintf(fpOut, " ) -> ( 'ifdef(`ENTRY_LOGIN',`ENTRY_LOGIN@')HOST` )\n\tinstall -owhole,nodescend %s;'\n", pBO->pcinto);
	if (0 == pBO->umap && 0 == pBO->usend) {
		fprintf(fpOut, "dnl no files to update\n");
		return;
	}

	fprintf(fpOut, "`");
	if (pBO->usplay < pBO->usend) {
		fNone = 1;
		for (i = 0; i < pBO->usend; ++i) {
			pcThis = pBO->ppcsend[i];
			if (InList((char **)ppcCmp, pcThis))
				continue;
			if (pcThis != pBO->ppctarg[i])
				continue;
			if (fNone)
				fprintf(fpOut, "files: (");
			fNone = 0;
			fprintf(fpOut, " ");
			REQuote(fpOut, pcThis, 0);
		}
		if (fNone) {
			pBO->usend = pBO->usplay;
			goto splay;
		}
		fprintf(fpOut, " ) -> ( 'ifdef(`ENTRY_LOGIN',`ENTRY_LOGIN@')HOST` )\n\texcept_pat (");

		for (ppcCur = apcSkipDir; (const char *)0 != *ppcCur; ++ppcCur) {
			if (InList((char **)ppcCmp, *ppcCur))
				continue;
			if (-1 == lstat(*ppcCur, & stThis) || !S_ISDIR(stThis.st_mode))
				continue;
			fprintf(fpOut, " /");
			REQuote(fpOut, *ppcCur, 1);
		}
		if ((const char **)0 != ppcCmp) {
			for (/*hold*/; (const char *)0 != *ppcCmp; ++ppcCmp) {
				fprintf(fpOut, " /");
				REQuote(fpOut, *ppcCmp, 1);
			}
		}
		fprintf(fpOut, " );\n\tinstall -owhole %s;\n", pBO->pcinto);
	}
 splay:
	if (0 != pBO->usplay) {
		fNone = 1;
		for (i = 0; i < pBO->usend; ++i) {
			pcThis = pBO->ppcsend[i];
			if (InList((char **)ppcCmp, pcThis))
				continue;
			if (pcThis == pBO->ppctarg[i]) {
				continue;
			}
			fNone = 0;
			fprintf(fpOut, "files: ( %s ) -> ( 'ifdef(`ENTRY_LOGIN',`ENTRY_LOGIN@')HOST` )\n\tinstall %s/%s;\n", pcThis, pBO->pcinto, pBO->ppctarg[i]);
		}
		if (fNone && pBO->usend == pBO->usplay) {
			pBO->usend = 0;
		}
	}

	for (i = 0, u = 1; i < pBO->umap; ++i, ++u) {
		fprintf(fpOut, "files: ( 'HXMD_%u` ) -> ( 'ifdef(`ENTRY_LOGIN',`ENTRY_LOGIN@')HOST` )\n", u);
		fprintf(fpOut, "\tinstall -b %s/%s ;\n", pBO->pcinto, (char *)pBO->ppcmap[i]);
	}
	fprintf(fpOut, "'dnl\n");
}

/* Shift the quote mode from `m4' (`), to [m4], to none ' '		(ksb)
 * Request '`' or '[' to get statered from ' ' mode, then
 * requesting ' ' ends the string (and returns the `default' quotes).
 * Normal reset pattern installed for piCur.
 */
static void
QuoteMe(FILE *fpOut, int *piCur, int iWant)
{
	static int fDirty = 0;

	if ((int *)0 == piCur) {
		fDirty = 0;
		return;
	}
	if (*piCur == iWant) {
		return;
	}
	switch (iWant) {
	case ' ':
		if ('`' == *piCur) {
			fputc('\'', fpOut);
			break;
		}
		if ('[' == *piCur) {
			fprintf(fpOut, "]dnl\nchangequote(`,')dnl\n");
			break;
		}
		break;
	case '`':
		if (' ' == *piCur) {
			fputc('`', fpOut);
			break;
		}
		if ('[' == *piCur) {
			fprintf(fpOut, "]dnl\nchangequote(`,')dnl\n`");
			break;
		}
		break;
	case '[':
		if (' ' == *piCur) {
			if (fDirty) {
				fprintf(fpOut, "`'");
			}
			fprintf(fpOut, "changequote([,])dnl\n[");
			break;
		}
		if ('`' == *piCur) {
			fprintf(fpOut, "'dnl\nchangequote([,])dnl\n[");
			break;
		}
		break;
	default:
		fprintf(stderr, "%s: %c: unknown internal quote mode\n", progname, iWant);
		exit(EX_SOFTWARE);
	}
	*piCur = iWant;
	fDirty = 1;
}

/* try to guess the optimal m4 quotes for the given word		(ksb)
 */
static int
GuessMode(const char *pcWord, int iDef)
{
	register char *pcSingle, *pcBracket, *pcClose;

	pcClose = strchr(pcWord, '\'');
	if ((char *)0 == (pcSingle = strchr(pcWord, '`')) || pcClose < pcSingle) {
		pcSingle = pcClose;
	}
	pcClose = strchr(pcWord, ']');
	if ((char *)0 == (pcBracket = strchr(pcWord, '[')) || pcClose < pcBracket) {
		pcBracket = pcClose;
	}
	if ((char *)0 == pcSingle && (char *)0 == pcBracket) {
		return iDef;
	}
	if ((char *)0 != pcSingle && (char *)0 != pcBracket) {
		if (pcSingle > pcBracket)
			pcSingle = (char *)0;
	}
	return (char *)0 != pcSingle ? '[' : '`';
}

/* Output a given word inside shell double quotes			(ksb)
 * sh -c "the local word's we speak"
 */
static void
LocalWord(FILE *fpOut, int *piMode, char *pcScan)
{
	static char acLocalBack[] = "\\";
	register int c;

	for (/* above */;'\000' != (c = *pcScan); ++pcScan) {
		switch (c) {
		case '\\':	/* shell special, backslash each */
		case '$':
		case '\"':
			fprintf(fpOut, "%s%c", acLocalBack, c);
			break;
		case '`':	/* shell and m4 special */
			QuoteMe(fpOut, piMode, '[');
			fprintf(fpOut, "%s%c", acLocalBack, c);
			break;
		case '\'':	/* just m4 special */
			QuoteMe(fpOut, piMode, '[');
			fprintf(fpOut, "%c", c);
			break;
		case '[':	/* might be in the other quotes */
		case ']':
			QuoteMe(fpOut, piMode, ' ');
			fprintf(fpOut, "%c", c);
			break;
		default: /* anything else is literal in shell doubles */
			putc(c, fpOut);
			break;
		}
	}
}

/* Build the per-host command stream for a local update			(ksb)
 * We are file0 (under hxmd's -F0), HXMD_[uProvision] is the client command,
 * we either need to get a directory from a masked ptbw, or just use the
 * only directory made (under -P1, for example).  In the local case with
 * -P set gt 1 we might need to pass a ptbw_d around our use of ptbw.
 * We also setup the command-line for ${1}..${5}, for local update script.
 */
static void
LocalShell(FILE *fpOut, const BOOTY *pBO, const char *pcPtbwSock, const char *pcOnly)
{
	register char *pcPass;
	register unsigned uProvision;

	uProvision = pBO->umap+1;
	fprintf(fpOut, "dnl local per-host update script for hxmd\n`#!/bin/sh\n");
	if ((char *)0 != pcOnly) {
		fprintf(fpOut, "mkdir -p %s\n'defn(`HXMD_%u')` \"%s\" \"%s\" \"%s\" \"'ifdef(`ENTRY_LOGIN',`ENTRY_LOGIN'@)HOST`\" \"%s\" </dev/null\nexec rm -rf %s\n'dnl\n", pcOnly, uProvision, pcOnly, pcMakefile, pBO->pcmode, pBO->pcinto, pcOnly);
		return;
	}
	if ((char *)0 != (pcPass = getenv("ptbw_d")) && '\000' != *pcPass) {
		auto int iMode;

		iMode = '`';
		fprintf(fpOut, "ptbw_d=\"");
		LocalWord(fpOut, & iMode, pcPass);
		fprintf(fpOut, "\"\n");
		QuoteMe(fpOut, &iMode, '`');
	} else {
		fprintf(fpOut, "unset ptbw_d\n");
	}
	fprintf(fpOut, "exec ptbw -t %s -R1 sh -c \"mkdir -p -m 0700 \\$ptbw_list;'defn(`HXMD_%u')` \\$ptbw_list \"%s\" \"%s\" \"'ifdef(`ENTRY_LOGIN',`ENTRY_LOGIN'@)HOST`\" \"%s\" </dev/null;exec rm -rf \\$ptbw_list\"\n'dnl\n", pcPtbwSock, uProvision, pcMakefile, pBO->pcmode, pBO->pcinto);
}

/* Make sure any required subdir of the target is made			(ksb)
 */
static void
LocalDir(FILE *fpOut, const char *pcTail, int *piMode, PPM_BUF *pPPMList, unsigned *puDx)
{
	register char **ppcUniq, *pcThis, *pcDir;
	register size_t wLen;

	while ('/' == *pcTail) {
		++pcTail;
	}
	wLen = (strlen(pcTail)+2)|7;
	wLen += 1;
	if ((char *)0 == (pcThis = malloc(wLen))) {
		fprintf(stderr, "%s: malloc: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	*pcThis = '\000';
	for (pcDir = (char *)pcTail; (char *)0 != (pcDir = strchr(pcDir, '/')); ++pcDir) {
		snprintf(pcThis, wLen-1, "%.*s", (int)(pcDir-pcTail), pcTail);
		ppcUniq = util_ppm_size(pPPMList, (*puDx|7)+1);
		ppcUniq[*puDx] = (char *)0;
		if (InList(ppcUniq, pcThis)) {
			*pcThis = '\000';
			continue;
		}
		ppcUniq[(*puDx)++] = strdup(pcThis);
	}
	if ((FILE *)0 == fpOut || '\000' == *pcThis) {
		return;
	}
	fprintf(fpOut, "mkdir -p $1/");
	LocalWord(fpOut, piMode, pcThis);
	fprintf(fpOut, "\n");
	free((void *)pcThis);
}

/* Build a provision file (mode +x)					(ksb)
 * be sure we are in the normal m4 quotes when a User Macro might be expanded
 */
static void
LocalProvision(FILE *fpOut, const BOOTY *pBO, char **argv)
{
	register const char **ppcCur, **ppcMap;
	register char **ppcDup;
	register int i;
	auto int iMode, iGuess;
	auto PPM_BUF PPMList;
	auto unsigned uDx;

	util_ppm_init(& PPMList, sizeof(char *), 256);
	uDx = 0;
	ppcMap = (const char **)pBO->ppcmap;
	fprintf(fpOut, "dnl complete the ephemeral directory then run command\n`#!/bin/sh\n'");
#if SMSRC_COMPAT
	fprintf(fpOut, "ifdef(`ENTRY_DEFS',`',`define(ENTRY_DEFS,`/usr/local/lib/distrib/local.defs')')dnl\n");
#endif
	fprintf(fpOut, "dnl put shell functions here\ndivert(%d)dnl\n", iFirstDiv);
	/* $1 $2 $3 $4 $5 set by the ptbw wrapper */
	fprintf(fpOut, "dnl params from ptbw wrapper\nifdef(`INCLUDE_CMD',`INCLUDE_CMD(`%s')\n')dnl\ndivert(%d)dnl\n", pBO->pcmode, iFirstDiv+2);
	fprintf(fpOut, "dnl before INIT\ndivert(%d)dnl\n", iFirstDiv+4);
	fprintf(fpOut, "ifdef(`INIT_CMD',`INIT_CMD\n')dnl\n");
	iMode = ' ';
	if (0 != pBO->usubdir && (const char **)0 != (ppcCur = (const char **)pBO->ppcsubdir)) {
		for (i = 0, iGuess = ' ' == iMode ? 'x' : iMode; 'x' == iGuess && (char *)0 != ppcCur[i]; ++i) {
			iGuess = GuessMode(ppcCur[i], 'x');
		}
		i = ('x' == iGuess) ? '`' : iGuess;
		QuoteMe(fpOut, & iMode, i);
		fprintf(fpOut, "(cd \"$1\" && mkdir -p");
		for (i = 0; (char *)0 != ppcCur[i]; ++i) {
			ppcDup = util_ppm_size(& PPMList, (uDx|15)+1);
			if (InList(ppcDup, (char *)ppcCur[i])) {
				continue;
			}
			ppcDup[uDx++] = (char *)ppcCur[i];
			fprintf(fpOut, " \"");
			LocalWord(fpOut, & iMode, (char *)ppcCur[i]);
			fprintf(fpOut, "\"");
		}
		fprintf(fpOut, ") || exit 73\n");
	}
	if (0 != pBO->usend && (const char **)0 != (ppcCur = (const char **)pBO->ppcsend)) {
		for (i = 0, iGuess = ' ' == iMode ? 'x' : iMode; 'x' == iGuess && (char *)0 != ppcCur[i]; ++i) {
			iGuess = GuessMode(ppcCur[i], 'x');
		}
		i = ('x' == iGuess) ? '`' : iGuess;
		QuoteMe(fpOut, & iMode, i);
		for (i = 0; (char *)0 != ppcCur[i]; ++i) {
			if (InList((char **)ppcMap, ppcCur[i]))
				continue;
			LocalDir(fpOut, pBO->ppctarg[i], & iMode, & PPMList, & uDx);
			fprintf(fpOut, "cp %s\"", ('-' == ppcCur[i][0]) ? "-- " : "");
			LocalWord(fpOut, & iMode, (char *)ppcCur[i]);
			fprintf(fpOut, "\" $1/");
			LocalWord(fpOut, & iMode, (char *)pBO->ppctarg[i]);
			fprintf(fpOut, "\n");
		}
	}
	for (i = 0; i < pBO->uorig; ++i) {
		QuoteMe(fpOut, & iMode, '`');
		LocalDir(fpOut, pBO->ppcmap[i], & iMode, & PPMList, & uDx);
		fprintf(fpOut, "cp -p 'defn(`HXMD_%u')` $1/\"", i+1);
		LocalWord(fpOut, & iMode, pBO->ppcmap[i]);
		fprintf(fpOut, "\"\n");
	}
	QuoteMe(fpOut, & iMode, ' ');
	fprintf(fpOut, "dnl before PRE\ndivert(%d)dnl\n", iFirstDiv+6);
	fprintf(fpOut, "ifdef(`PRE_CMD',`PRE_CMD\n')dnl\n");
	QuoteMe(fpOut, & iMode, '`');
	fprintf(fpOut, "(cd $1 || exit 69\n");
	QuoteMe(fpOut, & iMode, ' ');
	fprintf(fpOut, "ifdef(`ENTRY_DEFS',`. defn(`ENTRY_DEFS') && ')$SHELL -c \"");
	for (i = 0; (char *)0 != argv[i]; ++i) {
		if (0 != i) {
			fprintf(fpOut, " ");
		}
		LocalWord(fpOut, & iMode, (char *)argv[i]);
	}
	fprintf(fpOut, "\"\n");
	QuoteMe(fpOut, & iMode, ' ');
	fprintf(fpOut, ")\ndnl before POST\ndivert(%d)dnl\n", iFirstDiv+8);
	fprintf(fpOut, "ifdef(`POST_CMD',`POST_CMD\n')dnl\n");
	fprintf(fpOut, "`exit\n'dnl\n");
}

/* Output a given word as a singe word to the remote shell		(ksb)
 * We need a backslash for shell metas \$ -> $, so in C
 * we need to spell it as two.
 */
static void
RemoteWord(FILE *fpOut, int *piMode, char *pcScan)
{
	static char acRemoteBack[] = "\\";
	register int c;

	for (/* above */;'\000' != (c = *pcScan); ++pcScan) {
		switch (c) {
		case '\n': case '\r':	/* you mean ';' */
			c = ';';
			/* fall through */
		case '\\':	/* shell special, backslash it */
		case '$': case '\"':
		case '&': case '|': case ';':
		case '(': case ')':
		case '#':
		case '<': case '>':
		case '~': case '?': case '*':
		case ' ': case '\t':
			fprintf(fpOut, "%s%c", acRemoteBack, c);
			break;
		case '`':	/* shell and m4 special */
		case '\'':	/* shell and m4 special */
			QuoteMe(fpOut, piMode, '[');
			fprintf(fpOut, "%s%c", acRemoteBack, c);
			break;
		case '[':	/* might be in the other quotes */
		case ']':
			QuoteMe(fpOut, piMode, '`');
			fprintf(fpOut, "%s%c", acRemoteBack, c);
			break;
		default: /* @!^_-+={}:,./  isalpha isdigit */
			putc(c, fpOut);
			break;
		}
	}
}

/* Build the per-host command stream for a remote update		(ksb)
 * We are file0 (under -F0) and file
 */
static void
RemoteShell(FILE *fpOut, const BOOTY *pBO, unsigned int uProvision, int argc, char **argv)
{
	register int i;
	register char *pcScan, **ppcSubDir;
	auto int iMode;

	ppcSubDir = pBO->ppcsubdir;
	/* Fixed header to allow some custom code from the -Z hook
	 */
	/* set ${1} ${2} ${3} ${4}, ${5} as in local mode
	 *	local cache directory || "."
	 *	name of the makefile
	 *	MODE ("local" || "remote")
	 *	[ENTRY_LOGIN@]HOST
	 *	INTO
	 */
	fprintf(fpOut, "dnl remote per-host update script for hxmd\n`#!/bin/sh\n");
	fprintf(fpOut, "'dnl include shell functions and markup\ndivert(%d)dnl\n", iFirstDiv);
	fprintf(fpOut, "`set . \"");
	iMode = '`';
	LocalWord(fpOut, &iMode, pcMakefile);
	fprintf(fpOut, "\" \"%s\" \"", pBO->pcmode);
	QuoteMe(fpOut, & iMode, ' ');
	fprintf(fpOut, "ifdef(`ENTRY_LOGIN',`ENTRY_LOGIN'@)HOST\" \"");
	LocalWord(fpOut, &iMode, pBO->pcinto);
	QuoteMe(fpOut, & iMode, ' ');
	fprintf(fpOut, "\"\ndnl after params\nifdef(`INCLUDE_CMD',`INCLUDE_CMD(`%s')\n')dnl\ndivert(%d)dnl\n", pBO->pcmode, iFirstDiv+2);
	fprintf(fpOut, "ifdef(`SSH',`',`define(SSH,`ssh')')dnl\n");
	fprintf(fpOut, "ifdef(`RSH_PATH',`',`define(RSH_PATH,`SSH')')dnl\n");
	fprintf(fpOut, "ifdef(`RDIST_PATH',`',`define(RDIST_PATH,`rdist')')dnl\n");
	fprintf(fpOut, "ifdef(`SDIST',`',`define(SDIST,`RDIST_PATH `'ifdef(`RSH_PATH',`-P`'RSH_PATH') ifdef(`RDISTD_PATH',`-p`'RDISTD_PATH') -f')')dnl\n");

#if SMSRC_COMPAT
	/* The old secure master source script sourced local.defs on both
	 * sides, which was a mistake. -- ksb
	 */
	fprintf(fpOut, "ifdef(`ENTRY_DEFS',`',`define(ENTRY_DEFS,`/usr/local/lib/distrib/local.defs')')dnl\n");
#endif
	fprintf(fpOut, "dnl before INIT\ndivert(%d)dnl\n", iFirstDiv+4);
	fprintf(fpOut, "ifdef(`INIT_CMD',`INIT_CMD\n')dnl\n");
	fprintf(fpOut, "dnl myself, subdirs, then files:\nSDIST HXMD_%u myself || exit $?\n", uProvision);
	if ((char **)0 != ppcSubDir && (char *)0 != ppcSubDir[0]) {
		fprintf(fpOut, "SDIST HXMD_%u dirs\n", uProvision);
	} else {
		fprintf(fpOut, "dnl SUBDIR is empty\n");
	}
	if (0 != pBO->umap || 0 != pBO->usend) {
		fprintf(fpOut, "SDIST HXMD_%u files\n", uProvision);
	} else {
		fprintf(fpOut, "dnl SEND and MAP are empty\n");
	}
	fprintf(fpOut, "dnl before PRE\ndivert(%d)dnl\n", iFirstDiv+6);
	fprintf(fpOut, "ifdef(`PRE_CMD',`PRE_CMD\n')dnl\n");

	/* Now the hard part quote the command given so the shell on
	 * the remote host parses it correctly after the m4 and the local
	 * shell that runs the ssh. -- ksb
	 *
	 * We are in m4 quotes, then we're going from a local shell to ssh
	 * (assume Borne-like here), then eval/exec'd by the remote shell.
	 */
	fprintf(fpOut, "SSH ifdef(`ENTRY_LOGIN',`ENTRY_LOGIN@')HOST ifdef(`ENTRY_DEFS',`. defn(`ENTRY_DEFS') \\&\\& ')");
	for (i = 0, iMode = GuessMode(pBO->pcinto, 'x'); 'x' == iMode && i < argc; ++i) {
		iMode = GuessMode(argv[i], 'x');
	}
	i = ('x' == iMode) ? '`' : iMode;
	iMode = ' ';
	QuoteMe(fpOut, & iMode, i);
	fprintf(fpOut, "cd ");
	RemoteWord(fpOut, & iMode, pBO->pcinto);
	fprintf(fpOut, " \\&\\&");
	for (; argc > 0 && (char *)0 != (pcScan = *argv); ++argv, --argc) {
		fprintf(fpOut, " ");
		if ('\000' == *pcScan) {
			fprintf(fpOut, "\\\"\\\"");
			continue;
		}
		RemoteWord(fpOut, & iMode, pcScan);
	}
	QuoteMe(fpOut, & iMode, ' ');
	fprintf(fpOut, "\ndnl before POST\ndivert(%d)dnl\n", iFirstDiv+8);
	fprintf(fpOut, "ifdef(`POST_CMD',`POST_CMD\n')dnl\n");
}

static char
	*pcProvo = (char *)0,
	*pcPerHost = (char *)0,
	*pcDirList = (char *)0,
	*pcPtbwSock = (char *)0;

/* Handle the magicks to make the NEW master source system go.		(ksb)
 * Depends on (-Z) msrc.cf for defaults, we generate some from this code.
 * Rather we just install it from our lib, which might not
 * always be up-to-date with this program.
 */
static void
Msrc(unsigned argc, char **argv)
{
	static char acMisdirect[] = "/dev/null";
	static char acHXEnv[] = "HXMD_PASS";
	register unsigned i;
	register char **ppc;
	register const char *pcTFile, *pcCat;
	register FILE *fpProvo, *fpHCmd;
	auto pid_t wHxmd, wStatus;
	auto int iStatus;
	auto char acTemp[MAXPATHLEN+512];	/* MAX PATH + macro= */
	auto BOOTY BO;
	auto struct stat stInto, stPwd;

	if ((char *)0 == pcMakefile || 0 != Booty(& BO, pcMakefile, fShowProvision ? stdout : (FILE *)0)) {
		fprintf(stderr, "%s: no make recipe file found\n", progname);
		exit(EX_DATAERR);
	}
	if (-1 == stat(".", & stPwd)) {
		fprintf(stderr, "%s: stat: .: %s\n", progname, strerror(errno));
		exit(EX_IOERR);
	} else if (-1 != stat(BO.pcinto, & stInto) && stPwd.st_ino == stInto.st_ino && stPwd.st_dev == stInto.st_dev) {
		fprintf(stderr, "%s: %s: will not target the current working directory\n", progname, BO.pcinto);
		exit(EX_DATAERR);
	}

	if ((char *)0 == BO.pcmode) {
		/* they didn't set it in the recipe file, don't care */
	} else if ('A' == BO.pcmode[0] || 'a' == BO.pcmode[0]) {
		/* asked for "auto", or "any", don't check for one */
	} else if ('L' == BO.pcmode[0] || 'l' == BO.pcmode[0]) {
		fLocal = 1;
	} else if ('R' == BO.pcmode[0] || 'r' == BO.pcmode[0]) {
		if (fLocal) {
			fprintf(stderr, "%s: %s: specifically requests the mode \"%s\", but -l asked for \"local\"\n", progname, pcMakefile, BO.pcmode);
			exit(EX_CONFIG);
		}
	} else {
		fprintf(stderr, "%s: %s: specifically requests the unrecognized mode \"%s\"\n", progname, pcMakefile, BO.pcmode);
		exit(EX_CONFIG);
	}
	BO.pcmode = fLocal ? "local" : "remote";

	if ((char *)0 == (pcProvo = TempMake("provision")) || (-1 == (i = open(pcProvo, O_RDWR|O_CREAT, fLocal ? 0777 : 0666)))) {
		fprintf(stderr, "%s: mktemp: %s/provision: %s\n", progname, pcTmpdir, strerror(errno));
		exit(EX_OSERR);
	}
	if ((FILE *)0 == (fpProvo = fdopen(i, "w"))) {
		fprintf(stderr, "%s: fdopen: %d: %s\n", progname, i, strerror(errno));
		exit(EX_OSERR);
	}
	if ((char *)0 == (pcPerHost = TempMake("hostcmd")) || (-1 == (i = open(pcPerHost, O_RDWR|O_CREAT, 0777)))) {
		fprintf(stderr, "%s: mktemp: %s/hostcmd: %s\n", progname, pcTmpdir, strerror(errno));
		exit(EX_OSERR);
	}
	if ((FILE *)0 == (fpHCmd = fdopen(i, "w"))) {
		fprintf(stderr, "%s: fdopen: %d: %s\n", progname, i, strerror(errno));
		exit(EX_OSERR);
	}

	if (0 == argc || (char **)0 == argv) {
		argv = apcDefCmd;
		argc = sizeof(apcDefCmd)/sizeof(apcDefCmd[0]) - 1;
	}
	pcPtbwSock = pcDirList = acMisdirect;
	if (fLocal) {
		register FILE *fpDirs;
		register char *pcCur;

		if (1 < iParallel) {
			/* make the dirlist, the ptbw socket name
			 * then make the two m4 files, which use those names.
			 */
			if ((char *)0 == (pcDirList = TempMake("dirlist")) || (FILE *)0 == (fpDirs = fopen(pcDirList, "w+"))) {
				fprintf(stderr, "%s: fopen: %s/dirlist: %s\n", progname, pcTmpdir, strerror(errno));
				exit(EX_OSERR);
			}
			for (i = 0; i < iParallel; ++i) {
				if ((char *)0 == (pcCur = TempMake("lsXXXXXX")) || (char *)0 == mkdtemp(pcCur)) {
					fprintf(stderr, "%s: mkdtemp: %s/lsXXXXXX: %s\n", progname, pcTmpdir, strerror(errno));
					exit(EX_OSERR);
				}
				fprintf(fpDirs, "%s\n", pcCur);
			}
			fclose(fpDirs);
			pcPtbwSock = TempMake("broker");
			pcCur = (char *)0;
		} else if ((char *)0 == (pcCur = TempMake("joXXXXXX")) || (char *)0 == mkdtemp(pcCur)) {
			fprintf(stderr, "%s: mkdtemp: %s/joXXXXXX: %s\n", progname, pcTmpdir, strerror(errno));
			exit(EX_OSERR);
		}
		LocalProvision(fpProvo, &BO, argv);
		if (fTrace) {
			fprintf(stderr, "%s: cat <<\\! >%s\n", progname, pcProvo);
			LocalProvision(stderr, &BO, argv);
			fprintf(stderr, "!\n");
			fflush(stderr);
		}
		LocalShell(fpHCmd, &BO, pcPtbwSock, pcCur);
		if (fTrace) {
			fprintf(stderr, "%s: cat <<\\! >%s\n", progname, pcPerHost);
			LocalShell(stderr, &BO, pcPtbwSock, pcCur);
			fprintf(stderr, "!\n");
			fflush(stderr);
		}
	} else {
		RemoteProvision(fpProvo, &BO, ".");
		if (fTrace) {
			fprintf(stderr, "%s: cat <<\\! >%s\n", progname, pcProvo);
			RemoteProvision(stderr, &BO, ".");
			fprintf(stderr, "!\n");
			fflush(stderr);
		}
		RemoteShell(fpHCmd, &BO, BO.umap+1, argc, argv);
		if (fTrace) {
			fprintf(stderr, "%s: cat <<\\! >%s\n", progname, pcPerHost);
			RemoteShell(stderr, & BO, BO.umap+1, argc, argv);
			fprintf(stderr, "!\n");
			fflush(stderr);
		}
	}
	fclose(fpProvo);
	fclose(fpHCmd);

	/* Set $HXMD_PASS if HXINCLUDE is not empty or the string "--"
	 */
	if (fHXFlags && (char *)0 != (pcCat = EnvFromFile(BO.ppchxinclude, BO.uhxinclude))) {
		if ('-' == pcCat[0] && '-' == pcCat[1] && ('\000' == pcCat[2] || (' ' == pcCat[2] && '\000' == pcCat[3])))
			pcCat = "";
		if (fDebug) {
			fprintf(stderr, "%s: $%s=%s\n", progname, acHXEnv, pcCat);
		}
		setenv(acHXEnv, pcCat, 1);
	}

	/* Allocate the final hxmd argv, which is:
	 *	 hxmd [options] [-DENTRY_LOGIN=] -DMSRC_MODE=(local|remote)
	 *		[-Px] [-d ...[SPRX]..] -F0 [--]
	 * followed by
	 *		$pcPerHost $MAPs $pcProvo (char *)0
	 * rounded up to a multiple of 8.
	 */
	ppc = util_ppm_size(& PPMHxmd, (((uHxmd+1+fGaveLogin+fGavePara)+(2+1+1)+(1+BO.umap+1)+1)|7)+1);

	/* inv: ppc = {"ptbw", "-mdt", (char *)0, "-N", (char *)0, "hxmd" ...};
	 * Fill in the ptbw arguments, then discard them if we don't need them.
	 */
	for (i = 0; (char *)0 != ppc[i]; ++i) {
		/* find ptbw -t param slot*/
	}
	ppc[i++] = pcDirList;
	for (/* inv. */; (char *)0 != ppc[i]; ++i) {
		/* find ptbw -N param slot*/
	}
	ppc[i++] = pcPtbwSock;
	if (!fLocal || acMisdirect == pcPtbwSock) {
		uHxmd -= i, ppc += i;
	}

	/* We send our program name unless -Z was provided, because hxmd might
	 * do something More Clever with argv[0] than we ever imagined.
	 */
	if (0 == uZConf) {
		ppc[0] = progname;
	}
	if ((char *)0 != pcDefineMode && (char *)0 != BO.pcmode) {
		(void)strcat(pcDefineMode, BO.pcmode);
		ppc[uHxmd++] = pcDefineMode;
	}
	if (fGaveLogin && '\000' != *pcEntryLogin) {
		snprintf(acTemp, sizeof(acTemp), "-DENTRY_LOGIN=%s", pcEntryLogin);
		ppc[uHxmd++] = strdup(acTemp);
	}
	if (fGavePara) {
		snprintf(acTemp, sizeof(acTemp), "-P%u", iParallel);
		ppc[uHxmd++] = strdup(acTemp);
	}
	ppc[uHxmd++] = "-F0";
	/* How dare you set $TMPDIR to "-busted", only I would do that. (ksb)
	 */
	if ('-' == pcPerHost[0]) {
		ppc[uHxmd++] = "--";
	}
	ppc[uHxmd++] = pcPerHost;
	for (i = 0; i < BO.umap; ++i) {
		ppc[uHxmd++] = BO.ppcorig[i];
	}
	ppc[uHxmd++] = pcProvo;
	ppc[uHxmd] = (char *)0;

	/* Arguments to hxmd complete, show the results if asked (-d X)
	 */
	pcTFile = (!fLocal || acMisdirect == pcPtbwSock) ? acHxmd : acPtbw;
	if (fDebug) {
		fprintf(stderr, "%s: masquerade %s as our name\n", progname, pcTFile);
		fprintf(stderr, "%s:", progname);
		for (i = 0; (char *)0 != ppc[i]; ++i) {
			fprintf(stderr, " %s", ppc[i]);
		}
		fprintf(stderr, "\n");
	}
	fflush(stdout);
	fflush(stderr);

	/* Fork the (ptbw with a nested) hxmd co-process
	 */
	switch (wHxmd = fork()) {
	case -1:
		exit(EX_OSERR);
		/*NOTREACHED*/
	case 0:
		execvp(pcTFile, ppc);
		fprintf(stderr, "%s: execvp: %s: %s\n", progname, acHxmd, strerror(errno));
		exit(EX_OSERR);
		/*NOTREACHED*/
	default:
		break;
	}
	iStatus = EX_UNAVAILABLE << 8;
	while (-1 != (wStatus = wait((void *)& iStatus)) || EINTR == errno) {
		if (wStatus == wHxmd)
			break;
	}
	if ((char *)0 == pcMakeClean) {
		exit(WEXITSTATUS(iStatus));
	}

	/* Under a clean target you get the exit code from make, but
	 * make gets the _raw_ exit from the hxmd run.  This lets you
	 * check for a signal or whatever too. -- ksb
	 *
	 * make -s -f makefile pcMakeClean (char *)0
	 */
	snprintf(acTemp, sizeof(acTemp), "-DMSRC_EXITCODE=%d", iStatus);
	if ((char **)0 == (ppc = calloc((5|7)+9, sizeof(char *)))) {
		fprintf(stderr, "%s: out of memory\n", progname);
		exit(EX_OSERR);
	}
	i = 0;
	ppc[i++] = "make";
	ppc[i++] = "-s";	/* should be able to defeat this */
	ppc[i++] = "-f";
	ppc[i++] = pcMakefile;
	ppc[i++] = acTemp;
	ppc[i++] = pcMakeClean;
	ppc[i] = (char *)0;
	ExecMake(ppc);
	/* no return */
}
%%
