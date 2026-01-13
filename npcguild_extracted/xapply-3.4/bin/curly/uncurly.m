%i
/*
 * unculry -- uncurly expand a list of parameters			(ksb)
 *
 * Copyright 1988, All Rights Reserved
 *	Kevin S Braunsdorf
 *	ksb@fedex.com
 *	Math/Sci Building, Purdue Univ
 *	West Lafayette, IN
 *
 *  `You may redistibute this code as long as you don't claim to have
 *   written it. -- ksb'
 *
 * The command
 *	$ uncurly c1.c c2.c c3.c c4.c c5.c
 * outputs
 *	c{1,2,3,4,5}.c
 *
 * So one might pipe the ouptut of a find to uncurly to compress the filenames
 * like:
 *	$ find . -type f -print | uncurly | compress > /tmp/${USER}files.Z
 *	# later on we need the list again...
 *	$ zcat /tmp/${USER}files.Z | curly | xargs do_something
 *
 * Improvments:
 *
 * This code could be mixed with other globbing code to fully emulate
 * an `arcglob' function, however this assumes the files exist in there
 * present form and is therefore less useful (to me).
 *
 * We could free more memory, if we were more carefull with our bookkeeping.
 */
%%
from '<sys/param.h>'
from '<sys/types.h>'
from '<stdio.h>'
from '"machine.h"'

basename "uncurly" ""

number {
	named "iMaxDepth"
	init "0"
	param "depth"
	help "the maximum depth to search for compression"
}

%i
static char rcsid[] =
	"$Id: uncurly.m,v 3.5 2000/02/20 19:23:00 ksb Exp $";

#if !defined(MAXPATHLEN)
#define MAXPATHLEN	1024
#endif

static char acNoMem[] = "%s: out of memory\n";
%%

%c
/* find a matching close char for the open we just ate, or (char *)0	(ksb)
 *	pc = FindMatch("test(a,b))+f(d)", '(', ')', 1);
 *			         ^ pc points here
 */
static char *
FindMatch(pcBuf, cOpen, cClose, iLevel)
register char *pcBuf;
char cOpen, cClose;
int iLevel;
{
	while ('\000' != *pcBuf) {
		if (cClose == *pcBuf) {
			--iLevel;
		} else if (cOpen == *pcBuf) {
			++iLevel;
		}
		if (0 == iLevel)
			return pcBuf;
		++pcBuf;
	}
	return (char *)0;
}

/* save a string in malloc space					(ksb)
 * we would use strdup() if everyone had it
 */
static char *
strsave(pc)
char *pc;
{
	register char *pcMem;

	pcMem = malloc((unsigned int) strlen(pc)+1);
	if ((char *)0 == pcMem) {
		fprintf(stderr, acNoMem, progname);
		exit(1);
	}
	return strcpy(pcMem, pc);
}

#define FIRST_GUESS	8192	/* initial number of input files	*/
#define NEXT_GUESS	2048	/* add this many if too few		*/

/* Joe wants us to turn a piped list of files into a big glob list	(ksb)
 * we return the number of files (he gave us) and a vector of them.
 */
unsigned int
GetFiles(pppcArgv)
char ***pppcArgv;
{
	register unsigned int uCount, uLeft;
	register char **ppcVector;
	register char *pcFile;
	auto size_t iSize;

	uLeft = FIRST_GUESS;
	ppcVector = (char **) calloc(uLeft, sizeof(char *));
	uCount = 0;
	while (NULL != (pcFile = fgetln(stdin, & iSize))) {
		if (0 == uLeft) {
			uLeft = (uCount+NEXT_GUESS) * sizeof(char *);
			ppcVector = (char **)realloc((char *)ppcVector, uLeft);
			uLeft = NEXT_GUESS;
		}
		if (0 == iSize) {
			ppcVector[uCount] = "";
		} else if ('\n' == pcFile[iSize-1]) {
			pcFile[--iSize] = '\000';
			ppcVector[uCount] = strsave(pcFile);
		} else {
			ppcVector[uCount] = malloc((iSize|3)+1);
			(void)strncpy(ppcVector[uCount], pcFile, iSize);
		}
		++uCount;
		--uLeft;
	}

	*pppcArgv = ppcVector;
	return uCount;
}

/* longest common prefix of more than one string			(ksb)
 * Note that the prefix must have balanced '{'..'}' in it.
 */
int
Prefix(n, ppcList, puiLen)
unsigned int n;
char **ppcList;
unsigned *puiLen;
{
	register int cCmp, cCur, iBal;
	auto unsigned int j, i, uArea, uLen, uSpan, uCurlen;

	*puiLen = 0;

	iBal = 0;
	for (j = 0; j < n; ++j) {
		if ('\000' == ppcList[j][0]) {
			break;
		}
	}

	/* trivial case either first or second sring is empty
	 */
	if (j < 2) {
		return 0;
	}

	uCurlen = uArea = uLen = uSpan = 0;
	while ('\000' != (cCur = ppcList[0][uCurlen])) {
		if ('{' == cCur)
			++iBal;
		else if ('}' == cCur)
			--iBal;
		for (i = 1; i < j; ++i) {
			cCmp = ppcList[i][uCurlen];
			if ('\000' == cCmp || cCur != cCmp) {
				j = i;
				break;
			}
		}
		if (1 == j) {
			break;
		}
		++uCurlen;
		if (0 == iBal && uCurlen * j > uArea) {
			uArea = uCurlen*j;
			uLen = uCurlen;
			uSpan = j;
		}
	}
	*puiLen = uLen;
	return uSpan;
}

/* longest common suffix of more than one string			(ksb)
 *  1) find the ends of all the strings
 *  2) back off until we find a non-match, but keep looking
 *  3) return the one with most characters in it
 * Note that the suffix must have balanced '{'..'}' in it.
 */
int
Suffix(n, ppcList, puiLen)
unsigned int n;
char **ppcList;
unsigned *puiLen;
{
	register char **ppcLast, *pcTemp;
	register unsigned int j, i, uCurlen;
	auto unsigned uArea, uLen, uSpan;
	auto int iStopAt;
	auto int cCur, iBal;

	*puiLen = 0;

	ppcLast = (char **)calloc(n, sizeof(char *));
	if ((char **)0 == ppcLast) {
		fprintf(stderr, acNoMem, progname);
		exit(1);
	}
	for (j = 0; j < n; ++j) {
		ppcLast[j] = strrchr(ppcList[j], '\000');
		if (ppcLast[j] == ppcList[j]) {
			break;
		}
	}

	iBal = uCurlen = uArea = uLen = uSpan = 0;
	while (ppcLast[0] != ppcList[0]) {
		cCur = ppcLast[0][-1];
		if ('{' == cCur)
			++iBal;
		else if ('}' == cCur)
			--iBal;
		iStopAt = -1;
		for (i = 0; i < j; ++i) {
			pcTemp = --ppcLast[i];
			if (cCur != pcTemp[0]) {
				j = i;
				break;
			}
			if (ppcList[i] == pcTemp && -1 == iStopAt) {
				iStopAt = i;
			}
		}
		if (1 == j) {
			break;
		}
		++uCurlen;
		if (0 == iBal && uCurlen * j > uArea) {
			uArea = uCurlen*j;
			uLen = uCurlen;
			uSpan = j;
		}
		if (-1 != iStopAt) {
			j = iStopAt;
		}
	}
	*puiLen = uLen;
	free((char *)ppcLast);
	return uSpan;
}

/* determine context for a list ppcList[0..n-1]				(ksb)
 *	left { ... } right
 *
 * If the longest common prefix will eat more character then
 * we should use that, else try the longest common suffix.
 * If both are 0 chars just return the list (0).
 */
unsigned int
Split(n, ppcList, ppcLeft, ppcRight)
unsigned int n;
char **ppcList, **ppcLeft, **ppcRight;
{
	register unsigned int i, iLcs, iLcp;
	register char *pcEnd;
	auto unsigned int iLcsLen, iLcpLen;
	auto int cKeep;

	*ppcLeft = (char *)0;
	*ppcRight = (char *)0;
	if (n == 1) {
		return 1 ;
	}

	iLcp = Prefix(n, ppcList, & iLcpLen);
	/* the `1' below will allow prefixes to loose one character
	 * (grow the file by one character, but it is better fot a human
	 * to read. -- ksb
	 */
	if (iLcp * iLcpLen <= 1 + iLcpLen) {
		iLcp = 0;
	}

	iLcs = Suffix(n, ppcList, & iLcsLen);
	if (iLcs * iLcsLen <= 2 + iLcsLen) {
		iLcs = 0;
	}

	if (iLcp * iLcpLen < iLcs * iLcsLen) {
		pcEnd = strrchr(ppcList[0], '\000') - iLcsLen;
		*ppcRight = strsave(pcEnd);
		for (i = 0; i < iLcs; ++i) {
			pcEnd = strrchr(ppcList[i], '\000') - iLcsLen;
			*pcEnd = '\000';
		}
		iLcp = Prefix(iLcs, ppcList, & iLcpLen);
		if (iLcp == iLcs) {
			pcEnd = ppcList[0] + iLcpLen;
			cKeep = *pcEnd;
			*pcEnd = '\000';
			*ppcLeft = strsave(ppcList[0]);
			*pcEnd = cKeep;
			for (i = 0; i < iLcp; ++i) {
				ppcList[i] += iLcpLen;
			}
		}
		return iLcs;
	} else if (0 != iLcpLen && 0 != iLcp) {
		pcEnd = ppcList[0] + iLcpLen;
		cKeep = *pcEnd;
		*pcEnd = '\000';
		*ppcLeft = strsave(ppcList[0]);
		*pcEnd = cKeep;
		for (i = 0; i < iLcp; ++i) {
			ppcList[i] += iLcpLen;
		}
		iLcs = Suffix(iLcp, ppcList, & iLcsLen);
		if (iLcs == iLcp) {
			pcEnd = strrchr(ppcList[0], '\000') - iLcsLen;
			*ppcRight = strsave(pcEnd);
			for (i = 0; i < iLcs; ++i) {
				pcEnd = strrchr(ppcList[i], '\000') - iLcsLen;
				*pcEnd = '\000';
			}
		}
		return iLcp;
	}
	return 0;
}

/* If there are matched curlies around a				(ksb)
 * member of the list we can remove them.
 * uLen may be (a few chars) too big, who cares?
 */
void
mcat(pcAccum, pcElement)
register char *pcAccum, *pcElement;
{
	register char *pcMatch;
	register unsigned int uLen;

	if ('{' == pcElement[0]) {
		uLen = strlen(pcElement)-1;
		pcMatch = FindMatch(pcElement, '{', '}', 0);
		if (pcMatch == & pcElement[uLen]) {
			*pcMatch = '\000';
			(void)strcat(pcAccum, pcElement+1);
			*pcMatch = '}';
		} else {
			(void)strcat(pcAccum, pcElement);
		}
	} else {
		(void)strcat(pcAccum, pcElement);
	}
}

/* The machine below found a compression, does it work for		(ksb)
 * a subsequent line?
 *
 * If works if we can factor out a (possibly zero) length prefix and
 * suffix.  This can loose (2) chars, but if it makes us look like the line
 * above another pass might compress these two lines into one.  It happens.
 */
int
Suggest(uComm, ppcComm, ppcTry, pcBundle)
unsigned int uComm;
register char **ppcComm, **ppcTry, *pcBundle;
{
	register unsigned int uScan, uPreLen;
	register unsigned int uFCom, uLimit, uPos;
	register unsigned int uLen;
	register char *pcRet;

	uLimit = strlen(ppcTry[0]) + 1;
	if (uLimit < (uLen = strlen(ppcComm[0]))) {
		return 0;
	}
	uLimit -= uLen;
	uFCom = strlen(ppcComm[0]);
	for (uPos = 0; uPos < uLimit; ++uPos) {
		if (0 != strncmp(ppcTry[0]+uPos, ppcComm[0], uFCom)) {
			continue;
		}
		for (uScan = 0; uScan < uComm; ++uScan) {
			uPreLen = strlen(ppcComm[uScan]);
			if (uScan != 0 && 0 != strncmp(ppcTry[0], ppcTry[uScan], uPos))
				break;
			if (0 != strncmp(ppcTry[uScan]+uPos, ppcComm[uScan], uPreLen))
				break;
			if (uScan != 0 && 0 != strcmp(ppcTry[0]+uPos+uFCom, ppcTry[uScan]+uPos+uPreLen))
				break;
		}
		if (uScan != uComm) {
			continue;
		}
		uLen = uPos+strlen(pcBundle)+strlen(ppcTry[0]+uPos+uFCom);

		if ((char *)0 == (pcRet = malloc(uLen+100))) {
			fprintf(stderr, acNoMem, progname);
			exit(1);
		}
		(void)strncpy(pcRet, ppcTry[0], uPos);
		pcRet[uPos] = '\000';
		(void)strcat(pcRet, pcBundle);
		(void)strcat(pcRet, ppcTry[0]+uPos+uFCom);
		ppcTry[0] = pcRet;
		return 1;
	}
	return 0;
}

/* undo what a {...} does in csh					(ksb)
 * We make passes over the list until we can make no more reductions.
 * I think this works -- that is it does as good a job as I would.
 * return the new (shorter?) list length
 */
unsigned int
UnCurly(n, ppcWhole, iDepth)
unsigned int n, iDepth;
char **ppcWhole;
{
	register unsigned int m, i;
	register char **ppcList;
	auto unsigned int uInside, uLen, uEnd, uSquish;
	auto unsigned int iDest, iOrig;
	auto char *pcLeft, *pcRight;
	auto char *pcTemp, *pcSep;
	auto char *pcBundle;

	if (0 != iMaxDepth && iDepth >= iMaxDepth) {
		return n;
	}

	ppcList = ppcWhole;
	m = n;
	while (m > 0) {
		uInside = Split(m, ppcList, & pcLeft, & pcRight);
		/* skip boring files for next pass
		 */
		if (uInside < 2) {
			--m;
			++ppcList;
			continue;
		}

		/* we have to construct
		 *	Left "{" List[0] "," List[uInside-1] "}" Right
		 */

		/* we may be able to condensed the List in to uSquish
		 * elements, rather than uInside which means that
		 * List[uSquish], ... List[uInside-1] are dead slots
		 */
		uSquish = UnCurly(uInside, ppcList, iDepth+1);

		/* We are building a long string, start with a count of
		 * close curly and "\000", tack on space for the left string,
		 * for each List element add comma (or open curly) and
		 * substring, lastly tack on the right.
		 */
		uLen = 2;
		if ((char *)0 != pcLeft) {
			uLen += strlen(pcLeft);
		}
		for (i = 0; i < uSquish; ++i) {
			uLen += 1 + strlen(ppcList[i]);
		}
		if ((char *)0 != pcRight) {
			uLen += strlen(pcRight);
		}

		/* OK now we know how big, build it
		 */
		pcTemp = malloc(uLen);
		if ((char *)0 == pcTemp) {
			fprintf(stderr, acNoMem, progname);
			exit(1);
		}

		pcTemp[0] = '\000';
		if ((char *)0 != pcLeft) {
			(void)strcat(pcTemp, pcLeft);
			free(pcLeft);
		}
		pcBundle = strrchr(pcTemp, '\000');
		if (1 == uSquish) {
			mcat(pcTemp, ppcList[0]);
		} else {
			pcSep = "{";
			for (i = 0; i < uSquish; ++i) {
				(void)strcat(pcTemp, pcSep);

				mcat(pcTemp, ppcList[i]);
				pcSep = ",";
			}
			(void)strcat(pcTemp, "}");
		}
		if ((char *)0 != pcRight) {
			(void)strcat(pcTemp, pcRight);
			free(pcRight);
		}

		iOrig = uInside;
		/* look at the lines left, package them as we
		 * are packaged, even if it doesn't save much
		 */
		while (iOrig+uSquish <= m && Suggest(uSquish, ppcList, ppcList+iOrig, pcBundle)) {
			iOrig += uSquish;
		}

		/* copy out the new bundles, orig and suggested
		 * clones
		 */
		iDest = 0;
		ppcList[iDest++] = pcTemp;
		for (i = uInside; i < iOrig; i += uSquish) {
			ppcList[iDest++] = ppcList[i];
		}

		/* compress what is left, if we can
		 */
		uEnd = UnCurly(m-iOrig, ppcList+iOrig, iDepth+1);

		/* copy in the unchanged lines
		 */
		while (0 != uEnd--) {
			ppcList[iDest++] = ppcList[iOrig++];
		}

		/* tail recurse
		 */
		m = n = iDest + (ppcList - ppcWhole);
		ppcList = ppcWhole;
	}
	return n;
}

/* do the opposite of csh(1) {...}					(ksb)
 * we cannot process files with a comma in them, but as a special
 * case we will remove ",EXT" from the end of a list of files...
 * and process those if it is the only comma in each of the files.
 *  1) output UnCulry of files with no commas
 *  2) output UnCulry of files with `,EXT' (only) on the end
 *  3) output files with random commas in them (bletch)
 *  4) loop until all files have been done
 */
static void
List(argc, argv)
unsigned int argc;
char **argv;
{
	register unsigned int i, uReplace, uCommon;
	register char *pcExt;

	if (argc == 0) {
		argc = GetFiles(& argv);
	}
	while (0 < argc) {
		for (uCommon = 0; uCommon < argc; ++uCommon) {
			if ((char *)0 != strrchr(argv[uCommon], ',')) {
				break;
			}
		}
		if (0 != uCommon) {
			uReplace = UnCurly(uCommon, argv, 0);
			argc -= uCommon;
			for (i = 0; i < uReplace; ++i) {
				puts(argv[i]);
			}
			argv += uCommon;
		}
		do {
			pcExt = (char *)0;
			for (uCommon = 0; uCommon < argc; ++uCommon) {
				register char *pcComma;
				if ((char *)0 == (pcComma = strrchr(argv[uCommon], ','))) {
					break;
				}
				if ((char *)0 == pcExt) {
					*pcComma ='\000';
					pcExt = pcComma+1;
				} else if (0 != strcmp(pcExt, pcComma+1)) {
					break;
				} else {
					*pcComma = '\000';
				}
				if ((char *)0 != strrchr(argv[uCommon], ',')) {
					*pcComma = ',';
					break;
				}
			}
			if (0 != uCommon) {
				uReplace = UnCurly(uCommon, argv, 0);
				argc -= uCommon;
				for (i = 0; i < uReplace; ++i) {
					fputs(argv[i], stdout);
					putchar(',');
					puts(pcExt);
				}
				argv += uCommon;
			}
			if (argc > 0 && (char *)0 != strrchr(argv[0], ',')) {
				puts(argv[0]);
				argc -= 1;
				argv += 1;
				uCommon = 1;
			}
		} while (0 != uCommon);
	}
}
%%

list {
	named "List"
	param "names"
	help "names to compress"
}
