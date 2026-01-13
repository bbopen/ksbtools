/* $Id: check.c,v 8.8 1998/09/19 21:14:40 ksb Exp $
 */
#include <stdio.h>
#include <ctype.h>

#include "machine.h"
#include "main.h"
#include "type.h"
#include "option.h"
#include "scan.h"
#include "parser.h"
#include "check.h"
#include "list.h"
#include "mkcmd.h"
#include "emit.h"
#include "key.h"
#include "routine.h"

extern int compwidth(/* pOR */);

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

/* find a buffer [declared as <type> variable "ident" in mkcmd]		(ksb)
 * in the list of buffer (usually pORDecl)
 */
OPTION *
FindBuf(pORList, pcName)
OPTION *pORList;
char *pcName;
{
	while (nilOR != pORList) {
		if (0 == strcmp(pORList->pchname, pcName))
			return pORList;
		pORList = pORList->pORnext;
	}
	return nilOR;
}

/* Creates a sorted, uniqued string in place.				(ksb)
 * If the string is empty, or has one character return it
 * Else (skipping the first character) check to see if the
 * previous character is < us, until we are the '\000'.
 * then go crazy with bits.  This routine can account for 60% of
 * mkcmd's CPU usage so we tuned it a little -- ksb
 */
char *
struniq(str)
char *str;
{
	register char *pcUp;
	register unsigned char c;
	register unsigned int i, j;
	static unsigned short aiHas[16];

	pcUp = str;
	if ('\000' == *pcUp) {
		return str;
	}
	do {
		++pcUp;
	} while (pcUp[-1] < pcUp[0]);

	if ('\000' == *pcUp) {
		return str;
	}

	for (pcUp = str; '\000' != (c = *pcUp); ++pcUp) {
		register int iHi;
		iHi = (c >> 4);
		aiHas[0x0f & iHi] |= 1 << (0x0f & c);
	}
	for (j = 0, pcUp = str; j < 16; ++j) {
		if (0 == (i = aiHas[j])) {
			continue;
		}
		for (c = j << 4; 0 != i; ++c, i >>= 1) {
			if (1 & i) {
				*pcUp++ = c;
			}
		}
		aiHas[j] = 0;
	}
	*pcUp = '\000';
	return str;
}

/* this trys to emulate shell quoting, but I doubt it does a good job	(ksb)
 * [[ but not substitution -- that would be silly ]]
 */
static char *
f_mynext(u_pcScan, u_pcDest)
register char *u_pcScan, *u_pcDest;
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
		if ((char *)0 != u_pcDest) {
			*u_pcDest++ = *u_pcScan;
		}
	}
	if ((char *)0 != u_pcDest) {
		*u_pcDest = '\000';
	}
	return u_pcScan;
}

/* given an envirionment variable insert it in the option list		(ksb)
 * (exploded with the above routine)
 */
static int
f_envopt(cmd, pargc, pargv)
char *cmd, *(**pargv);
int *pargc;
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
		p = f_mynext(p, (char *)0);
		sum += sizeof(char *) + 1 + (unsigned)(p - s);
		while (*p == ' ' || *p == '\t')
			p++;
	}
	++i;
	/* vector starts at v, copy of string follows NULL pointer
	 * the extra 7 bytes on the end allow use to be alligned
	 */
	v = (char **)malloc(sum+sizeof(char *)+7);
	if (v == NULL)
		return 0;
	p = (char *)v + i * sizeof(char *); /* after NULL pointer */
	i = 0;				/* word count, vector index */
	v[i++] = (*pargv)[0];
	while (*cmd != '\000') {
		v[i++] = p;
		cmd = f_mynext(cmd, p);
		p += strlen(p)+1;
		while (*cmd == ' ' || *cmd == '\t')
			++cmd;
	}
	for (j = 1; j < *pargc; ++j)
		v[i++] = (*pargv)[j];
	v[i] = NULL;
	*pargv = v;
	*pargc = i;
	return i;
}

/* a hacked up getopt to scan what we are going to push when we		(ksb)
 * see a basename, just to make sure it is options
 */
static int
chk_getopt(pcBase, fargc, fargv, pORNumber, pOREscape)
int fargc;
char *pcBase, **fargv;
OPTION *pORNumber, *pOREscape;
{
	register OPTION	*pOR;		/* option letter list index	*/
	static char	EMSG[] = "";	/* just a null place		*/
	register char	*place = EMSG;	/* option letter processing	*/
	register int foptind, iOpt;
	register int retval = 0;
	register OPTION *pORStopBreak;
	auto char acForb[256+1], acGiven[256+1];

	pORStopBreak = nilOR;
	foptind = 1;
	place = EMSG;
	acForb[0] = '\000';
	acGiven[0] = '\000';
	for (;;) {
		if ('\000' == *place) {		/* update scanning pointer */
			if (foptind >= fargc) {
				return retval;
			}
			if (fargv[foptind][0] != '-') {
				if (nilOR != pOREscape && 0 == strncmp(pOREscape->pchgen, fargv[foptind], strlen(pOREscape->pchgen))) {
					foptind++;
					pOR = pOREscape;
					goto check_excl;
				}
				fprintf(stderr, "%s: basename `%s\' forces %s (%s%s)\n", progname, pcBase, foptind+1 < fargc ? "nonoption words" : "a nonoption word", fargv[foptind], foptind+1 < fargc ? "..." : "");
				return 2 | retval;
			}
			place = fargv[foptind];
			if ('\000' == *++place)	{ /* "-" (stdin) */
				fprintf(stderr, "%s: basename `%s\' forces a single dash (-) into the command line options\n", progname, pcBase);
				++foptind;
				retval |= 2;
				continue;
			}
			if (*place == '-' && '\000' == place[1]) { /* found "--" */
				fprintf(stderr, "%s: basename `%s\' forces a double dash (--) into the command line\n", progname, pcBase);
				++foptind;
				++place;
				retval |= 2;
				continue;
			}
		}				/* option letter okay? */
		/* if we find the letter, (not a `:')
		 * or a digit to match a # in the list
		 */
		if (':' == (iOpt = *place++)) {
			fprintf(stderr, "%s: basename `%s\' forces a colon (-:) into the command line, which is never good\n", progname, pcBase);
			retval |= 2;
			if ('\000' == *place)
				++foptind;
			continue;
		}
		if (nilOR != (pOR = newopt(iOpt, & pORRoot, 0, 0))) {
			/* found one we know */;
		} else if ((isdigit(iOpt) || '-'==iOpt)) {
			if (nilOR == (pOR = pORNumber)) {
				fprintf(stderr, "%s: basename `%s\' forces a number (%s) into the command line, but no number allowed\n", progname, pcBase, place -1);
				retval |= 2;
			}
			++foptind;
			place = EMSG;
		} else {
			fprintf(stderr, "%s: basename `%s\' forces an unknown option (`-%c\') into the command line\n", progname, pcBase, iOpt);
			return retval | 2;
		}
check_excl:
		if (nilOR != pORStopBreak) {
			fprintf(stderr, "%s: basename `%s\' forces %s after", progname, pcBase, usersees(pOR, nil));
			fprintf(stderr, " %s [which %s]\n", usersees(pORStopBreak, nil), ISABORT(pORStopBreak) ? "aborts" : "stops option processing");
			retval |= 2;
		} else if (ISBREAK(pOR) || ISABORT(pOR)) {
			pORStopBreak = pOR;
		}

		/* does some other option exclude this?
		 */
		if (nil != strchr(acForb, pOR->chname)) {
			register char *pcSearch;
			register OPTION *pORSearch;

			fprintf(stderr, "%s: basename `%s\' forces mutually exclusive %s and", progname, pcBase, usersees(pOR, nil));
			for (pcSearch = pOR->pchforbid; '\000' != *pcSearch; ++pcSearch) {
				if (nil == strchr(acGiven, *pcSearch))
					continue;
				if (nilOR == (pORSearch = newopt(*pcSearch, & pORRoot, 0, 0)))
					continue;
				fprintf(stderr, " %s", usersees(pORSearch, nil));
				break;
			}
			if ('\000' == *pcSearch) {
				fprintf(stderr, "<humph!>");
			}
			fprintf(stderr, "\n");
			retval |= 2;
		}
		if (ISFORBID(pOR)) {
			register char *pcEnd;
			(void)strcat(acForb, pOR->pchforbid);
			(void)struniq(acForb);
			pcEnd = strlen(acGiven) + acGiven;
			*pcEnd++ = pOR->chname;
			*pcEnd = '\000';
			(void)struniq(acGiven);
		}
		if (sbBInit == pOR->pOTtype->pchdef) {
			if ('\000' == *place)
				++foptind;
		} else {				/* need an argument */
			if ('\000' != *place) {
				/* no white space */
			} else if (fargc <= ++foptind) {	/* no arg!! */
				place = EMSG;
				fprintf(stderr, "%s: basename `%s\' forces %s into the command line, but no parameter is provided for %s\n", progname, pcBase, usersees(pOR, nil), pOR->pchdesc);
				return 2 | retval;
			} else {
				/* white space */
			}
			place = EMSG;
			++foptind;
		}
	}
	/*NOTREACHED*/
}

/* we need to build some implied excludes, all `stops' attribute	(ksb)
 * force an exclusion with all other stops.  Aborts are handled
 * in a more dramatic way in mkusgae.c
 */
char *
SynthExclude(pOR)
OPTION *pOR;
{
	auto char acSynth[256];	/* MAXOPTS */
	register char *pc;

	for (pc = acSynth; nilOR != pOR; pOR = pOR->pORnext) {
		if (! ISBREAK(pOR))
			continue;
		*pc++ = pOR->chname;
	}
	*pc = '\000';

	/* if only zero or one `stops' don't produce an error message
	 */
	if ('\000' == acSynth[0] || '\000' == acSynth[1])
		return (char *)0;

	pc = malloc((strlen(acSynth)|7)+1);
	if ((char *)0 != pc)
		return strcpy(pc, acSynth);
	return (char *)0;
}

/* fix the control points for an alternate ending (or the real one)	(ksb)
 */
static int
fixcontrol(ppORSAct)
OPTION **ppORSAct;
{
	register OPTION *pOR;
	register char **ppcList;
	register OPTION *pORLeft, *pORRight;
	register OPTION **ppORLeft, **ppORRight;
	register int i, retval;
	register char *pc;

	retval = 0;
	pORLeft = newopt('s', ppORSAct, 0, 0);
	pORRight = newopt('d', ppORSAct, 0, 0);

	if (nilOR != pORLeft && (char **)0 != pORLeft->ppcorder) {
		auto int *piCount;	/* gcc -Wall is wrong -- ksb */
		auto int iSawOptPar, iDepth;
		auto ATTR oNext;

		iDepth = iSawOptPar = 0;
		pORLeft->iorder = 0;
		for (ppcList = pORLeft->ppcorder; (char *)0 != *ppcList; ++ppcList) {
			if (acOpenOpt == *ppcList) {
				iSawOptPar = 1;
				++iDepth;
				continue;
			}
			if (acCloseOpt == *ppcList) {
				--iDepth;
				continue;
			}
			if (0 == iDepth && 0 != iSawOptPar) {
				fprintf(stderr, "%s: %s: all left parameters after the first optional one must be optional, or right justified\n", progname, *ppcList);
				retval |= 2;
			}
			++(pORLeft->iorder);
		}
		ppORLeft = (OPTION **)calloc(pORLeft->iorder+1,sizeof(OPTION *));
		if ((OPTION **)0 == ppORLeft) {
			fprintf(stderr, acOutMem, progname);
			exit(MER_LIMIT);
		}
		i = 0;
		oNext = 0;
		for (ppcList = pORLeft->ppcorder; (char *)0 != *ppcList; ++ppcList) {
			if (acOpenOpt == *ppcList) {
				oNext = OPT_OOPEN;
				continue;
			}
			if (acCloseOpt == *ppcList) {
				if (OPT_OOPEN != oNext) {
					ppORLeft[i-1]->oattr |= OPT_OCLOSE;
				}
				oNext = 0;
				continue;
			}
			pOR = FindBuf(pORDecl, *ppcList);
			if (nilOR == pOR) {
				fprintf(stderr, "%s: variable `%s\' in left list not found\n", progname, *ppcList);
				retval |= 2;
				continue;
			}
			if (ISDUPPARAM(pOR)) {
				fprintf(stderr, "%s: variable `%s\' listed more than once in left list\n", progname, pOR->pchname);
				retval |= 2;
				continue;
			}
			if (nil != pchVector && nil == pOR->pchverb) {
				fprintf(stderr, "%s: left parameter `%s\' doesn\'t have a verbose help message for the usage function\n", progname, pOR->pchname);
				pOR->pchverb = pOR->pOTtype->pchhelp;
			}
			if (0 == i || OPT_OOPEN == oNext) {
				piCount = & pOR->ibundle;
				*piCount = 1;
			} else {
				(*piCount)++;	/* NB: precedence here! */
			}
			ppORLeft[i++] = pOR;
			pOR->oattr |= OPT_DUPPARAM|OPT_PPARAM|oNext;

			/* the manual page gen needs to quickly now if a
			 * pparam is optional (thus the default is important)
			 */
			if (piCount != & ppORLeft[0]->ibundle || ISOOPEN(ppORLeft[0])) {
				pOR->gattr |= GEN_INOPT;
			}
			pOR->pOTtype->tattr |= _JARG;
			if (ISVERIFY(pOR)) {
				retval |= 1;
				pOR->pOTtype->tattr |= _CHK;
			}
			oNext = 0;
		}
		ppORLeft[i] = (OPTION *)0;
		pORLeft->ppORorder = ppORLeft;
		if (0 != (2 & retval)) {
			return retval;
		}
	} else {
		ppORLeft = (OPTION **)0;
	}

	if (nilOR != pORRight && (char **)0 != pORRight->ppcorder) {
		pORRight->iorder = 0;
		for (ppcList = pORRight->ppcorder; (char *)0 != *ppcList; ++ppcList) {
			if (acOpenOpt == *ppcList || acCloseOpt == *ppcList) {
				fprintf(stderr, "%s: optional right justified parameters not supported (yet)\n", progname);
				exit(MER_SEMANTIC);
			}
			++(pORRight->iorder);
		}
		ppORRight = (OPTION **)calloc(pORRight->iorder+1,sizeof(OPTION *));
		if ((OPTION **)0 == ppORRight) {
			fprintf(stderr, acOutMem, progname);
			exit(MER_LIMIT);
		}
		i = 0;
		for (ppcList = pORRight->ppcorder; (char *)0 != *ppcList; ++ppcList) {
			register int j;

#if 0
			if (acOpenOpt == *ppcList || acCloseOpt == *ppcList)
				continue;
#endif
			pOR = FindBuf(pORDecl, *ppcList);
			if (nilOR == pOR) {
				fprintf(stderr, "%s: variable `%s\' in right list not found\n", progname, *ppcList);
				retval |= 2;
				continue;
			}
			for (j = 0; j < i; ++j) {
				if (ppORRight[j] == pOR) {
					fprintf(stderr, "%s: variable `%s\' listed more than once in right list\n", progname, pOR->pchname);
					retval |= 2;
					break;
				}
			}
			if (nilOR != pORLeft) {
				for (j = 0; j < pORLeft->iorder; ++j) {
					if (ppORLeft[j] == pOR) {
						fprintf(stderr, "%s: variable `%s\' listed in left and right lists\n", progname, pOR->pchname);
						retval |= 2;
						break;
					}
				}
			}
			if (nil != pchVector && nil == pOR->pchverb) {
				fprintf(stderr, "%s: right parameter `%s\' doesn't have a verbose help message for the usage function\n", progname, pOR->pchname);
				pOR->pchverb = pOR->pOTtype->pchhelp;
			}
			ppORRight[i++] = pOR;
			pOR->oattr |= OPT_DUPPARAM|OPT_PPARAM;
			pOR->pOTtype->tattr |= _JARG;
			if (ISVERIFY(pOR)) {
				retval |= 1;
				pOR->pOTtype->tattr |= _CHK;
			}
		}
		ppORRight[0]->ibundle = i;
		ppORRight[i] = (OPTION *)0;
		pORRight->ppORorder = ppORRight;
		if (0 != (2 & retval)) {
			return retval;
		}
	}

	/* reset DUPPARAM bits
	 */
	if (nilOR != pORLeft && (char **)0 != pORLeft->ppcorder) {
		for (ppcList = pORLeft->ppcorder; (char *)0 != *ppcList; ++ppcList) {
			pOR = FindBuf(pORDecl, *ppcList);
			if (nilOR != pOR)
				pOR->oattr &= ~OPT_DUPPARAM;
		}
	}
	if (nilOR != pORRight && (char **)0 != pORRight->ppcorder) {
		for (ppcList = pORRight->ppcorder; (char *)0 != *ppcList; ++ppcList) {
			pOR = FindBuf(pORDecl, *ppcList);
			if (nilOR != pOR)
				pOR->oattr &= ~OPT_DUPPARAM;
		}
	}

	for (pc = "baelx"; '\000' != *pc; ++pc) {
		if (nilOR == (pOR = newopt(*pc, ppORSAct, 0, 0))) {
			continue;
		}
		if ((int (*)())0 != pOR->pOTtype->pfifix) {
			retval |= (*pOR->pOTtype->pfifix)(pOR);
		}
		/* GEN_OPTIONAL is ignored on left/every and the like
		 */
		if (nil == pOR->pchuupdate) {
			switch (pOR->chname) {
			case 'l':
				pOR->pchuupdate = "%n(%#, %N);";
				break;
			case 'e':
				pOR->pchuupdate = pOR->pOTtype->pchevery;
				break;
			default:
				pOR->pchuupdate = pOR->pOTtype->pchupdate;
				break;
			}
		}
		if ((ROUTINE *)0 != pOR->pRG) {
			retval |= CheckRoutine(pOR);
		}
		if (! ISVISIBLE(pOR)) {
			continue;
		}

		if (nil != pchVector && nil == pOR->pchverb && nil != pOR->pchdesc && '\000' != pOR->pchdesc[0]) {
			fprintf(stderr, "%s: control `%s\' doesn't have a verbose help message for the verbose vector\n", progname, (char *)0 != pOR->pchname ? pOR->pchname : sactstr(pOR->chname));
			pOR->pchverb = pOR->pOTtype->pchhelp;
		}

		i = strlen(pOR->pchdesc);
		if (i > iWidth) {
			iWidth = i;
		}
		if (! ISVERIFY(pOR))
			continue;
		retval |= 1;
		if (nil == pOR->pchverify) {
			pOR->pchverify = pOR->pOTtype->pchchk;
			pOR->pOTtype->tattr |= _CHK;
		}
		if (nil == pOR->pchverify) {
			fprintf(stderr, "%s: variable type for `%s\' has no default action to verify it\n", progname, pOR->pchname);
			pOR->oattr &= ~OPT_VERIFY;
			retval |= 2;
		}
	}
	return retval;
}

/* simple string compare interface for qsort call below			(ksb)
 */
static int
QCmp(pcA1, pcA2)
QSORT_ARG *pcA1, *pcA2;
{
	register char *pcLeft, *pcRight;

	pcLeft = *(char **)pcA1;
	pcRight = *(char **)pcA2;
	return strcmp(pcLeft, pcRight);
}


/* patch up the options so the rest of the job is easy			(ksb)
 * return value is 0/1 for "Do we need to include ctype.h?"
 * set the magic key mkcmd_verifies and mkcmd_declares for the code gen.
 */
int
FixOpts()
{
	register OPTION *pOR, *pORAlias;
	register int i, iFound;
	register char *pcAllows, *pcBreaks;
	auto int retval, s;
	auto OPTION *pORNumber, *pOREscape;
	auto char acBExcl[256], acAExcl[256], acAccum[514];

	/* resolve all the synthetic types used (turn them into
	 * things that look like std types)
	 */
	for (s = 0, pOR = nilOR; nilOR != (pOR = OptScanAll(&s, pOR)); ) {
		if (! IS_SYNTH(pOR->pOTtype))
			continue;
		if (0 != SynthResolve(pOR->pOTtype))
			iExit |= MER_SEMANTIC;
	}
	if (0 != iExit) {
		return 2;
	}

	/* Move all the requests for `once' type exclusion to the primary
	 * alias.  Move other excludes requests to primary alias too, and
	 * move inverted excludes to the primary name.
	 */
	for (pOR = pORRoot; nilOR != pOR; pOR = pOR->pORnext) {
		register char *pcScan;

		if (ISALIAS(pOR) && ISONLY(pOR)) {
			pOR->pORali->oattr |= OPT_ONLY;
			pOR->oattr &= ~OPT_ONLY;
		}
		if (!ISFORBID(pOR)) {
			continue;
		}
		if (ISALIAS(pOR) && ISFORBID(pOR->pORali)) {
			(void)strcpy(acAccum, pOR->pchforbid);
			(void)strcat(acAccum, pOR->pORali->pchforbid);
			pOR->pchforbid = malloc(strlen(struniq(acAccum))+1);
			(void)strcpy(pOR->pchforbid, acAccum);
		}
		for (pcScan = pOR->pchforbid; '\000' != *pcScan; ++pcScan) {
			pORAlias = newopt(*pcScan, & pORRoot, 0, 0);
			if (nilOR != pORAlias && ISALIAS(pORAlias)) {
				*pcScan = pORAlias->pORali->chname;
			}
		}
		(void)struniq(pOR->pchforbid);

		if (ISALIAS(pOR)) {
			pOR->pORali->pchforbid = pOR->pchforbid;
			pOR->pORali->oattr |= OPT_FORBID;
			pOR->pchforbid = (char *)0;
			pOR->oattr &= ~OPT_FORBID;
		}
	}

	/* some constructions imply an exclusion we have to construct
	 */
	pcAllows = acAExcl;
	pcBreaks = acBExcl;
	for (pOR = pORRoot; nilOR != pOR; pOR = pOR->pORnext) {
		if (nilOR != pOR->pORsact) {
			*pcAllows++ = pOR->chname;
		}
		if (ISBREAK(pOR)) {
			*pcBreaks++ = pOR->chname;
		}
	}
	*pcAllows = '\000';
	(void)struniq(acAExcl);
	*pcBreaks = '\000';
	(void)struniq(acBExcl);

	pORNumber = newopt('#', & pORRoot, 0, 0);
	pOREscape = newopt('+', & pORRoot, 0, 0);
	if (nilOR != pOREscape && nil == pOREscape->pchgen) {
		pOREscape->pchgen = "+";
	}

	retval = 0;
	for (pOR = pORDecl; nilOR != pOR; pOR = pOR->pORnext) {
		if ((int (*)())0 != pOR->pOTtype->pfifix) {
			retval |= (*pOR->pOTtype->pfifix)(pOR);
		}
		if (nil == pOR->pchuupdate) {
			pOR->pchuupdate = pOR->pOTtype->pchupdate;
		}

		if (ISAUGMENT(pOR)) {
			fprintf(stderr, "%s: pending augment on %s %s\n", progname, pOR->pOTtype->pchlabel, pOR->pchname);
			pOR->oattr &= ~OPT_AUGMENT;
			retval |= 2;
		}
		/* late conversion is more tricky than we make it look
		 * (EWLATE: error when late)
		 */
		if (ISLATE(pOR)) {
			pOR->oattr |= OPT_TRACK;
			if (IS_EWLATE(pOR->pOTtype)) {
				fprintf(stderr, "%s: %s is type %s which cannot be converted late\n", progname, usersees(pOR, nil), pOR->pOTtype->pchlabel);
				retval |= 2;
			}
			if (0 == retval && nil == pOR->pchkeep) {
				if (IS_LATEC(pOR->pOTtype))
					fprintf(stderr, "%s: %s requires two names because it is type %s\n", progname, usersees(pOR, nil), pOR->pOTtype->pchlabel);
				else
					fprintf(stderr, "%s: late %s requires two names\n", progname, usersees(pOR, nil));
				retval |= 2;
			}
		}
		if (! ISVISIBLE(pOR) || ! ISPPARAM(pOR)) {
			continue;
		}

		if (nil != pchVector && nil == pOR->pchverb && nil != pOR->pchdesc && '\000' != pOR->pchdesc[0]) {
			fprintf(stderr, "%s: variable `%s\' doesn't have a verbose help message for the verbose help list\n", progname, pOR->pchname);
			pOR->pchverb = pOR->pOTtype->pchhelp;
		}

		i = strlen(pOR->pchdesc);
		if (i > iWidth) {
			iWidth = i;
		}
		if (! ISVERIFY(pOR) || ISPPARAM(pOR))
			continue;
		retval |= 1;
		if (nil == pOR->pchverify) {
			pOR->pchverify = pOR->pOTtype->pchchk;
			pOR->pOTtype->tattr |= _CHK;
		}
		if (nil == pOR->pchverify) {
			fprintf(stderr, "%s: variable type for `%s\' has no default verified action\n", progname, pOR->pchname);
			pOR->oattr &= ~OPT_VERIFY;
			retval |= 2;
		}
	}
	for (pOR = pORRoot; nilOR != pOR; pOR = pOR->pORnext) {
		register OPTION *pORScan, *pORInside;
		register char *pcTail;
		register int iMax;

		if (isdigit(pOR->chname) && nilOR != pORNumber) {
			fprintf(stderr, "%s: both -number (-%s) and -%c cannot be supported in the same command line\n", progname, pORNumber->pchdesc ? pORNumber->pchdesc : pORNumber->pOTtype->pcharg, pOR->chname);
			retval |= 2;
		}
		if ('-' == pOR->chname) {
			/* we warn in the parser -- we fail here
			 */
			retval |= 2;
		}
		if (ISAUGMENT(pOR)) {
			fprintf(stderr, "%s: pending augment on %s %s\n", progname, pOR->pOTtype->pchlabel, usersees(pOR, nil));
			pOR->oattr &= ~OPT_AUGMENT;
			retval |= 2;
		}

		if (ISALIAS(pOR)) {
			continue;
		}
		if ((int (*)())0 != pOR->pOTtype->pfifix) {
			retval |= (*pOR->pOTtype->pfifix)(pOR);
		}
		if (nil == pOR->pchuupdate) {
			pOR->pchuupdate = pOR->pOTtype->pchupdate;
		}

		/* here we accumulate all the options excluded by
		 * getting one of these on the command line (pchforbid)
		 */

		/* other options that forbid us we need to add to
		 * our forbid's list or we confuse mkusage...
		 */
		if (ISFORBID(pOR)) {
			(void)struniq(pOR->pchforbid);
			(void)strcpy(acAccum, pOR->pchforbid);
		} else {
			acAccum[0] = '\000';
		}
		pcTail = acAccum + strlen(acAccum);
		for (pORScan = pORRoot; nilOR != pORScan; pORScan = pORScan->pORnext) {
			if (pORScan == pOR || !ISFORBID(pORScan)) {
				continue;
			}
			if (nil == strchr(pORScan->pchforbid, pOR->chname)) {
				continue;
			}
			if (nil != strchr(acAccum, pORScan->chname)) {
				continue;
			}
			*pcTail++ = pORScan->chname;
			*pcTail = '\000';
		}
		/* change `once' into `exclude <our-opts>', we know that
		 * this can't be an alias (see loop above).
		 * We used to put all the aliases in the exclude list
		 * for a `once' but this breaks the backend locks
		 * for us (it hides all the aliases)  this looks like
		 * a good solution.  We also have to manually remove
		 * such aliases from the user input exclude lists?
		 */
		if (ISONLY(pOR)) {
			*pcTail++ = pOR->chname;
			*pcTail = '\000';
			if (ISTOGGLE(pOR)) {
				fprintf(stderr, "%s: toggle `-%s\' can only be given once? (try boolean)\n", progname, usersees(pOR, nil));
			}
			pOR->oattr &= ~OPT_ONLY;
		}
		if (nilOR != pOR->pORsact) {
			(void)strcpy(pcTail, acAExcl);
			pcTail += strlen(pcTail);
		}
		if (ISBREAK(pOR)) {
			(void)strcpy(pcTail, acBExcl);
			pcTail += strlen(pcTail);
		}
		(void)struniq(acAccum);
		pcTail = (char *)0;

		/* check for semanitc errors in option nesting
		 * N.B. one usersees per printf! -- ksb
		 */
		for (pORInside = pOR->pORallow; nilOR != pORInside; pORInside = pORInside->pORallow) {
			if (pOR == pORInside) {
				fprintf(stderr, "%s: %s allows itself\n", progname, usersees(pOR, nil));
				retval |= 2;
				break;
			}
			if (ISVARIABLE(pORInside) || ISSACT(pORInside)) {
				fprintf(stderr, "%s: %s is allowed by ", progname, usersees(pOR, nil));
				fprintf(stderr, "a non-option (%s)\n", usersees(pORInside, nil));
				retval |= 2;
				break;
			}
			if (nil == strchr(acAccum, pORInside->chname)) {
				continue;
			}
			fprintf(stderr, "%s: %s excludes", progname, usersees(pOR, nil));
			fprintf(stderr, " %s which encloses it\n", usersees(pORInside, nil));
			retval |= 2;
			break;
		}

		/* after all that did we find any?
		 */
		if (!ISALIAS(pOR) && '\000' != acAccum[0]) {
			pOR->oattr |= OPT_FORBID;
			if ((char *)0 != strchr(acAccum, pOR->chname)) {
				pOR->oattr |= OPT_ONLY;
			}
			pOR->pchforbid = malloc(strlen(struniq(acAccum))+1);
			if (nil == pOR->pchforbid) {
				fprintf(stderr, acOutMem, progname);
				exit(MER_LIMIT);
			}
			(void)strcpy(pOR->pchforbid, acAccum);
		} else {
			pOR->oattr &= ~(OPT_FORBID|OPT_ONLY);
		}

		/* late conversion is more tricky than we make it look
		 * (EWLATE: error when late)
		 */
		if (ISLATE(pOR)) {
			pOR->oattr |= OPT_TRACK;
			if (IS_EWLATE(pOR->pOTtype)) {
				fprintf(stderr, "%s: %s is type %s which cannot be converted late\n", progname, usersees(pOR, nil), pOR->pOTtype->pchlabel);
				retval |= 2;
			}
			if (0 == retval && nil == pOR->pchkeep) {
				if (IS_LATEC(pOR->pOTtype))
					fprintf(stderr, "%s: %s requires two names because it is type %s\n", progname, usersees(pOR, nil), pOR->pOTtype->pchlabel);
				else
					fprintf(stderr, "%s: late %s requires two names\n", progname, usersees(pOR, nil));
				retval |= 2;
			}
		}
		if (ISENABLE(pOR) && (ISBREAK(pOR) || ISABORT(pOR))) {
			fprintf(stderr, "%s: %s ends option processing and enables options (which therefore cannot be reached)\n", progname, usersees(pOR, nil));
		}
		if (ISALTUSAGE(pOR) && (ISENDS(pOR) || ISABORT(pOR))) {
			fprintf(stderr, "%s: %s ends a command line and enables an alternate usage (which therefore cannot be reached)\n", progname, usersees(pOR, nil));
		}
		if (! ISVISIBLE(pOR)) {
			continue;
		}

		if (nil != pchVector && nil == pOR->pchverb) {
			fprintf(stderr, "%s: %s doesn't have a verbose help message for the usage function\n", progname, usersees(pOR, nil));
			pOR->pchverb = pOR->pOTtype->pchhelp;
		}

		iMax = compwidth(pOR);
		if (iMax > iWidth) {
			iWidth = iMax;
		}

		if (! ISVERIFY(pOR)) {
			continue;
		}
		retval |= 1;
		if (nil == pOR->pchverify) {
			pOR->pchverify = pOR->pOTtype->pchchk;
			pOR->pOTtype->tattr |= _CHK;
		}
		if (nil == pOR->pchverify) {
			fprintf(stderr, "%s: %s has no default verified action for its type\n", progname, usersees(pOR, nil));
			pOR->oattr &= ~OPT_VERIFY;
			retval |= 2;
		}
	}

	/* make sure number and escape are not aliased for boolean/toggle
	 * or action conversions -- because the data would be lost and
	 * in the option case optarg (%a/%N) won't be valid (at all)
	 */
	if (nilOR != pORNumber && ISALIAS(pORNumber) && sbBInit == pORNumber->pORali->pOTtype->pchdef) {
		fprintf(stderr, "%s: number `%s' is an alias for a %s which has no parameter (type %s)\n", progname, usersees(pORNumber, acAccum), usersees(pORNumber->pORali, nil), pORNumber->pORali->pOTtype->pchlabel);
		retval |= 2;
	}
	if (nilOR != pOREscape && ISALIAS(pOREscape) && sbBInit == pOREscape->pORali->pOTtype->pchdef) {
		fprintf(stderr, "%s: escape `%s' is an alias for a %s which has no parameter (type %s)\n", progname, usersees(pOREscape, acAccum), usersees(pOREscape->pORali, nil), pOREscape->pORali->pOTtype->pchlabel);
	}

	/* Check any routines called from other buffers routines...
	 */
	for (pOR = pORDecl; nilOR != pOR; pOR = pOR->pORnext) {
		if ((ROUTINE *)0 != pOR->pRG) {
			retval |= CheckRoutine(pOR);
		}
	}

	/* reto-fit control logic
	 */
	for (pOR = pORRoot; nilOR != pOR; pOR = pOR->pORnext) {
		if (nilOR != pOR->pORsact) {
			retval |= fixcontrol(& pOR->pORsact);
		}
		if ((ROUTINE *)0 != pOR->pRG) {
			retval |= CheckRoutine(pOR);
		}
	}
	retval |= fixcontrol(& pORActn);

	if (0 != (2 & retval)) {
		exit(MER_SEMANTIC);
	}

	/* Now adjust iWidth if any PPARAM is wider than
	 * the min we found in iWidth.
	 */
	for (pOR = pORDecl; nilOR != pOR; pOR = pOR->pORnext) {
		register int iMin;
		if (ISPPARAM(pOR) && (iWidth < (iMin = strlen(pOR->pchdesc)))) {
			iWidth = iMin;
		}
	}

	/* Check all basenames:
	 * Count them, sort them, look for dups.
	 * To make key basename work we have to insert the
	 * key basename into the basename list (if it is not there)
	 * with an option insert value of "".  Else you can't get
	 * to the "default" -h help and such.		(ksb)
	 */
	iFound = 0;
	for (i = 0; (char **)0 != ppcBases && (char *)0 != ppcBases[i]; ++i) {
		if ((char *)0 == pcKeyArg)
			continue;
		if (0 == strcmp(ppcBases[i], pcKeyArg))
			iFound = 1;
	}
	if ((char *)0 != pcKeyArg && 0 == iFound) {
		AddBasePair(pcKeyArg, "");
		++i;
	}
	if ((char **)0 == ppcBases) {
		/* no basename given -- should we warn if manual page
		 * generation is on?
		 */
		return retval;
	}

	/* all basenames should be unique, else we have a problem
	 * sort them, look for adjacents the same
	 */
	(void)qsort((char *)ppcBases, i, sizeof(char *), QCmp);
	for (iFound = 1; iFound < i; ++iFound) {
		if (0 != strcmp(ppcBases[iFound], ppcBases[iFound-1])) {
			continue;
		}
		fprintf(stderr, "%s: multiple basenames declrataions for \"%s\"\n", progname, ppcBases[iFound]);
		retval |= 2;
	}
	if (0 != (retval & 2)) {
		exit(MER_SEMANTIC);
	}

	/* options forced in with basename should:
	 *  exists, (fatal)
	 *  have any parameter they require (fatal)
	 * and non-options should not be forced on the user (warning?)
	 */
	for (i = 0; (char *)0 != ppcBases[i]; ++i) {
		auto char **fargv;
		auto int fargc;

		fargc = 1;
		fargv = (char **)malloc(2 * sizeof(char *));
		if ((char **)0 == fargv) {
			fprintf(stderr, acOutMem, progname);
			exit(MER_LIMIT);
		}
		fargv[0] = ppcBases[i];
		fargv[1] = (char *)0;
		f_envopt(ppcBases[i]+1+strlen(ppcBases[i]), &fargc, &fargv);

		retval |= chk_getopt(ppcBases[i], fargc, fargv, pORNumber, pOREscape);
		free((char *)fargv);
	}
	return retval;
}


/* output the only code							(ksb)
 */
void
mkonly(fp)
FILE *fp;
{
	register OPTION *pOR, *pORLast, *pORTemp;
	register char *pch;
	auto int chFirst, chLast, fError;
	auto OPTION *pORPlus, *pORNumber;
	auto int iNameWidth;	/* gcc -Wall is wrong here too -- ksb */

	pORLast = nilOR;
	pORPlus = pORNumber = nilOR;
	fError = 0;
	for (pOR = pORRoot; nilOR != pOR; pOR = pOR->pORnext) {
		if (! ISFORBID(pOR) && ! ISSTRICT(pOR)) {
			continue;
		}
		if (nilOR == pORLast || pORLast->chname < pOR->chname) {
			pORLast = pOR;
		}
		if ((char *)0 == pOR->pchforbid) {
			continue;
		}
		switch (pOR->chname) {
		case '+':	/* +word */
			pORPlus = pOR;
			break;
		case '#':	/* -number */
			pORNumber = pOR;
			break;
		default:
			break;
		}
		for (pch = pOR->pchforbid; '\000' != *pch; ++pch) {
			if (pOR->chname == *pch) {
				continue;
			}
			pORTemp = newopt(*pch, & pORRoot, 0, 0);
			if (nilOR == pORTemp) {
				fprintf(stderr, "%s: forbidden list for `-%c\' contains nonexistent option `-%c\'\n", progname, pOR->chname, *pch);
				++fError;
			} else {
				pORTemp->oattr |= OPT_STRICT;
			}
		}
	}

	if (0 != fError) {
		exit(fError);
	}

	if (nilOR == pORLast) {
		return;
	}

	chFirst = pORRoot->chname;
	chLast = pORLast->chname;

	if (nilOR != pORPlus || nilOR != pORNumber) {
		register int iP = 0, iN = 0;
		/* option `-X' = strlen("option `-") + 1 + 1 + 1 */
		iNameWidth = (10|3)+1;
		if (nilOR != pORPlus) {
			/* `+word'  = 1 + strlen(+) + strlen(word) + 1 + 1 */
			iP = ((1 + strlen(pORPlus->pchgen) + strlen(pORPlus->pchdesc) + 1)|3)+1;
		}
		if (nilOR != pORNumber) {
			/* `-word'  = 2 + strlen(word) + 1 + 1*/
			iN = ((2 + strlen(pORNumber->pchdesc) + 1)|3) + 1;
		}
		if (iNameWidth < iP)
			iNameWidth = iP;
		if (iNameWidth < iN)
			iNameWidth = iN;
		if (fAnsi) {
			fprintf(fp, "static void u_nameof(char *pcBuf, char ch)\n");
		} else {
			fprintf(fp, "static void\nu_nameof(pcBuf, ch)\nchar *pcBuf, ch;\n");
		}
		fprintf(fp, "{\n\t");

		/* we know one of these if's are true, maybe both
		 */
		if (nilOR != pORPlus) {
			fprintf(fp, "if ('+' == ch) {\n\t\t(void)strcpy(pcBuf, \"`%s%s\\'\");\n\t", pORPlus->pchgen, pORPlus->pchdesc);
			if (nilOR != pORNumber) {
				fprintf(fp, "} else ");
			}
		}
		if (nilOR != pORNumber) {
			fprintf(fp, "if ('#' == ch) {\n\t\t(void)strcpy(pcBuf, \"`-%s\\'\");\n\t", pORNumber->pchdesc);
		}
		fprintf(fp, "} else {\n\t\t(void)strcpy(pcBuf, \"option `-K\\'\");\n");
		fprintf(fp, "\t\tpcBuf[9] = ch;\n\t}\n}\n");
	}
	if (fAnsi) {
		fprintf(fp, "static void u_chkonly(char chSlot, char chOpt, char *pcList)\n");
	} else {
		fprintf(fp, "static void\nu_chkonly(chSlot, chOpt, pcList)\nint chSlot, chOpt;\nchar *pcList;\n");
	}
	fprintf(fp, "{\n\tregister int chWas;\n");
	fprintf(fp, "\tstatic int sbiOnly[\'%c\'-\'%c\'+1];\n", chLast, chFirst);
	if (nilOR != pORPlus || nilOR != pORNumber) {
		fprintf(fp, "\tauto char acOpt[%d], acWas[%d];\n", iNameWidth, iNameWidth);
	}

	fprintf(fp, "\n\tchWas = sbiOnly[chSlot-\'%c\'];\n", chFirst);
	if (nilOR != pORPlus || nilOR != pORNumber) {
		fprintf(fp, "\tu_nameof(acWas, chWas);\n");
	}
	fprintf(fp, "\tif (chOpt == chWas) {\n");
	if (nilOR != pORPlus || nilOR != pORNumber) {
		fprintf(fp, "\t\tfprintf(stderr, \"%%s: -%%s cannot be given more than once\\n\", %s, acWas);\n", pchProgName);
	} else {
		fprintf(fp, "\t\tfprintf(stderr, \"%%s: option `-%%c\\\' cannot be given more than once\\n\", %s, chWas);\n", pchProgName);
	}
	fprintf(fp, "\t\texit(1);\n");
	fprintf(fp, "\t} else if (0 != chWas) {\n");
	if (nilOR != pORPlus || nilOR != pORNumber) {
		fprintf(fp, "\t\tu_nameof(acOpt, chOpt);\n");
		fprintf(fp, "\t\tfprintf(stderr, \"%%s: %%s forbidden by %%s\\n\", %s, acOpt, acWas);\n", pchProgName);
	} else {
		fprintf(fp, "\t\tfprintf(stderr, \"%%s: option `-%%c\\\' forbidden by `-%%c\\\'\\n\", %s, chOpt, chWas);\n", pchProgName);
	}
	fprintf(fp, "\t\texit(1);\n\t}\n");
	fprintf(fp, "\tfor (/*parameter*/; \'\\000\' !=  *pcList; ++pcList) {\n");
	fprintf(fp, "\t\tsbiOnly[*pcList-\'%c\'] = chOpt;\n\t}\n}\n\n", chFirst);
}


/* output the code to make sure a string is a good integer		(ksb)
 */
static void
mkfint(fp)
register FILE *fp;
{
	if (fAnsi) {
		fprintf(fp, "static void u_chkint(char *param, char *pch)\n");
	} else {
		fprintf(fp, "static void\nu_chkint(param, pch)\nchar *param, *pch;\n");
	}
	fprintf(fp, "{\n\tregister int c;\n\n\twhile (isspace(*pch))\n");
	fprintf(fp, "\t\t++pch;\n\tc = *pch;\n\tif (\'+\' == c || \'-\' == c)\n");
	fprintf(fp, "\t\t++pch;\n\twhile (isdigit(*pch))\n\t\t++pch;\n");
	fprintf(fp, "\tif (\'\\000\' != *pch) {\n\t\tfprintf(stderr, sbData, %s, \"integer\", param)", pchProgName);
	fprintf(fp, ";\n\t\texit(1);\n\t}\n}\n\n");
}

/* output the function that checks string length			(ksb)
 */
static void
mkfstr(fp)
register FILE *fp;
{
	if (fAnsi) {
		fprintf(fp, "static void u_chkstr(char *param, char *pch, int len)\n");
	} else {
		fprintf(fp, "static void\nu_chkstr(param, pch, len)\nchar *param, *pch;\nint len;\n");
	}
	fprintf(fp, "{\n\tif (strlen(pch) > len) {\n");
	fprintf(fp, "\t\tfprintf(stderr, \"%%s: string too long for %%s\\n\", %s, param);", pchProgName);
	fprintf(fp, "\n\t\texit(1);\n\t}\n}\n\n");
}

/* output the code to make sure a string is a good float number		(ksb)
 */
static void
mkfdbl(fp)
register FILE *fp;
{
	if (fAnsi) {
		fprintf(fp, "static void u_chkdbl(char *param, char *pch)\n");
	} else {
		fprintf(fp, "static void\nu_chkdbl(param, pch)\nregister char *param, *pch;\n");
	}
	fprintf(fp, "{\n\twhile (isspace(*pch))\n\t\t++pch;\n\tif (\'+\' == *pch || \'-\' == *pch)\n");
	fprintf(fp, "\t\t++pch;\n\twhile (isdigit(*pch))\n\t\t++pch;\n");
	fprintf(fp, "\tif (\'.\' == *pch)\n\t\t++pch;\n\twhile (isdigit(*pch))\n");
	fprintf(fp, "\t\t++pch;\n\tif (\'e\' == *pch || \'E\' == *pch)\n");
	fprintf(fp, "\t\t++pch;\n\tif (\'+\' == *pch || \'-\' == *pch)\n");
	fprintf(fp, "\t\t++pch;\n\twhile (isdigit(*pch))\n\t\t++pch;\n");
	fprintf(fp, "\tif (\'\\000\' != *pch) {\n\t\tfprintf(stderr, sbData, %s, \"float\", param);\n", pchProgName);
	fprintf(fp, "\t\texit(1);\n\t}\n}\n\n");
}

/* output the code that updates an accum value				(ksb)
 */
static void
mkaccum(fp)
register FILE *fp;
{
	fprintf(fp, "/*\n * Accumulate a string, for string options that \"append with a sep\"");
	fprintf(fp, "\n * note: arg must start out as either \"(char *)0\" or a malloc'd strin");
	fprintf(fp, "g\n */\nstatic char *\noptaccum(pchOld, pchArg, pchSep)\n");
	fprintf(fp, "char *pchOld, *pchArg, *pchSep;\n{\n");
	fprintf(fp, "\tregister unsigned len;\n\tregister char *pchNew;\n");
	fprintf(fp, "\tstatic char acOutMem[] =  \"%%s: out of memory\\n\";\n");
	fprintf(fp, "\n\t/* Do not add null strings\n\t */\n");
	fprintf(fp, "\tif ((char *)0 == pchArg || 0 == (len = strlen(pchArg))) {\n");
	fprintf(fp, "\t\treturn pchOld;\n\t}\n\n\tif ((char *)0 == pchOld) {\n");
	fprintf(fp, "\t\tpchNew = malloc(len+1);\n\t\tif ((char *)0 == pchNew) {\n");
	fprintf(fp, "\t\t\tfprintf(stderr, acOutMem, %s);\n", pchProgName);
	fprintf(fp, "\t\t\texit(1);\n\t\t}\n\t\tpchNew[0] = '\\000';\n");
	fprintf(fp, "\t} else {\n\t\tlen += strlen(pchOld)+strlen(pchSep)+1;\n");
	fprintf(fp, "\t\tif ((char *)0 == (pchNew = realloc(pchOld, len))) {\n");
	fprintf(fp, "\t\t\tfprintf(stderr, acOutMem, %s);\n", pchProgName);
	fprintf(fp, "\t\t\texit(1);\n\t\t}\n\t\t(void)strcat(pchNew, pchSep);\n");
	fprintf(fp, "\t}\n\tpchOld = strcat(pchNew, pchArg);\n");
	fprintf(fp, "\treturn pchOld;\n}\n");
}


/* output the conversion routine for letter				(ksb)
 */
static void
mkfletters(fp)
register FILE *fp;
{

	fprintf(fp, "/* convert text to control chars, we take `cat -v' style\t\t(ksb)\n");
	fprintf(fp, " * \t[\\]octal\n *\t^X (or ^x)\t\tcontro-x\n");
	fprintf(fp, " *\tM-x\t\t\tx plus 8th bit\n *\tc\t\t\ta plain character\n");
	fprintf(fp, " * return -1 -> no chars to convert\n * return -2 -> unknown control code");
	fprintf(fp, "\n * return -3 -> trailing junk on letter specification\n");
	fprintf(fp, " */\nstatic int\ncvtletter(pcScan)\nchar *pcScan;\n");
	fprintf(fp, "{\n\tregister int cvt, n, i;\n\n\tif ('\\\\' == pcScan[0]) {\n");
	fprintf(fp, "\t\tswitch (*++pcScan) {\n\t\tcase 'a':\n");
	fprintf(fp, "\t\t\tcvt = '\\007';\n\t\t\t++pcScan;\n");
	fprintf(fp, "\t\t\tbreak;\n\t\tcase '\\n':\t/* why would this happen? */\n");
	fprintf(fp, "\t\tcase 'n':\t/* newline */\n\t\t\tcvt = '\\n';\n");
	fprintf(fp, "\t\t\t++pcScan;\n\t\t\tbreak;\n\t\tcase 't':\n");
	fprintf(fp, "\t\t\tcvt = '\\t';\n\t\t\t++pcScan;\n\t\t\tbreak;\n");
	fprintf(fp, "\t\tcase 'b':\n\t\t\tcvt = '\\b';\n\t\t\t++pcScan;\n");
	fprintf(fp, "\t\t\tbreak;\n\t\tcase 'r':\n\t\t\tcvt = '\\r';\n");
	fprintf(fp, "\t\t\t++pcScan;\n\t\t\tbreak;\n\t\tcase 'f':\n");
	fprintf(fp, "\t\t\tcvt = '\\f';\n\t\t\t++pcScan;\n\t\t\tbreak;\n");
	fprintf(fp, "\t\tcase 'v':\n\t\t\tcvt = '\\013';\n\t\t\t++pcScan;\n");
	fprintf(fp, "\t\t\tbreak;\n\t\tcase '\\\\':\n\t\t\t++pcScan;\n");
	fprintf(fp, "\t\tcase '\\000':\n\t\t\tcvt = '\\\\';\n");
	fprintf(fp, "\t\t\tbreak;\n\n\t\tcase '0': case '1': case '2': case '3':\n");
	fprintf(fp, "\t\tcase '4': case '5': case '6': case '7':\n");
	fprintf(fp, "\t\t\tgoto have_num;\n\t\tcase '8': case '9':\n");
	fprintf(fp, "\t\t\t/* 8 & 9 are bogus octals,\n\t\t\t * cc makes them literals\n");
	fprintf(fp, "\t\t\t */\n\t\t\t/*fallthrough*/\n\t\tdefault:\n");
	fprintf(fp, "\t\t\tcvt = *pcScan++;\n\t\t\tbreak;\n\t\t}\n");
	fprintf(fp, "\t} else if ('0' <= *pcScan && *pcScan <= '7' && '\\000' != pcScan[1]) {\n");
	fprintf(fp, "have_num:\n\t\tcvt = *pcScan++ - '0';\n");
	fprintf(fp, "\t\tfor (i = 0; i < 2; i++) {\n\t\t\tif (! isdigit(*pcScan)) {\n");
	fprintf(fp, "\t\t\t\tbreak;\n\t\t\t}\n\t\t\tcvt <<= 3;\n");
	fprintf(fp, "\t\t\tcvt += *pcScan++ - '0';\n\t\t}\n\t} else {\n");
	fprintf(fp, "\t\tif ('M' == pcScan[0] && '-' == pcScan[1] && '\\000' != pcScan[2]) {\n");
	fprintf(fp, "\t\t\tcvt = 0x80;\n\t\t\tpcScan += 2;\n");
	fprintf(fp, "\t\t} else {\n\t\t\tcvt = 0;\n\t\t}\n\n");
	fprintf(fp, "\t\tif ('\\000' == *pcScan) {\n\t\t\treturn -1;\n");
	fprintf(fp, "\t\t}\n\n\t\tif ('^' == (n = *pcScan++) && '\\000' != *pcScan) {\n");
	fprintf(fp, "\t\t\tn = *pcScan++;\n\t\t\tif (islower(n)) {\n");
	fprintf(fp, "\t\t\t\tn = toupper(n);\n\t\t\t}\n\t\t\tif ('@' <= n &&  n <= '_') {\n");
	fprintf(fp, "\t\t\t\tcvt |= n - '@';\n\t\t\t} else if ('?' == n) {\n");
	fprintf(fp, "\t\t\t\tcvt |= '\\177';\n\t\t\t} else {\n");
	fprintf(fp, "\t\t\t\treturn -2;\n\t\t\t}\n\t\t} else {\n");
	fprintf(fp, "\t\t\tcvt |= n;\n\t\t}\n\t}\n\t\n\tif ('\\000' != *pcScan) {\n");
	fprintf(fp, "\t\treturn -3;\n\t}\n\treturn cvt;\n}\n");
}

/* output more checking code for letters				(ksb)
 */
static void
mkfckletters(fp)
register FILE *fp;
{
	fprintf(fp, "/* output a botch if the letter spec is bogus\t\t\t\t(ksb)\n */\n");
	if (fAnsi) {
		fprintf(fp, "int u_chkletter(char *pchParam, char *pchData)\n");
	} else {
		fprintf(fp, "int\nu_chkletter(pchParam, pchData)\nchar *pchParam, *pchData;\n");
	}
	fprintf(fp, "{\n\tregister int r;\n");
	fprintf(fp, "\n\tswitch (r = cvtletter(pchData)) {\n");
	fprintf(fp, "\tdefault:\n\t\treturn r;\n\tcase -1:\n");
	fprintf(fp, "\t\tfprintf(stderr, \"%%s: %%s: no characters to convert\\n\", %s, ", pchProgName);
	fprintf(fp, "pchParam);\n\t\tbreak;\n\tcase -2:\n\t\tfprintf(stderr, \"%%s: %%s: unkno");
	fprintf(fp, "wn control code in `%%s\\'\\n\", %s, pchParam, pchData);\n", pchProgName);
	fprintf(fp, "\t\tbreak;\n\tcase -3:\n\t\tfprintf(stderr, \"%%s: %%s: trailing junk on ");
	fprintf(fp, "letter specification `%%s\\'\\n\", %s, pchParam, pchData);\n", pchProgName);
	fprintf(fp, "\t\tbreak;\n\t}\n\texit(-r);\n}\n\n");
}

/* output check code for fomal files					(ksb)
 * This might be a hook on an Amiga, or DOS machines
 * (where we have limits like 8 dot 3, or some such).
 */
static void
mkfckfiles(fp)
register FILE *fp;
{
	fprintf(fp, "/* output a botch if the filename given is not in the right format\t(ksb)\n */\n");
	if (fAnsi) {
		fprintf(fp, "int u_chkfile(char *pchParam, char *pcFile, char *pcHow)\n");
	} else {
		fprintf(fp, "int\nu_chkfile(pchParam, pcFile, pcHow)\nchar *pchParam, *pcFile, *pcHow;\n");
	}
	fprintf(fp, "{\n\t/* all UNIX file names are OK -- or fopen will tell us */\n");
	fprintf(fp, "\treturn;\n}\n");
}

/* output the code for the check functions				(ksb)
 * get all the types that have special code segments and output those
 * segments.
 */
void
mkchks(fp)
FILE *fp;
{
	auto OPTTYPE *pOTInt, *pOTUnsigned, *pOTLong;
	auto OPTTYPE *pOTDouble, *pOTString, *pOTNumber;
	auto OPTTYPE *pOTAccum, *pOTLetters;
	auto OPTTYPE *pOTFiles, *pOTFds;

	pOTInt = CvtType('i');
	pOTUnsigned = CvtType('u');
	pOTLong = CvtType('l');
	pOTDouble = CvtType('d');
	pOTString = CvtType('s');
	pOTNumber = CvtType('#');
	pOTAccum =  CvtType('+');
	pOTLetters = CvtType('c');
	pOTFiles = CvtType('F');
	pOTFds = CvtType('D');

	if (IS_CHK(pOTInt) || IS_CHK(pOTLong) || IS_CHK(pOTUnsigned) || IS_CHK(pOTDouble) || IS_CHK(pOTNumber)) {
		fprintf(fp, "static char sbData[] = \"%%s: illegal data for %%s %%s\\n\";\n\n");
	}

	if (IS_CHK(pOTInt) || IS_CHK(pOTLong) || IS_CHK(pOTUnsigned) || IS_CHK(pOTNumber)) {
		mkfint(fp);
	}
	if (IS_CHK(pOTDouble)) {
		mkfdbl(fp);
	}
	if (IS_CHK(pOTString) ) {
		mkfstr(fp);
	}
	if (IS_USED(pOTAccum)) {
		mkaccum(fp);
	}
	if (IS_USED(pOTLetters)) {
		mkfletters(fp);
	}
	if (IS_CHK(pOTLetters)) {
		mkfckletters(fp);
	}
	if (IS_CHK(pOTFiles) || IS_CHK(pOTFds)) {
		mkfckfiles(fp);
	}
}
