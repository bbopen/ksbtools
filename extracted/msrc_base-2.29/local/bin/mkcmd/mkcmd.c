/* $Id: mkcmd.c,v 8.29 2008/08/16 20:18:44 ksb Exp $
 * Based on some ideas from "mkprog", this program generates		(ksb)
 * a command line option parser based on a prototype file.
 * See the README file for usage of this program & the man page.
 *	$Compile: ${CC-cc} -c ${DEBUG--g} -I/usr/include/local %f
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

LIST
	LIComm,			/* header comments			*/
	aLIExits[11],		/* user hooks jusr before exit		*/
	aLIInits[11],		/* before before's before we run these	*/
	aLIAfters[11],		/* after afters's before we do params	*/
	LIExcl,			/* global mutual exclusions		*/
	LISysIncl,		/* included files for us		*/
	LIUserIncl;		/* included files for user		*/

int
	fBasename = 0,		/* find a base name for argv[0]		*/
	fMix = 0,		/* intermix options and arguments	*/
	iWidth = -1;		/* option col width for verbose help	*/
char
	*pchGetenv = nil,
	*pchProto = "%s_func",
	*pchTempl = "%s_opt",
	*pchMain = "main",
	*pchTerse = nil,
	*pchUsage = nil,
	*pchVector = nil,
	*pchProgName = "progname",
	**ppcBases = (char **)0,
	acKeepVal[] = "%N = %a;",
	acTrack[] = "%U = 1;",
	acFinger[] = "u_pckeyword",
	*pcKeyArg = (char *)0,		/* see argv[0] as this -> read [1] */
	*pcDefBaseOpt = (char *)0;


static char
	acParam[] = "%K<getopt_switches>2ev",
	acKeepQuoted[] = "%N = \"%qa\";",
	acArgOpt[] = "-1",
	acAuxOpt[] = "u_fNoArgs";

/* used to convert fprintf/%s to Emit/%a
 */
char ac_true[] = "1", ac_false[] = "0";

/* output an intro to the parser					(ksb)
 * add files to those to be included if need be
 */
/*ARGSUSED*/
void
mkintro(fp, fVerify, fpIncl, pLITop)
register FILE *fp, *fpIncl;
int fVerify;
LIST *pLITop;
{
	auto char acBuf[4094];
	register int n;
	auto LIST_INDEX u;
	register char **ppcComm;

	Emit(fp, "/* %W generated user interface\n", nilOR, nil, 0);
	for (ppcComm = ListCur(& LIComm, & u); 0 < u; --u) {
		Emit(fp, *ppcComm++, nilOR, nil, 0);
		Emit(fp, "%\n", nilOR, nil, 0);
	}
	Emit(fp, " */\n\n", nilOR, nil, 0);

	/* put the user include files after ours
	 */
	for (ppcComm = ListCur(& LIUserIncl, & u); 0 < u; --u) {
		ListAdd(& LISysIncl, *ppcComm++);
	}

	/* Output the sum of the include files, skip from "" which
	 * is used to tell mkcmd that something came from %c/%i magic.
	 * We could fix missing quotes here: if the word has a . in
	 * it like /only.h/ (ignore the slashes) then we know that no (single)
	 * CPP macro could expand to make that a legal header file name
	 * /<other.h/, (unless #define h i> is in effect-- then /<other.i>/
	 * would work).
	 * We can't tell for sure: so we do not even try. (That's a first.)
	 */
	for (ppcComm = ListCur(& LISysIncl, & u); 0 < u; --u, ++ppcComm) {
		if ('\000' == ppcComm[0][0]) {
			continue;
		}
		Emit(fp, "#include %a\n", nilOR, *ppcComm, 0);
	}

#if HAVE_SIMPLE_ERRNO
	if (fWeGuess) {
		if (IS_USED(CvtType('F')) || IS_USED(CvtType('D'))) {
			Emit(fp, "extern int errno;\n", nilOR, nil, 0);
		}
	}
#endif

#if !HAVE_STRERROR
	if (fWeGuess) {
		Emit(fp, "extern char *sys_errlist[];#define strerror(Me) (sys_errlist[Me])\n", nilOR, nil, 0);
	}
#endif

#if !USE_STDLIB && NEED_MALLOC_EXTERN
	if (fWeGuess) {
		Emit(fp, "extern char *malloc(), *calloc(), *realloc();\n", nilOR, nil, 0);
	}
#endif

	/* copy the template file into the parser, so
	 * we do not need a special library...
	 */
	while (0 < (n = fread(acBuf, sizeof(char), sizeof(acBuf), fpIncl))) {
		if (n == fwrite(acBuf, sizeof(char), n, fp)) {
			continue;
		}
		fprintf(stderr, "%s: transcription error from user includes to C file\n", progname);
		exit(MER_BROKEN);
	}

	GenMeta(fp, pLITop);
}

/* output the `track' booleans now (because we have a good spot)	(ksb)
 */
static int
mktrdecl(fp, iIndent, fGlobal, pcBase, fDefine)
register FILE *fp;
int iIndent, fGlobal, fDefine;		/* define vs declare the buffers */
char *pcBase;
{
	register OPTION *pOR;
	register int fAny, fNeedConv;
	auto int s;

	fAny = 0;
	for (s = 0, pOR = nilOR; nilOR != (pOR = OptScanAll(& s, pOR)); /**/) {
		if (!ISTRACK(pOR) || (0 != fGlobal) != ISTRGLOBAL(pOR)) {
			continue;
		}
		/* inv. fStatic == 0 in caller -- ksb
		 */
		if ((char *)0 != pcBase) {
			Emit(fp, "%a", nilOR, pcBase, iIndent);
			pcBase = (char *)0;
		} else {
			Emit(fp, ",", nilOR, nil, 0);
		}

		/* the track flag means we must have a conversion after
		 * the option processing and we don't have to look at
		 * runtime to know that.
		 */
		fNeedConv = ISLATE(pOR) && nil != pOR->pchinit && (IS_ACTIVE(pOR->pOTtype) || OPT_INITEXPR == ISWHATINIT(pOR));
		if (fDefine)
			EmitN(fp, "\n%U = %a", pOR, fNeedConv, iIndent+1);
		else
			Emit(fp, " %U", pOR, "", 0);
		fAny = 1;
	}
	return fAny;
}

/* output declarations AND fix up types (if 's' type)			(ksb)
 */
void
mkdecl(fp, iIndent, pchType, fGlobal, fStatic)
register FILE *fp;
register char *pchType;
int iIndent, fGlobal, fStatic;
{
	register OPTION **ppOR, *pORTemp;
	register OPTTYPE *pOT;
	auto OPTION *pORp, *pORq, *pORr, *pORk, *pORj, *pORl;
	auto OPTION *pORm, *pORs, *pORnm, *pORns, **ppORIndex;
	auto OPTTYPE *pOTPointer, *pOTBool;
	auto char acName[512];

	pOTPointer = CvtType('p');
	pOTBool = CvtType('b');
	pORk = pORp = pORRoot;
	pORj = pORq = pORDecl;
	pORl = pORr = pORActn;
	/* find special actions that need decls (allowed `every' or `list')
	 */
	pORnm = pORRoot;
	while (nilOR != pORnm && nilOR == pORnm->pORsact) {
		pORnm = pORnm->pORnext;
	}
	if (nilOR != (pORns = pORnm)) {
		pORm = pORnm->pORsact;
		pORs = pORns->pORsact;
	} else {
		pORm = pORs = nilOR;
	}
	while (nilOR != pORp || nilOR != pORq || nilOR != pORr || nilOR != pORs || nilOR != pORk || nilOR != pORj || nilOR != pORl || nilOR != pORm) {
		if (nilOR != pORp && (nil == pORp->pOTtype->pchbase || nil != pORp->pchfrom || ISALIAS(pORp) || nil == pORp->pOTtype->pchdecl || fGlobal != (OPT_GLOBAL & pORp->oattr))) {
			pORp = pORp->pORnext;
			continue;
		}
		if (nilOR != pORq && (nil == pORq->pOTtype->pchbase || nil != pORq->pchfrom || ISALIAS(pORq) || nil == pORq->pOTtype->pchdecl || fGlobal != (OPT_GLOBAL & pORq->oattr))) {
			pORq = pORq->pORnext;
			continue;
		}
		if (nilOR != pORr && (nil == pORr->pOTtype->pchbase || nil != pORr->pchfrom || ISALIAS(pORr) || nil == pORr->pOTtype->pchdecl || fGlobal != (OPT_GLOBAL & pORr->oattr))) {
			pORr = pORr->pORnext;
			continue;
		}
		if (nilOR != pORs && (nil == pORs->pOTtype->pchbase || nil != pORs->pchfrom || ISALIAS(pORs) || nil == pORs->pOTtype->pchdecl || fGlobal != (OPT_GLOBAL & pORs->oattr))) {
			pORs = pORs->pORnext;
			if (nilOR != pORs) {
				continue;
			}
			while (nilOR != (pORns = pORns->pORnext)) {
				if (nilOR != (pORs = pORns->pORsact)) {
					break;
				}
			}
			continue;
		}
		if (nilOR != pORk && (nil == pORk->pchkeep || fGlobal != (OPT_GLOBAL & pORk->oattr))) {
			pORk = pORk->pORnext;
			continue;
		}
		if (nilOR != pORj && (nil == pORj->pchkeep || fGlobal != (OPT_GLOBAL & pORj->oattr))) {
			pORj = pORj->pORnext;
			continue;
		}
		if (nilOR != pORl && (nil == pORl->pchkeep || fGlobal != (OPT_GLOBAL & pORl->oattr))) {
			pORl = pORl->pORnext;
			continue;
		}
		if (nilOR != pORm && (nil == pORm->pchkeep || fGlobal != (OPT_GLOBAL & pORm->oattr))) {
			pORm = pORm->pORnext;
			if (nilOR != pORm) {
				continue;
			}
			while (nilOR != (pORnm = pORnm->pORnext)) {
				if (nilOR != (pORm = pORnm->pORsact)) {
					break;
				}
			}
			continue;
		}

		/* pull off one item to decl
		 */
		ppORIndex = (OPTION **)0;
		if (nilOT != pOTBool && !fStatic && pchType == pOTBool->pchbase) {
			mktrdecl(fp, iIndent, fGlobal, (char *)0, 1);
			pOTBool = nilOT;
		}
		if (nilOR != pORm && pchType == pOTPointer->pchbase && fStatic == (pORm->oattr&OPT_STATIC)) {
			ppORIndex = & pORnm;
			ppOR = & pORm;
			pOT = pOTPointer;
		} else if (nilOR != pORk && pchType == pOTPointer->pchbase && fStatic == (pORk->oattr&OPT_STATIC)) {
			ppOR = & pORk;
			pOT = pOTPointer;
		} else if (nilOR != pORj && pchType == pOTPointer->pchbase && fStatic == (pORj->oattr&OPT_STATIC)) {
			ppOR = & pORj;
			pOT = pOTPointer;
		} else if (nilOR != pORl && pchType == pOTPointer->pchbase && fStatic == (pORl->oattr&OPT_STATIC)) {
			ppOR = & pORl;
			pOT = pOTPointer;
		} else if (nilOR != pORs && pchType == pORs->pOTtype->pchbase && fStatic == (pORs->oattr&OPT_STATIC)) {
			ppORIndex = & pORns;
			ppOR = & pORs;
			pOT = pORs->pOTtype;
		} else if (nilOR != pORr && pchType == pORr->pOTtype->pchbase && fStatic == (pORr->oattr&OPT_STATIC)) {
			ppOR = & pORr;
			pOT = pORr->pOTtype;
		} else if (nilOR != pORq && pchType == pORq->pOTtype->pchbase && fStatic == (pORq->oattr&OPT_STATIC)) {
			ppOR = & pORq;
			pOT = pORq->pOTtype;
		/* none that match, find any one to continue */
		} else if (nilOR != pORp) {
			ppOR = & pORp;
			pOT = pORp->pOTtype;
		} else if (nilOR != pORq) {
			ppOR = & pORq;
			pOT = pORq->pOTtype;
		} else if (nilOR != pORr) {
			ppOR = & pORr;
			pOT = pORr->pOTtype;
		} else if (nilOR != pORs) {
			ppORIndex = & pORns;
			ppOR = & pORs;
			pOT = pORs->pOTtype;
		} else if (nilOR != pORm) {
			ppORIndex = & pORnm;
			ppOR = & pORm;
			pOT = pOTPointer;
		} else if (nilOR != pORl) {
			ppOR = & pORl;
			pOT = pOTPointer;
		} else if (nilOR != pORj) {
			ppOR = & pORj;
			pOT = pOTPointer;
		} else {
			ppOR = & pORk;
			pOT = pOTPointer;
		}
		if (nilOR == (pORTemp = *ppOR)) {
			fprintf(stderr, "%s: internal error: mkdecl\n", progname);
			exit(MER_INV);
		}

		/* if we are searching sacts we have to update with care
		 */
		*ppOR = pORTemp->pORnext;
		if (nilOR == *ppOR && (OPTION **)0 != ppORIndex) {
			while (nilOR != (*ppORIndex = ppORIndex[0]->pORnext)) {
				if (nilOR != (*ppOR = ppORIndex[0]->pORsact)) {
					break;
				}
			}
		}

		if (IS_XBASE(pOT)) {
			/* assert pOT == pORTemp->pOTtype */
			if ((char *)0 != pchType)
				Emit(fp, ";", nilOR, nil, 0);
			Emit(fp, "%a%XZxB /* %Zxb */%\n", pORTemp, ISSTATIC(pORTemp) ? "static " : "", iIndent);
			pchType = (char *)0;
		} else if (pOT->pchbase != pchType || fStatic != (pORTemp->oattr&OPT_STATIC)) {
			if ((char *)0 != pchType)
				Emit(fp, ";", nilOR, nil, 0);
			pchType = pOT->pchbase;
			Emit2(fp, "%a%P%\n", pORTemp, ISSTATIC(pORTemp) ? "static " : "", pchType, iIndent);
			fStatic = pORTemp->oattr & OPT_STATIC;
		} else {
			Emit(fp, ",%\n", nilOR, nil, 0);
		}

		/* we have to force the keep for list to be (char **)
		 */
		if (ppOR == & pORk || ppOR == & pORj || ppOR == & pORl || ppOR == & pORm) {
			if (ISPTR2(pORTemp)) {
				Emit(fp, "**%N = (char **)0", pORTemp, nil, iIndent+1);
			} else if (OPT_INITCONST == ISWHATINIT(pORTemp) && pORTemp->pchinit != pORTemp->pOTtype->pchdef && !IS_ACTIVE(pORTemp->pOTtype)) {
				Emit(fp, "*%N = \"%qi\"", pORTemp, nil, iIndent+1);
			} else {
				Emit(fp, "*%N = %ypi", pORTemp, nil, iIndent+1);
			}
			if ((char *)0 == pchType) { /* can't happen */
				Emit(fp, ";", nilOR, nil, 0);
			}
			continue;
		}
		/* sprintf to re-map list conversion variable to a pointer
		 * to a datum (int *) or (char *([])) for example
		 */
		(void)mkid(pORTemp, acName);
		if (ISPTR2(pORTemp)) {
			Emit(fp, "%XxD", pORTemp, acName, iIndent+1);
		} else {
			Emit(fp, "%Xxd", pORTemp, acName, iIndent+1);
		}

		/* put on the initializer
		 */
		if (ISPTR2(pORTemp)) {
			Emit(fp, " = (%xb %XxA)0", pORTemp, "", 0);
		} else if (IS_ACTIVE(pOT) || OPT_INITCONST != ISWHATINIT(pORTemp)) {
			Emit(fp, " = %xi", pORTemp, nil, 0);
		} else if (nil != pORTemp->pchinit) {
			Emit(fp, " = %i", pORTemp, nil, 0);
		}
		if ((char *)0 == pchType) {
			Emit(fp, ";", nilOR, nil, 0);
		}
	}
	if ((char *)0 != pchType) {
		Emit(fp, ";", nilOR, nil, 0);
	}
	if (nilOT != pOTBool && mktrdecl(fp, iIndent, fGlobal, pOTBool->pchbase, 1)) {
		Emit(fp, ";", nilOR, nil, 0);
	}
}

/* output a call to the per-argument function				(ksb)
 */
static void
mkcall(fp, iIndent, ppORCntl)
FILE *fp;
int iIndent;
OPTION **ppORCntl;
{
	register OPTION *pORE;
	register char *pcEParam;

	pORE = newopt('e', ppORCntl, 0, 0);
	if (nilOR == pORE) {
		return;
	}
	pcEParam = StrExpand(acParam, pORE, (EMIT_OPTS *)0, 512);
	if (nil != pORE->pchkeep) {
		Emit(fp, acKeepVal, pORE, pcEParam, iIndent);
	}
	if (ISVERIFY(pORE)) {
		Emit(fp, (char *)0 != pORE->pchverify ? pORE->pchverify : pORE->pOTtype->pchchk, pORE, pcEParam, iIndent);
	}
	if (ISTRACK(pORE)) {
		Emit(fp, acTrack, pORE, nil, iIndent);
	}
	/* pORE->pchuupdate is not nil, checked in check.c
	 */
	Emit(fp, pORE->pchuupdate, pORE, pcEParam, iIndent);
	Emit(fp, pORE->pchuser, pORE, pcEParam, iIndent);

	if (ISABORT(pORE)) {
		Emit(fp, pORE->pchexit, pORE, "exit(1)", iIndent);
	}
}

/* make the switch/if/strcmps for the basename checks			(ksb)
 */
static void
mkbases(fp, ppcList, iCount, pcDef)
register FILE *fp;
register char **ppcList;
register int iCount;
char *pcDef;
{
	static char acRCheck[] = "if (0 != strcmp(%b, \"%qa\")) {";
	static char acRNext[] = "%} else if (0 != strcmp(%b, \"%qa\")) {";
	static char acCheck[] = "if (0 == strcmp(%b, \"%qa\")) {";
	static char acNext[] = "%} else if (0 == strcmp(%b, \"%qa\")) {";
	static char acClose[] = "%} else {";
	static char acThen[] = "u_envopt(\"%qa\", & argc, & argv);";
	static char acStop[] = "}";
	static char acNot[] = "/* nothing */\n";
	static char acIgnore[] = "/* \"%a\" forces no options */\n";
	register char *pcOpt, *pcCmp;
	register int i;
	register char ch;
	auto int fGoto;
	auto char acLable[20];
	static int iRand = 0xf01d;

	if ((char *)0 != pcDef && '\000' == *pcDef) {
		pcDef = (char *)0;
	}
	if (0 == iCount || ppcList[0][0] == ppcList[iCount-1][0]) {
		pcCmp = acCheck;
		for (i = 0; i < iCount; ++i) {
			pcOpt = ppcList[i] + 1 + strlen(ppcList[i]);
			if ('\000' == pcOpt[0]) {
				if (nil == pcDef) {
					Emit(fp, acIgnore, nilOR, ppcList[i], 1);
					continue;
				}
				if (i == iCount - 1) {
					if (acCheck == pcCmp)
						Emit(fp, acRCheck, nilOR, ppcList[i], 1);
					else
						Emit(fp, acRNext, nilOR, ppcList[i], 1);
					Emit(fp, acThen, nilOR, pcDef, 2);
					Emit(fp, acStop, nilOR, ppcList[i], 2);
					return;
				}
			}
			Emit(fp, pcCmp, nilOR, ppcList[i], 1);
			if ('\000' == pcOpt[0]) {
				Emit(fp, acNot, nilOR, pcOpt, 2);
			} else {
				Emit(fp, acThen, nilOR, pcOpt, 2);
			}
			pcCmp = acNext;
		}
		if (nil != pcDef) {
			if (acCheck == pcCmp) {
				Emit(fp, acThen, nilOR, pcDef, 1);
			} else {
				Emit(fp, acClose, nilOR, "", 1);
				Emit(fp, acThen, nilOR, pcDef, 2);
			}
		}
		if (acCheck != pcCmp ) {
			Emit(fp, acStop, nilOR, ppcList[i], 2);
		}
		return;
	}

	/* if the first letters are different we only compare to
	 * the names that match our progname
	 */
	Emit(fp, "switch (%b[0]) {", nilOR, (char *)0, 1);
	ch = ppcList[0][0] - 1;
	pcCmp = (char *)0;
	sprintf(acLable, "u_l%04x", iRand++);
	fGoto = 0;
	for (i = 0; i < iCount; ++i) {
		pcOpt = ppcList[i] + 1 + strlen(ppcList[i]);
		if (ch != ppcList[i][0]) {
			static char acBuf[2];
			acBuf[1] = '\000';
			acBuf[0] = ch = ppcList[i][0];
			if ((char *)0 != pcCmp) {
				if ((char *)0 == pcDef) {
					/* nothing */;
				} else if (!fGoto) {
					fGoto = 1;
					Emit(fp, acClose, nilOR, ppcList[i], 2);
					Emit(fp, "%a:\ndefault:\n", nilOR, acLable, 1);
					Emit(fp, acThen, nilOR, pcDef, 3);
				} else {
					Emit(fp, "%} else {goto %a;", nilOR, acLable, 2);
				}
				Emit(fp, "}break;", nilOR, ppcList[i], 3);
			}
			Emit(fp, "case '%a':\n", nilOR, acBuf, 1);
			pcCmp = acCheck;
		}
		Emit(fp, pcCmp, nilOR, ppcList[i], 2);
		if ('\000' == pcOpt[0])
			Emit(fp, acNot, nilOR, pcOpt, 3);
		else
			Emit(fp, acThen, nilOR, pcOpt, 3);
		pcCmp = acNext;
	}
	if ((char *)0 != pcDef) {
		Emit(fp, "%} else {goto %a;}", nilOR, acLable, 2);
		Emit(fp, "break;}", nilOR, (char *)0, 2);
	} else {
		Emit(fp, acStop, nilOR, ppcList[i], 3);
		Emit(fp, "default:\n\tbreak;%}\n", nilOR, (char *)0, 1);
	}
}

/* make the call to u_getopt						(ksb)
 */
static void
mkswitch(fp, pchZA, pcExit)
register FILE *fp;
char *pchZA;
char *pcExit;
{
	register OPTION *pORA;
	register OPTION *pOR;
	register char *pc_, *pcEParam;

	pcEParam = StrExpand(acParam, nilOR, (EMIT_OPTS *)0, 512);
	if (fMix) {
		Emit(fp, "for (%;%;) {if (EOF != (%w = %O)) {"/*}*/, nilOR, nil, 1);
		Emit(fp, "/* fallthrough */;", nilOR, nil, 3);
		Emit(fp, "%} else if (EOF != %A) {", nilOR, nil, 2);
		mkcall(fp, 3, & pORActn);
		if (nil != pchZA) {
			Emit(fp, "%a = 0;", nilOR, pchZA, 3);
		}
		Emit(fp, "continue;", nilOR, nil, 3);
		Emit(fp, "%} else {break;}", nilOR, nil, 2);
	} else {
		Emit(fp, "while (EOF != (%w = %O)) {"/*}*/, nilOR, nil, 1);
	}
	Emit(fp, "switch (%w) {"/*}*/, nilOR, nil, 2);

	for (pOR = pORRoot; nilOR != pOR; pOR = pOR->pORnext) {
		if (ISALIAS(pOR))
			continue;
		for (pORA = pOR->pORalias; nilOR != pORA; pORA = pORA->pORalias)
			Emit(fp, "%S\n", pORA, nil, 2);
		Emit(fp, "%S\n", pOR, nil, 2);
		if (ISALLOWED(pOR)) {
			Emit(fp, "if (0 == %CU) {fprintf(stderr, \"%%s: %L may only be given after %CL\\n\", %b);exit(1);}", pOR, nil, 3);
		}
		if (ISSTRICT(pOR) || ISFORBID(pOR)) {
			Emit(fp, "u_chkonly(\'%l\', %w, \"%Zf\");", pOR, nil, 3);
		}
		pc_ = (sbBInit != UNALIAS(pOR)->pOTtype->pchdef) ? pcEParam : sbPInit;
		if (ISTRACK(pOR)) {
			Emit(fp, acTrack, pOR, nil, 3);
		}
		if ((char *)0 != pOR->pchkeep) {
			Emit(fp, acKeepVal, pOR, pc_, 3);
		}
		/* check pcnoparam which finds the parametric data through
		 * some other interface if we can't see it here.  Fixes the
		 * keep value so we can go on.
		 */
		if (nil != pOR->pcnoparam) {
			Emit(fp, "if (%ypi == %N) {", pOR, pc_, 3);
			Emit(fp, pOR->pcnoparam, pOR, pc_, 4);
			Emit(fp, "}", pOR, pc_, 4);
		}
		if (ISLATE(pOR)) {
			Emit(fp, "/* late conversion */\n", pOR, pc_, 3);
			Emit(fp, "%a;", pOR, (ISABORT(pOR)||ISENDS(pOR)||ISBREAK(pOR) ? "break" : "continue"), 3);
			continue;
		}
		if (ISVERIFY(pOR)) {
			Emit(fp, pOR->pchverify, pOR, pc_, 3);
		}
		Emit(fp, pOR->pchuupdate, pOR, pc_, 3);
		Emit(fp, pOR->pchuser, pOR, pc_, 3);
		if (ISENDS(pOR)) {
			Emit(fp, ((char *)0 != pOR->pcends ? pOR->pcends : pcExit), pOR, pcExit, 3);
		} else {
			Emit(fp, (ISABORT(pOR) ? pOR->pchexit : "%a;"), pOR, (ISBREAK(pOR) ? "break" : "continue"), 3);
		}
	}

	/* balance {{ */
	Emit(fp, "}break;}", nilOR, nil, 3);
}

/* output the verbose usage function					(ksb)
 */
void
mkusage(fp, pch)
register FILE *fp;
char *pch;
{
	Emit(fp, "/*\n%ca usage function (for compatibility with V2)\n */\n", nilOR, nil, 0);
	if (fAnsi) {
		Emit(fp, "void %a(void)\n", nilOR, pch, 0);
	} else {
		Emit(fp, "int\n%a()\n", nilOR, pch, 0);
	}
	Emit2(fp, "{register char **ppch;for (ppch = a%t%; %ypi != *ppch%; ++ppch) {if ('\\000' == **ppch) {printf(\"%%s: with no parameters\\n\", %b);continue;}printf(\"%%s: usage%%s\\n\", %b, *ppch);}for (ppch = %v%; %ypi != *ppch%; ++ppch) {printf(\"%%s\\n\", *ppch);}%P}\n", nilOR, pch, (fAnsi ? "/* void */\n" : "return 0;\n"), 0);
}


/* if an option record ends option and command line processing		(ksb)
 * we hold its state sticky in an option pointer (reset by caller)
 */
static void
trackends(fp, iLevel, pORNow, ppOREnded, pcExit)
FILE *fp;
int iLevel;
OPTION *pORNow, **ppOREnded;
char *pcExit;
{
	if (nilOR == pORNow || !ISENDS(pORNow)) {
		return;
	}
	Emit(fp, (nil != pORNow->pcends ? pORNow->pcends : pcExit), pORNow, pcExit, iLevel);
	if (nilOR == *ppOREnded) {
		*ppOREnded = pORNow;
		return;
	}
	fprintf(stderr, "%s: %s ends the command line", progname, usersees(*ppOREnded, nil));
	fprintf(stderr, ", but %s follows\n", usersees(pORNow, nil));
	iExit |= MER_SEMANTIC;
}

/* xlate array, list has a funny type so convert them all		(ksb)
 * calloc space
 * loop and xlate argv
 * assign a sentinal value
 * update with indir list, rather than argv
 */
static void
mkxlate(fp, iLevel, pORList)
FILE *fp;
int iLevel;
OPTION *pORList;
{
	register char *pcHoldName, *pcHoldKeep;
	static char acI[] = "u_i";
	auto char acThis[1024], acRaw[1024], acBuf[512];

	pcHoldName = pORList->pchname;
	pcHoldKeep = pORList->pchkeep;

	Emit(fp, "{", pORList, nil, iLevel);
	Emit(fp, "register int %a;", pORList, acI, iLevel+1);
	Emit(fp, "if ((%XxB %XxA)0 == (%n = (%XxB %XxA)calloc(%#, sizeof(%XxB %Xxd)))) {", pORList, "", iLevel+1);
	Emit(fp, "fprintf(stderr, \"%%s: out of memory\\n\", %b);exit(1);", pORList, nil, iLevel+2);
	Emit(fp, "}", pORList, nil, iLevel+2);

	/* remap the name and such for a tricky emit stanza
	 */
	sprintf(acThis, "%s[%s]", mkid(pORList, acBuf), acI);
	sprintf(acRaw, "%s[%s]", nil != pcHoldKeep ? pcHoldKeep : StrExpand("(argv+%K<getopt_switches>1ev)", pORList, (EMIT_OPTS *)0, 512), acI);
	pORList->pchname = acThis;
	pORList->pchkeep = acRaw;

	Emit(fp, "for (%a = 0%; %a < %#%; ++%a) {", pORList, acI, iLevel+1);
	Emit(fp, "%Xxu/*Yow*/\n", pORList, acBuf, iLevel+2);
	Emit(fp, "}}", pORList, nil, iLevel+2);

	/* put back the real name for the user code to see
	 */
	pORList->pchname = pcHoldName;
	pORList->pchkeep = pcHoldKeep;
}

/* emit a "init" or "cleanup" list					(ksb)
 * if fDirection < 0 got to go backwards
 */
static void
MkTodo(fp, iLevel, pORWRT, pLIQueue, pcParam, fDirection)
FILE *fp;
int iLevel;
OPTION *pORWRT;
LIST *pLIQueue;
char *pcParam;
int fDirection;
{
	register int i, j;
	register char **ppcExit;
	auto LIST_INDEX u;
	auto char acCvt[8];

	for (i = 0; i < 11; ++i) {
		j = fDirection > 0 ? i : 10 - i;
		sprintf(acCvt, "%d", j);
		for (ppcExit = ListCur(& pLIQueue[j], & u); 0 < u; --u) {
			Emit2(fp, *ppcExit++, pORWRT, acCvt, pcParam, 1);
		}
	}
}

/* output the control points for the end of the command line		(ksb)
 */
static void
mkcontrol(fp, iLevel, ppORCtl, pchZA, pcJustDone, pcExit)
FILE *fp;
int iLevel;
OPTION **ppORCtl;
char *pchZA, *pcJustDone;
char *pcExit;		/* escape from the parser routine */
{
	register OPTION *pOR, *pORExit, *pORLeft, *pORRight;
	auto OPTION *pORZero, *pORList, *pOREvery;
	auto int fListCvt, fEveryTrap;
	auto int iLeftMand, iRightMand, iMands;
	auto char *pcELParam;
	static char acNotJP[] =  "%s: binding an action, boolean, or toggle to an argument makes no sense\n";
	static char acRParam[] = "argv[argc]";
	static char acNotReached[] = "/*NOTREACHED*/\n";
	auto char *pcJKeep;
	auto OPTION *pOREnds;

	/* To generate the left/right afters we mark each PPARAM we can assign
	 * and only `after' those we might touch.  We use the GEN_PPPARAM
	 * bit for this which we reset as we produce the code.
	 */
	pcJKeep = pcJustEscape;
	pcJustEscape = pcJustDone;

	pORLeft = newopt('s', ppORCtl, 0, 0);
	pORRight = newopt('d', ppORCtl, 0, 0);
	pORList = newopt('l', ppORCtl, 0, 0);
	pORZero = newopt('z', ppORCtl, 0, 0);
	pOREvery = newopt('e', ppORCtl, 0, 0);
	pORExit = newopt('x', ppORCtl, 0, 0);
	pOREnds = nilOR;

	pcELParam = StrExpand("argv[%K<getopt_switches>1ev]", pORLeft, (EMIT_OPTS *)0, 512);

	/* we need to keep track of a flag for `zero words are left after
	 * options' only if:
	 *	+ an every { } is visible that aborts
	 * else ignore the variable, some other scope uses it.
	 */
	fEveryTrap = (nilOR != pOREvery && ISABORT(pOREvery));
	if (! fEveryTrap && ! fMix) {
		pchZA = (char *)0;
	}

	fListCvt = nilOR != pORList && !IS_ACT(pORList->pOTtype);
	iLeftMand = (nilOR == pORLeft || 0 == pORLeft->iorder || ISOOPEN(pORLeft->ppORorder[0]) ? 0 : pORLeft->ppORorder[0]->ibundle);
	iRightMand = (nilOR == pORRight || 0 == pORRight->iorder || ISOOPEN(pORRight->ppORorder[0]) ? 0 : pORRight->ppORorder[0]->ibundle);
	if (nilOR != pORLeft) {
		EmitN(fp, pORLeft->pchbefore, pORLeft, iLeftMand, iLevel);
	}
	if (nilOR != pORRight) {
		EmitN(fp, pORRight->pchbefore, pORRight, iRightMand, iLevel);
	}
	if (0 != (iMands = iLeftMand + iRightMand)) {
		register char *pcCAbort;
		static char acMAbrt[] = "fprintf(stderr, \"%%s: not enough arguments for %a mandatory parameters\\n\", %b);exit(1);";
		static char acLAbrt[] = "fprintf(stderr, \"%%s: not enough arguments for left mandatory parameters\\n\", %b);exit(1);";
		static char acRAbrt[] = "fprintf(stderr, \"%%s: not enough arguments for right mandatory parameters\\n\", %b);exit(1);";

		/* pick an output format for missing left/rights
		 * Sj Safdar wanted fine control over this
		 */
		if (0 == iLeftMand) {
			if (nil != pORRight->pchexit)
				pcCAbort = pORRight->pchexit;
			else
				pcCAbort = acRAbrt;
		} else if (0 == iRightMand) {
			if (nil != pORLeft->pchexit)
				pcCAbort = pORLeft->pchexit;
			else
				pcCAbort = acLAbrt;
		} else if (nil == pORLeft->pchexit && nil == pORRight->pchexit) {
			pcCAbort = acMAbrt;
		} else {
			pcCAbort = nil;
		}
		if (nil != pcCAbort) {
			EmitN(fp, "if (argc - %K<getopt_switches>1ev < %a) {", nilOR, iMands, iLevel);
			EmitN(fp, pcCAbort, nilOR, iMands, iLevel+1);
			Emit(fp, "}", nilOR, nil, iLevel+1);
		} else {
			EmitN(fp, "if (argc - %K<getopt_switches>1ev < %a) {", nilOR, iLeftMand, iLevel);
			EmitN(fp, nil == pORLeft->pchexit ? acLAbrt : pORLeft->pchexit, nilOR, iLeftMand, iLevel+1);
			EmitN(fp, "%} else if (argc - %K<getopt_switches>1ev < %a) {", nilOR, iMands, iLevel);
			EmitN(fp, nil == pORRight->pchexit ? acRAbrt : pORRight->pchexit, nilOR, iRightMand, iLevel+1);
			Emit(fp, "}", nilOR, nil, iLevel+1);
		}
	}
	if (nilOR != pORLeft) {
		register int i;
		register OPTION *pORCur;

		for (i = 0; i < iLeftMand; ++i) {
			pORCur = pORLeft->ppORorder[i];
			if (sbBInit == UNALIAS(pORCur)->pOTtype->pchdef) {
				fprintf(stderr, acNotJP, progname);
				iExit |= MER_SEMANTIC;
				continue;
			}
			pORCur->gattr |= GEN_PPPARAM;
			if (nil != pORCur->pchkeep) {
				Emit(fp, acKeepVal, pORCur, pcELParam, iLevel);
			}
			if (ISVERIFY(pORCur)) {
				Emit(fp, pORCur->pOTtype->pchchk, pORCur, pcELParam, iLevel);
			}
			if (ISTRACK(pORCur)) {
				Emit(fp, acTrack, pORCur, nil, iLevel);
			}
			Emit(fp, pORCur->pchuupdate, pORCur, pcELParam, iLevel);
			Emit(fp, pORCur->pchuser, pORCur, pcELParam, iLevel);
			Emit(fp, "++%K<getopt_switches>1ev;", pORCur, "", iLevel);
		}
		if (iLeftMand >= pORLeft->iorder) {
			if (nil != pORLeft->pchkeep) {
				Emit(fp, acKeepVal, pORLeft, sbPInit, iLevel);
			}
			Emit(fp, pORLeft->pchuupdate, pORLeft, sbPInit, iLevel);
			Emit(fp, pORLeft->pchuser, pORLeft, sbPInit, iLevel);
		}
	}

	if (nilOR != pORRight) {
		register unsigned i;
		register OPTION *pORCur;

		i = pORRight->iorder;
		while (i-- > 0) {
			pORCur = pORRight->ppORorder[i];
			if (sbBInit == UNALIAS(pORCur)->pOTtype->pchdef) {
				fprintf(stderr, acNotJP, progname);
				iExit |= MER_SEMANTIC;
				continue;
			}
			Emit(fp, "--argc;", nilOR, "", iLevel);
			pORCur->gattr |= GEN_PPPARAM;
			if (nil != pORCur->pchkeep) {
				Emit(fp, acKeepVal, pORCur, acRParam, iLevel);
			}
			if (ISVERIFY(pORCur)) {
				Emit(fp, pORCur->pOTtype->pchchk, pORCur, acRParam, iLevel);
			}
			if (ISTRACK(pORCur)) {
				Emit(fp, acTrack, pORCur, nil, iLevel);
			}
			Emit(fp, pORCur->pchuupdate, pORCur, acRParam, iLevel);
			Emit(fp, pORCur->pchuser, pORCur, acRParam, iLevel);
		}
		Emit(fp, "argv[argc] = %a;", nilOR, sbPInit, iLevel);
		if (nil != pORRight->pchkeep) {
			Emit(fp, acKeepVal, pORRight, sbPInit, iLevel);
		}
		Emit(fp, pORRight->pchuupdate, pORRight, sbPInit, iLevel);
		Emit(fp, pORRight->pchuser, pORRight, sbPInit, iLevel);
	}

	/* optional params
	 */
	if (nilOR != pORLeft && iLeftMand < pORLeft->iorder) {
		register int i, j, iBundle;
		register OPTION *pORCur;

		for (i = iLeftMand; i < pORLeft->iorder; i += iBundle) {
			iBundle = pORLeft->ppORorder[i]->ibundle;
			Emit(fp, "if (argc - %K<getopt_switches>1ev > 0 && '-' == argv[%K<getopt_switches>1ev][0] && '-' == argv[%K<getopt_switches>1ev][1] && '\\000' == argv[%K<getopt_switches>1ev][2]) {++%K<getopt_switches>1ev;goto %j;}", nilOR, nil, iLevel);
			EmitN(fp, "if (argc - %K<getopt_switches>1ev < %a) {", nilOR, iBundle, iLevel);
			Emit(fp, "goto %j;", nilOR, nil, iLevel+1);
			Emit(fp, "}", nilOR, nil, iLevel+1);
			for (j = 0; j < iBundle; ++j) {
				pORCur = pORLeft->ppORorder[i+j];
				if (sbBInit == UNALIAS(pORCur)->pOTtype->pchdef) {
					fprintf(stderr, acNotJP, progname);
					exit(MER_SEMANTIC);
				}
				pORCur->gattr |= GEN_PPPARAM;
				if (nil != pORCur->pchkeep) {
					Emit(fp, acKeepVal, pORCur, pcELParam, iLevel);
				}
				if (ISVERIFY(pORCur)) {
					Emit(fp, pORCur->pOTtype->pchchk, pORCur, pcELParam, iLevel);
				}
				if (ISTRACK(pORCur)) {
					Emit(fp, acTrack, pORCur, nil, iLevel);
				}
				Emit(fp, pORCur->pchuupdate, pORCur, pcELParam, iLevel);
				Emit(fp, pORCur->pchuser, pORCur, pcELParam, iLevel);
				Emit(fp, "++%K<getopt_switches>1ev;", nilOR, "", iLevel);
			}
		}
		/* we should eat the "--" sep even if it is not *used*
		 */
		Emit(fp, "if (argc - %K<getopt_switches>1ev > 0 && '-' == argv[%K<getopt_switches>1ev][0] && '-' == argv[%K<getopt_switches>1ev][1] && '\\000' == argv[%K<getopt_switches>1ev][2]) {++%K<getopt_switches>1ev;}", nilOR, "", iLevel);
		if (nil != pORLeft->pchkeep) {
			Emit(fp, acKeepVal, pORLeft, sbPInit, iLevel);
		}
		Emit(fp, pORLeft->pchuupdate, pORLeft, sbPInit, iLevel);
		Emit(fp, pORLeft->pchuser, pORLeft, sbPInit, iLevel);
		Emit(fp, "%j: /* escape optional left arguments */;", nilOR, sbPInit, 0);
	}

	for (pOR = pORDecl; nilOR != pOR; pOR = pOR->pORnext) {
		if (!DIDPPPARAM(pOR)) {
			continue;
		}
		pOR->gattr &= ~GEN_PPPARAM;
		Emit(fp, pOR->pchafter, pOR, nil, iLevel);
		trackends(fp, iLevel, pOR, & pOREnds, pcExit);
	}
	if (nilOR != pORRight) {
		Emit(fp, pORRight->pchafter, pORRight, sbPInit, iLevel);
		trackends(fp, iLevel, pORRight, & pOREnds, pcExit);
	}
	if (nilOR != pORLeft) {
		Emit(fp, pORLeft->pchafter, pORLeft, sbPInit, iLevel);
		trackends(fp, iLevel, pORLeft, & pOREnds, pcExit);
	}

	/* we have the forced left/right params, now we need to process the
	 *	[zero] [list]  [every [every-abort]]  [exit]
	 *
	 * If `zero' aborts we move it up above `list'
	 * If `every' aborts we check after all of them are consumed
	 * If `list' aborts we do it in the normal pace, warning if there
	 * is an every?
	 */
	if (nilOR != pORZero) {
		Emit(fp, pORZero->pchbefore, pORZero, sbPInit, iLevel);
	}
	if (nilOR != pORList) {
		Emit(fp, pORList->pchbefore, pORList, sbPInit, iLevel);
	}
	if (nilOR != pOREvery) {
		Emit(fp, pOREvery->pchbefore, pOREvery, sbPInit, iLevel);
	}
	if (nilOR != pORZero) {
		register char *pchZ;

		pchZ = nil != pORZero->pchuupdate ? pORZero->pchuupdate : sbActn;
		if (fMix) {
			Emit(fp, "if (%a) {", nilOR, pchZA, iLevel);
		} else {
			Emit(fp, "if (%K<getopt_switches>1ev == argc) {", nilOR, pchZA, iLevel);
		}
		if (nil != pORZero->pchkeep) {
			Emit(fp, acKeepVal, pORZero, sbPInit, iLevel+1);
		}
		Emit(fp, pchZ, pORZero, sbPInit, iLevel+1);
		Emit(fp, pORZero->pchuser, pORZero, sbPInit, iLevel+1);
		Emit(fp, (ISABORT(pORZero) ? pORZero->pchexit : nil), pORZero, sbBInit, iLevel+1);
		Emit(fp, "}", nilOR, "", iLevel+1);
		Emit(fp, pORZero->pchafter, pORZero, sbPInit, iLevel);
		trackends(fp, iLevel, pORZero, & pOREnds, pcExit);
	}
	if (nilOR != pORList) {
		register char *pcEList, *pcECount;

		pcEList = StrExpand("& argv[%K<getopt_switches>1ev]", pORList, (EMIT_OPTS *)0, 512);
		pcECount = StrExpand("%#", pORList, (EMIT_OPTS *)0, 512);
		/* is pchkeep forced to the correct type in the decl
		 */
		if (nil != pORList->pchkeep) {
			Emit2(fp, acKeepVal, pORList, pcEList, pcECount, iLevel);
		}
		if (fListCvt) {
			mkxlate(fp, iLevel, pORList);
		} else {
			Emit2(fp, pORList->pchuupdate, pORList, pcEList, pcECount, iLevel);
		}
		Emit2(fp, pORList->pchuser, pORList, pcEList, pcECount, iLevel);
		Emit(fp, (ISABORT(pORList) ? pORList->pchexit : nil), pORList, sbBInit, iLevel);
		Emit(fp, pORList->pchafter, pORList, sbPInit, iLevel);
		trackends(fp, iLevel, pORList, & pOREnds, pcExit);
	}
	if (! fMix && nilOR != pOREvery) {
		Emit(fp, "%\n", nilOR, nil, 0);
		if (nil != pchZA) {
			Emit(fp, "%a = 1;", nilOR, pchZA, iLevel);
		}
		Emit(fp, "while (EOF != u_getarg(argc, argv)) {", nilOR, (char *)0, iLevel);
		/* balance }
		 */
		if (nil != pchZA) {
			Emit(fp, "%a = 0;", nilOR, pchZA, iLevel+1);
		}
		mkcall(fp, iLevel+1, ppORCtl); /*{*/
		Emit(fp, "}", nilOR, "", iLevel+1);
		Emit(fp, pOREvery->pchafter, pOREvery, sbPInit, iLevel);
		trackends(fp, iLevel, pOREvery, & pOREnds, pcExit);
		if (fEveryTrap) {
			Emit(fp, "if (!(%a)) {", pOREvery, pchZA, iLevel);
			Emit(fp, pOREvery->pchexit, pOREvery, pOREvery->pchdesc, iLevel+1);
			Emit(fp, "}", pOREvery, "", iLevel+1);
			fEveryTrap = 0;
		}
	}

	/* now just the exit and cleanup code
	 */
	if (nilOR != pORExit) {
		Emit(fp, pORExit->pchbefore, pORExit, "0", iLevel);
	}
	MkTodo(fp, iLevel, pORExit, aLIExits, pcJKeep, -1);
	if (nilOR != pORExit) {
		register char *pchX;

		pchX = nil != pORExit->pchuupdate ? pORExit->pchuupdate : sbActn;
		if (nil != pORExit->pchkeep) {
			Emit(fp, acKeepVal, pORExit, sbPInit, iLevel);
		}
		if ('\000' != pchX[0]) {
			Emit(fp, pchX, pORExit, sbPInit, iLevel);
		}
		Emit(fp, pORExit->pchuser, pORExit, sbPInit, iLevel);
		Emit(fp, (ISABORT(pORExit) ? pORExit->pchexit : pcExit), pORExit, sbBInit, iLevel);
	} else {
		Emit(fp, pcExit, pORExit, sbBInit, iLevel);
	}

	/* apply lint fix, or user supplied lint fix
	 */
	if (nilOR != pORExit) {
		register char *pchX;
		pchX = nil != pORExit->pchafter ? pORExit->pchafter : acNotReached;
		Emit(fp, pchX, pORExit, "0", iLevel);
	} else {
		Emit(fp, acNotReached, nilOR, nil, iLevel);
	}
	trackends(fp, iLevel, pORExit, & pOREnds, pcExit);

	/* no returns above this point -- ksb */
	pcJustEscape = pcJKeep;
}

/* We can't just output the various endings in alpha order		(ksb)
 * we have to check for the most restrictive first.
 */
static void
mkordcntl(fp, iLevel, pORCntl, pcZName, pcExit)
FILE *fp;
int iLevel;
OPTION *pORCntl;
char *pcZName, *pcExit;
{
	auto char acLab[32];
	register OPTION *pORScan;

	if (DIDCNTL(pORCntl) || !ISALTUSAGE(pORCntl)) {
		return;
	}
	pORCntl->gattr |= GEN_CNTL;
	if (ISENABLE(pORCntl)) {
		for (pORScan = pORRoot; nilOR != pORScan; pORScan = pORScan->pORnext) {
			if (pORScan->pORallow != pORCntl) {
				continue;
			}
			mkordcntl(fp, iLevel, pORScan, pcZName, pcExit);
		}
	}
	(void)sprintf(acLab, "%s", mkdefid(pORCntl));

	/* Right here we need to put in the code for shared alternate
	 * endings.  This will also change resolution and man pages.
	 * See TODO -- ksb ZZZ
	 *
	 * Rather than "if (%U)" we need "if (%U" (loop over coms: "|| %U") ")"
	 */
	Emit(fp, "/* alternate for option %l */\nif (%U) %{\n", pORCntl, acLab, 1);
	mkcontrol(fp, iLevel, & pORCntl->pORsact, pcZName, acLab, pcExit);
	Emit(fp, "}", pORCntl, "/* miracles happen here */", 2);
}

/* make active inits for a list of option/variables			(ksb)
 */
static void
mkpseudo(fp, pORMaster)
FILE *fp;
OPTION *pORMaster;
{
	register OPTION *pOR;
	register char *pcDraw;

	for (pOR = pORMaster; nilOR != pOR; pOR = pOR->pORnext) {
		if (! (IS_ACTIVE(pOR->pOTtype) || OPT_INITCONST != ISWHATINIT(pOR)))
			continue;
		if ((char *)0 == pOR->pchinit || 0 == strcmp(sbPInit, pOR->pchinit))
			continue;
		pcDraw = pOR->pchinit;
		if ('\000' == *pcDraw) {
			/* allow "init dynamic ''" to force no init at all
			 */
			continue;
		}
		switch (ISWHATINIT(pOR)) {
		case OPT_INITENV:
			Emit(fp, "if (%ypi != (%J = getenv(\"%i\"))) {", pOR, nil, 1);
			if (nil != pOR->pchkeep) {
				Emit(fp, acKeepVal, pOR, acEnvTemp, 2);
			}
			if (ISTRACK(pOR)) {
				Emit(fp, acTrack, pOR, nil, 2);
			}
			/* check late here because late options are only
			 * converted after all option processing
			 */
			if (!ISLATE(pOR)) {
				Emit(fp, pOR->pchuupdate, pOR, acEnvTemp, 2);
			}
			Emit(fp, "}", pOR, nil, 2);
			break;
		case OPT_INITEXPR:
			/* we need a string to convert for active inits,
			 * others the implementor gave us a naked value to use
			 * "const" must be an active init type (accum, file)
			 */
			if (IS_ACTIVE(pOR->pOTtype)) {
		case OPT_INITCONST:
				if (nil != pOR->pchkeep) {
					Emit(fp, acKeepVal, pOR, pcDraw, 1);
				}
				/* set track to 1 in the declaration for
				 * tracked, active, none nil, expr inits
				 */
				if (!ISLATE(pOR)) {
					Emit(fp, pOR->pchuupdate, pOR, pcDraw, 1);
				}
				break;
			}
			if (nil != pOR->pchkeep) {
				Emit(fp, acKeepQuoted, pOR, pcDraw, 1);
			}
			Emit(fp, "%n = %a;", pOR, pcDraw, 1);
			break;
		default:
			fprintf(stderr, "%s: %s: internal error in initcode\n", progname, usersees(pOR, nil));
			exit(MER_INV);
		}
	}
}

/* make the main program that does all the work				(ksb)
 */
void
mkmain(fp)
register FILE *fp;
{
	register OPTION *pOR;
	register char *pcZName;		/* zero argument flag name	*/
	register char *pcDeBase;	/* default basename */
	register OPTION *pORBefore, *pORAfter;
	register int iFound;
	static char acDefBefore[] = "before";
	static char acDefAfter[] = "after";
	static char *apcLocal[2] = { "register int ", ", " };
	auto char *pcExit;

	pcExit = (0 == strcmp("main", pchMain)) ? "exit(0);" : "return 0;";

	Emit(fp, "#line 1 \"mkcmd_generated_main\"\n/*\n%cparser\n */\n", nilOR, nil, 0);
	if (fAnsi) {
		Emit(fp, "int %f(int argc, char **argv)\n{", nilOR, pchMain, 0);
	} else {
		Emit(fp, "int\n%f(argc, argv)\nint argc;char **argv;{", nilOR, pchMain, 0);
	}
	if (nilOR != pORRoot) {
		Emit(fp, "static char\n\t%:[] = \"", nilOR, nil, 1);
		for (pOR = pORRoot; nilOR != pOR; pOR = pOR->pORnext) {
			if (ISNOGETOPT(pOR))
				continue;
			Emit(fp, "%l%_", pOR, nil, 0);
		}
		Emit(fp, "\"", nilOR, nil, 0);
		/* decl locals */
		mkdecl(fp, 1, & sbCBase[0], OPT_LOCAL, OPT_STATIC);
	} else {
		/* decl locals */
		mkdecl(fp, 1, (char *)0, OPT_LOCAL, 0);
	}

	/* if any context has both a zero and an every we need an extra
	 * flag to keep some state (bj@cs.purdue.edu found this bug)
	 */
	pOR = newopt('e', & pORActn, 0, 0);
	if (nilOR == newopt('z', & pORActn, 0, 0) && (nilOR == pOR || !ISABORT(pOR))) {
		pcZName = (char *)0;
	} else {
		pcZName = fMix ? acAuxOpt : acCurOpt;
	}
	for (pOR = pORRoot; (char *)0 == pcZName && nilOR != pOR; pOR = pOR->pORnext) {
		register OPTION *pOREvery;

		if (!ISALTUSAGE(pOR)) {
			continue;
		}
		pOREvery = newopt('e', & pOR->pORsact, 0, 0);
		if (nilOR == newopt('z', & pOR->pORsact, 0, 0) && (nilOR == pOREvery || !ISABORT(pOREvery))) {
			continue;
		}
		pcZName = fMix ? acAuxOpt : acCurOpt;
	}

	iFound = 0;
	if (acCurOpt == pcZName || nilOR != pORRoot) {
		Emit(fp, apcLocal[iFound], nilOR, nil, 1);
		iFound = 1;
		Emit(fp, "%a", nilOR, acCurOpt, 0);
	}
	if (pcZName == acAuxOpt) {
		Emit(fp, apcLocal[iFound], nilOR, nil, 1);
		iFound = 1;
		Emit(fp, "%a = 1", nilOR, pcZName, 0);
	}
	if (iFound) {
		Emit(fp, ";", nilOR, nil, 0);
	}
	if ((char *)0 != pcKeyArg || ('\000' == pchProgName[0] && fBasename)) {
		Emit(fp, "register char *%a;", nilOR, acFinger, 1);
	}

	if (nil != pchGetenv || fPSawEnvInit) {
		Emit(fp, "register char *%J;extern char *getenv();", nilOR, nil, 1);
	}
	mktypeext(fp);

	Emit(fp, "%\n", nilOR, nil, 0);	/* statements */
	fflush(fp);

	if ((char *)0 != pcKeyArg) {
		pcDeBase = pcKeyArg;
	} else if ((char **)0 != ppcBases && (char *)0 != ppcBases[0]) {
		register int iDef;
		register char *pc_;

		pcDeBase = ppcBases[0];
		for (iDef = 0; (char *)0 != (pc_ = ppcBases[iDef]); ++iDef) {
			if ('\000' != pc_[strlen(pc_)+1])
				continue;
			pcDeBase = pc_;
			break;
		}
	} else {
		pcDeBase = "prog";
	}
	if ('\000' == pchProgName[0] && fBasename) {
		pchProgName = acFinger;
	}
	if ('\000' == pchProgName[0]) {
		/* suppress progname */
	} else if (fBasename) {
		Emit(fp, "if ((char *)0 == argv[0]) {%b = \"%qa\";", nilOR, pcDeBase, 1);
		Emit(fp, "%} else if (%ypi == (%b = strrchr(argv[0], \'/\'))) {%b = argv[0];", nilOR, pcDeBase, 1);
		Emit(fp, "%} else {++%b;}", nilOR, pcDeBase, 1);
	} else {
		/* I guess the implementor will trap 0 == argc */
		Emit(fp, "%b = argv[0];", nilOR, nil, 1);
	}

	pORBefore = newopt('b', & pORActn, 0, 0);
	pORAfter = newopt('a', & pORActn, 0, 0);
	MkTodo(fp, 1, nilOR != pORBefore ? pORBefore : pORAfter, aLIInits, pcDeBase, 1);

	if (nil != pchGetenv) {
		Emit(fp, "if (%ypi != (%J = getenv(\"%a\"))) {u_envopt(%J, & argc, & argv);}", nilOR, pchGetenv, 1);
	}

	if ((char **)0 != ppcBases) {
		register char *pc_;
		register int i;

		pc_ = pchProgName;
		if ((char *)0 != pcKeyArg) {
			Emit2(fp, "if (0 == strcmp(%b, \"%qa\") && argc > 1 && '-' != argv[1][0]) {%P = *++argv, --argc;}", nilOR, pcKeyArg, acFinger, 1);
			if (acFinger != pchProgName) {
				Emit2(fp, "else {%P = %b;}", nilOR, pcKeyArg, acFinger, 1);
			}
			pchProgName = acFinger;
		}
		for (i = 0; (char *)0 != ppcBases[i]; ++i)
			/* empty */;
		mkbases(fp, ppcBases, i, pcDefBaseOpt);
		pchProgName = pc_;
	} else if ((char *)0 != pcDefBaseOpt && '\000' != *pcDefBaseOpt) {
		Emit(fp, "u_envopt(\"%qa\", & argc, & argv);", nilOR, pcDefBaseOpt, 1);
	}

	/* new we have to output pseudo-inits for the accum and file options
	 */
	mkpseudo(fp, pORRoot);
	mkpseudo(fp, pORDecl);

	/* to get all the dynamic hooks we might need we go a little
	 * crazy here and drop code from all over the place	-- ksb
	 *	before's before
	 *	var's before
	 *	option's before
	 *	before's update
	 *	after's before
	 */
	if (nilOR != pORBefore) {
		Emit(fp, pORBefore->pchbefore, pORBefore, acDefBefore, 1);
	}
	for (pOR = pORDecl; nilOR != pOR; pOR = pOR->pORnext) {
		Emit(fp, pOR->pchbefore, pOR, acDefBefore, 1);
	}
	for (pOR = pORRoot; nilOR != pOR; pOR = pOR->pORnext) {
		Emit(fp, pOR->pchbefore, pOR, acDefBefore, 1);
	}
	if (nilOR != pORBefore) {
		register char *pchB;

		pchB = nil != pORBefore->pchuupdate ? pORBefore->pchuupdate : sbActn;
		if (nil != pORBefore->pchkeep) {
			Emit(fp, acKeepVal, pORBefore, sbPInit, 1);
		}
		Emit(fp, pchB, pORBefore, sbPInit, 1);
		Emit(fp, pORBefore->pchuser, pORBefore, sbPInit, 1);
	}
	if (nilOR != pORAfter) {
		Emit(fp, pORAfter->pchbefore, pORAfter, acDefBefore, 1);
	}

	/* drop the switch statement if we have one
	 */
	if (nilOR != pORRoot) {
		pcCurOpt = acCurOpt;
		mkswitch(fp, pcZName, pcExit);
		pcCurOpt = acNoOpt;
	}

	/* more crazy code from all over the place to finish sanity checks
	 *	before's after
	 *	option's late conversion (for sockets on fds)
	 *	variable's late conversion (for rc files)
	 *	after's update
	 *	var's after
	 *	option's after
	 *	after todo list
	 *	after's after
	 */

	if (nilOR != pORBefore) {
		Emit(fp, pORBefore->pchafter, pORBefore, acDefAfter, 1);
	}
	for (pOR = pORRoot; nilOR != pOR; pOR = pOR->pORnext) {
		register int iIndent;
		auto char acSpell[8];

		if (!ISLATE(pOR))
			continue;

		/* we can track for this -- Jeeze
		 * boolean, function, action we track (to convert late)
		 */
		iIndent = 1;
		/* N.B. this is a bug: we don't keep which letter of
		 * all the aliases gave us this data.  So we lie and
		 * say that the One True name did it. XXX
		 */
		pcCurOpt = CSpell(acSpell, pOR->chname, 1);
		Emit(fp, "if (%U) {", pOR, nil, iIndent++);
		if (ISVERIFY(pOR)) {
			Emit(fp, pOR->pchverify, pOR, pOR->pchkeep, iIndent);
		}
		Emit(fp, pOR->pchuupdate, pOR, pOR->pchkeep, iIndent);
		Emit(fp, pOR->pchuser, pOR, sbPInit, iIndent);
		if (ISENDS(pOR)) {
			Emit(fp, ((char *)0 != pOR->pcends ? pOR->pcends : pcExit), pOR, pcExit, iIndent);
		} else if (ISABORT(pOR)) {
			/* this is this the right exit?
			 */
			Emit(fp, (char *)0 != pOR->pchexit ? pOR->pchexit : "exit(1);", pOR, "exit(0);", iIndent);
		}
		Emit(fp, "}", pOR, nil, iIndent--);
	}
	for (pOR = pORDecl; nilOR != pOR; pOR = pOR->pORnext) {
		register int iIndent;
		if (!ISLATE(pOR))
			continue;

		iIndent = 1;
		Emit(fp, "if (%U) {", pOR, nil, iIndent++);
		Emit(fp, pOR->pchuupdate, pOR, pOR->pchkeep, iIndent);
		Emit(fp, pOR->pchuser, pOR, pOR->pchkeep, iIndent);
		if (ISENDS(pOR)) {
			Emit(fp, ((char *)0 != pOR->pcends ? pOR->pcends : pcExit), pOR, pcExit, iIndent);
		} else if (ISABORT(pOR)) {
			/* this is this the right exit?
			 */
			Emit(fp, (char *)0 != pOR->pchexit ? pOR->pchexit : "exit(1);", pOR, "exit(0);", iIndent);
		}
		Emit(fp, "}", pOR, nil, iIndent--);
	}
	pcCurOpt = acNoOpt;
	if (nilOR != pORAfter) {
		register char *pchA;

		pchA = nil != pORAfter->pchuupdate ? pORAfter->pchuupdate : sbActn;
		if (nil != pORAfter->pchkeep) {
			Emit(fp, acKeepVal, pORAfter, sbPInit, 1);
		}
		Emit(fp, pchA, pORAfter, sbPInit, 1);
		Emit(fp, pORAfter->pchuser, pORAfter, sbPInit, 1);
	}
	for (pOR = pORDecl; nilOR != pOR; pOR = pOR->pORnext) {
		if (ISPPARAM(pOR))
			continue;
		Emit(fp, pOR->pchafter, pOR, acDefAfter, 1);
	}
	for (pOR = pORRoot; nilOR != pOR; pOR = pOR->pORnext) {
		Emit(fp, pOR->pchafter, pOR, acDefAfter, 1);
	}
	MkTodo(fp, 1, nilOR != pORAfter ? pORAfter : pORBefore, aLIAfters, pcDeBase, 1);
	if (nilOR != pORAfter) {
		Emit(fp, pORAfter->pchafter, pORAfter, acDefAfter, 1);
	}

	/* output the variable ends
	 * (for options that allow alternate control points)
	 */
	pcCurOpt = acArgOpt;
	for (pOR = pORRoot; nilOR != pOR; pOR = pOR->pORnext) {
		if (! ISALTUSAGE(pOR)) {
			continue;
		}
		mkordcntl(fp, 2, pOR, pcZName, pcExit);
	}
	mkcontrol(fp, 1, & pORActn, pcZName, nil, pcExit);
	pcCurOpt = acNoOpt;

	/* balance {
	 */
	Emit(fp, "}", nilOR, nil, 1);
}

/* emit header externs for special actions				(ksb)
 * list is a hack, BTW.
 */
static void
mkdot_sact(fp, pOR, pOTLine, piFound)
FILE *fp;
OPTION *pOR;
OPTTYPE *pOTLine;
int *piFound;
{
	register OPTTYPE *pOTPointer;
	auto char acName[512];

	pOTPointer = CvtType('p');
	for (/* param */; nilOR != pOR; pOR = pOR->pORnext) {
		if (ISSTATIC(pOR)) {
			continue;
		}
		if (nil != pOR->pchkeep && pOTPointer->pchbase == pOTLine->pchbase) {
			if (DIDEKEEP(pOR)) {
				continue;
			}
			pOR->gattr |= GEN_EKEEP;
			if (nil == pOR->pchkeep) {
				continue;
			}
			if (! *piFound) {
				Emit(fp, "extern %a", pOR, pOTPointer->pchbase, 0);
				*piFound = 1;
			} else {
				Emit(fp, ",", pOR, nil, 0);
			}
			if (ISPTR2(pOR))
				Emit(fp, " **%N", pOR, pOR->pchkeep, 0);
			else
				Emit(fp, " *%N", pOR, pOR->pchkeep, 0);
		}
		if (nil == pOR->pOTtype->pchdecl || ISALIAS(pOR) || nil != pOR->pchfrom) {
			continue;	/* no var */
		}
		if (pOTLine->pchbase != pOR->pOTtype->pchbase || DIDEDECL(pOR)) {
			continue;	/* done or wrong type */
		}
		/* punch it out to the header file
		 */
		pOR->gattr |= GEN_EDECL;
		if (IS_XBASE(pOR->pOTtype)) {
			if (*piFound)
				Emit(fp, ";", pOR, nil, 0);
			*piFound = 0;
		}
		if (! *piFound) {
			Emit(fp, "extern %XxB", pOR, pOTLine->pchbase, 0);
			*piFound = 1;
		} else {
			Emit(fp, ",", pOR, nil, 0);
		}
		if (ISPTR2(pOR)) {
			Emit(fp, " %XxA", pOR, mkid(pOR, acName), 0);
		} else {
			Emit(fp, " %Xxa", pOR, mkid(pOR, acName), 0);
		}
		if (IS_XBASE(pOR->pOTtype)) {
			Emit(fp, ";", pOR, nil, 0);
			*piFound = 0;
		}
	}
}

/* make a dot-h file							(ksb)
 */
void
mkdot_h(fp, iULines, iHLines)
register FILE *fp;
int iULines, iHLines;
{
	register OPTION *pOR;
	register OPTTYPE *pOT;
	register char *pchType = nil;
	register OPTTYPE *pOTPointer;
	auto int iFound;
	auto char acName[512];
	static char *apcDcl[2] = { "extern char ", ", " };

	iFound = 0;
	pOTPointer = CvtType('p');
	Emit(fp, "/* %W generated declarations for options and buffers\n */\n\n", nilOR, nil, 0);
	if (nil != pchProgName && '\000' != pchProgName[0]) {
		Emit(fp, apcDcl[iFound], nilOR, nil, 0);
		iFound = 1;
		Emit(fp, "*%b", nilOR, nil, 0);
	}
	if (nil != pchTerse && '\000' != pchTerse[0]) {
		Emit(fp, apcDcl[iFound], nilOR, nil, 0);
		iFound = 1;
		EmitN(fp, "*a%t[%a]", nilOR, iULines+1, 0);
	}
	if (nil != pchVector && '\000' != pchVector[0]) {
		Emit(fp, apcDcl[iFound], nilOR, nil, 0);
		iFound = 1;
		EmitN(fp, "*%v[%a]", nilOR, iHLines, 0);
	}
	if (iFound) {
		Emit(fp, ";", nilOR, nil, 0);
	}
	if (nil != pchTerse) {
		Emit(fp, "#ifndef %t\n#define %t\t(a%t[0])\n#endif\n", nilOR, pchTerse, 0);
	}

	if (fAnsi) {
		Emit(fp, "extern int %f(int, char **)", nilOR, pchMain, 0);
	} else {
		Emit(fp, "extern int %f()", nilOR, pchMain, 0);
	}
	if (nil != pchUsage && '\000' != pchUsage[0]) {
		if (fAnsi) {
			Emit(fp, ", %u(void)", nilOR, pchUsage, 0);
		} else {
			Emit(fp, ", %u()", nilOR, pchUsage, 0);
		}
	}
	mktrdecl(fp, 0, 1, (char *)0, 0);
	Emit(fp, ";", nilOR, nil, 0);

	for (pOR = pORRoot; nilOR != pOR; pOR = pOR->pORnext) {
		if (ISGLOBAL(pOR)) {
			continue;
		}
		pOR->gattr |= GEN_EDECL;
	}
	for (pOR = pORDecl; nilOR != pOR; pOR = pOR->pORnext) {
		if (ISGLOBAL(pOR)) {
			continue;
		}
		pOR->gattr |= GEN_EDECL;
	}

	iFound = 0;
	for (pOT = aOTTypes; '\000' != pOT->chkey; ++pOT) {
		if (nil == pOT->pchdecl)
			continue;
		if (pchType != pOT->pchbase) {
			if (iFound)
				Emit(fp, ";", nilOR, nil, 0);
			pchType = pOT->pchbase;
			iFound = 0;
		}
		mkdot_sact(fp, pORActn, pOT, & iFound);
		for (pOR = pORRoot; nilOR != pOR; pOR = pOR->pORnext) {
			if (ISALTUSAGE(pOR)) {
				mkdot_sact(fp, pOR->pORsact, pOT, & iFound);
			}
			if (ISSTATIC(pOR)) {
				continue;
			}
			if (nil != pOR->pchkeep && pOTPointer->pchbase == pOT->pchbase) {
				if (DIDEKEEP(pOR))
					continue;
				pOR->gattr |= GEN_EKEEP;
				if (! iFound) {
					Emit(fp, "extern %a", pOR, pchType, 0);
					iFound = 1;
				} else {
					Emit(fp, ",", pOR, nil, 0);
				}
				Emit(fp, " *%N", pOR, "*", 0);
			}
			if (nil == pOR->pOTtype->pchdecl || ISALIAS(pOR) || nil != pOR->pchfrom) {
				continue;	/* no var */
			}
			if (pchType == pOR->pOTtype->pchbase) {
				if (DIDEDECL(pOR))
					continue;
				pOR->gattr |= GEN_EDECL;
				if (! iFound) {
					Emit(fp, "extern %XxB", pOR, pchType, 0);
					iFound = 1;
				} else {
					Emit(fp, ",", pOR, nil, 0);
				}
				Emit(fp, " %Xxa", pOR, mkid(pOR, acName), 0);
			}
		}
		for (pOR = pORDecl; nilOR != pOR; pOR = pOR->pORnext) {
			if (nil == pOR->pOTtype->pchdecl || ISALIAS(pOR) || nil != pOR->pchfrom || ISSTATIC(pOR)) {
				continue;	/* no var */
			}
			if (nil != pOR->pchkeep && pOTPointer->pchbase == pOT->pchbase) {
				if (DIDEKEEP(pOR))
					continue;
				pOR->gattr |= GEN_EKEEP;
				if (! iFound) {
					Emit(fp, "extern %a", pOR, pchType, 0);
					iFound = 1;
				} else {
					Emit(fp, ",", pOR, nil, 0);
				}
				Emit(fp, " *%N", pOR, pOR->pchkeep, 0);
			}
			if (pchType == pOR->pOTtype->pchbase) {
				if (DIDEDECL(pOR))
					continue;
				pOR->gattr |= GEN_EDECL;
				if (! iFound) {
					Emit(fp, "extern %XxB", pOR, pchType, 0);
					iFound = 1;
				} else {
					Emit(fp, ",", pOR, nil, 0);
				}
				Emit(fp, " %Xxa", pOR, mkid(pOR, acName), 0);
			}
		}
	}
	if (iFound)
		Emit(fp, ";", nilOR, nil, 0);
}
