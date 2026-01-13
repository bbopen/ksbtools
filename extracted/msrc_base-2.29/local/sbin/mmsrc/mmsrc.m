#!mkcmd
# $Id: mmsrc.m,v 1.61 2010/08/14 18:22:56 ksb Exp $
# Bootstrap code to get msrc working on a new master source host	(ksb)
#
# You'll need the C code this program builds (from mkcmd) to even build
# mkcmd.  Which I know is odd, but that's the way it is.  This code
# produces the program "mmsrc.c" which is the 2008 version of msrc0.
# Compiled on a target host it gets you enough spell points to get going.
#
# We run the equiv of msrc -l for you, only for this single host, w/o
# any loop-back network connections.  To do that we can use a distrib
# configuration or a -DHOST= this host by name or `localhost'. -- ksb
#
# $Compile: ${mkcmd:-mkcmd} -I../msrc -I../hxmd -G -n mmsrc %f
comment "%c %kCompile: $%{cc-cc%} -g -Wall -o %%[f.-$] %%f %k%k"
from '<sys/types.h>'
from '<sys/param.h>'
from '<sys/socket.h>'
from '<sys/mman.h>'
from '<sys/wait.h>'
from '<fcntl.h>'
from '<sys/time.h>'
from '<sys/uio.h>'
from '<string.h>'
from '<unistd.h>'
from '<errno.h>'
from '<sys/stat.h>'
from '<signal.h>'
from '<sysexits.h>'
from '<time.h>'
from '<netdb.h>'


basename "mmsrc" ""

from '"config.h"'
from '"machine.h"'
require "util_errno.m" "util_ppm.m"

require "std_help.m" "std_version.m"
require "evector.m" "hostdb.m" "slot.m"
require "envfrom.m" "make.m"

getenv ;
char* named "pcETmp" { local hidden }
init 8 'if ((char *)0 != (pcETmp = getenv("MMSRC_PASS")) && \'\\000\' != *pcETmp) {u_envopt(pcETmp, & argc, & argv);} else if ((char *)0 != (pcETmp = getenv("MMSRC"))) {u_envopt(pcETmp, & argc, & argv);}'

augment action 'V' {
	user "Version();"
}
augment char* 'm' {
	key 1 {
		'"__msrc"'	# use the same default -m token, not __mmsrc
	}
}
%i
static char rcsid[] =
	"$Id: mmsrc.m,v 1.61 2010/08/14 18:22:56 ksb Exp $";

static char
	acHxmdPath[] = "HXMD_PATH",
	acMMEnv[] = "MMSRC_PASS";
static int
	fShowProvision = 0,
	fScriptX = 0,
	fDebug = 0,
	iFirstDiv = 1;
static char acMySpace[] = "/tmp/ms02XXXXXX";

/* hxmd forces slot-0 to be +x, we don't
 */
#undef SLOT_0_ORBITS

%%
augment action "h" {
	user 'printf("%%s: also any database option to hxmd\\n", %b);'
}


function 'DIU' {
	named "AddSwitch"
	param "m4-opts"
	help "pass an option down to m4"
}
type "argv" 'j' {
	named "pPPMM4Prep"
	param "m4prep"
	help "include specified files in every m4 process\'s arguments"
}
action 'b' {
	track 'uGaveB'
	help "output the list of words in the given make macro"
	exclude 'DIUjuCBEGXZkoY'
	list {
		update "Msync(%#, %@);"
		aborts "exit(EX_OK);"
		param "macro"
		help "output words from each make macro"
	}
	update "/* in our list */"
}
char* 'd' {
	named "pcDebug"
	param "flags"
	user "DebugFilter(%n);"
	help "debug flags we pass on to m4 ('?' for help)"
}
# We need to be compatible with any local HXMD macro munging
char* 'M' {
	hidden named "pcXM4Define"
	init '"HXMD_"'
	help "macro filename prefix for m4 cross file references"
}
int named "bHasRoot" {
	init dynamic "getuid() == 0"
}

after {
	hidden named "AfterGlow"
	update "%n(argc, argv);"
	help "check for re-exec cases"
}
boolean 'z' {
	named "fHXFlags"
	init "1"
	help "ignore HXINCLUDE from makefile"
}
list {
	named "List"
	param "utility"
	help "run utility in the platform directory"
}
init 5 "MmsrcStart();"

# optiions we consume because we are being nice
unsigned 'P' {
	hidden local named "iParallel"
	track "fGavePara"
	init "1"
	help "number of tasks to run in parallel, default %qi"
	param "[jobs]"
	noparam "if (%ypi == (%N = getenv(\"PARALLEL\")) || '\\000' == *%N) {%N = \"1\";}"
	after "if (%n < 1) {%n = 1;}"
}
boolean 'ls' {
	hidden named "fLocalForced"
	update "%n |= %w == 'l';"
	help "consumed to be more compatible with msrc"
}
before {
	update "/* used to move -l up to before AfterGlow */"
	after 'if (%rln) {u_argv_add(%ryn, "MODE=local");}'
}
function 'u' {
	named "SetEntryLogin"
	param 'login'
	help "shorthand to define ENTRY_LOGIN, for msrc compatiblity"
}
%c
static PPM_BUF PPMM4;
static unsigned int uM4c = 0;
static int
	fShowExec = 0,		/* show (fork'd and) exec'd processes	*/
	fShowRm = 0;		/* show tmp space management		*/

/* cleanup the wrapper directory if it is empty				(ksb)
 */
static void
MmsrcExit()
{
	if (0 == rmdir(acMySpace) && fShowRm) {
		fprintf(stderr, "%s: rmdir %s\n", progname, acMySpace);
	}
	TempNuke();
}

/* Any init actions we need						(ksb)
 * Strangely we can't masquerade as "msrc" when called "mmsrc", because
 * there might not be any zero configuration files installed (yet).
 * You'll have to explicitly rename us as "msrc" if you want that to
 * happen, or add "-Z msrc.cf" on the command-line (or in $MMSRC).
 */
static void
MmsrcStart()
{
	extern char *mkdtemp(char *);
	register char **ppc;
	auto char acTemp[MAXPATHLEN+4];

	if ((char *)0 == mkdtemp(acMySpace)) {
		fprintf(stderr, "%s: mkdtemp: %s: %s\n", progname, acMySpace, strerror(errno));
		exit(EX_OSERR);
	}
	if (fShowRm) {
		fprintf(stderr, "%s: mkdir %s\n", progname, acMySpace);
	}
	(void)atexit(MmsrcExit);
	if ((char *)0 == pcZeroConfig ? (0 == strcmp("-", progname) || 0 == strcmp("stdin", progname)) : (0 == strcmp("-", pcZeroConfig))) {
		pcZeroConfig = "-";
	} else if ((char *)0 == pcZeroConfig && 0 != strcmp("mmsrc", progname)) {
		snprintf(acTemp, sizeof(acTemp)-1, "%s.cf", progname);
		pcZeroConfig = strdup(acTemp);
	}
	util_ppm_init(& PPMM4, sizeof(char *), 256);
	ppc = util_ppm_size(& PPMM4, 256);
	uM4c = 0;
	ppc[uM4c++] = "m4";
}

static char *apcDefUtil[] = { "make", (char *)0 };

/* Add our voice to the version info, important because we can't be	(ksb)
 * rebuilt until all of mkcmd, explode, install, ptbw, xclate, xapply,
 * hxmd, and msrc are built (that's 11 directories with lib's).
 */
static void
Version()
{
	register int i;

	printf("%s:  makefile code: %s\n", progname, rcs_make);
	printf("%s:  slot code: %s\n", progname, rcs_slot);
	printf("%s:  hostdb code: %s\n", progname, rcs_hostdb);
	printf("%s:  evector code: %s\n", progname, rcs_evector);
	if ((char *)0 == pcMakefile) {
		printf("%s: no make recipe file\n", progname);
	} else {
		printf("%s: makefile: %s\n", progname, pcMakefile);
	}
	printf("%s: make hook: %s", progname, pcMakeHook);
	if ((char *)0 != pcMakeClean)
		printf(":%s", pcMakeClean);
	printf("\n%s: default utility:", progname);
	for (i = 0; i < sizeof(apcDefUtil)/sizeof(apcDefUtil[0])-1; ++i) {
		printf(" %s", apcDefUtil[i]);
	}
	printf("\n%s: temporary directory template: %.*sXXXXXX\n", progname, (int)strlen(acMySpace)-6, acMySpace);
	printf("%s: cache recipe file: %s\n", progname, acDownRecipe);
}

/* Remove tail component of the hostname until it matches.		(ksb)
 * Eats the input string, and should use strcasecmp(3), really -- ksb
 */
static META_HOST *
FindTarget(META_HOST *pMHList, char *pcWhole)
{
	register char *pcTail;
	register META_HOST *pMH;

	pcTail = strlen(pcWhole)+pcWhole;
	do {
		*pcTail = '\000';
		for (pMH = pMHList; (META_HOST *)0 != pMH; pMH = pMH->pMHnext) {
			if (0 == strcmp(pMH->pchost, pcWhole))
				return pMH;
		}
		pcTail = strrchr(pcWhole, '.');
	} while ((char *)0 != pcTail);
	return (META_HOST *)0;
}

#if !defined(HOST_NAME_MAX)
#define	HOST_NAME_MAX	(MAXHOSTNAMELEN)
#endif

/* Find a likely name for myself in a list of meta hosts		(ksb)
 *	if there is one host in the matched set, that must be us;
 *		(that is to say the invocation sepcified on the command line)
 *	fetch gethostname and search for that name;
 *	search for "localhost.domain.from.above"
 *	else we must be the first host matched (since -Y/-G selected order)
 */
static META_HOST *
FindMyself(META_HOST *pMHList)
{
	register META_HOST *pMHRet;
	register char *pcDot;
	auto char acHostname[HOST_NAME_MAX], acFake[HOST_NAME_MAX];

	if ((META_HOST *)0 == pMHList) {
		fprintf(stderr, "%s: no hosts selected, we can\'t find ourself in an empty list\n", progname);
		exit(EX_CONFIG);
	}
	if ((META_HOST *)0 == pMHList->pMHnext) {
		return pMHList;
	}
	pMHRet = (META_HOST *)0;
	snprintf(acFake, sizeof(acFake), "localhost");
	if (-1 == gethostname(acHostname, sizeof(acHostname))) {
		/* just try localhost */
	} else if ((META_HOST *)0 != (pMHRet = FindTarget(pMHList, acHostname))) {
		return pMHRet;
	} else if ((char *)0 != (pcDot = strchr(acHostname, '.'))) {
		snprintf(acFake, sizeof(acFake), "localhost.%s", pcDot+1);
	}
	if ((META_HOST *)0 == (pMHRet = FindTarget(pMHList, acFake))) {
		pMHRet = pMHList;
	}
	return pMHRet;
}

static char
	*pcMyName = (char *)0;

/* Pass a single letter switch or switch with a param down to m4.	(ksb)
 * copied from msrc's msrc.m, because we trap different options.
 */
static void
AddSwitch(int iOpt, char *pcArg)
{
	register char **ppc;

	/* Also tap -D option stream for some clues we need ... -- ksb
	 * HOST= for the host selection, and INTO= for a different cache
	 */
	if ('D' == iOpt && (char *)0 != pcArg) {
		register unsigned u;
		register char *pcReal;

		pcReal = pcArg;
		if ('_' != *pcArg && !isalnum(*pcArg)) {
			++pcReal;
		}
		if (0 == strncmp("HOST=", pcReal, 5) && '\000' != pcReal[5]) {
			pcMyName = pcArg+5;
		}
		ppc = util_ppm_size(& PPMAutoDef, 0);
		for (u = 0; (char *)0 != ppc[u]; ++u) {
			/* search for end */
		}
		ppc = util_ppm_size(& PPMAutoDef, ((u+1)|15)+1);
		ppc[u++] = pcArg;
		ppc[u] = (char *)0;
		pcArg = pcReal;
	}
	ppc = util_ppm_size(& PPMM4, (uM4c|7)+9);
	if ('\000' != iOpt) {
		ppc[uM4c] = strdup("-_");
		ppc[uM4c++][1] = iOpt;
	}
	if ((char *)0 != pcArg) {
		ppc[uM4c++] = ('I' == iOpt && '-' == pcArg[0] &&  '-' == pcArg[1] &&  '\000' == pcArg[2]) ? acTilde : pcArg;
	}
	ppc[uM4c] = (char *)0;
}

/* Make mmsrc look like msrc for the -u option				(ksb)
 */
static void
SetEntryLogin(int iOpt, char *pcArg)
{
	auto char acTemp[MAXPATHLEN+512];

	if ('\000' == *pcArg) {
		return;
	}
	snprintf(acTemp, sizeof(acTemp), "ENTRY_LOGIN=%s", pcArg);
	AddSwitch('D', strdup(acTemp));
}

/* Filter the -d options to snoop-out the flags we want to see		(ksb)
 * same as msrc mostly trap 'P' and 'S' and 'X', I suppose 'R' as well.
 */
static void
DebugFilter(const char *pcFlags)
{
	if ((const char *)0 != pcFlags && (char *)0 != strchr(pcFlags, 'P')) {
		++fShowProvision;
		++fShowMake;
	}
	if ((char *)0 != strchr(pcDebug, 'S')) {
		++fScriptX;
		++fShowMake;
	}
	if ((char *)0 != strchr(pcDebug, 'X')) {
		++fShowExec;
		++fDebug;
	}
	if ((char *)0 != strchr(pcDebug, 'R')) {
		++fShowRm;
	}
}

/* Shift the quote mode from `m4' (`), to [m4], to none ' '		(ksb)
 * Request '`' or '[' to get statered from ' ' mode, then
 * requesting ' ' ends the string (and returns the `default' quotes).
 * Normal reset pattern installed for piCur.  [Copied from msrc.m.]
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

/* Output a given word inside shell double quotes			(ksb)
 * sh -c "the local word's we speak"
 */
static void
LocalWord(FILE *fpOut, int *piMode, const char *pcScan)
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

/* Make sure any required subdir of the target is made			(ksb)
 */
static void
LocalDir(FILE *fpOut, BOOTY *pBO, const char *pcTail, int *piMode, PPM_BUF *pPPMList, unsigned *puDx)
{
	register char **ppcUniq, *pcThis, *pcDir;
	register size_t wLen;

	while ('/' == *pcTail) {
		++pcTail;
	}
	wLen = (strlen(pBO->pcinto)+1+strlen(pcTail))|7;
	wLen += 1;
	if ((char *)0 == (pcThis = malloc(wLen))) {
		fprintf(stderr, "%s: malloc: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	*pcThis = '\000';
	for (pcDir = (char *)pcTail; (char *)0 != (pcDir = strchr(pcDir, '/')); ++pcDir) {
		snprintf(pcThis, wLen-1, "%s/%.*s", pBO->pcinto, (int)(pcDir-pcTail), pcTail);
		ppcUniq = util_ppm_size(pPPMList, (*puDx|7)+1);
		ppcUniq[*puDx] = (char *)0;
		if (InList(ppcUniq, pcThis)) {
			*pcThis = '\000';
			continue;
		}
		ppcUniq[(*puDx)++] = strdup(pcThis);
	}
	if ('\000' == *pcThis) {
		return;
	}
	fprintf(fpOut, "mkdir -p ");
	LocalWord(fpOut, piMode, pcThis);
	fprintf(fpOut, "\n");
}

/* Make the remote directory look like the booty says it should		(ksb)
 * Copy the files in like msrc -l does, if we need to (don't update
 * files that are the same, so make(1) works. -- ksb
 */
static void
Lsync(FILE *fpOut, BOOTY *pBO, const char *pcM4ed, char **argv, META_HOST *pMHOnly)
{
	register int i;
	register const char **ppcCur, *pcSep;
	auto int iMode;
	auto PPM_BUF PPMList;
	auto unsigned uDx;
	static char acNoSlot[] = "\000";

	fprintf(fpOut, "dnl mmsrc local update script\n`#!/bin/sh\n'");
	SlotDefs(fpOut, acNoSlot);
	HostDefs(fpOut, pMHOnly);
	fprintf(fpOut, "dnl shell functions\ndivert(%d)dnl\n", iFirstDiv);
	iMode = ' ';
	fprintf(fpOut, "`set _ \"%s\" \"%s\" \"%s\" \"'ifdef(`ENTRY_LOGIN',`ENTRY_LOGIN'@)HOST\"` \"%s\"; shift'\n", pBO->pcinto, pcMakefile, pBO->pcmode, pBO->pcinto);
	fprintf(fpOut, "ifdef(`INCLUDE_CMD',`INCLUDE_CMD(`%s')\n')dnl\ndivert(%d)dnl\n", pBO->pcmode, iFirstDiv+2);
	fprintf(fpOut, "dnl before INIT\ndivert(%d)dnl\nifdef(`INIT_CMD',`INIT_CMD\n')dnl\n", iFirstDiv+4);
	if ((const char **)0 != (ppcCur = (const char **)pBO->ppcsubdir)) {
		QuoteMe(fpOut, &iMode, '`');
		for (i = 0; (char *)0 != ppcCur[i]; ++i) {
			fprintf(fpOut, "mkdir -p ");
			LocalWord(fpOut, & iMode, pBO->pcinto);
			fprintf(fpOut, "/");
			LocalWord(fpOut, & iMode, ppcCur[i]);
			fprintf(fpOut, " || exit 73\n");
		}
	}
	util_ppm_init(& PPMList, sizeof(char *), 256);
	uDx = 0;
	if ((const char **)0 != (ppcCur = (const char **)pBO->ppcsend)) {
		QuoteMe(fpOut, &iMode, '`');
		for (i = 0; (char *)0 != ppcCur[i]; ++i) {
			if (InList(pBO->ppcmap, ppcCur[i]))
				continue;
			LocalDir(fpOut, pBO, pBO->ppctarg[i], & iMode, & PPMList, & uDx);
			fprintf(fpOut, "cmp -s %s", '-' == ppcCur[i][0] ? "-- " : "");
			LocalWord(fpOut, & iMode, ppcCur[i]);
			fprintf(fpOut, " ");
			LocalWord(fpOut, & iMode, pBO->pcinto);
			fprintf(fpOut, "/");
			LocalWord(fpOut, & iMode, pBO->ppctarg[i]);
			fprintf(fpOut, " || cp -f -p %s", '-' == ppcCur[i][0] ? "-- " : "");
			LocalWord(fpOut, & iMode, ppcCur[i]);
			fprintf(fpOut, " ");
			LocalWord(fpOut, & iMode, pBO->pcinto);
			fprintf(fpOut, "/");
			LocalWord(fpOut, & iMode, pBO->ppctarg[i]);
			fprintf(fpOut, "\n");
		}
	}
	if ((const char **)0 != (ppcCur = (const char **)pBO->ppcmap)) {
		QuoteMe(fpOut, &iMode, '`');
		for (i = 0; (char *)0 != ppcCur[i]; ++i) {
			LocalDir(fpOut, pBO, ppcCur[i], & iMode, & PPMList, & uDx);
			fprintf(fpOut, "cmp -s %s", '-' == pcM4ed[0] ? "-- " : "");
			LocalWord(fpOut, & iMode, pcM4ed);
			fprintf(fpOut, " ");
			LocalWord(fpOut, & iMode, pBO->pcinto);
			fprintf(fpOut, "/");
			LocalWord(fpOut, & iMode, ppcCur[i]);
			fprintf(fpOut, " || cp -f -p %s", '-' == pcM4ed[0] ? "-- " : "");
			LocalWord(fpOut, & iMode, pcM4ed);
			fprintf(fpOut, " ");
			LocalWord(fpOut, & iMode, pBO->pcinto);
			fprintf(fpOut, "/");
			LocalWord(fpOut, & iMode, ppcCur[i]);
			fprintf(fpOut, "\n");

			pcM4ed += strlen(pcM4ed)+1;
		}
	}
	QuoteMe(fpOut, &iMode, ' ');
	fprintf(fpOut, "dnl before PRE\ndivert(%d)dnl\nifdef(`PRE_CMD',`PRE_CMD\n')dnl\n", iFirstDiv+6);
	fprintf(fpOut, "ifdef(`ENTRY_DEFS',`. defn(`ENTRY_DEFS')");

	/* Like all wrappers we take ":" for an internal no-op
	 */
	if (':' == argv[0][0] && '\000' == argv[0][1] && (char *)0 == argv[1]) {
		/* internal no-op: don't do anything at all */
		fprintf(fpOut, "')\n");
	} else {
		fprintf(fpOut, " && ')$SHELL -c \"cd ");
		QuoteMe(fpOut, &iMode, '`');
		LocalWord(fpOut, & iMode, pBO->pcinto);
		fprintf(fpOut, "\n");
		pcSep = "";
		while ((char *)0 != *argv) {
			LocalWord(fpOut, & iMode, pcSep);
			LocalWord(fpOut, & iMode, *argv++);
			pcSep = " ";
		}
		QuoteMe(fpOut, &iMode, ' ');
		fprintf(fpOut, "\"\n");
	}
	QuoteMe(fpOut, &iMode, ' ');
	fprintf(fpOut, "dnl before POST\ndivert(%d)dnl\nifdef(`POST_CMD',`POST_CMD\n')dnl\n", iFirstDiv+8);
}

static BOOTY BO;
extern char **environ;

/* Default missing data, debug level, plunder the makefile		(ksb)
 * if ppchxflags holds a config file option we have to add
 * it to our list, tag the fact we did it and re-exec ourself.
 */
static void
AfterGlow(int argc, char **argv)
{
	register const char *pcCat;
	auto PPM_BUF PPMExec;
	register int i, wOuch;
	register char **ppcExec;
	static const char acEmpty[] = "";

	if ((char *)0 != pcDebug) {
		AddSwitch('d', pcDebug);
	} else {
		pcDebug = "";
	}
	if ((char *)0 != strchr(pcDebug, '?')) {
		printf("%s: -d [PRS?]*[aceflqtxV]*\nP\ttrace makefile provisions\nR\ttrace temporary file creation/removal\nS\ttrace shell actions\nX\ttrace execution of subcommands\n?\tdisplay these options and exit\nall others are provided by m4\n", progname);
		exit(0);
	}
	if ((char *)0 == pcMakefile || '\000' == *pcMakefile) {
		fprintf(stderr, "%s: no make recipe file found\n", progname);
		exit(EX_DATAERR);
	}
	if (uGaveB) {
		return;
	}

	/* Plunder the proposed makefile, strangely it might not be
	 * the same makefile we are going to read later ... --ksb
	 */
	if (0 != Booty(& BO, pcMakefile, fShowProvision ? stdout : (FILE *)0)) {
		fprintf(stderr, "%s: %s: cannot read macro values\n", progname, pcMakefile);
		exit(EX_CONFIG);
	}

	/* Set $MMSRC_PASS if HXINCLUDE is not empty or the string "--", and
	 * re-exec ourself with -z.  We are kind of a wrapper, kinda.
	 */
	if (!fHXFlags || 0 == BO.uhxinclude || (const char *)0 == (pcCat = EnvFromFile(BO.ppchxinclude, BO.uhxinclude))) {
		setenv(acMMEnv, acEmpty, 1);
		return;
	}
	if ('-' == pcCat[0] && '-' == pcCat[1] && ('\000' == pcCat[2] || (' ' == pcCat[2] && '\000' == pcCat[3]))) {
		pcCat = acEmpty;
	}
	if (fDebug) {
		fprintf(stderr, "%s: $%s=%s\n", progname, acMMEnv, pcCat);
	}
	setenv(acMMEnv, pcCat, 1);
	util_ppm_init(& PPMExec, sizeof(char *), 256);
	ppcExec = util_ppm_size(& PPMExec, ((argc+2)|7)+1);
	i = 0;
	ppcExec[i++] = argv[0];
	ppcExec[i++] = "-z";
	while (argc-- > 0) {
		ppcExec[i++] = *++argv;
	}
	ppcExec[i] = (char *)0;
	fflush(stdout);
	if (fShowExec) {
		fprintf(stderr, "%s: execve:", progname);
		for (i = 0; (char *)0 != ppcExec[i]; ++i) {
			fprintf(stderr, " %s", ppcExec[i]);
		}
		fprintf(stderr, "\n");
	}
	fflush(stderr);
	execvp(ppcExec[0], ppcExec);
	wOuch = errno;
	execve(ppcExec[0], ppcExec, environ);
	fprintf(stderr, "%s: execvp: %s: %s\n", progname, ppcExec[0], strerror(wOuch));
	exit(EX_UNAVAILABLE);
	/*NOTREACHED*/
}

/* Msync needs to know the value of $SOURCE and maybe $GEN from the	(ksb)
 * make recipe, but make -V doesn't exist on Linux (just *BSD). Emmulate it.
 */
static void
Msync(int argc, char **argv)
{
	static MAKE_VAR aMVMsync[] = {
		{""},		/* copy these from this directory	*/
		{(char *)0}
	};
	register char **ppc;

	if (!uGaveReq) {
		pcMakeHook = "__msync";
	}
	while (argc-- > 0) {
		aMVMsync[0].pcname = *argv++;
		if (EX_OK != GatherMake(pcMakefile, aMVMsync)) {
			fprintf(stderr, "%s: -b: %s: target failed\n", progname, pcMakeHook);
			exit(EX_DATAERR);
		}
		if ((char **)0 != (ppc = GetMVValue(& aMVMsync[0]))) {
			for ( ; (char *)0 != *ppc; ++ppc) {
				printf("%s\n", *ppc);
			}
		}
	}
	exit(EX_OK);
}

/* We build a directory (INTO) to cache a configured copy of		(ksb)
 * the msrc spool in this directory (fake msrc -l), then
 * exec the remainder of argv to take some action.  We trap -DHOST
 * to "auto config" because we are used to build the more robust
 * tool chain before the Admin has any domain.cf built.
 */
static void
List(int argc, char **argv)
{
	register int i;
	auto char *pcHostSlot, *pcSlot, **ppcM4, **ppc;
	auto const char *pcCacheCtl;
	auto struct stat stPwd, stInto;
	auto int wExit, *piIsCache;
	auto pid_t wM4, wWait;
	auto unsigned int uSlotLen;
	auto FILE *fpL;
	auto META_HOST *pMHMyself, *pMHAll;
	auto struct stat stExec;

	/* Build the M4 phase buffer for HXMD_PHASE=(selection|%d|cmd),
	 */
	AddSwitch('D', SetupPhase("PHASE", 64));

	/* Put the fixed m4 prep files in the m4 template.
	 * Then add the dash for stdin, so m4 will read our pipe.
	 */
	if ((char **)0 != (ppc = util_ppm_size(pPPMM4Prep, 0))) {
		while ((char *)0 != *ppc) {
			AddSwitch('\000', *ppc++);
		}
	}
	AddSwitch('\000', "-");

	if (0 == argc || (char **)0 == argv) {
		argv = apcDefUtil;
		argc = sizeof(apcDefUtil)/sizeof(apcDefUtil[0]) - 1;
	}
	if (-1 == stat(".", & stPwd)) {
		fprintf(stderr, "%s: stat: .: %s\n", progname, strerror(errno));
		exit(EX_IOERR);
	}

	/* Read configuration data, first the -C/-X/-Z files, then the
	 * local make(1) file for macro values and prereqs (like msrc does).
	 * Then force INTO from any -D on the command line.
	 * Find the cache control slot from hostdb.m, or fail.
	 */
	if ((char *)0 == (pcHostSlot = HostSlot(acMySpace))) {
		exit(EX_DATAERR);
	}
	pcCacheCtl = FindCache("make", pcHostSlot);
	ppcM4 = util_ppm_size(& PPMM4, 0);
	pMHAll = HostInit(ppcM4);

	/* Find our hostname in the provided DB, -DHOST=name else guess.
	 */
	if ((char *)0 != pcMyName) {
		pMHMyself = FindTarget(pMHAll, strdup(pcMyName));
	} else {
		pMHMyself = FindMyself(pMHAll);
	}

	/* Inv. BO.pcinto is not empty or nil, so make sure it is not an alias
	 * for the current working directory.  That would cause us to clobber
	 * our source files, and that would be Poor Form. --ksb
	 */
	if (-1 != stat(BO.pcinto, & stInto)) {
		if (stPwd.st_dev == stInto.st_dev && stPwd.st_ino == stInto.st_ino) {
			fprintf(stderr, "%s: INTO cannot specify the current working directory, use a parallel space\n", progname);
			exit(EX_PROTOCOL);
		}
	} else if (-1 == mkdir(BO.pcinto, (07777 & stPwd.st_mode))) {
		/* We don't (and shan't) mkdir the enclosing directory -- ksb
		 */
		fprintf(stderr, "%s: mkdir: %s: %s\n", progname, BO.pcinto, strerror(errno));
		exit(EX_NOPERM);
	} else {
		(void)chmod(BO.pcinto, 0700 | (07777 & stPwd.st_mode));
	}
	if ((char *)0 == BO.pcmode || 'a' == BO.pcmode[0] || 'A' == BO.pcmode[0]) {
		/* don't look */
	} else if ('l' != BO.pcmode[0] && 'L' != BO.pcmode[0]) {
		fprintf(stderr, "%s: %s: requests \"%s\" mode, but \"local\" is all that is supported\n", progname, pcMakefile, BO.pcmode);
		exit(EX_UNAVAILABLE);
	}
	BO.pcmode = "local";

	/* Get our m4 processing setup al-la hxmd, do the local sync,
	 * change directory to that cache remove the temp files and
	 * run (exec) the command we wrap.
	 */
	if ((char *)0 == (pcSlot = malloc(sizeof(char)*BO.uorig*(MAXPATHLEN+4)))) {
		fprintf(stderr, "%s: malloc: %lu: %s\n", progname, (unsigned long)(sizeof(char)*BO.uorig*(MAXPATHLEN+4)), strerror(errno));
		exit(EX_OSERR);
	}
	if ((char **)0 == BO.ppcorig) {
		static char *pcSentinel;

		pcSentinel = (char *)0;
		BO.ppcorig = & pcSentinel;
	}
	uSlotLen = InitSlot("XXXXXX", pcSlot, BO.ppcorig, acMySpace);
	if ((int *)0 == (piIsCache = calloc((uSlotLen|3)+1, sizeof(int)))) {
		fprintf(stderr, "%s: calloc: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	/* do that same checks hxmd does for our files, don't process stdin
	 */
	for (i = 0; i < BO.uorig; ++i) {
		register int iOK;
		auto struct stat stCheck;
		auto char *pcFound, *apcLook[2];
		auto char acRecipe[MAXPATHLEN+4];

		piIsCache[i] = 0;
		if ('-' == BO.ppcorig[i][0] && '\000' == BO.ppcorig[i][1]) {
			fprintf(stderr, "%s: MAP: -: cannot process stdin (yet)\n", progname);
			exit(EX_SOFTWARE);
		}
		apcLook[0] = BO.ppcorig[i];
		apcLook[1] = (char *)0;
		if ('.' == BO.ppcorig[i][0] && '/' == BO.ppcorig[i][1]) {
			/* explicity do not search for file */
		} else if ((char *)0 == (pcFound = util_fsearch(apcLook, getenv(acHxmdPath)))) {
			fprintf(stderr, "%s: %s: cannot find file\n", progname, BO.ppcorig[i]);
		} else {
			BO.ppcorig[i] = pcFound;
		}
		if (0 != stat(BO.ppcorig[i], & stCheck)) {
			fprintf(stderr, "%s: stat: %s: %s\n", progname, BO.ppcorig[i], strerror(errno));
			exit(EX_NOINPUT);
		}
		if (S_ISDIR(stCheck.st_mode)) {
			snprintf(acRecipe, sizeof(acRecipe), "%s/%s", BO.ppcorig[i], acDownRecipe);
			if ((char *)0 == (BO.ppcorig[i] = strdup(acRecipe))) {
				fprintf(stderr, "%s: strdup: %s\n", progname, strerror(errno));
				exit(EX_TEMPFAIL);
			}
			if (0 != stat(BO.ppcorig[i], & stCheck)) {
				fprintf(stderr, "%s: stat: %s: %s\n", progname, BO.ppcorig[i], strerror(errno));
				exit(EX_NOINPUT);
			}
			piIsCache[i] = 1;
		}
		if (S_ISFIFO(stCheck.st_mode)) {
			/* hxmd allows it */
		} else if (!S_ISREG(stCheck.st_mode)) {
			fprintf(stderr, "%s: %s: must be a plain file, or FIFO\n", progname, BO.ppcorig[i]);
			exit(EX_PROTOCOL);
		} else if (-1 == (iOK = open(BO.ppcorig[i], O_RDONLY, 0666))) {
			fprintf(stderr, "%s: open: %s: %s\n", progname, BO.ppcorig[i], strerror(errno));
			exit(EX_OSERR);
		} else {
			close(iOK);
		}
	}
	if (0 == (wWait = FillSlot(pcSlot, BO.ppcorig, ppcM4, pMHMyself, piIsCache, pcCacheCtl))) {
		/* nada */
	} else if (WIFEXITED(wWait)) {
		fprintf(stderr, "%s: %s: %s: exits code %d\n", progname, pMHMyself->pchost, ppcM4[0], WEXITSTATUS(wWait));
		exit(EX_NOHOST);
	} else {
		fprintf(stderr, "%s: %s: %s: caught signal %d\n", progname, pMHMyself->pchost, ppcM4[0], WTERMSIG(wWait));
		exit(EX_NOHOST);
	}
	stExec.st_mode = 0700;
	stExec.st_uid = getuid();
	stExec.st_gid = getgid();
	SetCurPhase("cmd");
	if ((FILE *)0 == (fpL = M4Gadget(pcHostSlot, &stExec, &wM4, ppcM4))) {
		fprintf(stderr, "%s: fopen: %s: %s\n", progname, pcHostSlot, strerror(errno));
		exit(EX_OSERR);
	}
	if (fScriptX) {
		Lsync(stderr, & BO, pcSlot, argv, pMHMyself);
	}
	Lsync(fpL, & BO, pcSlot, argv, pMHMyself);
	fclose(fpL);
	SetCurPhase((char *)0);
	wExit = EX_UNAVAILABLE << 8;
	while (-1 != (wWait = wait(&wExit))) {
		if (wWait == wM4)
			break;
	}
	system(pcHostSlot);
	CleanSlot(pcSlot);
	CleanSlot(pcHostSlot);
	if ((char *)0 == pcMakeClean) {
		exit(WEXITSTATUS(wExit));
	}

	{
	auto char acTemp[MAXPATHLEN+256];

	/* Under a clean target you get the exit code from make, but
	 * make gets the _raw_ exit from the hxmd run.  This lets you
	 * check for a signal or whatever too. -- ksb
	 *
	 * make -s -f makefile pcMakeClean (char *)0
	 */
	snprintf(acTemp, sizeof(acTemp), "-DMSRC_EXITCODE=%d", wExit);
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
	MmsrcExit();
	ExecMake(ppc);
	}
}
%%
