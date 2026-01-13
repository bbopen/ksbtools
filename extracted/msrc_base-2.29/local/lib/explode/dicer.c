/*@Version $Revision: 6.19 $@*/
/*@Header@*/
/* $Id: dicer.c,v 6.19 2009/10/16 15:38:39 ksb Exp $
 * Most of the code to implement %[dicer separator field] and %(dicer,mixer)
 * $Compile: ${cc-cc} -DDICER_TEST -g -Wall -o %F %f
 */
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#if DEBUG
#include <stdio.h>
#endif

/*@Shell %echo ${MACHINE_H+"$MACHINE_H"}%@*/
#include "dicer.h"

/*@Explode dicer@*/
char DicerVersion[] =
	"$Id: dicer.c,v 6.19 2009/10/16 15:38:39 ksb Exp $";

/* Select a substring from the parameter we just found			(ksb)
 * this allows us to get params from the password/group file
 * We can recurse to get subfields...
 * We can split on any single character, or "white space".
 * If you want more, use an RE, or try Mixer as a turbo.
 *
 * The field number can be a base 10 number, or "$" for the last
 * in the record.  The empty count is taken as 1.
 * If we need memory and can't malloc it we return (char *)0.
 * Max is the length left in the buffer (in) and the size we wrote (out),
 * we '\000' terminate the string if we have room.
 */
char *
Dicer(char *pcDest, unsigned *puMax, char *pcTemplate, char *pcData)
{
	register int cSep, iCount, iNeg;
	auto char cKeep;
	auto char *pcEnd, *pcOrig, *pcAt;
	static char acEos[] = "\000";
	auto unsigned uMaxDown = 0;
	auto char *pcMem = (char *)0;

	if ((unsigned *)0 == puMax) {
		uMaxDown = strlen(pcData)+1;
		puMax = &uMaxDown;
	}
	switch (*pcTemplate) {
	case DICER_END:	/* we have it */
		++pcTemplate;
	case '\000':	/* we lost the closing bracket. humph */
		(void)strncpy(pcDest, pcData, *puMax);
		*puMax = strlen(pcDest);
		return pcTemplate;
	case '\\':
		++pcTemplate;
		cSep = *pcTemplate++;
		break;
	case ' ':
		cSep = -1;
		++pcTemplate;
		break;
	default:
		cSep = *pcTemplate++;
		break;
	}
	/* %[n sep @ _...] build the loop, return the cat of the results
	 */
	pcAt = (char *)0;
	if ('@' == *pcTemplate) {
		register char *pcCur, *pcEos;
		auto unsigned uSize, uLimit;

		pcAt = ++pcTemplate;
		if ((char *)0 == (pcMem = strdup(pcData))) {
			return (char *)0;
		}
		pcData = pcMem;
		uLimit = *puMax;

		pcEos = (char *)0;
		while ((char *)0 != pcData && '\000' != *pcData && uLimit > 1) {
			if ((char *)0 != pcEos) {
				*pcDest++ = (-1 == cSep) ? ' ' : cSep;
				--uLimit;
			}
			pcCur = pcData;
			if (-1 == cSep) {
				while ('\000' != *pcData && !isspace(*pcData)) {
					++pcData;
				}
				pcEos = pcData;
				while (isspace(*pcData)) {
					++pcData;
				}
				*pcEos = '\000';
			} else if ((char *)0 == (pcEos = strchr(pcData, cSep))) {
				pcData = acEos;
			} else {
				*pcEos++ = '\000';
				pcData = pcEos;
			}
			uSize = uLimit;
			pcTemplate = Dicer(pcDest, &uSize, pcAt, pcCur);
			if ((char *)0 == pcTemplate) {
				free((void *)pcMem);
				return (char *)0;
			}
			uLimit -= uSize;
			pcDest += uSize;
		}
		free((void *)pcMem);
		if ('\000' != *pcData) {
			return (char *)0;
		}
		if (0 != (*puMax -= uLimit)) {
			*pcDest = '\000';
		}
		return pcTemplate;
	}
	/* %[1 -$] and %[1 -1] are unshift, and pop
	 */
	if ((iNeg = ('-' == *pcTemplate))) {
		++pcTemplate;
	}
	if ('$' == *pcTemplate) {
		iCount = -1;
		++pcTemplate;
	} else if (! isdigit(*pcTemplate)) {
		iCount = 1;
	} else {
		iCount = 0;
		do {
			iCount *= 10;
			iCount += *pcTemplate++ - '0';
		} while (isdigit(*pcTemplate));
	}
	/* locate the field they want
	 */
	pcOrig = pcData;
	while ((-1 == iCount || --iCount > 0) && '\000' != *pcData) {
		register char *pcDollar;

		pcDollar = pcData;
		if (-1 == cSep) {
			while ('\000' != *pcData && !isspace(*pcData)) {
				++pcData;
			}
			while (isspace(*pcData)) {
				++pcData;
			}
			if ('\000' == *pcData) {
				if (-1 == iCount)
					pcData = pcDollar;
				break;
			}
			continue;
		}
		pcData = strchr(pcData, cSep);
		if ((char *)0 == pcData) {
			pcData = (-1 == iCount) ? pcDollar : acEos;
			break;
		}
		++pcData;
	}
	/* find end of field
	 * pcOrig ... pcData ... pcEnd ... \000
	 */
	if (-1 == cSep) {
		pcEnd = pcData;
		while ('\000' != *pcEnd && !isspace(*pcEnd))
			++pcEnd;
		if ('\000' == *pcEnd)
			pcEnd = (char *)0;
	} else {
		pcEnd = strchr(pcData, cSep);
	}
	/* put an EOS ('\000') at the end of the field and call down,
	 * then put it back and return to caller
	 */
	if ((char *)0 != pcEnd) {
		cKeep = *pcEnd;
		*pcEnd = '\000';
	}
	/* Worse, we want to remove the named field, leaving the others,
	 * so build what they asked for.  pcMem is init'd to (char *)0
	 * at the declaration to prevent free'd a junk pointer.
	 */
	if (iNeg && (char *)0 == pcEnd) {
		pcEnd = iCount > 0 ? acEos : pcData-1;
		cKeep = *pcEnd;
		*pcEnd = '\000';
		pcData = pcOrig;
	} else if (iNeg) {
		register int iCursor;
		register unsigned uNeedDown;

		iCursor = *pcData;
		*pcData = '\000';
		uNeedDown = ((strlen(pcOrig)+strlen(pcEnd+1))|255)+1;
		if ((char *)0 == (pcMem = malloc(uNeedDown*sizeof(char)))) {
			return (char *)0;
		}
		(void)strcpy(pcMem, pcOrig);
		(void)strcat(pcMem, pcEnd+1);
		*pcData = iCursor;
		pcData = pcMem;
	}
	pcTemplate = Dicer(pcDest, puMax, pcTemplate, pcData);
	if ((char *)0 != pcEnd) {
		*pcEnd = cKeep;
	}
	if ((char *)0 != pcMem) {
		free((void *)pcMem);
	}
	return pcTemplate;
}

/*@Explode mixer@*/

/* Allow the selection of a charcter range from a string.		(ksb)
 * expr ::= pos [, pos] ;
 * pos ::= <expr> pos | [~](number|$|*) [- (number|$)] | {empty} | "txt";
 * > don't touch the string
 * 1-$> the whole string;  $-1> reverse the string;
 * 1> first character; $> (or ~1) last character; 1,$> first&last character;
 * <$-2><$-2>> all but the first and last characters (Whoa)
 * <$-2><$-2><1,$>,1,$> characters 2 and $-1, 1, and $
 * ~1-4> the last four characters ($ is just shorthand for ~1).
 * We let you output more characters than you sent us.
 * We also let position 0 == 1.
 */
char *
Mixer(char *pcInplace, unsigned *puMax, char *pcExpr, int cExit)
{
	register char *pcMem, *pcTop, *pcAdd;
	register int i, fQuote, iLevel = 0;
	register int iLeft, iRight, iLen, fTilde;
	auto unsigned uSize, uCall, uCurLen, uKeep;

	uKeep = strlen(pcInplace);
	uSize = ((unsigned *)0 == puMax) ? uKeep : *puMax;
	if ((char *)0 == (pcMem = strdup(pcInplace))) {
		return (char *)0;
	}
	iLen = strlen(pcMem);
	pcAdd = (char *)0, uCurLen = iLen;
	pcTop = pcMem;
	fQuote = '\000';
	i = *pcExpr++;
	while ('\000' != i && (cExit != i || '\000' != fQuote)) {
		if (fQuote) {
			if ('\"' == fQuote && '\"' == i) {
				fQuote = '\000';
			} else if ('`' == fQuote && '`' == i) {
				++iLevel;
				if (uSize > 0) {
					*pcInplace++ = i;
					--uSize;
				}
			} else if ('`' == fQuote && '\'' == i) {
				if (0 == --iLevel) {
					fQuote = '\000';
				} else if (0 != uSize) {
					*pcInplace++ = i;
					--uSize;
				}
			} else if (uSize > 0) {
				if (0 != uSize) {
					*pcInplace++ = i;
					--uSize;
				}
			}
			i = *pcExpr++;
			continue;
		}
		if ('\"' == i || '`' == i) {
			iLevel = 1;
			fQuote = i;
			i = *pcExpr++;
			uKeep = 0;
			continue;
		}
		if (',' == i || ' ' == i || '\t' == i) {
			i = *pcExpr++;
			if (pcMem != pcTop) {
				free((char *)pcTop);
				pcTop = pcMem;
			}
			pcAdd = (char *)0, uCurLen = iLen;
			continue;
		}
		if (MIXER_RECURSE == i) {
			if (pcMem == pcTop) {
				pcTop = malloc((uSize|7)+1);
				(void)strcpy(pcTop, pcMem);
			}
			uCall = uSize;
			if ((char *)0 == (pcExpr = Mixer(pcTop, &uCall, pcExpr, MIXER_END)))
				break;
			pcAdd = pcTop, uCurLen = uCall;
			i = *pcExpr++;
			continue;
		}
		if ((char *)0 == pcAdd) {
			pcAdd = pcTop, uCurLen = iLen;
		}
		if ((fTilde = ('~' == i))) {
			i = *pcExpr++;
		}
		if ('*' == i) {
			i = *pcExpr++;
			(void)strncpy(pcInplace, pcAdd, uSize);
			uCall = strlen(pcInplace);
			uSize -= uCall;
			pcInplace += uCall;
			uKeep = 0;
			continue;
		}
		iLeft = iRight = -1;
		if ('$' == i) {
			i = *pcExpr++;
			if (fTilde) {
				iLeft = 0, iRight = 1;
			} else {
				iLeft = uCurLen-1, iRight = uCurLen;
			}
		} else if (isdigit(i)) {
			iLeft = 0;
			do {
				iLeft *= 10;
				iLeft += i - '0';
				i = *pcExpr++;
			} while (isdigit(i));
			if (fTilde) {
				if (1 > (iLeft = uCurLen+1-iLeft))
					iLeft = 1;
			}
			if (iLeft > 0) {
				--iLeft;
			}
		} else {
			/* syntax error, left side */
			return (char *)0;
		}
		if ('-' == i) {
			i = *pcExpr++;
			if ((fTilde = ('~' == i))) {
				i = *pcExpr++;
			}
			if ('$' == i || '*' == i) {
				i = *pcExpr++;
				iRight = fTilde ? 1 : uCurLen;
			} else if (isdigit(i)) {
				iRight = 0;
				do {
					iRight *= 10;
					iRight += i - '0';
					i = *pcExpr++;
				} while (isdigit(i));
				if (fTilde && 1 > (iRight = uCurLen+1-iRight)) {
					iRight = 1;
				}
				if (0 == iRight) {
					iRight = 1;
				}
			} else {
				/* syntax, missing right side */
				return (char *)0;
			}
			if (iLeft >= uCurLen && iRight <= uCurLen) {
				iLeft = uCurLen-1;
			}
		}
		if (iLeft >= uCurLen) {
			/* empty request */
		} else {
			if (-1 == iRight) {
				iRight = iLeft+1;
			} else if (iRight > uCurLen) {
				iRight = uCurLen;
			}
			if (iRight <= iLeft ) {
				++iLeft, --iRight;
				while (iLeft > iRight && uSize > 0) {
					*pcInplace++ = pcAdd[--iLeft];
					--uSize;
				}
			} else {
				while (iLeft < iRight && uSize > 0) {
					*pcInplace++ = pcAdd[iLeft++];
					--uSize;
				}
			}
		}
		uKeep = 0;
		*pcInplace = '\000';
		pcAdd = (char *)0, uCurLen = iLen;
	}
	if ((char *)0 != pcAdd && 0 != uKeep) {
		(void)strncpy(pcInplace, pcAdd, uSize);
		uKeep = strlen(pcInplace);
	}
	if (pcMem != pcTop) {
		free((void *)pcTop);
	}
	free((void *)pcMem);
	if ((unsigned *)0 != puMax) {
		*puMax -= uSize-uKeep;
	}
	return pcExpr;
}

/*@Explode slicer@*/
static char acOffEnd[] = "*too*big*";
static char *pcOffEnd = acOffEnd;

/* Like the generic xapply %1, %2, ... %n dicer loop,			(ksb)
 * most programs with just a vector of elements should use this.
 * Copy characters as-is, except %N, %{N} and %[N...] (last calls the dicer),
 * allow %% to stand for %, of course.
 * Max is the max to output (in), and the size we wrote (out)
 */
char *
Slicer(char *pcTo, unsigned *puMax, char *pcDicer, char **ppcList)
{
	register int i, iIndex, fDice, fGroup, fMixer;
	register char **ppcDatum, *pcError;
	auto unsigned uSize, uCall;

	uSize = ((unsigned *)0 == puMax) ? strlen(pcTo) : *puMax;
	fGroup = '\000';
	while ('\000' != (*pcTo = *pcDicer)) {
		if (1 > uSize) {
			break;
		}
		--uSize;
		if ('%' != *pcTo) {
			++pcTo, ++pcDicer;
			continue;
		}
		if ('%' == (i = *++pcDicer)) {
			++pcTo;
			++pcDicer;
			continue;
		}
		if ((fMixer = (MIXER_RECURSE == i))) {	/* enable the mixer */
			i = *++pcDicer;
		}
		if (0 != (fDice = (DICER_START == i))) {
			fGroup = ']';
			i = *++pcDicer;
		} else if ('{'/*}*/ == i) {
			fGroup = /*{*/ '}';
			i = *++pcDicer;
		} else {
			fGroup = '\000';
		}
		/* This is where you'd expand your own markup, else
		 * we need a digit to select an input word. -- ksb
		 */
		if (!isdigit(i)) {
			return (char *)0;	/* syntax error		*/
		}
		pcError = pcDicer-1;
		iIndex = 0;
		do {
			iIndex *= 10;
			iIndex += i - '0';
			i = *++pcDicer;
			if ('\000' == fGroup)
				break;
			if (fGroup == i)
				break;
		} while (isdigit(i));

		/* find the one in the data list we need
		 */
		for (ppcDatum = ppcList; iIndex-- > 1; ++ppcDatum) {
			if ((char *)0 == *ppcDatum)
				break;
		}
		if ((char *)0 == *ppcDatum) {
			if ((char *)0 == pcOffEnd)
				return pcError;		/* beyond vector end */
			ppcDatum = & pcOffEnd;
		}
		if (fDice) {
			uCall = uSize;
			pcDicer = Dicer(pcTo, &uCall, pcDicer, *ppcDatum);
			if ((char *)0 != pcDicer && fGroup == *pcDicer)
				++pcDicer;
		} else {
			uCall = strlen(*ppcDatum);
			if (uCall >= uSize) {
				/* we could break here -- ksb? */
				return (char *)0;	/* too big	*/
			}
			(void)strcpy(pcTo, *ppcDatum);
			if ('\000' == fGroup) {
				/* don't look */;
			} else if (fGroup != *pcDicer) {
				return (char *)0; /* error: missing close */
			} else {
				++pcDicer;
			}
		}
		if ((char *)0 != pcDicer && fMixer) {
			uCall = uSize;
			pcDicer = Mixer(pcTo, &uCall, pcDicer, MIXER_END);
		}
#if DEBUG
		if (strlen(pcTo) != uCall) {
			fprintf(stderr, "debug mixer: insane length (%u != %u) [dicer source line %d]\n", strlen(pcTo), uCall, __LINE__);
			uCall = strlen(pcTo);
		}
#endif
		if ((char *)0 == pcDicer) {
			break;
		}
		pcTo += uCall;
		uSize -= uCall;
	}
	*pcTo = '\000';
	if ((unsigned *)0 != puMax) {
		*puMax -= uSize;
	}
	/* Unless we filled buffer, this is the empty string
	 */
	return pcDicer;
}

/*@Remove@*/

#if defined(DICER_TEST)
/*@Explode main@*/
#include <stdio.h>
#include <string.h>

/* test driver								(ksb)
 */
typedef struct TDnode {
	char *pctemplate;
	char *pcdata;
	char *pccorrect;
	char *pcremainder;
} TEST_DICER;

static char acWord[] = "word";
static char acEmpty[] = "";
static char acX[] = "x";

TEST_DICER aTDOK[] = {
	{" @]", "1.2.3.4   A.B.C.D  I.II.III.IV", "1.2.3.4 A.B.C.D I.II.III.IV", ""},
	{      "", acWord, acWord, acEmpty},		/* super liberal */
	{     "1", acWord, acWord, acEmpty},		/* oops case */
	{     "1", acWord, acWord, acEmpty},		/* be liberal here */
	{    "1]", acWord, acWord, acEmpty},	/* normal case */
	{   "1]x", acWord, acWord, acX},		/* have a tail */
	{  "1 2]", "remove word", acWord, acEmpty},
	{  "1 1]", "word remove", acWord, acEmpty},
	{ "1 -1]", "clip word", acWord, acEmpty},
	{ "1 -2]", "word clip", acWord, acEmpty},
	{ "1 2]x", "remove word", acWord, acX},
	{ "1 1]x", "word remove", acWord, acX},
	{"1 -1]x", "clip word", acWord, acX},
	{"1 -2]x", "word clip", acWord, acX},
	{"\\13]", "131word1wrong1", acWord, acEmpty},
	{"\\13]", "131word1wrong1", acWord, acEmpty},
	{"1.-3]",    "aa.bb.cc.dd", "aa.bb.dd", acEmpty},
	{"1.-3.-2]", "aa.bb.cc.dd", "aa.dd", acEmpty},
	{"1.-2.-2]", "aa.bb.cc.dd", "aa.dd", acEmpty},
	{"1.-2.-2", "word.bar.foo", acWord, acEmpty},
	{"1.-2.-2]x", "aa.bb.cc.dd", "aa.dd", acX},
	{"1.-3.-2]x", "aa.bb.cc.dd", "aa.dd", acX},
	{".-$].0", "128.210.11.11", "128.210.11", ".0"},
	{".-2]z", "a..c", "a.c", "z"},
	{".-4]x", "imp.npcguild.org.", "imp.npcguild.org",  "x"},
	{".-$]z", "imp.npcguild.org.", "imp.npcguild.org",  "z"},
	{"/@.2]", "1.2.3.4/A.B.C.D/I.II.III.IV", "2/B/II", ""},
	{"/@.-2]", "1.2.3.4/A.B.C.D/I.II.III.IV", "1.3.4/A.C.D/I.III.IV", ""},
	{"/@.-$].four", "1.2.3.4/A.B.C/I.II", "1.2.3/A.B/I", ".four"},
	{" @.2]", "1.2.3.4   A.B.C.D  I.II.III.IV", "2 B II", ""},
	{" @]", "1.2.3.4   A.B.C.D  I.II.III.IV", "1.2.3.4 A.B.C.D I.II.III.IV", ""},
	{" -2]", "1234", "1234", ""},
	{" -2]", "1234  ", "1234  ", ""},
	{"\\ -2]", "1234  ", "1234 ", ""},
	/* insert more above */
	{acEmpty, acEmpty, acEmpty, acEmpty}
};

static char _ac1[] = "home.example.com",
	_ac2[] =  "100",
	_ac3[] =  "%3";
static char *apcData[] = { _ac1, _ac2, _ac3, (char *)0};
TEST_DICER aTDSlice[] = {
	{     "%([1]\"\")", acEmpty, acEmpty, acEmpty},
	{     "%([1]`\')", acEmpty, acEmpty, acEmpty},
	{     "%([1])", acEmpty, _ac1, acEmpty},
	{     "%([1])", acEmpty, _ac1, acEmpty},
	{    "%([2]*)", acEmpty, "100", acEmpty},
	{   "%([2],*)", acEmpty, "100", acEmpty},
	{ "%([1],1-$)", acEmpty, _ac1, acEmpty},
	{"%([1],~100-100)", acEmpty, _ac1, acEmpty},
	{    "%([1],)", acEmpty, _ac1, acEmpty},
	{"%([1](1-$))", acEmpty, _ac1, acEmpty},
	{    "%([1]1)", acEmpty, "h", acEmpty},
	{    "%([1]$)", acEmpty, "m", acEmpty},
	{   "%([1]~1)", acEmpty, "m", acEmpty},
	{   "%([1]~$)", acEmpty, "h", acEmpty},
	{   "%([1],1)", acEmpty, "h", acEmpty},
	{   "%([1],2)", acEmpty, "o", acEmpty},
	{"%([1.1],1-$)", acEmpty, "home", acEmpty},
	{"%([1.1],1-~1)", acEmpty, "home", acEmpty},
	{ "%([1.1],$-1)", acEmpty, "emoh", acEmpty},
	{  "%([1.1],~1)", acEmpty, "e", acEmpty},
	{"%([1.1](~1-2))", acEmpty, "emo", acEmpty},
	{"%([1.1](~2-1))", acEmpty, "moh", acEmpty},
	{"%(2,2-~2)", acEmpty, "0", acEmpty},
	{"%(2,($-2)($-2))", acEmpty, "0", acEmpty},
	{"%([1.1]$)%([1.2](1,$))", acEmpty, "eee", acEmpty},
	{ "%([1.1]3,3)", acEmpty, "mm", acEmpty},
	{"%([1.2](1-4)($$))", acEmpty, "mm", acEmpty},
	{ "%([1.3],$,$)", acEmpty, "mm", acEmpty},
	{"%([1.1]3,3)%([1.2](1-4)($$))%([1.3],$,$)", acEmpty, "mmmmmm", acEmpty},
	{"(%([1.1],*,*))", acEmpty, "(homehome)", ""},
	{"(%([1.1]**))", acEmpty, "(homehome)", ""},
	{"%([2\01]1\"'\"`_\"')", acEmpty, "1'_\"", ""},
	{"%([2]1,\"-\",2-3,`-\',$)", acEmpty, "1-00-0", ""},
	{"%([2](\"'\",*,\"`\"))", acEmpty, "'100`", ""},
	{"%([2](\"'\",*,\"`\")($-1))", acEmpty, "`001'", ""},
	{"%([2](\"'\",*,\"`\")($-1))", acEmpty, "`001'", ""},
	{"%([2](`\"',*,`\"'))", acEmpty, "\"100\"", ""},
	{"%([2](`(\',*,`)\'))", acEmpty, "(100)", ""},
	{"(%([2](*,`)\'))", acEmpty, "(100)", ""},
	{"%([2](*,\"-\",*))", acEmpty, "100-100", ""},
	{"%([2](1-$,`-',$-1))", acEmpty, "100-001", ""},
	{"%([2]($-2,`1',2-$))", acEmpty, "00100", ""},
	{"%(2,0-0)", "", "1", ""},	/* this is a little liberal */
	{"%(2,8-0)", "", "001", ""},
	{"%(2,3-0)", "", "001", ""},
	{"%(2,2-0)", "", "01", ""},
	{"%(2,2-1)", "", "01", ""},
	{"%(2,4)", "", "", ""},
	{"%(2,4-5)", "", "", ""},
	{"%(2,5-3)", "", "0", ""},
	/* insert more Mixer checks above */
	{      "", acEmpty, acEmpty, acEmpty},
	{    "1964", acEmpty, "1964", acEmpty},
	{      "%1", acEmpty, _ac1, acEmpty},
	{    "%[1]", acEmpty, _ac1, acEmpty},
	{    "%{1}", acEmpty, _ac1, acEmpty},
	{    "%{2}", acEmpty, _ac2, acEmpty},
	{    "%[3]", acEmpty, _ac3, acEmpty},
	{  "%[1.1]", acEmpty, "home", acEmpty},
	{"%[1.1] %[2 $]", acEmpty, "home 100", acEmpty},
	{      "%%", acEmpty, "%", acEmpty},
	{      "%9", acEmpty, acOffEnd, ""},
	/* insert more Dicer checks above */
};

int
main(int argc, char **argv)
{
	register char *pcRes, *pcBufWrong, *pcRemWrong;
	register int i, iExit;
	auto unsigned uSize;
	auto char acWriteMe[8192];
	auto char acBuffer[8192];
	static char acFailed[] = " [failed]";

	iExit = 0;
	for (i = 0; i < sizeof(aTDOK)/sizeof(aTDOK[0]); ++i) {
		pcRemWrong = pcBufWrong = acEmpty;
		uSize = sizeof(acBuffer);
		(void)strcpy(acWriteMe, aTDOK[i].pcdata);
		pcRes = Dicer(acBuffer, &uSize, aTDOK[i].pctemplate, acWriteMe);
		if (0 != strcmp(acBuffer, aTDOK[i].pccorrect)) {
			pcBufWrong = acFailed;
		}
		if (0 != strcmp(pcRes, aTDOK[i].pcremainder)) {
			pcRemWrong = acFailed;
		}
		if (acEmpty == pcBufWrong && acEmpty == pcRemWrong) {
			continue;
		}
		printf("\"%s\" \"%%[%s\" -> \"%s\".\"%s\"\n\t\"%s\"%s\n\t\"%s\"%s\n", aTDOK[i].pcdata, aTDOK[i].pctemplate, aTDOK[i].pccorrect, aTDOK[i].pcremainder, acBuffer, pcBufWrong, pcRes, pcRemWrong);
		++iExit;
	}
	for (i = 0; i < sizeof(aTDSlice)/sizeof(aTDSlice[0]); ++i) {
		pcRemWrong = pcBufWrong = acEmpty;
		uSize = sizeof(acBuffer);
		pcRes = Slicer(acBuffer, &uSize, aTDSlice[i].pctemplate, apcData);
		if ((char *)0 == pcRes) {
			printf("slice failed for \"%s\"\n", aTDSlice[i].pctemplate);
			continue;
		}
		if (0 != strcmp(acBuffer, aTDSlice[i].pccorrect)) {
			pcBufWrong = acFailed;
		}
		if (0 != strcmp(pcRes, aTDSlice[i].pcremainder)) {
			pcRemWrong = acFailed;
		}
		if (acEmpty == pcBufWrong && acEmpty == pcRemWrong) {
			continue;
		}
		printf("Slice \"%s\" {list} -> \"%s\".\"%s\"\n got \"%s\"%s\n\t\"%s\"%s\n", aTDSlice[i].pctemplate, aTDSlice[i].pccorrect, aTDSlice[i].pcremainder, acBuffer, pcBufWrong, pcRes, pcRemWrong);
		++iExit;
	}

	/* Check for full buffer return, just once -- ksb
	 * There are other cases here, but I don't want to code the tests.
	 */
	pcRemWrong = pcBufWrong = acEmpty;
	uSize = 10;
	/* 11003322<overflow> (which leaves the "%" on the end) -- ksb
	 */
	if ((char *)0 != (pcRes = Slicer(acBuffer, &uSize, "1%23322%1", apcData)) || 0 != strcmp("11003322%", acBuffer)) {
		printf("Slicer failed the overflow test: %s %s\n", acBuffer, pcRes);
		++iExit;
	}
	exit(0 != iExit);
}
/*@Remove@*/
#endif
