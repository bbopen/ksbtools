/* $Id: routine.c,v 8.17 2008/07/09 19:25:42 ksb Exp $
 * Emit code to read a (FILE*) or (fd) open'd file			(ksb)
 */
#include <sys/types.h>
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
#include "emit.h"
#include "key.h"
#include "routine.h"
#include "check.h"
#include "atoc.h"

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

static char
	acLineno[] = "u_ilineno",
	acMyFix[] = "u_pcFGLine",
	acFp[] = "u_fp",
	acFd[] = "u_fd",
	acHunkCnt[] = "u_ihunks",
	acSpanCnt[] = "u_ispan";


/* tell the option expander which column we are from			(ksb)
 * look for the allowed option's routine's current hunk's order list
 * to contain a back pointer to us.
 */
char *
RColWhich(pORCol, pcDst)
OPTION *pORCol;
char *pcDst;
{
	static char acNone[] = "00";
	register int iCol;
	register R_LINE *pRL;
	register ROUTINE *pRG;
	register OPTION *pORIn;

	if (nilOR == (pORIn = pORCol->pORallow) || (ROUTINE *)0 == (pRG = pORIn->pRG) || (R_LINE **)0 == pRG->ppRL || (R_LINE *)0 == (pRL = pRG->ppRL[0]) || (OPTION **)0 == pRL->ppORlorder) {
		return strcpy(pcDst, acNone);
	}

	/* ok, now it is safe to look...
	 */
	for (iCol = 0; iCol < pRL->icols; ++iCol) {
		if (pORCol == pRL->ppORlorder[iCol]) {
			break;
		}
	}
	if (iCol < pRL->icols) {
		++iCol;
	} else {
		iCol = 0;
	}
	(void)sprintf(pcDst, "%d", iCol);
	return pcDst;
}

/* which hunk we are in?						(ksb)
 * same kinda thing as RColWhich, but search for hunk.
 */
char *
RHunkWhich(pORHunk, pcDst)
OPTION *pORHunk;
char *pcDst;
{
	static char acNone[] = "00";
	register int iHunk;
	register R_LINE *pRLThis, *pRLCount;
	register ROUTINE *pRG;
	register OPTION *pORIn;

	if (nilOR == (pORIn = pORHunk->pORallow) || (ROUTINE *)0 == (pRG = pORIn->pRG) || (R_LINE **)0 == pRG->ppRL || (R_LINE *)0 == (pRLThis = pRG->ppRL[0])) {
		return strcpy(pcDst, acNone);
	}
	iHunk = 0;
	for (pRLCount = pRG->pRLfirst; (R_LINE *)0 != pRLCount; pRLCount = pRLCount->pRLnext) {
		++iHunk;
		if (pRLCount == pRLThis)
			break;
	}
	if ((R_LINE *)0 == pRLCount) {
		iHunk = 0;
	}
	(void)sprintf(pcDst, "%d", iHunk);
	return pcDst;
}

static EMIT_MAP aERRoutine[] = {
	{'#', "comment", "comment leader in hunk"},
	{'a', "after", "C code run after hunk is finished"},
	{'A', "advance", "advances to the next hunk for the next line"},
	{'b', "before", "C code run before hunk is started"},
	{'c', "update", "code to finish line conversion"},
	{'e', "aborts", "code to abort a bad line conversion"},
	{'E', "EOS", "C escape for end of an input line character"},
	{'f', "width", "max printf width of any parameter in this hunk"},
	{'g', "xraw", "expand WRT raw buffer's trigger..."},
	{'G', "raw", "buffer (or expression) for unconverted input line"},
	{'h', "help", "help text for this hunk"},
	{'i', "function", "expand WRT this routine's option record..."},
	{'I', "routine", "name of routine we are in"},
	{'j', "totalcolumn", "total columns to process"},
	{'J', "column", "which column we are in"},
	{'K', "key", "routine's unnamed API key..."},
	{'l', "buffers", "converted buffers in column order"},
	{'L', "params", "parameter names in column order"},
	{'M', "first", "go back to the first hunk"},
	{'n', "named", "name of function to call on each line"},
	{'N', "line", "C int with current line number"},
	{'p', "arg1", "parameter passed to the decode function"},
	{'P', "prototype", "prototype used to emit hunk check function names"},
	{'S', "span", "C int line number we are in a counted hunk only"},
	{'t', "mark", "end mark (start next hunk)"},
	{'T', "terse", "name of terse help vector"},
	{'u', "user", "C code for user control point"},
	{'U', "usage", "name of usage function for routine"},
	{'V', "vector", "name of long help vector or vectors"},
	{'w', "totalhunks", "total hunks in this file"},
	{'W', "hunk", "which hunk we are in now"},
	{'x', "next", "stuff from next hunk..."},
	{'z', "error", (char *)0}
};

EMIT_HELP EHRoutine = {
	"routine expander", "%m or %M",
	sizeof(aERRoutine)/sizeof(EMIT_MAP),
	aERRoutine,
	"select a column from the current hunk...",
	(EMIT_HELP *)0
};

/* decode the Line specific stuff for the emit functions		(ksb)
 * top level calls us from %M and %m with different ppRL's
 * inv.: pOR->pRG is the routine which contains ppRL[0]'s line parser.
 */
int
RLineEsc(ppcEsc, pOR, ppRL, pEO, pcOut)
char **ppcEsc;
OPTION *pOR;
R_LINE **ppRL;
EMIT_OPTS *pEO;
char *pcOut;
{
	register ROUTINE *pRGThis;
	register int iHunk, iCol, cCmd;
	register R_LINE *pRLCount;
	auto int iNumber;
	static char acNoAttr[] = "%s: %s: no %s attribute (or current hunk)\n";

	/* since %m can fallout to the enclosing scope we must too
	 */
	if ((ROUTINE *)0 == (pRGThis = pOR->pRG)) {
		pRGThis = pOR->pORallow->pRG;
	}
	if ((ROUTINE *)0 == pRGThis) {
		fprintf(stderr, "%s: %s: internal routine lost!\n", progname, usersees(pOR, nil));
		iExit |= MER_INV;
		return 1;
	}
	for (;;) { switch (cCmd = EscCvt(& EHRoutine, ppcEsc, & iNumber)) {
	case 'M':	/* start over a first hunk (%mM == %M) */
		ppRL = & pRGThis->pRLfirst;
		continue;
	case 'x':	/* next line-hunk */
		if ((R_LINE *)0 == ppRL[0]) {
			fprintf(stderr, "%s: %s: %s: no line descriptions\n", progname, usersees(pOR, nil), pRGThis->pcfunc);
			return 1;
		}
		ppRL = & ppRL[0]->pRLnext;
		if ((R_LINE *)0 == ppRL[0]) {
			fprintf(stderr, "%s: %s: %s: no next line description\n", progname, usersees(pOR, nil), pRGThis->pcfunc);
			return 1;
		}
		continue;

	/* %m<col> numbered in order left to right
	 * for example %m1n == col 1's name
	 */
	case 0:
		if ((R_LINE *)0 == ppRL[0]) {
			fprintf(stderr, "%s: %s: no current hunk to select a column (%d)\n", progname, usersees(pOR, nil), iNumber);
			return 1;
		}
		if (iNumber == 0 || iNumber >= ppRL[0]->icols) {
			fprintf(stderr, "%s: %s: %s: column %d (only ", progname, usersees(pOR, nil), pRGThis->pcfunc, iNumber);
			if (1 == ppRL[0]->icols)
				fprintf(stderr, "column 1");
			else
				fprintf(stderr, "1 to %d", ppRL[0]->icols);
			fprintf(stderr, " available)\n");
			return 1;
		}
		--iNumber;
		if ((OPTION **)0 == ppRL[0]->ppORlorder) {
			fprintf(stderr, "%s: %s: %s: no template attribute\n", progname, usersees(pOR, nil), pRGThis->pcfunc);
			return 1;
		}
		return TopEsc(ppcEsc, ppRL[0]->ppORlorder[iNumber], pEO, pcOut);

	/* line attributes for the hunks
	 */
	case 'h':
		if ((R_LINE *)0 == ppRL[0] || nil == ppRL[0]->pchelp) {
			fprintf(stderr, acNoAttr, usersees(pOR, nil), "help");
			return 1;
		}
		(void)strcpy(pcOut, ppRL[0]->pchelp);
		break;
	case 'b':
		if ((R_LINE *)0 == ppRL[0] || nil == ppRL[0]->pcbefore) {
			fprintf(stderr, acNoAttr, usersees(pOR, nil), "before");
			return 1;
		}
		(void)strcpy(pcOut, ppRL[0]->pcbefore);
		break;
	case 'n':
		if ((R_LINE *)0 == ppRL[0] || nil == ppRL[0]->pcnamed) {
			fprintf(stderr, acNoAttr, usersees(pOR, nil), "named");
			return 1;
		}
		(void)strcpy(pcOut, ppRL[0]->pcnamed);
		break;
	case 'c':
		if ((R_LINE *)0 == ppRL[0] || nil == ppRL[0]->pcupdate) {
			fprintf(stderr, acNoAttr, usersees(pOR, nil), "update");
			return 1;
		}
		(void)strcpy(pcOut, ppRL[0]->pcupdate);
		break;
	case 'u':
		if ((R_LINE *)0 == ppRL[0] || nil == ppRL[0]->pcuser) {
			fprintf(stderr, acNoAttr, usersees(pOR, nil), "user");
			return 1;
		}
		(void)strcpy(pcOut, ppRL[0]->pcuser);
		break;
	case 'e':
		if ((R_LINE *)0 == ppRL[0] || nil == ppRL[0]->pcaborts) {
			fprintf(stderr, acNoAttr, usersees(pOR, nil), "abort");
			return 1;
		}
		(void)strcpy(pcOut, ppRL[0]->pcaborts);
		break;
	case 't':
		if ((R_LINE *)0 == ppRL[0] || nil == ppRL[0]->pcemark) {
			fprintf(stderr, acNoAttr, usersees(pOR, nil), "end");
			return 1;
		}
		(void)strcpy(pcOut, ppRL[0]->pcemark + 1);
		break;
	case 'a':
		if ((R_LINE *)0 == ppRL[0] || nil == ppRL[0]->pcafter) {
			fprintf(stderr, acNoAttr, usersees(pOR, nil), "after");
			return 1;
		}
		(void)strcpy(pcOut, ppRL[0]->pcafter);
		break;
	case '#':
		if ((R_LINE *)0 == ppRL[0] || nil == ppRL[0]->pcopenc) {
			fprintf(stderr, acNoAttr, usersees(pOR, nil), "comment");
			return 1;
		}
		(void)strcpy(pcOut, ppRL[0]->pcopenc);
		break;
	case 'j':	/* total cols */
		if ((R_LINE *)0 == ppRL[0]) {
			fprintf(stderr, acNoAttr, usersees(pOR, nil), "column");
			return 1;
		}
		(void)sprintf(pcOut, "%d", ppRL[0]->icols);
		break;

	/* computed */
	case 'A':
		/* doesn't do any after cleanup - as documented */
		if (acMyFix == pRGThis->pcarg1) {
			sprintf(pcOut, "++%s;return 0;", acHunkCnt);
		} else {
			sprintf(pcOut, "break;");
		}
		break;
	case 'E':	/* \n or \000 */
		if (acMyFix == pRGThis->pcarg1) {
			sprintf(pcOut, "\\000");
		} else {
			sprintf(pcOut, "\\n");
		}
		break;
	case 'f':
		if ((R_LINE *)0 == ppRL[0]) {
			fprintf(stderr, acNoAttr, usersees(pOR, nil), "width");
			return 1;
		}
		(void)sprintf(pcOut, "%d", ppRL[0]->imaxpwidth);
		break;
	case 'l':
		if ((R_LINE *)0 == ppRL[0] || nil == ppRL[0]->pcargs) {
			fprintf(stderr, acNoAttr, usersees(pOR, nil), "args");
			return 1;
		}
		(void)strcpy(pcOut, ppRL[0]->pcargs);
		break;
	case 'L':
		if ((R_LINE *)0 == ppRL[0] || nil == ppRL[0]->pcpargs) {
			fprintf(stderr, acNoAttr, usersees(pOR, nil), "pargs");
			return 1;
		}
		(void)strcpy(pcOut, ppRL[0]->pcpargs);
		break;

	/* from the routine description */
	case 'g':
		return TopEsc(ppcEsc, pRGThis->pORraw, pEO, pcOut);
	case 'G':
		if (nil != pRGThis->pcraw) {
			(void)strcpy(pcOut, pRGThis->pcraw);
		} else {
			(void)strcpy(pcOut, acMyFix);
		}
		break;
	case 'i':
		if (nilOR == pRGThis->pORfunc) {
			fprintf(stderr, "%s: %s: %s does not have a description\n", progname, usersees(pOR, nil), pRGThis->pcfunc);
			return 1;
		}
		return TopEsc(ppcEsc, pRGThis->pORfunc, pEO, pcOut);
	case 'I':
		(void)strcpy(pcOut, pRGThis->pcfunc);
		break;
	case 'K':
		return KeyEsc(ppcEsc, pOR, pRGThis->pKV, pEO, pcOut);
	case 'p':
		(void)strcpy(pcOut, pRGThis->pcarg1);
		break;
	case 'P':
		if (nil != pRGThis->pcprototype) {
			(void)strcpy(pcOut, pRGThis->pcprototype);
		} else {
			(void)strcpy(pcOut, "u_%mW_%mI");
		}
		break;
	case 'T':
		if (nil == pRGThis->pcterse) {
			fprintf(stderr, "%s: %s: has no terse attribute\n", progname, pRGThis->pcfunc);
			return 1;
		}
		(void)strcpy(pcOut, pRGThis->pcterse);
		break;
	case 'U':
		if (nil == pRGThis->pcusage) {
			fprintf(stderr, "%s: %s: has no usage attribute\n", progname, pRGThis->pcfunc);
			return 1;
		}
		(void)strcpy(pcOut, pRGThis->pcusage);
		break;
	case 'V':
		if (nil == pRGThis->pcvector) {
			fprintf(stderr, "%s: %s: has no vector attribute\n", progname, pRGThis->pcfunc);
			return 1;
		}
		(void)strcpy(pcOut, pRGThis->pcvector);
		break;
	case 'w':	/* total hunks */
		(void)sprintf(pcOut, "%d", pRGThis->uhunks);
		break;

	/* helper buffers or computed vaules
	 */
	case 'S':
		if ((char *)0 != ppRL[0]->pcemark && '#' == ppRL[0]->pcemark[0]) {
			(void)strcpy(pcOut, acSpanCnt);
			break;
		}
		fprintf(stderr, "%s: %%mS: used without a counted hunk\n", progname);
		fprintf(stderr, " falling back to absolute line in file\n");
		iExit |= MER_SEMANTIC;
		/* fall though */
	case 'N':
		if (pRGThis->pclineno) {
			(void)strcpy(pcOut, pRGThis->pclineno);
		} else {
			(void)strcpy(pcOut, acLineno);
		}
		break;
	case 'W':	/* which hunk we are in */
		iHunk = 0;
		for (pRLCount = pRGThis->pRLfirst; (R_LINE *)0 != pRLCount; pRLCount = pRLCount->pRLnext) {
			++iHunk;
			if (pRLCount == ppRL[0])
				break;
		}
		if ((R_LINE *)0 == pRLCount) {
			iHunk = 0;
		}
		(void)sprintf(pcOut, "%d", iHunk);
		break;
	case 'J':	/* which column we are in? */
		if ((R_LINE *)0 == ppRL[0]) {
			(void)strcpy(pcOut, "00");
			break;
		}
		for (iCol = 0; iCol < ppRL[0]->icols; ++iCol) {
			if (pOR == ppRL[0]->ppORlorder[iCol]) {
				break;
			}
		}
		if (iCol < ppRL[0]->icols) {
			++iCol;
		} else {
			iCol = 0;
		}
		(void)sprintf(pcOut, "%d", iCol);
		break;

	case -1:
		fprintf(stderr, "%s: %c: unknown percent escape, use '-E routine' for help\n", progname, iNumber);
		return 1;

	case -2:
		fprintf(stderr, "%s: %*.*s: unknown long escape, use '-E routine' for help\n", progname, iNumber, iNumber, *ppcEsc);
		return 1;

	default:
	case 'z':
		CSpell(pcOut, cCmd, 0);
		fprintf(stderr, "%s: routine level: %s: %%%s: expander botch\n", progname, pRGThis->pcfunc, pcOut);
		iExit |= MER_PAIN;
		return 1;
	} break; }

	return 0;
}


/* if there is a line description in the slot return it,		(ksb)
 * else build an empty line/hunk description and fill it in.
 */
R_LINE *
RLine(ppRL)
R_LINE **ppRL;
{
	register R_LINE *pRLMem;

	if ((R_LINE *)0 != *ppRL) {
		return *ppRL;
	}
	if ((R_LINE *)0 == (pRLMem = malloc(sizeof(R_LINE)))) {
		fprintf(stderr, acOutMem, progname);
		exit(MER_LIMIT);
	}
	pRLMem->ppclorder = (char **)0;
	pRLMem->pcbefore = pRLMem->pcafter = nil;
	pRLMem->pcupdate = pRLMem->pcuser = nil;
	pRLMem->pcaborts = pRLMem->pcemark = nil;
	pRLMem->pcnamed = nil;
	pRLMem->pchelp = nil;
	pRLMem->pcopenc = nil;
	pRLMem->pRLnext = (R_LINE *)0;
	return *ppRL = pRLMem;
}

/* silly routine to make sure the fixed width args get displayed	(ksb)
 * correctly (in brackets for lack of something better)
 */
char *
PArgCat(pcDest, pcDel)
char *pcDest, *pcDel;
{
	register int fNumber;

	if ((fNumber = ('#' == pcDel[-1]))) {
		*pcDest++ = '[';
	}
	(void)strcpy(pcDest, pcDel);
	pcDest += strlen(pcDest);
	if (fNumber) {
		*pcDest++ = ']';
	}
	return pcDest;
}

/* return 1 for need ctype.h <old, bugus convention>			(ksb)
 * return 2 for fatal semantic error
 * else 0 [we are looking to make sure we can generate code to
 * read the file/fd.
 *
 * Loop though to verify all the variables (buffers) are real
 * and all the separators are cool.
 */
int
CheckRoutine(pORMaster)
OPTION *pORMaster;
{
	register char **ppcOrder, **ppcScan;
	register OPTION **ppOROrder, *pOR;
	register char **ppcSeps;
	register int iTotal, iBufs;
	register R_LINE *pRL;
	register ROUTINE *pRG;
	register OPTTYPE *pOTInv;
	auto int retval = 0;

	/* option should have a routine and doesn't
	 * mkcmd can't do this (yet)
	 */
	if ((ROUTINE *)0 == (pRG = pORMaster->pRG)) {
		fprintf(stderr, "%s: %s: needs a routine and doesn't have one\n", progname, usersees(pORMaster, nil));
		iExit |= MER_SEMANTIC;
		return 2;
	}
	if (nil != pRG->pcfunc) {
		pRG->pORfunc = FindBuf(pORDecl, pRG->pcfunc);
		if (nilOR != pRG->pORfunc && ! IS_ACT(pRG->pORfunc->pOTtype)) {
			fprintf(stderr, "%s: %s: %s: should be a function type\n", progname, usersees(pORMaster, nil), pRG->pcfunc);
			iExit |= MER_SEMANTIC;
			pRG->pORfunc = nilOR;
		}
	}

	if (nil == pRG->pcraw || '\000' == pRG->pcraw[0]) {
		pRG->pcraw = nil;
	} else {
		if (nilOR == (pRG->pORraw = FindBuf(pORDecl, pRG->pcraw))) {
			fprintf(stderr, "%s: %s: %s: unknown variable `%s'\n", progname, usersees(pORMaster, nil), pRG->pcfunc, pRG->pcraw);
			return 2;
		}
		if ('s' != pRG->pORraw->pOTtype->chkey) {
			fprintf(stderr, "%s: %s: %s: %s: should be type %s!\n", progname, usersees(pORMaster, nil), pRG->pcfunc, pRG->pcraw, "string");
			iExit |= MER_SEMANTIC;
			return 2;
		}
	}

	/* Because usage requires access to terse and vector, insert
	 * our private names for them, like at top level.
	 * We take the defid and prefix stuff on it.
	 */
	if (nil != pRG->pcusage && nil == pRG->pcvector) {
		register char *pcDef = mkdefid(pORMaster);
		static char acVTemplate[] = "u_vector_%s";

		pRG->pcvector = malloc(strlen(pcDef)+sizeof(acVTemplate)+3);
		if (nil == pRG->pcvector) {
			fprintf(stderr, acOutMem, progname);
			exit(MER_LIMIT);
		}
		(void)sprintf(pRG->pcvector, acVTemplate, pcDef);
	}
	if (nil != pRG->pcusage && nil == pRG->pcterse) {
		register char *pcDef = mkdefid(pORMaster);
		static char acTTemplate[] = "u_terse_%s";

		pRG->pcterse = malloc(strlen(pcDef)+sizeof(acTTemplate)+3);
		if (nil == pRG->pcterse) {
			fprintf(stderr, acOutMem, progname);
			exit(MER_LIMIT);
		}
		(void)sprintf(pRG->pcterse, acTTemplate, pcDef);
	}

	/* map type info to internal invarient codes, setup params
	 */
	pOTInv = pORMaster->pOTtype;
	if (IS_NO_RTN(pOTInv)) {
		fprintf(stderr, "%s: %s: type %s: no defined routine type, assume unconverted data\n", progname, usersees(pORMaster, nil), pOTInv->pchlabel);
		pRG->pcarg1 = acMyFix;
	} else if (IS_RTN_FP(pOTInv)) {
		pRG->pcarg1 = acFp;
	} else if (IS_RTN_FD(pOTInv)) {
		pRG->pcarg1 = acFd;
	} else if (IS_RTN_PC(pOTInv)) {
		pRG->pcarg1 = acMyFix;
	} else if (IS_RTN_VD(pOTInv)) {
		/* we pass a (void *)0, I hope */
		pRG->pcarg1 = acMyFix;
	}
	/* we have a problem-o because we want to see the keep value
	 * we'll have to make the keep variable global or change the
	 * call to the routine.  We claim "function(%n);" works so we'll
	 * have to make the keep global, but static.  Sigh.
	 */
	if (ISLOCAL(pORMaster)) {
		fprintf(stderr, "%s: %s forced to global scope by routine %s\n", progname, usersees(pORMaster, nil), pRG->pcfunc);
		pORMaster->oattr ^= OPT_GLOBAL;
		pORMaster->oattr |= OPT_STATIC;	/* XXX I think best */
	}

	pRG->uhunks = 0;
	pRG->umaxfixed = 0;
	for (pRL = pRG->pRLfirst; (R_LINE *)0 != pRL; pRL = pRL->pRLnext) {
		register int fSep, i, iArgLen, iPArgLen, iPLen;
		auto char *pcArgs, *pcPArgs;

		++pRG->uhunks;
		pRL->imaxpwidth = 0;
		if ((char **)0 == (ppcOrder = pRL->ppclorder)) {
			pRL->ppORlorder = (OPTION **)0;
			continue;
		}
		pRL->pcleader = (char *)0;

		for (iTotal = 0, ppcScan = ppcOrder; (char *)0 != *ppcScan; ++iTotal, ++ppcScan) {
		}
		/* we know there are at *most* iTotal buffers and at least
		 * iTotal/2 of them, because we freely mix buffers and seps
		 * if we over allocate by a factor of two here we don't care.
		 */
		ppOROrder = (OPTION **)calloc(iTotal+1, sizeof(OPTION *));
		ppcSeps = (char **)calloc(iTotal, sizeof(char *));
		if ((char **)0 == ppcSeps || (OPTION **)0 == ppOROrder) {
			fprintf(stderr, acOutMem, progname);
			exit(MER_LIMIT);
		}
		iPArgLen = iArgLen = 1;
		iBufs = 0;
		fSep = 0;
		for (ppcScan = ppcOrder; (char *)0 != *ppcScan; ++ppcScan) {
			register char *pcThis;

			pcThis = ppcScan[0]+1;
			/* \xxx, :, \t, !, /,  , ...
			 */
			if (!isalpha(pcThis[0])) {
				if (iBufs < 1) {
					pRL->pcleader = pcThis;
				} else if (fSep) {
					fprintf(stderr, "%s: %s: %s: no buffer between separators (%s and %s)\n", progname, usersees(pORMaster, nil), pRG->pcfunc, iBufs < 1 ? pRL->pcleader : ppcSeps[iBufs-1], pcThis);
					retval |= 2;
					continue;
				} else {
					ppcSeps[iBufs-1] = pcThis;
				}
				fSep = 1;
				iPArgLen += strlen(pcThis)+1;
				iArgLen += strlen(pcThis)+4;
				continue;
			}

			fSep = 0;
			pOR = FindBuf(pORDecl, pcThis);
			if (nilOR == (ppOROrder[iBufs++] = pOR)) {
				fprintf(stderr, "%s: %s: %s: %s undefined buffer\n", progname, usersees(pORMaster, nil), pRG->pcfunc, pcThis);
				retval |= 2;
				continue;
			}
			if (ISDUPPARAM(pOR)) {
				fprintf(stderr, "%s: %s: %s: variable `%s\' listed more than once\n", progname, usersees(pORMaster, nil), pRG->pcfunc, pOR->pchname);
				retval |= 2;
				continue;
			}
			if (nil == pOR->pchverb) {
				pOR->pchverb = pOR->pOTtype->pchhelp;
			}
			pOR->oattr |= OPT_DUPPARAM;
			pOR->pOTtype->tattr |= _JARG;
			if (ISVERIFY(pOR)) {
				retval |= 1;
				pOR->pOTtype->tattr |= _CHK;
			}
			iPLen = strlen(pOR->pchdesc);
			if (iPLen > pRL->imaxpwidth) {
				pRL->imaxpwidth = iPLen;
			}
			iPArgLen += iPLen+2;
			iArgLen += strlen(pOR->pchname)+2;
		}

		/* cleanup error checks
		 */
		pRL->pcargs = pcArgs = malloc(iArgLen);
		pRL->pcpargs = pcPArgs = malloc(iPArgLen);
		*pcArgs = '\000';
		*pcPArgs = '\000';

		/* build the parameter and help argument descriptions
		 */
		if (pRL->pcleader) {
			pcPArgs = PArgCat(pcPArgs, pRL->pcleader);
		}
		for (i = 0; i < iBufs; ++i) {
			if (nilOR == (pOR = ppOROrder[i]))
				continue;
			pOR->oattr &= ~OPT_DUPPARAM;

			(void)strcpy(pcPArgs, pOR->pchdesc);
			pcPArgs += strlen(pcPArgs);
			if (nil == ppcSeps[i] || '\000' == ppcSeps[i][0]) {
				if (i+1 < iBufs)
					*pcPArgs++ = ' ';
			} else {
				pcPArgs = PArgCat(pcPArgs, ppcSeps[i]);
			}

			if (0 < i) {
				*pcArgs++ = ',';
				*pcArgs++ = ' ';
			}
			(void)strcpy(pcArgs, pOR->pchname);
			pcArgs += strlen(pcArgs);
		}
		/* N.B. we do not copy (or size) the end of line mark yet,
		 * mostly it is "\n" which we do not want to put on?
		 * maybe we'd have to be clever here -- fine line, eh?
		 */
		pRL->icols = iBufs;
		pRL->ppORlorder = ppOROrder;
		pRL->ppcdelims = ppcSeps;

		/* compute room for fixed width input column buffers, if any
		 */
		pRL->ufixwidth = (nil != pRL->pcleader && '#' == pRL->pcleader[-1]) ? atoi(pRL->pcleader) : 0;
		for (i = 0; i < iBufs; ++i) {
			if ((char *)0 != ppcSeps[i] && '#' == ppcSeps[i][-1]) {
				pRL->ufixwidth += atoi(ppcSeps[i])+1;
			}
		}
		if (pRL->ufixwidth > pRG->umaxfixed) {
			pRG->umaxfixed = pRL->ufixwidth;
		}
	}
	if (0 != pRG->umaxfixed) {
		pRG->irattr |= ROUTINE_FIXED;
		/* round off to make the stack nice -- like a person would
		 */
		if (0 != (pRG->umaxfixed & 7)) {
			pRG->umaxfixed |= 7;
			pRG->umaxfixed += 1;
		}
	}
	pRG->irattr |= ROUTINE_CHECKED;
	return retval;
}

/* output side */

/* we might have to drop this before the vector/terse or after		(ksb)
 */
static void
UsageHead(fp, pRG, pORMaster, pcWhich)
FILE *fp;
ROUTINE *pRG;
OPTION *pORMaster;
char *pcWhich;
{
	Emit(fp, "/* help function for %L\n */\n", pORMaster, nil, 0);
	if (RNT_STATIC(pRG)) {
		Emit(fp, "static ", pORMaster, nil, 0);
	}
	if (fAnsi) {
		Emit(fp, "void %mU(%yFb *fp, %yub %a)\n", pORMaster, pcWhich, 0);
	} else {
		Emit(fp, "void\n%mU(fp, %a)\n%yFb *fp;%yub %a;", pORMaster, pcWhich, 0);
	}
	Emit(fp, "{register char **u_ppc;register %yub u_uHunk;\n", pORMaster, nil, 0);
}

/* output all the help we can for the file format			(ksb)
 *
 * Vector (%mV) should give the column params,
 * as vector at the top level gives the option help and params.
 *
 * Terse (%mT) should be like terse at the top level.
 *
 * Usage is a function that describes the whole file format only if
 * given (%mU).  Forces terses and vectors because it
 * uses them.
 *
 * We also write externs to the header file, for the variables
 * we define and initialize.
 */
static void
mkhelp(fp, pORMaster, pRG, fpCHeader)
FILE *fp, *fpCHeader;
OPTION *pORMaster;
ROUTINE *pRG;
{
	register OPTION *pOR;
	register R_LINE *pRL;
	register int iCol, iHunk, iMax, iMin;
	register int iLevel, fExtern;
	register char *pcStorage;
	auto char acFmt[1024];
	static char acWhich[] = "u_uWhich";

	iLevel = 0;
	fExtern = (FILE *)0 != fpCHeader && !RNT_STATIC(pRG);
	pcStorage = (RNT_STATIC(pRG) || RNT_LOCAL(pRG)) ? "static " : "";
	if (nil != pRG->pcvector || nil != pRG->pcterse || nil != pRG->pcusage) {
		static char acHBanner[] = "\n/* %W generated help for %L\n */\n";
		Emit(fp, acHBanner, pORMaster, "", iLevel);
		if (fExtern) {
			Emit(fpCHeader, acHBanner, pORMaster, nil, iLevel);
		}
	}

	/* local means hide the vector and usage facility in the
	 * usage function.  He should do this at lop level.
	 * [It almost makes the usage function at top make sense. --ksb ]
	 */
	if (nil != pRG->pcusage && RNT_LOCAL(pRG)) {
		UsageHead(fp, pRG, pORMaster, acWhich);
		iLevel = 1;
	}
	/* the usage function needs the terse and vector attributes so
	 * we forces them in CheckRoutine to be non-nil if we have usage.
	 */
	if (nil != pRG->pcvector) {
		/* if the hunks are about the same width unify them
		 */
		iMin = iMax = 1;	/* shut up gcc -Wall */
		for (pRG->ppRL = & pRG->pRLfirst, iHunk = 1; (R_LINE *)0 != (pRL = pRG->ppRL[0]); pRG->ppRL = & pRL->pRLnext, ++iHunk) {
			if (1 == iHunk) {
				iMin = iMax = pRL->imaxpwidth;
				continue;
			}
			if (iMin > pRL->imaxpwidth) {
				iMin = pRL->imaxpwidth;
			}
			if (iMax < pRL->imaxpwidth) {
				iMax = pRL->imaxpwidth;
			}
		}
		if (iMax - iMin > pRG->uhunks+1) {
			iMax = 0;
		}
		for (pRG->ppRL = & pRG->pRLfirst, iHunk = 1; (R_LINE *)0 != (pRL = pRG->ppRL[0]); pRG->ppRL = & pRL->pRLnext, ++iHunk) {
			register int iLen;
			Emit(fp, "%achar *%mV_%mW[%mj+1] = %{\n", pORMaster, pcStorage, iLevel);
			for (iCol = 0; iCol < pRL->icols; ++iCol) {
				pOR = pRL->ppORlorder[iCol];
				iLen = 0 != iMax ? iMax : pRL->imaxpwidth;
				sprintf(acFmt, "%-*.*s", iLen, iLen, pOR->pchdesc);
				Emit(fp, "\"%qa %qeh\",\n", pOR, acFmt, iLevel+1);
			}
			Emit(fp, "%ypi\n", pORMaster, nil, iLevel+1);
			Emit(fp, "%};\n", pORMaster, nil, iLevel);
			if (fExtern && !RNT_LOCAL(pRG)) {
				Emit(fpCHeader, "extern char *%mV_%mW[%mj+1];", pORMaster, nil, iLevel);
			}
		}
		/* fill in the real vector
		 */
		Emit(fp, "%achar **%mV[%mw+1] = %{\n", pORMaster, pcStorage, iLevel);

		for (pRG->ppRL = & pRG->pRLfirst, iHunk = 1; (R_LINE *)0 != (pRL = pRG->ppRL[0]); pRG->ppRL = & pRL->pRLnext, ++iHunk) {
			Emit(fp, "%mV_%mW,\n", pORMaster, nil, iLevel+1);
		}
		Emit(fp, "(char **)0\n", pORMaster, nil, iLevel+1);
		Emit(fp, "%};\n", pORMaster, nil, iLevel);
		if (fExtern && ! RNT_LOCAL(pRG)) {
			Emit(fpCHeader, "extern char **%mV[%mw+1];", pORMaster, nil, iLevel);
		}
	}
	if (nil != pRG->pcterse) {
		Emit(fp, "%achar *%mT[%mw+1] = %{\n", pORMaster, pcStorage, iLevel);

		for (pRG->ppRL = & pRG->pRLfirst, iHunk = 1; (R_LINE *)0 != (pRL = pRG->ppRL[0]); pRG->ppRL = & pRL->pRLnext, ++iHunk) {
			Emit(fp, "\"%qmL\",\n", pORMaster, nil, iLevel+1);
		}
		Emit(fp, "%ypi\n", pORMaster, nil, iLevel+1);
		Emit(fp, "%};\n", pORMaster, nil, iLevel);
		if (fExtern && !RNT_LOCAL(pRG)) {
			Emit(fpCHeader, "extern char *%mT[%mw+1];", pORMaster, nil, iLevel);
		}
	}
	if (nil != pRG->pcusage) {
		if (! RNT_LOCAL(pRG)) {
			UsageHead(fp, pRG, pORMaster, acWhich);
		}
		Emit(fp, "if (%a > %mw) {fprintf(stderr, \"%%s: %qL: %%d: only ", pORMaster, acWhich, 1);
		if (1 == pRG->uhunks) {
			Emit(fp, "1 hunk in %qp\\n\", progname, %a);", pORMaster, acWhich, 0);
		} else {
			Emit(fp, "hunks 1 to %mw in %qp\\n\", progname, %a);", pORMaster, acWhich, 0);
		}
		Emit(fp, "exit(1);}", pORMaster, nil, 2);
		Emit(fp, "for (u_uHunk = 0%; u_uHunk < %mw%; ++u_uHunk) {", pORMaster, acWhich, 1);
		Emit(fp, "if (0 != %a && u_uHunk+1 != %a) {continue;}\n", pORMaster, acWhich, 2);
		if (1 == pRG->uhunks) {
			Emit(fp, "fprintf(fp, \"%%s: %L: %%s\\n\", %b, %mT[u_uHunk]);", pORMaster, acWhich, 2);
		} else {
			Emit(fp, "fprintf(fp, \"%%s: %L: hunk %%d: %%s\\n\", %b, u_uHunk+1, %mT[u_uHunk]);", pORMaster, acWhich, 2);
		}
		Emit(fp, "for (u_ppc = %mV[u_uHunk]%; %ypi != *u_ppc%; ++u_ppc) {", pORMaster, acWhich, 2);
		Emit(fp, "fprintf(fp, \"%%s\\n\", *u_ppc);", pORMaster, nil, 3);
		Emit(fp, "}", pORMaster, nil, 3);
		Emit(fp, "}", pORMaster, nil, 2);
		Emit(fp, "}\n", pORMaster, nil, 1);
		if (!fExtern) {
			/* nothing */
		} else if (fAnsi) {
			Emit(fpCHeader, "extern void %mU(%yFb *, %yub);", pORMaster, nil, 0);
		} else {
			Emit(fpCHeader, "extern void %mU();", pORMaster, nil, 0);
		}
	}
}


/* if we gave this to the C compiler as a quoted string			(ksb)
 * what would strlen return?
 */
int
CountCs(pcStr)
char *pcStr;
{
	register int iLen;

	/* normal chars, \0, \00, \000, \\, \", \', \n and the like
	 * are all one character
	 */
	for (iLen = 0; '\000' != *pcStr; ++iLen) {
		if ('\\' != *pcStr++) {
			continue;
		}
		if ('0' <= *pcStr && *pcStr <= '7') {
			++pcStr;
			if ('0' <= *pcStr && *pcStr <= '7') {
				++pcStr;
			}
			if ('0' <= *pcStr && *pcStr <= '7') {
				++pcStr;
			}
			continue;
		}
		++pcStr;
	}
	return iLen;
}

/* pick C code to advance the set pointer to the current after del	(ksb)
 * or to span a fixed width mess.  Return the current input pointer.
 */
static char *
Skip(fp, pORIn, pcSet, pcCur, pcDel, iFail, pcTemp)
FILE *fp;
OPTION *pORIn;
char *pcSet, *pcCur, *pcDel, *pcTemp;
int iFail;
{
	register int iLen;
	auto char acStrlen[16];

	if ('\"' == pcDel[-1]) {
		if (0 != strcmp(pcSet, pcCur)) {
			Emit2(fp, "%a = %P;", nilOR, pcSet, pcCur, 1);
		}
		switch (iLen = CountCs(pcDel)) {
		case 0:
			if (0 == strcmp(pcTemp, pcCur)) {
				Emit2(fp, "while ('%mE' != *%a && !isspace(*%a))\n\t++%a;", pORIn, pcTemp, pcCur, 1);
			} else {
				Emit2(fp, "for (%a = %P%; '%mE' != *%a && !isspace(*%a)%; ++%a) /*empty*/;", pORIn, pcTemp, pcCur, 1);
			}
			Emit(fp, "if ('%mE' == *%a)\n", pORIn, pcTemp, 1);
			EmitN(fp, "return %a;", nilOR, iFail, 2);
			Emit(fp, "%a[0] = '\\000';", nilOR, pcTemp, 1);
			Emit(fp, "do\n\t++%a;while (isspace(*%a));", nilOR, pcTemp, 1);
			break;
		case 1:
			Emit(fp, "if ((char *)0 == (%a =", nilOR, pcTemp, 1);
			Emit2(fp, " strchr(%a, '%P')))\n", nilOR, pcCur, pcDel, 0);
			EmitN(fp, "return %a;", nilOR, iFail, 2);
			Emit(fp, "%a++[0] = '\\000';", nilOR, pcTemp, 1);
			break;
		default:
			Emit(fp, "if ((char *)0 == (%a =", nilOR, pcTemp, 1);
			Emit2(fp, " strstr(%a, \"%P\")))\n", nilOR, pcCur, pcDel, 0);
			EmitN(fp, "return %a;", nilOR, iFail, 2);
			Emit(fp, "%a[0] = '\\000';", nilOR, pcTemp, 1);
			sprintf(acStrlen, "%d", iLen);
			Emit2(fp, "%a += %P;", nilOR, pcTemp, acStrlen, 1);
			break;
		}
		return pcTemp;
	}
	if ('#' == pcDel[-1]) {
		register int iLen;
		if (0 == (iLen = atoi(pcDel))) {
			fprintf(stderr, "%s: column of zero width?\n", progname);
		}
		if (1 == iLen) {
			Emit(fp, "if ('%mE' == *%a)\n", pORIn, pcCur, 1);
			EmitN(fp, "return %a;", nilOR, iFail, 2);
			if (0 == strcmp(pcSet, pcCur)) {
				Emit2(fp, "++%a;", nilOR, pcCur, pcDel, 1);
				return pcCur;
			}
			Emit(fp, "%a = u_pcspace;", nilOR, pcSet, 1);
			Emit(fp, "*u_pcspace++ = *%a;", nilOR, pcCur, 1);
		} else {
			Emit2(fp, "if (strlen(%a) < %P || '\\n' == %a[%P-1])\n", nilOR, pcCur, pcDel, 1);
			EmitN(fp, "return %a;", nilOR, iFail, 2);
			if (0 == strcmp(pcSet, pcCur)) {
				Emit2(fp, "%a += %P;", nilOR, pcCur, pcDel, 1);
				return pcCur;
			}
			Emit2(fp, "%a = strncpy(u_pcspace, %P,", nilOR, pcSet, pcCur, 1);
			Emit(fp, " %a);", nilOR, pcDel, 0);
			Emit(fp, "u_pcspace += %a;", nilOR, pcDel, 1);
		}
		Emit(fp, "*u_pcspace++ = '\\000';", nilOR, pcDel, 1);
		if (0 != strcmp(pcTemp, pcCur)) {
			Emit(fp, "%a =", nilOR, pcTemp, 1);
			Emit2(fp, " %a + %P;", nilOR, pcCur, pcDel, 0);
		} else if (1 == iLen) {
			Emit2(fp, "++%a;", nilOR, pcTemp, pcDel, 1);
		} else {
			Emit2(fp, "%a += %P;", nilOR, pcTemp, pcDel, 1);
		}
		return pcTemp;
	}
	fprintf(stderr, "%s: internal flag inv. broken in Skip\n", progname);
	exit(MER_INV);
	/* NOTREACHED */
	return nil;
}

/* Code the read routine, this is called after the declaration of	(ksb)
 * the buffers and such, but before "main".
 * We can see them, and the user code that we are going to call.
 * If one of the buffers is _LOCAL we *could* pass the address
 * of the buffer and still work -- fine line there.
 *
 * We first drop hunk abort check functions because we need a simple
 *	if (line_fails(acLine, ...)) abort;
 * check at the top of the read loops, so we build them here.
 *
 * We need to skip past the data we want to convert,
 * this is the most important part of the file reading code:
 * any quoting or data specific rules should be applied here, but
 * we do not have any hooks (yet) into the type system to allow
 * special conversion/skip rules.
 *
 * For example a string of fixed width with no ending delim should
 * eat its dimension in characters, right?  What about tabs? What
 * about if it _has_ a delim.  We just don't worry about that now.
 *
 * N.B. the empty delimiter ("") is taken as `skip white space'.
 */
static void
mkroutine(fp, pORMaster, pRG)
FILE *fp;
OPTION *pORMaster;
ROUTINE *pRG;
{
	register R_LINE *pRL;
	register int i, iVec, iMaxVec, iLen, fHasCount;
	register char *pcLast, *pcDel;
	register OPTION *pOR;
	register int fAlt;		/* swap these two buffers */
	auto char acSwap[2][32];
	auto int fProcessed, iLabel;
	static char acTemp[] = "u_pctemp";

	fHasCount = 0;
	iMaxVec = 0;
	for (pRG->ppRL = & pRG->pRLfirst; (R_LINE *)0 != (pRL = pRG->ppRL[0]); pRG->ppRL = & pRL->pRLnext) {
		if (nil != pRL->pcemark && '#' == pRL->pcemark[0]) {
			fHasCount = 1;
		}
		if ((OPTION **)0 == pRL->ppORlorder) {
			Emit(fp, "/* no check for %mI, hunk %mW\n */\n", pORMaster, nil, 0);
			continue;
		}
		pRG->irattr |= ROUTINE_FMT;
		Emit(fp, "\n/* machine generated abort check for %mI, hunk %mW\n", pORMaster, nil, 0);
		if (0 != pRL->ufixwidth) {
			EmitN(fp, " * using %a bytes of fixed buffer space\n */\n", pORMaster, pRL->ufixwidth, 0);
			if (fAnsi) {
				Emit(fp, "static int %XmP(char *u_pcLine, char **u_ppcVec, char *u_pcspace)\n", pORMaster, nil, 0);
			} else {
				Emit(fp, "static int\n%XmP(u_pcLine, u_ppcVec, u_pcspace)\n", pORMaster, nil, 0);
				Emit(fp, "char *u_pcLine, **u_ppcVec, *u_pcspace;", pORMaster, nil, 0);
			}
		} else {
			Emit(fp, " */\n", pORMaster, nil, 0);
			if (fAnsi) {
				Emit(fp, "static int %XmP(char *u_pcLine, char **u_ppcVec)\n", pORMaster, nil, 0);
			} else {
				Emit(fp, "static int\n%XmP(u_pcLine, u_ppcVec)\n", pORMaster, nil, 0);
				Emit(fp, "char *u_pcLine, **u_ppcVec;", pORMaster, nil, 0);
			}
		}
		Emit(fp, "{", pORMaster, nil, 0);
		Emit(fp, "register char *%a;", pORMaster, acTemp, 1);
		Emit(fp, "\n", pORMaster, nil, 0);

		/* code below, decls above
		 */

		fAlt = iVec = 0;
		pcLast = "u_pcLine";
		if ((char *)0 != pRL->pcleader) {
			pcLast = Skip(fp, pORMaster, pcLast, pcLast, pRL->pcleader, -1, acTemp);
		}
		for (i = 0; i < pRL->icols; ++i) {
			register char *pc_;

			pOR = pRL->ppORlorder[i];
			if (nil == (pc_ = pOR->pchkeep)) {
				sprintf(acSwap[fAlt], "u_ppcVec[%d]", iVec++);
				pOR->pchkeep = acSwap[fAlt];
				fAlt ^= 1;
			}
			if (nil == (pcDel = pRL->ppcdelims[i])) {
				pcDel = (char *)(i+1 != pRL->icols ? "\"" : "\"\\n")+1;
			}

			/* if the next buffer has a keep and is a seeker
			 * try to get the tmp to be the keeper, this saves
			 * a *whole* assignment in the code, but looks so
			 * much smarter -- ksb
			 */
			if (i+1 == pRL->icols && acMyFix == pRG->pcarg1) {
				/* take the rest w/o looking (no \n to stomp)
				 */
				if (0 != strcmp(pOR->pchkeep, pcLast)) {
					Emit2(fp, "%a = %P;", nilOR, pOR->pchkeep, pcLast, 1);
				}
			} else if (i+1 < pRL->icols && nil != pRL->ppORlorder[i+1]->pchkeep && (nil == pRL->ppcdelims[i+1] || '\"' == pRL->ppcdelims[i+1][-1])) {
				pcLast = Skip(fp, pORMaster, pOR->pchkeep, pcLast, pcDel, i+1, pRL->ppORlorder[i+1]->pchkeep);
			} else {
				pcLast = Skip(fp, pORMaster, pOR->pchkeep, pcLast, pcDel, i+1, acTemp);
			}
			pOR->pchkeep = pc_;
		}
		Emit(fp, "return 0;}", pORMaster, nil, 1);
		if (iVec > iMaxVec) {
			iMaxVec = iVec;
		}
	}

	Emit(fp, "\n/* %W generated parser for (%XxB %Xxa) %L\n */\n", pORMaster, "", 0);

	if (0 != (pRG->irattr & ROUTINE_STATIC)) {
		Emit(fp, "static ", pORMaster, nil, 0);
	}
	if (fAnsi) {
		if (acMyFix == pRG->pcarg1) {
			Emit(fp, "int %mI(%ypb %Xypd)\n", pORMaster, pRG->pcarg1, 0);
		} else {
			Emit(fp, "int %mI(%XxB %Xxd)\n", pORMaster, pRG->pcarg1, 0);
		}
	} else if (acMyFix == pRG->pcarg1) {
		Emit(fp, "int\n%mI(%a)\n%ypb %Xypd;", pORMaster, pRG->pcarg1, 0);
	} else {
		Emit(fp, "int\n%mI(%a)\n%XxB %Xxd;", pORMaster, pRG->pcarg1, 0);
	}

	/* decls --
	 */
	Emit(fp, "{", pORMaster, nil, 0);
	if (acMyFix == pRG->pcarg1) {
		Emit(fp, "static unsigned %a, %mN = 0;", pORMaster, acHunkCnt, 1);
	} else {
		Emit(fp, "register unsigned %mN;", pORMaster, nil, 1);
	}
	if (fHasCount) {
		Emit2(fp, "%a unsigned %P = 0;", pORMaster, acMyFix == pRG->pcarg1 ? "static" : "register", acSpanCnt, 1);
	}
	if ((nil == pRG->pcraw && RNT_FMT(pRG)) || acMyFix != pRG->pcarg1) {
		Emit(fp, "auto int u_botch;", pORMaster, nil, 1);
	}
	if (acMyFix != pRG->pcarg1 && RNT_FMT(pRG)) {
		Emit(fp, "register char *%a;", pORMaster, acMyFix, 1);
	}
	if (acFd == pRG->pcarg1) {
		Emit(fp, "register FILE *%a;", pORMaster, acFp, 1);
	}
	if (0 != iMaxVec) {
		EmitN(fp, "auto char *u_apcCvt[%a];", pORMaster, iMaxVec, 1);
	}
	if (RNT_FIXED(pRG)) {
		EmitN(fp, "auto char u_acfixed[%a];", pORMaster, pRG->umaxfixed, 1);
	}

	/* code --
	 */
	Emit(fp, "%\n", pORMaster, nil, 0);

	/* remap fd to file* so we can read them
	 */
	if (acFd == pRG->pcarg1) {
		Emit(fp, "if ((FILE *)0 == (%a = fdopen(dup(%mp), \"r\"))) {fprintf(stderr, \"%%s: %L: %mI: fdopen: %%s\\n\", %b, %E);exit(2);}", pORMaster, acFp, 1);
	} else if (acMyFix == pRG->pcarg1) {
		Emit(fp, "if ((char *)0 == %a) {", pORMaster, acMyFix, 1);
		Emit(fp, "%mN = %a = 0;", pORMaster, acHunkCnt, 2);
		if (fHasCount) {
			Emit(fp, "%a = 0;", pORMaster, acSpanCnt, 2);
		}
		Emit(fp, "return 0;}", pORMaster, nil, 2);
	}
	/* We scan indirect through pRG's index such that the %escapes
	 * %m (and %M) can see what they need to work.
	 */
	iLabel = 0;
	if (acMyFix == pRG->pcarg1) {
		Emit(fp, "++%mN;", pORMaster, nil, 1);
		Emit(fp, "switch (%a) {", pORMaster, acHunkCnt, 1);
	} else {
		Emit(fp, "%mN = 0;", pORMaster, nil, 1);
	}
	/* for each hunk in the input stream:
	 *	count the vector temps we need,
	 *	drop the before code
	 *	check for end-of-hunk tokens and comments
	 *	then check for line count limits
	 *	then break-up and process the line
	 *	drop any after or cleanup code
	 */
	for (pRG->ppRL = & pRG->pRLfirst; (R_LINE *)0 != (pRL = pRG->ppRL[0]); pRG->ppRL = & pRL->pRLnext) {
		iVec = 0;
		if ((OPTION **)0 != pRL->ppORlorder) {
			for (i = 0; i < pRL->icols; ++i) {
				if (nil == pRL->ppORlorder[i]->pchkeep) {
					++iVec;
				}
			}
		}

		if (acMyFix == pRG->pcarg1) {
			if (nil != pRL->pcbefore) {
				EmitN(fp, "case %a:\n", pORMaster, iLabel++, 1);
				Emit(fp, pRL->pcbefore, pORMaster, nil, 2);
				Emit(fp, "++%a;", pORMaster, acHunkCnt, 2);
			}
			EmitN(fp, "case %a:\n", pORMaster, iLabel++, 1);
		} else {
			Emit(fp, pRL->pcbefore, pORMaster, nil, 1);
			if (nil != pRL->pcemark && '#' == pRL->pcemark[0]) {
				Emit(fp, "%mS = 0;", pORMaster, nil, 1);
			}
			if (nil == pRG->pcraw) {
				Emit(fp, "while ((char *)0 != (%mG = fgetln(%a, & u_botch))) {", pORMaster, acFp, 1); /*}*/
			} else {
				Emit(fp, "while ((char *)0 != fgets(%mG, sizeof(%mG), %a)) {", pORMaster, acFp, 1);
			}
			Emit(fp, "++%mN;", pORMaster, nil, 2);
		}

		/* LLL spaces on the end of _input_ vert delims are bad
		 */
		if (nil != pRL->pcemark && '\"' == pRL->pcemark[0] && 0 != (iLen = CountCs(pRL->pcemark+1))) {
			Emit(fp, "/* check for end of hunk token\n */\n", pORMaster, nil, 2);
			if (1 == iLen) {
				EmitN(fp, "if ('%qmt' == *%mG && '%mE' == %mG[1]) {", pORMaster, iLen, 2);
			} else if (acMyFix == pRG->pcarg1) {
				EmitN(fp, "if (0 == strncmp(%mG, \"%qmt\", %a)) {", pORMaster, iLen, 2);
			} else {
				EmitN(fp, "if (0 == strncmp(%mG, \"%qmt%mE\", %a)) {", pORMaster, iLen+1, 2);
			}
			/* end of hunk token in string routine:
			 * if the next hunk has a span reset the counter
			 */
			if (acMyFix == pRG->pcarg1) {
				Emit(fp, pRL->pcafter, pORMaster, nil, 3);
				Emit(fp, "++%a;", pORMaster, acHunkCnt, 3);
				if ((R_LINE *)0 != pRL->pRLnext && nil != pRL->pRLnext->pcemark && '#' == pRL->pRLnext->pcemark[0]) {
					Emit(fp, "%a = 0;", pORMaster, acSpanCnt, 3);
				}
				Emit(fp, "return 0;", pORMaster, nil, 3);
			} else {
				Emit(fp, "break;", pORMaster, nil, 3);
			}
			Emit(fp, "}", pORMaster, nil, 3);
		}
		if (nil != pRL->pcopenc && 0 != (iLen = CountCs(pRL->pcopenc))) {
			if (1 == iLen) {
				EmitN(fp, "if ('%qm#' == *%mG) {", pORMaster, iLen, 2);
			} else {
				EmitN(fp, "if (0 == strncmp(%mG, \"%qm#\", %a)) {", pORMaster, iLen, 2);
			}
			if (acMyFix == pRG->pcarg1) {
				Emit(fp, "return 1;", pORMaster, nil, 3); /*}*/
			} else {
				Emit(fp, "continue;", pORMaster, nil, 3); /*}*/
			}
			Emit(fp, "}", pORMaster, nil, 3); /*}*/
		}
		if (acMyFix != pRG->pcarg1 && nil != pRL->pcemark && '#' == pRL->pcemark[0]) {
			Emit(fp, "++%mS;", pORMaster, nil, 2);
		}

		/* nothing to check, always abort if we can?
		 */
		if ((OPTION **)0 == pRL->ppORlorder) {
			Emit(fp, pRL->pcaborts, pORMaster, "1", 2);
		} else {
			Emit2(fp, "if (0 != (u_botch = %XmP(%mG, %a%P))) {", pORMaster, 0 == iVec ? "(char **)0" : "u_apcCvt", 0 != pRL->ufixwidth ? ", u_acfixed" : "", 2);
			if (acMyFix == pRG->pcarg1) {
				Emit(fp, nil != pRL->pcaborts ? pRL->pcaborts : "fprintf(stderr, \"%%s: %qL(%%d): decode failed near column %%d\\n\", %b, %mN, %a);return %mN;" , pORMaster, "u_botch", 3);
			} else {
				Emit(fp, nil != pRL->pcaborts ? pRL->pcaborts : "fprintf(stderr, \"%%s: %%s(%%d): decode failed near column %%d\\n\", %b, %N, %mN, %a);return %mN;" , pORMaster, "u_botch", 3);
			}
			Emit(fp, "}", pORMaster, nil, 3);
		}

		fAlt = iVec = 0;
		fProcessed = 0;
		for (i = 0; i < pRL->icols; ++i) {
			register char *pc_;
			register OPTION *pOR_;

			pOR = pRL->ppORlorder[i];
			/* set special context for conversion from file
			 */
			if (nil == (pc_ = pOR->pchkeep)) {
				sprintf(acSwap[fAlt], "u_apcCvt[%d]", iVec++);
				pOR->pchkeep = acSwap[fAlt];
				fAlt ^= 1;
			}
			/* In this context the param is allowed by the
			 * controling trigger -- no matter that is might
			 * be allowed by another in another context.  (ksb)
			 */
			pOR_ = pOR->pORallow;
			pOR->pORallow = pORMaster;

			pOR->gattr |= GEN_PPPARAM;
			if (ISVERIFY(pOR)) {
				Emit(fp, pOR->pOTtype->pchchk, pOR, pOR->pchkeep, 2);
			}
			if (ISTRACK(pOR)) {
				Emit(fp, acTrack, pOR, nil, 2);
			}
			Emit(fp, pOR->pchuupdate, pOR, pOR->pchkeep, 2);
			Emit(fp, pOR->pchuser, pOR, pOR->pchkeep, 2);

			if (IS_ACT(pOR->pOTtype) || nil != pOR->pchuser) {
				fProcessed = 1;
			}
			/* restore normal context
			 */
			pOR->pORallow = pOR_;
			pOR->pchkeep = pc_;
		}
		/* We warn about read routines that just toss the data:
		 * if any of the column have function type or user updates
		 * we supress this.  Better would be to still warn if
		 * we toss the end of a line ("Data after col 10 is
		 * not processed") but that is way over the top, I think? (ksb)
		 */
		if (nil != pRL->pcupdate) {
			Emit(fp, pRL->pcupdate, pORMaster, "%mG", 2);
		} else if (nil != pRL->pcnamed) {
			Emit(fp, "%mn(%ml);", pORMaster, nil, 2);
		} else if (nil == pRL->pcuser && !fProcessed) {
			static char acHelp[] = "/* you need code here! */\n";
			register int iGccWallBroken;

			Emit(fp, acHelp, pORMaster, nil, 2);
			fprintf(stderr, "%s: %s: %s: no input line processing?\n", progname, usersees(pORMaster, nil), pRG->pcfunc);
			iGccWallBroken = sizeof(acHelp)-2;
			fprintf(stderr, " (see \"%*.*s\" in %s)\n", iGccWallBroken, iGccWallBroken, acHelp, pRG->pcfunc);
		}

		Emit(fp, pRL->pcuser, pORMaster, nil, 2);
		if ((char *)0 != pRL->pcemark && '#' == pRL->pcemark[0] && 0 != (iLen = atoi(pRL->pcemark+1))) {
			Emit(fp, "/* check for max lines in hunk\n */\n", pORMaster, nil, 2);
			if (acMyFix == pRG->pcarg1) {
				Emit(fp, "if (++%mS < %mt) {return 0;}", pORMaster, nil, 2);
				Emit(fp, pRL->pcafter, pORMaster, nil, 2);
				if ((R_LINE *)0 != pRL->pRLnext && nil != pRL->pRLnext->pcemark && '#' == pRL->pRLnext->pcemark[0]) {
					Emit(fp, "%mS = 0;", pORMaster, nil, 2);
				}
				Emit(fp, "++%a;", pORMaster, acHunkCnt, 2);
			} else {
				Emit(fp, "if (%mS == %mt) {break;}", pORMaster, acHunkCnt, 2);
			}
		}
		if (acMyFix == pRG->pcarg1) {
			Emit(fp, "break;", pORMaster, nil, 2);
			if ((R_LINE *)0 == pRL->pRLnext && nil != pRL->pcafter && nil == pRL->pcemark) {
				fprintf(stderr, "%s: %s: %s: after code is not reachable\n", progname, usersees(pORMaster, nil), pRG->pcfunc);
				Emit(fp, "/*NOTREACHED*/\n", pORMaster, nil, 1);
				EmitN(fp, "case %a:\n", pORMaster, iLabel++, 1);
				Emit(fp, pRL->pcafter, pORMaster, nil, 2);
				Emit(fp, "break;", pORMaster, nil, 2);
			}
		} else {
			Emit(fp, "}", pORMaster, nil, 2);
			Emit(fp, pRL->pcafter, pORMaster, nil, 1);
		}
	}
	if (acFd == pRG->pcarg1) {
		Emit(fp, "(void)fclose(%a);", pORMaster, acFp, 1);
	} else if (acMyFix == pRG->pcarg1) {
		Emit(fp, "}", pORMaster, nil, 2);
	}
	Emit(fp, "return 0;}\n", pORMaster, nil, 1);
	pRG->irattr |= ROUTINE_GEN;
}

/* output all the file readers requested by all options & params	(ksb)
 */
void
mkreaders(fpCBody, fpCHeader)
FILE *fpCBody, *fpCHeader;
{
	register OPTION *pOR;
	auto int s;

	for (s = 0, pOR = nilOR; nilOR != (pOR = OptScanAll(& s, pOR)); /**/) {
		if ((ROUTINE *)0 == pOR->pRG)
			continue;
		mkhelp(fpCBody, pOR, pOR->pRG, fpCHeader);
		mkroutine(fpCBody, pOR, pOR->pRG);
	}
}

/* output under the "FILES" section in the manual page			(ksb)
 * of course we could (should?) write a whole section.5 page
 * about the contents and format of the files we can read,
 * but we do not have the dataflow to produce the filename
 * for the manual page, or the _purpose_ of the ReadRoutine
 * we just built.
 */
static void
mkfilesec(fp, pORMaster, pRG)
FILE *fp;
OPTION *pORMaster;
ROUTINE *pRG;
{
	register R_LINE *pRL;
	register int i;
	register OPTION *pOR;
	register char *pcDel;

	Emit(fp, ".PP\nThe %L has a %W built routine to read data from the %xl \\fI%p\\fP.\n", pORMaster, nil, 0);
	Emit(fp, "These columns are converted from the input file:\n", pORMaster, nil, 0);
	for (pRG->ppRL = & pRG->pRLfirst; (R_LINE *)0 != (pRL = pRG->ppRL[0]); pRG->ppRL = & pRL->pRLnext) {
		Emit(fp, ".\\\" hunk %mW\n", pORMaster, nil, 0);
		if (nil != pRL->pchelp) {
			Emit(fp, ".br\n%Xmh\n", pORMaster, nil, 0);
		}
		for (i = 0; i < pRL->icols; ++i) {
			pOR = pRL->ppORlorder[i];
			if (nil == (pcDel = pRL->ppcdelims[i])) {
				pcDel = (i+1 == pRL->icols) ? "\\en" : " ";
			}
			Emit(fp, ".TP\n.CR %n %a\n", pOR, pcDel, 0);
			Emit(fp, "%h\n", pOR, pcDel, 0);
		}
	}
}

/* manual page code to output in the FILES section			(ksb)
 */
void
RLineMan(fp)
FILE *fp;
{
	auto int s;
	register OPTION *pOR;
	register int fFound;

	fFound = 0;
	for (s = 0, pOR = nilOR; nilOR != (pOR = OptScanAll(& s, pOR)); /**/) {
		if ((ROUTINE *)0 == pOR->pRG)
			continue;
		if (!fFound) {
			Emit(fp, ".SH FILES\n", pOR, nil , 0);
		}
		fFound = 1;
		mkfilesec(fp, pOR, pOR->pRG);
	}
}
