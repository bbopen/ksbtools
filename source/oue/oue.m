#!mkcmd
# Only Uniq Elements (oue) - from the xapply source README		(ksb)
# enhanced to make it a real filter.  The locked db is nice.
# mkcmd version
# $Compile: %b -mMkcmd %f && mk -m%m oue.c
comment " * %kCompile: $%{cc-gcc%} -g -Wall -I/usr/local/include -o %%F %%f -L/usr/local/lib -lgdbm %k%k"
from '<unistd.h>'
from '<stdlib.h>'
from '<string.h>'
from '<limits.h>'
from '<sysexits.h>'
from '<sys/types.h>'
from '<sys/param.h>'
from '<sys/types.h>'
from '<sys/stat.h>'
from '<gdbm.h>'
from '"machine.h"'

require "std_help.m" "std_version.m" "std_filter.m"
require "util_tmp.m"
require "dicer.c"

%i
static char rcsid[] =
	"$Id: oue.m,v 2.31 2012/05/03 17:30:55 ksb Exp $";
%%
augment action 'V' {
	user "OueVersion();"
}

action 'H' {
	named "QuickRef"
	update "%n(stdout);"
	aborts "exit(EX_OK);"
	help "output only on-line expander markup help"
}

# Input options
boolean 'z' {
	named "fZeroEOL"
	init "0"
	help "streams are NUL terminated, as find-print0 output"
}
char* 'p' {
	named "pcPad"
	param "pad"
	help "complete short elements with this token"
}
number {
	named "iDeck"
	param "span"
	init "1"
	help "number of input lines read to form an element (default %qi)"
}
type "bytes" 'b' {
	named "wBufSize" "pcBufSize"
	param "length"
	help "specify a bigger dicer buffer size (default %qi)"
	init dynamic '"10k"'
}

# Record processing features
boolean 'c' {
	named "fCounting"
	help "process keys for the count, rather than uniqueness"
}
boolean 'd' {
	named "fRepeated"
	help "report (reject) keys repeated more than once (like uniq)"
}
integer 'M' {
	hidden named "wMany"
	param "many"
	init '2'
	help "change the duplicate threshold, default %qi"
}
char* 'e' {
	named "pcEveryE"
	param "every"
	help "update the %%e accumulator for every key instance"
}
toggle 'v' {
	named "fInvert"
	help "invert the filter to include (or reject) elements from prev"
}
boolean 'l' {
	named "fLast"
	help "use the last instance of a key for the memory and output"
}
boolean 's' {
	named "fStable"
	help "always output elements in the order discovered (reduced performance)"
}
boolean 'S' {
	named "fSquelch"
	help "squelch sequential duplicates (treat as 1 occurrence)"
}

# Memory and dicer options
letter 'a' {
	named "cEscape"
	init "'%'"
	param "c"
	help "change the dicer escape character, default %qi"
}
boolean 'N' {
	named "fLockFree"
	init '1'
	help "never lock either dbm file for updates"
}
char* 'D' {
	named "pcState"
	track "fGaveD"
	param "state"
	help "save keys and memory values in state dbm between runs"
}
char* 'I' {
	named "pcPrevState"
	param "prev"
	help "suppress (include) elements from a previous state dbm"
}
# These only work under -I, but allow them for backwards compatibility
# before (or without) the -I.  People used to type "-iI local.db" a lot.
boolean 'i' {
	named "fInclude"
	help "begin output with replay output from elements recovered from prev"
}
char* 'B' {
	named "pcIncludeE"
	param "replay"
	user "%rin = 1;"
	help "output replay dicer for each prev entry, default to report"
}
char* 'f' {
	named "pcFirstE"
	param "first"
	help "provide a first value for the every accumulator (%%e)"
}
char* 'x' {
	named "pcExtract"
	param "extract"
	help "dicer expression to extract count from previous, else leading integer "
}
# end -I suboptions

char* 'k' {
	named "pcKeyE"
	param "key"
	help "build the element key from an element via the dicer"
}
char* 'r' {
	named "pcRememberE"
	param "memory"
	help "record expanded memory for each key in state file"
}
char* 'R' {
	named "pcReportE"
	param "report"
	help "output dicer for key/memory pairs, defaults to key only"
}
after {
	named "Starter"
	update "%n();"
}
exit {
	named "Cleanup"
	update "%n();"
	aborts "exit(EX_OK);"
}

%i
static const char
	acMyTemp[] = "oueXXXXXX";

/* Under -c we need to save the whole context of the _first_ match
 * for the line, this is the node that does it. -- ksb
 */
typedef struct CXnode {
	unsigned long ubytes, ucount, uorig;	/* meta info	*/
	unsigned long ulineno, uuniq, ufileno;	/* about location */
	char *pcfrom;				/* input file name */
	unsigned wis_dup;
	char acdeck[0];	/* really ubytes dynamically allocated */
} CONTEXT;
static CONTEXT CXGlobal = {
	.wis_dup = 0,
	.ubytes = 0,
	.ucount = 0,
	.uorig = 0,
	.ulineno = 0,
	.uuniq = 0,
	.ufileno = 0,
	.pcfrom = "ksb",
};

static datum GDMemory, GDInv, GDEvery, GDOldEvery;
static GDBM_FILE
	dbfPrev = (GDBM_FILE)0,
	dbfState = (GDBM_FILE)0,
	dbfCounts = (GDBM_FILE)0,
	dbfEvery = (GDBM_FILE)0;
static char
	acDefMem[] = ".",
	acEmpty[] = "",
	acEveryDb[MAXPATHLEN+4],
	acTmpDb[MAXPATHLEN+4],
	acCountDb[MAXPATHLEN+4],
	acStableList[MAXPATHLEN+4];
static FILE
	*fpStable = (FILE *)0;
%%

%c
/* Fix up any options that were not specified on the command-line	(ksb)
 */
static void
ReconOpts()
{
	static char acDefReport[16];	/* "%t %k" */
	static char acDefRemember[8];	/* "%t" or "%c"*/
	static char acDefExtract[8];	/* "%v" */
	static char acDefEvery[8];	/* "%p" */
	static char acDefKey[8];	/* "%1" */

	acTmpDb[0] = acCountDb[0] = acEveryDb[0] = acStableList[0] = '\000';
	if (iDeck <= 0) {
		iDeck = 1;
	}
	if ((char *)0 == pcKeyE) {
		snprintf(acDefKey, sizeof(acDefKey), "%c%c", cEscape, (1 == iDeck) ? '1' : '@');
		pcKeyE = acDefKey;
	}
	if ((char *)0 != pcFirstE && (char *)0 == pcEveryE) {
		snprintf(acDefEvery, sizeof(acDefEvery), "%cp", cEscape);
		pcEveryE = acDefEvery;
	}
	if ((char *)0 != pcEveryE && (char *)0 == pcFirstE) {
		pcFirstE = (char *)acEmpty;
	}
	if ((char *)0 == pcReportE) {
		if (fCounting)
			snprintf(acDefReport, sizeof(acDefReport), "%ct %ck", cEscape, cEscape);
		else
			snprintf(acDefReport, sizeof(acDefReport), "%ck", cEscape);
		pcReportE = acDefReport;
	}
	if ((char *)0 == pcIncludeE) {
		pcIncludeE = pcReportE;
	}
	if ('\000' == pcIncludeE[0]) {
		fInclude = 0;
	}
	/* under -dv we have to delay output to check the total 1 == count
	 */
	if (fInvert && fRepeated) {
		fCounting = 1;
	}
	if ((char *)0 == pcRememberE) {
		snprintf(acDefRemember, sizeof(acDefRemember), "%c%c", cEscape, ((char *)0 != pcPrevState ? 't' : 'c'));
		pcRememberE = fCounting ? acDefRemember : acDefMem;
	}
	/* under -l or -ds we delay output but don't change the default memory
	 */
	if (fStable && !(fRepeated || fLast || fCounting)) {
		fStable = 0;
	}
	if (fLast || (fStable && fRepeated)) {
		fCounting = 1;
	}
	if (0 == wMany) {
		fprintf(stderr, "%s: -M: a duplicate threshold of %d can never find any output\n", progname, wMany);
		exit(EX_DATAERR);
	}
	if ((char *)0 == pcPad) {
		pcPad = acEmpty;
	}
	if ((char *)0 == pcExtract) {
		snprintf(acDefExtract, sizeof(acDefExtract), "%cv", cEscape);
		pcExtract = (char *)0 == pcPrevState ? acEmpty : acDefExtract;
	}
	if ((char *)0 == pcState) {
		fLockFree = 1;
	}
	/* We're not going to hurt ourself with a 1K buffer, but 0b is Bad.
	 */
	if (wBufSize < 1024) {
		wBufSize = 1024;
	}
	wBufSize = ((wBufSize-1)|15)+1;
	GDMemory.dptr = GDInv.dptr = GDEvery.dptr = GDOldEvery.dptr = (char *)0;
	GDMemory.dsize = GDInv.dsize = GDEvery.dsize = GDOldEvery.dsize = 0;
}

/* tell the Driver what they need to know				(ksb)
 */
static void
OueVersion()
{
	ReconOpts();
	printf("%s:  %s\n", progname, DicerVersion);
	printf("%s:  %s\n", progname, gdbm_version);
	printf("%s: key dicer: %s\n", progname, pcKeyE);
	printf("%s: memory dicer: %s\n", progname, pcRememberE);
	printf("%s: report dicer: %s\n", progname, pcReportE);
	printf("%s: dicer escape %c, buffer length %lu\n", progname, cEscape, (unsigned long)wBufSize);
	printf("%s: safe file template: %s\n", progname, acMyTemp);
}

/* On-line help for percent markup, I know people like it.		(ksb)
 * N.B. expander %i is no longer documented, neither is %M (duplicate min).
 */
static void
QuickRef(FILE *fp)
{
	ReconOpts();
	fprintf(fp, "Basic markup:\n");
	fprintf(fp, " %c%c  a literal \"%c\"\n", cEscape, cEscape, cEscape);
	fprintf(fp, " %c0  the empty string (as in xapply)\n", cEscape);
	fprintf(fp, " %c1  the first line from this element\n", cEscape);
	fprintf(fp, " %c2  the 2nd input line from this element, as span allows\n", cEscape);
	fprintf(fp, " %c$  the last line in this element\n", cEscape);
	fprintf(fp, " %c@  the lines in the element joined with spaces\n", cEscape);
	fprintf(fp, " %c*  the lines in the element joined with newlines\n", cEscape);
	fprintf(fp, " %cf  the element's source file, %cF current source file\n", cEscape, cEscape);
	fprintf(fp, " %cn  the line number in that file, %cN current line number\n", cEscape, cEscape);
	fprintf(fp, " %cu  a unique count assigned based on first occurrence (0, 1, 2...)\n", cEscape);
	fprintf(fp, " %ck  the key built from the element (from -k)\n", cEscape);
	fprintf(fp, " %cm  the element's new memory (from -r, context of -R) [aka %cr]\n", cEscape, cEscape);
	fprintf(fp, " %cv  the memory from prev (under -i)\n", cEscape);
	fprintf(fp, " %ce  the accumulator for this key (under -e or -f)\n", cEscape);
	fprintf(fp, " %cp  the previous accumulator for this key\n", cEscape);
	fprintf(fp, " %cc  count occurrence from current run in decimal (under -c or -l)\n", cEscape);
	fprintf(fp, " %co  the memory count extracted from prev via -x and strtoul (under -c or -l)\n", cEscape);
	fprintf(fp, " %ct  total occurrence of key (%cc+%co)\n", cEscape, cEscape, cEscape);
	fprintf(fp, "Dicer (under %c[) and mixer [under %c(] examples:\n", cEscape, cEscape);
	fprintf(fp, "     %c[1 2]  the second word from the first line of this element\n", cEscape);
	fprintf(fp, "   %c(1,6-9)  four characters from the first line of this element\n", cEscape);
	fprintf(fp, " %c([2 3],$)  the last character from the third word on the element's 2nd line\n", cEscape);
	fprintf(fp, "   %c(m,$-1)  memory built from the element spelled backwards\n", cEscape);
	fprintf(fp, "     %c[f/$]  the element\'s source file without a leading path qualification\n", cEscape);
	fprintf(fp, "Fold case to lower with %cL..., or upper with %cU...\n", cEscape, cEscape);
}

/* Compute %* from a list (any of %*, %f*, or %t*)			(ksb)
 * call with ((char **)0, 0) to free space.  Taken in whole from xapply.
 */
static char *
CatAlloc(char **ppcList, unsigned int iMax, int cSep)
{
	register unsigned int i, uNeed;
	register char *pcCat;
	static char *pcMine = (char *)0;
	static unsigned int uMyMax = 0;

	uNeed = 0;
	if ((char **)0 == ppcList) {
		if ((char *)0 != pcMine) {
			free((void *)pcMine);
			pcMine = (char *)0;
		}
		uMyMax = 0;
		return (char *)0;
	}
	if (0 == iMax) {
		static char *apc_[2] = { acEmpty, (char *)0 };

		ppcList = apc_;
		iMax = 1;
	}
	for (i = 0; i < iMax; ++i) {
		uNeed += strlen(ppcList[i])+1;
	}
	if (uMyMax < uNeed) {
		if ((char *)0 != pcMine) {
			free((void *)pcMine);
			pcMine = (char *)0;
		}
		uNeed |= 31;
		uMyMax = ++uNeed;
		pcMine = (char *)malloc(uNeed);
	}
	if ((char *)0 != pcMine) {
		pcCat = pcMine;
		for (i = 0; i < iMax; ++i) {
			if (0 != i) {
				*pcCat++ = cSep;
			}
			(void)strcpy(pcCat, ppcList[i]);
			pcCat += strlen(ppcList[i]);
		}
	}
	return pcMine;
}

static int
	wLockOldEvery = 0,	/* stop the madness of -f "A%p%p"	*/
	wLockEvery = 0;		/* same for -e ".%e%e"			*/

/* Build the diced expression from the data provided			(ksb)
 * This is copied largely from xapply, of course.
 */
static char *
KFmt(char *pcBuffer, unsigned *puMax, char *pcExp, char **ppcCall, int iLimit, CONTEXT *pCX, char *pcFrom, char *pcKey, char *pcMemory)
{
	register char *pcScan, *pcP;
	register int i, cCache, fGroup, fSubPart, fMixer;
	register CONTEXT *pCXLocal;
	register int fCase;	/* -1 lower, 0 neutral, 1 upper */
	auto unsigned uMax, uCall, uHold;
	auto char acNumber[132];
	auto CONTEXT CXFake;

	if ((CONTEXT *)0 == (pCXLocal = pCX)) {
		CXFake.ubytes = 0,
		CXFake.ucount = 0,
		CXFake.uorig = 0,
		CXFake.ulineno = 0,
		CXFake.uuniq = 0,
		CXFake.ufileno = 0,
		CXFake.pcfrom = pcFrom,
		pCXLocal = & CXFake;
	}
	cCache = cEscape;
	pcScan = pcBuffer;
	uMax = *puMax;
	uHold = 0;
	while ('\000' != *pcExp) {
		if (cCache != (*pcScan++ = *pcExp++)) {
			++uHold;
			continue;
		}
		if (cCache == (i = *pcExp++)) {
			continue;
		}
		--pcScan;
		fCase = 0;
		if ('L' == i || 'U' == i) {
			fCase = 'L' == i ? -1 : 1;
			if ('\000' != (i = *pcExp))
				++pcExp;
		}
		if (0 != (fMixer = MIXER_RECURSE == i)) {
			i = *pcExp++;
		}
		fGroup = 0;
		if ((fSubPart = '[' == i) || '{' == i) {
			fGroup = 1;
			i = *pcExp++;
		}
		if ('F' == i) {
			pcP = CXGlobal.pcfrom;
		} else if ('N' == i) {
			snprintf(acNumber, sizeof(acNumber), "%lu", (unsigned long)CXGlobal.ulineno);
			pcP = acNumber;
		} else if ('M' == i) {
			snprintf(acNumber, sizeof(acNumber), "%d", wMany);
			pcP = acNumber;
		} else if ('f' == i) {
			pcP = pCXLocal->pcfrom;
		} else if ('n' == i) {
			snprintf(acNumber, sizeof(acNumber), "%lu", pCXLocal->ulineno);
			pcP = acNumber;
		} else if ('u' == i) {
			snprintf(acNumber, sizeof(acNumber), "%lu", pCXLocal->uuniq);
			pcP = acNumber;
		} else if ('i' == i) {
			snprintf(acNumber, sizeof(acNumber), "%lu", pCXLocal->ufileno);
			pcP = acNumber;
		} else if ('k' == i) {
			if ((char *)0 == (pcP = pcKey)) {
				fprintf(stderr, "%s: %ck: key not available in this context\n", progname, cEscape);
				return (char *)0;
			}
		} else if ('m' == i || 'r' == i) {
			if ((char *)0 == (pcP = pcMemory)) {
				fprintf(stderr, "%s: %cv: memory not available in this context\n", progname, cEscape);
				return (char *)0;
			}
		} else if ('p' == i || 'P' == i) {
			if (wLockOldEvery) {
				fprintf(stderr, "%s: %c%c: cannot be used to define itself\n", progname, cEscape, i);
				exit(EX_PROTOCOL);
			}
			if ((char *)0 == (pcP = GDOldEvery.dptr)) {
				static int fToldLast = 0;
				if (!fToldLast) {
					fprintf(stderr, "%s: %cv: no -e or -f specified\n", progname, cEscape);
					fToldLast = 1;
				}
				pcP = pcPad;
			}
		} else if ('e' == i) {
			if (wLockEvery) {
				fprintf(stderr, "%s: %c%c: cannot be used to define itself (or %cp)\n", progname, cEscape, i, cEscape);
				exit(EX_PROTOCOL);
			}
			if ((char *)0 == (pcP = GDEvery.dptr)) {
				static int fToldEvery = 0;
				if (!fToldEvery) {
					fprintf(stderr, "%s: %cv: no -e or -f specified\n", progname, cEscape);
					fToldEvery = 1;
				}
				pcP = pcPad;
			}
		} else if ('v' == i) {
			if ((char *)0 == (pcP = GDInv.dptr)) {
				static int fTold = 0;
				if (!fTold) {
					fprintf(stderr, "%s: %c%c: -I no previous state specified\n", progname, cEscape, i);
					fTold = 1;
				}
				pcP = pcPad;
			}
		} else if ('o' == i || 'c' == i || 't' == i) {
			register int cSpec;

			snprintf(acNumber, sizeof(acNumber), "00");
			pcP = acNumber;
			cSpec = fCounting ? 'c' : fRepeated ? 'd' : '\000';
			if ((CONTEXT *)0 == pCX) {
				static int fTold = 0;
				if (!fTold) {
					fprintf(stderr, "%s: %c%c: no counting context for %s, specify -c, -d or -l\n", progname, cEscape, i, pcKey);
					fTold = 1;
				}
			} else if (&CXGlobal == pCX) {
				fprintf(stderr, "%s: %c%c: counting context cannot form a key\n", progname, cEscape, i);
				exit(EX_DATAERR);
			} else if ('\000' == cSpec) {
				static int fToldOption = 0;
				if (!fToldOption) {
					fprintf(stderr, "%s: %c%c: not available unless either -c or -d is specified\n", progname, cEscape, i);
					fToldOption = 1;
				}
			} else if ('c' == i) {
				snprintf(acNumber, sizeof(acNumber), "%lu", pCXLocal->ucount-pCXLocal->uorig);
			} else if ('o' == i) {
				snprintf(acNumber, sizeof(acNumber), "%lu", pCXLocal->uorig);
			} else if ('t' == i) {
				snprintf(acNumber, sizeof(acNumber), "%lu", pCXLocal->ucount);
			} else { /*NOTREACHED*/
				fprintf(stderr, "%s: unknown markup: %c%c?\n", progname, cEscape, i);
				exit(EX_DATAERR);
			}
		} else if ((char **)0 == ppcCall) {
			static int fMsg = 0;
			pcP = pcPad;
			if (!fMsg) {
				fprintf(stderr, "%s: -I: original lines are not available in this context\n", progname);
				fMsg = 1;
			}
		} else if ('*' == i) {
			pcP = CatAlloc(ppcCall, iLimit, ' ');
		} else if ('@' == i) {
			pcP = CatAlloc(ppcCall, iLimit, '\n');
		} else if ('$' == i) {
			pcP = ppcCall[iLimit-1];
		} else if (isdigit(i)) {
			register int iParam, iTmp;

			iTmp = i;
			iParam = iTmp - '0';
			while (fGroup) {
				iTmp = *pcExp++;
				if (!isdigit(iTmp))
					break;
				iParam *= 10;
				iParam += iTmp - '0';
			}
			if (0 == iParam) {
				pcP = acEmpty;
			} else if (iParam < 1 || iParam > iLimit) {
				fprintf(stderr, "%s: %d: escape references an out of range parameter (span limit %d)\n", progname, iParam, iLimit);
				return (char *)0;
			} else {
				pcP = ppcCall[iParam-1];
			}
			if (fGroup) {
				--pcExp;
			}
		} else {
			fprintf(stderr, "%s: %c%1.1s: unknown escape sequence\n", progname, cEscape, pcExp-1);
			return (char *)0;
		}
		if (fSubPart) {
			uCall = uMax-uHold;
			pcExp = Dicer(pcScan, &uCall, pcExp, pcP);
			if (']' != pcExp[-1]) {
				fprintf(stderr, "%s: %c[: missing ']'\n", progname, cEscape);
				return (char *)0;
			}
		} else {
			uCall = strlen(pcP);
			if (uHold+uCall < uMax) {
				(void)strcpy(pcScan, pcP);
			}
		}
		if (fMixer) {
			uCall = uMax-uHold;
			if ((char *)0 == (pcExp = Mixer(pcScan, &uCall, pcExp, MIXER_END))) {
				fprintf(stderr, "%s: mixer %c%c..%s: syntax errorexpression\n", progname, cEscape, MIXER_RECURSE, pcExp);
				return (char *)0;
			}
		}
		if (uHold+uCall >= uMax) {
			fprintf(stderr, "%s: expansion is too large for buffer\n", progname);
			return (char *)0;
		}
		if (-1 == fCase) {
			for (i = 0; i < uCall; ++i)
				pcScan[i] = tolower(pcScan[i]);
		} else if (1 == fCase) {
			for (i = 0; i < uCall; ++i)
				pcScan[i] = toupper(pcScan[i]);
		}
		pcScan += uCall;
		uHold += uCall;
		if (fSubPart) {
			fSubPart = 0;
		} else if (fGroup) {
			if ('}' != *pcExp) {
				fprintf(stderr, "%s: %c{..%s: missing close culry (})\n", progname, cEscape, pcExp);
				return (char *)0;
			}
			++pcExp;
		}
		/* %p, plus remove following markup, after an empty value (up
		 * to escape or EOS): %0 is useful here to limit range.
		 */
		if ('P' == i && 0 == uCall) {
			while (cCache != *pcExp && '\000' != *pcExp)
				++pcExp;
		}
	}
	*puMax = uHold;
	if (uMax > uHold) {
		*pcScan = '\000';
	}
	return pcBuffer;
}

/* Make a user requested "report line", rather than just the key	(ksb)
 * in this context %k is the key, %m is the memory, and %f is the dbf, or
 * the input file.  The dbf in replay of prev, the input file otherwise.
 */
static void
Report(FILE *fp, char *pcFmtE, char *pcKey, char *pcValue, char *pcDb, CONTEXT *pCX, char **ppcDeck, unsigned uDeck)
{
	static char *pcOut = (char *)0;
	static unsigned uHold;
	register char cEOL;
	auto unsigned uMax;

	/* Allow the empty string to act like grep -s (silent), as the
	 * blank line output was not so useful, really.
	 */
	if ('\000' == *pcFmtE) {
		return;
	}
	cEOL = fZeroEOL ? '\000' : '\n';
	if ((char *)0 == pcOut) {
		uHold = wBufSize*(iDeck+1);
		if ((char *)0 == (pcOut = malloc(uHold))) {
			fprintf(stderr, "%s: malloc: %lu: %s\n", progname, (unsigned long)uHold, strerror(errno));
			exit(EX_OSERR);
		}
	}
	uMax = uHold;
	if ((char *)0 != KFmt(pcOut, & uMax, pcFmtE, ppcDeck, uDeck, pCX, pcDb, pcKey, pcValue)) {
		fprintf(fp, "%s%c", pcOut, cEOL);
	} else {
		fprintf(fp, "%s%c", pcKey, cEOL);
	}
}

/* Do all the startup stuff, like output the state if asked,		(ksb)
 * and set any dicer expressions we need via ReconOpts.
 */
static void
Starter()
{
	register int iTmpFd, iCntFd, iCheck, iEveryFd;
	register char *pcRemember;
	auto struct stat stPrev, stCur;
	auto datum GDTemp, GDValue, GDLoop;
	static char acDefTmp[] = "/tmp", acAltTmp[] = "/var/tmp";

	GDTemp.dptr = GDLoop.dptr = GDValue.dptr = (char *)0;
	GDTemp.dsize = GDLoop.dsize = GDValue.dsize = 0;
	ReconOpts();
	iTmpFd = iCntFd = iEveryFd = -1;
	dbfState = dbfPrev = dbfCounts = dbfEvery = (GDBM_FILE)0;

	/* Try the $TMPDIR, then /tmp then /var/tmp.  Some people set
	 * $TMPDIR to a nonexistent place to hose themselves really well.
	 */
	pcRemember = pcTmpdir;
	while (-1 == stat(pcTmpdir, &stCur)) {
		if (pcTmpdir == acAltTmp) {
			(void)stat(pcRemember, &stCur);
			fprintf(stderr, "%s: stat: %s: %s\n", progname, pcRemember, strerror(errno));
			exit(EX_CANTCREAT);
		}
		if (0 == strcmp(pcTmpdir, acDefTmp)) {
			pcTmpdir = acAltTmp;
		} else {
			pcTmpdir = acDefTmp;
		}
	}
	if (fStable) {
		snprintf(acStableList, sizeof(acStableList), "%s/%s", pcTmpdir, acMyTemp);
		if (-1 == (iTmpFd = mkstemp(acStableList))) {
			fprintf(stderr, "%s: mkstemp: %s: %s\n", progname, acTmpDb, strerror(errno));
			exit(EX_CANTCREAT);
		}
		if ((FILE *)0 == (fpStable = fdopen(iTmpFd, "w+"))) {
			fprintf(stderr, "%s: fdopen: %d: %s\n", progname, iTmpFd, strerror(errno));
			exit(EX_OSERR);
		}
		iTmpFd = -1;
	} else {
		fpStable = (FILE *)0;
	}
	if ((char *)0 == pcState) {
		snprintf(acTmpDb, sizeof(acTmpDb), "%s/%s", pcTmpdir, acMyTemp);
		if (-1 == (iTmpFd = mkstemp(acTmpDb))) {
			fprintf(stderr, "%s: mkstemp: %s: %s\n", progname, acTmpDb, strerror(errno));
			exit(EX_CANTCREAT);
		}
		pcState = acTmpDb;
	}

	/* If we have to count instances (under -c or -d) open that hash
	 */
	if (fCounting || fRepeated) {
		snprintf(acCountDb, sizeof(acCountDb), "%s/%s", pcTmpdir, acMyTemp);
		if (-1 == (iCntFd = mkstemp(acCountDb))) {
			fprintf(stderr, "%s: mkstemp: %s: %s\n", progname, acTmpDb, strerror(errno));
			exit(EX_CANTCREAT);
		}
	}
	if ((char *)0 != pcEveryE || (char *)0 != pcFirstE) {
		snprintf(acEveryDb, sizeof(acEveryDb), "%s/%s", pcTmpdir, acMyTemp);
		if (-1 == (iEveryFd = mkstemp(acEveryDb))) {
			fprintf(stderr, "%s: mkstemp: %s: %s\n", progname, acEveryDb, strerror(errno));
			exit(EX_CANTCREAT);
		}
	}
	/* If we were given a previous state we have to open it and
	 * maybe output the elements (under -i).
	 */
	if ((GDBM_FILE)0 == (dbfState = gdbm_open(pcState, 0, GDBM_WRCREAT|(fLockFree ? GDBM_NOLOCK : 0), 0666, (void (*))0))) {
		fprintf(stderr, "%s: gdbm_open: %s: %s\n", progname, pcState, gdbm_strerror(gdbm_errno));
		exit(EX_CONFIG);
	} else if (-1 == (iCheck = fstat(gdbm_fdesc(dbfState), & stCur))) {
		fprintf(stderr, "%s: stat: %s: %s\n", progname, pcState, strerror(errno));
		exit(EX_NOINPUT);
	} else if ((char *)0 == pcPrevState) {
		/* do not set one */
	} else if ((GDBM_FILE)0 == (dbfPrev = gdbm_open(pcPrevState, 0, GDBM_READER|(fLockFree ? GDBM_NOLOCK : 0), 0666, (void (*))0))) {
		fprintf(stderr, "%s: gdbm_open: %s: %s\n", progname, pcPrevState, gdbm_strerror(gdbm_errno));
		exit(EX_NOINPUT);
	/* When (after GDBM touched it) the prev is now the current we'll
	 * drop core when we close both later.  We must have raced with a
	 * process that renamed the file, or been given the same gbd file.
	 */
	} else if (-1 == fstat(gdbm_fdesc(dbfPrev), & stPrev)) {
		fprintf(stderr, "%s: stat: %s: %s\n", progname, pcPrevState, strerror(errno));
		exit(EX_OSERR);
	} else if (stPrev.st_dev == stCur.st_dev && stPrev.st_ino == stCur.st_ino) {
		dbfPrev = dbfState;
	}
	if ('\000' != acCountDb[0] && (GDBM_FILE)0 == (dbfCounts = gdbm_open(acCountDb, 0, GDBM_WRCREAT, 0666, (void (*))0))) {
		fprintf(stderr, "%s: gdbm_open: %s: %s\n", progname, acCountDb, gdbm_strerror(gdbm_errno));
		exit(EX_TEMPFAIL);
	}
	if ('\000' != acEveryDb[0] && (GDBM_FILE)0 == (dbfEvery = gdbm_open(acEveryDb, 0, GDBM_WRCREAT, 0666, (void (*))0))) {
		fprintf(stderr, "%s: gdbm_open: %s: %s\n", progname, acEveryDb, gdbm_strerror(gdbm_errno));
		exit(EX_TEMPFAIL);
	}
	if (-1 != iTmpFd) {
		close(iTmpFd);
	}
	if (-1 != iCntFd) {
		close(iCntFd);
	}
	if (-1 != iEveryFd) {
		close(iEveryFd);
	}

	if ((GDBM_FILE)0 == dbfPrev) {
		if (fInvert && !fCounting) {
			fprintf(stderr, "%s: -v: no prev GDBM, no keys can match\n", progname);
			exit(EX_DATAERR);
		}
		return;
	}
	/* Include the previous elements in the output under -i, if you gave
	 * both -v and -i I don't know why, but it must be more clever than I.
	 * N.B. -f doesn't apply to -i, we must see the key in an input file.
	 */
	if (fInclude) {
		auto CONTEXT CXFake = {
			.ubytes = 0,
			.ucount = 1,
			.uorig = 1,
			.ulineno = 0,
			.uuniq = 0,
			.ufileno = 0,
		};
		CXFake.pcfrom = pcPrevState;
		for (GDLoop = gdbm_firstkey(dbfPrev); (char *)0 != GDLoop.dptr; /* !continue */) {
			GDTemp = gdbm_nextkey(dbfPrev, GDLoop);
			GDInv = gdbm_fetch(dbfPrev, GDLoop);
			Report(stdout, pcIncludeE, GDLoop.dptr, GDInv.dptr, pcPrevState, (CONTEXT *)0, (char **)0, 0);
			free((void *)GDInv.dptr);
			GDInv.dptr = (char *)0;
			/* N.B. no continue's in this loop, as this is manditory
			 */
			free((void *)GDLoop.dptr);
			GDLoop.dptr = (char *)0;
			GDLoop = GDTemp;
		}
	}

	/* No point in checking the same file twice, ignore a duplicate prev.
	 */
	if (dbfPrev != dbfState) {
		return;
	}
	if (fInvert) {
		fprintf(stderr, "%s: -v: all matched excluded by state and prev being the same GDBM (%s == %s)\n", progname, pcState, pcPrevState);
		exit(EX_DATAERR);
	}
}

/* Remove our temporary db if'n we held one				(ksb)
 * Other than -c we don't need a way to summarize here, I've never found
 * another use for it.  Use -s to output the keys in discovery order.
 */
static void
Cleanup()
{
	register char **ppcCall;
	register int i;
	register size_t *pwLines, wStep, wAll;
	register CONTEXT *pCXFound;
	auto unsigned uMax;
	auto datum GDTemp, GDLoop, GDMemory, GDState;

	if (fCounting) {
		wAll = wBufSize*iDeck;
		if ((char **)0 == (ppcCall = calloc(iDeck+1, sizeof(char *))) || (size_t *)0 == (pwLines = calloc(iDeck+1, sizeof(size_t))) || (char *)0 == (GDMemory.dptr = malloc(wAll))) {
			fprintf(stderr, "%s: cleanup: calloc: out of memory: %s\n", progname, strerror(errno));
			exit(EX_TEMPFAIL);
		}
		if ((GDBM_FILE)0 != dbfEvery) {
			GDOldEvery.dptr = acEmpty;
			GDOldEvery.dsize = 1;
		}
		if ((FILE *)0 != fpStable) {
			rewind(fpStable);
			GDLoop.dptr = malloc(wBufSize);
		} else {
			GDLoop = gdbm_firstkey(dbfCounts);
		}
		while ((char *)0 != GDLoop.dptr) {
			if ((FILE *)0 != fpStable) {
				register int c;

				for (i = 0; i < wBufSize; ++i) {
					c = getc(fpStable);
					if (-1 == c)
						break;
					if ('\000' == (GDLoop.dptr[i] = c))
						break;
				}
				if (-1 == c)
					break;
				GDLoop.dsize = i+1;
			}
			if ((GDBM_FILE)0 != dbfEvery) {
				GDEvery = gdbm_fetch(dbfEvery, GDLoop);
			}
			if ((GDBM_FILE)0 != dbfPrev) {
				GDInv = gdbm_fetch(dbfPrev, GDLoop);
			}
			GDState = gdbm_fetch(dbfCounts, GDLoop);
			pCXFound = (CONTEXT *)GDState.dptr;
			if (fRepeated && (fInvert == pCXFound->wis_dup)) {
				/* ignore single reps under -d, or
				 * multireps under -vd
				 */
			} else {
				CXGlobal.pcfrom = pCXFound->pcfrom;
				CXGlobal.ulineno = pCXFound->ulineno;
				wStep = 0;
				for (i = 0; i < iDeck; ++i) {
					ppcCall[i] = pCXFound->acdeck+wStep;
					pwLines[i] = wStep;
					wStep += 1+strlen(ppcCall[i]);
				}
				uMax = wAll;
				if ((char *)0 == KFmt(GDMemory.dptr, & uMax, pcRememberE, ppcCall, iDeck, pCXFound, pCXFound->pcfrom, GDLoop.dptr, (char *)0)) {
					fprintf(stderr, "%s: dicer: cannot parse \"%s\", or buffer space too small\n", progname, pcRememberE);
					exit(EX_DATAERR);
				}
				GDMemory.dsize = strlen(GDMemory.dptr)+1;
				(void)gdbm_store(dbfState, GDLoop, GDMemory, GDBM_REPLACE);
				Report(stdout, pcReportE, GDLoop.dptr, GDMemory.dptr, pCXFound->pcfrom, pCXFound, ppcCall, iDeck);
			}
			free((void *)GDState.dptr);
			GDState.dptr = (char *)0;
			if ((char *)0 != GDInv.dptr) {
				free((void *)GDInv.dptr);
				GDInv.dptr = (char *)0;
			}
			if ((char *)0 != GDEvery.dptr) {
				free((void *)GDEvery.dptr);
				GDEvery.dptr = (char *)0;
			}
			/* N.B. no continue above, this update is manditory
			 */
 /* next_stable: */
			if ((FILE *)0 == fpStable) {
				GDTemp = gdbm_nextkey(dbfCounts, GDLoop);
				free((void *)GDLoop.dptr);
				GDLoop.dptr = (char *)0;
				GDLoop = GDTemp;
			}
		}
		free((void *)ppcCall);
		free((void *)pwLines);
		free((void *)GDMemory.dptr);
		GDOldEvery.dptr = (char *)0;
	}
	if ((GDBM_FILE)0 != dbfCounts) {
		gdbm_close(dbfCounts);
		dbfCounts = (GDBM_FILE)0;
	}
	if ((GDBM_FILE)0 != dbfPrev && dbfState != dbfPrev) {
		gdbm_close(dbfPrev);
		dbfPrev = (GDBM_FILE)0;
	}
	if ((GDBM_FILE)0 != dbfState) {
		gdbm_close(dbfState);
		dbfState = (GDBM_FILE)0;
	}
	if ((GDBM_FILE)0 != dbfEvery) {
		gdbm_close(dbfEvery);
		dbfEvery = (GDBM_FILE)0;
	}
	if ((FILE *)0 != fpStable) {
		fclose(fpStable);
	}
	if ('\000' != acCountDb[0]) {
		(void)unlink(acCountDb);
	}
	if ('\000' != acTmpDb[0]) {
		(void)unlink(acTmpDb);
	}
	if ('\000' != acEveryDb[0]) {
		(void)unlink(acEveryDb);
	}
	if ('\000' != acStableList[0]) {
		(void)unlink(acStableList);
	}
}

/* Do the filter operation for a given file				(ksb)
 */
static int
Filter(FILE *fpIn, char *pcName)
{
	register int iStep, iN, c, cEol;
	register char *pcPart, **ppcCall, *pcDKey;
	register CONTEXT *pCXEnable, *pCXRead;
	auto char *pcDSquelch;
	auto size_t wAll, wCur, *pwLines;
	auto unsigned uMax;
	auto unsigned long uLineCur, uCount;
	auto datum GDKey, GDCount;

	GDKey.dptr = (char *)0;
	GDKey.dsize = 0;
	GDCount.dptr = (char *)0;
	GDCount.dsize = 0;
	CXGlobal.ulineno = 0;
	CXGlobal.pcfrom = pcName;
	++CXGlobal.ufileno;
	wAll = wBufSize * iDeck + sizeof(CONTEXT);
	pcDSquelch = (char *)0;
	if ((char *)0 == (pcDKey = (char *)malloc(wBufSize)) || (fSquelch && (char *)0 == (pcDSquelch = (char *)malloc(wBufSize)))) {
		fprintf(stderr, "%s: malloc: %ld: %s\n", progname, (long)wBufSize, strerror(errno));
		exit(EX_OSERR);
	}
	if ((GDBM_FILE)0 == dbfEvery) {
		GDEvery.dptr = (char *)0;
	} else if ((char *)0 == (GDEvery.dptr = (char *)malloc(wAll))) {
		fprintf(stderr, "%s: malloc: %ld: %s\n", progname, (long)wAll, strerror(errno));
		exit(EX_OSERR);
	} else {
		GDEvery.dptr[0] = '\000';
	}
	if ((CONTEXT *)0 == (pCXEnable = (CONTEXT *)malloc(wAll)) || (char *)0 == (GDMemory.dptr = malloc(wAll))) {
		fprintf(stderr, "%s: malloc: %ld: %s\n", progname, (long)wAll, strerror(errno));
		exit(EX_OSERR);
	}
	if ((size_t *)0 == (pwLines = (size_t *)calloc(iDeck+1, sizeof(size_t))) || (char **)0 == (ppcCall = calloc(iDeck+1, sizeof(char *)))) {
		fprintf(stderr, "%s: malloc: out of memory: %s\n", progname, strerror(errno));
		exit(EX_TEMPFAIL);
	}

	uCount = 0;
	pwLines[0] = 0;
	ppcCall[iDeck] = (char *)0;
	cEol = fZeroEOL ? '\000' : '\n';
	for (uLineCur = 0;;) {
		iStep = 0;
		wCur = 0;
		while (EOF != (c = getc(fpIn))) {
			pCXEnable->acdeck[wCur++] = c;
			if (cEol == c) {
				++uLineCur;
				pwLines[++iStep] = wCur;
				pCXEnable->acdeck[wCur-1] = '\000';
				if (iDeck == iStep)
					break;
			}
			if (wCur < wAll) {
				continue;
			}
			wAll += 1024;
			if ((CONTEXT *)0 == (pCXEnable = realloc((void *)pCXEnable, wAll))) {
				fprintf(stderr, "%s: realloc: %ld: %s\n", progname, (long)wAll, strerror(errno));
				exit(EX_TEMPFAIL);
			}
		}
		if (0 == wCur) {
			break;
		}
		CXGlobal.ulineno = uLineCur - (iDeck-1);
		for (iN = 0; iN < iDeck; ++iN) {
			ppcCall[iN] = (iN < iStep) ? pCXEnable->acdeck+pwLines[iN] : pcPad;
		}
		uMax = wBufSize;
		pcPart = KFmt(pcDKey, & uMax, pcKeyE, ppcCall, iDeck, &CXGlobal, pcName, (char *)0, (char *)0);
		if ((char *)0 == pcPart) {
			Cleanup();
			exit(EX_DATAERR);
		}
		if ((char *)0 != pcDSquelch && (char *)0 != GDKey.dptr && 0 == strcmp(pcDSquelch, GDKey.dptr)) {
			continue;	/* -S duplicate */
		}
		GDKey.dptr = pcDKey;
		GDKey.dsize = strlen(GDKey.dptr)+1;
		if ((char *)0 != pcDSquelch) {
			strcpy(pcDSquelch, pcDKey);
		}

		if ((GDBM_FILE)0 != dbfPrev) {
			if ((char *)0 != GDInv.dptr) {
				free((void *)GDInv.dptr);
				GDInv.dptr = (char *)0;
			}
			GDInv = gdbm_fetch(dbfPrev, GDKey);
		}

		if ((char *)0 != GDEvery.dptr) {
			GDEvery.dptr[0] = '\000';
			if ((char *)0 != GDOldEvery.dptr) {
				free((void *)GDOldEvery.dptr);
				GDOldEvery.dptr = (char *)0;
			}
			GDOldEvery = gdbm_fetch(dbfEvery, GDKey);
			++wLockEvery, ++wLockOldEvery;
			uMax = wAll;
			if ((char *)0 != GDOldEvery.dptr) {
				/* cool */
			} else if ((char *)0 == KFmt(GDEvery.dptr, & uMax, pcFirstE, ppcCall, iDeck, &CXGlobal, pcName, GDKey.dptr, (char *)0)) {
				fprintf(stderr, "%s: -f: dicer failed to expand \"%s\"\n", progname, pcFirstE);
				exit(EX_DATAERR);
			} else {
				GDOldEvery.dptr = strdup(GDEvery.dptr);
				GDOldEvery.dsize = strlen(GDOldEvery.dptr)+1;
			}
			--wLockOldEvery;

			uMax = wAll;
			if ((char *)0 == KFmt(GDEvery.dptr, & uMax, pcEveryE, ppcCall, iDeck, &CXGlobal, pcName, GDKey.dptr, (char *)0)) {
				fprintf(stderr, "%s: -e: dicer failed to expand \"%s\"\n", progname, pcEveryE);
				exit(EX_DATAERR);
			}
			--wLockEvery;
			GDEvery.dsize = strlen(GDEvery.dptr)+1;
			(void)gdbm_store(dbfEvery, GDKey, GDEvery, GDBM_REPLACE);
		}

		/* Update the count of the key if that's the game, or
		 * skip line if the key is in the previous or the active dbf,
		 * or under -v, when it is not in there.
		 */
		if ((GDBM_FILE)0 != dbfCounts) {
			register int fSkip;
			auto char acNumber[1024];
			auto unsigned uTop;
			auto char *pcEnd;

			if ((void *)pCXEnable != GDCount.dptr && (char *)0 != GDCount.dptr) {
				free((void *)GDCount.dptr);
				GDCount.dptr = (char *)0;
			}
			GDCount = gdbm_fetch(dbfCounts, GDKey);
			if ((CONTEXT *)0 != (pCXRead = (CONTEXT *)GDCount.dptr)) {
				uCount = pCXRead->ucount;
				/* keep same memory below */
			} else if ('\000' == pcExtract[0] || (GDBM_FILE)0 == dbfPrev) {
				/* no extract text expected or -I to look in */
				uCount = 0;
			} else if ((char *)0 != GDInv.dptr) {
				uTop = sizeof(acNumber)-1;
				if ((char *)0 == KFmt(acNumber, &uTop, pcExtract, ppcCall, iDeck, &CXGlobal, pcName, GDKey.dptr, (char *)0)) {
					static int fToldExfail = 0;
					uCount = 1;
					if (!fToldExfail) {
						fprintf(stderr, "%s: -x: dicer expansion failed for \"%s\" (default to %lu)\n", progname, pcExtract, (unsigned long)uCount);
						fToldExfail = 1;
					}
				} else {
					register char *pcLead;

					pcLead = acNumber;
					while (isspace(*pcLead))
						++pcLead;
					uCount = strtoul(pcLead, &pcEnd, 0);
					if (pcLead == pcEnd)
						uCount = 1;
				}
			} else {
				uCount = 0;
			}
			++uCount;

			/* We record the first deck that made the key record
			 * as "<context><deck>", previously recorded get an
			 * updated count only. If we are under -l we update
			 * the count GDB only, either in-place, or from
			 * the big buffer we use to read elements.
			 */
			if ((CONTEXT *)0 == (pCXRead = (CONTEXT *)GDCount.dptr)) {
				GDCount.dptr = (void*)pCXEnable;
				GDCount.dsize = wCur+sizeof(CONTEXT);
				pCXEnable->ubytes = wCur;
				pCXEnable->uuniq = ++CXGlobal.uuniq;
				pCXEnable->ulineno = CXGlobal.ulineno;
				pCXEnable->ufileno = CXGlobal.ufileno;
				pCXEnable->pcfrom = pcName;
				pCXEnable->uorig = uCount-1;
				pCXEnable->wis_dup = 0;
				pCXRead = pCXEnable;
				if ((FILE *)0 != fpStable)
					fprintf(fpStable, "%s%c", GDKey.dptr, '\000');
			} else if (fLast && wCur <= pCXRead->ubytes) {
				pCXRead->ulineno = CXGlobal.ulineno;
				pCXRead->ufileno = CXGlobal.ufileno;
				pCXRead->pcfrom = pcName;
				pCXRead->ubytes = wCur;
				/* uniq and orig count are the same */
				(void)memmove(pCXRead->acdeck, pCXEnable->acdeck, wCur);
			} else if (fLast) {
				pCXEnable->ulineno = CXGlobal.ulineno;
				pCXEnable->ufileno = CXGlobal.ufileno;
				pCXEnable->pcfrom = pcName;
				pCXEnable->ubytes = wCur;
				pCXEnable->uuniq = pCXRead->uuniq;
				pCXEnable->uorig = pCXRead->uorig;
				pCXEnable->wis_dup = pCXRead->wis_dup;
				free((void *)GDCount.dptr);
				GDCount.dptr = (void *)pCXEnable;
				GDCount.dsize = wCur+sizeof(CONTEXT);
				pCXRead = pCXEnable;
			}

			/* "-M -2" => 'at least 2', "-M 3" => 'exactly 3'
			 */
			fSkip = fCounting;
			pCXRead->ucount = uCount;
			if (fRepeated && !pCXRead->wis_dup && (wMany < 0 ? uCount >= -wMany : wMany == uCount)) {
				pCXRead->wis_dup |= 1;
			} else {
				fSkip = 1;
			}
			(void)gdbm_store(dbfCounts, GDKey, GDCount, GDBM_REPLACE);
			if (fSkip) {
				continue;
			}
		} else {
			auto datum GDDup;

			if (fInvert ^ ((char *)0 != GDInv.dptr)) {
				continue;
			}
			if (dbfState != dbfPrev) {
				GDDup = gdbm_fetch(dbfState, GDKey);
				if ((char *)0 != GDDup.dptr) {
					free((void *)GDDup.dptr);
					GDDup.dptr = (char *)0;
					continue;
				}
			}
		}
		uMax = wAll;
		if ((char *)0 == KFmt(GDMemory.dptr, & uMax, pcRememberE, ppcCall, iDeck, fRepeated ? pCXRead : &CXGlobal, pcName, GDKey.dptr, (char *)0)) {
			static int fErr = 0;
			if (0 == fErr) {
				fprintf(stderr, "%s: dicer: cannot parse \"%s\"\n", progname, pcRememberE);
				fErr = 1;
			}
			(void)strcpy(GDMemory.dptr, acDefMem);
		}
		GDMemory.dsize = strlen(GDMemory.dptr)+1;
		(void)gdbm_store(dbfState, GDKey, GDMemory, GDBM_REPLACE);
		if (!fRepeated)
			++CXGlobal.uuniq;
		Report(stdout, pcReportE, GDKey.dptr, GDMemory.dptr, pcName, fRepeated ? pCXRead : &CXGlobal, ppcCall, iDeck);
	}
	if ((void *)pCXEnable != GDCount.dptr && (char *)0 != GDCount.dptr) {
		free((void *)GDCount.dptr);
		GDCount.dptr = (char *)0;
	}
	if ((char *)0 != GDInv.dptr) {
		free((void *)GDInv.dptr);
		GDInv.dptr = (char *)0;
	}
	if ((char *)0 != GDEvery.dptr) {
		free((void *)GDEvery.dptr);
		GDEvery.dptr = (char *)0;
	}
	free((void *)GDMemory.dptr);
	free((void *)ppcCall);
	free((void *)pwLines);
	free((void *)pCXEnable);
	free((void *)pcDKey); /* same as  GDKey.dptr */
	if ((char *)0 != pcDSquelch)
		free((void *)pcDSquelch);
	return 0;
}
