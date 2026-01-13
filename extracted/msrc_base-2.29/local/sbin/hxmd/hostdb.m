#!mkcmd
# $Id: hostdb.m,v 1.75 2010/07/26 17:24:35 ksb Exp $
# host seletion options and config file reader				(ksb)
# The implemetor must require "evector.m" before us, or a replacement.
from '<stdio.h>'
from '<string.h>'
from '<sys/types.h>'
from '<sys/socket.h>'
from '<sys/un.h>'

require "util_fsearch.m"
key "util_fsearch_cvt" {
	".:/usr/local/lib/hxmd:/usr/local/lib/distrib"
}
require "util_ppm.m"

accum[":"] 'C' {
	named "pcConfigs"
	param "configs"
	help "files of hosts and attribute macros, colon separated"
}
function 'B' {
	named "AddBoolean"
	param "macros"
	help "boolean check, macros must be defined to select host"
}
type "argv" 'G' {
	named "pPPMGuard"
	param "guard"
	help "expressions to select hosts by name"
}
function 'E' {
	named "AddExclude"
	param "compares"
	help "macro value must satisfy given relation to select hosts"
}
accum[":"] 'X' {
	named "pcExConfig"
	param "ex-configs"
	help "add attribute macro data to defined hosts, colon separated"
}
type "argv" 'Y' {
	named "pPPMSpells"
	param "top"
	help "m4 commands included at the top of the guard processing"
}
accum[":"] 'Z' {
	named "pcZeroConfig"
	param "zero-config"
	help "configuration file(s) to set default macro values"
}
char* 'k' {
	named "pcHOST"
	init '"HOST"'
	param "key"
	help "change the merge macro name from %qi to key"
}
accum ["\\t"] 'o' {
	named "pcOutMerge"
	track "fGaveMerge"
	init "(char *)0"
	param "attributes"
	help "output format of the merged configuration file"
	after "if (fGaveMerge && %ypi == %n) {%n = \"\";}"
}

%hi
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
%%
%i
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
%%

init 4 "HostStart(%K<util_fsearch_cvt>1qv);"

%c
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
%%
