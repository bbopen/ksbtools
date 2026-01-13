/* $Id: emit.c,v 8.21 1999/06/15 19:58:16 ksb Exp $
 * Emit some of the character to the output C program, all the
 * output to the C file should be in here.
 */
#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <ctype.h>

#include "machine.h"
#include "main.h"
#include "type.h"
#include "option.h"
#include "scan.h"
#include "parser.h"
#include "list.h"
#include "mkcmd.h"
#include "key.h"
#include "routine.h"
#include "emit.h"
#include "check.h"
#include "atoc.h"

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


ATTR
	wEmitMask = EMIT_AUTO_IND;	/* implicit emit options	*/
char
	acNoOpt[] = "\'\\000\'",
	acCurOpt[] = "u_curopt",
	acEnvTemp[] = "u_pcEnv",
	*pcCurOpt = acNoOpt,
	*pcJustEscape = nil;

static char
	*pcOptList = "sbOpt",
	acIndent[] = "\t\t\t\t\t\t\t\t\t                ",
	acVersion[32];
static struct tm
	tmNow;				/* interface to strftime	*/


/* make a spelling for a character that C will like			(ksb)
 * and the human can read
 */
char *
CSpell(pcOut, c, fQuote)
char *pcOut;
int c, fQuote;
{
	register char *pcRet;

	pcRet = pcOut;
	if (EOF == c) {
		sprintf(pcOut, "EOF");
		return pcOut;
	}
	if (fQuote) {
		*pcOut++ = '\'';
	}
	if (! isgraph(c)) {
		pcOut[0] = '\\';
		pcOut[2] = '\000';
		switch (c) {
		case ' ': /* don't do this to spaces */
			pcOut[0] = ' ';
			pcOut[1] = '\000';
			break;
		case '\b':
			pcOut[1] = 'b';
			break;
		case '\r':
			pcOut[1] = 'r';
			break;
		case '\f':
			pcOut[1] = 'f';
			break;
		case '\n':
			pcOut[1] = 'n';
			break;
		case '\t':
			pcOut[1] = 't';
			break;
		default:
			sprintf(pcOut+1, "%03o", c);
			break;
		}
	} else if ('\\' == c || '\'' == c || '\"' == c) {
		sprintf(pcOut, "\\%c", c);
	} else {
		sprintf(pcOut, "%c", c);
	}
	if (fQuote) {
		(void)strcat(pcOut, "\'");
	}
	return pcRet;
}

/* make the default name for an option/action/special			(ksb)
 */
char *
mkdefid(pOR)
OPTION *pOR;
{
	static char acTemp[16];

	if (ISVARIABLE(pOR)) {
		return pOR->pchname;
	}
	switch (pOR->chname) {
	case '?':
		return "ch";
	case '*':
		return "arg";
	case ':':
		return "def";
	case '#':
		return "number";
	case '+':
		return "escape";
	default:
		if (ISSACT(pOR)) {
			return sactstr(pOR->chname);
		}
		if (isdigit(pOR->chname)) {
			acTemp[0] = 'd';
			acTemp[1] = pOR->chname;
			acTemp[2] = '\000';
		} else if (isalnum(pOR->chname)) {
			acTemp[0] = pOR->chname;
			acTemp[1] = '\000';
		} else {
			sprintf(acTemp, "x%x", pOR->chname);
		}
		break;
	}
	return acTemp;
}

/* a option -> C identifier						(ksb)
 * (that is, convert it to a string the user can understand better)
 */
char *
mkid(pOR, pcDst)
OPTION *pOR;
char *pcDst;
{
	register char *pch;
	static char acFallback[32];

	if (nil == pcDst) {
		pcDst = acFallback;
	}
	while (ISALIAS(pOR)) {
		pOR = pOR->pORali;
	}
	if (nil == (pch = pOR->pchname)) {
		register char *pcFrom, *pcPlate;
		if (CHDECL == pOR->chname) {
			fprintf(stderr, "%s: globals must have names\n", progname);
			iExit |= MER_SEMANTIC;
		}
		pcFrom = OPACTION(pOR) ? pchProto : pchTempl;
		pcPlate = OPACTION(pOR) ? "prototype" : "template";
		(void)sprintf(pcDst, pcFrom, mkdefid(pOR));
		if ('\000' == pcDst[0]) {
			fprintf(stderr, "%s: %s: %s \"%s\" produced the empty string\n", progname, usersees(pOR, nil), pcPlate, pcFrom);
			iExit |= MER_SEMANTIC;
			(void)strcpy(pcDst, pcPlate);
		}
	} else if ('\000' == pch[0]) {
		fprintf(stderr, "%s: %s: identifiers may not be the empty string\n", progname, usersees(pOR, nil));
		iExit |= MER_SEMANTIC;
		(void)strcpy(pcDst, "<empty>");
	} else {
		(void)strcpy(pcDst, pch);
	}
	return pcDst;
}

static EMIT_MAP aERTop[] = {
	{'a', "argument", "special contexts argument"},
	{'A', "getarg", "a C expression to get the next argument"},
	{'b', "basename", "the basename of the running application"},
	{'B', "keyword",  "the keyword found after a key basename"},
	{'c', "comment",  "the comment continuation for block comments"},
	{'D', "value",	  "expand WRT the following string as a value..."},
	{'e', "expand",   (char *)0},
	{'E', "strerror", "C expression producing a string for errno"},
	{'f', "main",     "the name of the function we are building"},
	{'F', "output",   "-n command line parameter"},
	{'G', "argkey",	  "use special argument as a key name..."},
	{'H', "hold",	  "change the value of %a"},
	/* 'I' is available for interface modules (output.c) */
	{'J', "pctemp",   "temporary character pointer variable"},
	{'k', "mkdelim",  "mk's marker character"},
	{'K', "key",	  "expand API key values..."},
	{'o', "optopt",   "getopt's record of the option presented"},
	{'O', "getopt",   "C expression to call getopt for next option"},
	{'P', "altparam", "alternate parameter in special contexts"},
	{'q', "quote",    "output the expansion of this quoted for C strings"},
	{'Q', "megaquote", "output remainder of line quoted for C strings"},
	{'s', "broken",   "if we see this we believe unquoted a printf-percent"},
	{'t', "terse",    "global terse usage variable, prefix with an 'a' for array of all of them"},
	{'T', "update",   "(depricated) conversion for option"},
	{'u', "usage",    "global usage function name"},
	{'v', "vector",   "global usage vector name (array of strings)"},
	{'V', "version",  "mkcmd's version identifier"},
	{'w', "which",    "C expression for the alias that caused this conversion"},
	{'W', "mkcmd",    "what name was mkcmd called?"},
	{'X', "expand",   "force re-expansion of this result"},
	{'y', "type",     "lookup a type by name..."},
	{'Y', "time",     "call strftime on time of compilation..."},
	{'z', "error",    "force an internal error"},
	{'Z', "option",   "force next down to option level"},
	{':', "optlist",  "name of local C variable holding getopt parameter"},
	{'{', "lcurly",   "open curly brace with no C formatting"},
	{'}', "rcurly",   "close curly brace with no C formatting"},
	{';', "semicolon", "semicolon with no C formatting"},
	{'\n', "newline", (char *)0},
	{'+', "newline",  "a literal newline character"},
	{'\\', "escape",  "the 4th parameter to getopt, based on the escape trigger"},
	{'#', "remainder", "a C expression for the number of parameters remaining"},
	{'@', "params",    "a C expression for the vector of parameters remaining"},
	{'%', "percent",  "a percent sign"},
};

EMIT_HELP EHTop = {
	"top level expander", (char *)0,
	sizeof(aERTop)/sizeof(EMIT_MAP),
	aERTop,
	(char *)0,
	(EMIT_HELP *)0
};

/* provide clues about the expander described, to the user		(ksb)
 * OK.  Complex, but not too strange.  Some escapes are covered
 * by the level above (OptEsc A covered by TopLevel A).  In the
 * help output we should mark the blocked ones with a "*" and footnote
 * that they are blocked from above.  Some escape in the level above
 * should force expansion to the level below (%Z, %<option> in TopLevel).
 */
static void
EmitHelpFmt(fp, pEH)
FILE *fp;
EMIT_HELP *pEH;
{
	register int i, iWidth, iCWidth, j, iBlocked;
	register EMIT_MAP *pER, *pERScan;
	auto char acChar[5]; /* \134, \n, x */
	register char *pcBlock;

	if ((EMIT_HELP *)0 == pEH) {
		fprintf(fp, "%s: no help\n", progname);
		return;
	}

	if ((EMIT_HELP *)0 != pEH->pEHcovers) {
		pcBlock = calloc(pEH->uhas, sizeof(char));
	} else {
		pcBlock = (char *)0;
	}
	/* find widths and block stats
	 */
	iBlocked = 0;
	iWidth = 0;
	iCWidth = 1;
	for (pER = pEH->pER, i = 0; i < pEH->uhas; ++i, ++pER) {
		register int iLen;
		if (ER_HIDDEN(pER))
			continue;
		(void)CSpell(acChar, pER->cekey, 0);
		if (iCWidth < (iLen = strlen(acChar)))
			iCWidth = iLen;
		if (iWidth < (iLen = strlen(pER->pcword)))
			iWidth = iLen;
		if ((char *)0 == pcBlock)
			continue;
		for (pERScan = pEH->pEHcovers->pER, j = 0; j < pEH->pEHcovers->uhas; ++j, ++pERScan) {
			if (pERScan->cekey == pER->cekey)
				break;
			if (0 == strcmp(pERScan->pcword, pER->pcword))
				break;
		}
		if (' '!= (pcBlock[i] = j == pEH->pEHcovers->uhas ? ' ' : '*'))
			++iBlocked;
	}
	if (EH_NUMBER(pEH)) {
		while (iCWidth + iWidth < 2) {
			++iWidth;
		}
	}
	if (0 == iBlocked) {
		/* we can loose memory here, but we exit soon */
		pcBlock = (char *)0;
	}

	/* output the table requested
	 */
	fprintf(fp, "%s: %s", progname, pEH->pclevel);
	if ((char *)0 != pcBlock) {
		fprintf(fp, " (%d expansion%s blocked by %s)", iBlocked, 1 == iBlocked ? "" : "s", pEH->pEHcovers->pclevel);
	}
	fprintf(fp, "\n");
	if (EH_NUMBER(pEH)) {
		fprintf(fp, "%s%-*s %s\n", (char *)0 == pcBlock ? "" : "  ", iCWidth+iWidth+1, "0-9", pEH->pcnumber);
	}
	for (pER = pEH->pER, i = 0; i < pEH->uhas; ++i, ++pER) {
		if (ER_HIDDEN(pER))
			continue;
		(void)CSpell(acChar, pER->cekey, 0);
		if ((char *)0 != pcBlock) {
			fprintf(fp, "%c ", pcBlock[i]);
		}
		fprintf(fp, "%*s %-*s %s\n", iCWidth, acChar, iWidth, pER->pcword, pER->pcdescr);
	}
}

typedef struct ELnode {
	char *pctoken;
	char *pctext;
} EMIT_LIST;
static EMIT_LIST aELMeta[] = {
	{"abort message", "fail to integrate with message"},
	{"if conditional", "conditionally include code"},
	{"elif conditional", "else-if clause"},
	{"else", "alternate default text"},
	{"endif", "close an if block"},
	{"eval text", "expand text again"},
	{"foreach keep", "expand code once for each value in keep (uses %a)"},
	{"endfor", "close a for block"},
	{"expand files", "interpolate files under present context"},
	{(char *)0, (char *)0 }
};
static EMIT_LIST aELExpr[] = {
	{"value", "a C identifier or quoted string"},
	{"key[integer]", "select a value from the key (which must exist)"},
	{"e in key", "true when the left side is contained the key"},
	{"e == key[integer]", "true when the left side compares exactly equal to the key at the given slot"},
	{"e != key[integer]", "false when the above it true"},
	{"key is integer", "true when key has this integer version"},
	{"e && e", "both are true"},
	{"e || e", "either is true"},
	{"(e)", "change binding of operators"},
	{(char *)0, (char *)0}
};

/* explain the emit conditionals or expander control logic		(ksb)
 */
static void
EmitMetaHelp(fp, pEL, pcTag)
FILE *fp;
EMIT_LIST *pEL;
char *pcTag;
{
	register int iWidth, i;

	if ((char *)0 == pcTag) {
		pcTag = "";
	}
	iWidth = 1;
	for (i = 0; (char *)0 != pEL[i].pctoken; ++i) {
		register int iLen;
		if (iWidth < (iLen = strlen(pEL[i].pctoken))) {
			iWidth = iLen+1;
		}
	}
	for (i = 0; (char *)0 != pEL[i].pctoken; ++i) {
		fprintf(fp, "%s%-*s %s\n", pcTag, iWidth, pEL[i].pctoken, pEL[i].pctext);
	}
}

/* output the given emit help info, or a table of what we have		(ksb)
 * I usually would not build a "one off" data structure for this
 * kinda thing.  I could not find a need for any other function
 * to see/share this data. -- ksb
 */
void
EmitHelp(fp, pcName)
FILE *fp;
char *pcName;
{
	static char acMetaHelp[] = "meta", acMetaConds[] = "conditional";
	static struct EInode {
		char *pccalled;
		EMIT_HELP *pHE;
	} aEIDex[] = {
		{ "top",	& EHTop},
		{ "base",	& EHBase},
		{ "control",	& EHSact},
		{ "key",	& EHKey},
		{ "option",	& EHOpt},
		{ "routine",	& EHRoutine},
		{ "type",	& EHType},
		{ "value",	& EHValue},
		{ acMetaHelp,	(EMIT_HELP *)0},	/* other style */
		{ acMetaConds,	(EMIT_HELP *)0},
		{ nil,		(EMIT_HELP *)0}
	};
	register struct EInode *pEIScan;
	register int iWidth;

	if ((char *)0 == pcName || '\000' == pcName[0]) {
		pcName = "?";	/* a string not in the table */
	}

	/* lookup pcName or output aEIDex table
	 */
	for (pEIScan = aEIDex; (char *)0 != pEIScan->pccalled; ++pEIScan) {
		if (0 == strcmp(pcName, pEIScan->pccalled))
			break;
	}
	if ((EMIT_HELP *)0 != pEIScan->pHE) {
		EmitHelpFmt(fp, pEIScan->pHE);
		return;
	}
	/* the klunky ones, but needed
	 */
	if (acMetaConds == pEIScan->pccalled) {
		EmitMetaHelp(fp, aELExpr, (char *)0);
		return;
	}
	if (acMetaHelp == pEIScan->pccalled) {
		EmitMetaHelp(fp, aELMeta, "%@");
		return;
	}

	/* find col1 width
	 */
	iWidth = 1;
	for (pEIScan = aEIDex; (char *)0 != pEIScan->pccalled; ++pEIScan) {
		register int iLen;
		iLen = strlen(pEIScan->pccalled);
		if (iLen > iWidth)
			iWidth = iLen;
	}

	/* clue the user in on the tables we can show
	 */
	fprintf(fp, "%s: available expanders are:\n", progname);
	for (pEIScan = aEIDex; (char *)0 != pEIScan->pccalled; ++pEIScan) {
		fprintf(fp, " %-*.*s ", iWidth, iWidth, pEIScan->pccalled);
		if (0 == pEIScan->pHE) {
			fprintf(fp, "output syntax clues\n");
			continue;
		}
		fprintf(fp, "%s", pEIScan->pHE->pclevel);
		if ((char *)0 != pEIScan->pHE->pchow) {
			fprintf(fp, " (%s)", pEIScan->pHE->pchow);
		}
		fprintf(fp, "\n");
	}
}

/* balance character computer						(ksb)
 * map '(' to ')' and such,
 * map '/' to '/' for R.E. style quoting
 */
static int
MapBal(cOpen)
int cOpen;
{
	register char *pcFnd;
	static char
		acOpen[] = "([<`",
		acClose[]= ")]>'";

	if ((char *)0 != (pcFnd = strchr(acOpen, cOpen))) {
		return acClose[pcFnd - acOpen];
	}
	return -1;
}

/* find a percent escape or a longer name under the cursor		(ksb)
 * advance the cursor and return the escape (or number)
 * numbers are 0, BTW.
 * N.B. only jump the cursor by one on fail, whole escape on success
 * -1 == bad single char (*piNumber = character)
 * -2 == bad word (*piNumber = length of bad word)
 * (For keys we return the entry that matched index into the map.)
 */
int
EscCvt(pEH, ppcCursor, piNumber)
EMIT_HELP *pEH;
char **ppcCursor;
int *piNumber;
{
	register int i, cCmd;
	register EMIT_MAP *pER;
	register int cScan;
	register char *pcClose;

	pEH->ufound = pEH->uhas;	/* off end, didn't find it */
	if ('\000' == (*ppcCursor)[0]) {
		if ((int *)0 != piNumber) {
			*piNumber = '\000';
		}
		return -1;
	}
	if ((int *)0 != piNumber && isdigit((*ppcCursor)[0])) {
		i = 0;
		do {
			i *= 10;
			i += (*ppcCursor)++[0] - '0';
		} while (isdigit(ppcCursor[0][0]));
		*piNumber = i;
		return 0;
	} else if ((int *)0 != piNumber) {
		/* `coded bad' is an internal debug aid -- ksb
		 */
		*piNumber = 0xc0dedbad;
	}
	cCmd = (*ppcCursor)++[0];
	for (pER = pEH->pER, i = 0; i < pEH->uhas; ++i, ++pER) {
		if (cCmd == pER->cekey) {
			pEH->ufound = i;
			return cCmd;
		}
	}
	if (-1 != (cScan = MapBal(cCmd)) && (char *)0 != (pcClose = strchr(*ppcCursor, cScan))) {
		register int iLen;
		register char *pcWord;
		register EMIT_MAP *pERBest;

		pcWord = *ppcCursor;
		iLen = pcClose - pcWord;
		/* find escape "%c%.*s%c" , cCmd, iLen, pcWord, cScan
		 */
		pERBest = (EMIT_MAP *)0;
		for (pER = pEH->pER, i = 0; i < pEH->uhas; ++i, ++pER) {
			if (0 != strncmp(pER->pcword, pcWord, iLen)) {
				continue;
			}
			if ((EMIT_MAP *)0 == pERBest || '\000' == pER->pcword[iLen]) {

				pEH->ufound = i;
				pERBest = pER;
			}
		}
		if ((EMIT_MAP *)0 != pERBest) {
			*ppcCursor = pcWord+iLen+1;
			return pERBest->cekey;
		}
		if ((int *)0 != piNumber) {
			*piNumber = iLen;
		}
		return -2;
	}
	if ((int *)0 != piNumber) {
		*piNumber = cCmd;
	}
	return -1;
}

/* this part of emit is recursive, we filter at the top			(ksb)
 * level and recurse this part only
 */
static void
Remit(pcState, pOR, pEO)
char *pcState;
OPTION *pOR;
EMIT_OPTS *pEO;
{
	register int c;
	auto char acExp[4096];

	while ('\000' != *pcState) {
		if (EA_AUTO_IND(pEO) && !EA_DID_TABS(pEO) && '\n' != *pcState) {
			register int iTemp;

			iTemp = pEO->iindent;
			if (RGROUP == *pcState) {
				--iTemp;
			}
			fprintf(pEO->fp, "%.*s", iTemp, acIndent);
			pEO->weattr |= EMIT_DID_TABS;
		}
		switch (c = *pcState) {
		case LGROUP:
			++pEO->iindent;
			putc(c, pEO->fp);
			if (EA_AUTO_IND(pEO)) {
				fputc('\n', pEO->fp);
				pEO->weattr &= ~EMIT_DID_TABS;
			}
			break;
		case RGROUP:
			if (pEO->iindent > 0)
				--pEO->iindent;
			/*fallthrough*/
		case ';':
			putc(c, pEO->fp);
			if (EA_AUTO_IND(pEO)) {
				fputc('\n', pEO->fp);
				pEO->weattr &= ~EMIT_DID_TABS;
			}
			break;
		case '\n':
			/* If we in C QUOTES:
			 *	we had to be passed the indent string-- yucko
			 */
			if (0 != EA_QUOTE(pEO)) {
				fprintf(pEO->fp, "\\n");
			}
			fputc('\n', pEO->fp);
			pEO->weattr &= ~EMIT_DID_TABS;
			break;
		case '%':
			if ('\000' == *++pcState) {
				fprintf(stderr, "%s: incomplete escape sequence\n", progname);
				iExit |= MER_SYNTAX;
				continue;
			}
			if (0 != TopEsc(& pcState, pOR, pEO, acExp)) {
				fprintf(stderr, "%s: failed to expand\n", progname);
				iExit |= MER_BROKEN;
				break;
			}
			if (EA_EXPAND(pEO)) {
				pEO->weattr &= ~EMIT_EXPAND;
				Remit(acExp, pOR, pEO);
			} else if (0 != EA_QUOTE(pEO)) {
				atoc(pEO->fp, acExp, "", "", TTOC_BLOCK, 509);
			} else {
				fputs(acExp, pEO->fp);
			}
			continue;
		case '\\':
		case '\"':
			if (0 != EA_QUOTE(pEO)) {
				fputc('\\', pEO->fp);
			}
			/*fallthrough*/
		default:
			putc(c, pEO->fp);
			break;
		}
		++pcState;
	}
}

/* convert a top level escape sequence					(ksb)
 * return the string to the expander, it will out it before it
 * calls us again, or string save it (if it is re-expanding the results)
 * (default to the option level, if we can't figure out what to do...)
 */
int
TopEsc(ppcCursor, pOR, pEO, pcDst)
char **ppcCursor, *pcDst;
OPTION *pOR;
EMIT_OPTS *pEO;
{
	register int cCmd;
	register int iRet;
	register char *pcCopy;
	register OPTTYPE *pOT;
	register char *pcBack;
	register KEY *pKV;
	auto char acStuff[32];
	auto int iBad;
	auto char *pcFake;

	pcBack = *ppcCursor;
	for (;;) { switch (cCmd = EscCvt(& EHTop, ppcCursor, & iBad)) {
	case 'a':
		(void)strcpy(pcDst, nil == pEO->pcarg1 ? "argc" : pEO->pcarg1);
		break;
	case 'A':
		/* if we have the function call it, else generate
		 * inline code and hope lint doesn't barf too much
		 */
		if (fMix || fPSawEvery)
			pcFake = "%K<getarg>/ef";
		else
			pcFake = "((argc <= %K<getopt_switches>1ev) ? ((%K<getopt_switches>2ev = (char *) 0), EOF) : ((%K<getopt_switches>2ev = argv[%K<getopt_switches>1ev++]), 0))";
		pcFake = StrExpand(pcFake, pOR, pEO, 2048);
		if ((char *)0 == pcFake) {
			iExit |= MER_SEMANTIC;
			break;
		}
		(void)strcpy(pcDst, pcFake);
		free(pcFake);
		break;
	case 'B':	/* keyword -- fall back to progname */
		/* ZZZ this is not really available until after init's */
		if ((char *)0 == pcKeyArg) {
			fprintf(stderr, "%s: %%B: no \"key basename\" defined\n", progname);
			iExit |= MER_SEMANTIC;
		} else {
			(void)strcpy(pcDst, acFinger);
			break;
		}
		/* fallthrough */
	case 'b':	/* running binary name */
		if ('\000' == pchProgName) {
			fprintf(stderr, "%s: %%b: no \"named\" variable defined\n", progname);
			iExit |= MER_SEMANTIC;
			(void)strcpy(pcDst, "\"\"");
		} else {
			(void)strcpy(pcDst, pchProgName);
		}
		break;
	case 'c':	/* comment leader */
		(void)strcpy(pcDst, " * ");
		break;
	case 'D':	/* this gets rid of 1 item bogus keys */
		/* %D/tar/p  would be best use
		 * %D,string,e is a less obvious use (as parens kinda)
		 */
		pcFake = *ppcCursor;
		pcCopy = pcFake+1;
		iBad = 0;
		while ('\000' != *pcCopy && *pcFake != *pcCopy) {
			++iBad, ++pcCopy;
		}
		pcCopy = malloc(((iBad)|7)+1);
		if ((char *)0 == pcCopy) {
			fprintf(stderr, acOutMem, progname);
			exit(MER_LIMIT);
		}
		(void)strncpy(pcCopy, ++pcFake, iBad);
		pcCopy[iBad] = '\000';
		pcFake += iBad+1;
		while ('\000' != *pcFake && 'v' != *pcFake && 'N' != *pcFake) {
			pcCopy = ValueEsc(& pcFake, pcCopy, "top level", 0, pOR, pEO);
		}
		if ((char *)0 != pcCopy) {
			(void)strcpy(pcDst, pcCopy);
			/* free(pcCopy)  might be acStop! don't free it*/
		}
		pcFake += '\000' != *pcFake;
		*ppcCursor = pcFake;
		break;
	case 'E':
#if HAVE_STRERROR
		(void)strcpy(pcDst, "strerror(errno)");
#else
		if (fWeGuess) {
			(void)strcpy(pcDst, "sys_errlist[errno]");
		} else {
			(void)strcpy(pcDst, "strerror(errno)");
		}
#endif
		break;
	case 'f':
		(void)strcpy(pcDst, pchMain);
		break;
	case 'F':	/* basename of output file set (-n param) */
		(void)strcpy(pcDst, pchBaseName);
		break;
	case 'G':	/* convert %a to a key and call KeyEsc with it */
		{
		auto char *pcTemp;
		if ((char *)0 == (pcTemp = pEO->pcarg1)) {
			fprintf(stderr, "%s: %%a: no special context to find key name\n", progname);
			return 1;
		}
		if ((KEY *)0 == (pKV = KeyParse(& pcTemp))) {
			fprintf(stderr, "%s: %%a: %s is not a key name\n", progname, pEO->pcarg1);
			return 1;
		}
		return KeyEsc(ppcCursor, pOR, pKV, pEO, pcDst);
		}
	case 'H':	/* move %a -> %P, move string to %a */
		pcFake = *ppcCursor;
		pcCopy = pcFake+1;
		iBad = 0;
		while ('\000' != *pcCopy && *pcFake != *pcCopy) {
			++iBad, ++pcCopy;
		}
		pcCopy = malloc(((iBad)|7)+1);
		if ((char *)0 == pcCopy) {
			fprintf(stderr, acOutMem, progname);
			exit(MER_LIMIT);
		}
		(void)strncpy(pcCopy, ++pcFake, iBad);
		pcCopy[iBad] = '\000';
		pcFake += iBad+1;
		*ppcCursor = pcFake;
		pcFake = pEO->pcarg2;
		pEO->pcarg2 = pEO->pcarg1;
		pEO->pcarg1 = pcCopy;
		iRet = TopEsc(ppcCursor, pOR, pEO, pcDst);
		pEO->pcarg1 = pEO->pcarg2;
		pEO->pcarg2 = pcFake;
		return iRet;
	/* case 'I': */
	case 'J':	/* temp char * we use in mail */
		strcpy(pcDst, acEnvTemp);
		break;
	case 'k':	/* mk's marker character (good for ksb) */
		*pcDst++ = '$';
		*pcDst = '\000';
		break;
	case 'K':
		if ((KEY *)0 == (pKV = KeyParse(ppcCursor))) {
			return 1;
		}
		return KeyEsc(ppcCursor, pOR, pKV, pEO, pcDst);
	case 'o':
		pcFake = "K<getopt_switches>3ev";
		iRet = TopEsc(& pcFake, pOR, pEO, pcDst);
		break;
	case '\\':
		if (nilOR != (pOR = newopt('+', & pORRoot, 0, 0))) {
			sprintf(pcDst, "\"%s\"", pOR->pchgen);
		} else {
			sprintf(pcDst, "(char *)0");
		}
		break;
	case 'O':
		pcFake = "K<getopt>/ef";
		iRet = TopEsc(& pcFake, pOR, pEO, pcDst);
		break;
	case 'P':
		(void)strcpy(pcDst, nil == pEO->pcarg2 ? "argv" : pEO->pcarg2);
		break;
	case 'q':
		pEO->weattr ^= EMIT_QUOTE;
		iRet = TopEsc(ppcCursor, pOR, pEO, pcDst);
		pEO->weattr ^= EMIT_QUOTE;
		return iRet;
	case 'Q':
		pEO->weattr ^= EMIT_QUOTE;
		break;
	case 's':
		fprintf(stderr, "%s: `%%s\' mostly means the user code is not quoted correctly (optsil not supported)\n", progname);
		strcpy(pcDst, "%s");		/* was optsil */
		break;
	case 't':
		if (nil == pchTerse) {
			fprintf(stderr, "%s: terse usage message used but never defined\n", progname);
			exit(MER_SEMANTIC);
		}
		(void)strcpy(pcDst, pchTerse);
		break;
	case 'T':	/* should be %Xxu in new context */
		pcFake = "XxSu";
		iRet = TopEsc(& pcFake, pOR, pEO, pcDst);
		break;
	case 'u':
		if (nil == pchUsage) {
			fprintf(stderr, "%s: usage function used but never defined\n", progname);
			exit(MER_SEMANTIC);
		}
		(void)strcpy(pcDst, pchUsage);
		break;
	case 'v':
		if (nil == pchVector) {
			fprintf(stderr, "%s: verbose help vector used but never defined\n", progname);
			exit(MER_SEMANTIC);
		}
		(void)strcpy(pcDst, pchVector);
		break;
	case 'V':
		(void)strcpy(pcDst, acVersion);
		break;
	case 'w':
		(void)strcpy(pcDst, pcCurOpt);
		break;
	case 'W':
		(void)strcpy(pcDst, progname);
		break;
	case 'e':	/* re-expand -- old name form 7.4 release */
	case 'X':	/* re-user-call like %Xxd */
		if (0 != (iRet = TopEsc(ppcCursor, pOR, pEO, pcDst))) {
			return iRet;
		}
		if (nil == (pcCopy = malloc((strlen(pcDst)|7)+1))) {
			fprintf(stderr, acOutMem, progname);
			exit(MER_LIMIT);
		}
		(void)strcpy(pcCopy, pcDst);
		pcFake = pcCopy;
		while ('\000' != *pcFake) {
			if ('%' != (*pcDst++ = *pcFake++))
				continue;
			--pcDst;
			iRet = TopEsc(& pcFake, pOR, pEO, pcDst);
			if (0 != iRet) {
				break;
			}
			pcDst += strlen(pcDst);
		}
		*pcDst = '\000';
		free((void *)pcCopy);
		return iRet;
	case 'y':	/* type by name */
		if ((OPTTYPE *)0 == (pOT = TypeParse(ppcCursor))) {
			return 1;
		}
		return TypeEsc(ppcCursor, pOT, pEO, pcDst);
	case 'Y':	/* thYme - leverage the whole strftime expander */
		pcCopy = acStuff;
		*pcCopy++ = '%';
		/* also allow %Y[${WIDTH}[.${PREC}]]b from AIX & HPUX strftime
		 */
		if ('-' == *(ppcCursor[0])) {
			*pcCopy++ = *(ppcCursor[0])++;
		}
		do {
			cCmd = *pcCopy++ = *(ppcCursor[0])++;
		} while (isdigit(cCmd) || '.' == cCmd);
		*pcCopy++ = '\000';
		(void)strftime(pcDst, 64, acStuff, & tmNow);
		break;
	case ':':
		(void)strcpy(pcDst, pcOptList);
		break;
	case '@':
	case '#':
		(void)strcpy(pcDst, '#' == cCmd ? "(argc-" : "(argv+");
		pcDst += strlen(pcDst);
		pcFake = "K<getopt_switches>1ev";
		iRet = TopEsc(& pcFake, pOR, pEO, pcDst);
		pcDst += strlen(pcDst);
		*pcDst++ = ')';
		*pcDst = '\000';
		break;
	case '+':
		cCmd = '\n';
		/* fallthrough */
	case ';':
	case '{':
	case '}':
	case '%':
	case '\n':
		*pcDst++ = cCmd;
		*pcDst = '\000';
		break;
	default:
	case 'z':
		(void)CSpell(pcDst, cCmd, 0);
		fprintf(stderr, "%s: top level: %%%s: expand error\n", progname, pcDst);
		iExit |= MER_PAIN;
		return 1;

	case  0: /* let the option level have the number	*/
	case -2: /* not a top level, try the option level	*/
	case -1: /* same as -2					*/
		*ppcCursor = pcBack;
		/*fallthough*/
	case 'Z':	/* force into hidden options onlys */
		return OptEsc(ppcCursor, pOR, pEO, pcDst);
	} break; }
	return 0;
}

/* expand a string in malloc'd memory, compact allocation		(ksb)
 */
char *
StrExpand(pcDo, pOR, pEO, uLen)
char *pcDo;
OPTION *pOR;
EMIT_OPTS *pEO;
unsigned uLen;
{
	auto EMIT_OPTS EOLevel;
	register char *pcMem, *pcCopy;
	unsigned int uMin;

	if (uLen < (uMin = strlen(pcDo)+1)) {
		uLen = (uMin|511)+1;
	}
	pcCopy = pcMem = malloc(uLen);
	if ((char *)0 == pcMem) {
		fprintf(stderr, acOutMem, progname);
		exit(MER_LIMIT);
	}
	if ((EMIT_OPTS *)0 == pEO) {
		EOLevel.weattr = wEmitMask & ~EMIT_AUTO_IND;
		EOLevel.iindent = 0;
		EOLevel.pcarg1 = nil;
		EOLevel.pcarg2 = nil;
		EOLevel.fp = stderr;	/* LLL */
		pEO = & EOLevel;
	}
	while ('\000' != *pcDo) {
		if ('%' != *pcDo) {
			*pcCopy++ = *pcDo++;
			continue;
		}
		++pcDo;
		if (0 == TopEsc(& pcDo, pOR, pEO, pcCopy)) {
			pcCopy += strlen(pcCopy);
			continue;
		}
		return (char *)0;
	}
	*pcCopy = '\000';

	return (char *)realloc(pcMem, (strlen(pcMem)|7)+1);
}

static char *pcArg2 = nil;

/* Emit a call to a user specified C code				(ksb)
 * iIndent is the number of tabs the C code is indented to, modified
 * by "{" and "}" -- not really by C syntax (for example "if(1) foo();"
 * doesn't [put in a newline or] indent the "foo();").
 */
void
Emit(fp, pcState, pOR, pcArg, iIndent)
register FILE *fp;
OPTION *pOR;
char *pcState, *pcArg;
int iIndent;
{
	auto EMIT_OPTS EOLevel;

	if (nil == pcState) {		/* no call			*/
		return;
	}
#if 0
	/* do not insert blank lines as "statements"
	 * N.B. "%\n" will force it.
	 */
	if ('\n' == pcState[0] && '\000' == pcState[1]) {
		return;
	}
#endif
	EOLevel.weattr = wEmitMask;
	if (iIndent < 0) {
		fprintf(stderr, "%s: %s: emit fell off the left margin\n", progname, usersees(pOR, nil));
		EOLevel.weattr &= ~EMIT_AUTO_IND;
		EOLevel.iindent = -iIndent;
	} else {
		EOLevel.iindent = iIndent;
	}

	EOLevel.pcarg1 = pcArg;
	EOLevel.pcarg2 = pcArg2;
	EOLevel.fp = fp;
	Remit(pcState, pOR, & EOLevel);
}

/* when we have to have 2 variable strings (%a and %P) we call this	(ksb)
 */
void
Emit2(fp, pcState, pOR, pcArg, pcA2, iIndent)
register FILE *fp;
OPTION *pOR;
char *pcState, *pcArg, *pcA2;
int iIndent;
{
	auto char *pcKeep;

	pcKeep = pcArg2;
	pcArg2 = pcA2;
	Emit(fp, pcState, pOR, pcArg, iIndent);
	pcArg2 = pcKeep;
}

/* when we want an integer as %a we call EmitN()			(ksb)
 */
void
EmitN(fp, pcState, pOR, iArg, iIndent)
register FILE *fp;
OPTION *pOR;
char *pcState;
int iArg, iIndent;
{
	auto char acCvt[32];

	(void)sprintf(acCvt, "%d", iArg);
	Emit(fp, pcState, pOR, acCvt, iIndent);
}

/* trim the rcs version string to something I like			(ksb)
 *  $Id: emit.c,v 7.4 94/06/01 11:13:09 ksb Alpha Locker: ksb
 *	       ^ fixed points	 ^->   ^-> ^
 * becomes
 *  7.4 Alpha
 * the `Exp' State is compressed out
 */
void
EmitInit(pcId)
char *pcId;
{
	register char *pcEnd, *pc;
	register int iVerLen, iStateLen;
	register char *pcVersion, *pcState;
	auto time_t tNow;

	time(& tNow);
	tmNow = *localtime(& tNow);

	if ((char *)0 == pcId) {
		(void)strcpy(acVersion, "unknown");
		return;
	}
	if ((char *)0 == (pcVersion = strchr(pcId, ',')) || 'v' != *++pcVersion) {
		strcpy(acVersion, "unparsable");
		return;
	}

	do {
		++pcVersion;
	} while (isspace(*pcVersion));
	pcEnd = pcVersion;
	while ('\000' != *pcEnd && !isspace(*pcEnd))
		++pcEnd;

	if (sizeof(acVersion) < (iVerLen = pcEnd - pcVersion))
		iVerLen = sizeof(acVersion);

	if (nil != (pc = strchr(pcEnd, ':')) &&
	    nil != (pc = strchr(pc+1, ' ')) &&
	    nil != (pc = strchr(pc+1, ' '))) {
		pcState = pc+1;
		if (nil != (pcEnd = strchr(pcState, ' '))) {
			iStateLen = (pcEnd - pcState);
		} else {
			iStateLen = strlen(pcState);
		}
		if (sizeof(acVersion) < iStateLen + iVerLen + 1) {
			iStateLen = sizeof(acVersion) - 1 - iVerLen;
		}
		if ((3 == iStateLen && 0 == strncmp("Exp", pcState, 3))) {
			(void)sprintf(acVersion, "%.*s", iVerLen, pcVersion);
		} else {
			(void)sprintf(acVersion, "%.*s %.*s", iVerLen, pcVersion, iStateLen, pcState);
		}
	} else {
		(void)sprintf(acVersion, "%.*s", iVerLen, pcVersion);
	}
}

#define MT_ABORT	'a'	/* %@abort message			*/
#define MT_IF		'i'	/* %@if conditional text		*/
#define MT_ELIF		'I'	/* %@elif conditional text		*/
#define MT_ELSE		'e'	/* %@else text				*/
#define MT_ENDIF	'E'	/* %@endif marker			*/
#define MT_EVAL		'X'	/* %@eval line		(double expand)	*/
#define MT_FOREACH	'f'	/* %@foreach keep loop			*/
#define MT_ENDFOR	'n'	/* %@endfor marker			*/
#define MT_EXPAND	'<'	/* %@expand files			*/

/* find a conditional token if there is one, else return EOF		(ksb)
 */
int
MetaToken(pcIn, ppcOut)
char *pcIn, **ppcOut;
{
	static char acAbort[] = "abort",
		acExpand[] = "expand",
		acIf[] = "if",
		acElif[] = "elif",
		acElse[] = "else",
		acEndif[] = "endif",
		acEval[] = "eval",
		acForeach[] = "foreach",
		acEndfor[] = "endfor";
	static struct MTnode {
		char ctoken;
		char *pctext;
		int ulen;
	} aMT[] = {
		{MT_ABORT, acAbort, sizeof(acAbort)-1},
		{MT_ELIF, acElif, sizeof(acElif)-1},
		{MT_ELSE, acElse, sizeof(acElse)-1},
		{MT_ENDIF, acEndif, sizeof(acEndif)-1},
		{MT_EVAL, acEval, sizeof(acEval)-1},
		{MT_IF, acIf, sizeof(acIf)-1},
		{MT_EXPAND, acExpand, sizeof(acExpand)-1},
		{MT_FOREACH, acForeach, sizeof(acForeach)-1},
		{MT_ENDFOR, acEndfor, sizeof(acEndfor)-1}
	};
	register int i;

	for (i = 0; i < sizeof(aMT)/sizeof(aMT[0]); ++i) {
		if (0 != strncmp(aMT[i].pctext, pcIn, aMT[i].ulen) || !isspace(pcIn[aMT[i].ulen]))
			continue;
		if ((char **)0 != ppcOut)
			*ppcOut = pcIn + aMT[i].ulen;
		return aMT[i].ctoken;
	}
	return EOF;
}

/* this code agrees with SynthFix's generation of these strings		(ksb)
 * the tokens look like mkcmd's expander langauge, but are not really.
 */
OPTION *
SynthDecode(pcThis)
char *pcThis;
{
	register OPTION *pORScope;

	if ('%' != *pcThis++) {
		return nilOR;
	}
	pORScope = nilOR;
	while ('\000' != *pcThis) {
		if ('r' == *pcThis) {
			++pcThis;
			pORScope = OptParse(& pcThis, pORScope);
		} else if ('R' == *pcThis) {
			++pcThis;
			pORScope = SactParse(& pcThis, pORScope, (int *)0);
		} else {
			break;
		}
	}
	/* This would only happen if the Implementor gave us a non-empty
	 * init on a client key... what a Joker.  Screw them.
	 */
	if ('\000' != *pcThis) {
		fprintf(stderr, "%s: client key inv. broken (%s)\n", progname, pcThis);
		iExit |= MER_INV;
	}
	return pORScope;
}

typedef struct MPnode {
	char ctype;		/* token type of this level		*/
	char cstatus;		/* looping/done satisfied, else?	*/
	off_t wwhere;		/* lseek value of next line		*/
	unsigned int iline;	/* correct line number for top of loop	*/
	KEY *pKVloop;		/* key we loop for			*/
	LIST_INDEX wloop;	/* number of loop passes left		*/
	char **ppccur;		/* current value for emit		*/
	char *pcmem;
	char *pccntl;		/* control expression in foreach	*/
	struct MPnode
		*pMPprev,	/* pushed status			*/
		*pMPfor;	/* next best for loop			*/
	OPTION *pORwas;		/* old pORScope				*/
} META_P;

#define MS_ECHO		'e'	/* we are echoing text			*/
#define MS_DELETE	'd'	/* we are deleting text			*/
#define MS_FALSE	'f'	/* we are looking for a true condition	*/

/* build a new META_P							(ksb)
 */
static META_P *
NewMP(cTok, iLine, pMPPrev)
char cTok;
int iLine;
META_P *pMPPrev;
{
	register META_P *pMPNew;

	if ((META_P *)0 == (pMPNew = (META_P *)malloc(sizeof(META_P)))) {
		fprintf(stderr, acOutMem, progname);
		exit(MER_LIMIT);
	}
	pMPNew->ctype = cTok;
	pMPNew->cstatus = MS_DELETE;
	pMPNew->iline = iLine;
	pMPNew->wwhere = 0L;
	pMPNew->pMPprev = pMPPrev;
	pMPNew->pORwas = (OPTION *)0;
	return pMPNew;
}

/* the next word in the meta expression					(ksb)
 * We (may) destroy the input string.
 * you'll have to put a EOS at *ppcExpr
 */
static char *
MetaString(ppcExpr)
char **ppcExpr;
{
	register char *pcRet, *pcEnd, *pcMem;
	register char cQuote;

	/* value in quotes, ends as close quote or EOS
	 * we can backqoute a quote in quotes.
	 *
	 * else value ends at EOS, or brackets or spaces
	 */
	pcRet = *ppcExpr;
	if ('\"' == *pcRet || '\'' == *pcRet) {
		cQuote = *pcRet;
		pcMem = pcRet;
		for (pcEnd = pcRet+1; '\000' != *pcEnd && cQuote != *pcEnd; ++pcEnd) {
			if ('\\' == *pcEnd && '\000' != pcEnd[1])
				++pcEnd;
			*pcMem++ = *pcEnd;
		}
		*pcMem = '\000';
		*ppcExpr = pcEnd+(cQuote == *pcEnd);
	} else {
		for (pcEnd = pcRet+1; '\000' != *pcEnd; ++pcEnd) {
			if (!isalnum(*pcEnd) && '_' != *pcEnd && '-' != *pcEnd)
				break;
		}
		*ppcExpr = pcEnd;
	}
	return pcRet;
}

/* we know the next word in the meta expression is a key name		(ksb)
 * we pulled the string with MetaString, now look it up.
 */
static KEY *
MetaKey(pcName, pcNull, fMust)
char *pcName, *pcNull;
int fMust;
{
	register char cKeep;
	register KEY *pKVRet;

	if ((char *)0 == pcNull)
		return KeyFind(pcName, KEY_NOSHORT, 0);

	cKeep = *pcNull;
	*pcNull = '\000';
	pKVRet = KeyFind(pcName, KEY_NOSHORT, 0);
	if ((KEY *)0 == pKVRet) {
		if (fMust) {
			fprintf(stderr, "%s: key %s doesn't exist in this configuration\n", progname, pcName);
			iExit |= MER_SEMANTIC;
		}
		/* a way to be warned if you like -K
		 */
		if (fKeyTable) {
			fprintf(stderr, "%s: ignored undefined key %s\n", progname, pcName);
		}
	}
	*pcNull = cKeep;
	return pKVRet;
}

/* like strtod for ints, eat training spaces				(ksb)
 */
static int
MetaInt(ppcExpr)
char **ppcExpr;
{
	register int iRet;
	register char *pcCache;

	pcCache = *ppcExpr;
	iRet = 0;
	while (isdigit(*pcCache)) {
		iRet *= 10;
		iRet += *pcCache++ - '0';
	}
	while ('\000' != *pcCache && isspace(*pcCache))
		++pcCache;
	*ppcExpr = pcCache;
	return iRet;
}

static int fVisible = 1;	/* can we see the control expressions */

/* return a pointer to the element in the key given			(ksb)
 * syntax: key[which] or value.  Subscripted keys (if given) must exist.
 */
static char *
MetaElement(ppcExpr)
char **ppcExpr;
{
	register char *pcKey, *pcLook;
	register KEY *pKVFrom;
	register LIST_INDEX wCmp;
	register char **ppcList;
	auto char *pcCache;
	auto LIST_INDEX wHas;

	pcCache = *ppcExpr;
	while ('\000' != *pcCache && isspace(*pcCache))
		++pcCache;
	pcKey = MetaString(& pcCache);
	pcLook = pcCache;
	while ('\000' != *pcCache && isspace(*pcCache))
		++pcCache;
	if ('[' != *pcCache) {
		*ppcExpr = pcLook;
		return pcKey;
	}

	do
		++pcCache;
	while ('\000' != *pcCache && isspace(*pcCache));
	wCmp = MetaInt(& pcCache);
	if (']' != *pcCache) {
		fprintf(stderr, "%s: meta expression missing close bracket (])\n", progname);
		iExit |= MER_SYNTAX;
	} else {
		++pcCache;
	}
	*ppcExpr = pcCache;

	if ((KEY *)0 == (pKVFrom = MetaKey(pcKey, pcLook, fVisible))) {
		return (char *)0;
	}

	ppcList = ListCur(& pKVFrom->LIvalues, & wHas);
	return (wCmp >= 1 && wCmp <= wHas) ? ppcList[wCmp-1] : (char *)0;
}

static int MetaOr();

/* evaluate a %@if/elif expression					(ksb)
 * EXPR KEY			does this key exist
 *	KEY[INTEGER]		does this element of this key exist
 *	value in KEY		is the value in the key (in any slot)
 *	value == KEY[INTEGER]	is the value equal (!= works too) to key[slot]
 *	KEY is INTEGER		check the version of the key
 *LLL	KEY has INTEGER		key has at least this many values
 * KEY.prop, KEY[INTEGER], "value" and would all be good things to do.
 */
static int
MetaBase(ppcExpr)
char **ppcExpr;
{
	register KEY *pKVExpr;
	register char *pcKey, *pcValue, *pcNull;
	register int iRet;
	register char **ppcList;
	auto LIST_INDEX wHas;
	auto char *pcCache;

	iRet = MS_FALSE;
	pcCache = *ppcExpr;
	while (isspace(*pcCache))
		++pcCache;
	if ('(' == *pcCache) {
		++pcCache;
		iRet = MetaOr(& pcCache);
		if (')' != *pcCache) {
			fprintf(stderr, "%s: missing close parenthesis in meta expression\n", progname);
			iExit |= MER_SYNTAX;
		}
		*ppcExpr = pcCache + 1;
		return iRet;
	}

	*ppcExpr = pcCache;
	pcValue = MetaString(ppcExpr);
	if (pcCache == *ppcExpr) {
		fprintf(stderr, "%s: meta expansion fails progress check\n", progname);
		exit(MER_INV);
	}
	pcNull = pcCache = *ppcExpr;
	while ('\000' != *pcCache && isspace(*pcCache))
		++pcCache;
	if ('[' == pcCache[0]) {
		/* syntax is: key [_ integer ]
		 */
		register LIST_INDEX wNeed;

		pKVExpr = MetaKey(pcValue, pcNull, fVisible);
		++pcCache;
		while ('\000' != *pcCache && isspace(*pcCache))
			++pcCache;
		wNeed = MetaInt(& pcCache);
		if ((KEY *)0 != pKVExpr) {
			ppcList = ListCur(& pKVExpr->LIvalues, & wHas);
			if (wNeed < 1) {	/* key[0] => any values */
				iRet = wHas > 0 ? MS_ECHO : MS_FALSE;
			} else if (wNeed <= wHas && (char *)0 != ppcList[wNeed-1]) {
				iRet = MS_ECHO;
			}
		}
		if (']' != pcCache[0]) {
			fprintf(stderr, "%s: missing close bracket (]) in meta expression\n", progname);
			iExit |= MER_SYNTAX;
		} else {
			++pcCache;
		}
	} else if ('i' == pcCache[0] && 'n' == pcCache[1]) {
		/* syntax is: value in_ keyname
		 * keyname may be key[value] to take the key name from a key
		 */
		register char cEOS;
		register LIST_INDEX wScan;

		pcCache += 2;	/* skip keyword 'in' */
		while ('\000' != *pcCache && isspace(*pcCache))
			++pcCache;
		pcKey = MetaElement(& pcCache);
		if ((char *)0 != pcKey && (KEY *)0 != (pKVExpr = MetaKey(pcKey, pcCache, fVisible))) {
			ppcList = ListCur(& pKVExpr->LIvalues, & wHas);
			cEOS = *pcNull;
			*pcNull = '\000';
			for (wScan = 0; wScan < wHas; ++wScan) {
				if ((char *)0 == ppcList[wScan])
					continue;
				if (0 == strcmp(ppcList[wScan], pcValue)) {
					iRet = MS_ECHO;
					break;
				}
			}
			*pcNull = cEOS;
		}
	} else if ('i' == pcCache[0] && 's' == pcCache[1]) {
		/* syntax: key is_ INTEGER
		 * to check key's version field
		 */
		register int iVersion;

		pKVExpr = MetaKey(pcValue, pcNull, fVisible);
		pcCache += 2;	/* skip keywork "is" */
		while ('\000' != *pcCache && isspace(*pcCache))
			++pcCache;
		iVersion = MetaInt(& pcCache);
		if ((KEY *)0 != pKVExpr && (char *)0 != pKVExpr->pcversion && iVersion == atoi(pKVExpr->pcversion)) {
			iRet = MS_ECHO;
		}
	} else if (('!' == pcCache[0] || '=' == pcCache[0]) && '=' == pcCache[1]) {
		/* syntax is: value ==_ key[integer]
		 * or:	value !=_ key[integer]
		 * or:  valie ==_ value
		 */
		auto char cEq, cKeep1, cKeep2;
		register char *pcElement;

		cEq = *pcCache;
		pcCache += 2;	/* skip keyword '==' or '!=' */
		pcElement = MetaElement(& pcCache);
		cKeep1 = *pcNull;
		cKeep2 = *pcCache;
		*pcNull = '\000';
		*pcCache = '\000';
		if ((char *)0 != pcElement && ('=' == cEq) == (0 == strcmp(pcValue, pcElement)))
			iRet = MS_ECHO;
		*pcNull = cKeep1;
		*pcCache = cKeep2;
	} else if ((KEY *)0 != MetaKey(pcValue, pcNull, 0)) {
		/* syntax is: key_
		 * to check for existance (might have 0 value)
		 */
		iRet = MS_ECHO;
	}
	while ('\000' != *pcCache && isspace(*pcCache))
		++pcCache;
	*ppcExpr = pcCache;
	return iRet;
}

/* evaluate a meta expression						(ksb)
 * EXPR&&EXPR
 */
static int
MetaAnd(ppcExpr)
char **ppcExpr;
{
	register int iRet;
	auto char *pcCache;

	pcCache = *ppcExpr;
	iRet = MetaBase(& pcCache);
	while (MS_ECHO == iRet && '\000' != *pcCache) {
		while (isspace(*pcCache))
			++pcCache;
		if ('&' == pcCache[0] && '&' == pcCache[1]) {
			pcCache += 2;
			iRet = MetaBase(& pcCache);
		} else {
			break;
		}
	}
	*ppcExpr = pcCache;
	return iRet;
}

/* evaluate a meta expression						(ksb)
 * EXPR||EXPR
 */
static int
MetaOr(ppcExpr)
char **ppcExpr;
{
	register int iRet;
	auto char *pcCache;

	pcCache = *ppcExpr;
	iRet = MetaAnd(& pcCache);
	while (MS_FALSE == iRet && '\000' != *pcCache) {
		while (isspace(*pcCache))
			++pcCache;
		if ('|' == pcCache[0] && '|' == pcCache[1]) {
			pcCache += 2;
			iRet = MetaAnd(& pcCache);
		} else {
			break;
		}
	}
	*ppcExpr = pcCache;
	return iRet;
}

/* Copy the given meta file to the output through Emit			(ksb)
 * only lines presented with a % in col 1 are actually expanded
 * ANSI says that a C line can be upto 509 characters at most.
 * We read an extra % and allow for double (1k).
 *
 * There is no (other) safe way to check for the existance of a key, or
 * for the presentation of some key value.  We allow three forms here,
 *	%@abort message
 *	%@if EXPR ... {%@elif EXPR ...} [%@else ....] %@endif
 *	%@foreach KEY ... %@endfor
 *	%@eval line
 *	%@expand files
 * In the foreach clause %a is the current KEY value, in all escaped
 * lines %P is the meta filename that the source text came from.
 *
 * N.B. few modules use this code at present, I believe few will in
 * the future (L7 is the new tool to replace this code) so it is not
 * optimized for speed at all.
 */
void
MetaEmit(fp, pORScope, fpMeta, pcMeta)
FILE *fp, *fpMeta;
OPTION *pORScope;
char *pcMeta;
{
	auto char acLine[1024], acEval[1024];	/* same size buffers */
	register META_P *pMPCur, *pMPFor, *pMPTemp;
	register int iLine;
	auto char *pcExpr;
	register char *pcDst;
	auto EMIT_OPTS EOEval;

	EOEval.weattr = EMIT_DID_TABS;
	EOEval.iindent = 0;
	EOEval.pcarg1 = acVersion;	/* %a */
	EOEval.pcarg2 = pcMeta;		/* %P */
	EOEval.fp = fp;

	Emit(fp, "#line 1 \"%a\"\n", pORScope, pcMeta, 0);
	iLine = 0;
	pMPFor = pMPCur = (META_P *)0;
	pcExpr = (char *)0;
	while ((char *)0 != fgets(acLine, sizeof(acLine), fpMeta)) {
		fVisible = (META_P *)0 == pMPCur || MS_ECHO == pMPCur->cstatus;
		++iLine;
		if ('%' != acLine[0]) {
			if ((META_P *)0 == pMPCur || MS_ECHO == pMPCur->cstatus)
				fputs(acLine, fp);
			continue;
		}
		if ('@' == acLine[1]) {
			register char cTmp;
			register char *pcNl;

 meta_eval:
			cTmp = MetaToken(acLine+2, & pcExpr);
			while ('\n' != *pcExpr && isspace(*pcExpr))
				++pcExpr;
			if ((char *)0 != (pcNl = strchr(pcExpr, '\n'))) {
				do
					*pcNl = '\000';
				while (pcNl > pcExpr && (--pcNl, isspace(*pcNl)));
			}

			switch (cTmp) {
			case MT_ABORT:
				/* output message on line and quit
				 */
				fprintf(stderr, "%s: %s(%d) %s\n", progname, pcMeta, iLine, pcExpr);
				exit(MER_BROKEN);
			case MT_IF:
				/* build and push an if record
				 */
				pMPCur = NewMP(MT_IF, iLine, pMPCur);
				pMPCur->cstatus = MetaOr(& pcExpr);
				continue;
			case MT_ELSE:
				/* make sure cur is a if or elif
				 * update it
				 */
				if ((META_P *)0 == pMPCur) {
					fprintf(stderr, "%s: %s(%d): no enclosing %%@if\n", progname, pcMeta, iLine);
					iExit |= MER_SYNTAX;
					return;
				} else if (MT_IF != pMPCur->ctype) {
					fprintf(stderr, "%s: %s(%d): enclosing construction is not a %%@if\n", progname, pcMeta, iLine);
					iExit |= MER_SYNTAX;
					return;
				}
				if (MS_ECHO == pMPCur->cstatus) {
					pMPCur->cstatus = MS_DELETE;
				} else if (MS_FALSE == pMPCur->cstatus) {
					pMPCur->cstatus = MS_ECHO;
				}
				continue;
			case MT_ELIF:
				/* make sure cur is an if or elif
				 * update it
				 */
				if ((META_P *)0 == pMPCur) {
					fprintf(stderr, "%s: %s(%d): no enclosing %%@if\n", progname, pcMeta, iLine);
					iExit |= MER_SYNTAX;
					return;
				} else if (MT_IF != pMPCur->ctype) {
					fprintf(stderr, "%s: %s(%d): enclosing construction is not a %%@if\n", progname, pcMeta, iLine);
					iExit |= MER_SYNTAX;
					return;
				}
				if (MS_ECHO == pMPCur->cstatus) {
					pMPCur->cstatus = MS_DELETE;
				} else if (MS_FALSE == pMPCur->cstatus) {
					pMPCur->cstatus = MetaOr(& pcExpr);
				}
				continue;
			case MT_ENDIF:
				/* make sure cur is an if/elif/else
				 * end it, pop
				 */
				if ((META_P *)0 == pMPCur) {
					fprintf(stderr, "%s: %s(%d): no enclosing %%@if\n", progname, pcMeta, iLine);
					iExit |= MER_SYNTAX;
					return;
				} else if (MT_IF != pMPCur->ctype) {
					fprintf(stderr, "%s: %s(%d): enclosing construction is not a %%@if\n", progname, pcMeta, iLine);
					iExit |= MER_SYNTAX;
					return;
				}
				pMPTemp = pMPCur;
				pMPCur = pMPCur->pMPprev;
				free((void *) pMPTemp);
				break;
			case MT_EVAL:
				/* percent (toplevel) expand the line and
				 * re-interp as a command (yow!)
				 */
				pcExpr = strcpy(acEval, pcExpr);
				pcDst = acLine;
				*pcDst++ = '%';
				*pcDst++ = '@';
				EOEval.pcarg1 = (META_P *)0 != pMPFor ? pMPFor->ppccur[0] : acVersion;
				while ('\000' != *pcExpr) {
					if ('%' != (*pcDst++ = *pcExpr++))
						continue;
					--pcDst;
					if (0 != TopEsc(& pcExpr, pORScope, & EOEval, pcDst)) {
						iExit |= MER_SEMANTIC;
						break;
					}
					pcDst += strlen(pcDst);
				}
				*pcDst = '\000';
				if ('\000' == *pcExpr) {
					goto meta_eval;
				}
				fprintf(stderr, "%s: meta-eval failed to expand\n", progname);
				exit(MER_BROKEN);
				/*NOTREACHED*/
				break;
			case MT_FOREACH:
				/* build and push a loop record
				 */
				pMPCur = NewMP(MT_FOREACH, iLine, pMPCur);
				pMPCur->pMPfor = pMPFor;
				pMPFor = pMPCur;
				pMPCur->wwhere = ftell(fpMeta);
				pMPCur->pORwas = pORScope;

				pcDst = MetaElement(& pcExpr);
				pMPCur->pKVloop = (char *)0 != pcDst ? MetaKey(pcDst, pcExpr, fVisible) : (KEY *)0;
				while ('\000' != *pcExpr && isspace(*pcExpr))
					++pcExpr;
				pMPCur->pccntl = pMPCur->pcmem = strdup(pcExpr);
				pMPCur->wloop = 0;
				if ((KEY *)0 == pMPCur->pKVloop) {
					continue;
				}
				pMPCur->ppccur = ListCur(& pMPCur->pKVloop->LIvalues, & pMPCur->wloop);
				if (0 == pMPCur->wloop) {
					continue;
				}
				pMPCur->cstatus = MS_ECHO;
				if (KEY_CLIENT(pMPCur->pKVloop)) {
					pORScope = SynthDecode(pMPCur->ppccur[0]);
				}
				continue;
			case MT_ENDFOR:
				/* pop the for at the top of the stack
				 */
				if ((META_P *)0 == pMPCur) {
					fprintf(stderr, "%s: %s(%d): no enclosing %%@foreach\n", progname, pcMeta, iLine);
					iExit |= MER_SYNTAX;
					return;
				} else if (MT_FOREACH != pMPCur->ctype) {
					fprintf(stderr, "%s: %s(%d): enclosing construction is not a %%@foreach\n", progname, pcMeta, iLine);
					iExit |= MER_SYNTAX;
					return;
				}
				if (MS_ECHO == pMPCur->cstatus) {
					--pMPCur->wloop, ++pMPCur->ppccur;
				}
				/* while no values and more cntl exprs
				 * until the next key exists and it has values
				 * don't complain about undef extra keys
				 * this is used to L7 leverage
				 */
				pcExpr = pMPCur->pccntl;
				while (0 == pMPCur->wloop && '\000' != *pcExpr) {
					pcDst = MetaElement(& pcExpr);
					pMPCur->pKVloop = (char *)0 != pcDst ? MetaKey(pcDst, pcExpr, /*fVisible*/0) : (KEY *)0;
					while ('\000' != *pcExpr && isspace(*pcExpr))
						++pcExpr;
					if ((KEY *)0 == pMPCur->pKVloop)
						continue;
					pMPCur->ppccur = ListCur(& pMPCur->pKVloop->LIvalues, & pMPCur->wloop);
				}
				pMPCur->pccntl = pcExpr;
				if (0 != pMPCur->wloop) {
					pMPCur->cstatus = MS_ECHO;
					if (KEY_CLIENT(pMPCur->pKVloop)) {
						pORScope = SynthDecode(pMPCur->ppccur[0]);
					}
					fseek(fpMeta, pMPCur->wwhere, SEEK_SET);
					iLine = pMPCur->iline;
					continue;
				}
				pMPTemp = pMPCur;
				pORScope = pMPTemp->pORwas;
				free((void *) pMPTemp->pcmem);
				pMPCur = pMPTemp->pMPprev;
				pMPFor = pMPTemp->pMPfor;
				free((void *) pMPTemp);
				break;
			case MT_EXPAND:	/* expand a file */
				if ((META_P *)0 != pMPCur && MS_ECHO != pMPCur->cstatus) {
					continue;
				}
				while ('\000' != *pcExpr) {
					auto FILE *fpMeta;
					int (*pfiClose)();;

					pcDst = MetaElement(& pcExpr);
					if ((char *)0 == pcDst) {
						continue;
					}
					fpMeta = Search(pcDst, "r", & pfiClose);
					if ((FILE *)0 == fpMeta) {
						continue;
					}
					MetaEmit(fp, pORScope, fpMeta, pcDst);
					pfiClose(fpMeta);
				}
				break;
			default: /* internal error */
				fprintf(stderr, "%s: %s: meta expander internal error\n", progname, pcMeta);
				iExit |= MER_INV;
				return;
			}
			EmitN(fp, "#line %a ", pORScope, iLine+1, 0);
			Emit(fp, "\"%a\"\n", pORScope, pcMeta, 0);
			continue;
		}
		if ((META_P *)0 == pMPCur) {
			Emit2(fp, acLine+1, pORScope, (char *)0, pcMeta, 0);
			continue;
		}
		if (MS_ECHO != pMPCur->cstatus) {
			continue;
		}
		if ((META_P *)0 != pMPFor) {
			Emit2(fp, acLine+1, pORScope, pMPFor->ppccur[0], pcMeta, 0);
			continue;
		}
		Emit2(fp, acLine+1, pORScope, (char *)0, pcMeta, 0);
	}
}
