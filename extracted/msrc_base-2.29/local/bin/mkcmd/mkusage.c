/* $Id: mkusage.c,v 8.36 2010/07/08 22:28:52 ksb Exp $
 * Based on some ideas from "mkprog", this program generates		(ksb)
 * a command line option parser based on a prototype file.
 * See the README file for usage of this program & the man page.
 *	$Compile: ${CC-cc} -c ${DEBUG--g} %f
 */
#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

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

#if ADD_MULTI_BITS
int fMultiBases = 0;		/* do we need clues on mkuvec's strings	*/
#endif

/* test if a (char *) is null, or the the empty (zero length) string	(ksb)
 */
static int
isempty(pc)
register char *pc;
{
	return (char *)0 == pc || '\000' == *pc;
}

typedef struct BBnode {		/* bundle buffer */
	int ibmembers;		/* number of options bundled		*/
	OPTION **ppORblist;	/* bundle argv				*/
	struct BBnode *pBBnext;	/* next bundle on the list		*/
	short int imand;	/* bundle is manitory			*/
	short int ipipes;	/* bundle contains [-a | -b]		*/
	/* only used in CyclePrint */
	short int ireduced;	/* CyclePrint diddled this bundle	*/
} BUNDLE;

/* sort prefix bundles on the final command line			(ksb)
 * order: mandatory bundles, bigger bundles, alpha order
 * negative is correct order, plus for swap
 */
static int
BLineOrder(ppBBLeft, ppBBRight)
QSORT_ARG *ppBBLeft, *ppBBRight;
{
	register const BUNDLE *pBBSin, *pBBDex;
	register int iRet;

	pBBSin = *(BUNDLE **)ppBBLeft, pBBDex = *(BUNDLE **)ppBBRight;
	if (0 != (iRet = pBBDex->imand - pBBSin->imand)) {
		return iRet;
	}
	if (0 != (iRet = pBBDex->ibmembers - pBBSin->ibmembers)) {
		return iRet;
	}
	return pBBSin->ppORblist[0]->chname-pBBDex->ppORblist[0]->chname;
}

static int u_ilines;
static int u_nroff = 0;

/* general output routines						(ksb)
 */
static void
u_vecstart(fp)
FILE *fp;
{
	u_ilines = 0;
	if (!u_nroff)
		fprintf(fp, "{\n");
}

static char u_awRecbit[MAXOPTS];

/* start the next possible usage version				(ksb)
 * record how many we saw
 */
static void
u_start(fp)
FILE *fp;
{
	static char *apcFmt[] = { "\t\t\"", "\\fI\\*(PN\\fP" };	/* ) */
	register int i;

	for (i = 0; i < sizeof(u_awRecbit); ++i) {
		u_awRecbit[i] = 0;
	}
	++u_ilines;
	fprintf(fp, "%s", apcFmt[u_nroff]);
}

#define U_OUT_NULL	0x00	/* no special handling			*/
#define U_OUT_MUST	0x01	/* option is not optional, is forced	*/

/* Output option, if this option *must* be included here output in	(ksb)
 * a format the user groks.  If part of a parent disjunction [-a | -b]
 * then we might change the pipes in an alias alternation... or not.
 */
static void
u_outopt(fp, fBits, pOR)
FILE *fp;
int fBits;
OPTION *pOR;
{
	static char *apcDash[] = { "-", "\\-" };
	static char *apcAlt[]  = { "|", "|" };
	static char *apcOFmt[] = { "%c", "\\fB%c\\fP" };
	static char *apcCFmt[] = { "-%c", "\\-\\fB%c\\fP" };
	static char *apcAFmt[] = { "|%c", "|\\fB%c\\fP" };
	static char *aapcDesc[2][2] = {
		{ " %s", "\\~\\fI%s\\fP" },
		{ "%s", "\\fI%s\\fP" }
	};
	static char *apcNFmt[] = { "-%s", "\\-\\fI%s\\fP" };
	static char *apcEFmt[] = { "%s%s", "\\fB%s\\fP\\fI%s\\fP" };
	register OPTION *pORAlias;
	register char **ppcDesc;

	for (pORAlias = pOR; nilOR != pORAlias; pORAlias = pORAlias->pORalias) {
		u_awRecbit[pOR->chname] = 1;
	}
	ppcDesc = aapcDesc[DIDOPTIONAL(pOR)];
	if (0 == (U_OUT_MUST & fBits)) {
		fprintf(fp, "[");
	}

	if (sbBInit == (ISALIAS(pOR) ? pOR->pORali : pOR)->pOTtype->pchdef) {
		fprintf(fp, apcCFmt[u_nroff], pOR->chname);
		for (pORAlias = pOR->pORalias; nilOR != pORAlias; pORAlias = pORAlias->pORalias) {
			if (! ISVISIBLE(pORAlias)) {
				continue;
			}
			/* show that only options are still disjuncted
			 */
			if (ISONLY(pOR)) {
				fprintf(fp, apcAFmt[u_nroff], pORAlias->chname);
			} else {
				fprintf(fp, apcOFmt[u_nroff], pORAlias->chname);
			}
		}
	} else {
		register OPTION *pOREscape, *pORNumber;
		register int fCompact;
		register int iCount;

		/* if the option has aliases that cannot be expressed in
		 * the compact `-f|t|w param' format then we have to
		 * express as `-f param|-t param|-w file' cases are:
		 *	has a `number' or `escape'
		 *	has more than one description of a parameter
		 *
		 * We always disjunct because we need the noise to sep.
		 * viz.:  [-a|b|c foo] is better than [-abc foo] which
		 *  might be mistaken for a `long option' {we won't do}.
		 */
		pORNumber = pOREscape = nilOR;
		iCount = 0;
		for (pORAlias = pOR; nilOR != pORAlias; pORAlias = pORAlias->pORalias) {
			if (! ISVISIBLE(pORAlias)) {
				continue;
			}
			if ('+' == pORAlias->chname) {
				pOREscape = pORAlias;
			} else if ('#' == pORAlias->chname) {
				pORNumber = pORAlias;
			}
			if ((char *)0 != pORAlias->pchdesc) {
				++iCount;
			}
		}
		fCompact = nilOR == pORNumber && nilOR == pOREscape && 1 == iCount;
		iCount = 0;
		for (pORAlias = pOR; nilOR != pORAlias; pORAlias = pORAlias->pORalias) {
			register char *pcDesc;
			if (! ISVISIBLE(pORAlias)) {
				continue;
			}
			if (fCompact) {
				if (0 == iCount) {
					fprintf(fp, "%s", apcDash[u_nroff]);
					iCount = 1;
				} else {
					fprintf(fp, "%s", apcAlt[u_nroff]);
				}

				fprintf(fp, apcOFmt[u_nroff], pORAlias->chname);
				continue;
			}
			if (0 == iCount) {
				iCount = 1;
			} else {
				fprintf(fp, "%s", apcAlt[u_nroff]);
			}
			if ((char *)0 != pORAlias->pchdesc) {
				pcDesc = pORAlias->pchdesc;
			} else {
				pcDesc = pOR->pchdesc;
			}
			if (pORAlias == pORNumber) {
				fprintf(fp, apcNFmt[u_nroff], pcDesc);
			} else if (pORAlias == pOREscape) {
				fprintf(fp, apcEFmt[u_nroff], pORAlias->pchgen, pcDesc);
			} else {
				fprintf(fp, apcCFmt[u_nroff], pORAlias->chname);
				fprintf(fp, ppcDesc[u_nroff], pcDesc);
			}
		}
		if (fCompact) {
			fprintf(fp, ppcDesc[u_nroff], pOR->pchdesc);
		}
	}
	if (0 == (U_OUT_MUST & fBits)) {
		fprintf(fp, "]");
	}
	pOR->gattr |= GEN_SEEN;
}


/* output a positional parameter					(ksb)
 */
static void
u_outparam(fp, pOR)
FILE *fp;
OPTION *pOR;
{
	static char *apcFmt[] = { "%s", "\\fI%s\\fP" };
	fprintf(fp, apcFmt[u_nroff], pOR->pchdesc);
	pOR->gattr |= GEN_SEEN;
}

/* start a block of options, might be mandatory, might not		(ksb)
 */
static void
u_obundle(fp, fMand)
FILE *fp;
int fMand;
{
	if (fMand)
		fprintf(fp, " ");
	else
		fprintf(fp, " [");
}


/* two options are mutually exclusive on the same line			(ksb)
 */
static void
u_or(fp, fOr)
FILE *fp;
int fOr;
{
	fprintf(fp, fOr ? " | " : " ");
}

/* end a bundle, note no trailing spaces (might be the end of line)	(ksb)
 */
static void
u_cbundle(fp, fMand)
FILE *fp;
int fMand;
{
	if (!fMand)
		fprintf(fp, "]");
}

/* just space over for the next item on the line, might chnage in	(ksb)
 * some Text formatters (we only do nroff now, *sigh*).
 */
static void
u_space(fp)
FILE *fp;
{
	fputc(' ', fp);
}

/* output the leader on the bundled switches ([-abc] kinda)		(ksb)
 */
static void
u_oswitches(fp)
FILE *fp;
{
	static char *apcFmt[] = { " [-", " [\\-" };
	fputs(apcFmt[u_nroff], fp);
}

/* output a switch letter						(ksb)
 */
static void
u_switch(fp, pOR)
FILE *fp;
OPTION *pOR;
{
	static char *apcFmt[] = { "%c", "\\fB%c\\fP" };

	u_awRecbit[pOR->chname] = 1;
	fprintf(fp, apcFmt[u_nroff], pOR->chname);
	pOR->gattr |= GEN_SEEN;
}

/* close the bundled switches						(ksb)
 */
static void
u_cswitches(fp)
FILE *fp;
{
	fputc(']', fp);
}

/* output a control point's parameter description			(ksb)
 */
static void
u_outaction(fp, pOR)
FILE *fp;
OPTION *pOR;
{
	if (DIDOPTIONAL(pOR)) {
		static char *apcFmt[] = { " [%s]", " [\\fI%s\\fP]" };
		fprintf(fp, apcFmt[u_nroff], pOR->pchdesc);
	} else {
		static char *apcFmt[] = { " %s", " \\fI%s\\fP" };
		fprintf(fp, apcFmt[u_nroff], pOR->pchdesc);
	}
	pOR->gattr |= GEN_SEEN;
}

/* bundle could be a `marriage of convenience'				(ksb)
 * in this case we have to pull out the bool/toggles
 * then dump the [-x param]'s all alone
 */
static void
u_BB(fp, pBBPrint)
FILE *fp;
BUNDLE *pBBPrint;
{
	register int iMem, iFound;
	register ATTR wKeep;
	register OPTION *pOR;

	if (!pBBPrint->ipipes) {
		iFound = 0;
		for (iMem = 0; iMem < pBBPrint->ibmembers; ++iMem) {
			pOR = pBBPrint->ppORblist[iMem];
			if (sbBInit != pOR->pOTtype->pchdef) {
				continue;
			}
			if (0 == iFound) {
				u_oswitches(fp);
				iFound = 1;
			}
			wKeep = pOR->oattr;
			pOR->oattr &= ~OPT_HIDDEN;
			u_switch(fp, pOR);
			pOR->oattr |= OPT_HIDDEN & wKeep;
		}
		if (iFound) {
			u_cswitches(fp);
		}
	}
	for (iMem = 0; iMem < pBBPrint->ibmembers; ++iMem) {
		pOR = pBBPrint->ppORblist[iMem];
		if (!pBBPrint->ipipes && sbBInit == pOR->pOTtype->pchdef) {
			continue;
		}
		wKeep = pOR->oattr;
		pOR->oattr &= ~OPT_HIDDEN;
		if (pBBPrint->ipipes) {
			if (0 == iMem) {
				u_obundle(fp, pBBPrint->imand);
			} else {
				u_or(fp, 1);
			}
			u_outopt(fp, U_OUT_MUST, pOR);
		} else {
			u_space(fp);
			u_outopt(fp, U_OUT_NULL, pOR);
		}
		pOR->oattr |= OPT_HIDDEN & wKeep;
	}
	if (pBBPrint->ipipes) {
		u_cbundle(fp, pBBPrint->imand);
	}
}

/* end the line we are on						(ksb)
 */
static void
u_end(fp)
FILE *fp;
{
	static char *apcFmt[] = { "\",\n", "\n.br\n" };

#if ADD_MULTI_BITS
	if (fMultiBases && 0 == u_nroff) {
		register int i;
		register char *pcSep;

		/* we could put in more info (param colons) here
		 * or what options are the basename set options.
		 * This is all triggered by more than one basename
		 * setting options.
		 */
		fprintf(fp,"\\000");
		for (i = 0; i < sizeof(u_awRecbit); ++i) {
			if (! u_awRecbit[i])
				continue;
			fprintf(fp, "%c", i);
			u_awRecbit[i] = 0;
		}
	}
#endif
	fprintf(fp, "%s", apcFmt[u_nroff]);
}

/* and all the lines (the whole vector)					(ksb)
 */
static void
u_vecend(fp)
FILE *fp;
{
	static char *apcFmt[] = { "\t\t(char *)0\n\t}", "\n.sp\n" };
	fprintf(fp, "%s", apcFmt[u_nroff]);
}

static int awTrack[MAXOPTS];	/* keep track of levels of hidden-ness	*/
static int awAttr[MAXOPTS];	/* keep track to restore visible	*/

/*#define DEBUG 1*/

/* GLOCK/GUNLOCK will hide options for a mk_cycle or a few cycles
 * always balance the calls, please!
 */
#if DEBUG
void
GLOCK(MpOR, Mpc)
OPTION *MpOR;
char *Mpc;
{
	if (0 == awTrack[(MpOR)->chname]++) {
		fprintf(stderr, "%s: locked %c (%s)\n", progname, (MpOR)->chname, Mpc);
		awAttr[(MpOR)->chname] |= (MpOR)->oattr & OPT_HIDDEN;
		(MpOR)->gattr |= GEN_FAKEHIDE;
		(MpOR)->oattr |= OPT_HIDDEN;
	} else {
		fprintf(stderr, "%s: relocked %c (%s) %d\n", progname, (MpOR)->chname, Mpc, awTrack[(MpOR)->chname]);
	}
}
#else
/* GLOCK/GUNLOCK will hide options for a ccycle or a few cycles
 * always balance the calls, please!
 */
#define GLOCK(MpOR, Mpc) \
	if (0 == awTrack[(MpOR)->chname]++) { \
		awAttr[(MpOR)->chname] |= (MpOR)->oattr & OPT_HIDDEN; \
		(MpOR)->gattr |= GEN_FAKEHIDE; \
		(MpOR)->oattr |= OPT_HIDDEN; \
	} else /*eat the semicolon*/
#endif


/* locked options are not printed by Cycle<stuff>			(ksb)
 * so we have to be able to unlock them too
 */
#if DEBUG
void
GUNLOCK(MpOR, Mpc)
OPTION *MpOR;
char *Mpc;
{
	if (0 == --awTrack[(MpOR)->chname]) {
		fprintf(stderr, "%s: unlocked %c (%s)\n", progname, (MpOR)->chname, Mpc);
		(MpOR)->oattr &= ~OPT_HIDDEN;
		(MpOR)->gattr &= ~GEN_FAKEHIDE;
		(MpOR)->oattr |= awAttr[(MpOR)->chname]&OPT_HIDDEN;
		awAttr[(MpOR)->chname] &= ~OPT_HIDDEN;
	} else {
		fprintf(stderr, "%s: still locked %c (%s) %d\n", progname, (MpOR)->chname, Mpc, awTrack[(MpOR)->chname]);
	}
}
#else
#define GUNLOCK(MpOR, Mpc) \
	if (0 == --awTrack[(MpOR)->chname]) { \
		(MpOR)->oattr &= ~OPT_HIDDEN; \
		(MpOR)->gattr &= ~GEN_FAKEHIDE; \
		(MpOR)->oattr |= awAttr[(MpOR)->chname]&OPT_HIDDEN; \
		awAttr[(MpOR)->chname] &= ~OPT_HIDDEN; \
	} else /*eat the semicolon*/
#endif


/* output one of the usage strings					(ksb)
 */
static void
mkuline(fp, pORDispl, ppORCtl, uPrefix, ppBBSort, pBBBreak)
register FILE *fp;
OPTION *pORDispl, **ppORCtl;
unsigned uPrefix;
BUNDLE **ppBBSort, *pBBBreak;
{
	register OPTION *pOR, *pORZero;
	register unsigned uFound;
	register BUNDLE *pBBPrint;
	register OPTION *pORLeft, *pORRight;


	/* begin the output with before and mandatory options
	 */
	u_start(fp);
	if (nilOR != (pOR = newopt('b', ppORCtl, 0, 0)) && !isempty(pOR->pchdesc)) {
		u_outaction(fp, pOR);
	}

	/* now we are clear to output the bundles, at last
	 */
	for (uFound = 0; uFound < uPrefix; ++uFound) {
		pBBPrint = ppBBSort[uFound];
		if (pBBPrint->ireduced) {
			continue;
		}
		u_BB(fp, pBBPrint);
	}

	/* then output `only' switches so they come before the ones that can
	 * be repeated, each in its own bundle.  This is fine-line-clever.
	 */
	for (pOR = pORDispl; nilOR != pOR; pOR = pOR->pORnext) {
		if (! ISVISIBLE(pOR) || ISALIAS(pOR) || ISBREAK(pOR)) {
			continue;
		}
		if (!ISONLY(pOR) || sbBInit != pOR->pOTtype->pchdef) {
			continue;
		}
		u_space(fp);
		u_outopt(fp, U_OUT_NULL, pOR);
	}
	/* then output the switches, but none of the `breaks' (output below)
	 * or `once' (output just above)		-- ksb
	 */
	uFound = 0;
	for (pOR = pORDispl; nilOR != pOR; pOR = pOR->pORnext) {
		register OPTION *pORCheck;

		pORCheck = ISALIAS(pOR) ? pOR->pORali : pOR;
		if (! ISVISIBLE(pOR) || ! ISVISIBLE(pORCheck) || ISONLY(pORCheck) || ISBREAK(pORCheck)) {
			continue;
		}
		if (sbBInit != pORCheck->pOTtype->pchdef) {
			continue;
		}
		if (0 == uFound) {
			u_oswitches(fp);
			uFound = 1;
		}
		u_switch(fp, pOR);
	}
	if (uFound) {
		u_cswitches(fp);
	}

	/* then output the options with parameters, of course there is
	 * no way to say `once' here, sigh.
	 */
	for (pOR = pORDispl; nilOR != pOR; pOR = pOR->pORnext) {
		if (ISALIAS(pOR) || ! ISVISIBLE(pOR) || ISBREAK(pOR)) {
			continue;
		}
		if (sbBInit == pOR->pOTtype->pchdef) {
			continue;
		}
		u_space(fp);
		u_outopt(fp, U_OUT_NULL, pOR);
	}

	/* now output the breaks, which are visible
	 */
	for (pBBPrint = pBBBreak; (BUNDLE *)0 != pBBPrint; pBBPrint = pBBPrint->pBBnext) {
		if (!(ISBREAK(pBBPrint->ppORblist[0]) || ISABORT(pBBPrint->ppORblist[0]))) {
			fprintf(stderr, "%s: compl. inv. broken in usage bundles\n", progname);
			iExit |= MER_INV;
			continue;
		}
		u_BB(fp, pBBPrint);
	}
	/* might still have a single (unbundled) stops option - will
	 * the special cases never end? (doesn't happen anymore, right?)
	 */
	uFound = 0;
	for (pOR = pORDispl; nilOR != pOR; pOR = pOR->pORnext) {
		if (! ISVISIBLE(pOR) || !ISBREAK(pOR))
			continue;
		if (sbBInit != (ISALIAS(pOR) ? pOR->pORali : pOR)->pOTtype->pchdef) {
			continue;
		}
		if (0 == uFound) {
			u_oswitches(fp);
			uFound = 1;
		}
		u_switch(fp, pOR);
	}
	if (uFound) {
		u_cswitches(fp);
	}
	for (pOR = pORDispl; nilOR != pOR; pOR = pOR->pORnext) {
		if (! ISVISIBLE(pOR) || !ISBREAK(pOR)) {
			continue;
		}
		if (ISALIAS(pOR) || sbBInit == pOR->pOTtype->pchdef) {
			continue;
		}
		u_space(fp);
		u_outopt(fp, U_OUT_NULL, pOR);
	}

	/* done with - options, now `after' and the positional params
	 * (`after' is common to all actions so we take it from pORActn)
	 */
	if (nilOR != (pOR = newopt('a', & pORActn, 0, 0)) && !isempty(pOR->pchdesc)) {
		u_outaction(fp, pOR);
	}
	pORLeft = newopt('s', ppORCtl, 0, 0);
	pORZero = newopt('z', ppORCtl, 0, 0);
	pORRight = newopt('d', ppORCtl, 0, 0);
	for (uFound = 0; uFound < (nilOR == pORLeft ? 0 : pORLeft->iorder); ++uFound) {
		pOR = pORLeft->ppORorder[uFound];
		u_obundle(fp, !ISOOPEN(pOR));
		u_outparam(fp, pOR);
		u_cbundle(fp, !ISOCLOSE(pOR));
	}
	if (nilOR != (pOR = newopt('e', ppORCtl, 0, 0)) && !isempty(pOR->pchdesc)) {
		if (nilOR == pORZero || (nilOR != pORZero && !ISABORT(pORZero)))
			pOR->gattr |= GEN_OPTIONAL;
		u_outaction(fp, pOR);
	} else if (nilOR != (pOR = newopt('l', ppORCtl, 0, 0)) && !isempty(pOR->pchdesc)) {
		if (nilOR == pORZero || (nilOR != pORZero && !ISABORT(pORZero)))
			pOR->gattr |= GEN_OPTIONAL;
		u_outaction(fp, pOR);
	}
	for (uFound = 0; uFound < (nilOR == pORRight ? 0 : pORRight->iorder); ++uFound) {
		pOR = pORRight->ppORorder[uFound];
		/* we can't accept optional rights, but we can print them
		 */
		u_obundle(fp, !ISOOPEN(pOR));
		u_outparam(fp, pOR);
		u_cbundle(fp, !ISOCLOSE(pOR));
	}

	/* now the exit mark, wonder why anyone would convert here?
	 */
	if (nilOR != (pOR = newopt('x', ppORCtl, 0, 0)) && !isempty(pOR->pchdesc)) {
		u_outaction(fp, pOR);
	}
	u_end(fp);
}


/* so we use a dumb data structute for state into on command lines
 * I sick of being so Damn Clever.		-- ksb
 */
#define STATE_SEEN	0x0001	/* has had all transiations		*/
#define STATE_STOPS	0x0002	/* force a break			*/
#define STATE_FINAL	0x0004	/* from an abort option			*/
#define STATE_GENERIC	0x0010	/* emited a generic usage		*/
#define STATE_SYNTH	0x0100	/* the product of synthetic contraints	*/
typedef struct OSnode {
	char *pcexcl;		/* list of excludes			*/
	char *pcallow;		/* path through allows			*/
	char *pcpath;		/* path through options			*/
	short int sattr;	/* state attributes			*/
	struct OSnode *pOSnext;	/* next on this list			*/
} OSTATE;
#define ST_FINAL(MpOS)	(0 != ((MpOS)->sattr & STATE_FINAL))
#define ST_SEEN(MpOS)	(0 != ((MpOS)->sattr & STATE_SEEN))
#define ST_STOPS(MpOS)	(0 != ((MpOS)->sattr & STATE_STOPS))
#define ST_GENERIC(MpOS) (0 != ((MpOS)->sattr & STATE_GENERIC))
#define ST_SYNTH(MpOS)	(0 != ((MpOS)->sattr & STATE_SYNTH))


static OSTATE OSAllAbort =	/* sentinal value for aborts options */
	{"_", "_", nil, STATE_STOPS|STATE_FINAL, (OSTATE *)0};

#if DEBUG
/* when debugging we might need this					(ksb)
 */
static void
StatePrint(pOS, pcFrom)
OSTATE *pOS;
char *pcFrom;
{
	if (& OSAllAbort == pOS) {
		fprintf(stderr, "%s: %s: in the abort state\n", progname, pcFrom);
		return;
	}
	fprintf(stderr, "%s: %s: %p excludes [%s]", progname, pcFrom, pOS, pOS->pcexcl);
	if ('\000' != pOS->pcallow[0]) {
		fprintf(stderr, " allow [%s]", pOS->pcallow);
	}
	if (nil == pOS->pcpath) {
		fprintf(stderr, " unset path");
	} else if ('\000' != pOS->pcpath[0]) {
		fprintf(stderr, " path [%s]", pOS->pcpath);
	}
	if (ST_SEEN(pOS))
		fprintf(stderr, " seen");
	if (ST_STOPS(pOS))
		fprintf(stderr, " stops");
	if (ST_FINAL(pOS))
		fprintf(stderr, " final");
	if (ST_GENERIC(pOS))
		fprintf(stderr, " generic");
	if (ST_SYNTH(pOS))
		fprintf(stderr, " synthetic");
	fprintf(stderr, "\n");
}
#endif

/* record all the forbid options under an enable option			(ksb)
 */
static void
CutEnable(pcRecord, pORFound)
char *pcRecord;
OPTION *pORFound;
{
	register OPTION *pOR;

	for (pOR = pORRoot; nilOR != pOR; pOR = pOR->pORnext) {
		/* maybe look at HIDDEN? XXX */
		if (pOR->pORallow == pORFound /* && ISFORBID(pOR) */) {
			*pcRecord++ = pOR->chname;
		}
		if (pOR->pORallow == pORFound && ISENABLE(pOR)) {
			CutEnable(pcRecord, pOR);
			pcRecord += strlen(pcRecord);
		}
	}
	*pcRecord = '\000';
}

/* We want to add the list to the current excludes			(ksb)
 * If we add an enable option we have to scan for options we are transatively
 * excluding from the list (because the enable can't happen).
 */
static void
XansForbid(pcExcl, pcList, pORCross)
char *pcExcl, *pcList;
OPTION *pORCross;
{
	register int iLen;
	register OPTION *pORFound;
	auto char acRecur[MAXOPTS+2];

	iLen = strlen(pcExcl);
	for (/* param */; '\000' != *pcList; ++pcList) {
		if ((char *)0 != strchr(pcExcl, *pcList) || pORCross->chname == *pcList) {
			continue;
		}

		/* have to add this one
		 */
		pcExcl[iLen++] = *pcList;
		pcExcl[iLen] = '\000';
		pORFound = newopt(*pcList, & pORRoot, 0, 0);
		if ((OPTION *)0 == pORFound) {
			/* we complain later */
			continue;
		}
		if (!ISENABLE(pORFound)) {
			continue;
		}

		/* OK, find all the options we can't reach because we
		 * will never enable this option
		 */
		CutEnable(acRecur, pORFound);
		XansForbid(pcExcl, acRecur, pORCross);
		iLen = strlen(pcExcl);
	}
	struniq(pcExcl);
}

/* the states for the current line, this helps up break up lines for
 * the more simple (:-}) routines above
 */
static OSTATE *pOSFree = 0, *apOSHash[MAXOPTS];

/* and now we remember the states we've seen				(ksb)
 */
static OSTATE *
StateFind(pcAllow, pcForbid, pcPath)
char *pcAllow, *pcForbid, *pcPath;
{
	register OSTATE **ppOSKey, *pOS;
	register int d = 0;

	for (ppOSKey = & apOSHash[(int)pcForbid[0]]; (OSTATE *)0 != (pOS = *ppOSKey);  ppOSKey = & pOS->pOSnext) {
		if (0 > (d = strcmp(pOS->pcexcl, pcForbid))) {
			break;
		}
		if (0 != d) {
			continue;
		}
		if (0 > (d = strcmp(pOS->pcallow, pcAllow))) {
			break;
		}
		if (0 != d) {
			continue;
		}
		if (nil != pcPath) {
			/* we found a state that has not set a path yet,
			 * steal that one
			 */
			if (nil == pOS->pcpath) {
				pOS->pcpath = malloc((strlen(pcPath)|7)+1);
				(void)strcpy(pOS->pcpath, pcPath);
			} else if (0 > (d = strcmp(pOS->pcpath, pcPath))) {
				break;
			} else if (0 != d) {
				continue;
			}
		}
		return pOS;
	}
	/* build the new state...
	 */
	if ((OSTATE *)0 != (pOS = pOSFree)) {
		pOSFree = pOS->pOSnext;
	} else if ((OSTATE *)0 == (pOS = (OSTATE *)malloc(sizeof (OSTATE)))) {
		fprintf(stderr, acOutMem, progname);
		exit(MER_LIMIT);
	}
	/* note path is nil, caller fills it in
	 */
	pOS->pOSnext = *ppOSKey;
	pOS->sattr = 0;
	pOS->pcexcl = malloc(((strlen(pcForbid)+strlen(pcAllow))|7)+3);
	if ((char *)0 == pOS->pcexcl) {
		fprintf(stderr, acOutMem, progname);
		exit(MER_LIMIT);
	}
	pOS->pcallow = pOS->pcexcl + strlen(pcForbid) + 1;
	if (nil == pcPath) {
		pOS->pcpath = nil;
	} else {
		pOS->pcpath = strdup(pcPath);
	}
	(void)strcpy(pOS->pcexcl, pcForbid);
	(void)strcpy(pOS->pcallow, pcAllow);

	*ppOSKey = pOS;
	return pOS;
}

static BUNDLE
	*pBBHead,		/* current bundle list			*/
	*pBBTail;		/* the stops bundle			*/

/* stop cycle and output what we have					(ksb)
 * if this state is a mock of one we've already printed just skip it.
 * This is if the reduced (low fat?) state is one we've seen, or later
 * if we've reduced to that state -- don't print one of them.
 *
 * The reduced state is one with the enable options that we reduce'd
 * removed.
 */
void
CyclePrint(fp, pOSCur, ppORCtl)
FILE *fp;
OSTATE *pOSCur;
OPTION **ppORCtl;
{
	register unsigned uFound, uPrefix;
	register BUNDLE *pBBPrint;
	register OPTION *pORCache, *pOR;
	register unsigned uPanic;
	register char *pcScan;
	register OSTATE *pOSReduced;
	auto BUNDLE *apBBSort[MAXOPTS];
	auto OPTION *pOREmpty;
	auto char acNewPath[MAXOPTS+2], acNewAllow[MAXOPTS+2];

	/* sort the bundles
	 */
	uPrefix = 0;
	for (pBBPrint = pBBHead; (BUNDLE *)0 != pBBPrint; pBBPrint = pBBPrint->pBBnext) {
		apBBSort[uPrefix++] = pBBPrint;
		pBBPrint->ireduced = 0;
	}
	qsort((char *)apBBSort, uPrefix, sizeof(apBBSort[0]), BLineOrder);

	/* This is the "unobvious way" see CycleMerge:
	 * reduce the optional bundles of one option to be "common".
	 * N.B. this loop makes the next one work, don't merge them
	 */
	for (uFound = 0; uFound < uPrefix; ++uFound) {
		pBBPrint = apBBSort[uFound];
		pORCache = pBBPrint->ppORblist[0];
		if (ISBREAK(pORCache)) {
			fprintf(stderr, "%s: stops inv. broken in usage line for %c\n", progname, pORCache->chname);
			iExit |= MER_INV;
			continue;
		}

		if (ISALTUSAGE(pORCache) || 1 != pBBPrint->ibmembers || 0 != pBBPrint->imand) {
			continue;
		}
		pBBPrint->ireduced = 1;
		pORCache->oattr &= ~OPT_HIDDEN;
	}

	/* If an option enabled only options that were since
	 * hidden then we need to cast as spell to put it in the
	 * common part of the usage as well, sigh.
	 *
	 * This loop is  not quite as simple as you'd think:  we scan at
	 * the end over the same data to undo a fix we didn't like...
	 */
	for (uFound = 0; uFound < uPrefix; ++uFound) {
		pBBPrint = apBBSort[uFound];
		if (pBBPrint->ireduced) {
			continue;
		}

		pORCache = pBBPrint->ppORblist[0];
		if (ISALTUSAGE(pORCache) || 1 != pBBPrint->ibmembers || !ISENABLE(pORCache)) {
			continue;
		}

		for (pOR = pORRoot; nilOR != pOR; pOR = pOR->pORnext) {
			if (pORCache != pOR->pORallow) {
				continue;
			}
			if (ISVISIBLE(pOR) || DIDBUNDLE(pOR)) {
				break;
			}
		}
		if (nilOR != pOR) {
			continue;
		}
		pBBPrint->ireduced = 1;
		pORCache->oattr &= ~OPT_HIDDEN;

		/* We just made an option visible. This might cause us to
		 * need to undo a spell we just cast.  Oh my.  Find the
		 * reduced bundle that holds the our allow option and
		 * un-reduce it.
		 */
		for (uPanic = 0; uPanic < uPrefix; ++uPanic) {
			pBBPrint = apBBSort[uPanic];
			if (!pBBPrint->ireduced || 1 != pBBPrint->ibmembers) {
				continue;
			}
			pOR = pBBPrint->ppORblist[0];
			if (pOR->pORallow != pORCache) {
				continue;
			}
			/* egads this option allows the option we made
			 * common, how uncommon.  Put it back.
			 */
			pOR->oattr |= OPT_HIDDEN;
			pBBPrint->ireduced = 0;
			break;
		}
	}

	/* ZZZ find the reduced state and don't print if we are
	 * a duplicate after reduction, mark it in any case
	 */
	(void)strcpy(acNewPath, nil != pOSCur->pcpath ? pOSCur->pcpath : "");
	(void)strcpy(acNewAllow, nil != pOSCur->pcallow ? pOSCur->pcallow : "");

	/* find a state with the reduced options not in it
	 */
	for (uFound = 0; uFound < uPrefix; ++uFound) {
		pBBPrint = apBBSort[uFound];
		if (!pBBPrint->ireduced) {
			continue;
		}
		pORCache = pBBPrint->ppORblist[0];
		if (!ISENABLE(pORCache)) {
			continue;
		}
		if ((char *)0 != (pcScan = strchr(acNewPath, pORCache->chname))) {
			while ('\000' != (pcScan[0] = pcScan[1]))
				++pcScan;
		}
		if ((char *)0 != (pcScan = strchr(acNewAllow, pORCache->chname))) {
			while ('\000' != (pcScan[0] = pcScan[1]))
				++pcScan;
		}
	}
	pOSReduced = (& OSAllAbort != pOSCur) ?  StateFind(acNewAllow, pOSCur->pcexcl, acNewPath) : (OSTATE *)0;

	/* if no special control points are enabled use an empty list
	 */
	if ((OPTION **)0 == ppORCtl) {
		ppORCtl = & pOREmpty;
		pOREmpty = nilOR;
	}

	if (& OSAllAbort == pOSCur || pOSCur == pOSReduced || !ST_SEEN(pOSReduced) /* || ST_SYNTH(pOSCur) */) {
		mkuline(fp, pORRoot, ppORCtl, uPrefix, apBBSort, pBBTail);
#if DEBUG
		fprintf(stderr, "state %p:", pOSCur);
		mkuline(stderr, pORRoot, ppORCtl, uPrefix, apBBSort, pBBTail);
		fprintf(stderr, "\n");
#endif
	}
	if (& OSAllAbort != pOSCur && pOSCur != pOSReduced /* && !ST_SYNTH(pOSCur) */) {
		pOSReduced->sattr |= STATE_SEEN;
	}

	/* un-hack the single option [optional] bundles, sigh
	 */
	for (uFound = 0; uFound < uPrefix; ++uFound) {
		pBBPrint = apBBSort[uFound];
		if (!pBBPrint->ireduced)
			continue;
		pBBPrint->ppORblist[0]->oattr |= OPT_HIDDEN;
		pBBPrint->ireduced = 0;
	}
}

/* pull all the state node out of the table se we can reuse them in the	(ksb)
 * next call to mkvector
 */
static void
StateReset()
{
	register int i;
	register OSTATE **ppOSFree, *pOSTail;
	register OSTATE *pOS;

	ppOSFree = & pOSFree;
	pOSTail =  pOSFree;
	for (i = 0; i < MAXOPTS; ++i) {
		if ((OSTATE *)0 == (pOS = apOSHash[i])) {
			continue;
		}
		apOSHash[i] = (OSTATE *)0;
		for (*ppOSFree = pOS; (OSTATE *)0 != pOS->pOSnext; pOS = pOS->pOSnext) {
			pOS->sattr = 0;
			free(pOS->pcexcl);
		}
		ppOSFree = & pOS->pOSnext;
	}
	*ppOSFree = pOSTail;
}

/* If we traversed this option where would we be?			(ksb)
 * Aborts options all go to the state "OSAllAbort".
 * Forbid options that forbid an enable option also forbid any would-be
 * enable'd option that is itself a forbid option.
 */
static OSTATE *
StateVirt(pOS, pORVirt)
OSTATE *pOS;
OPTION *pORVirt;
{
	auto char acNewForbid[2*MAXOPTS+2];

	if (ISABORT(pORVirt)) {
		return & OSAllAbort;
	}
	sprintf(acNewForbid, "%c%s", pORVirt->chname, pOS->pcexcl);
	struniq(acNewForbid);
	if (ISFORBID(pORVirt)) {
		XansForbid(acNewForbid, pORVirt->pchforbid, pORVirt);
	}
	return StateFind(pOS->pcallow, acNewForbid, nil);
}

/* state and								(ksb)
 * NB: pcTo can == pcA
 * "aixyz" "aix" -> "aix"
 */
static char *
SAnd(pcTo, pcA, pcB)
register char *pcTo, *pcA, *pcB;
{
	register char *pcOrig = pcTo;

	while ('\000' != *pcA && '\000' != *pcB) {
		if (*pcA == *pcB) {
			*pcTo++ = *pcB++, ++pcA;
			continue;
		}
		if (*pcA < *pcB)
			++pcA;
		else
			++pcB;
	}
	*pcTo = '\000';
	return pcOrig;
}

/* put the letters in the order we want to see (aAbB...)		(ksb)
 */
static int
IsDictOrder(cL, cR)
int cL, cR;
{
	if (isupper(cL)) {
		if (isupper(cR))
			return cL < cR;
		return tolower(cL) < cR;
	}
	if (isupper(cR)) {
		return cL <= tolower(cR);
	}
	return cL < cR;
}

/* merge two bundles (zip them)						(ksb)
 * pBBOut->ppORblist is big enough to hold the combined list
 * and there are no identical options in the bundles
 */
void
BundMerge(pBBOut, pBBLeft, pBBRight, pfiCmp)
BUNDLE *pBBOut, *pBBLeft, *pBBRight;
int (*pfiCmp)();
{
	auto int l, r;

	pBBOut->pBBnext = (BUNDLE *)0;
	pBBOut->ibmembers = 0;
	if ((BUNDLE *)0 == pBBLeft) {
		if ((BUNDLE *)0 == pBBRight) {
			return;
		}
		pBBLeft = pBBRight;
		pBBRight = (BUNDLE *)0;
	}
	if ((BUNDLE *)0 != pBBRight && pBBLeft->ipipes != pBBRight->ipipes) {
		fprintf(stderr, "%s: bundle merge fails sanity check\n", progname);
		exit(MER_INV);
	}
	pBBOut->ipipes = pBBLeft->ipipes;
	pBBOut->imand = pBBLeft->imand;
	if ((BUNDLE *)0 != pBBLeft)
		pBBOut->ibmembers += pBBLeft->ibmembers;
	if ((BUNDLE *)0 != pBBRight)
		pBBOut->ibmembers += pBBRight->ibmembers;

	if ((int (*)())0 == pfiCmp)
		pfiCmp = IsDictOrder;
	r = l = 0;
	while (r + l < pBBOut->ibmembers) {
		if (l == pBBLeft->ibmembers) {
			/* below */;
		} else if ((BUNDLE *)0 == pBBRight || r == pBBRight->ibmembers || (*pfiCmp)(pBBLeft->ppORblist[l]->chname, pBBRight->ppORblist[r]->chname)) {
			pBBOut->ppORblist[l+r] = pBBLeft->ppORblist[l];
			++l;
			continue;
		}
		pBBOut->ppORblist[l+r] = pBBRight->ppORblist[r];
		++r;
	}
}

/* after we build the proper groups for states we have to choose	(ksb)
 * an order to display them...
 */
typedef struct BDnode {
	OPTION **ppORfirst;	/* options we span			*/
	OSTATE *pOSto;		/* state we move to on these		*/
	short int ilen;		/* and number of options like us	*/
} BUNDDESCR;

/* the silly SGI "C" compiler complains about static forward
 * function declarations so moved them out here.  Sigh. -- ksb
 */
static void CycleSplit(), CyclePandora();

/* we are adding a bundle to the line and are limited by		(ksb)
 * having only one break bundle.
 * We know we don't have both BREAK and not BREAK options in a single bundle
 * because the top split won't do that to us.  Nor will it put two alternate
 * usages in the same bundle.
 * As a side effect we convert the bundle description to a BBnode.
 */
static void
CycleMerge(fp, pOSCur, pBDThis, ppBBTack, ppORCtl)
FILE *fp;
OSTATE *pOSCur;
BUNDDESCR *pBDThis;
OPTION **ppORCtl;
BUNDLE **ppBBTack;
{
	register OPTION *pORFirst;
	register BUNDLE *pBBBack, *pBBThis, **ppBBDown;
	register int i;
	auto BUNDLE BBMerge, BBCvt;
	auto OPTION *apORTemp[MAXOPTS];

#if DEBUG
	fprintf(stderr, "%s: %c moves from path %s to %s\n", progname, pBDThis->ppORfirst[0]->chname, pOSCur->pcpath, pBDThis->pOSto->pcpath);
#endif
	pORFirst = pBDThis->ppORfirst[0];
	/* convert this request to BB node format...
	 */
	BBCvt.ibmembers = pBDThis->ilen;
	BBCvt.ppORblist = pBDThis->ppORfirst;
	BBCvt.pBBnext = (BUNDLE *)0;
	BBCvt.imand = ISALTUSAGE(pORFirst) || ISENABLE(pORFirst);
	BBCvt.ipipes = 1;
	for (i = 0; i < BBCvt.ibmembers; ++i) {
		pBDThis->ppORfirst[i]->gattr |= GEN_BUNDLE;
	}

	if (ISALTUSAGE(pORFirst)) {
		ppORCtl = & pORFirst->pORsact;
	}

	/* if we go in the tail bundle fake the params we it looks like
	 * the caller knew that.  He doesn't look, be we can fake that.
	 */
	if (ISBREAK(pORFirst)) {
		ppBBDown = ppBBTack;
		ppBBTack = & pBBTail;
		BBMerge.ppORblist = apORTemp;
		BundMerge(& BBMerge, & BBCvt, pBBTail, IsDictOrder);
		pBBThis = & BBMerge;
	} else {
		ppBBDown = & BBCvt.pBBnext;
		pBBThis = & BBCvt;
	}

	/* N.B. state locks out returns from here to end of function -- ksb
	 * where we put this pointer back
	 */
	{
		pBBBack = *ppBBTack;
		*ppBBTack = pBBThis;
		CycleSplit(fp, pBDThis->pOSto, ppBBDown, ppORCtl);
		*ppBBTack = pBBBack;
	}
	/* end critical region
	 */
	for (i = 0; i < BBCvt.ibmembers; ++i) {
		pBDThis->ppORfirst[i]->gattr &= ~GEN_BUNDLE;
	}
}

/* Transitions to some less complex states can be folded into more	(ksb)
 * than one transition into more complex states.  As an example (Test/b.m):
 * at the first split we see:
 * [b] lead to mkcmd: substate: 40014038 excludes [ab] path [b]
 * [x] lead to mkcmd: substate: 400140a8 excludes [xy] path [x]
 * [a] lead to mkcmd: substate: 40014000 excludes [abi] path [a]
 * [i] lead to mkcmd: substate: 40014070 excludes [aiy] path [i]
 * [y] lead to mkcmd: substate: 400140e0 excludes [ixy] path [y]
 * [h] lead to mkcmd: substate: in the abort state
 * We can fold two of these transitions ([b] and [x]) because the other 4
 * can reach more "interesting" states (those that include more information
 * but sill cover the "less interesting" cases).
 * {The [a] transition kills the [x] and the [y] kills the [b].}
 * We want to see:
 *	[-bx] [-i level]
 *	[-a | -b] [-y]
 *	[-x | -y] [-a]
 * killing the redundant:
 *	[-by]
 *	[-az]
 *
 * The math to figure this out is less than obvious, like the rest of this
 * part of mkcmd.
 * If we can find N states on the transition list that don't forbid all our
 * options (those that we transition on) and together allow all the options
 * we forbid (at least once) then we don't need this transition because
 * those states will pick up all the combinations we would.
 *
 * N.B. These cases stop us:
 *	Special endings (we need to show that the ending is optional)
 *	Stops/abort states (they won't do the combinations)
 *	Enable optons (we might mix with those enabled)
 *ZZZ	one that allready killed another? ZZZ
 */
static int
NoFold(pBDTest)
BUNDDESCR *pBDTest;
{
	return ISALTUSAGE(pBDTest->ppORfirst[0]) ||
		ST_STOPS(pBDTest->pOSto) || & OSAllAbort == pBDTest->pOSto ||
		ISENABLE(pBDTest->ppORfirst[0]);
}
static void
mkFfold(pBDList, piLen)
BUNDDESCR *pBDList;
int *piLen;
{
	register int j, i;
	register BUNDDESCR *pBDThis, *pBDKiller;
	auto char acAnd[MAXOPTS], acTemp[MAXOPTS];
	auto short int aiFold[MAXOPTS];

	if (1 >= *piLen) {
		return;
	}
	/* search all transitions for transitions that can be killed
	 */
	for (i = 0; i < *piLen; /* maybe ++i, maybe we shorten *piLen */) {
		pBDThis = & pBDList[i];
		if (NoFold(pBDThis)) {
			++i;
			continue;
		}

		/* build the transition option list
		 */
		for (j = 0; j < pBDThis->ilen; ++j) {
			acTemp[j] = pBDThis->ppORfirst[j]->chname;
		}
		acTemp[j] = '\000';

		/* found one, look for a killer transition
		 * Oh, don't kill yourself, BTW.
		 */
		pBDKiller = (BUNDDESCR *)0;
		for (j = 0; j < *piLen; ++j) {
			aiFold[j] = 0;
			pBDKiller = & pBDList[j];
			if (i == j || NoFold(pBDKiller)) {
				continue;
			}
			SAnd(acAnd, acTemp, pBDKiller->pOSto->pcexcl);
			if ('\000' != acAnd[0]) {
				continue;
			}
			SAnd(acAnd, pBDThis->pOSto->pcexcl, pBDKiller->pOSto->pcexcl);
			if ('\000' != acAnd[0]) {
				continue;
			}
			break;
		}
		if (j == *piLen) {
			++i;
			continue;
		}
		/* keep the "more interesting" of the two
		 */
		if ((BUNDDESCR *)0 != pBDKiller && strlen(pBDKiller->pOSto->pcexcl) >= strlen(pBDThis->pOSto->pcexcl)) {
			j = i;
		}
#if DEBUG
		fprintf(stderr, "%s: %d [%c]\n", progname, j, pBDList[j].ppORfirst[0]->chname);
#endif

		/* and remove the extra state transition now..
		 */
		--*piLen;
		for (/* killed is j */; j < *piLen; ++j) {
			pBDList[j] = pBDList[j+1];
		}
	}
}

/* we might delay transitions across forbid options that have		(ksb)
 * partners that are enabled by an option we can see.
 * This is a not-so-obvious (oh like suddenly the rest of this file
 * becomes clear) way to force [-a | -b] bundles where we can.
 * Until the allowed option we'll just be a plain visible option,
 * if all our patners are hidden with pins.
 */
static int
MayDelayX(pORCache)
OPTION *pORCache;
{
	register char *pcExcl;
	register OPTION *pORForbid;

	if (ISALTUSAGE(pORCache) || ISENABLE(pORCache) || !ISFORBID(pORCache)) {
		return 0;
	}

	/* If all our forbid options are pin'd save the transition, just
	 * act as a global option. (We don't count of course.)
	 */
	for (pcExcl = pORCache->pchforbid; '\000' != *pcExcl; ++pcExcl) {
		if (*pcExcl == pORCache->chname)
			continue;
		pORForbid = newopt(*pcExcl, & pORRoot, 0, 0);
		if ((OPTION *)0 == pORForbid)
			continue;
		if (0 == (pORForbid->oattr & OPT_PIN))
			return 0;
	}
	return 1;
}

/* a second level sort to choose bundles to traverse in order:		(ksb)
 * most options first, then fewest excludes, then alpha by option
 */
static int
BundOrder(pGenLeft, pGenRight)
QSORT_ARG *pGenLeft, *pGenRight;
{
	register int l, r, d;
	register BUNDDESCR *pBDLeft, *pBDRight;

	pBDLeft = (BUNDDESCR *)pGenLeft, pBDRight = (BUNDDESCR *)pGenRight;
	if (0 != (d = strlen(pBDLeft->pOSto->pcallow) - strlen(pBDRight->pOSto->pcallow))) {
		return d;
	}
	if (0 != (d = strlen(pBDLeft->pOSto->pcexcl) - strlen(pBDRight->pOSto->pcexcl))) {
		return d;
	}
#if 00
	if (0 != (d = pBDRight->ilen - pBDLeft->ilen)) {
		return d;
	}
#endif
	l = pBDLeft->ppORfirst[0]->chname;
	r = pBDRight->ppORfirst[0]->chname;
	if (isupper(l)) {
		if (isupper(r))
			return l - r ;
		d = tolower(l) - r;
		if (0 == d)
			return 1;
		return d;
	} else if (isupper(r)) {
		d = l - tolower(r);
		if (0 == d)
			return -1;
		return d;
	}
	return l - r;
}

/* traverse this allow option to get to a new state, but		(ksb)
 * do not add excludes
 * N.B. no fix is required for the STATE_STOPS bit because no allow option
 * should ever also stop option processing... (ksb)
 */
static OSTATE *
StateTrav(pOSFrom, pORFollow)
OSTATE *pOSFrom;
OPTION *pORFollow;
{
	auto char acNewPath[2*MAXOPTS+2], acNewAllow[2*MAXOPTS+2], acNewExcl[2*MAXOPTS+2];

	if (nil == pOSFrom->pcpath) {
		fprintf(stderr, "%s: inv. broken in TravState\n", progname);
		exit(MER_INV);
	}
	(void)strcpy(acNewExcl, pOSFrom->pcexcl);
	if (ISFORBID(pORFollow)) {
		XansForbid(acNewExcl, pORFollow->pchforbid, pORFollow);
	}
	sprintf(acNewPath, "%c%s", pORFollow->chname, pOSFrom->pcpath);
	sprintf(acNewAllow, "%c%s", pORFollow->chname, pOSFrom->pcallow);

	return StateFind(struniq(acNewAllow), struniq(acNewExcl), struniq(acNewPath));
}


/* remove the options from pBBFrom that we list in pcExcl		(ksb)
 * Pass in a BBnode with a MAXOPTS ppORblist set and we fill it in.
 */
static BUNDLE *
FilterTail(pBBDel, pBBFrom, pcExcl)
BUNDLE *pBBDel, *pBBFrom;
char *pcExcl;
{
	register int l;

	if ((BUNDLE *)0 == pBBFrom || (char *)0 == pcExcl) {
		return pBBFrom;
	}

	pBBDel->ipipes = pBBFrom->ipipes;
	pBBDel->imand = pBBFrom->imand;
	pBBDel->pBBnext = pBBFrom->pBBnext;
	pBBDel->ibmembers = 0;
	for (l = 0; l < pBBFrom->ibmembers; ++l) {
		if (ISENABLE(pBBFrom->ppORblist[l])) {
			continue;
		}
		pBBDel->ppORblist[pBBDel->ibmembers++] = pBBFrom->ppORblist[l];
	}

	/* Did we get them all?  Did we get none?
	 */
	if (0 == pBBDel->ibmembers) {
		return (BUNDLE *)0;
	}
	if (pBBFrom->ibmembers == pBBDel->ibmembers) {
		return pBBFrom;
	}
	return pBBDel;
}



/* For each aborts option we allow, which we call pandora's allows	(ksb)
 * we shift to a new state and descend with an empty control space
 * (we do abort, right?) and we also hide all breaks options, because
 * we can't get to them anymore.  We create a state to keep us from
 * coming back here, but we can't get to it again because we hide
 * ourself.
 */
static void
CyclePandora(fp, pOSCur, ppBBTack, pORPandora)
FILE *fp;
OSTATE *pOSCur;
BUNDLE **ppBBTack;
OPTION *pORPandora;
{
	register OPTION *pOR;
	auto OPTION *apORSingle[2];
	auto BUNDLE BBAbort;
	auto BUNDLE *pBBSave;
	auto OPTION *pOREmpty;

#if DEBUG
	fprintf(stderr, "%s: pandora %c\n", progname, pORPandora->chname);
#endif
	pBBSave = pBBTail;

	pOREmpty = apORSingle[1] = (OPTION *)0;
	BBAbort.ipipes = 1;
	BBAbort.imand = 1;
	BBAbort.pBBnext = (BUNDLE *)0;
	BBAbort.ibmembers = 1;
	BBAbort.ppORblist = apORSingle;
	apORSingle[0] = pORPandora;
	pORPandora->gattr |= GEN_BUNDLE;
	pBBTail = & BBAbort;
	for (pOR = pORRoot; nilOR != pOR; pOR= pOR->pORnext) {
		if (pOR != pORPandora->pORallow) {
			GLOCK(pOR, "pandora hides");
		}
	}
	/* just drop the pretext here */
	CyclePrint(fp, pOSCur, & pOREmpty);
	for (pOR = pORRoot; nilOR != pOR; pOR= pOR->pORnext) {
		if (pOR != pORPandora->pORallow) {
			GUNLOCK(pOR, "pandora hides");
		}
	}
	pORPandora->gattr &= ~GEN_BUNDLE;
	pBBTail = pBBSave;
}


/* We are forced into a new state, if this is a final state output	(ksb)
 * else try to get the the final state (all options excluded)
 * we are traversing a graph of all the combinations of `excludes' X
 * `options that got us here'.  Each option letter is an edge each
 * unique cross product is a verticy.  Final states have no edges out.
 *
 * If we were given a control line to output make sure we get to a
 * state that outputs it (ppORCtl != (OPTION **)0).
 */
static void
CycleSplit(fp, pOSCur, ppBBTack, ppORCtl)
FILE *fp;
OPTION **ppORCtl;
OSTATE *pOSCur;
BUNDLE **ppBBTack;
{
	register OPTION *pORThis;
	register OSTATE *pOS;
	register int j, i;
	register char *pcAllow, *pcRecord, *pcDemote;
	register BUNDLE *pBBKeepTail;	/* hold orig pBBTail */
	auto int iPacked, iBaked, iBoxed, iRemainder;
	auto int fShadow, fNarrow, fShowGlob, fPermute;
	auto OPTION *apORLens[MAXOPTS];
	auto OPTION *apORBaked[MAXOPTS];
	auto OSTATE *apOSMap[MAXOPTS];
	auto BUNDDESCR aBDOrder[MAXOPTS];
	auto BUNDLE BBDel;
	auto OPTION *apORDel[MAXOPTS];
	auto char acExclude[MAXOPTS], acEnable[MAXOPTS], acDemote[MAXOPTS];

#if DEBUG
	StatePrint(pOSCur, "split");
#endif
	if (ST_SEEN(pOSCur)) {
		return;
	}
	pOSCur->sattr |= STATE_SEEN;

	/* we need to map all the states we can get to then sort them
	 * and call each one once...  Jeeze.
	 */
	for (i = 0; i < MAXOPTS; ++i) {
		apORLens[i] = nilOR;
		apORBaked[i] = nilOR;
		apOSMap[i] = (OSTATE *)0;
		aBDOrder[i].ilen = 0;
	}

	/* find an option ready to be uncoverd that excludes and we
	 * do not exclude ourselves (if this option exclude an option in
	 * path don't take it) and add it to the mapping.
	 */
	fPermute = 0;
	iRemainder = iPacked = 0;
	pcRecord = acEnable;
	pcDemote = acDemote;
	for (pORThis = pORRoot; nilOR != pORThis; pORThis = pORThis->pORnext) {
		register int fEnable;
		register char *pcForce;
		auto char acForce[MAXOPTS];

		if (!ISVISIBLE(pORThis)) {
			continue;
		}
		/* the option may only enable hidden options, if so ignore
		 * the obvious state transiation but allow others.
		 */
		fEnable = 0;
		pcForce = acForce;
		if (ISENABLE(pORThis)) {
			register OPTION *pOR;

			for (pOR = pORRoot; nilOR != pOR; pOR= pOR->pORnext) {
				if (pOR->pORallow != pORThis) {
					continue;
				}
				if (0 != (pOR->oattr & (OPT_NOUSAGE|OPT_HIDDEN))) {
					continue;
				}
				*pcForce++ = pOR->chname;
				fEnable = 1;
			}
			if (fEnable) {
				*pcRecord++ = pORThis->chname;
				if (!DIDPERMUTE(pORThis)) {
					fPermute = 1;
				}
			} else {
				*pcDemote++ = pORThis->chname;
				pORThis->oattr &= ~OPT_ENABLE;
			}
		}
		*pcForce = '\000';
		if (!(ISFORBID(pORThis) || ISABORT(pORThis) || fEnable)) {
			++iRemainder;
			continue;
		}
		if (ISBREAK(pORThis) && ST_STOPS(pOSCur)) {
			continue;
		}

		/* look though the passed down allowed path for
		 * a reason we can't (really) exist
		 */
		if (nil != pORThis->pchforbid) {
			for (pcAllow = pOSCur->pcallow; '\000' != *pcAllow; ++pcAllow) {
				if (nil != strchr(pORThis->pchforbid, *pcAllow)) {
					break;
				}
			}
			if ('\000' != *pcAllow) {
#if DEBUG
				fprintf(stderr, "%s: option %c excludes a current allow option %c\n", progname, pORThis->chname, *pcAllow);
#endif
				continue;
			}
		}
		if (nil != strchr(pOSCur->pcexcl, pORThis->chname)) {
#if DEBUG
			fprintf(stderr, "%s: option %c excluded by this state (%s)\n", progname, pORThis->chname, pOSCur->pcexcl);
#endif
			continue;
		}
		/* we can and should delay transitions to forbid some
		 * forbid options, see comments at MayDelayX
		 */
		if (MayDelayX(pORThis)) {
			continue;
		}

		/* which state would we map to?
		 */
		if (fEnable) {
			pOS = StateTrav(pOSCur, pORThis);
		} else {
			pOS = StateVirt(pOSCur, pORThis);
		}
		pOS->sattr |= pOSCur->sattr & (STATE_STOPS|STATE_SYNTH);
		apORLens[pORThis->chname] = pORThis;
		apOSMap[pORThis->chname] = pOS;
		++iPacked;
	}
	*pcRecord = '\000';
	*pcDemote = '\000';
	/* we still have a "remainder" if we have a special action
	 * after the command line options and this state doesn't stop
	 */
	if (((OPTION **)0 != ppORCtl || (OPTION *)0 != pORActn) && !ST_STOPS(pOSCur)) {
		++iRemainder;
	}

	/* none left, now output the line we found and get out
	 */
	if (0 == iPacked) {
		if ('\000' == acDemote[0] || !ST_GENERIC(pOSCur)) {
			CyclePrint(fp, pOSCur, ppORCtl);
		}
		pOSCur->sattr |= STATE_GENERIC;
		goto cleanup;
	}

	/* create BUNDLEs for all the groups we find and call down
	 * aborts states we just call down (the hide the option).
	 *  + group options by common state, except aborts
	 *  + grouping aborts leads to   foo: usage -h | -V
	 *	which I don't like.  So there.
	 *  + also special endings are always unique
	 */
	iBoxed = iBaked = 0;
	for (i = 0; 0 < iPacked && i < MAXOPTS; ++i) {
		auto char acPath[MAXOPTS];
		register char *pcTemp;
		register BUNDDESCR *pBDThis;

		if ((OPTION *)0 == (pORThis = apORLens[i])) {
			continue;
		}

		/* map aborts into their own boxes in alpha order
		 */
		pBDThis = & aBDOrder[iBoxed++];
		pBDThis->ilen = 1;
		pBDThis->pOSto = (pOS = apOSMap[i]);
		pBDThis->ppORfirst = & apORBaked[iBaked];
		apORBaked[iBaked++] = pORThis;
		--iPacked;
		if (& OSAllAbort == pOS) {
			continue;
		}
		if (ISENABLE(pORThis)) {
			continue;
		}

		/* find options to put in the BUNDLE, they go to
		 * the same state, and break (or not) the same.
		 * only one with a special ending, please.
		 */
		pcTemp = acPath;
		*pcTemp++ = i;
		for (j = i+1; j < MAXOPTS; ++j) {
			if ((OSTATE *)0 == apOSMap[j] || pOS != apOSMap[j])
				continue;
			if (ISBREAK(apORLens[j]) != ISBREAK(pORThis))
				continue;
			if ((ISALTUSAGE(apORLens[j]) || ISALTUSAGE(pORThis)))
				continue;
			*pcTemp++ = apORLens[j]->chname;
			apORBaked[iBaked++] = apORLens[j];
			apORLens[j] = nilOR;
			apOSMap[j] = (OSTATE *)0;
			++(pBDThis->ilen);
			--iPacked;
		}
		(void)strcpy(pcTemp, pOSCur->pcpath);
		(void)struniq(acPath);

		if (nil == pOS->pcpath) {
			pOS->pcpath = malloc((strlen(acPath)|7)+1);
			(void)strcpy(pOS->pcpath, acPath);
		} else if (0 != strcmp(pOS->pcpath, acPath)) {
			pOS = StateFind(pOS->pcallow, pOS->pcexcl, acPath);
			pOS->sattr |= pOSCur->sattr & (STATE_STOPS|STATE_SYNTH);
			pBDThis->pOSto = apOSMap[i] = pOS;
		}
		if (ISBREAK(pORThis)) {
			pOS->sattr |= STATE_STOPS;
		}
	}
	if (0 != iPacked) {
		fprintf(stderr, "%s: 0 != %d up/down sanity failed in CycleSplit\n", progname, iPacked);
		exit(MER_INV);
	}

	/* now fold out (overlapping) trasitions we can emulate further down
	 * and sort the remaining ones
	 */
	iPacked = iBoxed;
	mkFfold(aBDOrder, & iBoxed);
	qsort((char *)aBDOrder, iBoxed, sizeof(aBDOrder[0]), BundOrder);

#if DEBUG
	iPacked -= iBoxed;
	if (0 != iPacked) {
		fprintf(stderr, "%s: state %p folded out %d substate%ss\n", progname, pOSCur, iPacked, 1 == iPacked ? "" : "s");
	}
	fprintf(stderr, "%s: state %p has %d substate%s:\n", progname, pOSCur, iBoxed, iBoxed == 1 ? "" : "s");
	for (i = 0; i < iBoxed; ++i) {
		fprintf(stderr, " %d options [", aBDOrder[i].ilen);
		for (j = 0; j < aBDOrder[i].ilen; ++j) {
			fprintf(stderr, "%c", aBDOrder[i].ppORfirst[j]->chname);
		}
		fprintf(stderr, "] lead to ");
		StatePrint(aBDOrder[i].pOSto, "substate");
		if (aBDOrder[i].ppORfirst[0] != apORLens[aBDOrder[i].ppORfirst[0]->chname]) {
			fprintf(stderr, " last state's lens is broken\n");
		}
		if (& OSAllAbort == aBDOrder[i].pOSto) {
			continue;
		}
		if ((char *)0 == strchr(aBDOrder[i].pOSto->pcpath, aBDOrder[i].ppORfirst[0]->chname)) {
			fprintf(stderr, " last state's path doesn't traverse option that keys' it\n");
		}
	}
#endif

	/* "narrow" states have forced options.  This state is narrow
	 * if all the states we will visit force at least one option.
	 * we "show global" if at least one state will show our ending
	 * control points.  Else we don't
	 */
	fShowGlob = 0;
	fNarrow = 1;
	for (i = 0; i < iBoxed; ++i) {
		pORThis = aBDOrder[i].ppORfirst[0];
		if (& OSAllAbort == aBDOrder[i].pOSto) {
			GLOCK(pORThis, "abort out");
			continue;
		}
		if (!ISALTUSAGE(pORThis)) {
			fShowGlob = 1;
		}
		if (ISENABLE(pORThis)) {
			continue;
		}
		fNarrow = 0;
	}
	/* if the state narrows and we have a remainder
	 * or if the state doesn't show global and we have a special end
	 * then we need to force a shadow state
	 */
	fShadow = (fNarrow && 0 != iRemainder) || fPermute || ((OPTION **)0 != ppORCtl && !fShowGlob);
#if DEBUG
	fprintf(stderr, "%s: state %p enable [%s,%s] narrow=%d show global=%d permute=%d: shadow=%d\n", progname, pOSCur, acEnable, acDemote, fNarrow, fShowGlob, fPermute, fShadow);
#endif

	/* We may need a state with no special action endings to show
	 * our special ending.  Hide the altusage options and shadow.
	 * We may to try a state where the enable options are not really
	 * enable options (we build a parallel fake state and call ourself).
	 * Or we can just call CyclePrint and hope for the best.
	 */
	if (!fShadow) {
		/* don't need to shadow this state */;
#if DEBUG
		fprintf(stderr, "\tnot shadowed\n");
#endif
	} else if (ST_GENERIC(pOSCur)) {
		/* we've already shadow'd this state
		 * this prevents infinite loops with the code below -- ksb
		 */;
#if DEBUG
		fprintf(stderr, "\talready shadowed %p\n", pOSCur);
#endif
	} else if ('\000' != acEnable[0]) {
		auto OSTATE OSNot;
		auto char acNotForbid[MAXOPTS*2+2], acTemp[MAXOPTS+2];
		auto char acAltUsage[MAXOPTS+2];

		pcRecord = acAltUsage;
		for (i = 0; i < iBoxed; ++i) {
			pORThis = aBDOrder[i].ppORfirst[0];
			if (! ISALTUSAGE(pORThis)) {
				continue;
			}
			*pcRecord++ = pORThis->chname;
			pORThis->oattr &= ~OPT_ALTUSAGE;
			pORThis->oattr |= OPT_HIDDEN;
		}
		*pcRecord = '\000';
		strcpy(acNotForbid, pOSCur->pcexcl);
		for (pcRecord = acEnable; '\000' != *pcRecord; ++pcRecord) {
			pORThis = newopt(*pcRecord, & pORRoot, 0, 0);
			pORThis->oattr &= ~OPT_ENABLE;
			CutEnable(acTemp, pORThis);
			XansForbid(acNotForbid, acTemp, pORThis);
			/* doesn't work for alt usage's  which we fix above */
		}
		OSNot.pcexcl = acNotForbid;
		OSNot.pcallow = pOSCur->pcallow;
		OSNot.pcpath = pOSCur->pcpath;
		OSNot.sattr = pOSCur->sattr & ~STATE_SEEN;
		OSNot.sattr |= STATE_SYNTH;
		OSNot.pOSnext = (OSTATE *)0;
#if DEBUG
		StatePrint(pOSCur, "shadow");
		fprintf(stderr, "\tshadow %p forbid [%s] lockout [%s]\n", &OSNot, acNotForbid, acAltUsage);
#endif
		CycleSplit(fp, &OSNot, ppBBTack, ppORCtl);
		for (pcRecord = acEnable; '\000' != *pcRecord; ++pcRecord) {
			pORThis = newopt(*pcRecord, & pORRoot, 0, 0);
			pORThis->oattr |= OPT_ENABLE;
		}
		pcRecord = acAltUsage;
		for (i = 0; '\000' != *pcRecord && i < iBoxed; ++i) {
			pORThis = aBDOrder[i].ppORfirst[0];
			if (pORThis->chname != *pcRecord)
				continue;
			++pcRecord;
			pORThis->oattr &= ~OPT_HIDDEN;
			pORThis->oattr |= OPT_ALTUSAGE;
		}
		pOSCur->sattr |= STATE_GENERIC;
	} else {
		pOSCur->sattr |= STATE_GENERIC;
#if DEBUG
		fprintf(stderr, "\tforce a print\n");
#endif
		CyclePrint(fp, pOSCur, ppORCtl);
	}

	/* now the more limited versions:
	 *	lock all the options that transition out of this state,
	 *	expose them only to call out
	 *	unlock the lot
	 */
	for (i = 0; i < iBoxed; ++i) {
		if (& OSAllAbort == aBDOrder[i].pOSto) {
			continue;
		}
	}
	pBBKeepTail = pBBTail;
	BBDel.ppORblist = apORDel;
	for (i = 0; i < iBoxed; pBBTail = pBBKeepTail, ++i) {
		register OPTION *pOR;
		register char *pcForbid;

		pORThis = aBDOrder[i].ppORfirst[0];
		if (& OSAllAbort == aBDOrder[i].pOSto) {
			GUNLOCK(pORThis, "aborts path");
			CyclePandora(fp, & OSAllAbort, ppBBTack, pORThis);
			GLOCK(pORThis, "aborts path");
			continue;
		}
		if (ISENABLE(pORThis)) {
			for (pOR = pORRoot; nilOR != pOR; pOR= pOR->pORnext) {
				if (pOR->pORallow != pORThis) {
					continue;
				}
				pOR->oattr &= ~OPT_PIN;
			}
		}
		aBDOrder[i].pOSto->sattr |= pOSCur->sattr & (STATE_GENERIC|STATE_SYNTH);
		/* ZZZ what condition to reset this? -- ksb
		 */
		if (ISENABLE(pORThis) || ISALTUSAGE(pORThis)) {
			aBDOrder[i].pOSto->sattr &= ~STATE_GENERIC;
		}
		pcRecord = acExclude;
		for (pcForbid = aBDOrder[i].pOSto->pcexcl; (char *)0 != pcForbid && '\000' != *pcForbid; ++pcForbid) {
			pOR = newopt(*pcForbid, & pORRoot, 0, 0);
			if (nilOR == pOR || ISHIDDEN(pOR))
				continue;
			*pcRecord++ = *pcForbid;
			GLOCK(pOR, "state forbids");
		}
		*pcRecord = '\000';
		for (pcForbid = aBDOrder[i].pOSto->pcpath; (char *)0 != pcForbid && '\000' != *pcForbid; ++pcForbid) {
			pOR = newopt(*pcForbid, & pORRoot, 0, 0);
			if (nilOR == pOR)
				continue;
			GLOCK(pOR, "state covers");
		}
		pBBTail = FilterTail(& BBDel, pBBTail, acExclude);
		CycleMerge(fp, pOSCur, & aBDOrder[i], ppBBTack, ppORCtl);
		if (ISENABLE(pORThis)) {
			for (pOR = pORRoot; nilOR != pOR; pOR= pOR->pORnext) {
				if (pOR->pORallow != pORThis) {
					continue;
				}
				pOR->oattr |= OPT_PIN;
			}
		}
		for (pcForbid = aBDOrder[i].pOSto->pcpath; (char *)0 != pcForbid && '\000' != *pcForbid; ++pcForbid) {
			pOR = newopt(*pcForbid, & pORRoot, 0, 0);
			if (nilOR == pOR)
				continue;
			GUNLOCK(pOR, "state covers");
		}
		for (pcRecord = acExclude; '\000' != *pcRecord; ++pcRecord) {
			pOR = newopt(*pcRecord, & pORRoot, 0, 0);
			GUNLOCK(pOR, "state forbids");
		}
	}
	for (i = 0; i < iBoxed; ++i) {
		if (& OSAllAbort == aBDOrder[i].pOSto) {
			GUNLOCK(aBDOrder[i].ppORfirst[0], "abort out");
		}
	}

	/* re-enable options that enabled only hidden options
	 */
 cleanup:
	for (pcDemote = acDemote; '\000' != *pcDemote; ++pcDemote) {
		pORThis = newopt(*pcDemote, & pORRoot, 0, 0);
		pORThis->oattr |= OPT_ENABLE;
	}

	/* we left some memory unfree'd, how sad. (?? XXX)
	 */
}

/* output the whole usage vector					(ksb)
 * setup the recursive usage line generation functions
 * mkuline via CyclePrint via CycleSplit
 */
int
mkuvector(fpDone, fNroff)
FILE *fpDone;
int fNroff;
{
	register OPTION *pORThis, **ppORCtl;
	register int i;
	auto OSTATE *pOSRoot;

	/* awTrack should be all zeros now, we coud check DEBUG HERE
	 * we set a static to tell use to output pretty nroff or
	 * C code.
	 */
	u_nroff = 0 != fNroff;
	u_vecstart(fpDone);

	/* the awAttr array still might need to be reset
	 */
	for (i = 0; i < sizeof(awAttr[0]); ++i) {
		awAttr[i] = 0;
	}

	/* restart state diagram
	 */
	StateReset();
	pOSRoot = StateFind("", "", "");

	/* put the pin in options that are enable'd (like a grenade)
	 * remove bits we set as we go
	 */
	for (pORThis = pORRoot; nilOR != pORThis; pORThis = pORThis->pORnext) {
		if (nilOR != pORThis->pORallow) {
			pORThis->oattr |= OPT_PIN;
		}
		pORThis->gattr &= ~(GEN_PERMUTE|GEN_SEEN);
	}

	/* found a stop/break option,  put them in a special bundle
	 * at the end
	 */
	pBBHead = pBBTail = (BUNDLE *)0;

	/* do we need to output the global special actions?
	 * if all we have is a "before" then we don't.
	 */
	if (nilOR != pORActn && ('b' != pORActn->chname || nilOR != pORActn->pORnext)) {
		ppORCtl = & pORActn;
	} else {
		ppORCtl = (OPTION **)0;
	}

	/* turn the (big ugly) crank
	 */
	CycleSplit(fpDone, pOSRoot, & pBBHead, ppORCtl);

	/* now finish cleanup
	 */
	for (pORThis = pORRoot; nilOR != pORThis; pORThis = pORThis->pORnext) {
		if (nilOR != pORThis->pORallow) {
			pORThis->oattr &= ~OPT_PIN;
		}
		/* check that we landed back at zero
		 */
		if (0 != awTrack[pORThis->chname]) {
			fprintf(stderr, "%s: abort option `-%c\' failed sanity check in usage generator (0 != %d)\n", progname, pORThis->chname, awTrack[pORThis->chname]);
			iExit |= MER_INV;
		}
	}
	u_vecend(fpDone);
	return u_ilines;
}

static int u_cvlines;

/* output the control params in a neato format				(ksb)
 */
static void
mkvcontrol(fp, ppORCntl)
FILE *fp;
OPTION **ppORCntl;
{
	register OPTION *pOR;
	register OPTION *pORLeft, *pORRight;
	register unsigned uFound;
	auto char acPad[80];

	(void)sprintf(acPad, "\n%-*s", iWidth+1, " ");
	if (nilOR != (pOR = newopt('a', ppORCntl, 0, 0)) && ISVISIBLE(pOR) && ! isempty(pOR->pchverb)) {
		fprintf(fp, "\t\t\"%-*s ", iWidth, pOR->pchdesc);
		Emit(fp, pOR->pchverb, pOR, acPad, 0);
		fprintf(fp, "\",\n");
		++u_cvlines;
	}

	pORLeft = newopt('s', ppORCntl, 0, 0);
	pORRight = newopt('d', ppORCntl, 0, 0);
	for (uFound = 0; uFound < (nilOR == pORLeft ? 0 : pORLeft->iorder); ++uFound) {
		pOR = pORLeft->ppORorder[uFound];
		if (isempty(pOR->pchverb))
			continue;
		if (ISDUPPARAM(pOR))
			continue;
		pOR->oattr |= OPT_DUPPARAM;
		fprintf(fp, "\t\t\"%-*s ", iWidth, pOR->pchdesc);
		Emit(fp, pOR->pchverb, pOR, acPad, 0);
		fprintf(fp, "\",\n");
		++u_cvlines;
	}
	if (nilOR != (pOR = newopt('l', ppORCntl, 0, 0)) && ISVISIBLE(pOR) && ! isempty(pOR->pchverb)) {
		fprintf(fp, "\t\t\"%-*s ", iWidth, pOR->pchdesc);
		Emit(fp, pOR->pchverb, pOR, acPad, 0);
		fprintf(fp, "\",\n");
		++u_cvlines;
	}
	if (nilOR != (pOR = newopt('e', ppORCntl, 0, 0)) && ISVISIBLE(pOR) && !isempty(pOR->pchverb)) {
		fprintf(fp, "\t\t\"%-*s ", iWidth, pOR->pchdesc);
		Emit(fp, pOR->pchverb, pOR, acPad, 0);
		fprintf(fp, "\",\n");
		++u_cvlines;
	}
	for (uFound = 0; uFound < (nilOR == pORRight ? 0 : pORRight->iorder); ++uFound) {
		pOR = pORRight->ppORorder[uFound];
		if (isempty(pOR->pchverb))
			continue;
		if (ISDUPPARAM(pOR))
			continue;
		pOR->oattr |= OPT_DUPPARAM;
		fprintf(fp, "\t\t\"%-*s ", iWidth, pOR->pchdesc);
		Emit(fp, pOR->pchverb, pOR, acPad, 0);
		fprintf(fp, "\",\n");
		++u_cvlines;
	}
	if (nilOR != (pOR = newopt('x', ppORCntl, 0, 0)) && ISVISIBLE(pOR) && ! isempty(pOR->pchverb)) {
		fprintf(fp, "\t\t\"%-*s ", iWidth, pOR->pchdesc);
		Emit(fp, pOR->pchverb, pOR, acPad, 0);
		fprintf(fp, "\",\n");
		++u_cvlines;
	}
}

/* make the name in colunm one of the verbose help table for		(ksb)
 * the option given (in the string buffer provided).
 */
static int
VecName(pcName, pcWord, pOR)
char *pcName, *pcWord;
OPTION *pOR;
{
	register char *pcDesc;

	if ((char *)0 != pOR->pchdesc) {
		pcDesc = pOR->pchdesc;
	} else if (ISALIAS(pOR)) {
		if ((char *)0 == (pcDesc = pOR->pORali->pchdesc)) {
			pcDesc = "";
		}
	} else {
		pcDesc = "";
	}

	if ('#' == pOR->chname) {
		if ('\000' == pcDesc[0]) {
			pcDesc = "number";
		}
		(void)sprintf(pcName, "-%s", pcDesc);
		*pcWord = '\000';
		return 1;
	}
	if ('+' == pOR->chname) {
		register char *pcGen;
		if ('\000' == pcDesc[0]) {
			pcDesc = "word";
		}
		if ((char *)0 == (pcGen = pOR->pchgen)) {
			fprintf(stderr, "%s: invar. broken for escape\n", progname);
			iExit |= MER_INV;
		}
		(void)sprintf(pcName, "%s%s", pcGen, pcDesc);
		*pcWord = '\000';
		return strlen(pcGen);
	}
	(void)sprintf(pcName, "%c", pOR->chname);
	(void)strcpy(pcWord, pcDesc);
	return 0;
}

/* output a verbose help vector						(ksb)
 */
int
mkvector(fp, pcVerbose)
register FILE *fp;
char *pcVerbose;
{
	register OPTION *pOR, *pORAlias;
	register int wtmp, iCost;
	register char *pcHelp;
	auto ATTR wKeepMask;
	auto char acPad[80], acName[132], acParam[132];
	auto char acCur[132], acWord[132];

	wKeepMask = wEmitMask & (EMIT_AUTO_IND|EMIT_QUOTE);
	wEmitMask |= EMIT_QUOTE;
	wEmitMask &= ~EMIT_AUTO_IND;

	(void)sprintf(acPad, "\n%-*s", iWidth+1, " ");
	u_cvlines = 0;
	fprintf(fp, "%c\n", LGROUP);
	if (nilOR != (pOR = newopt('b', & pORActn, 0, 0)) && ISVISIBLE(pOR) && ! isempty(pOR->pchverb)) {
		fprintf(fp, "\t\t\"%-*s ", iWidth, pOR->pchdesc);
		Emit(fp, pOR->pchverb, pOR, acPad, 0);
		fprintf(fp, "\",\n");
		++u_cvlines;
	}

	/* Looking at the primary options, if if is not visible (or
	 * documented) it and all of its aliases are ignored.
	 * Visible and documented options are scanned with all their
	 * aliases for placement on a verbose help line.
	 */
	pcHelp = (char *)0;	/* shut up gcc -Wall */
	for (pOR = pORRoot; nilOR != pOR; pOR = pOR->pORnext) {
		if (! ISVISIBLE(pOR) || isempty(pOR->pchverb) || ISALIAS(pOR)) {
			continue;
		}

		wtmp = 0;
		acCur[0] = '\000';
		acParam[0] = '\000';
		for (pORAlias = pOR; nilOR != pORAlias; pORAlias = pORAlias->pORalias) {
			if (! ISVISIBLE(pORAlias)) {
				continue;
			}

			(void)VecName(acName, acWord, pORAlias);
			iCost = strlen(acName);

			/* If we can merge this line with the last (because
			 * the params are the same and the help text is the
			 * same) add the name to the current with a pipe.
			 * If no last word then make this the only one.
			 */
			if ('\000' == acCur[0]) {
				/* fall into code at loop end */
			} else if (iWidth >= iCost + wtmp + 1 && ((char *)0 == pORAlias->pchverb || 0 == strcmp(pORAlias->pchverb, pcHelp)) && 0 == strcmp(acParam, acWord)) {
				(void)strcat(acCur, "|");
				(void)strcat(acCur, acName);
				wtmp += iCost+1;
				continue;
			} else {
				/* can't merge, dump out the old one
				 */
				if (DIDOPTIONAL(pORAlias)) {
					fprintf(fp, "\t\t\"%s%-*s", acCur, iWidth-wtmp+1, acParam);
				} else {
					fprintf(fp, "\t\t\"%s %-*s", acCur, iWidth-wtmp, acParam);
				}
				Emit(fp, pcHelp, pORAlias, acPad, 0);
				fprintf(fp, "\",\n");
				++u_cvlines;
			}
			/* make the current line the old one
			 */
			(void)strcpy(acCur, acName);
			(void)strcpy(acParam, acWord);
			pcHelp = (char *)0 != pORAlias->pchverb ? pORAlias->pchverb : pOR->pchverb;
			wtmp = iCost;
		}
		if ('\000' != acCur[0]) {
			if (DIDOPTIONAL(pOR)) {
				fprintf(fp, "\t\t\"%s%-*s", acCur, iWidth-wtmp+1, acParam);
			} else {
				fprintf(fp, "\t\t\"%s %-*s", acCur, iWidth-wtmp, acParam);
			}
			Emit(fp, pcHelp, pOR, acPad, 0);
			fprintf(fp, "\",\n");
			++u_cvlines;
		}
		if (ISALTUSAGE(pOR)) {
			mkvcontrol(fp, & pOR->pORsact);
		}
	}
	mkvcontrol(fp, & pORActn);
	fprintf(fp, "\t\t(char *)0\n\t%c", RGROUP);

	/* put the quote bit back as it was
	 */
	wEmitMask &= ~EMIT_QUOTE;
	wEmitMask |= wKeepMask;

	return ++u_cvlines;
}

/* compute the optimal value for iWidth based on what we might		(ksb)
 * emit in the verbose help vector.  The min width for any visible
 * option is 1 (`f help' for f).
 */
int
compwidth(pOR)
OPTION *pOR;
{
	register OPTION *pORAlias;
	register int wtmp, iCost, fCur, iNew, fBreak;
	register char *pcHelp;
	auto char acName[132], acParam[132], acWord[132];

	if (nilOR == pOR || ! ISVISIBLE(pOR) || isempty(pOR->pchverb) || ISALIAS(pOR)) {
		return 0;
	}

	pcHelp = (char *)0;	/* shut up gcc -Wall */
	iNew = 1;
	wtmp = 0;
	fCur = 0;
	acParam[0] = '\000';
	for (pORAlias = pOR; nilOR != pORAlias; pORAlias = pORAlias->pORalias) {
		if (! ISVISIBLE(pORAlias)) {
			continue;
		}

		fBreak = VecName(acName, acWord, pORAlias);
		iCost = strlen(acName);

		/* If we can merge this line with the last (because
		 * the params are the same and the help text is the
		 * same) add the name to the current with a pipe.
		 * If no last word then make this the only one.
		 */
		if (!fCur) {
			/* fall into code at loop end */
		} else if (!fBreak && ((char *)0 == pORAlias->pchverb || 0 == strcmp(pORAlias->pchverb, pcHelp)) && 0 == strcmp(acParam, acWord)) {
			wtmp += iCost+1;
			continue;
		} else if (wtmp > iNew) {
			iNew = wtmp;
		}
		/* make the current line the old one
		 */
		pcHelp = (char *)0 != pORAlias->pchverb ? pORAlias->pchverb : pOR->pchverb;
		(void)strcpy(acParam, acWord);
		fCur = 1;
		if ('\000' != acParam[0]) {
			wtmp = iCost + strlen(acParam) + !DIDOPTIONAL(pORAlias);
		} else {
			wtmp = iCost;
		}
	}
	if (fCur && wtmp > iNew) {
		iNew = wtmp;
	}
	return iNew;
}
