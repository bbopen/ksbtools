/* $Id: option.c,v 8.6 2009/10/14 00:00:00 ksb Exp $
 * parse mkcmd config files (new version)
 *	$Compile: ${CC-cc} -c ${DEBUG--g} -I/usr/include/local %f
 */
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>

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

OPTION
	*pORActn = nilOR,	/* special actions in program		*/
	*pORDecl = nilOR,	/* globals we are to declare		*/
	*pORRoot = nilOR,	/* program's options			*/
	*pORType = nilOR;	/* library defined types		*/

static char
	acUGave[] = "u_gave_";

/* maintain a list of unique options					(ksb)
 * (we can use linear search, only ~(26+26+10+1+1+1) of them!)
 * Use order '0' '1' ... 'a' 'A' 'b' 'B' ... 'z' 'Z' ...
 */
OPTION *
newopt(cThis, ppOR, fAdd, nAttr)
int cThis, fAdd, nAttr;
OPTION **ppOR;
{
	register OPTION *pORTemp;
	register char cLower, cScan;

	if (OPT_UNIQUE == cThis)
		cLower = cThis;
	else
		cLower = isupper(cThis) ? tolower(cThis) : cThis;
	while (nilOR != (pORTemp = *ppOR) && OPT_UNIQUE != cThis) {
		if (cThis == pORTemp->chname)
			return pORTemp;
		cScan = isupper(pORTemp->chname) ?
			tolower(pORTemp->chname) : pORTemp->chname;
		if (cLower < cScan || (cLower == cScan && isupper(pORTemp->chname)))
			break;
		ppOR = & (*ppOR)->pORnext;
	}
	if (! fAdd) {
		return nilOR;
	}
	*ppOR = newOR();
	(*ppOR)->pORnext = pORTemp;
	pORTemp = *ppOR;
	pORTemp->chname = cThis;
	pORTemp->oattr = nAttr;
	pORTemp->gattr = 0;
	pORTemp->ibundle = 0;
	pORTemp->pORalias = nilOR;
	pORTemp->pORali = nilOR;
	pORTemp->pORallow = nilOR;
	pORTemp->pORsact = nilOR;
	pORTemp->pOTtype = nilOT;
	pORTemp->pchinit = nil;
	pORTemp->pchname = nil;
	pORTemp->pchuupdate = nil;
	pORTemp->pcnoparam = nil;
	pORTemp->pchuser = nil;
	pORTemp->pchdesc = nil;
	pORTemp->pchverb = nil;
	pORTemp->pchtrack = nil;
	pORTemp->pchforbid = nil;
	pORTemp->pchfrom = nil;
	pORTemp->pchexit = nil;
	pORTemp->pchverify = nil;
	pORTemp->pcdim = nil;
	pORTemp->pcends = nil;
	pORTemp->pchkeep = nil;
	pORTemp->pchafter = nil;
	pORTemp->pchbefore = nil;
	pORTemp->pchgen = nil;
	pORTemp->iorder = 0;
	pORTemp->ppcorder = (char **)0;
	pORTemp->ppORorder = (OPTION **)0;
	pORTemp->pRG = (ROUTINE *)0;
	pORTemp->pKV = pORTemp->pKVclient = (KEY *)0;
	pORTemp->pctmannote = nil;
	pORTemp->pctupdate = nil;
	pORTemp->pctevery = nil;
	pORTemp->pctdef = nil;
	pORTemp->pctarg = nil;
	pORTemp->pctchk = nil;
	pORTemp->pcthelp = nil;

	return pORTemp;
}

/* return the C code for the options case constant			(ksb)
 */
char *
CaseName(pOR, pcDst)
OPTION *pOR;
char *pcDst;
{
	int ch;

	if (DEFCH == (ch = pOR->chname)) {
		(void)strcpy(pcDst, "default:");
	} else if (isprint(ch)) {
		sprintf(pcDst, "case \'%c\':", ch);
	} else {
		sprintf(pcDst, "case \'\\%03o\':", ch);
	}
	return pcDst;
}

/* find a mkcmd variable in the list, for %r{name}i			(ksb)
 * or the type facility
 */
OPTION *
FindVar(pcName, pORList)
char *pcName;
OPTION *pORList;
{
	register OPTION *pOR;

	for (pOR = pORList; (OPTION *)0 != pOR; pOR = pOR->pORnext) {
		if (0 == strcmp(pOR->pchname, pcName))
			return pOR;
	}
	return (OPTION *)0;
}

/* scan through all the option records in some order,			(ksb)
 * set *pi to 0 and pOR to nilOR and call me in a while loop
 * to visit *all( the option records (even the nested ones)
 *	for (s = 0, pOR = nilOR; nilOR != (pOR = OptScanAll(&s, pOR)); ) {...}
 */
OPTION *
OptScanAll(pi, pOR)
int *pi;
OPTION *pOR;
{
	static int wMax = 0, wCnt = 0;
	static PPM_BUF PPMState;
	register OPTION **ppORList;
	register int i;

	if ((int *)0 == pi) {			/* reset this machine */
		util_ppm_free(& PPMState);
		wMax = wCnt = 0;
		return nilOR;
	}
	if (0 == wMax) {
		wMax = 32;
		wCnt = 3;
		util_ppm_init(& PPMState, sizeof(OPTION *), wMax);
		ppORList = util_ppm_size(& PPMState, wCnt+2);
		ppORList[0] = pORRoot;
		ppORList[1] = pORDecl;
		ppORList[2] = pORActn;
		ppORList[3] = nilOR;
	} else {
		ppORList = util_ppm_size(& PPMState, wCnt+2);
	}
	if (nilOR != pOR) {
		pOR = pOR->pORnext;
	}
	while (nilOR == pOR && *pi< wCnt) {
		pOR = ppORList[(*pi)++];
	}
	if (nilOR == pOR || nilOR == pOR->pORsact) {
		return pOR;
	}

	/* The second time though we don't want to append the nesting again
	 */
	for (i = 0; i < wCnt; ++i) {
		if (pOR->pORsact == ppORList[i])
			return pOR;
	}
	ppORList[wCnt++] = pOR->pORsact;
	ppORList[wCnt] = nilOR;
	return pOR;
}

/* setup the synthetic link for the base types				(ksb)
 */
void
BuiltinTypes()
{
	register OINIT *pOI;
	register OPTION *pOR;
	register OPTTYPE *pOT;

	for (pOI = aOIBases; (char *)0 != pOI->pciname; ++pOI) {
		if ((OPTTYPE *)0 == (pOT = CvtType(pOI->cikey))) {
			fprintf(stderr, "%s: internal type botch\n", progname);
			exit(MER_PAIN);
		}
		pOR = newopt(OPT_UNIQUE, & pORType, 1, pOI->wiattr);
		SynthNew(pOR, pOT, pOI->pciname);
		pOR->pchinit = pOI->pciinit;
	}
}

static EMIT_MAP aERSact[] = {
	{'a', "after","control point after dash options are done"},
	{'b', "before", "control point before dash options started"},
	{'d', "right", "list of right justified positional parameters"},
	{'e', "every", "parameters remainign after dash options one at a time"},
	{'l', "list", "parameters remaining after dash options as a unit"},
	{'u', "error", (char *)0},
	{'s', "left", "list of left justified positional parameters"},
	{'x', "exit", "control for return/exit from converter"},
	{'z', "zero", "control for no parameters left after dash options"},
};

EMIT_HELP EHSact = {
	"control points", "%R",
	sizeof(aERSact)/sizeof(EMIT_MAP),
	aERSact,
	(char *)0,
	(EMIT_HELP *)0
};

/* find a special action from an options view in a expansion		(ksb)
 */
OPTION *
SactParse(ppcCursor, pORFrom, piWant)
char **ppcCursor;
OPTION *pORFrom;
int *piWant;
{
	register int cCmd;
	register OPTION *pORDown;
	auto int iBad;

	if ((int *)0 == piWant) {
		piWant = & iBad;
	}

	cCmd = EscCvt(& EHSact, ppcCursor, piWant);
	switch (cCmd) {
	case 0:
		fprintf(stderr, "%s: option: %%R: what number (%d)?\n", progname, *piWant);
		iExit |= MER_SEMANTIC;
		return (OPTION *)0;
	case -1:
		fprintf(stderr, "%s: %c: unknown percent escape, use '-E control' for help\n", progname, *piWant);
		iExit |= MER_SEMANTIC;
		return (OPTION *)0;
	case -2:
		fprintf(stderr, "%s: %*.*s: unknown long escape, use '-E control' for help\n", progname, *piWant, *piWant, *ppcCursor);
		iExit |= MER_SEMANTIC;
		return (OPTION *)0;
	default:
		break;
	}

	*piWant = cCmd;
	pORDown = nilOR;
	if (nilOR != pORFrom && nilOR != pORFrom->pORsact) {
		pORDown = newopt(cCmd, & pORFrom->pORsact, 0, 0);
	}
	if (nilOR == pORDown && nilOR != pORFrom && nilOR != pORFrom->pORallow) {
		pORDown = newopt(cCmd, & pORFrom->pORallow->pORsact, 0, 0);
	}
	if (nilOR == pORDown) {
		pORDown = newopt(cCmd, & pORActn, 0, 0);
	}
	return pORDown;
}


/* stringize a special action						(ksb)
 */
char *
sactstr(cName)
int cName;
{
	register int i;
	register EMIT_MAP *pER;

	for (pER = EHSact.pER, i = 0; i < EHSact.uhas; ++i, ++pER) {
		if (cName == pER->cekey)
			return pER->pcword;
	}

	fprintf(stderr, "%s: %c: unknown special action code (internal error in Emit?)\n", progname, cName);
	exit(MER_INV);
}

/* produce a name for an option record the programmer might recognize	(ksb)
 */
char *
usersees(pOR, pcOut)
OPTION *pOR;
char *pcOut;
{
	static char acTmp[48];
	if (nil == pcOut) {
		pcOut = acTmp;
	}

	if (ISSACT(pOR)) {
		(void)sprintf(pcOut, "control `%s\'", sactstr(pOR->chname));
	} else if (ISPPARAM(pOR)) {
		(void)sprintf(pcOut, "parameter %s", pOR->pchdesc);
	} else if (ISSYNTHTYPE(pOR)) {
		(void)sprintf(pcOut, "type %s", pOR->pchname);
	} else if (ISVARIABLE(pOR)) {
		(void)sprintf(pcOut, "variable %s", pOR->pchname);
	} else if ('+' == pOR->chname) {
		register char *pcGen;
		if ((char *)0 == (pcGen = pOR->pchgen)) {
			if (ISALIAS(pOR) && (char *)0 != pOR->pORali->pchgen) {
				pcGen = pOR->pORali->pchgen;
			} else {
				pcGen = "+";
			}
		}
		if ((char *)0 != pOR->pchdesc) {
			(void)sprintf(pcOut, "escape `%s%s\'", pcGen, pOR->pchdesc);
		} else if (ISALIAS(pOR) && (char *)0 != pOR->pORali->pchdesc) {
			(void)sprintf(pcOut, "escape `%s%s\'", pcGen, pOR->pORali->pchdesc);
		} else {
			(void)sprintf(pcOut, "escape `%sword\'", pcGen);
		}
	} else if ('#' == pOR->chname) {
		if ((char *)0 != pOR->pchdesc) {
			(void)sprintf(pcOut, "-%s", pOR->pchdesc);
		} else if (ISALIAS(pOR) && (char *)0 != pOR->pORali->pchdesc) {
			(void)sprintf(pcOut, "-%s", pOR->pORali->pchdesc);
		} else {
			(void)sprintf(pcOut, "-number");
		}
	} else if ('*' == pOR->chname) {
		(void)sprintf(pcOut, "noparameter");
	} else if ('?' == pOR->chname) {
		(void)sprintf(pcOut, "badoption");
	} else if (':' == pOR->chname) {
		(void)sprintf(pcOut, "otherwise");
	} else {
		(void)sprintf(pcOut, "option `-%c\'", pOR->chname);
	}
	return pcOut;
}


static EMIT_MAP aEROpt[] = {
	{'a', "after",	  "after attribute text"},
	{'A', "aliases",  "option we alias..."},
	{'b', "before",	  "before attribute text"},
	{'c', "update",	  "update attribute text"},
	{'C', "in",	  "option that allows us..."},
	{'d', "dimension","dimension attribute"},
	{'D', "ibundle",  (char *)0},
	{'e', "ends",	  "ends attribute text"},
	{'E', "exit",	  "exits attribute text"},
	{'f', "forbids",  "forbids list"},
	{'F', "from",	  "from file name"},
	{'g', "gen",	  (char *)0},
	{'G', "routine",  "routine name attribute"},	/* %mG */
	{'h', "help",	  "help attribute text"},
	{'i', "initializer", "initializer attribute text"},
	{'j', "jump",	  "jump out of justified parameter parser"},
	{'J', "column",	  (char *)0},	/* %mJ */
	{'K', "key",	  "expand the options unnamed private key..."},
	{'l', "letter",	  "option letter identifier"},
	{'L', "usersees", "what the user might call us"},
	{'m', "input",	  "chain to current input routine map..."},
	{'M', "finput",	  "chain to first input routine map..."},
	{'n', "named",	  "named attribute text (converted)"},
	{'N', "keep",	  "second named attribute text (keep buffer)"},
	{'p', "parameter", "parameter attribute text"},
	{'r', "respect",  "with respect to option..."},
	{'R', "control",  "with respect to control point..."},
	{'S', "case",	  "option's C case prefix (case '.':)"},
	{'u', "user",	  "user attribute text"},
	{'U', "track",	  "variable name for track boolean"},
	{'v', "verify",	  "verify function call"},
	{'W', "hunk",	  (char *)0},	/* %mW */
	{'x', "mytype",	  "select from type attributes..."},
	{'z', "error",	  (char *)0},
	{'_', "optstring", "getopt option string, letters and colons"},
	{'%', "percent",  (char *)0},
};

EMIT_HELP EHOpt = {
	"option expander", "%Z, or default from top",
	sizeof(aEROpt)/sizeof(EMIT_MAP),
	aEROpt,
	"select a option from the left/right list by index (1-n)...",
	& EHTop
};

/* parse an option description or buffer name from a string		(ksb)
 */
OPTION *
OptParse(ppcCursor, pOR)
char **ppcCursor;
OPTION *pOR;
{
	register char *pcVTemp;
	register int cCmd;
	register OPTION *pORDown;
	register int iLGroup = LGROUP;
	register int iRGroup = RGROUP;

	if (LGROUP == ppcCursor[0][0]) {
		iLGroup = LGROUP;
		iRGroup = RGROUP;
	} else if ('<' == ppcCursor[0][0]) {
		iLGroup = '<';
		iRGroup = '>';
	} else if ('(' == ppcCursor[0][0]) {
		iLGroup = '(';
		iRGroup = ')';
	} else {
		iLGroup = -1;
	}
	if (iLGroup == ppcCursor[0][0]) {
		pcVTemp = ++*ppcCursor;
		while (iRGroup != ppcCursor[0][0] && '\000' != ppcCursor[0][0])
			++*ppcCursor;
		if ('\000' == ppcCursor[0][0]) {
			fprintf(stderr, "%s: reference: unclosed {name}\n", progname);
			exit(MER_SYNTAX);
		}
		ppcCursor[0][0] = '\000';
		pORDown = FindVar(pcVTemp, pORDecl);
		if ((OPTION *)0 == pORDown) {
			fprintf(stderr, "%s: emit: reference to nonexistent variable `%s\'\n", progname, pcVTemp);
			iExit |= MER_SEMANTIC;
			return (OPTION *)0;
		}
		*ppcCursor[0]++ = iRGroup;
	} else {
		cCmd = *ppcCursor[0]++;
		pORDown = newopt(cCmd, & pORRoot, 0, 0);
		if ((OPTION *)0 == pORDown) {
			fprintf(stderr, "%s: emit: reference to nonexistent option `-%c\'\n", progname, cCmd);
			iExit |= MER_SEMANTIC;
			return (OPTION *)0;
		}
	}
	return pORDown;
}

/* convert an escape about an option into output text			(ksb)
 */
int
OptEsc(ppcCursor, pOR, pEO, pcDst)
char **ppcCursor, *pcDst;
OPTION *pOR;
EMIT_OPTS *pEO;
{
	register char *pcTmp;
	register OPTION *pORDown;
	register int iRet, cCmd;
	auto OPTION *pORKeep;
	auto int iNumber;
	static char *aapcGetopt[2][2] = {
		{ "", ":" }, { ":", "::"}
	};
	static OPTION *pORRecur = (OPTION *)0;

	cCmd = EscCvt(& EHOpt, ppcCursor, & iNumber);
	if (-1 == cCmd) {
		if (isprint(iNumber)) {
			fprintf(stderr, "%s: option level: %%%c: unknown percent escape, use '-E option' for help\n", progname, iNumber);
		} else {
			fprintf(stderr, "%s: option level: %%\\%03o: unknown percent escape, use '-E option' for help\n", progname, iNumber);
		}
		iExit |= MER_SYNTAX;
		return 1;
	}
	if (-2 == cCmd) {
		fprintf(stderr, "%s: option level: %*.*s: unknown long escape, use '-E option' for help\n", progname, iNumber, iNumber, *ppcCursor);
		iExit |= MER_SYNTAX;
		return 1;
	}
	if (nilOR == pOR && 'j' != cCmd && 'r' != cCmd && 'R' != cCmd) {
		if (0 == cCmd) {
			fprintf(stderr, "%s: option level: number %d given where no option available\n", progname, iNumber);
		} else {
			fprintf(stderr, "%s: option level: %%%c: given where no option available\n", progname, cCmd);
		}
		fprintf(stderr, " ...%s\n", *ppcCursor);
		exit(MER_SYNTAX);
	}

	switch (cCmd) {
	case 0:
		/* if we don't have an order list, but the option we are
		 * allowed by has one, take its order list (implicit %Cnumber)
		 */
		if ((OPTION **)0 == pOR->ppORorder && nilOR != pOR->pORallow && (OPTION **)0 != pOR->pORallow->ppORorder) {
			pOR = pOR->pORallow;
		}
		if (iNumber == 0 || iNumber > pOR->iorder) {
			fprintf(stderr, "%s: %s: no column %d (only ", progname, usersees(pOR, nil), iNumber);
			if (1 == pOR->iorder)
				fprintf(stderr, "1 column");
			else
				fprintf(stderr, "columns 1 to %d", pOR->iorder);
			fprintf(stderr, " available)\n");
			iExit |= MER_SEMANTIC;
			return 1;
		}
		--iNumber;
		pORKeep = pORRecur;
		pORRecur = pOR;
		iRet = TopEsc(ppcCursor, pOR->ppORorder[iNumber], pEO, pcDst);
		pORRecur = pORKeep;
		return iRet;
		break;
	case 'a':	/* A masked */
		if (nil == pOR->pchafter) {
			fprintf(stderr, "%s: %s doesn't have after text\n", progname, usersees(pOR, nil));
			iExit |= MER_SEMANTIC;
			return 1;
		}
		(void)strcpy(pcDst, pOR->pchafter);
		break;
	case 'A':	/* Z masked */
		pORKeep = pORRecur;
		pORRecur = pOR;
		if (nilOR != pOR->pORali) {
			fprintf(stderr, "%s: %s not an alias option\n", progname, usersees(pOR, nil));
			iExit |= MER_SEMANTIC;
		} else {
			pOR = pOR->pORali;
		}
		iRet = TopEsc(ppcCursor, pOR, pEO, pcDst);
		pORRecur = pORKeep;
		return iRet;
	case 'b':	/* Z masked */
		if (nil == pOR->pchbefore) {
			fprintf(stderr, "%s: %s doesn't have before text\n", progname, usersees(pOR, nil));
			iExit |= MER_SEMANTIC;
			return 1;
		}
		(void)strcpy(pcDst, pOR->pchbefore);
		break;
	case 'c':	/* Z masked */
		if (nil != pOR->pchuupdate)
			strcpy(pcDst, pOR->pchuupdate);
		else
			strcpy(pcDst, pOR->pOTtype->pchupdate);
		break;
	case 'C':	/* contained in */
		if (nilOR == pOR->pORallow) {
			fprintf(stderr, "%s: %s not an allow option\n", progname, usersees(pOR, nil));
			exit(MER_SEMANTIC);
		}
		pORKeep = pORRecur;
		pORRecur = pOR;
		iRet = TopEsc(ppcCursor, pOR->pORallow, pEO, pcDst);
		pORRecur = pORKeep;
		return iRet;
	case 'd':
		if (nil == pOR->pcdim) {
			fprintf(stderr, "%s: %s doesn\'t have a dimension for %%d\n", progname, usersees(pOR, nil));
			(void)strcpy(pcDst, "0");
			return 1;
		}
		(void)strcpy(pcDst, pOR->pcdim);
		break;
	case 'D':
		(void)sprintf(pcDst, "%d", pOR->ibundle);
		break;
	case 'e':	/* Z masked */
		if (nil == pOR->pcends) {
			fprintf(stderr, "%s: %s: doesn't have end text\n", progname, usersees(pOR, nil));
			return 1;
		}
		(void)strcpy(pcDst, pOR->pcends);
		break;
	case 'E':	/* Z masked */
		if (nil == pOR->pchexit) {
			fprintf(stderr, "%s: %s: doesn't have exit text\n", progname, usersees(pOR, nil));
			return 1;
		}
		(void)strcpy(pcDst, pOR->pchexit);
		break;
	case 'f':	/* Z masked, forbid list */
		(void)strcpy(pcDst, nil != pOR->pchforbid ? pOR->pchforbid : "");
		break;
	case 'F':	/* Z masked */
		if (nil == pOR->pchfrom) {
			fprintf(stderr, "%s: %s: doesn't have a from file name\n", progname, usersees(pOR, nil));
			return 1;
		}
		(void)strcpy(pcDst, pOR->pchfrom);
		break;
	case 'g':
		if (nil == pOR->pchgen) {
			fprintf(stderr, "%s: emit: internal error: no gen on `%s\'\n", progname, usersees(pOR, nil));
			exit(MER_INV);
		}
		(void)strcpy(pcDst, pOR->pchgen);
		break;
	case 'G':
		if ((struct RGnode *)0 == pOR->pRG || (char *)0 == pOR->pRG->pcraw) {
			fprintf(stderr, "%s: %s: has no input routine\n", progname, usersees(pOR, nil));
			exit(MER_SEMANTIC);
		}
		(void)strcpy(pcDst, pOR->pRG->pcraw);
		break;
	case 'h':
		if (nil == pOR->pchverb) {
			fprintf(stderr, "%s: emit: no help text given for %s\n", progname, usersees(pOR, nil));
			exit(MER_SEMANTIC);
		}
		(void)strcpy(pcDst, pOR->pchverb);
		break;
	case 'i':
		/* we can't check ISWHATINIT() here... we need the text
		 */
		if (nil == pOR->pchinit) {
			(void)strcpy(pcDst, pOR->pOTtype->pchdef);
		} else {
			(void)strcpy(pcDst, pOR->pchinit);
		}
		break;
	case 'j':
		if ((OPTION *)0 != pORRecur && pORRecur != pOR) {
			fprintf(stderr, "%s: %s: ", progname, usersees(pORRecur, nil));
			fprintf(stderr, "%%j: doesn\'t do what you want\n");
			exit(MER_BROKEN);
		}
		if (nil == (pcTmp = pcJustEscape)) {
			pcTmp = "done";
		}
		(void)sprintf(pcDst, "just_%s", pcTmp);
		break;
	case 'J':	/* which column are we in (in a file reader) use %mJ */
		(void)RColWhich(pOR, pcDst);
		break;
	case 'K':
		if ((KEY *)0 == pOR->pKV) {
			fprintf(stderr, "%s: %s: no unnamed key\n", progname, usersees(pOR, nil));
			return 1;
		}
		return KeyEsc(ppcCursor, pOR, pOR->pKV, pEO, pcDst);
	case 'l':
		*pcDst++ = pOR->chname;
		*pcDst = '\000';
		break;
	case 'L':
		usersees(pOR, pcDst);
		break;
	case 'M':	/* from first input map	*/
	case 'm':	/* from current input map */
		if ((ROUTINE *)0 != pOR->pRG) {
			pORDown = pOR;
		} else if (nilOR != pOR->pORallow && (ROUTINE *)0 != pOR->pORallow->pRG) {
			pORDown = pOR->pORallow;
		} else {
			fprintf(stderr, "%s: %s: has no input routine\n", progname, usersees(pOR, nil));
			exit(MER_SEMANTIC);
		}
		return RLineEsc(ppcCursor, pOR, 'M' == cCmd ? & pORDown->pRG->pRLfirst : pORDown->pRG->ppRL, pEO, pcDst);
	case 'N':
		if ((char *)0 != pOR->pchkeep) {
			(void)strcpy(pcDst, pOR->pchkeep);
			break;
		}
		/* if in the scope of %C, %rX or other such magic
		 * the value of %a doesn't match the %N we are asked for
		 * so we can't fall back			-- ksb
		 */
		if ((OPTION *)0 != pORRecur) {
			fprintf(stderr, "%s: %s: ", progname, usersees(pORRecur, nil));
			fprintf(stderr, "%s doesn\'t have a name for its unconverted value\n", usersees(pOR, nil));
			exit(MER_SEMANTIC);
		}
		(void)strcpy(pcDst, pEO->pcarg1);
		break;
	case 'n':
		if (nil == mkid(pOR, pcDst)) {
			return 1;
		}
		break;
	case 'p':
		(void)strcpy(pcDst, pOR->pchdesc);
		break;
	case 'r':
		pORKeep = pORRecur;
		pORRecur = pOR;
		pORDown = OptParse(ppcCursor, pOR);
		if ((OPTION *)0 == pORDown) {
			return 1;
		}
		iRet = TopEsc(ppcCursor, pORDown, pEO, pcDst);
		pORRecur = pORKeep;
		return iRet;
	case 'R':
		pORKeep = pORRecur;
		pORRecur = pOR;
		pORDown = SactParse(ppcCursor, pOR, & iNumber);
		if ((OPTION *)0 == pORDown) {
			fprintf(stderr, "%s: reference to nonexistent control point `%s\' from emit\n", progname, sactstr(iNumber));
			return MER_SEMANTIC;
		}
		iRet = TopEsc(ppcCursor, pORDown, pEO, pcDst);
		pORRecur = pORKeep;
		return iRet;
	case 'S':			/* select in switch/case */
		CaseName(pOR, pcDst);
		return 0;
	case 'u':	/* Z masked */
		if (nil == pOR->pchuser) {
			fprintf(stderr, "%s: %s: has not user attribute\n", progname, usersees(pOR, nil));
			return 1;
		}
		(void)strcpy(pcDst, pOR->pchuser);
		break;
	case 'U':
		if (!ISTRACK(pOR)) {
			fprintf(stderr, "%s: %s is not tracked\n", progname, usersees(pOR, nil));
			exit(MER_SEMANTIC);
		}
		if (nil != pOR->pchtrack) {
			(void)strcpy(pcDst, pOR->pchtrack);
		} else {
			(void)sprintf(pcDst, "%s%s", acUGave, mkdefid(pOR));
		}
		break;
	case 'v':	/* Z masked */
		if (nil == pOR->pchverify) {
			fprintf(stderr, "%s: %s: is not marked as needing verify code\n", progname, usersees(pOR, nil));
			if (nil != pOR->pOTtype->pchchk) {
				(void)strcpy(pcDst, pOR->pOTtype->pchchk);
			}
		} else {
			(void)strcpy(pcDst, pOR->pchverify);
		}
		break;
	case 'W':	/* masked by TopEsc W, use %mW which works */
		RHunkWhich(pOR, pcDst);
		break;
	case 'x':	/* some part of the type */
		iRet = TypeEsc(ppcCursor, pOR->pOTtype, pEO, pcDst);
		return iRet;
	case '_':
		(void)strcpy(pcDst, aapcGetopt[DIDOPTIONAL(pOR)][sbBInit != UNALIAS(pOR)->pOTtype->pchdef]);
		break;
	case '%':
		*pcDst++ = '%';
		*pcDst = '\000';
		break;
	default:
	case 'z':
		(void)CSpell(pcDst, cCmd, 0);
		fprintf(stderr, "%s: option level: %%%s: expander botch\n", progname, pcDst);
		iExit |= MER_PAIN;
		return 1;
	}

	return 0;
}

