# $Id: evector.m,v 1.8 2008/01/26 18:09:06 ksb Exp $
# Like distrib's elist abstraction, but later in my skills		(ksb)
#
require "util_ppm.m"

%hi
#if !defined(EVECTOR_H)
/* A vector of these represents our macro defines, they are in the
 * order defined, because m4 macros are order dependent. Last name is NULL.
 * I know a lexical sort would make updates faster, that's not reason
 * enough to break the order semanitics here -- ksb.
 * We depend on ppm from mkcmd, of course.  And we leak memory like we are
 * inside a CGI!  This is not mean for a long running application.  -- ksb
 */
typedef struct MDnode {
	char *pcname, *pcvalue;		/* name defined as value	*/
} MACRO_DEF;

extern int ScanMacros(MACRO_DEF *, int (*)(char *, char *, void *), void *);
extern void BindMacro(char *, char *);
extern MACRO_DEF *CurrentMacros();
extern char *MacroValue(char *, MACRO_DEF *);
extern void ResetMacros();
extern MACRO_DEF *MergeMacros(MACRO_DEF *, MACRO_DEF *);
#define EVECTOR_H	1
#endif
%%
%i
static char rcs_evector[] =
	"$Id: evector.m,v 1.8 2008/01/26 18:09:06 ksb Exp $";
static void MacroStart();
%%

init 4 "MacroStart();"
%c
static PPM_BUF PPMCurMacros;
static unsigned uMacroCount;

/* retsart the macro list						(ksb)
 */
static void
MacroStart()
{
	register MACRO_DEF *pMD;

	uMacroCount = 0;
	util_ppm_init(& PPMCurMacros, sizeof(MACRO_DEF), 128);
	pMD = (MACRO_DEF *)util_ppm_size(& PPMCurMacros, 128);
	pMD->pcname = (char *)0;
	pMD->pcvalue = (char *)0;
}

/* Process the list in order, return the first				(ksb)
 * nonzero result from the Call function
 */
int
ScanMacros(MACRO_DEF *pMDList, int (*pfiCall)(char *, char *, void *), void *pvData)
{
	register int iRet;

	if ((MACRO_DEF *)0 == pMDList) {
		return 0;
	}
	for (iRet = 0; (char *)0 != pMDList->pcname; ++pMDList) {
		if (0 != (iRet = pfiCall(pMDList->pcname, pMDList->pcvalue, pvData)))
			break;
	}
	return iRet;
}

/* Replace the value of the given macro in the current list		(ksb)
 */
void
BindMacro(char *pcName, char *pcValue)
{
	register unsigned i;
	register MACRO_DEF *pMD;

	pMD = util_ppm_size(& PPMCurMacros, 1);
	for (i = 0; i < uMacroCount; ++i, ++pMD) {
		if (0 == strcmp(pMD->pcname, pcName))
			break;
	}
	if (i != uMacroCount) {
		pMD->pcvalue = pcValue;
		return;
	}

	/* Resize and add the new one to the end
	 */
	pMD = util_ppm_size(& PPMCurMacros, ((uMacroCount+1)|31)+1);
	pMD[uMacroCount].pcname = pcName;
	pMD[uMacroCount].pcvalue = pcValue;
	++uMacroCount;
	pMD[uMacroCount].pcname = (char *)0;
	pMD[uMacroCount].pcvalue = (char *)0;
}

/* Save the list as it is now, don't save the undefined ones		(ksb)
 */
MACRO_DEF *
CurrentMacros()
{
	register unsigned i, iData;
	register MACRO_DEF *pMDCur, *pMDRet;

	pMDCur = util_ppm_size(& PPMCurMacros, 0);
	iData = 0;
	for (i = 0; i < uMacroCount; ++i) {
		if ((char *)0 != pMDCur[i].pcvalue)
			++iData;
	}
	if ((MACRO_DEF *)0 == (pMDRet = (MACRO_DEF *)calloc((iData|7)+1, sizeof(MACRO_DEF)))) {
		return (MACRO_DEF *)0;
	}
	iData = 0;
	for (i = 0; i < uMacroCount; ++i) {
		if ((char *)0 == pMDCur[i].pcvalue)
			continue;
		pMDRet[iData].pcname = pMDCur[i].pcname;
		pMDRet[iData].pcvalue = pMDCur[i].pcvalue;
		++iData;
	}
	pMDRet[iData].pcname = (char *)0;
	pMDRet[iData].pcvalue = (char *)0;
	return pMDRet;
}

/* Merge two macro lists, the first wins in any overlap			(ksb)
 * I know we N**2 when we could do better, but the order of the macros
 * is important to m4.dnl
 */
MACRO_DEF *
MergeMacros(MACRO_DEF *pMDLeft, MACRO_DEF *pMDRight)
{
	register MACRO_DEF *pMDCur, *pMDRet;
	register unsigned iLeft, iData, j;

	iData = 0;
	for (pMDCur = pMDLeft; (char *)0 != pMDCur->pcname; ++pMDCur) {
		++iData;
	}
	iLeft = iData;
	for (pMDCur = pMDRight; (char *)0 != pMDCur->pcname; ++pMDCur) {
		++iData;
	}
	if ((MACRO_DEF *)0 == (pMDRet = (MACRO_DEF *)calloc((iData|3)+1, sizeof(MACRO_DEF)))) {
		return (MACRO_DEF *)0;
	}
	iData = 0;
	for (pMDCur = pMDLeft; (char *)0 != pMDCur->pcname; ++pMDCur) {
		pMDRet[iData].pcname = pMDCur->pcname;
		pMDRet[iData].pcvalue = pMDCur->pcvalue;
		++iData;
	}
	for (pMDCur = pMDRight; (char *)0 != pMDCur->pcname; ++pMDCur) {
		for (j = 0; j < iLeft; ++j) {
			if (0 == strcmp(pMDRet[j].pcname, pMDCur->pcname))
				break;
		}
		if (j < iLeft)
			continue;
		pMDRet[iData].pcname = pMDCur->pcname;
		pMDRet[iData].pcvalue = pMDCur->pcvalue;
		++iData;
	}
	return pMDRet;
}

/* Reset the list of bound macros at the top of the next config.	(ksb)
 * Due to the many pointers refernecing the strings we can't free anything.
 */
void
ResetMacros()
{
	register unsigned i;
	register MACRO_DEF *pMDCur;

	pMDCur = util_ppm_size(& PPMCurMacros, 0);
	for (i = 0; i < uMacroCount; ++i) {
		pMDCur[i].pcvalue = (char *)0;
	}
}

/* Recover the value from a fixed macro list for a given name		(ksb)
 * We know the "fixed" list only has defined values, BTW.
 */
char *
MacroValue(char *pcName, MACRO_DEF *pMDList)
{
	for (/*param*/; (char *)0 != pMDList->pcname; ++pMDList) {
		if (0 == strcmp(pMDList->pcname, pcName))
			return pMDList->pcvalue;
	}
	return (char *)0;
}
%%
