/* $Id: parser.c,v 8.33 2000/06/14 13:02:52 ksb Exp $
 * parse new mkcmd config file
 *	$Compile: ${CC-cc} -c ${DEBUG--g} -I/usr/include/local %f
 */
#include <stdio.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/stat.h>

#include "machine.h"
#include "type.h"
#include "option.h"
#include "main.h"
#include "stracc.h"
#include "scan.h"
#include "list.h"
#include "mkcmd.h"
#include "check.h"
#include "mkusage.h"
#include "emit.h"
#include "key.h"

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

#if TILDEDIR
#include <pwd.h>
#endif

char
	acOpenOpt[] =
		"[",
	acCloseOpt[] =
		"]";
static char
	acFailOpen[] =
		"%s: fopen: %s: %s\n";

short int
	fGetenv,	/* parser saw a `getenv'			*/
	fPSawEnvInit,	/* parser saw `init getenv "..."'		*/
	fPSawList,	/* this parser saw any list control		*/
	fPSawZero,
	fPSawEvery;	/* this parser saw *any* every control		*/

static void MetaRequire();

/* This parser accepts a simple (?) configuration language for the	(ksb)
 * mkcmd command line option parser generator
 *
 * $Compile: ${cc-cc} -c -g -I/usr/include/local %f
 */

static TOKEN TokCur = TEOF;

/* make sure next token is a string					(ksb)
 */
static void
CheckString(pchWhat)
char *pchWhat;
{
	if (STRING != TokCur) {
		fprintf(stderr, "%s: %s(%d) %s must be followed by a string in quotes\n", progname, pchScanFile, iLine, pchWhat);
		exit(MER_SYNTAX);
	}
}

/* make aliases for an option (all chars in string)			(ksb)
 * We put these aliases on the end of the alias chain to give the
 * implementor some power to control the usage message for aliases.
 * [That is the order in the alias chain is used in the help text.]
 */
static void
MakeAliases(pORDef, pchAlias, ppORList, pOTGetting)
OPTION *pORDef, **ppORList;
char *pchAlias;
OPTTYPE *pOTGetting;
{
	register OPTION **ppORTail, *pORTemp;
	auto int nAttr;
	auto int cWho;
	auto char acMesg[64], acError[64];

	ppORTail = & pORDef->pORalias;
	while (nilOR != *ppORTail) {
		ppORTail = & (*ppORTail)->pORalias;
	}

	while ('\000' != (cWho = *pchAlias++)) {
		nAttr = '?' == cWho || '*' == cWho || DEFCH == cWho ? OPT_NOUSAGE|OPT_NOGETOPT : OPT_GLOBAL;
		pORTemp = newopt(cWho, ppORList, 1, nAttr);
		if (pORTemp == pORDef) {
			continue;
		}

		/* alias type and redeclaration checks
		 */
		if (nilOT == pORTemp->pOTtype) {
			/* OK we've never seen it, add it
			 */
		} else if (& OTAlias == pORTemp->pOTtype) {
			/* already an alias for pORDef, skip it; or another opt?
			 */
			if (pORTemp->pORali == pORDef) {
				continue;
			}
			fprintf(stderr, "%s: requested alias %s for %s collides with an alias of %s\n", progname, usersees(pORTemp, acMesg), usersees(pORDef, nil), usersees(pORTemp->pORali, acError));
			continue;
		} else if (pOTGetting != pORTemp->pOTtype) {
			fprintf(stderr, "%s: alias `-%c\' for %s collides with an option of type %s\n", progname, cWho, usersees(pORDef, nil), pORTemp->pOTtype->pchlabel);
			continue;
		} else {
			pORTemp->oattr &= ~OPT_AUGMENT;
		}

		/* add it to the chain
		 */
		*ppORTail = pORTemp;
		pORTemp->oattr |= OPT_ALIAS;
		pORTemp->pOTtype = & OTAlias;
		pORTemp->pORali = pORDef;
		ppORTail = & pORTemp->pORalias;
	}

	*ppORTail = nilOR;
}

/* check for a string or an integer in the token stream			(ksb)
 * return the value prefixed with a quote or an octothorp
 */
static char *
CheckSOrI(pcWhere)
char *pcWhere;
{
	register char cFlag;
	register char *pcTmp, *pcMem;
	auto unsigned sLen;

	if (INTEGER == TokCur) {
		cFlag = '#';
	} else {
		CheckString(pcWhere);
		cFlag = '\"';
	}
	pcTmp = UseChars(& sLen);
	pcMem = malloc(((sLen+1)|7)+1);	/* add at least _2_ */
	pcMem[0] = cFlag;
	(void)strcpy(pcMem+1, pcTmp);
	return pcMem;
}

static void Body(), Head();

/* read an ordered list (left|right)					(ksb)
 */
static char **
GrabList()
{
	register char **ppcList;
	register unsigned iMax, iCount;

#define GULP_LIST	10
	iMax = GULP_LIST;
	ppcList = (char **)malloc(iMax*sizeof(char *));
	iCount = 0;
	while (INTEGER == TokCur || STRING == TokCur) {
		ppcList[iCount] = CheckSOrI("data list");
		++iCount;

		TokCur = GetTok();
		if (iCount == iMax) {
			iMax += GULP_LIST;
			ppcList = (char **)realloc(ppcList, iMax*sizeof(char *));
			if ((char **)0 == ppcList) {
				fprintf(stderr, acOutMem, progname);
				exit(MER_LIMIT);
			}
		}
	}
	ppcList[iCount] = (char *)0;
#undef GULP_LIST
	return ppcList;
}

#include "routine.h"

/* another victim of the SGI broken "C" compiler.  Real C compilers,
 * like gcc, handle this with no problem.
 */
static KEY *KeyDef();


/* parse the special input format for a configuration file reader	(ksb)
 * Allocate a `routine gen' structure and fill it in, built R_LINE
 * nodes for each hunk of lines.
 * Hunks lines are separated by a HERE document style token by default
 * this is percent-percent (%%).  [Or the last line's delimiter?]
 */
static void
Routine(pOR)
OPTION *pOR;
{
	register ROUTINE *pRGFile;
	register TOKEN TokRtn;
	register R_LINE *pRLLine, **ppRL;
	auto unsigned sLen;
	auto int fRtnAugment;

	/* if there is already a pOR->pRG we should choke here
	 */
	if ((ROUTINE *)0 != pOR->pRG) {
		fprintf(stderr, "%s: %s(%d) %s already has an input/output routine description\n", progname, pchScanFile, iLine, usersees(pOR, nil));
		exit(MER_SEMANTIC);
	}
	pRGFile = (ROUTINE *)malloc(sizeof(ROUTINE));
	if ((ROUTINE *)0 == pRGFile) {
		fprintf(stderr, acOutMem, progname);
		exit(MER_LIMIT);
	}

	if (STRING == TokCur) {
		pRGFile->pcfunc = TakeChars(& sLen);
		TokCur = GetTok();
	} else {
		register char *pcTemp;
		static char acDefRoutine[] = "u_read_%s";
		pcTemp = mkdefid(pOR);
		pRGFile->pcfunc = malloc(strlen(pcTemp)+sizeof(acDefRoutine));
		(void)sprintf(pRGFile->pcfunc, acDefRoutine, pcTemp);
	}
	pRGFile->pORfunc = nilOR;
	pRGFile->pcprototype = pRGFile->pcvector = (char *)0;
	pRGFile->pcterse = pRGFile->pcusage = (char *)0;
	pRGFile->pclineno = (char *)0;
	pRGFile->pKV = (KEY *)0;
	if (STRING == TokCur) {
		pRGFile->pcraw = TakeChars(& sLen);
		TokCur = GetTok();
	} else {
		pRGFile->pcraw = nil;
	}
	if (LCURLY != TokCur) {
		fprintf(stderr, "%s: %s(%d) left curly brace missing for routine definition (at %s)\n", progname, pchScanFile, iLine, TextTok(TokCur));
		exit(MER_SYNTAX);
	}
	TokCur = GetTok();
	ppRL = & pRGFile->pRLfirst;
	*ppRL = (R_LINE *)0;

	/* read line descriptions
	 */
	fRtnAugment = 0;
	while (RCURLY != (TokRtn = TokCur)) {
		if (fRtnAugment) {
			fprintf(stderr, "%s: %s(%d) routine: augment what?\n", progname, pchScanFile, iLine);
		}
		TokCur = GetTok();
		if ((fRtnAugment = (TAUGMENT == TokRtn))) {
			TokRtn = TokCur;
			TokCur = GetTok();
		}
		pRLLine = RLine(ppRL);
		switch (TokRtn) {
		/* routine attributes */
		case TLOCAL:	/* make terse and vector local to usage */
			pRGFile->irattr |= ROUTINE_LOCAL;
			continue;
		case TGLOBAL:
			pRGFile->irattr &= ~ROUTINE_LOCAL;
			continue;
		case TSTATIC:	/* mark routine (plus terse/vector) static */
			pRGFile->irattr |= ROUTINE_STATIC;
			continue;
		case TPROTOTYPE:
			CheckString("prototype");
			pRGFile->pcprototype = TakeChars(& sLen);
			break;
		case TTERSE:
			CheckString("terse");
			pRGFile->pcterse = TakeChars(& sLen);
			break;
		case TTRACK:	/* set line number name */
			CheckString("track line number");
			pRGFile->pclineno = TakeChars(& sLen);
			break;
		case TUSAGE:
			CheckString("usage");
			pRGFile->pcusage = TakeChars(& sLen);
			break;
		case TVECTOR:
			CheckString("vector");
			pRGFile->pcvector = TakeChars(& sLen);
			break;

		/* hunk attributes */
		case TCOMMENT:
			CheckString("comment");
			pRLLine->pcopenc = TakeChars(& sLen);
			break;
		case TNAMED:
			CheckString("named");
			pRLLine->pcnamed = TakeChars(& sLen);
			break;
		case TBEFORE:
			CheckString("before");
			pRLLine->pcbefore = TakeLine(& sLen);
			break;
		case TAFTER:
			CheckString("after");
			pRLLine->pcafter = TakeLine(& sLen);
			break;
		case THELP:
			CheckString("help");
			pRLLine->pchelp = TakeChars(& sLen);
			break;
		case TLIST:
			pRLLine->ppclorder = GrabList();
			continue;
		case TUPDATE:
			CheckString("update");
			pRLLine->pcupdate = TakeLine(& sLen);
			break;
		case TUSER:
			CheckString("user");
			pRLLine->pcuser = TakeLine(& sLen);
			break;
		case TABORTS:
			CheckString("aborts");
			pRLLine->pcaborts = TakeLine(& sLen);
			break;
		case TENDS:
			pRLLine->pcemark = CheckSOrI("ends");
			ppRL = &(ppRL[0]->pRLnext);
			*ppRL = (R_LINE *)0;
			break;
		case TSTOPS:	/* just move to next hunk */
			ppRL = &(ppRL[0]->pRLnext);
			*ppRL = (R_LINE *)0;
			continue;
		case STRING:
			fprintf(stderr, "%s: %s(%d) input routine found a string \"%s\"\n", progname, pchScanFile, iLine, UseChars(& sLen));
			break;

		/* We allow triggers/buffers here */
		case TTYPE:
		case TACTION:
		case TTOGGLE:
		case TBOOLEAN:
		case TDOUBLE:
		case TFUNCTION:
		case TINTEGER:
		case TUNSIGNED:
		case TLONG:
		case TCHARP:
		case TPOINTER:
		case TLETTER:
		case TFD:
		case TFILE:
		case TACCUM:
		case TSTRING:
		case TESCAPE:
		case TNUMBER:
			Head(TokRtn, pOR, fRtnAugment);
			fRtnAugment = 0;
			break;

		case TKEY:	/* unnamed key in routine */
			KeyDef(KV_ROUTINE, fRtnAugment, 1, 0, pOR, & pRGFile->pKV);
			fRtnAugment = 0;
			break;

		default:
			fprintf(stderr, "%s: %s(%d) syntax error in input routine specification\n", progname, pchScanFile, iLine);
			if (TEOF == TokRtn) {
				exit(MER_SYNTAX);
			}
			goto botch;
		}
		TokCur = GetTok();
	}
	TokCur = GetTok();
botch:
	pRGFile->ppRL = ppRL;
	pOR->pRG = pRGFile;
}

/* assumes we are on the string after the keyword "type"		(ksb)
 */
static int
ParseType()
{
	register char *pcTName;
	register OPTION *pOR;
	auto int sLen;

	CheckString("type");
	pcTName = TakeChars(& sLen);
	if (sLen < 1) {
		fprintf(stderr, "%s: %s(%d) type names cannot be null\n", progname, pchScanFile, iLine);
		exit(MER_SEMANTIC);
	}
	pOR = FindVar(pcTName, pORType);
	if (nilOR == pOR) {
		pOR = newopt(OPT_UNIQUE, & pORType, 1, OPT_GLOBAL|OPT_SYNTHTYPE);
		SynthNew(pOR, nilOT, pcTName);
	}
	return pOR->pOTtype->chkey;
}

/* get the subtype of an every or an escape				(ksb)
 */
static void
SubType(piMap, ppcDim)
int *piMap;
char **ppcDim;
{
	register int iMap;
	register char *pcDim;
	auto unsigned sLen;

	pcDim = *ppcDim;
	switch (TokCur) {
	case TACTION:
	case TTOGGLE:
	case TBOOLEAN:
		fprintf(stderr, "%s: %s(%d) every type must take a parameter\n", progname, pchScanFile, iLine);
		exit(MER_SYNTAX);

	case TDOUBLE:
		iMap = 'd';
		break;
	case TFUNCTION:
		iMap = 'f';
		pcDim = sbIBase;
		break;
	case TINTEGER:
		iMap = 'i';
		break;
	case TUNSIGNED:
		iMap = 'u';
		break;
	case TLONG:
		iMap = 'l';
		break;
	case TCHARP:
		iMap = 'p';
		break;
	case TPOINTER:
		iMap = 'v';
		break;
	case TLETTER:
		iMap = 'c';
		break;
	case TFD:
		iMap = 'D';
		pcDim = sbGenFd;
		break;
	case TFILE:
		iMap = 'F';
		pcDim = "r";
		break;
	case TACCUM:
		iMap = '+';
		pcDim = ":";
		break;
	case TSTRING:
		iMap = 's';
		pcDim = "";
		break;
	case TTYPE:
		TokCur = GetTok();
		iMap = ParseType();
		break;
	default:
		return;
	}
	TokCur = GetTok();

	/* things that might take ["stuff"]
	 */
	if (LBRACKET == TokCur) {
		TokCur = GetTok();
		if (STRING == TokCur) {
			pcDim = TakeChars(& sLen);
			TokCur = GetTok();
		} else if (INTEGER == TokCur) {
			pcDim = TakeChars(& sLen);
			TokCur = GetTok();
		}
		if (RBRACKET != TokCur) {
			fprintf(stderr, "%s: %s(%d) missing right bracket in declaration\n", progname, pchScanFile, iLine);
			exit(MER_SYNTAX);
		}
		TokCur = GetTok();
	}
	*piMap = iMap;
	*ppcDim = pcDim;
}


/* output a nifty error message for the developer			(ksb)
 * when we find  <"augment" STRING> in the input file
 */
static void
BrokenAug(pcRec, fAugment, pcWhere)
register char *pcRec, *pcWhere;
int fAugment;
{
	register OPTION *pOR;

	if (fAugment) {
		pOR = newopt(pcRec[0], & pORRoot, 0, 0);
		if (nilOR == pOR) {
			fprintf(stderr, "%s: %s(%d) must provide a type to augment %s\n", progname, pchScanFile, iLine, pcRec);
		} else {
			fprintf(stderr, "%s: %s(%d) to augment \"%s\" provide type %s\n", progname, pchScanFile, iLine, pcRec, pOR->pOTtype->pchlabel);
		}
	} else {
		fprintf(stderr, "%s: %s(%d) dangling string (%s) %s\n", progname, pchScanFile, iLine, pcRec, pcWhere);
	}
	exit(MER_SYNTAX);
}

/* check all the augment/type rules				(wam & ksb)
 */
static void
AugCheck(pOR, pOTTest, fAugment, pcNewDim)
OPTION *pOR;
OPTTYPE *pOTTest;
int fAugment;
char *pcNewDim;
{
	if (fAugment) {
		if (& OTAlias == pOR->pOTtype) {
			if (pOR->pORali->pOTtype != pOTTest) {
				fprintf(stderr, "%s: %s(%d) augment for alias %s should be type %s\n", progname, pchScanFile, iLine, usersees(pOR, nil), pOR->pORali->pOTtype->pchlabel);
				exit(MER_SEMANTIC);
			}
		} else if (nilOT == pOR->pOTtype) {
			pOR->oattr |= OPT_AUGMENT;
			pOR->pOTtype = pOTTest;
		} else if (IS_SYNTH(pOR->pOTtype)) {
			/* test is the contextual base type to cmp */
			if (pOR->pOTtype->pOTsbase != pOTTest) {
				fprintf(stderr, "%s: %s(%d) base type for synthetic %s differs from definition type (%s != %s)\n", progname, pchScanFile, iLine, usersees(pOR, nil), pOR->pOTtype->pOTsbase->pchlabel, pOTTest->pchlabel);
				exit(MER_SEMANTIC);
			}
		} else if (pOTTest != pOR->pOTtype) {
			if (ISAUGMENT(pOR)) {
				fprintf(stderr, "%s: %s(%d) type for augment of %s differs from previous type (%s != %s)\n", progname, pchScanFile, iLine, usersees(pOR, nil), pOR->pOTtype->pchlabel, pOTTest->pchlabel);
			} else {
				fprintf(stderr, "%s: %s(%d) type for augment of %s differs from definition type (%s != %s)\n", progname, pchScanFile, iLine, usersees(pOR, nil), pOR->pOTtype->pchlabel, pOTTest->pchlabel);
			}
			exit(MER_SEMANTIC);
		}
	} else if (nilOT == pOR->pOTtype) {
		pOR->pOTtype = pOTTest;
	} else if (! ISAUGMENT(pOR)) {
		fprintf(stderr, "%s: %s(%d) %s given more than once (augment?)\n", progname, pchScanFile, iLine, usersees(pOR, nil));
		exit(MER_SEMANTIC);
	} else if (pOTTest != pOR->pOTtype) {
		fprintf(stderr, "%s: %s(%d) defined type of %s differs from previous augment type (%s != %s)\n", progname, pchScanFile, iLine, usersees(pOR, nil), pOTTest->pchlabel, pOR->pOTtype->pchlabel);
		exit(MER_SEMANTIC);
	} else {
		pOR->oattr &= ~OPT_AUGMENT;
	}
	if ((char *)0 == pOR->pcdim || '\000' == pOR->pcdim[0]) {
		pOR->pcdim = pcNewDim;
	} else if ((char *)0 == pcNewDim || '\000' == pcNewDim[0]) {
		/* nothing, thanks */
	} else if (0 == strcmp(pcNewDim, pOR->pcdim)) {
		/* that is OK too */
	} else {
		fprintf(stderr, "%s: %s(%d) dimension for %s %s differs from previous (%s != %s)\n", progname, pchScanFile, iLine, pOTTest->pchlabel, usersees(pOR, nil), pOR->pcdim, pcNewDim);
	}
	pOR->pOTtype->tattr |= _USED;
}

/* read an ordered list (left|right)					(ksb)
 */
static char **
GrabOrder()
{
	register char **ppcList;
	register unsigned iMax, iCount, iDepth;
	auto unsigned sLen;

#define GULP_GRAB	10
	iMax = GULP_GRAB;
	ppcList = (char **)malloc(iMax*sizeof(char *));
	iDepth = iCount = 0;
	while (LBRACKET == TokCur || RBRACKET == TokCur || STRING == TokCur) {
		if (LBRACKET == TokCur) {
			if (0 != iDepth++) {
				fprintf(stderr, "%s: %s(%d) cannot nest brackets\n", progname, pchScanFile, iLine);
			}
			ppcList[iCount++] = acOpenOpt;
		} else if (RBRACKET == TokCur) {
			if (1 != iDepth--) {
				fprintf(stderr, "%s: %s(%d) list missing open bracket?\n", progname, pchScanFile, iLine);
			}
			ppcList[iCount++] = acCloseOpt;
		} else {
			CheckString("ordered list");
			ppcList[iCount++] = TakeChars(& sLen);
		}
		TokCur = GetTok();
		if (iCount == iMax) {
			iMax += GULP_GRAB;
			ppcList = (char **)realloc(ppcList, iMax*sizeof(char *));
			if ((char **)0 == ppcList) {
				fprintf(stderr, acOutMem, progname);
				exit(MER_LIMIT);
			}
		}
	}
	ppcList[iCount] = (char *)0;
#undef GULP_GRAB
	return ppcList;
}

/* get a control point from the user, used to be a special action	(ksb)
 */
static void
Special(TokLoc, ppORIn, fSpecialAugment, pORAllow)
TOKEN TokLoc;
OPTION **ppORIn, *pORAllow;
int fSpecialAugment;
{
	register OPTION *pOR;
	auto int iMap;
	auto char *pcDim;

	switch (TokLoc) {
	case TLEFT:
		iMap = 's';
		pcDim = "left";
		if (0) {
	case TRIGHT:
			iMap = 'd';
			pcDim = "right";
		}
		pOR = newopt(iMap, ppORIn, 1, OPT_SACT);
		AugCheck(pOR, CvtType('a'), fSpecialAugment, (char *)0);
		pOR->ppcorder = GrabOrder();
		if ((char *)0 == pOR->ppcorder[0]) {
			fprintf(stderr, "%s: %s(%d) special action `%s\' given no positional parameters (then remove it)\n", progname, pchScanFile, iLine, pcDim);
			iExit |= MER_SEMANTIC;
		}
		break;

	case TAFTER:
		iMap = 'a';
		while (0) {
	case TBEFORE:
			iMap = 'b';
			break;
	case TEVERY:
			iMap = 'e';
			fPSawEvery = 1;
			break;
	case TEXIT:
			iMap = 'x';
			break;
	case TLIST:
			fPSawList = 1;
			iMap = 'l';
			break;
	case TZERO:
			fPSawZero = 1;
			iMap = 'z';
		}
		pOR = newopt(iMap, ppORIn, 1, OPT_GLOBAL|OPT_SACT);
		if ('l' == iMap) {
			pOR->oattr |= OPT_PTR2;
		}
		pcDim = nil;
		if ('e' == iMap || 'l' == iMap) {
			iMap = 'f';
			SubType(& iMap, & pcDim);
		} else {
			iMap = 'a';
		}
		AugCheck(pOR, CvtType(iMap), fSpecialAugment, pcDim);
		break;
	default:
		fprintf(stderr, "%s: internal flow error\n", progname);
		exit(MER_INV);
	}
	pOR->pORallow = pORAllow;
	Body(pOR, fSpecialAugment);
}

/* parse a initial list or cleanup list					(ksb)
 *  [prio] C1 C2 C3 ...
 */
static void
TodoList(pcName, pLIQueue)
char *pcName;
LIST *pLIQueue;
{
	register LIST *pLIIn;
	auto unsigned sLen;
	register int iPrio, iCvt;

	iPrio = 5;
	if (INTEGER == TokCur) {
		iCvt = atoi(UseChars(& sLen));
		if (iCvt < 0 || iCvt > 10) {
			fprintf(stderr, "%s: %s(%d) %s priority levels run from 0 to 10 (%d is out of range)\n", progname, pchScanFile, iLine, pcName, iCvt);
			iExit |= MER_SEMANTIC;
		} else {
			iPrio = iCvt;
		}
		TokCur = GetTok();
	}
	pLIIn = & pLIQueue[iPrio];
	CheckString(pcName);
	do {
		ListAdd(pLIIn, TakeLine(& sLen));
		TokCur = GetTok();
	} while (STRING == TokCur);
}

/* parse option bodies -- for given option only				(ksb)
 * we eat the LCURLY or SEMI token for the caller
 */
static void
Body(pOR, fInAugment)
register OPTION *pOR;
int fInAugment;
{
	register TOKEN TokBody;
	register char *pchTemp;
	auto unsigned sLen;
	auto int fBodyAugment;

	TokBody = TokCur;
	TokCur = GetTok();
	if (SEMI == TokBody) {
		return;
	}
	if (LCURLY != TokBody) {
		fprintf(stderr, "%s: %s(%d) left curly brace missing for definition (at %s)\n", progname, pchScanFile, iLine, TextTok(TokBody));
		exit(MER_SYNTAX);
	}
	fBodyAugment = 0;
	while (TokCur != RCURLY) {
		if (fBodyAugment) {
			fprintf(stderr, "%s: %s(%d) augment what?\n", progname, pchScanFile, iLine);
		}
		TokBody = TokCur;
		TokCur = GetTok();
		/* gcc -Wall extra parens, sigh -- ksb */
		if ((fBodyAugment = (TAUGMENT == TokBody))) {
			TokBody = TokCur;
			TokCur = GetTok();
		}
		switch (TokBody) {
		case TABORTS:
			CheckString("aborts");
			pOR->oattr |= OPT_ABORT;
			pOR->pchexit = TakeLine(& sLen);
			break;
		case TEXCLUDES:
			CheckString("excludes");
			pOR->oattr |= OPT_FORBID;
			do {
				pchTemp = TakeChars(& sLen);
				if ((char *)0 == pOR->pchforbid) {
					pOR->pchforbid = pchTemp;
				} else {
					pOR->pchforbid = realloc(pOR->pchforbid, strlen(pOR->pchforbid)+sLen+1);
					(void)strcat(pOR->pchforbid, pchTemp);
					free(pchTemp);
				}
				TokCur = GetTok();
			} while (STRING == TokCur);
			continue;
		case THELP:
			CheckString("help");
			pOR->pchverb = TakeChars(& sLen);
			break;
		case THIDDEN:
			pOR->oattr |= OPT_HIDDEN;
			continue;
		case TCLEANUP:
			TodoList("cleanup", aLIExits);
			continue;
		case TINITIALIZER:
			if (TGETENV == TokCur) {
				TokCur = GetTok();
				pOR->oattr |= OPT_INITENV;
				fPSawEnvInit = 1;
			} else if (TDYNAMIC == TokCur) {
				TokCur = GetTok();
				pOR->oattr |= OPT_INITEXPR;
			} else if (TSTATIC == TokCur) {
				TokCur = GetTok();
				pOR->oattr |= OPT_INITCONST;
			} else if (TACTION == TokCur) {
				TokCur = GetTok();
				TodoList("initializer", aLIInits);
				continue;
			}
			CheckString("initializer");
			pOR->pchinit = TakeChars(& sLen);
			break;
		case TKEY:	/* key in trigger */
			(void)KeyDef(KV_OPTION, fBodyAugment, ! fInAugment, 0, pOR, & pOR->pKV);
			fBodyAugment = 0;
			break;
		case TREQUIRES:
			CheckString("attribute requires");
			do {
				MetaRequire(TakeChars(& sLen));
				TokCur = GetTok();
			} while (STRING == TokCur);
			continue;
		case TROUTINE:
			(void)Routine(pOR);
			continue;
		case TSTATIC:
			pOR->oattr |= OPT_STATIC;
			continue;
		case TLATE:
			pOR->oattr |= OPT_LATE;
			continue;
		case TLOCAL:
			pOR->oattr &= ~OPT_GLOBAL;
			continue;
		case TGLOBAL:
			pOR->oattr |= OPT_GLOBAL;
			continue;
		case TFROM:
			/* if we overwrite the last one it is OK. -- ksb
			 */
			CheckString("from");
			pOR->pchfrom = TakeChars(& sLen);
			if (0 != sLen) {
				ListAdd(& LIUserIncl, pOR->pchfrom);
			}
			break;
		case TNAMED:	/* named "convert" ["keep"] */
			CheckString("named");
			pOR->pchname = TakeChars(& sLen);
			TokCur = GetTok();
			if (STRING == TokCur) {
				pOR->pchkeep = TakeChars(& sLen);
				TokCur = GetTok();
			}
			continue;
		case TKEEP:	/* just the keep part */
			CheckString("keep");
			pOR->pchkeep = TakeChars(& sLen);
			break;
		case TNOPARAM:
			CheckString("noparameter");
			pOR->pcnoparam = TakeChars(& sLen);
			break;
		case TONCE:
			pOR->oattr |= OPT_ONLY;
			continue;
		case TPARAMETER:
			CheckString("parameter");
			pOR->pchdesc = TakeChars(& sLen);
			break;
		case TTRACK:
			pOR->oattr |= OPT_TRACK|OPT_TRGLOBAL;
			if (STRING != TokCur) {
				continue;
			}
			pOR->pchtrack = TakeChars(& sLen);
			break;
		case TUPDATE:
			CheckString("update");
			pOR->pchuupdate = TakeLine(& sLen);
			break;
		case TUSER:
			CheckString("user");
			pOR->pchuser = TakeLine(& sLen);
			break;
		case TVERIFY:
			pOR->oattr |= OPT_VERIFY;
			if (STRING == TokCur) {
				pOR->pchverify = TakeLine(& sLen);
				break;
			}
			pOR->pOTtype->tattr |= _VUSED;
			continue;
		case TENDS:
			pOR->oattr |= OPT_BREAK|OPT_ENDS;
			if (STRING == TokCur) {
				pOR->pcends = TakeLine(& sLen);
				break;
			}
			continue;
		case TSTOPS:
			pOR->oattr |= OPT_BREAK;
			continue;

		case TTYPE:	/* type "name" or type attribute */
			if (STRING == TokCur) {
				/* avoid the checks below */
			} else if (!ISSYNTHTYPE(pOR)) {
				fprintf(stderr, "%s: %s(%d) type attribute on non-type trigger\n", progname, pchScanFile, iLine);
				iExit |= MER_SEMANTIC;
				TokCur = GetTok();
				break;	/* skip string */
			} else {
				TokBody = TokCur;
				TokCur = GetTok();
				switch (TokBody) {
				case TCOMMENT:
					CheckString("type manual comment");
					pOR->pctmannote = TakeLine(& sLen);
					break;
				case TUPDATE:
					CheckString("type update");
					pOR->pctupdate = TakeLine(& sLen);
					break;
				case TEVERY:
					CheckString("type every");
					pOR->pctevery = TakeLine(& sLen);
					break;
				case TKEEP:
					pOR->pOTtype->tattr |= _MKEEP;
					continue;
				case TDYNAMIC:
					pOR->pOTtype->tattr |= _ACTIVE;
					continue;
				case TINITIALIZER:
					if (TSTATIC == TokCur) {
						TokCur = GetTok();
						pOR->oattr |= OPT_INITCONST;
					} else if (TDYNAMIC == TokCur) {
						TokCur = GetTok();
						pOR->pOTtype->tattr |= _ACTIVE;
					} else if (TGETENV == TokCur) {
						fprintf(stderr, "%s: %s(%d) type initailizers constant expressions\n", progname, pchScanFile, iLine);
						iExit |= MER_SYNTAX;
					}
					CheckString("type initializer");
					pOR->pctdef = TakeChars(& sLen);
					break;
				case THELP:
					CheckString("type help");
					pOR->pcthelp = TakeChars(& sLen);
					break;
				case TPARAMETER:
					CheckString("type parameter");
					pOR->pctarg = TakeChars(& sLen);
					break;
				case TROUTINE:	/* routine type */
					TokBody = TokCur;
					TokCur = GetTok();
					switch (TokBody) {
					case TACTION:
						pOR->pOTtype->tattr |= _RTN_VD;
						break;
					case TCHARP:
					case TACCUM:
					case TSTRING:
					case TESCAPE:
						pOR->pOTtype->tattr |= _RTN_PC;
						break;
					case TFD:
						pOR->pOTtype->tattr |= _RTN_FD;
						break;
					case TFILE:
						pOR->pOTtype->tattr |= _RTN_FP;
						break;
					default:
						fprintf(stderr, "%s: %s(%d) syntax error in type routine declaration (%s)\n", progname, pchScanFile, iLine, TextTok(TokBody));
						iExit |= MER_SYNTAX;
						continue;
					}
					continue;
				case TVERIFY:
					CheckString("type verify");
					pOR->pctchk = TakeLine(& sLen);
					break;
				default:
					fprintf(stderr, "%s: %s(%d) syntax error in type attribute (%s)\n", progname, pchScanFile, iLine, TextTok(TokBody));
					iExit |= MER_SYNTAX;
					continue;
				}
				break;
			}
			/* fallthrough */
		case TACTION:
		case TTOGGLE:
		case TBOOLEAN:
		case TDOUBLE:
		case TFUNCTION:
		case TINTEGER:
		case TUNSIGNED:
		case TLONG:
		case TCHARP:
		case TPOINTER:
		case TLETTER:
		case TFD:
		case TFILE:
		case TACCUM:
		case TSTRING:
		case TESCAPE:
		case TNUMBER:
		/* do not include badopt, noparam, and otherwise here
		 * because they cannot be allowed by another options --
		 * they are error states and must always be active (ksb)
		 */
			Head(TokBody, pOR, fBodyAugment);
			fBodyAugment = 0;
			break;

		case TEVERY:
		case TLIST:
		case TLEFT:
		case TRIGHT:
		case TZERO:
		case TEXIT:
			Special(TokBody, & pOR->pORsact, fBodyAugment, pOR);
			fBodyAugment = 0;
			pOR->oattr |= OPT_ALTUSAGE|OPT_TRACK;
			break;

		case TAFTER:
			if (LCURLY == TokCur) {
				fprintf(stderr, "%s: %s(%d) a nested `after\' is used to sanity check input: it is not an action\n", progname, pchScanFile, iLine);
				exit(MER_SYNTAX);
			}
			CheckString("after");
			pOR->pchafter = TakeLine(& sLen);
			break;
		case TBEFORE:
			if (LCURLY == TokCur) {
				fprintf(stderr, "%s: %s(%d) nested `before\' is used as a statement inializer: it is not an action\n", progname, pchScanFile, iLine);
				exit(MER_SYNTAX);
			}
			CheckString("before");
			pOR->pchbefore = TakeLine(& sLen);
			break;
		case RCURLY:
			fprintf(stderr, "%s: %s(%d) too many right curlys?\n", progname, pchScanFile, iLine);
			break;
		case STRING:
			BrokenAug(TakeChars(& sLen), fBodyAugment, "in attribute list");
			continue;
		default:
			fprintf(stderr, "%s: %s(%d) syntax error in specification (%s)\n", progname, pchScanFile, iLine, TextTok(TokBody));
			if (TEOF == TokCur) {
				exit(MER_SYNTAX);
			}
			continue;
		}
		TokCur = GetTok();
	}
}

/* we found an option head (base type) grab the options			(ksb)
 */
static void
Head(TokBase, pORAllow, fAugment)
TOKEN TokBase;
OPTION *pORAllow;
int fAugment;
{
	register OPTION *pOR, **ppORList;
	register char *pcOpt;
	register OPTTYPE *pOTTest;
	auto char *pcGen;
	auto int nAttr, iMap;
	auto unsigned sLen;
	auto char *pcDim = (char *)0;

	pcGen = (char *)0;
	switch (TokBase) {
	default:
		fprintf(stderr, "%s: %s(%d) attempt to augment unknown construct\n", progname, pchScanFile, iLine);
		exit(MER_SYNTAX);
		/*NOTREACHED*/
	case TBOOLEAN:
		do {
			iMap = 'b';
			break;
	case TDOUBLE:
			iMap = 'd';
			break;
	case TINTEGER:
			iMap = 'i';
			break;
	case TUNSIGNED:
			iMap = 'u';
			break;
	case TLONG:
			iMap = 'l';
			break;
	case TCHARP:
			iMap = 'p';
			break;
	case TLETTER:
			iMap = 'c';
			break;
	case TPOINTER:
			iMap = 'v';
			pcDim = (char *)0;
			if (0) {
	case TACTION:
				iMap = 'a';
				pcDim = sbIBase;
			}
			if (0) {
	case TFUNCTION:
				iMap = 'f';
				pcDim = sbIBase;
			}
			if (0) {
	case TFD:
				iMap = 'D';
				pcDim = sbGenFd;
			}
			if (0) {
	case TFILE:
				iMap = 'F';
				pcDim = "r";
			}
			if (0) {
	case TACCUM:
				iMap = '+';
				pcDim = ":";
			}
			if (0) {
	case TSTRING:
				iMap = 's';
				pcDim = "";
			}
			if (0) {
	case TTYPE:
				iMap = ParseType();
				TokCur = GetTok();
				pcDim = (char *)0;
			}
			/* do we want a dim?
			 */
			if (LBRACKET == TokCur) {
				TokCur = GetTok();
				if (STRING == TokCur) {
					pcDim = TakeChars(& sLen);
					TokCur = GetTok();
				} else if (INTEGER == TokCur) {
					pcDim = TakeChars(& sLen);
					TokCur = GetTok();
				}
				if (RBRACKET != TokCur) {
					fprintf(stderr, "%s: %s(%d) missing right bracket in declaration\n", progname, pchScanFile, iLine);
					exit(MER_SYNTAX);
				}
				TokCur = GetTok();
			}
			break;
	case TTOGGLE:
			iMap = 't';
		} while (0);

		/* base-type type "name" ["client-key"] { attr-list }
		 */
		if (TTYPE == TokCur) {
			register char *pcTName;

			TokCur = GetTok();
			CheckString("type");
			pcTName = TakeChars(& sLen);
			TokCur = GetTok();
			if (sLen < 1) {
				fprintf(stderr, "%s: %s(%d) type names cannot be null\n", progname, pchScanFile, iLine);
				exit(MER_SEMANTIC);
			}
			pOR = FindVar(pcTName, pORType);
			ppORList = & pORType;
			/* three cases: not seen, forward, or seen
			 */
			pOTTest = CvtType(iMap);
			if (nilOR == pOR) {
				pOR = newopt(OPT_UNIQUE, ppORList, 1, OPT_GLOBAL|OPT_SYNTHTYPE);
				SynthNew(pOR, pOTTest, pcTName);
				if (fAugment)
					pOR->oattr |= OPT_AUGMENT;
			} else if (nilOT == pOR->pOTtype->pOTsbase) {
				pOR->pOTtype->pOTsbase = pOTTest;
				if (fAugment)
					pOR->oattr |= OPT_AUGMENT;
			} else if (pOTTest != pOR->pOTtype->pOTsbase) {
				fprintf(stderr, "%s: %s is derived from two different base types\n", progname, usersees(pOR, nil));
				fprintf(stderr, " %s and %s\n", pOTTest->pchlabel, pOR->pOTtype->pOTsbase->pchlabel);
				iExit |= MER_SEMANTIC;
			}
			if (TBASENAME == TokCur) {
				TokCur = GetTok();
				pOR->pOTtype->tattr |= _OPAQUE;
			}
			/* the client key list
			 */
			if (STRING == TokCur) {
				KeyDef(KV_CLIENT, fAugment, 2, 1, pOR, & pOR->pKVclient);
			} else {
				pOR->pKVclient = (KEY *)0;
			}
			/* Break one inv. to fix another, sigh:
			 * We break the synth type loop for a call here
			 * to confirm the augment rules.  Just for a few
			 * instructions....
			 */
			if (!fAugment) {
				pOTTest = pOR->pOTtype;
				pOR->pOTtype = nilOT;
			}
		/* depricate the "variable" keyword (use "named")
		 */
		} else if (TNAMED == TokCur || TVARIABLE == TokCur) {
			register char *pcVName;

			TokCur = GetTok();
			CheckString("named");
			pcVName = TakeChars(& sLen);
			TokCur = GetTok();

			if (sLen < 1) {
				fprintf(stderr, "%s: %s(%d) names cannot be null\n", progname, pchScanFile, iLine);
				exit(MER_SEMANTIC);
			}

			pOR = FindVar(pcVName, pORDecl);
			ppORList = & pORDecl;
			if (nilOR == pOR) {
				pOR = newopt(OPT_UNIQUE, ppORList, 1, OPT_GLOBAL|OPT_VARIABLE);
				pOR->pchname = pcVName;
			}
			/* implicit second parameter to `named' -- ksb
			 */
			if (STRING == TokCur) {
				pOR->pchkeep = TakeChars(& sLen);
				if (sLen < 1) {
					fprintf(stderr, "%s: %s(%d) keep names cannot be null\n", progname, pchScanFile, iLine);
					exit(MER_SEMANTIC);
				}
				TokCur = GetTok();
			}
			pOTTest = CvtType(iMap);
		} else {
			CheckString("a type");
			pcOpt = TakeChars(& sLen);
			TokCur = GetTok();
			if (sLen < 1) {
				fprintf(stderr, "%s: %s(%d) empty option name string\n", progname, pchScanFile, iLine);
				exit(MER_SEMANTIC);
			}
			if ((char *)0 != strchr(pcOpt, '-')) {
				fprintf(stderr, "%s: %s(%d) the option dash (-) can't ever happen due to dash-dash (--) being a delimiter\n", progname, pchScanFile, iLine);
			}
			nAttr = OPT_GLOBAL;
			while (0) {
	case TBADOPT:
				pcOpt = "?";
				iMap = 'a';
				nAttr = OPT_NOGETOPT|OPT_NOUSAGE;
				break;
	case TESCAPE:
				pcOpt = "+";
				iMap = 'p';
				nAttr = OPT_NOGETOPT|OPT_GLOBAL;
				break;
	case TNOPARAM:
				pcOpt = "*";
				iMap = 'a';
				nAttr = OPT_NOGETOPT|OPT_NOUSAGE;
				break;
	case TNUMBER:
				pcOpt = "#";
				iMap = '#';
				nAttr = OPT_GLOBAL;
				break;
	case TOTHERWISE:
				pcOpt = ":";
				iMap = 'a';
				nAttr = OPT_NOGETOPT|OPT_NOUSAGE;
			}
			if ('+' == pcOpt[0]) {
				if (STRING == TokCur) {
					pcGen = TakeChars(& sLen);
					TokCur = GetTok();
				} else {
					pcGen = "+";
				}
				SubType(& iMap, & pcDim);
			}
			pOTTest = CvtType(iMap);

			ppORList = & pORRoot;
			pOR = newopt(pcOpt[0], ppORList, 1, nAttr);
			MakeAliases(UNALIAS(pOR), pcOpt+1, ppORList, pOTTest);
		}

		AugCheck(pOR, pOTTest, fAugment, pcDim);
		pOR->pchgen = pcGen;
		if (nilOR != pORAllow) {
			if (nilOR != pOR->pORallow && pORAllow != pOR->pORallow) {
				fprintf(stderr, "%s: %s(%d) augmented enclosing  option doesn\'t match declared (%s", progname, pchScanFile, iLine, usersees(pOR->pORallow, nil));
				fprintf(stderr, "!= %s)\n", usersees(pORAllow, nil));
				exit(MER_SEMANTIC);
			}
			pOR->pORallow = pORAllow;
			pORAllow->oattr |= OPT_ENABLE|OPT_TRACK;
		}

		while (LCURLY != TokCur && SEMI != TokCur) {
			switch (TokCur) {
			case STRING:
				pcOpt = TakeChars(& sLen);
				if ((char *)0 != strchr(pcOpt, '-')) {
					fprintf(stderr, "%s: %s(%d) the option dash (-) can't ever happen due to dash-dash (--) being a delimiter\n", progname, pchScanFile, iLine);
				}
				MakeAliases(UNALIAS(pOR), pcOpt, ppORList, pOR->pOTtype);
				free(pcOpt);
				break;
			case TBADOPT:
				MakeAliases(UNALIAS(pOR), "?", ppORList, pOR->pOTtype);
				break;
			case TESCAPE:
				MakeAliases(UNALIAS(pOR), "+", ppORList, pOR->pOTtype);
				break;
			case TNOPARAM:
				MakeAliases(UNALIAS(pOR), "*", ppORList, pOR->pOTtype);
				break;
			case TNUMBER:
				MakeAliases(UNALIAS(pOR), "#", ppORList, pOR->pOTtype);
				break;
			case TOTHERWISE:
				MakeAliases(UNALIAS(pOR), ":", ppORList, pOR->pOTtype);
				break;
			default:
				fprintf(stderr, "%s: %s(%d) strange noise (%s) before left curly brace definition of %s\n", progname, pchScanFile, iLine, TextTok(TokCur), usersees(pOR, nil));
				exit(MER_SYNTAX);
			}
			TokCur = GetTok();
		}
		break;
	}
	Body(pOR, fAugment);
}

/* add a special option push for 0 == strcmp(progname, pcSpecial)	(ksb)
 * We hide the options after the end of the key string:
 *	keyword \000 options \000
 */
void
AddBasePair(pcSpecial, pcOptions)
char *pcSpecial, *pcOptions;
{
	static int iNumBase = 0, iMaxBase = 0;
	register unsigned l;
	register char *pc;

	if ((char *)0 == pcSpecial) {
		if ((char *)0 != pcDefBaseOpt) {
			fprintf(stderr, "%s: %s(%d) default basename options given more than once\n", progname, pchScanFile, iLine);
			exit(MER_SEMANTIC);
		}
		pcDefBaseOpt = pcOptions;
		return;
	}
	if (iNumBase+1 >= iMaxBase) {
		if ((char **)0 == ppcBases) {
			iMaxBase = 10;
			ppcBases = (char **)calloc(iMaxBase, sizeof (char *));
		} else {
			iMaxBase += 20;
			ppcBases = (char **)realloc(ppcBases, iMaxBase * sizeof (char *));
		}
		if ((char **)0 == ppcBases) {
			fprintf(stderr, acOutMem, progname);
			exit(MER_LIMIT);
		}
	}
	l = strlen(pcSpecial)+1;
	pc = malloc(l+strlen(pcOptions)+1);
	if ((char *)0 == pc) {
		fprintf(stderr, acOutMem, progname);
		exit(MER_LIMIT);
	}
	ppcBases[iNumBase++] = strcpy(pc, pcSpecial);
	(void)strcpy(pc+l, pcOptions);
	ppcBases[iNumBase] = (char *)0;
}

/* parse the key definition stuff					(ksb)
 * [augment] key "name" 'letter-opt' [ initializer ] [once] [base-key] {
 *	[skip] "value1" [skip] "value2" [skip] ...
 * }
 * [augment] key "Filter" { "myFilter" }
 * If SpecOnly is set just parse up to the open curly brace.
 * fDefInit values 0 (not), 1 (if unnamed), 2 always
 */
static KEY *
KeyDef(iKeyType, fKeyAugment, fDefInit, fSpecOnly, pORAllow, ppKVInstall)
int iKeyType, fKeyAugment, fDefInit, fSpecOnly;
OPTION *pORAllow;
KEY **ppKVInstall;
{
	register char *pcName, *pcTmp, **ppcCheck, *pcKeyVer;
	register int cKey, fInit, fOnce;
	register TOKEN TokKey;
	register KEY *pKVThis, *pKVInit;
	register LIST *pLI;
	auto LIST_INDEX wCur, wAdd, wHas;
	auto unsigned sLen;

	if (STRING == TokCur) {
		CheckString("key");
		pcName = TakeChars(& sLen);
		if (0 == sLen) {
			fprintf(stderr, "%s: %s(%d) keys must not be the empty string\n", progname, pchScanFile, iLine);
			pcName = "u_bogus";
		}
		TokCur = GetTok();
		/* Default init for unnamed key, but not named ones
		 */
		if (1 == fDefInit) {
			fDefInit = 0;
		}
	} else {
		pcName = (char *)0;
	}

	/* get short name and version number
	 * (in the if below pcName cannot be nil).
	 */
	cKey = KEY_NOSHORT;
	if (STRING == TokCur) {
		pcTmp = UseChars(& sLen);
		if (1 != strlen(pcTmp)) {
			fprintf(stderr, "%s: %s(%d) key \"%s\" alias ('%s') must be a single character (length %d)\n", progname, pchScanFile, iLine, pcName, pcTmp, strlen(pcTmp));
		} else {
			cKey = pcTmp[0];
		}
		TokCur = GetTok();
	}
	if (INTEGER == TokCur) {
		pcKeyVer = TakeChars(& sLen);
		TokCur = GetTok();
	} else {
		pcKeyVer = (char *)0;
	}

	/* get initial and once flags
	 */
	pKVInit = (KEY *)0;
	if (0 != (fInit = (TINITIALIZER == TokCur))) {
		TokCur = GetTok();
		if (STRING == TokCur) {
			pcTmp = TakeChars(& sLen);
			if (0 == sLen) {
				fprintf(stderr, "%s: %s(%d) key initializer cannot be the empty string\n", progname, pchScanFile, iLine);
			} else {
				pKVInit = KeyFind(pcTmp, KEY_NOSHORT, 1);
			}
			TokCur = GetTok();
		}
	}
	fInit |= fDefInit;
	if (0 != (fOnce = (TONCE == TokCur))) {
		TokCur = GetTok();
	}
	pKVThis = KeyAttach(iKeyType, pORAllow, pcName, cKey, ppKVInstall);
	if ((char *)0 == pcName) {
		pcName = "unnamed";
	}
	if (fInit) {
		if ((char *)0 != pKVThis->pcowner && 0 != strcmp(pchScanFile, pKVThis->pcowner)) {
			fprintf(stderr, "%s: %s(%d) API key %s: owner %s(%d) also declared\n", progname, pchScanFile, iLine, pcName, pKVThis->pcowner, pKVThis->uoline);
		}
		pKVThis->pcowner = strdup(pchScanFile);
		pKVThis->uoline = (unsigned)iLine;
		if (fOnce) {
			pKVThis->wkattr |= KV_V_ONCE;
		}
	} else if ((char *)0 == pKVThis->pcclient) {
		pKVThis->pcclient = strdup(pchScanFile);
		pKVThis->ucline = (unsigned)iLine;
		if (fOnce) {
			pKVThis->wkattr |= KV_A_ONCE;
		}
	}
	if ((char *)0 == pKVThis->pcversion) {
		pKVThis->pcversion = pcKeyVer;
	} else if ((char *)0 == pcKeyVer) {
		/* nothing to do */
	} else if (atol(pcKeyVer) != atol(pKVThis->pcversion)) {
		fprintf(stderr, "%s: %s(%d) API key %s: version mismatch (%s != %s)\n", progname, pchScanFile, iLine, pcName, pcKeyVer, pKVThis->pcversion);
		if ((char *)0 != pKVThis->pcowner) {
			fprintf(stderr, " see %s\n", pKVThis->pcowner);
		}
	}
	if (nilOR == pKVThis->pORin) {
		pKVThis->pORin = pORAllow;
	} else if (nilOR == pORAllow) {
		/* ignore top level diddle */
	} else if (pKVThis->pORin != pORAllow) {
		fprintf(stderr, "%s: %s(%d) API key %s: owner mismatch (%s", progname, pchScanFile, iLine, pcName, usersees(pKVThis->pORin, nil));
		fprintf(stderr, " != %s)\n", usersees(pORAllow, nil));
	}
	if ((KEY *)0 == pKVInit) {
		/* ignore -- none given */
	} else if ((KEY *)0 == pKVThis->pKVinit) {
		pKVThis->pKVinit = pKVInit;
	} else if (pKVThis->pKVinit != pKVInit) {
		fprintf(stderr, "%s: %s(%d) %s: cannot have more than one key init\n", progname, pchScanFile, iLine, pcName);
	}

	/* just a version/existance check
	 */
	if (SEMI == TokCur || fSpecOnly) {
		return pKVThis;
	}

	/* read values and diddle unique list
	 */
	if (LCURLY != TokCur) {
		fprintf(stderr, "%s: %s(%d) left curly brace missing for key definition (at %s)\n", progname, pchScanFile, iLine, TextTok(TokCur));
		exit(MER_SYNTAX);
	}
	TokCur = GetTok();

	/* augments to key add to a defered integration list
	 */
	if (fKeyAugment) {
		pLI = & pKVThis->LIaugments;
		(void)ListCur(pLI, & wCur);
		pKVThis->wkattr |= KV_AUGMENT;
	} else {
		pLI = & pKVThis->LIinputs;
		wCur = 0;
	}
	while (RCURLY != (TokKey = TokCur)) {
		ppcCheck = ListCur(pLI, & wHas);
		switch (TokKey) {
		case INTEGER:	/* how many to skip before next */
			/* get the current size, +1 for each
			 * slot build, add a nil pointer for those
			 * unborn ones
			 */
			pcTmp = UseChars(& sLen);
			for (wAdd = atoi(pcTmp); wAdd > 0; --wAdd) {
				++wCur;
				if (wCur >= wHas) {
					++wHas;
					ListAdd(pLI, (char *)0);
				}
			}
			break;
		case STRING:	/* next key value */
			if (wCur == wHas) {
				ListAdd(pLI, TakeChars(& sLen));
				++wHas;
			} else if (!fInit || (char *)0 == ppcCheck[wCur]) {
				ListReplace(pLI, TakeChars(& sLen), wCur);
			} else {
				/* ignore unused init string */
			}
			++wCur;
			break;
		default:
			fprintf(stderr, "%s: %s(%d) key %s: unknown syntax (%s)\n", progname, pchScanFile, iLine, pcName, TextTok(TokKey));
			break;
		}
		TokCur = GetTok();
	}
	return pKVThis;
}

/* parse the file level stuff						(ksb)
 * if nilOR != pORAllow we are inside another option, yeah!
 */
static void
File(pORAllow)
OPTION *pORAllow;
{
	register TOKEN TokLoc;
	auto unsigned sLen;
	auto char *pcSpecBase;
	auto int fFileAugment;

	fFileAugment = 0;
	while (TEOF != TokCur) {
		if (fFileAugment) {
			fprintf(stderr, "%s: %s(%d) augment what?\n", progname, pchScanFile, iLine);
		}
		TokLoc = TokCur;
		TokCur = GetTok();
		if ((fFileAugment = (TAUGMENT == TokLoc))) {
			TokLoc = TokCur;
			TokCur = GetTok();
		}

		switch (TokLoc) {
		case TEXCLUDES:
			CheckString("excludes");
			do {
				ListAdd(& LIExcl, struniq(TakeChars(& sLen)));
				TokCur = GetTok();
			} while (STRING == TokCur);
			break;
		case TFROM:
			CheckString("from");
			do {
				register char *pc;
				pc = TakeChars(& sLen);
				if (0 != sLen)
					ListAdd(& LIUserIncl, pc);
				TokCur = GetTok();
			} while (STRING == TokCur);
			break;
		case TBASENAME:
			fBasename = 1;
			if (STRING == TokCur) {
				pcSpecBase = TakeChars(& sLen);
			} else if (TOTHERWISE == TokCur) {
				pcSpecBase = (char *)0;
			} else {
				break;
			}
			TokCur = GetTok();
			if (STRING != TokCur) {
				if ((char *)0 == pcSpecBase) {
					Head(TOTHERWISE, pORAllow, 0);
					TokCur = GetTok();
					break;
				}
				fprintf(stderr, "%s: %s(%d) what options should the special name `%s' set?\n", progname, pchScanFile, iLine, pcSpecBase);
				exit(MER_SEMANTIC);
			}
			if ((char *)0 == pcSpecBase || '\000' == pcSpecBase[0]) {
				pcSpecBase = (char *)0;
			}
			AddBasePair(pcSpecBase, TakeChars(& sLen));
			TokCur = GetTok();
			break;
		case TCOMMENT:
			CheckString("comment");
			do {
				ListAdd(& LIComm, TakeChars(& sLen));
				TokCur = GetTok();
			} while (STRING == TokCur);
			break;
		case TCLEANUP:
			TodoList("cleanup", aLIExits);
			break;
		case TINITIALIZER:
			TodoList("initializer", aLIInits);
			break;
		case TKEY:	/* named key at top level */
			if (TBASENAME != TokCur) {
				KeyDef(KV_NAMED, fFileAugment, 0, 0, pORAllow, (KEY **)0);
				TokCur = GetTok();
				fFileAugment = 0;
				break;
			}
			TokCur = GetTok();
			CheckString("key basename");
			pcKeyArg = TakeChars(& sLen);
			TokCur = GetTok();
			if (STRING == TokCur) {
				AddBasePair(pcKeyArg, TakeChars(& sLen));
				TokCur = GetTok();
			}
			break;
		case TROUTINE:
			CheckString("routine");
			pchMain = TakeChars(& sLen);
			TokCur = GetTok();
			break;
		case TGETENV:
			fGetenv = 1;
			if (STRING == TokCur) {
				pchGetenv = TakeChars(& sLen);
				TokCur = GetTok();
			}
			break;
		case TMIX:
			fMix = 1;
			break;
		case TNAMED:
			CheckString("named");
			pchProgName = TakeChars(& sLen);
			TokCur = GetTok();
			break;
		case TPROTOTYPE:
			CheckString("prototype");
			pchProto = TakeChars(& sLen);
			TokCur = GetTok();
			break;
		case TREQUIRES:
			CheckString("requires");
			do {
				MetaRequire(TakeChars(& sLen));
				TokCur = GetTok();
			} while (STRING == TokCur);
			break;
		case TTEMPLATE:
			CheckString("template");
			pchTempl = TakeChars(& sLen);
			TokCur = GetTok();
			break;
		case TVECTOR:
			CheckString("vector");
			pchVector = TakeChars(& sLen);
			TokCur = GetTok();
			break;
		case TUSAGE:
			CheckString("usage");
			pchUsage = TakeChars(& sLen);
			TokCur = GetTok();
			break;
		case TTERSE:
			CheckString("terse");
			pchTerse = TakeChars(& sLen);
			TokCur = GetTok();
			break;

		case TAFTER:
		case TBEFORE:
			if (STRING == TokCur || INTEGER == TokCur) {
				if (TAFTER == TokLoc) {
					TodoList("after", aLIAfters);
				} else {
					TodoList("initializer", aLIInits);
				}
				break;
			}
			/* fallthough */
		case TLIST:
		case TLEFT:
		case TRIGHT:
		case TEVERY:
		case TZERO:
		case TEXIT:
			Special(TokLoc, & pORActn, fFileAugment, pORAllow);
			TokCur = GetTok();
			fFileAugment = 0;
			break;

		case TACTION:
		case TBOOLEAN:
		case TDOUBLE:
		case TFUNCTION:
		case TINTEGER:
		case TUNSIGNED:
		case TLONG:
		case TCHARP:
		case TPOINTER:
		case TLETTER:
		case TFD:
		case TFILE:
		case TACCUM:
		case TSTRING:
		case TTOGGLE:
		case TESCAPE:
		case TNUMBER:
		case TTYPE:
		case TBADOPT: /* plus the top only ones */
		case TNOPARAM:
		case TOTHERWISE:
			Head(TokLoc, pORAllow, fFileAugment);
			TokCur = GetTok();
			fFileAugment = 0;
			break;

		case SEMI:
			fprintf(stderr, "%s: %s(%d) extra semicolon\n", progname, pchScanFile, iLine);
			break;

		case STRING:
			BrokenAug(TakeChars(& sLen), fFileAugment, "at top level");
			continue;
		default:
			fprintf(stderr, "%s: %s(%d) syntax error, unrecognized declaration (... %s %s)\n", progname, pchScanFile, iLine, TextTok(TokLoc), TextTok(TokCur));
			exit(MER_SYNTAX);
			/*NOTREACHED*/
		}
		if (SEMI == TokCur) {
			TokCur = GetTok();
		}
	}
}

static char
	acCTmp[48+1] = "/tmp/mkscXXXXXX",
	acHTmp[48+1] = "/tmp/mkshXXXXXX",
	acTTmp[48+1] = "/tmp/mksiXXXXXX";

static LIST
	LIDelete,		/* files to delete on exit		*/
	LIMetaM,		/* mkcmd input modules we need		*/
	LIMetaC,		/* meta C code files to include		*/
	LIMetaI,		/* meta include files to include	*/
	LIMetaH;		/* meta header files to include		*/
static KEY
	*pKVOptions,		/* options to mkcmd itself		*/
	*pKVGetopt;		/* options to getopt.m			*/

/* Yeah, we need some init code -- could go in main			(ksb)
 * we put which version of mkcmd wrote the file in the comments
 * and build the diversion buffers
 */
void
Init(pcVersion)
char *pcVersion;
{
	register int i;

	fPSawList = fPSawEvery = fPSawZero = 0;
	ListInit(& LIUserIncl, 1);
	ListInit(& LISysIncl, 1);
	ListInit(& LIExcl, 1);
	ListInit(& LIComm, 0);
	for (i = 0; i < 11; ++i) {
		ListInit(& aLIExits[i], 0);
		ListInit(& aLIInits[i], 0);
		ListInit(& aLIAfters[i], 0);
	}

	/* our mkcmd source and delayed C, include, and header meta code
	 */
	ListInit(& LIDelete, 1);
	ListInit(& LIMetaM, 1);
	ListInit(& LIMetaC, 1);
	ListInit(& LIMetaI, 1);
	ListInit(& LIMetaH, 1);

	EmitInit(pcVersion);
	BuiltinTypes();
	(void)ListAdd(& LIComm, "%cbuilt by %W version %V");

	/* key "-mkcmd" '-' init { $fAnsi $fWeGuess $fCppLines }
	 */
	(void)KeyAttach(KV_NAMED, nilOR, "-mkcmd", '-', & pKVOptions);
	pKVOptions->pcowner = progname;
	pKVOptions->uoline = 0;
	pKVOptions->pcversion = "8";
	ListAdd(& pKVOptions->LIinputs, BOOL_STR(fAnsi));    /* [1] fAnsi */
	ListAdd(& pKVOptions->LIinputs, BOOL_STR(fWeGuess)); /* [2] fWeGuess */
	ListAdd(& pKVOptions->LIinputs, BOOL_STR(fCppLines));/* [3] fCppLines */

	/* key ".mkcmd" '.' init { envopt_used getarg_used getopt_used }
	 */
	(void)KeyAttach(KV_NAMED, nilOR, ".mkcmd", '.', & pKVGetopt);
	pKVGetopt->pcowner = progname;
	pKVGetopt->uoline = 0;
	pKVGetopt->pcversion = "8";
	ListAdd(& pKVGetopt->LIinputs, (char *)0);	/* [1] envopt */
	ListAdd(& pKVGetopt->LIinputs, (char *)0);	/* [2] getarg */
	ListAdd(& pKVGetopt->LIinputs, (char *)0);	/* [3] getopt */

	/* key "mkcmd_declares" init once { [type-names] }
	 */
	(void)KeyAttach(KV_NAMED, nilOR, "mkcmd_declares", KEY_NOSHORT, & pKVDeclare);
	pKVDeclare->pcowner = progname;
	pKVDeclare->uoline = 0;
	pKVDeclare->pcversion = "8";
	pKVDeclare->wkattr |= KV_V_ONCE;

	/* key "mkcmd_verifies" init once { [type-names] }
	 */
	(void)KeyAttach(KV_NAMED, nilOR, "mkcmd_verifies", KEY_NOSHORT, & pKVVerify);
	pKVVerify->pcowner = progname;
	pKVVerify->uoline = 0;
	pKVVerify->pcversion = "8";
	pKVVerify->wkattr |= KV_V_ONCE;

	(void)mktemp(acCTmp);
	if ((FILE *)0 == (fpDivert = fopen(acCTmp, "w+"))) {
		fprintf(stderr, acFailOpen, progname, acCTmp, strerror(errno));
		exit(MER_BROKEN);
	}
	(void)ListAdd(& LIDelete, acCTmp);
	(void)mktemp(acHTmp);
	if ((FILE *)0 == (fpHeader = fopen(acHTmp, "w+"))) {
		fprintf(stderr, acFailOpen, progname, acHTmp, strerror(errno));
		exit(MER_BROKEN);
	}
	(void)ListAdd(& LIDelete, acHTmp);
	(void)mktemp(acTTmp);
	if ((FILE *)0 == (fpTop = fopen(acTTmp, "w+"))) {
		fprintf(stderr, acFailOpen, progname, acTTmp, strerror(errno));
		exit(MER_BROKEN);
	}
	(void)ListAdd(& LIDelete, acTTmp);
}

/* copy a file by name into a buffer file				(ksb)
 */
static void
TransCp(fpOut, pcIn)
FILE *fpOut;
char *pcIn;
{
	register FILE *fpTemp;
	register int n;
	auto char acBuf[8192];
	auto int (*pfiClose)();

	if ((FILE *)0 == (fpTemp = Search(pcIn, "r", & pfiClose))) {
		fprintf(stderr, acFailOpen, progname, pcIn, strerror(errno));
		return;
	}
	fprintf(fpOut, "#line 1 \"%s\"\n", pcIn);
	while (0 < (n = fread(acBuf, sizeof(char), sizeof(acBuf), fpTemp))) {
		if (n == fwrite(acBuf, sizeof(char), n, fpOut)) {
			continue;
		}
		exit(MER_BROKEN);
	}
	(void)fflush(fpOut);
	pfiClose(fpTemp);
}

/* Find #inclue lines in the file, add them to the UserIncl list,	(ksb)
 * copy the rest the the output file.
 * ANSI C says no C line can be more than 509 characters + cr + lf + \000
 */
static void
RpcInclude(pcCmd, fpOut)
char *pcCmd;
FILE *fpOut;
{
	static char acMatch[] = " # include ";
	register FILE *fpFile;
	register int c;
	register char *pcState, *pcCursor;
	auto char acBuf[MAXPATHLEN+512]; /* <file.h>\0 */

	if ((FILE *)0 == (fpFile = popen(pcCmd, "r"))) {
		fprintf(stderr, "%s: popen: %s: %s\n", progname, pcCmd, strerror(errno));
		return;
	}

	pcState = acMatch;
	pcCursor = acBuf;
	while (EOF != (c = getc(fpFile))) {
		if ((char *)0 == pcState) {
			putc(c, fpOut);
			if ('\n' == c) {
				pcState = acMatch;
				pcCursor = acBuf;
			}
			continue;
		}
		/* finished a line ?
		 */
		if ('\n' == c) {
			*pcCursor = '\000';
			if ('\000' == *pcState) {
				register char *pcEnd;
				/* silly FreeBSD puts a comment on
				 * the end of some includes, sigh. --ksb
				 */
				for (pcEnd = acBuf; '\000' != *pcEnd; ++pcEnd) {
					if ('/' != pcEnd[0] || '*' != pcEnd[1])
						continue;
					*pcEnd = '\000';
					break;
				}
				while (pcEnd > acBuf && isspace(pcEnd[-1]))
					--pcEnd;
				if (isspace(*pcEnd))
					*pcEnd = '\000';
				/* don't add our header, or empty strings
				 */
				if ('"' != acBuf[0] && '\000' != acBuf[0])
					ListAdd(& LISysIncl, strdup(acBuf));
			} else {
				fprintf(fpOut, "%s\n", acBuf);
			}
			pcState = acMatch;
			pcCursor = acBuf;
			continue;
		}
		*pcCursor++ = c;
 xlate:
		switch (*pcState) {
		case '\000':
			continue;
		case '\t':
		case ' ':
			if (isspace(c)) {
				continue;
			}
			if ('\000' == *++pcState) {
				pcCursor = acBuf;
				*pcCursor++ = c;
			}
			goto xlate;
		default:
			if (*pcState != c) {
				*pcCursor = '\000';
				fprintf(fpOut, "%s", acBuf);
				pcState = (char *)0;
				pcCursor = acBuf;
				break;
			}
			if ('\000' == *++pcState) {
				pcCursor = acBuf;
			}
			break;
		}
	}
	if (pcCursor != acBuf) {
		*pcCursor = '\000';
		fprintf(fpOut, "%s", acBuf);
	}
	pclose(fpFile);
}

/* Ask for an RPC source file to be included				(ksb)
 * We can take (to mock explode kinda)
 *	file.x	    rpcgen the whole thing (add all below)
 *	file-xdr.x  rpcgen -c file.x; include file_xdr.c & file.i
 *	file-svc.x  rpcgen -m; include file_svc.c & file.i
 *	file-clnt.x rpcgen -l; include file_clnt.c & file.i
 *	file-hdr.x  rpcgen -h; include file.h & file.i
 *	file-inc.x  rpcgen -h; include file.h & file.i
 * any of those gets the file.i, but only the first one (once)
 * any raw #includes in any file go into the UserInclude list for generation
 *
 * Default ("ALL") to a client only implementation, viz. file-inc,server,xdr.x
 */
#define RGEN_PRINT	0x40	/* @ ascii pad */
#define RGEN_HDR	0x20	/* -h */
#define RGEN_INC	0x10	/* -h in include section, always */
#define RGEN_XDR	0x01	/* -c */
#define RGEN_SVC	0x02	/* -m */
#define RGEN_CLNT	0x04	/* -l */
#define RGEN_TBL	0x08	/* -t */
#define RGEN_ALL	(RGEN_XDR|RGEN_CLNT|RGEN_INC)
void
RpcFile(pcAsk)
char *pcAsk;
{
	static LIST LIRpc;	/* rpc file.x's we've seen	*/
	static char bInitRpc = 0;
	register char *pcDash, *pcNext, **ppcList;
	register FILE *fpSource, *fpDest;
	register int n, iGen;
	auto char acTFilex[MAXPATHLEN+4], acDupToken[48];
	auto char acBuf[4096+MAXPATHLEN];
	auto struct stat stSeen;
	auto LIST_INDEX uEnd;
	auto int (*pfiClose)();

	if (!bInitRpc) {
		ListInit(& LIRpc, 1);
		bInitRpc = 1;
	}
	(void)strcpy(acTFilex, "/tmp/mkdrXXXXXX");
	(void)mktemp(acTFilex);
	(void)strcat(acTFilex, ".x");

	fpSource = (FILE *)0;
	if ((char *)0 != (pcDash = strrchr(pcAsk, '-'))) {
		auto char acSave[2];

		acSave[0] = pcDash[1];
		acSave[1] = pcDash[2];
		pcDash[0] = '.';
		pcDash[1] = 'x';
		pcDash[2] = '\000';
		fpSource = Search(pcAsk, "r", & pfiClose);
		pcDash[0] = '-';
		pcDash[1] = acSave[0];
		pcDash[2] = acSave[1];
		++pcDash;
	}
	/* try search with explode options, almost never what you want
	 */
	if ((FILE *)0 == fpSource) {
		pcDash = (char *)0;
		fpSource = Search(pcAsk, "rb", & pfiClose);
	}
	if ((FILE *)0 == fpSource) {
		fprintf(stderr, "%s: fopen: %s: %s\n", progname, pcAsk, strerror(errno));
		exit(MER_LIMIT);
	}

	/* see what to generate, all by default
	 */
	for (iGen = 0; (char *)0 != pcDash; pcDash = pcNext) {
		if ((char *)0 != (pcNext = strchr(pcDash, ',')))
			++pcNext;
		if (0 == strncmp(pcDash, "clnt", 4) || 0 == strncmp(pcDash, "client", 6)) {
			iGen |= RGEN_CLNT;
		}
		if (0 == strncmp(pcDash, "svc", 3) || 0 == strncmp(pcDash, "server", 6)) {
			iGen |= RGEN_SVC;
		}
		if (0 == strncmp(pcDash, "hdr", 3) || 0 == strncmp(pcDash, "header", 6)) {
			iGen |= RGEN_HDR;
		}
		if (0 == strncmp(pcDash, "xdr", 3) || 0 == strncmp(pcDash, "XDR", 3)) {
			iGen |= RGEN_XDR;
		}
		if (0 == strncmp(pcDash, "inc", 3) || 0 == strncmp(pcDash, "include", 7)) {
			iGen |= RGEN_INC;
		}
		/* should we take "dispatch" as well?
		 */
		if (0 == strncmp(pcDash, "tbl", 3) || 0 == strncmp(pcDash, "table", 5)) {
			iGen |= RGEN_TBL;
		}
	}

	/* have we seen this before?  We do this to avoid multiple xdrs
	 */
	if (-1 == fstat(fileno(fpSource), & stSeen)) {
		fprintf(stderr, "%s: fstat: %d: %s\n", progname, fileno(fpSource), strerror(errno));
		exit(MER_LIMIT);
	}
	(void)sprintf(acDupToken, "%x/%x", stSeen.st_dev, stSeen.st_ino);
	ppcList = ListCur(& LIRpc, & uEnd);
	pcDash = (char *)0;
	while (uEnd > 0) {
		--uEnd;
		if (0 == strcmp(ppcList[uEnd], acDupToken))
			continue;
		pcDash = ppcList[uEnd];
		break;
	}

	/* remember the stuff we did in a bit vector after the string,
	 * or recall what we did and don't do it again
	 */
	if ((char *)0 == pcDash) {
		register char *pcMem;

		pcMem = calloc((strlen(acDupToken)|3)+5, 1);
		if ((char *)0 == pcMem) {
			fprintf(stderr, acOutMem, progname);
			exit(MER_LIMIT);
		}
		ListAdd(& LIRpc, strcpy(pcMem, acDupToken));
		pcDash = pcMem + strlen(pcMem);
		pcDash[1] = RGEN_PRINT;
		pcDash[2] = '\000';
	} else {
		pcDash += strlen(pcDash);
		iGen &= ~pcDash[1];
	}

	/* We won't do anything we did last time again (and don't do this
	 * next time either).
	 * If you asked for nothing default to most everything
	 * if you asked for code but no header get local headers
	 * for forward defines
	 */
	n = (iGen | pcDash[1]) & ~RGEN_PRINT;
	if (0 == n) {
		iGen = RGEN_ALL;
	}
	/* Presumptuous of us, but it usually works out:		(ksb)
	 * when they asked for "xdr" but not a header we have to include
	 * the header data to get the types, or the C compiler chokes.
	 * Most rpcgen's protect the typedefs with a #ifdef trap, anyway.
	 */
	if (0 == (n & (RGEN_INC|RGEN_HDR))) {
		iGen |= RGEN_INC;
	}
	pcDash[1] |= iGen;

	/* copy the rpcgen source file to /tmp, some rpcgens won't take stdin
	 */
	if ((FILE *)0 == (fpDest = fopen(acTFilex, "wb"))) {
		fprintf(stderr, "%s: fopen: %s: %s\n", progname, acTFilex, strerror(errno));
		exit(MER_LIMIT);
	}
	while (0 < (n = fread(acBuf, sizeof(char), sizeof(acBuf), fpSource))) {
		if (n == fwrite(acBuf, sizeof(char), n, fpDest)) {
			continue;
		}
		exit(MER_BROKEN);
	}
	(void)fclose(fpDest);
	(*pfiClose)(fpSource);

	/* pump out the code
	 */
	if (0 != (RGEN_HDR & iGen)) {
		sprintf(acBuf, "rpcgen -h %s", acTFilex);
		RpcInclude(acBuf, fpHeader);
	}
	if (0 != (RGEN_INC & iGen)) {
		sprintf(acBuf, "rpcgen -h %s", acTFilex);
		RpcInclude(acBuf, fpTop);
	}
	if (0 != (RGEN_TBL & iGen)) {
		sprintf(acBuf, "rpcgen -t %s", acTFilex);
		RpcInclude(acBuf, fpTop);
	}
	if (0 != (RGEN_XDR & iGen)) {
		sprintf(acBuf, "rpcgen -c %s", acTFilex);
		RpcInclude(acBuf, fpDivert);
	}
	if (0 != (RGEN_CLNT & iGen)) {
		sprintf(acBuf, "rpcgen -l %s", acTFilex);
		RpcInclude(acBuf, fpDivert);
	}
	if (0 != (RGEN_SVC & iGen)) {
		sprintf(acBuf, "rpcgen -m %s", acTFilex);
		RpcInclude(acBuf, fpDivert);
	}

	/* cleanup and go back to mkcmd
	 */
	(void)unlink(acTFilex);
}

/* figure out which list a filename belongs on				(ksb)
 */
static void
MetaRequire(pcConfig)
char *pcConfig;
{
	register char *pcDot;

	if ((char *)0 != (pcDot = strrchr(pcConfig, '.'))) {
		if (0 == strcmp(pcDot+1, "mc")) {
			ListAdd(& LIMetaC, pcConfig);
			return;
		}
		if (0 == strcmp(pcDot+1, "mi")) {
			ListAdd(& LIMetaI, pcConfig);
			return;
		}
		if (0 == strcmp(pcDot+1, "mh")) {
			ListAdd(& LIMetaH, pcConfig);
			return;
		}
		if (0 == strcmp(pcDot+1, "c")) {
			TransCp(fpDivert, pcConfig);
			return;
		}
		if (0 == strcmp(pcDot+1, "h")) {
			TransCp(fpHeader, pcConfig);
			return;
		}
		if (0 == strcmp(pcDot+1, "i")) {
			TransCp(fpTop, pcConfig);
			return;
		}
		if (0 == strcmp(pcDot+1, "x")) {
			RpcFile(pcConfig);
			return;
		}
	}
	ListAdd(& LIMetaM, pcConfig);
}

/* read in a file, make option records					(ksb)
 * or delay it to the meta code processor
 */
void
Config(pcConfig)
char *pcConfig;
{
	register FILE *fp;
	register char **ppcList, *pcNext;
	auto LIST_INDEX uCur, uEnd;
	auto int (*pfiClose)();

	(void)ListCur(& LIMetaM, & uCur);
	if ((char *)0 == pcConfig || ('-' == pcConfig[0] && '\000' == pcConfig[1])) {
		SetFile(stdin, "stdin");
		TokCur = GetTok();
		File(nilOR);
		fseek(stdin, 0L, 1);	/* try to clear EOF */
	} else {
		MetaRequire(pcConfig);
	}

	for (;;) {
		ppcList = ListCur(& LIMetaM, & uEnd);
		if (uEnd == uCur) {
			return;
		}
		pcNext = ppcList[uCur++];

		if ((FILE *)0 == (fp = Search(pcNext, "r", & pfiClose))) {
			fprintf(stderr, acFailOpen, progname, pcNext, strerror(errno));
			return;
		}
		SetFile(fp, pcNext);
		TokCur = GetTok();
		File(nilOR);
		pfiClose(fp);
	}
}

/* someone used a type they didn't define, look for it before		(ksb)
 * we give up and fail the type resolution.  This might cross that
 * infamous fine line a little.
 *	type "foo"   -->   require "foo.m"
 */
int
TypeRequire(pcType)
char *pcType;
{
	auto char acTail[MAXPATHLEN+2];

	(void)sprintf(acTail, "%s.m", pcType);
	Config(strdup(acTail));
	return 0;
}

/* find the files to pump to the meta emit stage			(ksb)
 */
void
GenMeta(fp, pLI)
FILE *fp;
LIST *pLI;
{
	register char **ppcList, *pcDelay;
	register FILE *fpMeta;
	auto LIST_INDEX u;
	auto int (*pfiClose)();

	for (ppcList = ListCur(pLI, & u); u > 0; --u) {
		pcDelay = *ppcList++;
		if ((FILE *)0 == (fpMeta = Search(pcDelay, "r", & pfiClose))) {
			fprintf(stderr, "%s: fopen: %s: %s\n", progname, pcDelay, strerror(errno));
			continue;
		}
		MetaEmit(fp, nilOR, fpMeta, pcDelay);
		pfiClose(fpMeta);
	}
}

/* tell the user the names of the files in the list on fp		(ksb)
 * (called with "fMaketd" set only).
 */
static void
DepMeta(pLI)
LIST *pLI;
{
	register char **ppcList;
	register FILE *fpMeta;
	register char *pcDelay;
	auto LIST_INDEX u;
	auto int (*pfiClose)();

	for (ppcList = ListCur(pLI, & u); u > 0; --u) {
		pcDelay = *ppcList++;
		if ((FILE *)0 == (fpMeta = Search(pcDelay, "r", & pfiClose))) {
			fprintf(stderr, "%s: fopen: %s: %s\n", progname, pcDelay, strerror(errno));
			iExit |= MER_BROKEN;
		}
		pfiClose(fpMeta);
	}
}

/* base may insert option letter					(ksb)
 * if any of the basenames given can call u_envopt we return 1, else 0
 */
static int
BaseMayInsert(ppcList)
char **ppcList;
{
	register char *pcDo;
	register int l;

	if ((char **)0 == ppcList)
		return 0;
	while ((char *)0 != (pcDo = *ppcList++)) {
		l = strlen(pcDo);
		if ('\000' != pcDo[l+1]) {
			return 1;
		}
	}
	return 0;
}

/* generate the requested files						(ksb)
 */
void
Gen(pcBaseName, pcManPage)
char *pcBaseName, *pcManPage;
{
	register FILE *fpCBody, *fpCHeader, *fpMan;
	register int ch;
	register char *pcExcl, *pcScan, *pcC1, *pcC2;
	auto char *pchDot;
	auto int fVerify, iUsageLines, iHelpLines;
	auto char acFile[MAXPATHLEN+1];
	auto LIST_INDEX u;
	register char **ppcExcl, **ppcValues;

	if (nil != pcManPage && '-' == pcBaseName[0] && '\000' == pcBaseName[1] && '-' == pcManPage[0] && '\000' == pcManPage[1]) {
		fprintf(stderr, "%s: both man page and C code directed to stdout\n", progname);
		return;
	}

	/* force names for the attributes we must have
	 */
	if (nil != pchUsage) {
		if (nil == pchTerse)
			pchTerse = "u_line";
		if (nil == pchVector)
			pchVector = "u_help";
	}

#if ADD_MULTI_BITS
	/* quick and dirty check for more than one basename
	 */
	fMultiBases = ((char **)0 != ppcBases) && (char *)0 != ppcBases[0] && (char *)0 != ppcBases[1];
#endif

	/* Config told us we expected some args.. but not what to
	 * convert them to, or tryed to mix and list (both).
	 */
	if ((fMix || fPSawZero) && !(fPSawEvery || fPSawList)) {
		fprintf(stderr, "%s: no function to call for arguments\n", progname);
	}
	if (fMix && fPSawList) {
		fprintf(stderr, "%s: cannot intermix lists with options\n", progname);
		exit(MER_SEMANTIC);
	}

	(void)strcpy(acFile, pcBaseName);
	pchDot = strrchr(acFile, '.');
	if (fMaketd) {
		fpCBody = fopen("/dev/null", "w");
		fpCHeader = (FILE *)0;
	} else if ('-' == pcBaseName[0] && '\000' == pcBaseName[1]) {
		fpCBody = stdout;
		fpCHeader = (FILE *)0;
	} else if (nil == pchDot || ('c' == pchDot[1] && '\000' == pchDot[2])) {
		if (nil == pchDot) {
			pchDot = strrchr(acFile, '\000');
			pchDot[0] = '.';
			pchDot[1] = 'c';
			pchDot[2] = '\000';
		}
		if (NULL == (fpCBody = fopen(acFile, "w"))) {
			fprintf(stderr, acFailOpen, progname, acFile, strerror(errno));
			exit(MER_BROKEN);
		}
		pchDot[1] = 'h';
		pchDot[2] = '\000';
		if (NULL == (fpCHeader = fopen(acFile, "w"))) {
			fprintf(stderr, acFailOpen, progname, acFile, strerror(errno));
			exit(MER_BROKEN);
		}
	} else {
		fpCBody = (FILE *)0;
		fpCHeader = (FILE *)0;
	}

	/* finish pre-scan of excludes --
	 * We note here that all options that `stop' are mutually  exclusive
	 * (well, you can't give two of them, right?) The bundle packer in
	 * mkusage.c will push then into a clump and the mkuline routine will
	 * put it at the and of the of each option set.  Yow.
	 */
	if ((char *)0 != (pcExcl = SynthExclude(pORRoot))) {
		ListAdd(& LIExcl, pcExcl);
	}
	/* global reflexive excludes to *not* force a once on
	 * the target options, but they do accumulate
	 */
	for (ppcExcl = ListCur(& LIExcl, & u); u > 0; --u) {
		register OPTION *pOR;

		pcExcl = *ppcExcl++;
		if ('\000' == pcExcl[1]) {
			fprintf(stderr, "%s: global exclusion lists of length one do not do anything (useful)\n", progname);
			continue;
		}
		for (pcScan = pcExcl; '\000' != *pcScan; ++pcScan) {
			pOR = newopt(*pcScan, & pORRoot, 0, 0);
			if (nilOR == pOR) {
				fprintf(stderr, "%s: global exclusion list `%s\' contains unknown option `-%c\'\n", progname, pcExcl, *pcScan);
				exit(MER_SEMANTIC);
			}
			pOR->oattr |= OPT_FORBID;
			pcC1 = (char *)0;
			if ((char *)0 == pOR->pchforbid) {
				/* take extra space to save reallocs */
				pOR->pchforbid = malloc((unsigned)strlen(pcExcl)+16);
				pcC1 = pOR->pchforbid;
			} else {
				pOR->pchforbid = realloc(pOR->pchforbid, strlen(pOR->pchforbid)+strlen(pcExcl)+1);
				if ((char *)0 != pOR->pchforbid) {
					pcC1 = pOR->pchforbid+strlen(pOR->pchforbid);
				}
			}
			if ((char *)0 == pcC1) {
				fprintf(stderr, acOutMem, progname);
				exit(MER_LIMIT);
			}
			for (pcC2 = pcExcl; '\000' != *pcC2; ++pcC2) {
				if (*pcScan == *pcC2)
					continue;
				*pcC1++ = *pcC2;
			}
			*pcC1 = '\000';
		}
	}

	/* type resolution can read in more files
	 */
	fVerify = FixOpts();

	/* should be in "getopt.m" -- ksb
	 */
	ListAdd(& LISysIncl, "<stdio.h>");
	ListAdd(& LISysIncl, "<ctype.h>");
	if (fWeGuess) {
#if USE_STRINGS
		ListAdd(& LISysIncl, "<strings.h>");
#else
		ListAdd(& LISysIncl, "<string.h>");
#endif
#if USE_STDLIB
		ListAdd(& LISysIncl, "<stdlib.h>");
#endif
#if USE_MALLOC_H
		ListAdd(& LISysIncl, "<malloc.h>");
#endif
	}

	/* don't build the program, just output depends
	 */
	if (fMaketd) {
		DepMeta(& LIMetaC);
		DepMeta(& LIMetaI);
		DepMeta(& LIMetaH);
		return;
	}

	ppcValues = ListCur(& pKVGetopt->LIinputs, & u);
	if (nil == ppcValues[0]) {	/* [1] envopt called */
		ppcValues[0] = BOOL_STR(fGetenv || nil != pchGetenv || ((char *)0 != pcDefBaseOpt && '\000' != *pcDefBaseOpt) || BaseMayInsert(ppcBases));
	}
	if (nil == ppcValues[1]) {	/* [2] getarg called */
		ppcValues[1] = BOOL_STR(fMix || fPSawEvery);
	}
	if (nil == ppcValues[2]) {	/* [3] getopt called */
		ppcValues[2] = BOOL_STR(nilOR != pORRoot);
	}

	KeyFix();
	if (nilOR == pORRoot) {	/* turn off intermix, no options	*/
		fMix = 0;
	}

	iHelpLines = iUsageLines = 0;
	if ((FILE *)0 != fpCBody) {
		auto int iFound;
		static char *apcDef[2] = { "char\n\t", ",\n\t" };
		rewind(fpTop);
		mkintro(fpCBody, fVerify, fpTop, & LIMetaI);

		iFound = 0;
		if (nil != pchProgName && '\000' != pchProgName[0]) {
			fprintf(fpCBody, apcDef[iFound]);
			iFound = 1;
			fprintf(fpCBody, "*%s = \"%cId$\"", pchProgName, '$');
		}
		if (nil != pchTerse && '\000' != pchTerse[0]) {
			fprintf(fpCBody, apcDef[iFound]);
			iFound = 1;
			fprintf(fpCBody, "*a%s[] = ", pchTerse);
			iUsageLines = mkuvector(fpCBody, 0);
		}
		if (nil != pchVector && '\000' != pchVector[0]) {
			fprintf(fpCBody, apcDef[iFound]);
			iFound = 1;
			fprintf(fpCBody, "*%s[] = ", pchVector);
			iHelpLines = mkvector(fpCBody, pchVector);
		}
		if (iFound)
			mkdecl(fpCBody, 0, & sbCBase[0], OPT_GLOBAL, 0);
		else
			mkdecl(fpCBody, 0, nil, OPT_GLOBAL, 0);
		fputc('\n', fpCBody);
		if (nil != pchTerse) {
			fprintf(fpCBody, "#ifndef %s\n#define %s\t(a%s[0])\n#endif\n", pchTerse, pchTerse, pchTerse);
		}

		if (nil != pchUsage) {
			mkusage(fpCBody, pchUsage);
		}
		mkonly(fpCBody);
		mkchks(fpCBody);
	}
	if ((FILE *)0 != fpCHeader) {
		mkdot_h(fpCHeader, iUsageLines, iHelpLines);
	}
	if ((FILE *)0 != fpCBody) {
		mkreaders(fpCBody, fpCHeader);
		/* restore saved routines from user code and copy to gen'd
		 */
		rewind(fpDivert);
		while (EOF != (ch = getc(fpDivert))) {
			putc(ch, fpCBody);
		}
		putc('\n', fpCBody);
		GenMeta(fpCBody, & LIMetaC);
		mkmain(fpCBody);
		if (stdout != fpCBody) {
			(void)fclose(fpCBody);
		}
	}
	if ((FILE *)0 != fpCHeader) {
		/* restore saved head code
		 */
		rewind(fpHeader);
		while (EOF != (ch = getc(fpHeader))) {
			putc(ch, fpCHeader);
		}
		putc('\n', fpCHeader);
		GenMeta(fpCHeader, & LIMetaH);
		(void)fclose(fpCHeader);
	}

	if (nil != pcManPage) {
		if ('-' == pcManPage[0] && '\000' == pcManPage[1]) {
			fpMan = stdout;
		} else if (-1 != access(pcManPage, 0)) {
			fprintf(stderr, "%s: access: %s: File exists\n", progname, pcManPage);
			return;
		} else if (NULL == (fpMan = fopen(pcManPage, "w"))) {
			fprintf(stderr, acFailOpen, progname, pcManPage, strerror(errno));
			exit(MER_BROKEN);
		}
		mkman(fpMan);
		if (stdout != fpMan) {
			(void)fclose(fpMan);
		}
	}
	if ((FILE *)0 != fpDivert) {
		(void)fclose(fpDivert);
	}
	if ((FILE *)0 != fpHeader) {
		(void)fclose(fpHeader);
	}
	if ((FILE *)0 != fpTop) {
		(void)fclose(fpTop);
	}
}

/* our own cleanup code
 */
void
Cleanup()
{
	register char **ppcDel;
	auto LIST_INDEX u;

	for (ppcDel = ListCur(& LIDelete, & u); u > 0; --u) {
		(void)unlink(*ppcDel++);
	}
}
