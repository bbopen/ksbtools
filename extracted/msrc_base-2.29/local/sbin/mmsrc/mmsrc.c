/* mkcmd generated user interface
 * built by mkcmd version 8.17 Rel
 *  $Compile: ${cc-cc} -g -Wall -o %[f.-$] %f $$
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <signal.h>
#include <sysexits.h>
#include <time.h>
#include <netdb.h>
#include "config.h"
#include "machine.h"
#include <stdlib.h>
#include <sys/un.h>
#include <sys/signal.h>
#include <dirent.h>
#include <pwd.h>
/* from getopt.m */
/* from getopt_key.m */
/* from util_fgetln.m */
#line 9 "util_fgetln.m"
extern char *fgetln(/* FILE *, size_t * */);
/* from mmsrc.m */
#line 57 "mmsrc.m"
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

/* from util_errno.m */
#line 8 "util_errno.m"
extern int errno;
/* from util_ppm.m */
#line 11 "util_ppm.m"
#if !defined(UTIL_PPM_ONCE)
#define UTIL_PPM_ONCE	1
typedef struct PPMnode {
	void *pvbase;		/* present start			*/
	unsigned int uimax;	/* max we can hold now			*/
	unsigned int uiwide;	/* how wide is one element?		*/
	unsigned int uiblob;	/* how many elements do we grow by?	*/
} PPM_BUF;

extern PPM_BUF *util_ppm_init(PPM_BUF *, unsigned int, unsigned int);
extern void *util_ppm_size(PPM_BUF *, unsigned int);
extern void util_ppm_free(PPM_BUF *);
extern int util_ppm_print(PPM_BUF *, FILE *);
#endif
/* from std_help.m */
/* from std_version.m */
/* from evector.m */
#line 7 "evector.m"
#if !defined(EVECTOR_H)
/* A vector of these represents our macro defines, they are in the
 * order defined, because m4 macros are order dependent. Last name is NULL.
 * I know a lexical sort would make updates faster, that's not reason
 * enough to break the order semanitics here -- ksb.
 * We depend on ppm from mkcmd, of course.  And we leak memory like we are
 * inside a CGI!  This is not mean for a long running application.  -- ksb
 */
typedef struct MDnode {
	char *pcname, *pcvalue;		/* name defined as value	*/
} MACRO_DEF;

extern int ScanMacros(MACRO_DEF *, int (*)(char *, char *, void *), void *);
extern void BindMacro(char *, char *);
extern MACRO_DEF *CurrentMacros();
extern char *MacroValue(char *, MACRO_DEF *);
extern void ResetMacros();
extern MACRO_DEF *MergeMacros(MACRO_DEF *, MACRO_DEF *);
#define EVECTOR_H	1
#endif
#line 29 "evector.m"
static char rcs_evector[] =
	"$Id: evector.m,v 1.8 2008/01/26 18:09:06 ksb Exp $";
static void MacroStart();
/* from hostdb.m */
#line 68 "hostdb.m"
typedef struct MHnode {
	unsigned udefs;		/* number of times we were defined	*/
	unsigned iline;		/* line number, also "boolean forbids"	*/
	char *pcfrom;		/* config file we were first mentioned	*/
	char *pcrecent;		/* most recently mentioned file		*/
	char *pchost;		/* our host name from HOST		*/
	int uu;			/* the value we think %u has for the cmd*/
	int istatus;		/* exit code from xclate, under -r|-K	*/
	MACRO_DEF *pMD;		/* our macro definition list		*/
	struct MHnode *pMHnext;	/* traverse active hosts in order	*/
} META_HOST;
#line 81 "hostdb.m"
static char rcs_hostdb[] =
	"$Id: hostdb.m,v 1.75 2010/07/26 17:24:35 ksb Exp $";
static const char *FindCache(const char *, const char *);
static void HostDefs(FILE *, META_HOST *);
static char *HostSlot(char *), *HostTarget(META_HOST *pMHFor);
static META_HOST *NextHost(), *HostInit(char **);
static void BindHost();
static void FdCopy(int, int , char *);
static void AddBoolean(int, char *);
static void AddExclude(int, char *);
static char **ReserveStdin(char **, char *);
void RecurBanner(FILE *fp);
void BuildM4(FILE *fp, char **ppcBody, META_HOST *pMHFor, char *pcPre, char *pcPost);
static PPM_BUF PPMAutoDef;
static char *pcHostSearch;
static char acTilde[MAXPATHLEN+4];
/* from slot.m */
#line 16 "slot.m"
static char rcs_slot[] =
	"$Id: slot.m,v 1.15 2010/08/04 23:08:06 ksb Exp $";
static const char acDownRecipe[] =
	"Cache.m4";

static int InitSlot(char *, char *, char **, const char *);
static void PrintSlot(char *pcSlot, unsigned uLeft, unsigned uRight, int fdOut);
static int FillSlot(char *pcSlot, char **argv, char **ppcM4Argv, META_HOST *pMHThis, const int *piIsCache, const char *pcCacheCtl);
static void SlotDefs(FILE *fpOut, char *pcSlot);
static void CleanSlot(char *pcSlot);
static char *SetupPhase(const char *pcMnemonic, size_t wPad);
static const char *SetCurPhase(const char *pcNow);

static FILE *M4Gadget(const char *pcOut, struct stat *pstLike, pid_t *pwM4, char **ppcM4Argv);
static int MakeGadget(const char *pcOut, struct stat *pstLike, const char *pcRecipe, char *pcTarget, const char *pcDir);

static size_t
	wPhaseLen = 0;
static char
	*pcM4Phase = (char *)0;	/* wPhaseLen character for the m4 pass info */
/* from envfrom.m */
#line 13 "envfrom.m"
extern char *EnvFromFile(char **ppcList, unsigned uCount);
/* from make.m */
#line 46 "make.m"
/* The generic pull macros from a makefile interface
 */
typedef struct MVnode {
	char *pcname;			/* ${name} from the Makefile	*/
	unsigned int flocks;		/* never/always add to this	*/
	unsigned int fsynth;		/* we build from debris left	*/
	unsigned int ucur;		/* number of values found	*/
	/* opaque part */
	PPM_BUF PPMlist;		/* accumulated names		*/
	int afdpipe[2];			/* the result pipe we use	*/
} MAKE_VAR;

/* The specific interface for msrc and mmsrc (makeme2), this is the
 * booty we plunder from the makefile.
 */
typedef struct BOnode {
	char *pcinto;			/* INTO from the -y|makefile	*/
	char *pcmode;			/* MODE from the -y|makefile	*/
	char **ppcsubdir;		/* subdirs from SUBDIR		*/
	char **ppcmap;			/* target names from MAP	*/
	char **ppcorig;			/* source files	from MAP	*/
	char **ppcsend;			/* source files from SEND	*/
	char **ppctarg;			/* target names from SEND	*/
	char **ppcignore;		/* IGNORE + MAP's		*/
	char **ppchxinclude;		/* options to hxmd, viz. db	*/
	unsigned usubdir, umap, uorig, usend, utarg,
		uignore, uhxinclude;	/* counts of the above		*/
	unsigned usplay;		/* count of send != targ elems	*/
} BOOTY;
#line 78 "make.m"
static char rcs_make[] =
	"$Id: make.m,v 1.41 2010/08/14 18:20:21 ksb Exp $";

static void MakeSetHooks(char **ppcPre, char **ppcPost, char *pcParam);
static char *strRstr(const char *pcBig, const char *pcSmall);
static int InList(char **ppcList, const char *pcValue);
static void AddMVValue(MAKE_VAR *, char *);
static unsigned GetMVCount(MAKE_VAR *);
static char **GetMVValue(MAKE_VAR *);
static void ExecMake(char **ppcMake);
extern int GatherMake(const char *, MAKE_VAR *);
static char *TempMake(const char *);
static void TempNuke(void);
extern int fDebug;
static const char acMakeTempPat[] = "mtfcXXXXXX";
static const char *FindMakefile();
static int Plunder(MAKE_VAR *pMVList, unsigned uList, const char *pcRecipes);
static unsigned Booty(BOOTY *pBO, const char *pcRecipes, FILE *fpShow);

static char
	acMapSuf[] = MAP_SUFFIX,
	acInclSuf[] = INCLUDE_SUFFIX;
static MAKE_VAR aMVNeed[] = {
	{"SEND"},			/* copy these from this directory */
#define DX_SEND		0
	{"INTO"},			/* platform source directory name */
#define DX_INTO		1
	{"IGNORE"},			/* but don't send these, */
#define DX_IGNORE	2
	{"MAP"},			/* s/.host$// and m4 process these */
#define DX_MAP		3
	{"SUBDIR"},			/* mkdir, for user processing */
#define DX_SUBDIR	4
	{"HXINCLUDE"},			/* options to hxmd, viz. -C/-X/-Z */
#define DX_HXINCLUDE	5
	{"MODE"},			/* local|remote|auto (-l local) */
#define DX_MODE		6
	{(char *)0}
#define DX__END		7
};
static const char *apcSkipDir[] = {
	"RCS", "CVS", "SCCS", "SVN",
	(const char *)0
};
#if !defined(MAKE_AUTO_PLUS)
#define MAKE_AUTO_PLUS	"+++"
#endif
static const char acAutoPlus[] =
	MAKE_AUTO_PLUS;			/* explicit add here meta token */
#if !defined(MAKE_AUTO_LOCK)
#define MAKE_AUTO_LOCK	"."
#endif
static const char acNoAuto[] =
	MAKE_AUTO_LOCK;			/* explicit never add meta token */
/* from util_fsearch.m */
/* from util_tmp.m */
/* from util_home.m */
/* from argv.m */
#line 1 "getopt.mi"
/* $Id: getopt.mi,v 8.10 2000/05/31 13:16:24 ksb Exp $
 */

#if 1 || 0 || 0
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
#line 1 "util_fsearch.mi"
/* $Id: util_fsearch.mi,v 8.10 1998/09/16 13:37:42 ksb Exp $
 */
extern char *util_fsearch();
#line 1 "util_home.mi"
/* $Id: util_home.mi,v 8.3 2004/12/15 22:20:18 ksb Exp $
 */
extern char *util_home();
char
	*progname = "$Id$",
	*au_terse[] = {
		" [-z] [-B macros] [-C configs] [-d flags] [-D|I|U m4-opts] [-E compares] [-f makefile] [-G guard] [-j m4prep] [-k key] [-m prereq] [-o attributes] [-u login] [-X ex-configs] [-y yoke] [-Y top] [-Z zero-config] [utility]",
		" -b [-z] [-d flags] [-f makefile] [-m prereq] [-y yoke] [macro]",
		" -h",
		" -V",
		(char *)0
	},
	*u_help[] = {
		"b             output the list of words in the given make macro",
		"macro         output words from each make macro",
		"B macros      boolean check, macros must be defined to select host",
		"C configs     files of hosts and attribute macros, colon separated",
		"d flags       debug flags we pass on to m4 ('?' for help)",
		"D|I|U m4-opts pass an option down to m4",
		"E compares    macro value must satisfy given relation to select hosts",
		"f makefile    search this master source makefile for macros",
		"G guard       expressions to select hosts by name",
		"h             print this help message",
		"j m4prep      include specified files in every m4 process's arguments",
		"k key         change the merge macro name from \"HOST\" to key",
		"m prereq      the prerequisite make target to gather source files",
		"o attributes  output format of the merged configuration file",
		"u login       shorthand to define ENTRY_LOGIN, for msrc compatiblity",
		"V             show version information",
		"X ex-configs  add attribute macro data to defined hosts, colon separated",
		"y yoke        force make macro assignments or command-line options",
		"Y top         m4 commands included at the top of the guard processing",
		"z             ignore HXINCLUDE from makefile",
		"Z zero-config configuration file(s) to set default macro values",
		"utility       run utility in the platform directory",
		(char *)0
	},
	*pcTmpdir = (char *)0,
	*pcConfigs = (char *)0,
	*pcDebug = (char *)0,
	*pcMakefile = (char *)0;
struct PPMnode /* void */
	*pPPMGuard = (void *)0;
struct PPMnode /* void */
	*pPPMM4Prep = (void *)0;
char
	*pcHOST = "HOST";
int
	fLocalForced = 0,
	uGaveB = 0,
	uGaveReq = 0,
	fGaveMerge = 0,
	fGavePara = 0,
	fShowMake = 0;
char
	*pcMakeHook = (char *)0,
	*pcMakeClean = (char *)0,
	*pcXM4Define = "HXMD_",
	*pcOutMerge = (char *)0,
	*pcExConfig = (char *)0;
struct PPMnode /* void */
	*pPPMMakeOpt = (void *)0;
struct PPMnode /* void */
	*pPPMSpells = (void *)0;
int
	fHXFlags = 1,
	bHasRoot = 0;
char
	*pcZeroConfig = (char *)0;

#ifndef u_terse
#define u_terse	(au_terse[0])
#endif
static void
u_chkonly(chSlot, chOpt, pcList)
int chSlot, chOpt;
char *pcList;
{
	register int chWas;
	static int sbiOnly['u'-'*'+1];

	chWas = sbiOnly[chSlot-'*'];
	if (chOpt == chWas) {
		fprintf(stderr, "%s: option `-%c\' cannot be given more than once\n", progname, chWas);
		exit(1);
	} else if (0 != chWas) {
		fprintf(stderr, "%s: option `-%c\' forbidden by `-%c\'\n", progname, chOpt, chWas);
		exit(1);
	}
	for (/*parameter*/; '\000' !=  *pcList; ++pcList) {
		sbiOnly[*pcList-'*'] = chOpt;
	}
}

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
/* from util_fgetln.m */
/* from mmsrc.m */
#line 160 "mmsrc.m"
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
/* from util_errno.m */
/* from util_ppm.m */
#line 28 "util_ppm.m"
/* init a PPM, let me know how big each element is			(ksb)
 * and about how many to allocate when we run out, or pick 0 for
 * either and well guess at the other.
 */
PPM_BUF *
util_ppm_init(PPM_BUF *pPPM, unsigned int uiWidth, unsigned int uiBlob)
{
	register unsigned int iTemp;

	if ((PPM_BUF *)0 == pPPM)
		return (PPM_BUF *)0;
	pPPM->uimax = 0;
	pPPM->pvbase = (void *)0;

	pPPM->uiwide = uiWidth ? uiWidth : 1 ;
	iTemp = pPPM->uiwide < 2048 ? 8191-3*pPPM->uiwide : 1024*13-1;
	pPPM->uiblob = uiBlob ? uiBlob : (iTemp/pPPM->uiwide | 3)+1;
	return pPPM;
}

/* resize this buffer and return the new base address.			(ksb)
 * When we fail to resize we do NOT release the previous buffer.
 */
void *
util_ppm_size(PPM_BUF *pPPM, unsigned int uiCount)
{
	register unsigned int uiRound;
	register void *pvRet;

	if ((PPM_BUF *)0 == pPPM) {
		return (void *)0;
	}
	if (pPPM->uimax >= uiCount) {
		return pPPM->pvbase;
	}
	uiRound = uiCount / pPPM->uiblob;
	uiRound *= pPPM->uiblob;

	/* this could overflow, but we can't fix that
	 */
	if (uiRound < uiCount) {
		uiRound += pPPM->uiblob;
	}
	uiCount = uiRound;
	uiRound *= pPPM->uiwide;

	if ((void *)0 == pPPM->pvbase) {
		pvRet = (void *)malloc(uiRound);
	} else {
		pvRet = (void *)realloc(pPPM->pvbase, uiRound);
	}
	if ((void *)0 != pvRet) {
		pPPM->pvbase = pvRet;
		pPPM->uimax = uiCount;
	}
	return pvRet;
}

/* frees the present buffer, but doesn't forget the init		(ksb)
 */
void
util_ppm_free(PPM_BUF *pPPM)
{
	if ((PPM_BUF *)0 == pPPM) {
		return;
	}
	if (0 != pPPM->uimax && (void *)0 != pPPM->pvbase) {
		free(pPPM->pvbase);
		pPPM->pvbase = (void *)0;
	}
	pPPM->uimax = 0;
}

#if defined(DEBUG)
/* for debugging							(ksb)
 */
int
util_ppm_print(PPM_BUF *pPPM, FILE *fpOut)
{
	fprintf(fpOut, "ppm %p", (void *)pPPM);
	if ((PPM_BUF *)0 == pPPM) {
		fprintf(fpOut, " <is NULL>\n");
		return 0;
	}
	fprintf(fpOut, ": <max %u, width %u, blob %u> base %p\n", pPPM->uimax, pPPM->uiwide, pPPM->uiblob, pPPM->pvbase);
	return 1;
}
#endif
/* from std_help.m */
/* from std_version.m */
/* from evector.m */
#line 36 "evector.m"
static PPM_BUF PPMCurMacros;
static unsigned uMacroCount;

/* retsart the macro list						(ksb)
 */
static void
MacroStart()
{
	register MACRO_DEF *pMD;

	uMacroCount = 0;
	util_ppm_init(& PPMCurMacros, sizeof(MACRO_DEF), 128);
	pMD = (MACRO_DEF *)util_ppm_size(& PPMCurMacros, 128);
	pMD->pcname = (char *)0;
	pMD->pcvalue = (char *)0;
}

/* Process the list in order, return the first				(ksb)
 * nonzero result from the Call function
 */
int
ScanMacros(MACRO_DEF *pMDList, int (*pfiCall)(char *, char *, void *), void *pvData)
{
	register int iRet;

	if ((MACRO_DEF *)0 == pMDList) {
		return 0;
	}
	for (iRet = 0; (char *)0 != pMDList->pcname; ++pMDList) {
		if (0 != (iRet = pfiCall(pMDList->pcname, pMDList->pcvalue, pvData)))
			break;
	}
	return iRet;
}

/* Replace the value of the given macro in the current list		(ksb)
 */
void
BindMacro(char *pcName, char *pcValue)
{
	register unsigned i;
	register MACRO_DEF *pMD;

	pMD = util_ppm_size(& PPMCurMacros, 1);
	for (i = 0; i < uMacroCount; ++i, ++pMD) {
		if (0 == strcmp(pMD->pcname, pcName))
			break;
	}
	if (i != uMacroCount) {
		pMD->pcvalue = pcValue;
		return;
	}

	/* Resize and add the new one to the end
	 */
	pMD = util_ppm_size(& PPMCurMacros, ((uMacroCount+1)|31)+1);
	pMD[uMacroCount].pcname = pcName;
	pMD[uMacroCount].pcvalue = pcValue;
	++uMacroCount;
	pMD[uMacroCount].pcname = (char *)0;
	pMD[uMacroCount].pcvalue = (char *)0;
}

/* Save the list as it is now, don't save the undefined ones		(ksb)
 */
MACRO_DEF *
CurrentMacros()
{
	register unsigned i, iData;
	register MACRO_DEF *pMDCur, *pMDRet;

	pMDCur = util_ppm_size(& PPMCurMacros, 0);
	iData = 0;
	for (i = 0; i < uMacroCount; ++i) {
		if ((char *)0 != pMDCur[i].pcvalue)
			++iData;
	}
	if ((MACRO_DEF *)0 == (pMDRet = (MACRO_DEF *)calloc((iData|7)+1, sizeof(MACRO_DEF)))) {
		return (MACRO_DEF *)0;
	}
	iData = 0;
	for (i = 0; i < uMacroCount; ++i) {
		if ((char *)0 == pMDCur[i].pcvalue)
			continue;
		pMDRet[iData].pcname = pMDCur[i].pcname;
		pMDRet[iData].pcvalue = pMDCur[i].pcvalue;
		++iData;
	}
	pMDRet[iData].pcname = (char *)0;
	pMDRet[iData].pcvalue = (char *)0;
	return pMDRet;
}

/* Merge two macro lists, the first wins in any overlap			(ksb)
 * I know we N**2 when we could do better, but the order of the macros
 * is important to m4.dnl
 */
MACRO_DEF *
MergeMacros(MACRO_DEF *pMDLeft, MACRO_DEF *pMDRight)
{
	register MACRO_DEF *pMDCur, *pMDRet;
	register unsigned iLeft, iData, j;

	iData = 0;
	for (pMDCur = pMDLeft; (char *)0 != pMDCur->pcname; ++pMDCur) {
		++iData;
	}
	iLeft = iData;
	for (pMDCur = pMDRight; (char *)0 != pMDCur->pcname; ++pMDCur) {
		++iData;
	}
	if ((MACRO_DEF *)0 == (pMDRet = (MACRO_DEF *)calloc((iData|3)+1, sizeof(MACRO_DEF)))) {
		return (MACRO_DEF *)0;
	}
	iData = 0;
	for (pMDCur = pMDLeft; (char *)0 != pMDCur->pcname; ++pMDCur) {
		pMDRet[iData].pcname = pMDCur->pcname;
		pMDRet[iData].pcvalue = pMDCur->pcvalue;
		++iData;
	}
	for (pMDCur = pMDRight; (char *)0 != pMDCur->pcname; ++pMDCur) {
		for (j = 0; j < iLeft; ++j) {
			if (0 == strcmp(pMDRet[j].pcname, pMDCur->pcname))
				break;
		}
		if (j < iLeft)
			continue;
		pMDRet[iData].pcname = pMDCur->pcname;
		pMDRet[iData].pcvalue = pMDCur->pcvalue;
		++iData;
	}
	return pMDRet;
}

/* Reset the list of bound macros at the top of the next config.	(ksb)
 * Due to the many pointers refernecing the strings we can't free anything.
 */
void
ResetMacros()
{
	register unsigned i;
	register MACRO_DEF *pMDCur;

	pMDCur = util_ppm_size(& PPMCurMacros, 0);
	for (i = 0; i < uMacroCount; ++i) {
		pMDCur[i].pcvalue = (char *)0;
	}
}

/* Recover the value from a fixed macro list for a given name		(ksb)
 * We know the "fixed" list only has defined values, BTW.
 */
char *
MacroValue(char *pcName, MACRO_DEF *pMDList)
{
	for (/*param*/; (char *)0 != pMDList->pcname; ++pMDList) {
		if (0 == strcmp(pMDList->pcname, pcName))
			return pMDList->pcvalue;
	}
	return (char *)0;
}
/* from hostdb.m */
#line 102 "hostdb.m"
/* find the cache file based on the mktemp prefix we want		(ksb)
 */
static const char *
FindCache(const char *pcBase, const char *pcSlot)
{
	register const char *pcEnd;

	for (/* param */; '\000' != pcSlot; pcSlot += strlen(pcSlot)+1) {
		if ((char *)0 == (pcEnd = strrchr(pcSlot, '/')))
			pcEnd = pcSlot;
		else
			++pcEnd;
		if (0 == strncmp(pcBase, pcEnd, strlen(pcBase)-1))
			return pcSlot;
	}
	fprintf(stderr, "%s: hostdb code out of sync with me (no slot named %s)\n",  progname, pcBase);
	exit(EX_SOFTWARE);
	/*NOTREACHED*/
}

/* It is sad that sendfile is so specific				(ksb)
 */
static void
FdCopy(int fdIn, int fdOut, char *pcTag)
{
	auto char acBuffer[8192];
	register int iCc, iOc;
	register char *pc;

	while (0 < (iCc = read(fdIn, acBuffer, sizeof(acBuffer)))) {
		for (pc = acBuffer; iCc > 0; pc += iOc, iCc -= iOc) {
			if (0 < (iOc = write(fdOut, pc, iCc)))
				continue;
			fprintf(stderr, "%s: write: %s: %s\n", progname, pcTag, strerror(errno));
			exit(EX_OSERR);
		}
	}
}

/* Reserve the stdin stream for some use, one of			(ksb)
 *	zero configuration, files, configuration
 * or fetch the pointer (and reset the state machine).
 */
static char **
ReserveStdin(char **ppcIn, char *pcWhat)
{
	static char *pcHolding = (char *)0;
	static char **ppcCur;

	if ((char **)0 == ppcIn) {
		ppcIn = ppcCur;
		ppcCur = (char **)0;
		pcHolding = (char *)0;
		return ppcIn;
	}
	if ((char *)0 != pcHolding) {
		fprintf(stderr, "%s: stdin: invalid multiple uses \"%s\" and \"%s\" conflict\n", progname, pcHolding, pcWhat);
		exit(EX_USAGE);
	}
	pcHolding = pcWhat;
	return ppcCur = ppcIn;
}

static char *pcWhichFile;
static unsigned iLine;

/* Copy the m4 string, stop at the first unbalanced close quote		(ksb)
 * return the number of characters in the string.
 */
static void
M4String(FILE *fpCursor, char *pcConvert)
{
	register unsigned uDepth;
	register int c, iErrLine;

	iErrLine = iLine;
	for (uDepth = 1; EOF != (c = getc(fpCursor)); ++pcConvert) {
		*pcConvert = c;
		if ('\n' == c) {
			++iLine;
		} else if ('`' == c) {
			++uDepth;
		} else if ('\'' == c && 0 == --uDepth) {
			break;
		}
	}
	if ('\000' == c) {
		fprintf(stderr, "%s: %s:%d unterminalted m4 string\n", progname, pcWhichFile, iErrLine);
		exit(EX_DATAERR);
	}
	*pcConvert = '\000';
}

/* Convert the C string into internal data, find the length of		(ksb)
 * the input representation.
 */
static void
CString(FILE *fpCursor, char *pcConvert)
{
	register int i, c;
	auto int iErrLine;

	iErrLine = iLine;
	while (EOF != (c = getc(fpCursor))) {
		*pcConvert++ = c;
		if ('\"' == c) {
			--pcConvert;
			break;
		}
		if ('\\' != c)
			continue;
		c = getc(fpCursor);
		switch (c) {
		case EOF:
			break;
		case '\n':	/* \ followed by newline is the empty string */
			++iLine;
			--pcConvert;
			continue;
		case 'e':	/* \e -> \ */
			continue;
		case 'n':
			pcConvert[-1] = '\n';
			continue;
		case 't':
			pcConvert[-1] = '\t';
			continue;
		case 'b':
			pcConvert[-1] = '\b';
			continue;
		case 'r':
			pcConvert[-1] = '\r';
			continue;
		case 'f':
			pcConvert[-1] = '\f';
			continue;
		case 'v':
			pcConvert[-1] = '\v';
			continue;
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
			pcConvert[-1] = c - '0';
			for (i = 1; i < 3; ++i) {
				c = getc(fpCursor);
				if ('0' > c || c > '7') {
					ungetc(c, fpCursor);
					break;
				}
				pcConvert[-1] <<= 3;
				pcConvert[-1] += c - '0';
			}
			continue;
		case '8': case '9': case '\\': case '\"': case '\'':
		default:
			pcConvert[-1] = c;
			continue;
		}
		break;
	}
	if ('\000' == c) {
		fprintf(stderr, "%s: %s:%d unterminalted C string\n", progname, pcWhichFile, iErrLine);
		exit(EX_DATAERR);
	}
	*pcConvert = '\000';
}

#define HT_EOF		-1	/* no more tokens			*/
#define HT_STRING	'\"'	/* either "c-string" or `m4-string'	*/
#define HT_EQ		'='	/* literal (token in character)		*/
#define HT_NL		'\n'	/* literal				*/
#define HT_UNDEF	'.'	/* literal				*/
#define HT_HEADER	'%'	/* literal				*/
#define HT_WORD		'W'	/* word not a valid m4 macro name	*/
#define HT_MACRO	'M'	/* word | macro				*/

/* Tokenize the configuration file					(ksb)
 */
int
Token(FILE *fpCursor, char *pcText)
{
	register int iRet, i;

	*pcText = '\000';
	while (EOF != (iRet = getc(fpCursor))) {
		if ('\n' == iRet || !isspace(iRet)) {
			break;
		}
	}
	switch (iRet) {
	case EOF:
		return HT_EOF;
	case '=': case '\n': case '%':
		break;
	case '\"':
		CString(fpCursor, pcText);
		break;
	case '#':
		while (EOF != (iRet = getc(fpCursor))) {
			if ('\n' == iRet)
				break;
		}
		break;
	case '`':
		iRet = '\"';
		M4String(fpCursor, pcText);
		break;
	case '\'':
		fprintf(stderr, "%s: %s:%u: stray \', unbalanced m4 quotes?\n", progname, pcWhichFile, iLine);
		return Token(fpCursor, pcText);
	case '.':  /* the undefined value, or a string like ".eq." */
		i = getc(fpCursor);
		if (EOF != i) {
			ungetc(i, fpCursor);
		}
		if (EOF == i || isspace(i) || '#' == i) {
			break;
		}
		/*fallthrough*/
	default:
		*pcText++ = iRet;
		iRet = (isalpha(iRet) || '_' == iRet) ? HT_MACRO : HT_WORD;
		for (/*nada*/; EOF != (i = getc(fpCursor)); ++pcText) {
			*pcText = i;
			if ('=' == i || isspace(i))
				break;
			if (HT_MACRO == iRet && !isalpha(i) && !isdigit(i) && '_' != i)
				iRet = HT_WORD;
		}
		if (EOF != i) {
			ungetc(i, fpCursor);
		}
		*pcText = '\000';
		break;
	}
	return iRet;
}

/* Extract the first absolute path from a $PATH-like string		(ksb)
 */
static void
FirstAbsolute(char *pcTemp, size_t wLen, char *pcFrom)
{
	register int fSaw, fColon, c;

	fSaw = 0, fColon = 1;
	while ('\000' != (c = *pcFrom++)) {
		if (fColon) {
			if (fSaw) {
				*pcTemp = '\000';
				return;
			}
			fColon = 0;
			fSaw = ('/' == c);
		}
		if (':' == c) {
			fColon = 1;
			continue;
		}
		if (!fSaw) {
			continue;
		}
		if (0 == wLen--) {
			return;
		}
		*pcTemp++ = c;
	}
	*pcTemp = '\000';
}

static PPM_BUF PPMString, PPMHeaders, PPMHosts, PPMBooleans, PPMWardPre, PPMWardFix;

/* make sure the PPMs are all setup for the space we'll need to start	(ksb)
 */
static void
HostStart(char *pcSearch)
{
	register char **ppc, *pc, *pcHxPath;
	register META_HOST *pMH;

	pcHostSearch = pcSearch;
	acTilde[0] = '\000';
	if ((char *)0 != (pcHxPath = getenv(acHxmdPath))) {
		FirstAbsolute(acTilde, sizeof(acTilde)-1, pcHxPath);
	}
	if ('\000' == acTilde[0]) {
		FirstAbsolute(acTilde, sizeof(acTilde)-1, pcSearch);
	}
	acTilde[sizeof(acTilde)-1] = '\000';
	util_ppm_init(& PPMAutoDef, sizeof(char *), 32);
	ppc = (char **)util_ppm_size(& PPMAutoDef, 16);
	ppc[0] = (char *)0;
	util_ppm_init(& PPMString, sizeof(char), 1024);
	util_ppm_init(& PPMHeaders, sizeof(char *), 64);
	ppc = (char **)util_ppm_size(& PPMHeaders, 16);
	ppc[0] = (char *)0;
	util_ppm_init(& PPMHosts, sizeof(META_HOST), 512);
	pMH = (META_HOST *)util_ppm_size(& PPMHosts, 1024);
	pMH[0].iline = 0;
	pMH[0].pcfrom = (char *)0;
	pMH[0].pcrecent = (char *)0;
	pMH[0].pchost = (char *)0;
	pMH[0].pMD = (MACRO_DEF *)0;
	util_ppm_init(& PPMBooleans, sizeof(char *), 16);
	ppc = (char **)util_ppm_size(& PPMBooleans, 16);
	ppc[0] = (char *)0;
	util_ppm_init(& PPMWardPre, sizeof(char), 256);
	pc = (char *)util_ppm_size(& PPMWardPre, 256);
	*pc = '\000';
	util_ppm_init(& PPMWardFix, sizeof(char), 256);
	pc = (char *)util_ppm_size(& PPMWardFix, 256);
	*pc = '\000';
}

/* Read host configuration file, distrib format or our format		(ksb)
 * file ::= line * '\000'
 * line ::= ( '#' comment | marco '=' term * | definition | '%' headers ) '\n'
 * comment ::= .*
 * macro ::= [a-zA-Z_][a-zA-Z0-9_]*
 * term ::= '.' | [^space]* | "C-string" | `m4 string'
 * headers ::= macro macro*
 * definition ::= term term*
 *
 * Just start with minimal default header: %HOST
 * When you want distrib's set them in the config file, please.
 */
static void
ReadHcf(FILE *fpCursor, char *pcSpace, int fAllowDef)
{
	register int iTok, iNext, iLast;
	static char acUndef[] = ".";
	auto char *pcKeep;
	register char **ppc;

	ppc = util_ppm_size(& PPMHeaders, 16);
	ppc[0] = strdup(pcHOST);
	ppc[1] = (char *)0;

	iLine = 1;
	/* file
	 */
	iTok = Token(fpCursor, pcSpace);
	while (HT_EOF != iTok) {
		switch (iTok) {
		case HT_EQ:		/* errors			*/
			fprintf(stderr, "%s: %s:%u stray operator `%c'\n", progname, pcWhichFile, iLine, iTok);
			exit(EX_DATAERR);
		case HT_NL:		/* track blank/comment lines	*/
			++iLine;
			iTok = Token(fpCursor, pcSpace);
			continue;
		case HT_HEADER:		/* redefine headers		*/
			/* first unbind the old ones, then unbind the new
			 * ones (they must be instanced to be bound).
			 */
			for (ppc = util_ppm_size(& PPMHeaders, 0); (char *)0 != *ppc; ++ppc) {
				if (acUndef != *ppc)
					BindMacro(*ppc, (char *)0);
			}

			for (iNext = 0; HT_NL != (iTok = Token(fpCursor, pcSpace)); ++iNext) {
				ppc = util_ppm_size(& PPMHeaders, ((iNext+1)|7)+1);
				if (HT_UNDEF == iTok) {
					ppc[iNext] = acUndef;
					continue;
				}
				if (HT_HEADER == iTok) {
					ppc[iNext] = pcHOST;
					continue;
				}
				if (HT_MACRO != iTok) {
					fprintf(stderr, "%s: %s:%u header contruction must include only m4 macro names, or the undef token (%s)\n", progname, pcWhichFile, iLine, acUndef);
					exit(EX_DATAERR);
				}
				ppc[iNext] = strdup(pcSpace);
				BindMacro(ppc[iNext], (char *)0);
			}
			if (HT_NL != iTok) {
				fprintf(stderr, "%s: %s:%u header syntax error\n", progname, pcWhichFile, iLine);
				exit(EX_DATAERR);
			}
			ppc[iNext] = (char *)0;
			continue;
		case HT_UNDEF:
			*pcSpace = '\000';
			break;
		case HT_WORD:
		case HT_MACRO:
		case HT_STRING:
			break;
		}
		iLast = iTok;
		pcKeep = strdup(pcSpace);
		iTok = Token(fpCursor, pcSpace);
		if (HT_EQ == iTok) {
			iTok = Token(fpCursor, pcSpace);
			switch (iTok) {
			case HT_WORD:
			case HT_MACRO:
			case HT_STRING:
				BindMacro(pcKeep, strdup(pcSpace));
				break;
			case HT_UNDEF:
				BindMacro(pcKeep, (char *)0);
				break;
			default:
				fprintf(stderr, "%s: %s:%u %s= syntax error\n", progname, pcWhichFile, iLine, pcKeep);
				exit(EX_DATAERR);
			}
			iTok = Token(fpCursor, pcSpace);
			continue;
		}
		/* we need one value for each macro header
		 */
		ppc = util_ppm_size(& PPMHeaders, 0);

		iNext = 0;
		if (acUndef != ppc[iNext]) {
			BindMacro(ppc[iNext], (HT_UNDEF == iLast) ? (char *)0 : pcKeep);
		}
		for (++iNext; HT_STRING == iTok || HT_WORD == iTok || HT_MACRO == iTok || HT_UNDEF == iTok; ++iNext) {
			if ((char *)0 == ppc[iNext]) {
				fprintf(stderr, "%s: %s:%u: out of columns for value `%s'\n", progname, pcWhichFile, iLine, pcSpace);
				exit(EX_CONFIG);
			} if (acUndef == ppc[iNext]) {
				;
			} if (HT_UNDEF == iTok) {
				BindMacro(ppc[iNext], (char *)0);
			} else {
				BindMacro(ppc[iNext], strdup(pcSpace));
			}
			iTok = Token(fpCursor, pcSpace);
		}
		/* Warn about missing columns under -d M
		 */
		while ((char *)0 != ppc[iNext]) {
			if (acUndef != ppc[iNext]) {
				if ((char *)0 != strchr(pcDebug, 'M')) {
					fprintf(stderr, "%s: %s:%d missing value for `%s' definition\n", progname, pcWhichFile, iLine, ppc[iNext]);
				}
				BindMacro(ppc[iNext], (char *)0);
			}
			++iNext;
		}
		if (HT_NL != iTok) {
			fprintf(stderr, "%s: %s:%u extra field after all headers assigned (%s)\n", progname, pcWhichFile, iLine, pcSpace);
			exit(EX_DATAERR);
		}
		BindHost(fAllowDef);
	}
	for (ppc = util_ppm_size(& PPMHeaders, 0); (char *)0 != *ppc; ++ppc) {
		if (acUndef != *ppc)
			BindMacro(*ppc, (char *)0);
	}
}

static char *apcMake[] = {
	"cache",		/* cache for main			*/
#define pcstdin	apctemp[0]
	"scratch",		/* the -E, -G temp file, fill to filter	*/
#define pcscratch apctemp[1]
	"make",			/* recipe file from cache.host dir	*/
#define pcmake apctemp[2]
	"filter"		/* a valid slot to fill (\000\000)	*/
#define pcfilter apctemp[3]
};
static struct {			/* very private data			*/
	unsigned ucount;	/* how many hosts we saw		*/
	unsigned uselected;	/* how many we picked			*/
	char *apctemp[sizeof(apcMake)/sizeof(char *)];	/* apcMake shadow*/
} HostDb;

/* Define a new host, or update some bindings				(ksb)
 */
static void
BindHost(int fAlways)
{
	register META_HOST *pMHCur;
	register MACRO_DEF *pMDHost;
	register char *pcHost;
	register unsigned i;

	pMDHost = CurrentMacros();
	if ((char *)0 == (pcHost = MacroValue(pcHOST, pMDHost))) {
		fprintf(stderr, "%s: %s:%u no %s value defined\n", progname, pcWhichFile, iLine, pcHOST);
		return;
	}
	if ((META_HOST *)0 == (pMHCur = (META_HOST *)util_ppm_size(& PPMHosts, (HostDb.ucount|1023)+1))) {
		fprintf(stderr, "%s: out of memory\n", progname);
		exit(EX_OSERR);
	}
	for (i = 0; i < HostDb.ucount; ++i) {
		if (0 == strcasecmp(pMHCur[i].pchost, pcHost)) {
			break;
		}
	}
	if (i < HostDb.ucount) {
		/* We could free the old pMD, with very litle space saved
		 */
		pMHCur[i].pMD = MergeMacros(pMHCur[i].pMD, pMDHost);
		if (pcWhichFile != pMHCur[i].pcrecent) {
			pMHCur[i].udefs += fAlways;
		}
		pMHCur[i].pcrecent = pcWhichFile;
		if ((char *)0 != strchr(pcDebug, 'C')) {
			fprintf(stderr, "%s %s:%u redefined from %s:%u (B=%u)\n", pMHCur[i].pchost, pcWhichFile, iLine, pMHCur[i].pcfrom, pMHCur[i].iline, pMHCur[i].udefs);
		}
		return;
	}
	if (!fAlways) {
		return;
	}
	pMHCur[HostDb.ucount].iline = iLine;
	pMHCur[HostDb.ucount].pcfrom = pcWhichFile;
	pMHCur[HostDb.ucount].pcrecent = pcWhichFile;
	pMHCur[HostDb.ucount].udefs = 1;
	pMHCur[HostDb.ucount].pchost = pcHost;
	pMHCur[HostDb.ucount].uu = -1;
	pMHCur[HostDb.ucount].pMD = pMDHost;
	pMHCur[HostDb.ucount].pMHnext = (META_HOST *)0;
	++HostDb.ucount;
}

/* Find a host by name							(ksb)
 */
static META_HOST *
FindHost(char *pcLook, size_t wLen)
{
	register unsigned u;
	register META_HOST *pMHAll;

	if ((char *)0 != strchr(pcDebug, 'L')) {
		fprintf(stderr, "%.*s\n", (int)wLen, pcLook);
	}
	pMHAll = (META_HOST *)util_ppm_size(& PPMHosts, 0);
	for (u = 0; u < HostDb.ucount; ++u) {
		if (0 == strncasecmp(pcLook, pMHAll[u].pchost, wLen) && '\000' == pMHAll[u].pchost[wLen])
			return & pMHAll[u];
	}
	return (META_HOST *)0;
}

/* Add a new config file to the mix					(ksb)
 * If the `file' is a socket we connect, expect a +length\n
 * as the first line, then read the configuration from the socket.
 * This is undocumented in version 1, but will be spiffy in version 2.
 */
static void
ReadConfig(char *pcConfig, int fAllowDef)
{
	register char *pcFound, *pcSearch;
	register FILE *fpConfig;
	auto struct stat stLen;
	auto char *apcLook[2];
	auto char acBanner[1024];

	apcLook[0] = pcConfig;
	apcLook[1] = (char *)0;
	if ((char *)0 != (pcSearch = getenv(acHxmdPath)) && '\000' == *pcSearch) {
		pcSearch = (char *)0;
	}
	if ((char *)0 == (pcFound = util_fsearch(apcLook, pcSearch))) {
		fprintf(stderr, "%s: %s: cannot find file\n", progname, pcConfig);
		exit(EX_NOINPUT);
	} else if (0 != stat(pcFound, &stLen)) {
		fprintf(stderr, "%s: stat: %s: %s\n", progname, pcFound, strerror(errno));
		exit(EX_NOINPUT);
	} else if (S_ISFIFO(stLen.st_mode)) {
		if ((FILE *)0 == (fpConfig = fopen(pcFound, "r"))) {
			fprintf(stderr, "%s: fopen: %s: %s\n", progname, pcFound, strerror(errno));
			exit(EX_OSERR);
		}
		if ((char *)0 == fgets(acBanner, sizeof(acBanner), fpConfig)) {
			fprintf(stderr, "%s: %s: no data\n", progname, pcFound);
			fclose(fpConfig);
			return;
		}
		if ('+' != acBanner[0]) {
			fprintf(stderr, "%s: %s: %s", progname, pcFound, acBanner);
			fclose(fpConfig);
			exit(EX_SOFTWARE);
		}
		stLen.st_size = strtol(acBanner+1, (char **)0, 0);
		pcWhichFile = pcFound;
	} else if (S_ISSOCK(stLen.st_mode)) {
		register int s;
		auto struct sockaddr_un run;

		if (-1 == (s = socket(AF_UNIX, SOCK_STREAM, 0))) {
			fprintf(stderr, "%s: socket: %s\n", progname, strerror(errno));
			exit(EX_UNAVAILABLE);
		}
		run.sun_family = AF_UNIX;
		(void)strcpy(run.sun_path, pcFound);
		if (-1 == connect(s, (struct sockaddr *)&run, EMU_SOCKUN_LEN(pcFound))) {
			fprintf(stderr, "%s: connect: %s: %s\n", progname, pcFound, strerror(errno));
			exit(EX_UNAVAILABLE);
		}
		if ((FILE *)0 == (fpConfig = fdopen(s, "r"))) {
			fprintf(stderr, "%s: fdopen: %d: %s\n", progname, s, strerror(errno));
			exit(EX_OSERR);
		}
		if ((char *)0 == fgets(acBanner, sizeof(acBanner), fpConfig)) {
			fprintf(stderr, "%s: %s: no data\n", progname, pcFound);
			fclose(fpConfig);
			return;
		}
		if ('+' != acBanner[0]) {
			fprintf(stderr, "%s: %s: %s", progname, pcFound, acBanner);
			fclose(fpConfig);
			exit(EX_SOFTWARE);
		}
		stLen.st_size = strtol(acBanner+1, (char **)0, 0);
		pcWhichFile = pcFound;
	} else if ((FILE *)0 == (fpConfig = fopen(pcFound, "r"))) {
		fprintf(stderr, "%s: fopen: %s: %s\n", progname, pcConfig, strerror(errno));
		exit(EX_OSERR);
	} else if (-1 == fstat(fileno(fpConfig), & stLen)) {
		fprintf(stderr, "%s: fstat: %d on %s: %s\n", progname, fileno(fpConfig), pcConfig, strerror(errno));
		exit(EX_OSERR);
	} else {
		pcWhichFile = pcFound;
	}
	/* We know the longest string possible in file is st_size-4
	 * (viz. X=`string'), I'm lazy so reuse one longer than that.
	 */
	if (0 != stLen.st_size) {
		ReadHcf(fpConfig, util_ppm_size(& PPMString, (stLen.st_size|3)+1), fAllowDef);
	}
	if (stdin != fpConfig) {
		fclose(fpConfig);
	}
}

/* Our compares come from 'E', turn them into m4 expressions		(ksb)
 * -E [!]macro=value
 * -E [!]macro!value
 *	"ifelse(macro,`value',`"  "')"
 *	"ifelse(macro,`value',`',`"  "')"
 * -E [!]macro { == | != | < | > | <= | >= } value
 * -E [!][-]int { == | != | < | > | <= | >= } value
 *	"ifelse(eval(macro {op} value), 1,`"  "')"
 * macro = word | word( params )
 * We add at most strlen("ifelse(eval(" . ",1,`" . "',`")  + stlen(pcGuard)
 * to the prefix and less to the Suffix, which is always "')"
 * N.B. use quotes to get PUNCT=`='
 */
static void
AddExclude(int iOpt, char *pcOrig)
{
	register unsigned uFix, uAdd;
	register char *pcPrefix, *pcSuffix, *pcMacro, *pcEos, *pcCompare;
	register int fComplement, fMath, fNegate;
	auto char acOp[4];

	pcCompare = pcOrig;
	for (fComplement = 0; '!' == *pcCompare || isspace(*pcCompare); ++pcCompare) {
		if ('!' == *pcCompare)
			fComplement ^= 1;
	}
	/* we can't write on the arguments as we might exec with them in
	 * mmsrc, which share this code --ksb
	 */
	pcCompare = strdup(pcCompare);

	uAdd = strlen(pcCompare)+32;
	pcPrefix = (char *)util_ppm_size(& PPMWardPre, 0);
	pcSuffix = (char *)util_ppm_size(& PPMWardFix, 0);
	pcPrefix = (char *)util_ppm_size(& PPMWardPre, ((strlen(pcPrefix)+uAdd)|255)+1);
	uFix = strlen(pcSuffix);
	pcSuffix = (char *)util_ppm_size(& PPMWardFix, ((uFix+uAdd)|255)+1);
	fNegate = 0;
	while ('-' == *pcCompare) {
		fNegate ^= 1;
		++pcCompare;
	}
	/* Eat host or macro name and params after that name. Allows things like
	 *	-E "COLOR(HOST)=TINT(RACK)"
	 *	-E "www.example.com=HOST"    # better -E HOST=www.example.com
	 * We DO NOT look for m4 quotes here around parans, there is a limit to
	 * my madness (as unbalanced parens are never required), sorry.
	 * Maybe use -D!OPEN="(" then -E "defn(\`OPEN')=PUNCT"
	 * We also allow ':' for an IPv6 address.
	 */
	for (pcMacro = pcCompare; isalnum(*pcCompare) || '_' == *pcCompare || '.' == *pcCompare || ':' == *pcCompare || '-' == *pcCompare; ++pcCompare) {
		/*nada*/
	}
	if ('(' == *pcCompare) {
		register int iCnt = 1;
		++pcCompare;
		while (0 != iCnt) {
			switch (*pcCompare++) {
			case ')':
				--iCnt;
				continue;
			case '(':
				++iCnt;
				continue;
			case '\000':
				fprintf(stderr, "%s: -E: unbalanced parenthesis in macro parameter list: %s\n", progname, pcOrig);
				exit(EX_USAGE);
			default:
				continue;
			}
		}
	}
	if (fNegate) {
		*--pcMacro = '-';
	}
	do {
		acOp[0] = *pcCompare;
		*pcCompare++ = '\000';
	} while (isspace(acOp[0]));
	acOp[1] = '\000';
	acOp[2] = '\000';
	fMath = 0;
	switch (acOp[0]) {
	case '=':	/* == or =	*/
		if ('=' == *pcCompare) {
			acOp[1] = '=';
			fMath = 1;
			++pcCompare;
		}
		break;
	case '!':	/* != or ! -- slang for !macro=value */
		if ('=' == *pcCompare) {
			acOp[1] = '=';
			fMath = 1;
			++pcCompare;
		} else {
			fComplement ^= 1;
		}
		break;
	case '<':	/* < or <=	*/
	case '>':	/* > or >=	*/
		fMath = 1;
		if ('=' == *pcCompare) {
			acOp[1] = '=';
			++pcCompare;
		}
		break;
	default:
		fprintf(stderr, "%s: -E: %s: unknown compare operator\n", progname, acOp);
		exit(EX_USAGE);
	}

	/* Append to the prefix the new condition, prepend to the suffix "')"
	 */
	pcEos = pcPrefix + strlen(pcPrefix);
	if (fMath) {
		snprintf(pcEos, uAdd, "ifelse(eval(%s%s(%s)),%s,`", /*)*/ pcMacro, acOp, pcCompare, fComplement ? "0" : "1");
	} else {
		snprintf(pcEos, uAdd, "ifelse(%s,%s%s,`", /*)*/ pcMacro, pcCompare, fComplement ? ",`'" : "");
	}
	memmove(pcSuffix+2, pcSuffix, uFix+1);
	pcSuffix[0] = '\'';
	pcSuffix[1] = /*(*/ ')';
}

/* We'll take [!]*word[,[!]*word]*					(ksb)
 * We won't do any '|'s, use a -G (or -o) for that, dude.  Build an
 * intersection list (-B2) or even run two hxmd's.  We can't write on Name.
 */
static void
AddBoolean(int iOpt, char *pcName)
{
	register char *pcFilter, *pcScan, **ppc;
	register int fComplement, i;
	register unsigned uCount;

	if ((char *)0 == (pcName = strdup(pcName))) {
		fprintf(stderr, "%s: strdup: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	ppc = (char **)util_ppm_size(& PPMBooleans, 1);
	for (uCount = 0; (char *)0 != ppc[uCount]; ++uCount) {
		/*nada*/
	}
	for (pcScan = pcFilter = pcName; '\000' != (i = *pcScan); ++pcScan) {
		if (isspace(i))
			continue;
		if (isalnum(i) || '_' == i || '!' == i) {
			/*nada*/
		} else if (',' == i || '&' == i) {
			i = ',';
		} else {
			fprintf(stderr, "%s: %s: illegal character \"%c\" in macro name\n", progname, pcName, i);
			exit(EX_USAGE);
		}
		*pcFilter++ = i;
	}
	*pcFilter = '\000';

	pcScan = pcName;
	do {
		for (fComplement = 0; '!' == *pcScan; ++pcScan) {
			fComplement ^= 1;
		}
		pcName = pcScan;
		while (isalnum(*pcScan) || '_' == *pcScan) {
			++pcScan;
		}
		i = *pcScan;
		*pcScan = '\000';

		/* All digits is a define number
		 */
		for (pcFilter = pcName; isdigit(*pcFilter); ++pcFilter) {
			/*nada*/;
		}
		if (isdigit(*pcName) && '\000' != *pcFilter) {
			fprintf(stderr, "%s: %s: m4 macro names cannot start with a digit\n", progname, pcName);
			exit(EX_USAGE);
		}
		if (fComplement) {
			*--pcName = '!';
		}
		/* add to the list
		 */
		ppc = (char **)util_ppm_size(& PPMBooleans, uCount+1);
		ppc[uCount++] = strdup(pcName);
		*pcScan = i;
		if (',' == i) {
			++pcScan;
		}
	} while ('\000' != *pcScan);
	ppc[uCount] = (char *)0;
}

/* Break a colon separated list into the strings, trash the orginal.	(ksb)
 * We consume /:+/ as the separator, empty file names are not meaningful.
 * If you give us a directory we look for the default name in it, and
 * the name "--" (or "--/") is replaced with the first full path in the
 * search list (kept in acTilde by HostStart).
 */
static char **
BreakList(PPM_BUF *pPPM, char *pcData, char *pcWho, char *pcDup)
{
	register unsigned int u;
	register char *pcThis, **ppcList;
	auto char acTemp[MAXPATHLEN+4];

	if ((char *)0 == pcData || '\000' == pcData[0]) {
		return (char **)0;
	}
	ppcList = util_ppm_size(pPPM, 32);
	for (u = 0; '\000' != *pcData; ++u) {
		ppcList = util_ppm_size(pPPM, ((u+1)|15)+1);
		ppcList[u] = pcThis = pcData;
		while ('\000' != *++pcData) {
			if (':' == *pcData)
				break;
		}
		while (':' == *pcData) {
			*pcData++ = '\000';
		}
		if ('-' != pcThis[0]) {
			continue;
		}
		if ('\000' == pcThis[1]) {
			if ((char *)0 != pcDup) {
				ppcList[u] = pcDup;
			} else {
				ReserveStdin(& ppcList[u], pcWho);
			}
			continue;
		}
		if ('-' != pcThis[1] || !('/' == pcThis[2] || '\000' == pcThis[2])) {
			continue;
		}
		/* "--/file" or "--" maps to a file under the first absolute
		 * path in the search list -- ksb
		 */
		(void)strncpy(acTemp, acTilde, sizeof(acTemp));
		if ('/' == pcThis[2]) {
			(void)strncat(acTemp, pcThis+2, sizeof(acTemp));
		}
		ppcList[u] = strdup(acTemp);
	}
	ppcList[u] = (char *)0;
	return ppcList;
}

/* Reproduce the list BreakList saw, with "-" translated to a file.	(ksb)
 * We have to use full-path, which is not obvious from the context here. --ksb
 * You can "break" this with "./" as a prefix -- I wouldn't do that.
 * Technically we can still generate a pathname longer than MAXPATHLEN
 * I doubt that in the real world we'll use a lot of ../../../
 * stuff in the filenames.  --ksb
 */
static int
MakeList(PPM_BUF *pPPM, char **ppcOut)
{
	register char **ppcList, *pcMem, *pcRet, *pcComp;
	register unsigned uLen, u, fDotSlash, fFail;
	register char *pcSearch;
	auto struct stat stDir, stFound;
	auto char *apcLook[2], acCWD[MAXPATHLEN+4];
	static char *pcCacheCWD = (char *)0;

	*ppcOut = (char *)0;
	if ((char *)0 != (pcSearch = getenv(acHxmdPath)) && '\000' == *pcSearch) {
		pcSearch = (char *)0;
	}
	/* We cache getcwd return to avoid slow disk searches for "../../"...
	 */
	if ((char *)0 != pcCacheCWD) {
		/* We have it from last call. */
	} else if ((char *)0 == getcwd(acCWD, sizeof(acCWD))) {
		fprintf(stderr, "%s: getcwd: %s\n", progname, strerror(errno));
		return 2;
	} else if ((char *)0 == (pcCacheCWD = strdup(acCWD))) {
		fprintf(stderr, "%s: strdup: %s\n", progname, strerror(ENOMEM));
		return 3;
	}
	if ((char **)0 == (ppcList = util_ppm_size(pPPM, 0)) || (char *)0 == *ppcList) {
		return 0;
	}
	fFail = 0;
	uLen = 0;
	for (u = 0; (char *)0 != ppcList[u]; ++u) {
		uLen += MAXPATHLEN+1;
	}
	pcRet = pcMem = malloc((uLen|7)+1);
	/* We have a buffer that is long enough for any legal list,
	 * fill it with the paths to the files given, but convert them
	 * to absolute paths.
	 */
	for (u = 0; (char *)0 != ppcList[u]; ++u, pcMem += strlen(pcMem)) {
		register char *pcRepl, *pcChop;
		register unsigned uRepl;

		if (0 != u) {
			*pcMem++ = ':';
		}
		pcComp = ppcList[u];
		apcLook[0] = pcComp;
		apcLook[1] = (char *)0;
		fDotSlash = 0;
		while ('.' == pcComp[0] && '/' == pcComp[1]) {
			do
				++pcComp;
			while ('/' == pcComp[0]);
			fDotSlash = 1;
		}
		if (fDotSlash) {
			if (-1 == stat(pcComp, &stDir)) {
				fprintf(stderr, "%s: stat: %s: %s\n", progname, pcComp, strerror(errno));
				fFail = 2;
				continue;
			}
		} else if ((char *)0 == (pcComp = util_fsearch(apcLook, pcSearch))) {
			fprintf(stderr, "%s: %s: is not in our %s\n", progname, apcLook[0], acHxmdPath);
			fFail = 1;
			continue;
		} else if (-1 == stat(pcComp, &stDir)) {
			fprintf(stderr, "%s: stat: %s: %s\n", progname, pcComp, strerror(errno));
			fFail = 2;
			continue;
		} else if (S_ISREG(stDir.st_mode) || S_ISFIFO(stDir.st_mode) || S_ISSOCK(stDir.st_mode)) {
			/* OK */
		} else if (S_ISDIR(stDir.st_mode)) {
			/* build:    comp."/".argv[0].".cf\000" */
			uRepl = strlen(pcComp)+1+strlen(progname)+3+1;
			pcRepl = malloc((uRepl|7)+1);
			snprintf(pcRepl, uRepl, "%s/%s.cf", pcComp, progname);
			ppcList[u] = pcRepl;
			if (-1 == stat(pcRepl, &stDir)) {
				fprintf(stderr, "%s: stat: %s: %s\n", progname, pcRepl, strerror(errno));
				fFail = 3;
			}
			pcComp = pcRepl;
		} else if (S_ISCHR(stDir.st_mode)) {
			/* we hope it's /dev/null, not /dev/tty */
		} else {
			fprintf(stderr, "%s: %s: not a plain file\n", progname, pcComp);
			fFail = 4;
			continue;
		}
		if ('/' == pcComp[0]) {
			(void)strcpy(pcMem, pcComp);
			pcMem += strlen(pcMem);
			continue;
		}
		(void)strcpy(pcMem, pcCacheCWD);

		/* The path search could pick "./file" or "../file"
		 */
		pcRepl = pcComp;
		while ('.' == pcComp[0] && ('/' == pcComp[1] || ('.' == pcComp[1] && '/' == pcComp[2]))) {
			if ('.' == *++pcComp) {
				++pcComp;
				if ((char *)0 != (pcChop = strrchr(pcMem+1, '/')))
					*pcChop = '\000';
			}
			do {
				++pcComp;
			} while ('/' == pcComp[0]);
		}
		(void)strcat(pcMem, "/");
		(void)strcat(pcMem, pcComp);
		/* If our remapping is not the same file, backout to the
		 * relative path. Must be symbolic link in the way, but
		 * getcwd(3) should not return a symbolic link, we raced?
		 */
		if (-1 == stat(pcMem, &stFound) || stDir.st_ino != stFound.st_ino || stDir.st_dev != stFound.st_dev) {
			if ((char *)0 != strchr(pcDebug, 'M')) {
				fprintf(stderr, "%s: path compression failed (%s != %s), patched\n", progname, pcMem, pcRepl);
			}
			(void)strcpy(pcMem, pcRepl);
		}
	}
	if (0 != fFail) {
		return 1;
	}
	*pcMem = '\000';
	*ppcOut = pcRet;
	return 0;
}

/* See if a file is included in the given list				(ksb)
 * first check by name, then by inode/device.
 */
static int
CheckDup(char *pcTarget, char **ppcIn)
{
	register char *pc;
	register int i;
	auto struct stat stTry, stTarget;

	if ((char **)0 == ppcIn) {
		return 0;
	}
	for (i = 0; (char *)0 != (pc = ppcIn[i]); ++i) {
		if (pc == pcTarget || 0 == strcmp(pc, pcTarget))
			return 1;
	}
	/* We can't tell by device it if doesn't exist or no perm
	 */
	if (-1 == stat(pcTarget, & stTarget)) {
		return 0;
	}
	for (i = 0; (char *)0 != (pc = ppcIn[i]); ++i) {
		if (-1 == stat(pc, & stTry))
			continue;
		if (stTry.st_ino != stTarget.st_ino || stTry.st_dev != stTarget.st_dev)
			continue;
		return 1;
	}
	return 0;
}

/* Return a slot sequence with our temp files listed for cleanup.	(ksb)
 * We need to make them in our tempdir so the Hxmd parent process can
 * rm them after the Child is done with them.
 */
static char *
HostSlot(char *pcCache)
{
	register int fd;
	register char *pcRet, *pcAdv;
	register int iBuild, i;
	auto PPM_BUF PPMConfigs, PPMZConfigs, PPMXConfigs;
	register char **ppcList, **ppcZList, **ppcXList;
	register char **ppcStdin;
	register MACRO_DEF *pMDDefault;

	iBuild = sizeof(apcMake)/sizeof(char *);
	if ((char *)0 == (pcRet = calloc(iBuild*(MAXPATHLEN+2), sizeof(char)))) {
		fprintf(stderr, "%s: calloc(%u,%u) %s\n", progname, (unsigned)2*(MAXPATHLEN+2), (unsigned)sizeof(char), strerror(errno));
		return (char *)0;
	}
	pcAdv = pcRet;
	for (i = 0; i < iBuild; ++i) {
		snprintf(pcAdv, MAXPATHLEN, "%s/%sXXXXXX", pcCache, apcMake[i]);
		if (-1 == (fd = mkstemp(pcAdv))) {
			fprintf(stderr, "%s: mkstemp: %s: %s\n", progname, pcAdv, strerror(errno));
			return (char *)0;
		}
		close(fd);
		HostDb.apctemp[i] = pcAdv;
		pcAdv += strlen(pcAdv)+1;
	}
	*pcAdv = '\000';
	util_ppm_init(& PPMConfigs, sizeof(char *), 0);
	ppcList = BreakList(& PPMConfigs, pcConfigs, "configuration file", (char *)0);
	util_ppm_init(& PPMZConfigs, sizeof(char *), 0);
	ppcZList = BreakList(& PPMZConfigs, pcZeroConfig, "zero configuration", (char *)0);
	for (i = 0, pcAdv = (char *)0; (char **)0 != ppcZList && (char *)0 != ppcZList[i]; ++i) {
		if (HostDb.pcstdin != ppcZList[i])
			continue;
		pcAdv = ppcZList[i];
		break;
	}
	util_ppm_init(& PPMXConfigs, sizeof(char *), 0);
	ppcXList = BreakList(& PPMXConfigs, pcExConfig, "extra configuration", pcAdv);

	/* you _cannot_ need stdin before you get here */
	if ((char **)0 != (ppcStdin = ReserveStdin((char **)0, "fetch"))) {
		if (-1 == (fd = open(HostDb.pcstdin, O_RDWR, 0600))) {
			fprintf(stderr, "%s: open: %s: %s\n", progname, HostDb.pcstdin, strerror(errno));
			return (char *)0;
		}
		FdCopy(0, fd, "save stdin");
		close(fd);
		*ppcStdin = HostDb.pcstdin;
	}
	if (MakeList(& PPMConfigs, &pcConfigs) ||
		MakeList(& PPMZConfigs, &pcZeroConfig) ||
		MakeList(& PPMXConfigs, &pcExConfig)) {
		return (char *)0;
	}

	ResetMacros();
	pMDDefault = CurrentMacros();
	if ((char **)0 != ppcZList) {
		while ((char *)0 != (pcAdv = *ppcZList++)) {
			ReadConfig(pcAdv, !CheckDup(pcAdv, ppcXList));
			pMDDefault = MergeMacros(CurrentMacros(), pMDDefault);
			ResetMacros();
		}
	}
	if ((char **)0 != ppcList) {
		while ((char *)0 != (pcAdv = *ppcList++)) {
			ReadConfig(pcAdv, 1);
			ResetMacros();
		}
	}
	if ((char **)0 != ppcXList) {
		while ((char *)0 != (pcAdv = *ppcXList++)) {
			ReadConfig(pcAdv, 0);
			ResetMacros();
		}
	}
	if ((MACRO_DEF *)0 != pMDDefault) {
		register unsigned u;
		register META_HOST *pMH;
		pMH = (META_HOST *)util_ppm_size(& PPMHosts, 1);

		for (u = 0; (META_HOST *)0 != pMH && u < HostDb.ucount; ++u) {
			pMH[u].pMD = MergeMacros(pMH[u].pMD, pMDDefault);
		}
	}
	return pcRet;
}

/* Output the m4 macros to allow a recursive call to hxmd, -Z and -C/-X. (ksb)
 * We figure one can get HOST when you need it, or as for other spells.
 */
void
RecurBanner(FILE *fp)
{
	if ((char *)0 != pcZeroConfig) {
		fprintf(fp, "define(`%sOPT_Z',`%s')", pcXM4Define, pcZeroConfig);
	}
	if ((char *)0 != pcConfigs) {
		fprintf(fp, "define(`%sOPT_C',`%s')", pcXM4Define, pcConfigs);
	}
	if ((char *)0 != pcExConfig) {
		fprintf(fp, "define(`%sOPT_X',`%s')", pcXM4Define, pcExConfig);
	}
	fprintf(fp, "define(`%sU_COUNT',`%u')dnl\n", pcXM4Define, HostDb.ucount);
	if (0 != HostDb.uselected) {
		if ((char *)0 != pcOutMerge) {
			fprintf(fp, "define(`%sU_MERGED',`%s')", pcXM4Define, HostDb.pcfilter);
		}
		fprintf(fp, "define(`%sU_SELECTED',`%u')dnl\n", pcXM4Define, HostDb.uselected);
	}
}

/* Output a macro define from the configuration file			(ksb)
 */
static int
M4Define(char *pcName, char *pcValue, void *fpOut)
{
	if ((char *)0 != pcValue) {
		fprintf((FILE *)fpOut, "define(`%s',`%s')", pcName, pcValue);
	}
	return 0;
}

/* Output all the defines for a host to the m4 process, then the	(ksb)
 * caller adds an "include(argv[x])dnl", some other text.
 */
static void
HostDefs(FILE *fp, META_HOST *pMH)
{
	RecurBanner(fp);
	if ((META_HOST *)0 == pMH) {
		fprintf(fp, "dnl null meta host pointer\n");
		return;
	}
	if (-1 != pMH->uu) {
		fprintf(fp, "define(`%sU',%u)", pcXM4Define, (unsigned)pMH->uu);
	}
	fprintf(fp, "define(`%sB',%u)", pcXM4Define, pMH->udefs);
	ScanMacros(pMH->pMD, M4Define, (void *)fp);
	fprintf(fp, "dnl\n");
}

static int
M4Pushdef(char *pcName, char *pcValue, void *fpOut)
{
	if ((char *)0 != pcValue)
		fprintf((FILE *)fpOut, "pushdef(`%s',`%s')", pcName, pcValue);
	return 0;
}

/* Hackish, I just wanted to list the macros to popdef			(ksb)
 * but lame a hades Linux m4 only pops a single define.  Sigh.
 */
static int
M4ListMacro(char *pcName, char *pcValue, void *fpOut)
{
	if ((char *)0 != pcValue) {
#if M4_POPDEF_MANY
		fprintf((FILE *)fpOut, ",");
#else
		fprintf((FILE *)fpOut, ")popdef(");
#endif
		fprintf((FILE *)fpOut, "`%s'", pcName);
	}
	return 0;
}


static META_HOST *pMHOrder;

/* Pick up the next host. The next one that has not been deselected	(ksb)
 * by -B, -E, or -G below.
 */
static META_HOST *
NextHost()
{
	register META_HOST *pMHRet;

	if ((META_HOST *)0 != (pMHRet = pMHOrder)) {
		pMHOrder = pMHOrder->pMHnext;
	}
	return pMHRet;
}

/* We are accessing a cache and need a target to request		(ksb)
 */
static char *
HostTarget(META_HOST *pMHFor)
{
	return MacroValue(pcHOST, pMHFor->pMD);
}

/* Drop a common define-set, action, undefine-set out for the host	(ksb)
 */
void
BuildM4(FILE *fp, char **ppcBody, META_HOST *pMHFor, char *pcPre, char *pcPost)
{
	fprintf(fp, "pushdef(`%sB',%u)", pcXM4Define, pMHFor->udefs);
	ScanMacros(pMHFor->pMD, M4Pushdef, (void *)fp);
	if ((char *)0 != pcPre)
		fprintf(fp, "dnl\n%s", pcPre);
	else
		fprintf(fp, "dnl\n");
	for (/*param*/; (char *)0 != *ppcBody; ++ppcBody)
		fprintf(fp, "%s\n", *ppcBody);
	/* The blank line on the end of this is ignored when we import -- ksb
	 */
	if ((char *)0 != pcPost)
		fprintf(fp, "%s\n", pcPost);
	fprintf(fp, "popdef(`%sB'", pcXM4Define);
	ScanMacros(pMHFor->pMD, M4ListMacro, (void *)fp);
	fprintf(fp, ")dnl\n");
}

/* Output a macro value under our configuration file quote rules,	(ksb)
 * when pcValue has a default value we'll use that for undefined.
 * Don't pass pcMacro as a null pointer, please.
 */
static void
DropMacro(FILE *fpDrop, char *pcMacro, MACRO_DEF *pMD, char *pcValue)
{
	register char *pcCursor;
	register int c, iM4Qs, iCQs, iIFS, iNonPrint, fM4Ever;

	if ((MACRO_DEF *)0 != pMD && (char *)0 != (pcCursor = MacroValue(pcMacro, pMD))) {
		pcValue = pcCursor;
	} else if ((char *)0 == pcValue) {
		fprintf(fpDrop, ".");
		return;
	}
	fM4Ever = 0;
	iM4Qs = iIFS = iCQs = iNonPrint = 0;
	for (pcCursor = pcValue; '\000' != (c = *pcCursor++); /* nada */) {
		if (isspace(c) || '=' == c || '#' == c)
			++iIFS;
		else if (!isprint(c))
			++iNonPrint;
		else if ('\'' == c)
			fM4Ever = 1, --iM4Qs;
		else if ('`' == c)
			fM4Ever = 1, ++iM4Qs;
		else if ('\"' == c || '\\' == c)
			++iCQs;
	}
	if ('\000' == pcValue[0] || ('.' == pcValue[0] && '\000' == pcValue[1])) {
		fprintf(fpDrop, "`%s\'", pcValue);
	} else if (pcValue[0] != '.' && !fM4Ever && 0 == iCQs && 0 == iIFS && 0 == iNonPrint) {
		fprintf(fpDrop, "%s", pcValue);
	} else if (0 == iM4Qs && 0 == iNonPrint) {
		fprintf(fpDrop, "`%s\'", pcValue);
	} else if (0 == iCQs && 0 == iNonPrint) {
		fprintf(fpDrop, "\"%s\"", pcValue);
	} else {
		fputc('\"', fpDrop);
		for (pcCursor = pcValue; '\000' != (c = *pcCursor++); /* nada */) {
			switch (c) {
			case '\n':
				fputc('\\', fpDrop); c = 'n'; break;
			case '\t':
				fputc('\\', fpDrop); c = 't'; break;
			case '\b':
				fputc('\\', fpDrop); c = 'b'; break;
			case '\r':
				fputc('\\', fpDrop); c = 'r'; break;
			case '\f':
				fputc('\\', fpDrop); c = 'f'; break;
			case '\v':
				fputc('\\', fpDrop); c = 'v'; break;
			case '\"':
			case '\'':
			case '\\':
				fputc('\\', fpDrop); break;
			default:
				if (isprint(c))
					break;
				fprintf(fpDrop, "\\%03o", (unsigned)c);
				continue;
			}
			putc(c, fpDrop);
		}
		fputc('\"', fpDrop);
	}
}

/* A merged configuration file is used to recurse or chain to	(ksb)
 * another instance of hxmd (or msrc).  The -o option builds one
 * and the _U_MERGED macro presents the filename.
 */
static void
BuildMerge(const char *pcOutput, char *pcCurMerge, META_HOST *pMHList)
{
	register FILE *fpDrop;
	register char *pcScan, *pcCur, *pcSep1, **ppc, *pcKeyMacro;
	register int c, fSkip;

	pcKeyMacro = pcHOST;
	if ((FILE *)0 == (fpDrop = fopen(pcOutput, "w"))) {
		fprintf(stderr, "%s: fopen: %s: %s\n", progname, pcOutput, strerror(errno));
		exit(EX_OSERR);
	}
	/* output the -D macros
	 */
	for (ppc = util_ppm_size(&PPMAutoDef, 0); (char *)0 != (pcScan = *ppc); ++ppc) {
		fSkip = !(isalnum(*pcScan) || '_' == *pcScan);
		if (fSkip) {
			fputc('#', fpDrop);
			++pcScan;
		}
		while (isalnum(*pcScan) || '_' == *pcScan) {
			fputc(*pcScan++, fpDrop);
		}
		if (fSkip) {
			fputc('\n', fpDrop);
			continue;
		}
		while (isspace(*pcScan)) {
			++pcScan;
		}
		if ('\000' == *pcScan) {
			pcScan = "=";
		}
		fputc(*pcScan++, fpDrop);
		DropMacro(fpDrop, "-D", (MACRO_DEF *)0, pcScan);
		fputc('\n', fpDrop);
	}

	/* If they asked for the key macro in the header don't replicate it for
	 * them.  They are picking the order of the headers, I guess.  (ksb)
	 */
	for (pcScan = strstr(pcCurMerge, pcKeyMacro); (char *)0 != pcScan; pcScan = strstr(pcScan+1, pcKeyMacro)) {
		if (pcScan != pcCurMerge && !isspace(pcScan[-1]))
			continue;
		if ('\000' != (c = pcScan[strlen(pcKeyMacro)]) && !isspace(c))
			continue;
		pcKeyMacro = (char *)0;
		break;
	}

	/* Render the head for -o "HOST:COLOR" as HOST_COLOR.  This lets
	 * us rename a macro "@LOC" -> "_LOC", later (assuming that `@'
	 * can be removed from the _LOC string) a compare of LOC and _LOC
	 * with -C + -X and/or -E is easy.  In the header map non-macro
	 * chars to `_' and prefix numeric macro names with `d'.	(ksb)
	 */
	fprintf(fpDrop, "%%");
	pcSep1 = " ";
	if ((char *)0 != pcKeyMacro) {
		fprintf(fpDrop, "%s", pcKeyMacro);
	} else {
		++pcSep1;
	}
	if ('\000' != pcCurMerge[0]) {
		register int fMac;

		if (isspace(*pcCurMerge) && '\000' != *pcSep1)
			++pcSep1;
		fprintf(fpDrop, "%s", pcSep1);
		fMac = 0;
		for (pcCur = pcCurMerge; '\000' != (c = *pcCur); ++pcCur) {
			if (isalpha(c) || '_' == c) {
				fMac = 1;
				putc(c, fpDrop);
				continue;
			}
			if (isdigit(c)) {
				if (0 == fMac)
					putc('d', fpDrop);
				putc(c, fpDrop);
				fMac = 1;
				continue;
			}
			if (isspace(c)) {
				putc(c, fpDrop);
				fMac = 0;
				continue;
			}
			if (2 != fMac)
				putc('_', fpDrop);
			fMac = 2;
		}
	} else if ('\000' != *pcSep1) {
		++pcSep1;
	}
	fputc('\n', fpDrop);
	for (/*param*/; (META_HOST *)0 != pMHList; pMHList = pMHList->pMHnext) {
		if ((char *)0 != pcKeyMacro) {
			DropMacro(fpDrop, pcKeyMacro, pMHList->pMD, (char *)0);
		}
		fprintf(fpDrop, "%s", pcSep1);
		for (pcCur = pcScan = pcCurMerge; '\000' != *pcScan; ++pcScan) {
			c = *pcScan;
			if (isalnum(c) || '_' == c) {
				continue;
			}
			if (pcCur != pcScan) {
				*pcScan = '\000';
				DropMacro(fpDrop, pcCur, pMHList->pMD, (char *)0);
				*pcScan = c;
			}
			pcCur = pcScan+1;
			putc(c, fpDrop);
		}
		if (pcScan != pcCur) {
			DropMacro(fpDrop, pcCur, pMHList->pMD, (char *)0);
		}
		fputc('\n', fpDrop);
	}
	fclose(fpDrop);
}

/* Figure out which hosts to select from the whole list,		(ksb)
 * then return the first one.
 */
static META_HOST *
HostInit(char **ppcM4)
{
	register char **ppcBool, **ppc, *pc;
	register unsigned u;
	register META_HOST *pMHAll, **ppMHTail;
	register FILE *fpTempt;
	auto char *apcSource[2];
	auto char *pcPrefix, *pcSuffix, **ppcGuards;
	auto char *apcDefault[2];
	auto size_t wTotal, wLen;
	auto int wExit;

	if ((FILE *)0 == (fpTempt = fopen(HostDb.pcscratch, "w"))) {
		fprintf(stderr, "%s: fopen: %s: %s\n", progname, HostDb.pcscratch, strerror(errno));
		return (META_HOST *)0;
	}
	RecurBanner(fpTempt);
	for (ppc = util_ppm_size(pPPMSpells, 0); (char *)0 != *ppc; ++ppc) {
		fprintf(fpTempt, "%s\n", *ppc);
	}
	pcPrefix = (char *)util_ppm_size(& PPMWardPre, 0);
	pcSuffix = (char *)util_ppm_size(& PPMWardFix, 0);
	ppcGuards = (char **)util_ppm_size(pPPMGuard, 0);
	if ((char *)0 == ppcGuards[0]) {
		apcDefault[0] = pcHOST;
		apcDefault[1] = (char *)0;
		ppcGuards = apcDefault;
	}

	ppcBool = (char **)util_ppm_size(& PPMBooleans, 0);
	pMHAll = (META_HOST *)util_ppm_size(& PPMHosts, 0);
	for (u = 0; u < HostDb.ucount; ++u) {
		for (ppc = ppcBool; (char *)0 != (pc = *ppc); ++ppc) {
			register int fComplement, fTruth;

			fComplement = ('!' == *pc);
			if (fComplement)
				++pc;
			if (isdigit(*pc))
				fTruth = atoi(pc) == pMHAll[u].udefs;
			else
				fTruth = (char *)0 != MacroValue(pc, pMHAll[u].pMD);
			if (0 == (fTruth ^ fComplement))
				break;
		}
		if ((char *)0 != pc) {
			if ((char *)0 != strchr(pcDebug, 'B')) {
				fprintf(stderr, "%s: %s rejected by -B %s\n", progname, pMHAll[u].pchost, pc);
			}
			pMHAll[u].iline = 0;
			continue;
		}
		BuildM4(fpTempt, ppcGuards, & pMHAll[u], pcPrefix, pcSuffix);
	}
	fclose(fpTempt);
	if ((char *)0 != strchr(pcDebug, 'G')) {
		fprintf(stderr, "%s: host expressions in \"%s\" (pausing 5 second)\n", progname, HostDb.pcscratch);
		sleep(5);
	}
	apcSource[0] = HostDb.pcscratch;
	apcSource[1] = (char *)0;

	/* We check the return code here, if this m4 failed we
	 * don't want to live.
	 */
	SetCurPhase("selection");
	if (0 != (wExit = FillSlot(HostDb.pcfilter, apcSource, ppcM4, (META_HOST *)0, (const int *)0, (const char *)0))) {
		fprintf(stderr, "%s: guard %s: failed with code %d\n", progname, ppcM4[0], WEXITSTATUS(wExit));
		exit(wExit);
	}
	SetCurPhase((char *)0);

	/* This is also where we get the order for the host, use divert
	 * to migrate hosts up or down the list.
	 */
	ppMHTail = & pMHOrder;
	if ((FILE *)0 == (fpTempt = fopen(HostDb.pcfilter, "r"))) {
		fprintf(stderr, "%s: fopen: %s: %s\n", progname, HostDb.pcfilter, strerror(errno));
		exit(EX_OSERR);
	}
	/* Read in a line as a hostname, trim off white-space and look for
	 * it in the config host list.  If not found skip it, also skip
	 * if on the list already or fobidden by -B.
	 */
	while ((char *)0 != (pc = fgetln(fpTempt, & wTotal))) {
		register META_HOST *pMHFound;
		register char *pcCur;

		for (/*go*/; 0 < wTotal; wTotal -= wLen){
			while (0 < wTotal && isspace(*pc)) {
				++pc, --wTotal;
			}
			pcCur = pc;
			for (wLen = 0; wLen < wTotal; ++pc, ++wLen) {
				if (isspace(*pc))
					break;
			}
			if (0 == wLen) {
				break;
			}
			if ((META_HOST *)0 == (pMHFound = FindHost(pcCur, wLen))) {
				if ((char *)0 != strchr(pcDebug, 'G')) {
					fprintf(stderr, "%s: -G raised \"%.*s\", no such key\n", progname, (int)wLen, pcCur);
				}
				continue;
			}
			if ((META_HOST *)0 != pMHFound->pMHnext || ppMHTail == & pMHFound->pMHnext) {
				continue;
			}
			if (0 == pMHFound->iline) {
				if ((char *)0 != strchr(pcDebug, 'B')) {
					fprintf(stderr, "%s: -B squelched %s from guard list\n", progname, pMHFound->pchost);
				}
				continue;
			}
			*ppMHTail = pMHFound;
			ppMHTail = & pMHFound->pMHnext;
			pMHFound->uu = HostDb.uselected++;
		}
	}
	fclose(fpTempt);
	*ppMHTail = (META_HOST *)0;
	if ((char *)0 != pcOutMerge) {
		BuildMerge(HostDb.pcfilter, pcOutMerge, pMHOrder);
	}

	return NextHost();
}
/* from slot.m */
#line 39 "slot.m"
/* Construct a new slot, return the length				(ksb)
 * build the directory and the empty files to over-write.
 * We know the called allocated space for all the files we need to make.
 * Build "temp[1]\000temp[2]\000...\000\000", we don't need a "0"
 * return the length up to (not including) the last literal \000
 * We can write this to xapply -fz (via xclate) to run this program.
 */
static int
InitSlot(char *pcMkTmp, char *pcSlot, char **argv, const char *pcTmpSpace)
{
	register unsigned i, fd;
	register char *pc, *pcTail, *pcCursor;
	auto char acEmpty[MAXPATHLEN+1];

#if DEBUG
	fprintf(stderr, "%s: initSlot %p\n", progname, pcSlot);
#endif
	pcCursor = pcSlot;
	*pcCursor = '\000';
	acEmpty[0] = '\000';
	acEmpty[MAXPATHLEN] = '\000';
	for (i = 0; (char *)0 != (pc = argv[i]); /*nada*/) {
		if ('\000' == acEmpty[0]) {
			snprintf(acEmpty, MAXPATHLEN, "%s/%s", pcTmpSpace, pcMkTmp);
			if ((char *)0 == mkdtemp(acEmpty)) {
				fprintf(stderr, "%s: mkdtemp: %s: %s\n", progname, acEmpty, strerror(errno));
				exit(EX_CANTCREAT);
			}
		}
		if ((char *)0 == (pcTail = strrchr(pc, '/'))) {
			pcTail = pc;
		} else {
			++pcTail;
		}
		snprintf(pcCursor, MAXPATHLEN, "%s/%s", acEmpty, pcTail);
		fd = open(pcCursor, O_WRONLY|O_CREAT|O_EXCL, 0600);
		/* Two params with same tail, build another direcory
		 * for the next one
		 */
		if (-1 == fd && EEXIST == errno) {
			acEmpty[0] = '\000';
			continue;
		}
		close(fd);
		pcCursor += strlen(pcCursor);
		*++pcCursor = '\000';
		++i;
	}
	return pcCursor - pcSlot;
}

/* Remove the junk we built for the temporary files for a slot		(ksb)
 */
static void
CleanSlot(char *pcSlot)
{
	register char *pcSlash;

	if ((char *)0 == pcSlot) {
		return;
	}
#if DEBUG
	fprintf(stderr, "%s: clean slot %s\n", progname, pcSlot);
#endif
	while ('\000' != *pcSlot) {
		if ((char *)0 == (pcSlash = strrchr(pcSlot, '/')))
			break;
		if ((char *)0 != strchr(pcDebug, 'R')) {
			fprintf(stderr, "%s: rm -f %s\n", progname, pcSlot);
		}
		(void)unlink(pcSlot);
		*pcSlash = '\000';
		if (0 == rmdir(pcSlot) && (char *)0 != strchr(pcDebug, 'R')) {
			fprintf(stderr, "%s: rmdir %s\n", progname, pcSlot);
		}
		*pcSlash = '/';
		pcSlot = pcSlash+strlen(pcSlash)+1;
	}
}

/* Let each file know where the other files are (or will be).		(ksb)
 * This lets Distfiles know where any processed *.host is, for example.
 * It does depend on presented order in argv, like any other program.
 */
static void
SlotDefs(FILE *fpOut, char *pcSlot)
{
	register unsigned uCount;

	for (uCount = 0; '\000' != *pcSlot; ++uCount) {
		fprintf(fpOut, "define(`%s%u',`%s')", pcXM4Define, uCount, pcSlot);
		pcSlot += strlen(pcSlot)+1;
	}
	fprintf(fpOut, "define(`%sC',`%u')dnl\n", pcXM4Define, uCount);
	RecurBanner(fpOut);
}


/* Build the -D spec for the m4 line, hold the pointer to the value	(ksb)
 */
static char *
SetupPhase(const char *pcMnemonic, size_t wPad)
{
	register char *pcRet;
	register int i;

	if (64 < wPad) {
		wPad = 64;
	}
	/* round(len("HXMD_") +      len("PHASE") + '='   )  )+ 64c */
	i = ((strlen(pcXM4Define)+strlen(pcMnemonic)+1)|63)+1 + wPad;
	if ((char *)0 == (pcRet = malloc(i))) {
		fprintf(stderr, "%s: malloc: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	snprintf(pcRet, i, "%s%s=", pcXM4Define, pcMnemonic);
	pcM4Phase = pcRet+strlen(pcRet);
	wPhaseLen = wPad;
	return pcRet;
}

/* Tell the -j files where the output is bound for the gadgets,		(ksb)
 * used outside of this file to force a phase name, we set the numbers.
 */
static const char *
SetCurPhase(const char *pcNow)
{
	if ((char *)0 == pcM4Phase)
		return (char *)0;
	if ((char *)0 == pcNow)
		pcNow = "";
	return strncpy(pcM4Phase, pcNow, wPhaseLen);
}

/* start an M4 filter eval $M4Argv > $Out &   *pwM4 = $!		(ksb)
 */
static FILE *
M4Gadget(const char *pcOut, struct stat *pstLike, pid_t *pwM4, char **ppcM4Argv)
{
	register int iChmodErr, iTry;
	register int fdTemp;
	register FILE *fpRet;
	auto int aiM4[2];

	if (-1 == chmod(pcOut, 0600)) {
		/* If someone unlinked the file, can we put it back?
		 */
		iChmodErr = errno;
		if (ENOENT == errno && -1 != (fdTemp = open(pcOut, O_WRONLY|O_CREAT|O_EXCL, 0600))) {
			/* recovered, we hope */
		} else {
			fprintf(stderr, "%s: chmod: %s: %s\n", progname, pcOut, strerror(iChmodErr));
		}
	}
	if (-1 == (fdTemp = open(pcOut, O_WRONLY|O_TRUNC, 0600))) {
		return (FILE *)0;
	}
	if ((struct stat *)0 != pstLike) {
		(void)fchmod(fdTemp, pstLike->st_mode & 07777);
		(void)fchown(fdTemp, bHasRoot ? pstLike->st_uid : -1, pstLike->st_gid);
	}
	if (-1 == pipe(aiM4)) {
		fprintf(stderr, "%s: pipe: %s\n", progname, strerror(errno));
		close(fdTemp);
		return (FILE *)0;
	}
	for (iTry = 0; iTry < 5; ++iTry) { switch (*pwM4 = fork()) {
	case -1:
		/* we need to backoff and retry at least once
		 */
		if (iTry == 4) {
			fprintf(stderr, "%s: fork m4: %s\n", progname, strerror(errno));
			exit(EX_OSERR);
		}
		sleep(iTry+1);
		continue;
		/*NOTREACHED*/
	case 0:
		close(aiM4[1]);		/* We don't write to ourself */
		if (0 != aiM4[0]) {
			dup2(aiM4[0], 0);
			close(aiM4[0]);
		}
		if (1 != fdTemp) {
			dup2(fdTemp, 1);
			close(fdTemp);
		}
#if defined(M4_SYS_PATH)
		execvp(M4_SYS_PATH, ppcM4Argv);
#endif
		execvp("/usr/bin/m4", ppcM4Argv);
		execvp("/usr/local/bin/m4", ppcM4Argv);
		fprintf(stderr, "%s: execvp: m4: %s\n", progname, strerror(errno));
		exit(EX_OSFILE);
	default:
		close(fdTemp);
		close(aiM4[0]);		/* We don't read m4 input */
		break;
	} break; }
	if ((FILE *)0 == (fpRet = fdopen(aiM4[1], "w"))) {
		fprintf(stderr, "%s: fdopen: %d: %s\n", progname, aiM4[1], strerror(errno));
		close(aiM4[1]);
	}
	return fpRet;
}

/* We have a cache access form, we've built the recipe now run make	(ksb)
 * First make a name for make to help trace errors.
 */
static int
MakeGadget(const char *pcOut, struct stat *pstLike, const char *pcRecipe, char *pcTarget, const char *pcDir)
{
	register int iTry;
	static char *pcMakeName = (char *)0;
	auto pid_t wMake, wWait;
	auto int wStatus;
	auto char *apcMake[8];	/* "make", "-[s]f", $recipe, $target, NULL */
	static const char acTempl[] = "%s: make";

	if ((char *)0 != pcMakeName) {
		/* did it last time, keep it */
	} else if ((char *)0 == (pcMakeName = calloc(wStatus = ((strlen(progname)+sizeof(acTempl))|7)+1, sizeof(char)))) {
		fprintf(stderr, "%s: calloc: %d: %s\n", progname, wStatus,strerror(errno));
		exit(EX_TEMPFAIL);
	} else {
		snprintf(pcMakeName, wStatus, acTempl, progname);
	}

	for (iTry = 0; iTry < 5; ++iTry) { switch (wMake = fork()) {
	case -1:
		/* we need to backoff and retry at least once
		 */
		if (iTry == 4) {
			fprintf(stderr, "%s: fork make: %s\n", progname, strerror(errno));
			exit(EX_OSERR);
		}
		sleep(iTry+1);
		continue;
		/*NOTREACHED*/
	case 0:
		close(1);
		if (-1 == open(pcOut, O_WRONLY, 0666)) {
			exit(EX_OSERR);
		}
		if ((struct stat *)0 != pstLike) {
			(void)fchmod(1, pstLike->st_mode & 07777);
			(void)fchown(1, bHasRoot ? pstLike->st_uid : -1, pstLike->st_gid);
		}
		if ((char *)0 == pcDir || ('.' == pcDir[0] && '\000' == pcDir[1])) {
			/* no chdir */
		} else if (-1 == chdir(pcDir)) {
			fprintf(stderr, "%s: chdir: %s: %s\n", progname, pcDir, strerror(errno));
			exit(EX_OSERR);
		} else if ((char *)0 != strchr(pcDebug, 'C')) {
			fprintf(stderr, "%s: chdir %s\n", progname, pcDir);
		}
		/* building: make -f $recipe $target
		 */
		apcMake[0] = pcMakeName;
		apcMake[1] = "-sf";
		apcMake[2] = (char *)pcRecipe;
		apcMake[3] = pcTarget;
		apcMake[4] = (char *)0;
		if ((char *)0 != strchr(pcDebug, 'C')) {
			register int i;

			fprintf(stderr, "%s:", progname);
			for (i = 0; (char *)0 != apcMake[i]; ++i) {
				fprintf(stderr, " %s", apcMake[i]);
			}
			fprintf(stderr, " >%s\n", pcOut);
			fflush(stderr);
		}
#if defined(MAKE_SYS_PATH)
		execvp(MAKE_SYS_PATH, apcMake);
#endif
		execvp("/usr/bin/make", apcMake);
		execvp("/usr/local/bin/make", apcMake);
		fprintf(stderr, "%s: execvp: make: %s\n", progname, strerror(errno));
		_exit(0);
		_exit(126);
	default:
		break;
	} break; }
	wStatus = 126;
	while (-1 != (wWait = wait(&wStatus)) || EINTR == errno) {
		if (wWait != wMake) {
			continue;
		}
		if (WIFSTOPPED(wStatus)) {
			kill(wMake, SIGCONT);
			continue;
		}
		break;
	}
	return wStatus;
}

/* Run m4 for each file in the slot, return 0 for error,		(ksb)
 * or non-zero for OK.  Some of these might be cache access if we
 * are passed a non-NULL piIsCache && 0 != IsCache[i].
 */
static int
FillSlot(char *pcSlot, char **argv, char **ppcM4Argv, META_HOST *pMHThis, const int *piIsCache, const char *pcCacheCtl)
{
	register int i, cSave;
	auto struct stat stArg;
	auto pid_t wM4, wWait;
	auto int wStatus, wRet;
	register FILE *fpChatter;
	register char *pcAll;
	register const char *pcInto;
	register char *pcTarget, *pcChdir;
	static const char acMarkup[] = "CACHE_";

#if DEBUG
	fprintf(stderr, "%s: FillSlot: %p:\n", progname, pcSlot);
#endif
	pcAll = pcSlot;
	wRet = 0;
	pcChdir = (char *)0;
	for (i = 0; '\000' != *pcSlot && 0 == wRet; ++i) {
		/* If we can't copy the mode/group we still try to process
		 */
		if ((char *)0 == argv[i] || -1 == stat(argv[i], & stArg)) {
			stArg.st_mode = 0700;
			stArg.st_uid = getuid();
			stArg.st_gid = getgid();
		}
#if defined(SLOT_0_ORBITS)
		if (0 == i) {
			stArg.st_mode |= SLOT_0_ORBITS;
		}
#endif
		/* For make path/[.]target[.ext]/recipe.m4 becomes just target
		 * the empty string (from "./recipe.m4" ) becomes the key
		 */
		if ((const int *)0 != piIsCache && 0 != piIsCache[i]) {
			register char *pcSlash, *pcDot, *pcLook;

			pcInto = pcCacheCtl;
			if ((char *)0 != (pcTarget = argv[i])) {
				if ((char *)0 != (pcLook = strrchr(pcTarget, '/'))) {
					*pcLook = '\000';
					pcDot = pcTarget;
					while ('.' == pcDot[0] && '/' == pcDot[1]) {
						while ('/' == *++pcDot)
							;
					}
					pcChdir = strdup(pcDot);
				}
				if ((char *)0 != (pcSlash = strrchr(pcTarget, '/')))
					pcTarget = pcSlash+1;
				while ('.' == *pcTarget)
					++pcTarget;
				if ((char *)0 == (pcDot = strrchr(pcTarget, '.')))
					pcDot = (char *)0;
				else
					*pcDot = '\000';

				if ('\000' == *pcTarget) {
					pcTarget = HostTarget(pMHThis);
				} else if ((char *)0 == (pcTarget = strdup(pcTarget))) {
					fprintf(stderr, "%s: strdup: out of memeory\n", progname);
					exit(EX_TEMPFAIL);
				}
				if ((char *)0 != pcDot)
					*pcDot = '.';
				if ((char *)0 != pcLook)
					*pcLook = '/';
			}
		} else {
			pcChdir = (char *)0;
			pcTarget = (char *)0;
			pcInto = pcSlot;
		}
		if ((char *)0 != pcM4Phase && '\000' == (cSave = *pcM4Phase)) {
			snprintf(pcM4Phase, 64, "%d", i);
		} else {
			cSave = '\000';
		}
		if ((FILE *)0 == (fpChatter = M4Gadget(pcInto, & stArg, & wM4, ppcM4Argv))) {
			exit(EX_OSERR);
		}
		if ((char *)0 != pcTarget) {
			fprintf(fpChatter, "define(`%s%sRECIPE',`%s')define(`%s%sTARGET',`%s')define(`%s%sDIR',`%s')dnl\n", pcXM4Define, acMarkup, pcCacheCtl, pcXM4Define, acMarkup, pcTarget, pcXM4Define, acMarkup, pcChdir);
		}
		SlotDefs(fpChatter, pcAll);
		HostDefs(fpChatter, pMHThis);
		if ((char *)0 != argv[i])
			fprintf(fpChatter, "include(`%s')`'dnl\n", argv[i]);
		fclose(fpChatter);
		if ((char *)0 != strchr(pcDebug, 'D')) {
			register char **ppc;

			fprintf(stderr, "%s:", progname);
			for (ppc = ppcM4Argv; (char *)0 != *ppc; ++ppc) {
				fprintf(stderr, " %s", *ppc);
			}
			fprintf(stderr, "<<\\! >%s\n", pcInto);
			if ((char *)0 != pcTarget) {
				fprintf(stderr, "define(`%s%sRECIPE',`%s')define(`%s%sTARGET',`%s')", pcXM4Define, acMarkup, pcCacheCtl, pcXM4Define, acMarkup, pcTarget);
			}
			SlotDefs(stderr, pcSlot);
			HostDefs(stderr, pMHThis);
			if (-1 == i) {
				fprintf(stderr, "%s\n", argv[i]);
			} else if ((char *)0 != argv[i]) {
				fprintf(stderr, "include(`%s')dnl\n", argv[i]);
			}
			fprintf(stderr, "!\n");
			fflush(stderr);
		}
		if ((char *)0 != pcM4Phase && '\000' == cSave) {
			*pcM4Phase = cSave;
		}
		/* When wait fails we want a known error, nice uses 126
		 */
		wStatus = 126;
		while (-1 != (wWait = wait(&wStatus)) || EINTR == errno) {
			if (wWait != wM4) {
				continue;
			}
			if (WIFSTOPPED(wStatus)) {
				kill(wM4, SIGCONT);
				continue;
			}
			break;
		}
		wRet = wStatus;
		/* Run what ever processor they really wanted, via make.
		 */
		if (0 == wStatus && (char *)0 != pcTarget) {
			wRet = MakeGadget(pcSlot, & stArg, pcCacheCtl, pcTarget, pcChdir);
		}
		if ((char *)0 != pcChdir) {
			free(pcChdir);
			pcChdir = (char *)0;
		}
		pcSlot += strlen(pcSlot)+1;
	}
	return wRet;
}

/* Output the slot, this know that the first position is		(ksb)
 * the literal command, don't do anything for empty slots.
 * We are pretty sure that the user command doesn't have and \000's
 * in it after m4 chewed on it.  We have to stop at the first
 * \000 character, or we have to change the \000 in to spaces.
 */
static void
PrintSlot(char *pcSlot, unsigned uLeft, unsigned uRight, int fdOut)
{
	register unsigned u;
	register int fdCat, iCc;

	if ((char *)0 == pcSlot || '\000' == *pcSlot)
		return;
	for (u = 0; '\000' != *pcSlot; pcSlot += iCc, ++u) {
		iCc = strlen(pcSlot)+1;
		if (u >= uLeft && u < uRight) {
			if (-1 == write(fdOut, pcSlot, iCc)) {
				fprintf(stderr, "%s: write: %d: %s\n", progname, fdOut, strerror(errno));
				return;
			}
			continue;
		}
		if (-1 == (fdCat = open(pcSlot, O_RDONLY, 0))) {
			fprintf(stderr, "%s: open: %s: %s\n", progname, pcSlot, strerror(errno));
			return;
		}
		FdCopy(fdCat, fdOut, pcSlot);
		close(fdCat);
		(void)write(fdOut, "\000", 1);
	}
	errno = 0;
}
/* from envfrom.m */
#line 17 "envfrom.m"
/* Cat all the files together into a malloc'd blob of text		(ksb)
 * We remove comment lines (for #! and mk usage) and puts missing
 * white-space between files.  Newlines become spaces, BTW.
 * We skip the file named "." so you can force "no file" into the
 * list.
 *
 * Added $ENV on a line by itself to interpolate the variable. This
 * allows $HXMD to include the (preset) value in the $HXMD_PASS variable,
 * if the source wants to allow it.
 */
char *
EnvFromFile(char **ppcList, unsigned uCount)
{
	register int c, fNeedPad, fComment, fCol1;
	register FILE *fpIn;
	register char *pcRet;
	register unsigned uEnd, uEnv;
	auto PPM_BUF PPMEver;

	pcRet = (char *)0;
	util_ppm_init(& PPMEver, sizeof(char *), 2048);
	uEnd = 0;
	for (fComment = fNeedPad = 0; uCount > 0; --uCount, ++ppcList) {
		if ('.' == ppcList[0][0] && '\000' == ppcList[0][1]) {
			continue;
		}
		if ((FILE *)0 == (fpIn = fopen(*ppcList, "r"))) {
			fprintf(stderr, "%s: fopen: %s: %s\n", progname, *ppcList, strerror(errno));
			exit(EX_NOINPUT);
		}
		pcRet = util_ppm_size(& PPMEver, (uEnd|1023)+1);
		if (fNeedPad) {
			pcRet[uEnd++] = ' ';
		}
		fCol1 = 1, uEnv = 0;
		while (EOF != (c = getc(fpIn))) {
			if (fComment) {
				if ('\n' == c) {
					fNeedPad = fComment = 0;
					fCol1 = 1;
				}
				continue;
			}
			if (fCol1) {
				uEnv = uEnd;
			}
			if (fCol1 && '#' == c) {
				fComment = 1;
				continue;
			}

			pcRet = util_ppm_size(& PPMEver, (uEnd|255)+1);
			if ((fCol1 = ('\n' == c))) {
				register char *pcFetch;
				register size_t uLen;

				c = ' ';
				pcRet[uEnd] = '\000';
				if ('$' != pcRet[uEnv] || uEnv+2 > uEnd) {
					/* nada */
				} else if ((char *)0 == (pcFetch = getenv(pcRet+uEnv+1)) || '\000' == *pcFetch) {
					uEnd = uEnv;
				} else {
					uLen = strlen(pcFetch);
					pcRet = util_ppm_size(& PPMEver, ((uLen+uEnv+1)|255)+1);
					(void)strcpy(pcRet+uEnv, pcFetch);
					uEnd = uEnv+uLen;
				}
			}
			pcRet[uEnd++] = c;
			fNeedPad = !isspace(c);
		}
		fclose(fpIn);
		if (uEnd > 0 && isspace(pcRet[uEnd])) {
			--uEnd;
		}
		pcRet = util_ppm_size(& PPMEver, (uEnd|3)+1);
		pcRet[uEnd] = '\000';
		/* BUG: XXX a $ENV on the end of the file w/o a \n on
		 * the end leaves the variable name in place --ksb
		 */
	}
	return pcRet;
}
/* from make.m */
#line 135 "make.m"

/* as strrchr is to strchr, this function is to strstr			(ksb)
 */
static char *
strRstr(const char *pcBig, const char *pcSmall)
{
	register const char *pcCur;

	for (pcBig = strstr(pcBig, pcSmall); (char *)0 != pcBig; pcBig = pcCur) {
		if ((char *)0 == (pcCur = strstr(pcBig+1, pcSmall)))
			break;
	}
	return (char *)pcBig;
}

/* The normal C idiom for is this string in the (short) list		(ksb)
 */
static int
InList(char **ppcList, const char *pcValue)
{
	register int cFirst;
	register const char *pc;

	if ((char **)0 == ppcList) {
		return 0;
	}
	for (cFirst = *pcValue; (const char *)0 != (pc = (const char *)*ppcList); ++ppcList) {
		if (cFirst == *pc && 0 == strcmp(pcValue, pc))
			return 1;
	}
	return 0;
}

/* Under -m we can take a "prereq:clean" set of targets, or just	(ksb)
 * the older "prereq" without a clean target (the default).  We are
 * always called before option parsing with "" to set a run-time
 * default value (__+progname).
 * Map the empty strings to the correct values for -m "", -m ":"
 * and -m ":__clearCache" (N.B. if the progname is "clean" you get dups).
 * Easter Egg: we allow "::" to reset the -m option back to defaults.
 */
static void
MakeSetHooks(char **ppcPre, char **ppcPost, char *pcParam)
{
	register char *pcIPre, *pcIPost;

	if ((char *)0 != pcParam && 0 == strcmp("::", pcParam)) {
		pcParam = (char *)0;
	}
	if ((char *)0 == pcParam) {
		uGaveReq = 0;
		*ppcPre = *ppcPost = (char *)0;
	}
	if ((char *)0 == (pcIPre = pcParam)) {
		pcIPost = (char *)0;
	} else if ((char *)0 != (pcIPost = strchr(pcParam, ':'))) {
		*pcIPost++ = '\000';
	}

	/* If we are a reset, or we didnt' have one then form a default
	 * from the current progname, else take what was given.
	 */
	if ((char *)0 == pcIPre || ('\000' == *pcIPre && (char *)0 == *ppcPre)) {
		if ((char *)0 == (*ppcPre = malloc(((strlen(progname)|3)+5)))) {
			fprintf(stderr, "%s: out of memory\n", progname);
			exit(EX_OSERR);
		}
		snprintf(*ppcPre, strlen(progname)+3, "__%s", progname);
	} else if ('\000' != *pcIPre) {
		*ppcPre = pcIPre;
	}

	/* If you didn't give a colon spec, or the one you gave was empty and
	 * we already have a post target, then we are done.
	 * Think about -m ":_cache_clear" -m "_open_bank:", which are in the
	 * wrong order, but should work.
	 */
	if ((char *)0 == pcIPost || ((char *)0 != *ppcPost && '\000' == *pcIPost))
		return;
	*ppcPost = '\000' == *pcIPost ? "__clean" : pcIPost;
}

/* Find the make(1) description file we need to make this work:		(ksb)
 * the first one that we can access(2), or the first that exists.
 * N.B. We are assuming that we are not running setuid/setgid here.
 */
static const char *
FindMakefile()
{
	register unsigned u;
	register const char *pc, *pcAlt;
	auto struct stat stThis;
	static const char *apcLook[] = {
		"Msrc.mk", "msrc.mk", "makefile", "Makefile", (char *)0
	};

	pcAlt = (char *)0;
	for (u = 0; (char *)0 != (pc = apcLook[u]); ++u) {
		if (0 != stat(pc, &stThis) || !S_ISREG(stThis.st_mode)) {
			continue;
		}
		if (0 != access(pc, R_OK)) {
			pcAlt = pc;
			continue;
		}
		return pc;
	}
	return pcAlt;
}

/* Build a temp file in cooperation with $TMPDIR, pcTmpdir util_tmp;	(ksb)
 * reset with (char *)0; return our mkdtemp dir with "".
 */
static char *
TempMake(const char *pcBase)
{
	register int iLen;
	register char *pcRet;
	static char *pcMySpace = (char *)0;

	if ((char *)0 == pcBase) {
		if ((char *)0 != pcMySpace) {
			pcMySpace = (char *)0;
		}
		return (char *)0;
	}
	if ('\000' == pcBase[0]) {
		return pcMySpace;
	}
	if ((char *)0 == pcMySpace) {
		iLen = strlen(pcTmpdir)+1+sizeof(acMakeTempPat)+1;
		iLen |= 15;
		if ((char *)0 == (pcRet = malloc(iLen+1))) {
			return (char *)0;
		}
#if HAVE_SNPRINTF
		snprintf(pcRet, iLen, "%s/%s", pcTmpdir, acMakeTempPat);
#else
		sprintf(pcRet, "%s/%s", pcTmpdir, acMakeTempPat);
#endif
		pcMySpace = mkdtemp(pcRet);
		atexit(TempNuke);
	}
	if ((char *)0 == pcMySpace) {
		return (char *)0;
	}
	iLen = strlen(pcMySpace)+1+strlen(pcBase)+1;
	iLen |= 15;
	if ((char *)0 == (pcRet = malloc(iLen+1))) {
		return (char *)0;
	}
#if HAVE_SNPRINTF
	snprintf(pcRet, iLen, "%s/%s", pcMySpace, pcBase);
#else
	sprintf(pcRet, "%s/%s", pcMySpace, pcBase);
#endif
	return pcRet;
}

/* Get rid of all our /tmp space, please (atexit)			(ksb)
 */
static void
TempNuke()
{
	register char *pcMySpace;
	register DIR *pDR;
	register struct dirent *pDI;
	register int fCarp;
	auto struct stat stDir;
	auto char acRm[MAXPATHLEN+4];

	if ((char *)0 == (pcMySpace = TempMake(""))) {
		return;
	}
	fCarp = 1;
	if ((DIR *)0 != (pDR = opendir(pcMySpace))) {
		while ((struct dirent *)0 != (pDI = readdir(pDR))) {
			if ('.' == pDI->d_name[0] && ('\000' == pDI->d_name[1] || ('.' == pDI->d_name[1] && '\000' == pDI->d_name[2]))) {
				continue;
			}
#if HAVE_SNPRINTF
			snprintf(acRm, sizeof(acRm), "%s/%s", pcMySpace, pDI->d_name);
#else
			sprintf(acRm, "%s/%s", pcMySpace, pDI->d_name);
#endif
			if (-1 == stat(acRm, & stDir) || !S_ISDIR(stDir.st_mode)) {
				if (-1 == unlink(acRm) && fCarp) {
					fprintf(stderr, "%s: unlink: %s: %s\n", progname, acRm, strerror(errno));
					fCarp = 0;
				}
				continue;
			}
			if (-1 == rmdir(acRm)) {
				fprintf(stderr, "%s: rmdir: %s: %s\n", progname, acRm, strerror(errno));
				fCarp = 0;
			}
		}
		closedir(pDR);
	}
	if (fCarp) {
		(void)rmdir(pcMySpace);
	}
	(void)TempMake((char *)0);
}

/* Add the value to the macro's list of values				(ksb)
 */
static void
AddMVValue(MAKE_VAR *pMVThis, char *pcValue)
{
	register char **ppcList;
	register unsigned u;

	u = pMVThis->ucur++;
	if ((char **)0 == (ppcList = (char **)util_ppm_size(& pMVThis->PPMlist, pMVThis->ucur+1))) {
		fprintf(stderr, "%s: malloc: out of memory\n", progname);
		exit(EX_OSERR);
	}
	ppcList[u] = pcValue;
	ppcList[u+1] = (char *)0;
}

/* Read this single macro's values into the PPM bound to it		(ksb)
 * We can use stdio here because we're going to close (fclose) the fd.
 */
static int
AquireMacro(MAKE_VAR *pMVThis)
{
	register FILE *fp;
	register char *pcLine, *pcToken;
	register int iLast;
	auto size_t iSize;

	if ((FILE *)0 == (fp = fdopen(pMVThis->afdpipe[0], "rb"))) {
		return -1;
	}
	pMVThis->ucur = 0;
	iLast = 0;
	while ((char *)0 != (pcLine = fgetln(fp, & iSize))) {
		for (pcToken = (char *)0; iSize > 0; --iSize, ++pcLine) {
			if (isspace(*pcLine)) {
				if ((char *)0 != pcToken) {
					*pcLine = '\000';
					AddMVValue(pMVThis, strdup(pcToken));
					pcToken = (char *)0;
				}
				continue;
			}
			if ((char *)0 != pcToken) {
				continue;
			}
			pcToken = pcLine;
			iLast = iSize;
		}
		if ((char *)0 == pcToken) {
			continue;
		}
		pcLine = malloc((iLast|7)+1);
		strncpy(pcLine, pcToken, iLast);
		pcLine[iLast] = '\000';
		AddMVValue(pMVThis, pcLine);
	}
	return fclose(fp);
}

/* "Make it happen"							(ksb)
 * When make is not in our search path ($PATH) try some internal paths.
 */
static void
ExecMake(char **ppcMake)
{
	register char **ppcCast;
	register int i;

	ppcCast = (char **)ppcMake;
	if (fDebug) {
		fprintf(stderr, "%s:", progname);
		for (i = 0; (char *)0 != ppcCast[i]; ++i) {
			fprintf(stderr, " %s", ppcCast[i]);
		}
		fprintf(stderr, "\n");
		fflush(stderr);
	}
	execvp(PATH_MAKE, ppcCast);
	execvp("/usr/bin/make", ppcCast);
	execvp("/usr/ccs/bin/make", ppcCast);
	execvp("/usr/local/bin/make", ppcCast);
	fprintf(stderr, "%s: execvp: make: %s\n", progname, strerror(errno));
	exit(EX_OSFILE);
}

/* Output the new stanza in the make(1) makefile			(ksb)
 * We allow the makefile to force a depend on the hook for prep work.
 */
static void
MakeServo(int fdOut, MAKE_VAR *pMV, int iCount, const char *pcHook)
{
	register int i;
	auto char acCvt[32];	/* 32 character is a long number here */
	static char acEcho[] = "\techo ${";

	write(fdOut, "\n", 1);
	write(fdOut, pcHook, strlen(pcHook));
	write(fdOut, ":\n", 2);
	for (i = 0; i < iCount; ++i) {
		write(fdOut, acEcho, sizeof(acEcho)-1);
#if HAVE_SNPRINTF
		snprintf(acCvt, sizeof(acCvt), "} 1>&%d\n", pMV[i].afdpipe[1]);
#else
		sprintf(acCvt, "} 1>&%d\n", pMV[i].afdpipe[1]);
#endif
		write(fdOut, pMV[i].pcname, strlen(pMV[i].pcname));
		write(fdOut, acCvt, strlen(acCvt));
	}
	fsync(fdOut);
}

/* Move the read side (only) up above 10 so we can fit more macros	(ksb)
 * in each itteraion of the make process.  Reset the machine with a NULL.
 */
static int
MoveReadUp(int *piFd)
{
	static char iLow = 11;
	register int fdWas;
	auto int iDummy;

	if ((int *)0 == piFd) {
		iLow = 11;
		return 0;
	}
	if (10 < (fdWas = *piFd)) {
		return 0;
	}
	while (-1 != fcntl(iLow, F_GETFD, (void *)&iDummy)) {
		++iLow;
	}
	if (errno == EBADF && -1 != dup2(fdWas, iLow)) {
#if DEBUG
		if (fDebug)
			fprintf(stderr, "moved %d to %d\n", fdWas, iLow);
#endif
		*piFd = iLow;
		++iLow;
		return close(fdWas);
	}
	/* we can't, sigh */
	return 0;
}

/* Get a make(1) rolling to plunder the given make variables.		(ksb)
 * Then read the results and return a success(0) or failure(-1).
 * Called in cleanup mode (when pcFile = (char *)0) we remove our tempfile.
 */
static int
LaunchMake(const char *pcFile, MAKE_VAR *pMV, int iCount)
{
	static char *pcTemp = (char *)0;
	static int fdTemp = -1;
	auto char acBuffer[16*1024];
	register int iCc, iChunk, iVar;
	register char *pcOut, **ppcYoke;
	auto pid_t wMake, wStatus;
	auto int iStatus, fdSource;
	auto char **ppcMake;

	if ((char *)0 == pcFile) {
		if (-1 != fdTemp) {
			close(fdTemp);
			fdTemp = -1;
		}
		if ((char *)0 != pcTemp) {
			(void)unlink(pcTemp);
			free((void *)pcTemp);
			pcTemp = (char *)0;
		}
		return EX_OK;
	}

	/* Copy the file to out temp file, we could even use mmap here -- ksb
	 */
	if (-1 == (fdSource = open(pcFile, O_RDONLY, 0444))) {
		fprintf(stderr, "%s: open: %s: %s\n", progname, pcFile, strerror(errno));
		exit(EX_NOINPUT);
	}
	if (-1 == MoveReadUp(& fdSource)) {
		fprintf(stderr, "%s: dup: %d: %s\n", progname, fdSource, strerror(errno));
		return EX_OSERR;
	}

	if ((char *)0 == pcTemp && (char *)0 == (pcTemp = TempMake("makefile"))) {
		fprintf(stderr, "%s: tempfile: %s\n", progname, strerror(errno));
		return EX_OSERR;
	}
	if (-1 == fdTemp && -1 == (fdTemp = open(pcTemp, O_RDWR|O_CREAT, 0666))) {
		fprintf(stderr, "%s: open: %s: %s\n", progname, pcTemp, strerror(errno));
		return EX_OSERR;
	}
	if (-1 == MoveReadUp(& fdTemp)) {
		fprintf(stderr, "%s: dup: %d: %s\n", progname, fdTemp, strerror(errno));
		return EX_OSERR;
	}

	if ((off_t)0 != lseek(fdTemp, (off_t)0, SEEK_SET)) {
		fprintf(stderr, "%s: lseek: %d: %s\n", progname, fdTemp, strerror(errno));
		exit(EX_OSERR);
	}
	if (-1 == ftruncate(fdTemp, (off_t) 0)) {
		fprintf(stderr, "%s: ftruncate: %d: %s\n", progname, fdTemp, strerror(errno));
		exit(EX_OSERR);
	}
	iCc = 0;
	while (0 < (iChunk = read(fdSource, acBuffer, sizeof(acBuffer)))) {
		pcOut = acBuffer;
		while (0 < iChunk && 0 <= (iCc = write(fdTemp, pcOut, iChunk)))
			pcOut += iCc, iChunk -= iCc;
	}
	if (-1 == iCc) {
		fprintf(stderr, "%s: write: %s(%d): %s\n", progname, pcTemp, fdSource, strerror(errno));
		(void)close(fdSource);
		return EX_OSERR;
	}
	(void)close(fdSource);

	/* generate the pipe connection target and shell code
	 */
	MakeServo(fdTemp, pMV, iCount, pcMakeHook);
	if (fShowMake) {
		fprintf(stderr, "%s: cat %s - <<\\! >%s\n", progname, pcFile, pcTemp);
		fflush(stderr);
		MakeServo(2, pMV, iCount, pcMakeHook);
		fprintf(stderr, "!\n");
	}

	/* Fork the make -f $pcTemp __hook
	 */
	fflush(stdout);
	fflush(stderr);
	switch (wMake = fork()) {
	case -1:
		fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
		/*NOTREACHED*/
	case 0:
		close(fdTemp);
		for (iVar = 0; iVar < iCount; ++iVar) {
			close(pMV[iVar].afdpipe[0]);	/* we don't read */
		}
		iVar = 0;
		if ((char **)0 == (ppcYoke = (char **)util_ppm_size(pPPMMakeOpt, 0))) {
			while ((char *)0 != ppcYoke[iVar])
				++iVar;
		}
		/* make -s -f makefile <yoke> __hook (char *)0 */
		if ((char **)0 == (ppcMake = (char **)calloc(((4+iVar+2)|7)+1, sizeof(char *)))) {
			fprintf(stderr, "%s: calloc: %s\n", progname, strerror(errno));
			exit(EX_OSERR);
		}
		iCc = 0;
		ppcMake[iCc++] = "make";
		ppcMake[iCc++] = "-s";		/* should be able to defeat */
		ppcMake[iCc++] = "-f";
		ppcMake[iCc++] = pcTemp;
		for (iVar = 0; (char **)0 != ppcYoke && (char *)0 != ppcYoke[iVar]; ++iVar) {
			ppcMake[iCc++] = ppcYoke[iVar];
		}
		ppcMake[iCc++] = pcMakeHook;
		ppcMake[iCc++] = (char *)0;

		ExecMake(ppcMake);
		/*NOTREACHED*/
	default:
		for (iVar = 0; iVar < iCount; ++iVar) {
			close(pMV[iVar].afdpipe[1]);	/* we don't write */
		}
		break;
	}
	/* Read his data from each pipe into the buffers provided then close,
	 * and wait for the make process
	 */
	for (iVar = 0; iVar < iCount; ++iVar) {
		(void)AquireMacro(& pMV[iVar]);
	}
	iStatus = EX_UNAVAILABLE << 8;
	while (-1 != (wStatus = wait((void *)& iStatus)) || EINTR == errno) {
		if (wStatus == wMake)
			break;
	}
	return WEXITSTATUS(iStatus);
}


/* Fetch the value of the given variables from the Makefile		(ksb)
 * We assume they are lists of space separated words without quotes.
 * We are going to give them to rdist or rsync or cp, so they should be
 * well-behaved (else those will crash/fail).
 * A (char *) sentinal marks the end of the list, by convention the
 * given make(1) file should not use targets that match m/^__[a-z]*$/ for
 * normal stuff, as we use those.
 *
 * We can only use (at most) fd's 3-9 (7 of them) at a time, as the shell
 * won't dup any two digit descriptors.  Sigh.  Well just fork more make
 * processes until they are all satisfied.
 */
int
GatherMake(const char *pcFrom, MAKE_VAR *pMV)
{
	register unsigned int u, uBase;
	register int iRet;

	iRet = EX_OK;
	if ((MAKE_VAR *)0 == pMV || (char *)0 == pMV->pcname) {
		return iRet;
	}
	(void)MoveReadUp((int *)0);
	for (uBase = u = 0; (char *)0 != pMV[u].pcname; /* humm */) {
		if (-1 == pipe(pMV[u].afdpipe)) {
			/* no more pipes, try to flush to free fds */
		} else if (10 <= pMV[u].afdpipe[1] && 10 <= pMV[u].afdpipe[0]) {
			/* oops both sides >= 10, free some lower numbers */
			close(pMV[u].afdpipe[0]);
			close(pMV[u].afdpipe[1]);
		} else if (-1 == MoveReadUp(& pMV[u].afdpipe[0])) {
			/* fatal error on close of read-side */
			close(pMV[u].afdpipe[1]);
			close(pMV[u].afdpipe[0]);
		} else {
			/* move the write down if we must */
			if (10 <= pMV[u].afdpipe[1]) {
				register int iT;
				iT = pMV[u].afdpipe[1];
				pMV[u].afdpipe[1] = dup(iT);
				close(iT);
			}
			/* we queue'd it, yeah */
			if (-1 != pMV[u].afdpipe[1]) {
				util_ppm_init(& pMV[u].PPMlist, sizeof(char *), 0);
				++u;
				continue;
			}
			close(pMV[u].afdpipe[0]);
		}

		/* When we can't even get one going, use stdout?
		 * I don't think we can even use stdout, at least no with 100%
		 * certainty that the Customer's tools chain doesn't use it
		 * as a resource to get the file list (viz. prompts). --ksb
		 */
		if (uBase == u) {
			fprintf(stderr, "%s: make -f %s: pipe: no fd < 10 available\n", progname, pcFrom);
			return -1;
		}
		if (EX_OK != (iRet = LaunchMake(pcFrom, pMV+uBase, u-uBase))) {
			return iRet;
		}
		uBase = u;
	}
	if (uBase < u && EX_OK != (iRet = LaunchMake(pcFrom, pMV+uBase, u-uBase))) {
		return iRet;
	}

	/* cleanup and return our victory
	 */
	return LaunchMake((char *)0, pMV, u);
}

/* Accessor method for the list we got					(ksb)
 */
static char **
GetMVValue(MAKE_VAR *pMVThis)
{
	return (char **)util_ppm_size(& pMVThis->PPMlist, 0);
}

/* If token "+++" is part of the make variable allow adds to it		(ksb)
 * (and remove the token).  Use "./+++" to send a file name "+++".
 * The opposite is "." which means don't add any (more) to this macro.
 * Return bit-0 for open ended, bit-1 for closed, no bits for unspecified.
 */
static int
OpenEnded(MAKE_VAR *pMVThis)
{
	register char *pc, **ppcIn, **ppcOut;
	register int iRet;

	iRet = 0;
	if ((char **)0 == (ppcOut = GetMVValue(pMVThis))) {
		return iRet;
	}
	for (ppcIn = ppcOut; (char *)0 != (pc = *ppcIn); ++ppcIn) {
		if (0 == strcmp(acAutoPlus, pc))
			--pMVThis->ucur, iRet |= 1;
		else if (0 == strcmp(acNoAuto, pc))
			--pMVThis->ucur, iRet |= 2;
		else
			*ppcOut++ = pc;
	}
	*ppcOut = (char *)0;
	return iRet;
}

/* Accessor method for the number of words we saw in the value		(ksb)
 */
static unsigned
GetMVCount(MAKE_VAR *pMVThis)
{
	return pMVThis->ucur;
}

/* Look for the variables we need from the Makefile given		(ksb)
 * The files must be in ".", we are $PWD context sensitive like make(1)
 */
static int
Plunder(MAKE_VAR *pMVList, unsigned uList, const char *pcRecipes)
{
	register unsigned i;
	register struct dirent *pDE;
	register DIR *pDRDot;
	register const char *pcTail;
	register int iCode;
	register char **ppcTrim, *pcSnip;
	auto struct stat stThis;

	if ((MAKE_VAR *)0 == pMVList) {
		pMVList = aMVNeed;
		uList = (sizeof(aMVNeed)/sizeof(aMVNeed[0]))-1;
	}
	/* Sigh, make exits "1" for any nonzero exit from a recipe, which
	 * is not a useful exit code for me -- ksb
	 */
	if (EX_OK != (iCode = GatherMake(pcRecipes, pMVList))) {
		fprintf(stderr, "%s: %s: make failed with exit code %d\n", progname, pcRecipes, iCode);
		return EX_NOINPUT;
	}
	/* If you set SUBDIR (MAP, HXINCLUDE) to "." we take that to mean
	 * you have none, all the dirs in here are the opposite kind dirs.
	 * Empty macros imply auto selection, or macros with the open-ended
	 * token "+++".  This could remove a lone dot from a messaged INTO,
	 * that's why we can put an underscore before it.
	 */
	for (i = 0; i < uList; ++i) {
		pMVList[i].flocks = OpenEnded(& pMVList[i]);
		if (2 & pMVList[i].flocks)
			pMVList[i].fsynth = 0;
		else if ((1 & pMVList[i].flocks) || 0 == GetMVCount(& pMVList[i]))
			pMVList[i].fsynth = 1;
	}
	/* The MAP macro can have directory entries now -- hxmd does it,
	 * so we must remove any training "/" from the names of dirs in MAP
	 * or the SUBDIR code will add them redundantly.	(ksb)
	 */
	ppcTrim = GetMVValue(& pMVList[DX_MAP]);
	for (i = GetMVCount(& pMVList[DX_MAP]); i-- > 0; ) {
		while ((char *)0 != (pcSnip = strrchr(ppcTrim[i], '/')) && '\000' == pcSnip[1])
			*pcSnip = '\000';
	}
	if ((DIR *)0 == (pDRDot = opendir("."))) {
		fprintf(stderr, "%s: opendir: .: %s\n", progname, strerror(errno));
		return EX_NOPERM;
	}

	/* Disposition of each file made easy here -- ksb
	 * skip . and .., of course
	 * if the node is in one of (MAP, SEND, IGNORE) skip it
	 * if -d node
	 *	Iff named RCS, SCCS, SVN, CVS skip it
	 *	Iff no SUBDIR, then [-d] (dirs) -> SUBDIR
	 *	continue
	 * if ! -f put
	 *	-> IGNORE
	 *	continue
	 * if none in MAP, then *.host -> MAP (else fall into SEND)
	 * if none in SEND and -f || -L (files or symlink) -> SEND
	 */
	while ((struct dirent *)0 != (pDE = readdir(pDRDot))) {
		if ('.' == pDE->d_name[0] && ('\000' == pDE->d_name[1] || ('.' == pDE->d_name[1] && '\000' == pDE->d_name[2]))) {
			continue;
		}
		if (-1 == stat(pDE->d_name, & stThis)) {
			fprintf(stderr, "%s: stat: %s: %s\n", progname, pDE->d_name, strerror(errno));
			return EX_NOPERM;
		}
		if (InList(GetMVValue(& pMVList[DX_MAP]), pDE->d_name) || InList(GetMVValue(& pMVList[DX_SEND]), pDE->d_name) || InList(GetMVValue(& pMVList[DX_IGNORE]), pDE->d_name)) {
			continue;
		}

		/* Any directory we find that is not a revision control one
		 * we try to add to SUBDIR or MAP, if we are building each.
		 * MAP gets it if it ends in ".host", just like a file.
		 */
		if (S_ISDIR(stThis.st_mode)) {
			register char *pcCheck;
			for (i = 0; (char *)0 != apcSkipDir[i]; ++i)
				if (0 == strcmp(apcSkipDir[i], pDE->d_name))
					break;
			if ((char *)0 != apcSkipDir[i])
				continue;
			if (InList(GetMVValue(& pMVList[DX_SUBDIR]), pDE->d_name))
				continue;

			if (0 != pMVList[DX_MAP].fsynth && (char *)0 != (pcCheck = strRstr(pDE->d_name, acMapSuf)) && pDE->d_name != pcCheck && '\000' == pcCheck[sizeof(acMapSuf)-1])
				AddMVValue(& pMVList[DX_MAP], strdup(pDE->d_name));
			else if (0 != pMVList[DX_SUBDIR].fsynth)
				AddMVValue(& pMVList[DX_SUBDIR], strdup(pDE->d_name));
			else if (0 != pMVList[DX_IGNORE].fsynth)
				AddMVValue(& pMVList[DX_IGNORE], strdup(pDE->d_name));
			continue;
		}
		if (!S_ISREG(stThis.st_mode)) {
			AddMVValue(& pMVList[DX_IGNORE], strdup(pDE->d_name));
			continue;
		}

		if (0 != pMVList[DX_MAP].fsynth && (const char *)0 != (pcTail = strRstr(pDE->d_name, acMapSuf)) && pDE->d_name != pcTail && '\000' == pcTail[sizeof(acMapSuf)-1]) {
			AddMVValue(& pMVList[DX_MAP], strdup(pDE->d_name));
			continue;
		}
		if (0 != pMVList[DX_HXINCLUDE].fsynth && (const char *)0 != (pcTail = strRstr(pDE->d_name, acInclSuf)) && '\000' == pcTail[sizeof(acInclSuf)-1]) {
			AddMVValue(& pMVList[DX_HXINCLUDE], strdup(pDE->d_name));
			continue;
		}
		if (0 != pMVList[DX_SEND].fsynth) {
			AddMVValue(& pMVList[DX_SEND], strdup(pDE->d_name));
			continue;
		}
		if (0 != pMVList[DX_IGNORE].fsynth) {
			AddMVValue(& pMVList[DX_IGNORE], strdup(pDE->d_name));
			continue;
		}
		/* If you locked SEND, INCLUDE, and IGNORE we have to map it
		 * (so do not lock IGNORE).
		 */
		if (0 != pMVList[DX_MAP].fsynth) {
			AddMVValue(& pMVList[DX_MAP], strdup(pDE->d_name));
			continue;
		}
	}
	closedir(pDRDot);

	/* Iff no INTO, then try to match $PWD =~ s!/msrc/!/src/!o
	 */
	if (0 != pMVList[DX_INTO].fsynth) {
		register char *pcMem, *pcCut;

		if ((char *)0 == (pcMem = malloc(MAXPATHLEN+4))) {
			fprintf(stderr, "%s: malloc: %d: %s\n", progname, MAXPATHLEN+3, strerror(errno));
			return EX_OSERR;
		}
		if ((char *)0 == (pcMem = getcwd(pcMem, MAXPATHLEN+4))) {
			fprintf(stderr, "%s: getcwd: %s\n", progname, strerror(errno));
			return EX_OSERR;
		}
		if ((char *)0 == (pcCut = strstr(pcMem, "/msrc/"))) {
			fprintf(stderr, "%s: %s: cannot deduce \"INTO\" not under \"msrc\": %s\n", progname, pcRecipes, pcMem);
			return EX_DATAERR;
		}
		memmove(pcCut+1, pcCut+2, strlen(pcCut+1));
		AddMVValue(& pMVList[DX_INTO], pcMem);
	}
	/* When into has more than one value we are baked --ksb
	 * Easter eggs are us, when INTO="_reason _for _botch..."
	 */
	if (1 != (i = GetMVCount(& pMVList[DX_INTO]))) {
		register char **ppcEgg, *pcText;

		if (1 > i || (char **)0 == (ppcEgg = GetMVValue(& pMVList[DX_INTO])) || '_' != ppcEgg[0][0]) {
			fprintf(stderr, "%s: %s: \"INTO\" should have exactly one value, not %d\n", progname, pcRecipes, i);
		} else {
			fprintf(stderr, "%s: %s:", progname, pcRecipes);
			while ((char *)0 != (pcText = *ppcEgg++)) {
				if ('_' == *pcText)
					++pcText;
				fprintf(stderr, " %s", pcText);
			}
			fprintf(stderr, "\n");
		}
		return EX_DATAERR;
	}
	return 0;
}

/* Get the data form the makefile and condition it a little:		(ksb)
 * we add the MAP files mapped name to the IGNORE list.  Both makeme
 * and msrc itself need this common data.
 */
static unsigned
Booty(BOOTY *pBO, const char *pcRecipes, FILE *fpShow)
{
	static char acRsyncHack[] = "/./";
	register unsigned i;
	register char **ppc;
	register char *pcTail, *pcOrig;

	if (EX_OK != (i = Plunder(aMVNeed, (sizeof(aMVNeed)/sizeof(aMVNeed[0]))-1, pcRecipes))) {
		exit(i);
	}
	if ((char **)0 == (ppc = GetMVValue(& aMVNeed[DX_INTO])) || (char *)0 == *ppc) {
		fprintf(stderr, "%s: %s: INTO: no value found\n", progname, pcRecipes);
		exit(EX_CONFIG);
	}
	pBO->pcinto = *ppc;
	if ((char **)0 == (ppc = GetMVValue(& aMVNeed[DX_MODE])) || (char *)0 == *ppc || 0 == (i = GetMVCount(& aMVNeed[DX_MODE]))) {
		AddMVValue(& aMVNeed[DX_MODE], "auto");
		aMVNeed[DX_MODE].fsynth = 1;
		ppc = GetMVValue(& aMVNeed[DX_MODE]);
		i = GetMVCount(& aMVNeed[DX_MODE]);
	}
	if (1 != i) {
		fprintf(stderr, "%s: %s: MODE: too many values (%u)\n", progname, pcRecipes, i);
		exit(EX_CONFIG);
	} else {
		pBO->pcmode = *ppc;
	}
	pBO->ppcorig = GetMVValue(& aMVNeed[DX_MAP]);
	pBO->umap = pBO->uorig = GetMVCount(& aMVNeed[DX_MAP]);
	pBO->ppcsubdir = GetMVValue(& aMVNeed[DX_SUBDIR]);
	pBO->usubdir = GetMVCount(& aMVNeed[DX_SUBDIR]);
	if ((char **)0 == (pBO->ppcmap = (char **)calloc((pBO->umap|7)+1, sizeof(char *)))) {
		fprintf(stderr, "%s: calloc: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	/* Build a target/map path vector from orig/send, we honor the rsync
	 * "/./" markup _or_ remove any "../" prefix from the source path. (ksb)
	 */
	for (i = 0; i < pBO->umap; ++i) {
		pcOrig = pBO->ppcorig[i];
		if ((char *)0 != (pcTail = strRstr(pcOrig, acRsyncHack))) {
			pcOrig = pcTail + (sizeof(acRsyncHack)-1);
			while ('/' == *pcOrig)
				++pcOrig;
		} else {
			while ('.' == pcOrig[0] && '.' == pcOrig[1] && '/' == pcOrig[2]) {
				pcOrig += 2;
				while ('/' == *pcOrig)
					++pcOrig;
			}
		}
		pBO->ppcmap[i] = strdup(pcOrig);
		if ((char *)0 != (pcTail = strRstr(pBO->ppcmap[i], acMapSuf)) && '\000' == pcTail[sizeof(acMapSuf)-1])
			*pcTail = '\000';
		if (InList(GetMVValue(& aMVNeed[DX_IGNORE]), pBO->ppcmap[i]))
			continue;
		AddMVValue(& aMVNeed[DX_IGNORE], pBO->ppcmap[i]);
	}
	pBO->ppcsend = GetMVValue(& aMVNeed[DX_SEND]);
	pBO->utarg = pBO->usend = GetMVCount(& aMVNeed[DX_SEND]);
	if ((char **)0 == (pBO->ppctarg = (char **)calloc((pBO->usend|7)+1, sizeof(char *)))) {
		fprintf(stderr, "%s: calloc: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	pBO->usplay = 0;
	for (i = 0; i < pBO->usend; ++i) {
		pcOrig = pBO->ppcsend[i];
		if ((char *)0 != (pcTail = strRstr(pcOrig, acRsyncHack))) {
			++pBO->usplay;
			pcOrig = pcTail + (sizeof(acRsyncHack)-1);
			while ('/' == *pcOrig)
				++pcOrig;
		} else {
			while ('.' == pcOrig[0] && '.' == pcOrig[1] && '/' == pcOrig[2]) {
				pcOrig += 2;
				while ('/' == *pcOrig)
					++pcOrig;
			}
		}
		pBO->ppctarg[i] = pcOrig;
	}
	pBO->ppcignore = GetMVValue(& aMVNeed[DX_IGNORE]);
	pBO->uignore = GetMVCount(& aMVNeed[DX_IGNORE]);
	pBO->ppchxinclude = GetMVValue(& aMVNeed[DX_HXINCLUDE]);
	pBO->uhxinclude = GetMVCount(& aMVNeed[DX_HXINCLUDE]);

	for (i = 0; (FILE *)0 != fpShow && i < (sizeof(aMVNeed)/sizeof(aMVNeed[0]))-1; ++i) {
		if ((char **)0 == (ppc = GetMVValue(& aMVNeed[i]))) {
			fprintf(fpShow, "%s: [empty]\n", aMVNeed[i].pcname);
			continue;
		}
		fprintf(fpShow, "%s: [%s]\n", aMVNeed[i].pcname, (2 & aMVNeed[i].flocks) ? "make locked" : (1 & aMVNeed[i].flocks) ? "make open" : 0 == aMVNeed[i].fsynth ? "make" : "generated");
		for (/* ppc */; (char *)0 != *ppc; ++ppc) {
			fprintf(fpShow, "\t%s\n", *ppc);
		}
	}
	return 0;
}
/* from util_fsearch.m */
/* from util_tmp.m */
/* from util_home.m */
/* from argv.m */

#line 1 "getopt.mc"
/* $Id: getopt.mc,v 8.13 2004/09/01 15:26:37 ksb Exp $
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

#if 0 || 0
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
	if ('#' == *oli) {		/* accept as -digits, not "-" alone */
		u_optarg = place-1;
		while (isdigit(*place)) {
			++place;
		}
		if(!*place) ++u_optInd;
		return isdigit(place[-1]) ? '#' : '?';
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
#line 1 "util_fgetln.mc"
/* $Id: util_fgetln.mc,v 8.8 1999/06/14 13:27:42 ksb Exp $
 */

/* return a pointer to the next line in the stream			(ksb)
 * set *piRet = strlen(the_line); but NO '\000' is appended
 * this is really cheap if implemented in stdio.h, but not so
 * cool when we have to emulate it.  Sigh.
 *
 * N.B. We grab RAM in 128 byte increments, because we never free it.
 * [the stdio implementation frees it when you fclose() the fp, of course]
 */
char *
fgetln(fp, piRet)
FILE *fp;
size_t *piRet;
{
	static char *pcFGBuf = (char *)0;
	static size_t iSize = 0;
	register size_t iRead;
	register int c;
	register char *pc;	/* realloc test */

	if (0 == iSize) {
		iSize = 256;
		pcFGBuf = calloc(iSize, sizeof(char));
	}

	/* inv. the buffer has room for the next character at least
	 */
	for (iRead = 0; EOF != (c = getc(fp)); /* nothing */) {
		if ('\n' == (pcFGBuf[iRead++] = c))
			break;
		if (iRead < iSize)
			continue;
		iSize += 128;
		pc = realloc(pcFGBuf, iSize * sizeof(char));
		if ((char *)0 == pc)
			return (char *)0;
		pcFGBuf = pc;
	}
	pcFGBuf[iRead] = '\000';
	if ((size_t *)0 != piRet)
		*piRet = iRead;

	return (0 == iRead) ? (char *)0 : pcFGBuf;
}
#line 1 "util_fsearch.mc"
/* $Id: util_fsearch.mc,v 8.10 1998/09/16 13:37:42 ksb Exp $
 */

/* we take a vector of possible lexicons and a $PATH mocking		(ksb)
 * list of places to search for them.  I'd rather not take the $PATH
 * compatible, but it is the most common use, so I used it.
 */
char *
util_fsearch(ppcScan, pcPath)
char **ppcScan;
char *pcPath;
{
	register char *pcThis, *pcCursor, *pcWrite;
	auto char acFull[MAXPATHLEN+4];
	auto struct stat stDummy;
	static char acDefPath[] = ".:/usr/local/lib/hxmd:/usr/local/lib/distrib";

	if ((char *)0 == pcPath) {
		pcPath = acDefPath;
	}

	/* search lexicons for full paths ref, or
	 * relatives with each prefix tried
	 */
	while ((char *)0 != (pcThis = *ppcScan++)) {
		acFull[0] = '\000';
		if ('$' == pcThis[0]) {
			if ((char *)0 == (pcThis = getenv(pcThis+1))) {
				continue;
			}
		}
		if ('~' == pcThis[0]) {
			pcThis = util_home(acFull, pcThis+1);
			if ((char *)0 == pcThis) {
				continue;
			}
			(void)strcat(acFull, pcThis);
		} else if ('/' == pcThis[0]) {
			(void)strcpy(acFull, pcThis);
		}
		if ('\000' != acFull[0]) {
			if (-1 != stat(acFull, & stDummy)) {
				return strdup(acFull);
			}
			continue;
		}

		/* search colon-separated prefix path
		 */
		pcWrite = (char *)0;
		for (pcCursor = pcPath; /**/; ++pcCursor) {
			if ((char *)0 == pcWrite) {
				if ('\000' == *pcCursor)
					break;
				pcWrite = acFull;
			}
			if (':' != *pcCursor && '\000' != *pcCursor) {
				*pcWrite++ = *pcCursor;
				continue;
			}
			if (acFull != pcWrite && pcWrite[-1] != '/') {
				*pcWrite++ = '/';
			}
			(void)strcpy(pcWrite, pcThis);
			if (-1 != stat(acFull, & stDummy)) {
				return strdup(acFull);
			}
			if ('\000' == *pcCursor) {
				break;
			}
			pcWrite = (char *)0;
		}
	}
	return (char *)0;
}
#line 1 "util_home.mc"
/* $Id: util_home.mc,v 8.8 1998/09/16 13:34:24 ksb Exp $
 */

/* find a home directory for ~usr style interface			(ksb)
 *	do ~login and ~
 * Input login names are terminated by '\000' or '/' or ':',
 * anything else you can do.  pcDest should have about MAXPATHLEN
 * characters available, more or less.
 *
 * We return the pointer after the consumed text, or (char *)0 for fail,
 * if the destination buffer is empty we could not find $HOME/getpwuid,
 * else we could not find the login name in the buffer.
 */
char *
util_home(pcDest, pcWho)
char *pcDest, *pcWho;
{
	extern char *getenv();
	register char *pcHome;
	register struct passwd *pwd;
	register char *pcTmp;

	pcHome = getenv("HOME");
	if ((char *)0 == pcHome) {
		pwd = getpwuid(getuid());
		/* only good for a short time... long enough
		 */
		if ((struct passwd *)0 != pwd) {
			pcHome = pwd->pw_dir;
		}
	}
	if ((char *)0 == pcWho || '/' == *pcWho || ':' == *pcWho || '\000' == *pcWho) {
		if ((char *)0 == pcHome) {
			*pcDest = '\000';
			return (char *)0;
		}
		(void)strcpy(pcDest, pcHome);
		return (char *)0 == pcWho || '\000' == *pcWho ? "" : pcWho;
	}

	pcTmp = pcDest;
	while ('/' != *pcWho && ':' != *pcWho && '\000' != *pcWho) {
		*pcTmp++ = *pcWho++;
	}
	*pcTmp = '\000';

	if ((struct passwd *)0 == (pwd = getpwnam(pcDest))) {
		return (char *)0;
	}
	(void)strcpy(pcDest, pwd->pw_dir);
	return pcWho;
}
#line 1 "argv.mc"
/* $Id: argv.mc,v 8.10 2002/12/02 18:05:33 ksb Exp $
 * ksb's support for a list of words taken from mulitple options	(ksb)
 * which together form an argv (kinda).
 */

#if !defined(U_ARGV_BLOB)
#define U_ARGV_BLOB	16
#endif
#if 0 == U_ARGV_BLOB
#undef U_ARGV_BLOB
#define U_ARGV_BLOB	2
#endif

/* Add an element to the present vector, yeah we are N^2,		(ksb)
 * and we depend on the init below putting an initial (char *)0 in slot 0.
 */
static void
u_argv_add(pPPM, pcWord)
struct PPMnode *pPPM;
char *pcWord;
{
	register char **ppc;
	register int i;
	if ((struct PPMnode *)0 == pPPM || (char *)0 == pcWord)
		return;
	ppc = util_ppm_size(pPPM, U_ARGV_BLOB);
	for (i = 0; (char *)0 != ppc[i]; ++i)
		/* nada */;
	ppc = util_ppm_size(pPPM, ((i+1)/U_ARGV_BLOB)*U_ARGV_BLOB);
	ppc[i] = pcWord;
	ppc[i+1] = (char *)0;
}

/* Setup to process all the argv's the Implementor declared.		(ksb)
 * We allocate them all at once, then the initial vectors for each.
 */
static void
u_argv_init()
{
	register struct PPMnode *pPPMInit;
	register char **u_ppc;

	if ((struct PPMnode *)0 == (pPPMInit = (struct PPMnode *)calloc(4, sizeof(struct PPMnode)))) {
		fprintf(stderr, "%s: calloc: %d: no core\n", progname, 4);
		exit(60);
	}
	pPPMGuard = util_ppm_init(pPPMInit++, sizeof(char *), U_ARGV_BLOB);
	u_ppc = (char **)util_ppm_size(pPPMGuard, 2);
	u_ppc[0] = (char *)0;	/* sentinal value, do not remove */
	pPPMM4Prep = util_ppm_init(pPPMInit++, sizeof(char *), U_ARGV_BLOB);
	u_ppc = (char **)util_ppm_size(pPPMM4Prep, 2);
	u_ppc[0] = (char *)0;	/* sentinal value, do not remove */
	pPPMMakeOpt = util_ppm_init(pPPMInit++, sizeof(char *), U_ARGV_BLOB);
	u_ppc = (char **)util_ppm_size(pPPMMakeOpt, 2);
	u_ppc[0] = (char *)0;	/* sentinal value, do not remove */
	pPPMSpells = util_ppm_init(pPPMInit++, sizeof(char *), U_ARGV_BLOB);
	u_ppc = (char **)util_ppm_size(pPPMSpells, 2);
	u_ppc[0] = (char *)0;	/* sentinal value, do not remove */
#line 52 "argv.mc"
}
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
		sbOpt[] = "bB:C:d:D:E:f:G:hI:j:k:lm:M:o:P::su:U:VX:y:Y:zZ:",
		*u_pch = (char *)0;
	static unsigned
		iParallel = 1;
	static int
		u_loop = 0;
	static char
		*pcETmp = (char *)0;
	register int u_curopt;
	register char *u_pcEnv;
	extern char *getenv();
	extern int atoi();
	extern int atoi();

	if ((char *)0 == argv[0]) {
		progname = "mmsrc";
	} else if ((char *)0 == (progname = strrchr(argv[0], '/'))) {
		progname = argv[0];
	} else {
		++progname;
	}
	MacroStart();
	HostStart(".:/usr/local/lib/hxmd:/usr/local/lib/distrib");
	u_argv_init();
	MmsrcStart();
	if ((char *)0 != (pcETmp = getenv("MMSRC_PASS")) && '\000' != *pcETmp) {
		u_envopt(pcETmp, & argc, & argv);
	}
	 else if ((char *)0 != (pcETmp = getenv("MMSRC"))) {
		u_envopt(pcETmp, & argc, & argv);
	}
	/* "mmsrc" forces no options */
	pcMakefile = (char *)FindMakefile();
	u_argv_add(pPPMGuard, (void *)0);
	u_argv_add(pPPMM4Prep, (void *)0);
	u_argv_add(pPPMMakeOpt, (void *)0);
	u_argv_add(pPPMSpells, (void *)0);
	if ((char *)0 != (u_pcEnv = getenv("TMPDIR"))) {
		pcTmpdir = u_pcEnv;
	}
	bHasRoot = getuid() == 0;
	if ((char *)0 == pcTmpdir || '\000' == *pcTmpdir) {
		pcTmpdir = "/tmp";
	}
	MakeSetHooks(& pcMakeHook, & pcMakeClean, "__msrc");
	/* used to move -l up to before AfterGlow */
	while (EOF != (u_curopt = u_getopt(argc, argv, sbOpt, (char *)0))) {
		switch (u_curopt) {
		case '*':
			fprintf(stderr, "%s: option `-%c\' needs a parameter\n", progname, u_optopt);
			exit(1);
		case '?':
			fprintf(stderr, "%s: unknown option `-%c\', use `-h\' for help\n", progname, u_optopt);
			exit(1);
		case 'b':
			u_chkonly('b', u_curopt, "BCDEGXYZbjkou");
			uGaveB = 1;
			/* in our list */
			continue;
		case 'B':
			u_chkonly('B', u_curopt, "b");
			AddBoolean(u_curopt, u_optarg);
			continue;
		case 'C':
			u_chkonly('C', u_curopt, "b");
			pcConfigs = optaccum(pcConfigs, u_optarg, ":");
			continue;
		case 'd':
			pcDebug = u_optarg;
			DebugFilter(pcDebug);
			continue;
		case 'I':
		case 'U':
		case 'D':
			u_chkonly('D', u_curopt, "b");
			AddSwitch(u_curopt, u_optarg);
			continue;
		case 'E':
			u_chkonly('E', u_curopt, "b");
			AddExclude(u_curopt, u_optarg);
			continue;
		case 'f':
			pcMakefile = u_optarg;
			continue;
		case 'G':
			u_chkonly('G', u_curopt, "b");
			u_argv_add(pPPMGuard, u_optarg);
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
			printf("%s: also any database option to hxmd\n", progname);
			exit(0);
		case 'j':
			u_chkonly('j', u_curopt, "b");
			u_argv_add(pPPMM4Prep, u_optarg);
			continue;
		case 'k':
			u_chkonly('k', u_curopt, "b");
			pcHOST = u_optarg;
			continue;
		case 's':
		case 'l':
			fLocalForced |= u_curopt == 'l';
			continue;
		case 'm':
			uGaveReq = 1;
			MakeSetHooks(& pcMakeHook, & pcMakeClean, u_optarg);
			continue;
		case 'M':
			pcXM4Define = u_optarg;
			continue;
		case 'o':
			u_chkonly('o', u_curopt, "b");
			fGaveMerge = 1;
			pcOutMerge = optaccum(pcOutMerge, u_optarg, "\t");
			continue;
		case 'P':
			fGavePara = 1;
			if ((char *)0 == u_optarg) {
				if ((char *)0 == (u_optarg = getenv("PARALLEL")) || '\000' == *u_optarg) {
					u_optarg = "1";
				}
			}
			iParallel = atoi(u_optarg);
			continue;
		case 'u':
			u_chkonly('u', u_curopt, "b");
			SetEntryLogin(u_curopt, u_optarg);
			continue;
		case 'V':
			printf("%s: %s\n", progname, rcsid);
			Version();
			exit(0);
		case 'X':
			u_chkonly('X', u_curopt, "b");
			pcExConfig = optaccum(pcExConfig, u_optarg, ":");
			continue;
		case 'y':
			u_argv_add(pPPMMakeOpt, u_optarg);
			continue;
		case 'Y':
			u_chkonly('Y', u_curopt, "b");
			u_argv_add(pPPMSpells, u_optarg);
			continue;
		case 'z':
			fHXFlags = ! 1;
			continue;
		case 'Z':
			u_chkonly('Z', u_curopt, "b");
			pcZeroConfig = optaccum(pcZeroConfig, u_optarg, ":");
			continue;
		}
		break;
	}
	if (fLocalForced) {
		u_argv_add(pPPMMakeOpt, "MODE=local");
	}
	AfterGlow(argc, argv);
	if (fGaveMerge && (char *)0 == pcOutMerge) {
		pcOutMerge = "";
	}
	if (iParallel < 1) {
		iParallel = 1;
	}
	/* alternate for option b */
	if (uGaveB) {
		Msync((argc-u_optInd), (argv+u_optInd));
		exit(EX_OK);
		exit(0);
		/*NOTREACHED*/
	}
	List((argc-u_optInd), & argv[u_optInd]);
	exit(0);
	/*NOTREACHED*/
}
