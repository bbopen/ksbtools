/* $Id: type.c,v 8.25 2009/10/14 00:12:36 ksb Exp $
 * mkcmd's type abstraction
 */
#include <stdio.h>
#include <ctype.h>

#include "machine.h"
#include "type.h"
#include "main.h"
#include "option.h"
#include "parser.h"
#include "list.h"
#include "mkcmd.h"
#include "key.h"
#include "routine.h"
#include "emit.h"

extern char *progname;

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


char
	sbEmpty[] = "",
	sbIBase[] = "int",
	sbCBase[] = "char",
	sbUBase[] = "unsigned",
	sbLBase[] = "long",
	sbDBase[] = "double",
	sbVBase[] = "void",
	sbFBase[] = "FILE",
	sbF2Base[] =
#if USE_FD_T
			"fd_t",
#else
			"int /* fd_t */",
#endif
	sbCXBase[] = "%Zxb",
	sbVXBase[] = "%Zg",

	sbPInit[] = "(char *)0",
	sbVInit[] = "(void *)0",
	sbBInit[] = "0",
	sbIInit[] = "0",
	sbSInit[] = "\"\"",
	sbLInit[] = "0L",
	sbDInit[] = "0.0",
	sbEInit[] = "\'\\000\'",
	sbFInit[] = "(FILE *)0",
	sbF2Init[] = "-1",

	acBDCall[] = "%a",		/* user call for decl		*/
	acSDCall[] = "%a[%d]",		/* also used in %B, %P		*/
	acPDCall[] = "*%a",		/* %a is mkid(pOR, nil) mostly	*/
	acFDCall[] = "%a()",		/* user call for function	*/

#define acBECall acBDCall		/* user call for extern decl	*/
	acSECall[] = "%a[]",
#define acPECall acPDCall
#define acFECall acFDCall

	acBTo[] = "*%a",		/* list support			*/
	acSTo[] = "*(%a[%d])",
	acPTo[] = "**%a",

#define acBETo	acBTo			/* extern list support		*/
#define acSETo	acSTo
#define acPETo	acPTo

	sbBCopy[] = "%n = ! %i;",	/* user call strings		*/
	sbTCopy[] = "%n ^= 1;",
	sbICopy[] = "%n = atoi(%N);",
	sbPCopy[] = "%n = %N;",
	sbVCopy[] = "%n = (void *)%N;",
	sbSCopy[] = "(void)strncpy(%n, %N, sizeof(%n));",
	sbLCopy[] = "%n = atol(%N);",
	sbDCopy[] = "%n = atof(%N);",
	sb1Call[] = "%n(%w);",
	sb2Call[] = "%n(%w, %N);",
	sbAccum[] = "%n = optaccum(%n, %N, \"%qd\");",
	sbECopy[] = "%n = cvtletter(%N);",
	sbFCopy[] = "if ((FILE *)0 != %n && stdin != %n && stdout != %n) {\
(void)fclose(%n);}if (\'-\' == %N[0] && \'\\000\' == %N[1]) {\
%n = %Zg;\
}else if ((FILE *)0 == (%n = fopen(%N, \"%d\"))) {\
fprintf(stderr, \"%%s: %L: fopen: %%s: %%s\\n\", %b, %N, %E);\
exit(1);}",
	sbF2Copy[] = "if (-1 != %n && 0 != %n && 1 != %n) {\
(void)close(%n);}if (\'-\' == %N[0] && \'\\000\' == %N[1]) {\
%n = %Zg;\
}else if (-1 == (%n = open(%N, %d))) {\
fprintf(stderr, \"%%s: %L: open: %%s: %%s\\n\", %b, %N, %E);\
exit(1);}",

	sbGenFd[] = "O_RDONLY",

	sbIExt[] = "int atoi()",
	sbDExt[] = "double atof()",
	sbLExt[] = "long atol()",
	sbIAnsi[] = "int atoi(const char *)",
	sbDAnsi[] = "double atof(const char *)",
	sbLAnsi[] = "long atol(const char *)",

	sbActn[] = "%n(%N);",		/* user call strings		*/
	sbIChk[] = "u_chkint(\"%L\", %N);",
	sbDChk[] = "u_chkdbl(\"%L\", %N);",
	sbSChk[] = "u_chkstr(\"%L\", %N, sizeof(%n));",
	sbEChk[] = "u_chkletter(\"%L\", %N);",
	sbFChk[] = "u_chkfile(\"%L\", %N, \"%d\");",

	sbAccMan[] = "This option accumulates words separated by ``%d\'\'.\n",
	sbFilMan[] = "This option fopens its file with the key ``%d\'\'.\n",
	sbFdMan[] = "This option opens its file with the key ``%d\'\'.\n";

/* per-type fix-up code, generic case					(ksb)
 * return 1 or 2 for severity of the errors found
 */
static int
fixany(pOR)
OPTION *pOR;
{
	register int ilx;
	register char *pc;

	if (DIDBAKE(pOR)) {
		return 0;
	}

	if (ISLOCAL(pOR)) {
		pOR->oattr |= OPT_STATIC;
	}
	if (IS_MKEEP(pOR->pOTtype) && nil == pOR->pchkeep) {
		fprintf(stderr, "%s: %s %s requires a name and a keep", progname, pOR->pOTtype->pchlabel, usersees(pOR, nil));
		fprintf(stderr, " [a (%s", pOR->pOTtype->pchbase);
		for (pc = pOR->pOTtype->pchdecl; '\000' != *pc; ) {
			if ('%' == pc[0] && 'a' == pc[1]) {
				pc += 2;
				continue;
			}
			fputc(*pc++, stderr);
		}
		fprintf(stderr, ") and a (char*)]\n");
		return 2;
	}
	if (nil == pOR->pchinit ? IS_REQVAL(pOR->pOTtype) : 0 == strcmp(pOR->pchinit, pOR->pOTtype->pchdef)) {
		pOR->pchinit = pOR->pOTtype->pchdef;
	}
	if (nil != pOR->pchfrom && nil != pOR->pchinit) {
		if (OPT_INITCONST == ISWHATINIT(pOR)) {
			pOR->oattr &= ~OPT_INITMASK;
			pOR->oattr |= OPT_INITEXPR;
		}
	}
	if (nil == pOR->pchdesc) {
		pOR->pchdesc = pOR->pOTtype->pcharg;
	} else if ('[' == pOR->pchdesc[0]) {
		ilx = strlen(pOR->pchdesc);
		if (']' == pOR->pchdesc[ilx-1]) {
			pOR->gattr |= GEN_OPTIONAL;
			pOR->pchdesc[ilx-1] = '\000';
			++pOR->pchdesc;
		}
	}
	if (DIDOPTIONAL(pOR) && IS_EWNOPR(pOR->pOTtype) && nil == pOR->pcnoparam) {
		fprintf(stderr, "%s: %s (type %s) please trap missing parameter with a \"noparam\" attribute (conversion cannot accept a nil pointer)\n", progname, usersees(pOR, nil), pOR->pOTtype->pchlabel);
		return 2;
	}
	if (!DIDOPTIONAL(pOR) && nil != pOR->pcnoparam) {
		fprintf(stderr, "%s: %s: noparameter attribute cannot be triggered (put parameter \"%s\" in square brackets)\n", progname, usersees(pOR, nil), pOR->pchdesc);
		/* just keep going -- it is harmless */
	}

	/* the generic pointer type can be to any base type,
	 * default to the void type
	 */
	if (IS_XBASE(pOR->pOTtype)) {
		if (nil != pOR->pchgen) {
			/* leave it as the Implenmtor said */
		} else if (nil == pOR->pcdim || '\000' == pOR->pcdim[0]) {
			pOR->pchgen = "void";
		} else {
			pOR->pchgen = pOR->pcdim;
		}
		pOR->pcdim = nil;
	}
	/* if the type is used to declare a buffer/trigger we mart it
	 * in the mkcmd key for those "mkcmd_defined"
	 */
	if (pOR->pOTtype) {
		ListAdd(& pKVDeclare->LIinputs, pOR->pOTtype->pchlabel);
	}
	/* if the type is checked/verified we mark that as well
	 */
	if (IS_VUSED(pOR->pOTtype)) {
		ListAdd(& pKVVerify->LIinputs, pOR->pOTtype->pchlabel);
	}

	pOR->gattr |= GEN_BAKED;
	return 0;
}

/* boolean, toggle, action all don't have parameter slots		(ksb)
 * (warn them but do not abort the construction because it is a nit).
 */
static int
fixnoparam(pOR)
OPTION *pOR;
{
	register int iRet;

	iRet = fixany(pOR);
	/* the control "after" is a action but it's "parameter"
	 * can be forced with "%A" -- ksb
	 */
	if (ISSACT(pOR) && 'a' == pOR->chname) {
		return iRet;
	}
	if (nil != pOR->pchdesc && '\000' != pOR->pchdesc[0]) {
		fprintf(stderr, "%s: %s: %s: parameter name \"%s\" can never be seen\n", progname, pOR->pOTtype->pchlabel, usersees(pOR, nil), pOR->pchdesc);
		pOR->pchdesc = "";
	}
	if (nil != pOR->pcnoparam) {
		fprintf(stderr, "%s: %s: %s: optional parameter cannot happen\n", progname, pOR->pOTtype->pchlabel, usersees(pOR, nil));
		/* keep it */
	}
	return iRet;
}

/* file a file option so we know it is `valid'				(ksb)
 */
static int
fixfile(pOR)
OPTION *pOR;
{
	if (0 != fixany(pOR)) {
		exit(MER_SEMANTIC);
	}
	ListAdd(& LISysIncl, "<errno.h>");
	switch (pOR->pcdim[0]) {
	default:
		fprintf(stderr, "%s: file %s: unknown file type `%s' only [rwa]\n", progname, usersees(pOR, nil), pOR->pcdim);
		return 2;
	case 'r':
		pOR->pchgen = "stdin";
		break;
	case 'a':
	case 'w':
		pOR->pchgen = "stdout";
		break;
	}
	return 0;
}

/* file an fd option so we know it is `valid'				(ksb)
 * If he used O_??? look for WR, else look for 01 or 02 to set value for ``-''.
 * Then add a mode if we have too.
 */
static int
fixfd(pOR)
OPTION *pOR;
{
	register char *pcWr;

	if (0 != fixany(pOR)) {
		exit(MER_SEMANTIC);
	}
	ListAdd(& LISysIncl, "<errno.h>");
	ListAdd(& LISysIncl, "<fcntl.h>");
	pOR->pchgen = "0";
	if (isdigit(pOR->pcdim[0])) {
		switch (3 & atoi(pOR->pcdim)) {
		case 1:
		case 2:
			pOR->pchgen = "1";
		default:
			break;
		}
	} else {
		for (pcWr = strchr(pOR->pcdim, 'W'); nil != pcWr && '\000' != *pcWr; pcWr = strchr(pcWr+1, 'W')) {
			if ('R' == pcWr[1]) {	/* O_WRONLY or O_RDWR */
				pOR->pchgen = "1";
			}
		}
	}
	/* if the user didn't include a mode we have to append one
	 */
	if (nil == strchr(pOR->pcdim, ',')) {
		register char *pcMem;
		static char acDefMode[] = ", 0666";

		if (nil == (pcMem = malloc(((strlen(pOR->pcdim)+strlen(acDefMode))|7)+1))) {
			fprintf(stderr, acOutMem, progname);
			exit(MER_LIMIT);
		}
		pOR->pcdim = strcpy(pcMem, pOR->pcdim);
		(void)strcat(pcMem, acDefMode);
	}
	return 0;
}

/* fix a string so we know it has a dim					(ksb)
 */
static int
fixstr(pOR)
OPTION *pOR;
{
	(void)fixany(pOR);
	if (nil == pOR->pchfrom && '\000' == pOR->pcdim[0] && sbSInit == pOR->pchinit) {
		fprintf(stderr, "%s: %s: string has unknown dimension\n", progname, usersees(pOR, nil));
		return 2;
	}
	return 0;
}

static int SynthFix(/* pOR, pOTType */);

/* do all the work for synthetic (user defined) types			(ksb)
 */
static int
fixsynth(pOR)
OPTION *pOR;
{
	register int iRet;

	if (0 != (iRet = SynthFix(pOR, pOR->pOTtype)))
		return iRet;
	return fixany(pOR);
}

static char acPD[] = "Made Public Domain 1987-1998, by Kevin S Braunsdorf";

OPTTYPE OTAlias =
	{'@',  0,	    nil,     nil, acPD,  nil,
		nil,    nil,	nil, nil,   nil,
		nil,	nil,	nil, nil,   nil,	fixany,
		"alias", "an alias for option %n"};

/* N.B. all type must be in the EMIT HELP map below to
 * be visible to TypeEsc
 */
OPTTYPE aOTTypes[] = {
	{'+', _EWLATE|_ACTIVE|_ARG|_RTN_PC, sbCBase, sbCXBase, acPDCall, acPECall,
		acPTo, acPETo, sbAccum, sbAccum, sbPInit,
		"name",  nil,   nil,   nil,     sbAccMan, fixany,
		"accumulate", "concatenate words on %n"},
	{'s', _ARG|_REQVAL|_EWNOPR|_RTN_PC, sbCBase, sbCXBase, acSDCall, acSECall,
		acSTo, acSETo, sbSCopy, sbSCopy, sbSInit,
		"string", nil,   nil,   sbSChk, nil,      fixstr,
		"string", "fill the %n buffer"},
	{'p', _ARG|_REQVAL|_RTN_PC, sbCBase, sbCXBase, acPDCall, acPECall,
		acPTo, acPETo, sbPCopy, sbPCopy, sbPInit,
		"string", nil,   nil,   nil,   nil,      fixany,
		"char*", "provide a new string for %n"},
	{'b', _ARG|_REQVAL|_RTN_VD, sbIBase, sbCXBase, acBDCall, acBECall,
		acBTo, acBETo, sbBCopy, nil,    sbBInit,
		sbEmpty, nil,    nil,    nil,   nil,      fixnoparam,
		"boolean", "set the %n flag"},
	{'t', _EWLATE|_ARG|_REQVAL|_RTN_VD, sbIBase, sbCXBase, acBDCall, acBECall,
		acBTo, acBETo, sbTCopy, nil,    sbBInit,
		sbEmpty, nil,    nil,    nil,   nil,      fixnoparam,
		"toggle",  "toggle the %n flag"},
	{'c', _ARG|_REQVAL|_EWNOPR, sbIBase, sbCXBase, acBDCall, acBECall,
		acBTo, acBETo, sbECopy, sbECopy, sbEInit,
		"letter", nil,    nil,    sbEChk, nil,      fixany,
		"letter", "provide a new letter for %n"},
	{'i', _ARG|_REQVAL|_EWNOPR, sbIBase, sbCXBase, acBDCall, acBECall,
		acBTo, acBETo, sbICopy, sbICopy, sbIInit,
		sbIBase, sbIExt, sbIAnsi, sbIChk, nil,      fixany,
		"integer", "provide an integer for %n"},
	{'#', _ARG|_REQVAL|_EWNOPR, sbIBase, sbCXBase, acBDCall, acBECall,
		acBTo, acBETo, sbICopy, sbICopy, sbIInit,
		sbIBase, sbIExt, sbIAnsi, sbIChk, nil,	    fixany,
		"number", "provide a new value for %n"},
	{'D', _ACTIVE|_ARG|_EWNOPR|_MKEEP|_RTN_FD, sbF2Base, sbCXBase, acBDCall, acBECall,
		acBTo, acBETo, sbF2Copy, sbF2Copy, sbF2Init,
		"file",   nil,   nil,   sbFChk,  sbFdMan, fixfd,
		"fd", "open a new file for %n"},
	{'u', _ARG|_REQVAL|_EWNOPR, sbUBase, sbCXBase, acBDCall, acBECall,
		acBTo, acBETo, sbICopy, sbICopy, sbIInit,
		sbUBase, sbIExt, sbIAnsi, sbIChk, nil,      fixany,
		"unsigned", "provide an unsigned integer for %n"},
	{'l', _ARG|_REQVAL|_EWNOPR, sbLBase, sbCXBase, acBDCall, acBECall,
		acBTo, acBETo, sbLCopy, sbLCopy, sbLInit,
		sbIBase, sbLExt, sbLAnsi, sbIChk, nil,      fixany,
		"long", "provide a long integer for %n"},
	{'d', _ARG|_REQVAL|_EWNOPR, sbDBase, sbCXBase, acBDCall, acBECall,
		acBTo, acBETo, sbDCopy, sbDCopy, sbDInit,
		"float", sbDExt, sbDAnsi, sbDChk, nil,      fixany,
		"double", "provide a float value for %n"},
	{'f', _ACT|_XBASE|_RTN_PC, nil, sbVXBase, nil,  acFDCall, acFECall,
		nil, sb2Call, sbActn, nil,
		"arg",   nil,    nil,    nil,    nil,      fixany,
		"function", "feed %n() a new word"},
	{'a', _ACT|_XBASE|_RTN_VD, nil, sbVXBase, nil,   acFDCall, acFECall,
		nil, sb1Call, nil,     sbBInit,
		sbEmpty, nil,	nil,	nil,     nil,      fixnoparam,
		"action", "call %n()"},
	{'F', _ACTIVE|_ARG|_EWNOPR|_MKEEP|_RTN_FP, sbFBase, sbCXBase, acPDCall, acPECall,
		acPTo, acPETo, sbFCopy, sbFCopy, sbFInit,
		"file", nil,   nil,   sbFChk,  sbFilMan, fixfile,
		"file", "open a new file for %n"},
	{'v', _ARG|_REQVAL|_XBASE|_RTN_PC, sbVBase, sbVXBase, acPDCall, acPECall,
		acPTo, acPETo, sbVCopy, sbVCopy, sbVInit,
		"word", nil,   nil,   nil,   nil,      fixany,
		"pointer", "provide a new word for %n"},
	{'\000', 0, 0, "ksb",
	    "$Id: type.c,v 8.25 2009/10/14 00:12:36 ksb Exp $"}
};

OINIT aOIBases[] = {
	/* name, ckey, cvt'd, init .. */
	{"string", '+', sbSInit, OPT_GLOBAL|OPT_SYNTHTYPE},
	{"char*", 's', sbPInit, OPT_GLOBAL|OPT_SYNTHTYPE},
	{"boolean", 'b', sbBInit, OPT_GLOBAL|OPT_SYNTHTYPE},
	{"toggle", 't', sbBInit, OPT_GLOBAL|OPT_SYNTHTYPE},
	{"letter", 'c', sbEInit, OPT_GLOBAL|OPT_SYNTHTYPE},
	{"integer", 'i', sbIInit, OPT_GLOBAL|OPT_SYNTHTYPE},
	{"number", '#', sbIInit, OPT_GLOBAL|OPT_SYNTHTYPE},
	{"file", 'D', sbFInit, OPT_GLOBAL|OPT_SYNTHTYPE},
	{"unsigned", 'u', sbIInit, OPT_GLOBAL|OPT_SYNTHTYPE},
	{"long", 'l', sbLInit, OPT_GLOBAL|OPT_SYNTHTYPE},
	{"double", 'd', sbDInit, OPT_GLOBAL|OPT_SYNTHTYPE},
	{"function", 'f', nil, OPT_GLOBAL|OPT_SYNTHTYPE},
	{"action", 'a', nil, OPT_GLOBAL|OPT_SYNTHTYPE},
	{"fd", 'F', sbF2Init, OPT_GLOBAL|OPT_SYNTHTYPE},
	{"pointer", 'v', sbVInit, OPT_GLOBAL|OPT_SYNTHTYPE},
	{(char *)0, '\000', (char *)0, 0}
};

static EMIT_MAP aERBase[] = {	/* base type map */
	{'#', "number", "the special type for -number"},
	{'+', "accumulate", "collect all words specified to convert later"},
	{'a', "action", "a C function with no command line parameter"},
	{'b', "boolean", "a simple switch"},
	{'c', "letter", "description of a single charater"},
	{'d', "double", "floating point number"},
	{'D', "fd", "a file descriptor"},
	{'f', "function", "process a command line parameter with C code"},
	{'F', "file", "file pointer stream (as from stdio.h)"},
	{'i', "int", "a signed integer"},
	{'l', "long", "a long signed integer"},
	{'p', "char*", "unconverted string"},
	{'s', "string", "a fixed length string"},
	{'t', "toggle", "a flip switch on, off, on, off, on"},
	{'u', "unsigned", "an unsigned number"},
	{'v', "void*", "generic pointer base"},
	{'v', "pointer", (char *)0},
	{'z', "error", (char *)0}
};

EMIT_HELP EHBase = {
	"base types", "%y",
	sizeof(aERBase)/sizeof(EMIT_MAP),
	aERBase,
	(char *)0,
	(EMIT_HELP *)0
};

/* extract a type represenation from a string				(ksb)
 */
OPTTYPE *
TypeParse(ppcCursor)
char **ppcCursor;
{
	register OPTTYPE *pOT;
	register int cCmd;
	auto int iBad;

	switch (cCmd = EscCvt(& EHBase, ppcCursor, & iBad)) {
	case 0:
		fprintf(stderr, "%s: number (%d): conext requires a type\n", progname, iBad);
		return (OPTTYPE *)0;
	case -1:
		fprintf(stderr, "%s: %c: type representation, use '-E base' for help\n", progname, iBad);
		return (OPTTYPE *)0;
	case -2:
		fprintf(stderr, "%s: %*.*s: unknown long type representation, use '-E base' for help\n", progname, iBad, iBad, *ppcCursor);
		return (OPTTYPE *)0;
	default:
		break;
	}
	if ((OPTTYPE *)0 == (pOT = CvtType(cCmd))) {
		/* this should never happen, BTW
		 */
		fprintf(stderr, "%s: base level: %%%c: unknown type\n", progname, cCmd);
		return (OPTTYPE *)0;
	}
	return pOT;
}

/* mkcmd 8.0 lets use get into building "derived types" from the	(ksb)
 * primative mkcmd base tpyes
 */
#define SYNTH_TYPE_BASE		0x1000
static OPTION **ppORSynth;		/* index of all synthetic types */
static int iCurType;
static OPTTYPE *
SynthType(iIndex)
int iIndex;
{
	if (iIndex < iCurType) {
		return ppORSynth[iIndex]->pOTtype;
	}
	fprintf(stderr, "%s: type table undeflow\n", progname);
	exit(MER_INV);
}


static EMIT_MAP aERType[] = {	/* type expander maps */
	{'a', "declarator", "extern declarator"},
	{'A', "pdeclarator", "declarator for a pointer to this type"},
	{'b', "base", "C base type"},
	{'B', "xbase", "C base type -- expand WRT this trigger only"},
	{'d', "define", "definition for this type"},
	{'D', "pdefine", "defininition for a pointer to this type"},
	{'e', "every", "default every conversion"},
	{'h', "help", "yucky default help text"},
	{'i', "initial", "default initial value"},
	{'k', "key", "key letter used to describe this type"},
	{'l', "label", "English label for type"},
	{'m', "manual", "manual page note on type"},
	{'p', "parameter", "describe type in command line usage"},
	{'s', "subtype", "expand WRT subtype of this user type..."},
	{'S', "subtypes", "expand WRT all subtypes..."},
	{'u', "update", "default conversion code"},
	{'v', "verify", "C code to check for valid data"},
	{'x', "extern", "C declaration of type conversion function"},
	{'Z', "synth", "look at synthetic type attributes"},
	{'z', "error", (char *)0},
};

EMIT_HELP EHType = {
	"type expander", "%x",
	sizeof(aERType)/sizeof(EMIT_MAP),
	aERType,
	(char *)0,
	(EMIT_HELP *)0
};

/* get an attribute from the type part of an option			(ksb)
 */
int
TypeEsc(ppcCursor, pOT, pEO, pcDst)
char **ppcCursor;
OPTTYPE *pOT;
EMIT_OPTS *pEO;
char *pcDst;
{
	register int cWhich, iRet;
	auto int iNumber;
	auto char *pcStart;

	pcStart = *ppcCursor;
	switch (cWhich = EscCvt(& EHType, ppcCursor, & iNumber)) {
	case 'a':
		(void)strcpy(pcDst, pOT->pchextn);
		break;
	case 'A':
		(void)strcpy(pcDst, pOT->pchextn2);
		break;
	case 'b':
		(void)strcpy(pcDst, pOT->pchbase);
		break;
	case 'B':
		(void)strcpy(pcDst, pOT->pchxbase);
		break;
	case 'd':
		(void)strcpy(pcDst, pOT->pchdecl);
		break;
	case 'D':
		(void)strcpy(pcDst, pOT->pchdecl2);
		break;
	case 'e':
		(void)strcpy(pcDst, pOT->pchevery);
		break;
	case 'h':
		(void)strcpy(pcDst, pOT->pchhelp);
		break;
	case 'i':
		(void)strcpy(pcDst, pOT->pchdef);
		break;
	case 'k':
		*pcDst++ = pOT->chkey;
		*pcDst = '\000';
		break;
	case 'l':
		(void)strcpy(pcDst, pOT->pchlabel);
		break;
	case 'm':
		(void)strcpy(pcDst, pOT->pchmannote);
		break;
	case 'p':
		(void)strcpy(pcDst, pOT->pcharg);
		break;
	case 's':
		if (nilOT == pOT->pOTsbase) {
			fprintf(stderr, "%s: %s has no sub-type\n", progname, pOT->pchlabel);
			return 1;
		}
		return TypeEsc(ppcCursor, pOT->pOTsbase, pEO, pcDst);
	case 'S':
		if (nilOT == pOT->pOTsbase) {
			return TypeEsc(ppcCursor, pOT, pEO, pcDst);
		}
		iRet = TypeEsc(& pcStart, pOT->pOTsbase, pEO, pcDst);
		pcDst += strlen(pcDst);
		iRet |= TypeEsc(ppcCursor, pOT, pEO, pcDst);
		return iRet;
	case 'u':
		(void)strcpy(pcDst, pOT->pchupdate);
		break;
	case 'v':
		(void)strcpy(pcDst, pOT->pchchk);
		break;
	case 'x':
		if (fAnsi) {
			strcpy(pcDst, pOT->pchaext);
		} else {
			strcpy(pcDst, pOT->pchcext);
		}
		break;
	case 'Z':	/* base into the trigger for the type */
		if (pOT->chkey < SYNTH_TYPE_BASE) {
			fprintf(stderr, "%s: %s: not a synthetic type\n", progname, pOT->pchlabel);
			return 1;
		}
		return OptEsc(ppcCursor, ppORSynth[pOT->chkey-SYNTH_TYPE_BASE], pEO, pcDst);
	case 0:
		fprintf(stderr, "%s: type expander doesn't know what to do with a number (%d)\n", progname, iNumber);
		return 1;
	case -1:
		fprintf(stderr, "%s: %c: unknown percent escape, use '-E type' for help\n", progname, iNumber);
		return 1;
	case -2:
		fprintf(stderr, "%s: %*.*s: unknown long escape, use '-E type' for help\n", progname, iNumber, iNumber, *ppcCursor);
		return 1;
	default:
	case 'z':
		CSpell(pcDst, cWhich, 0);
		fprintf(stderr, "%s: type level: %%%s: expander botch\n", progname, pcDst);
		iExit |= MER_PAIN;
		return 1;
	}
	return 0;
}

/* do the bookkeeping for a new synthetic type				(ksb)
 */
OPTTYPE *
SynthNew(pORType, pOTBase, pcName)
OPTION *pORType;
OPTTYPE *pOTBase;
char *pcName;
{
	register OPTTYPE *pOTRet;
	static int iMaxType;

	if ((OPTTYPE *)0 == (pOTRet = (OPTTYPE *)malloc(sizeof(OPTTYPE)))) {
		fprintf(stderr, acOutMem, progname);
		exit(MER_LIMIT);
	}
	if (iCurType+1 >= iMaxType) {
		iMaxType += 16;
		if ((OPTION **)0 == ppORSynth) {
			ppORSynth = (OPTION **)calloc(iMaxType, sizeof(OPTION*));
		} else {
			ppORSynth = (OPTION **)realloc((void *)ppORSynth, iMaxType * sizeof(OPTION *));
		}
		if ((OPTION **)0 == ppORSynth) {
			fprintf(stderr, acOutMem, progname);
			exit(MER_LIMIT);
		}
	}
	ppORSynth[iCurType] = pORType;
	pORType->pchname = pcName;
	pORType->pOTtype = pOTRet;
	pOTRet->pchbase = nil;
	pOTRet->pchxbase = nil;
	pOTRet->pchdecl = nil;
	pOTRet->pchextn = nil;
	pOTRet->pchdecl2 = nil;
	pOTRet->pchextn2 = nil;
	pOTRet->pchevery = nil;
	pOTRet->pchcext = nil;
	pOTRet->pchaext = nil;
	pOTRet->pchdef = nil;
	pOTRet->pchupdate = nil;
	pOTRet->pcharg = nil;
	pOTRet->pchchk = nil;
	pOTRet->pORname = nilOR;
	pOTRet->chkey = SYNTH_TYPE_BASE+iCurType++;
	pOTRet->tattr = _SYNTH;
	pOTRet->pcharg = "unresolved synthetic type";
	pOTRet->pchmannote = (char *)0;
	pOTRet->pfifix = fixsynth;
	pOTRet->pchlabel = "synthetic";
	pOTRet->pchhelp = "synthetic type";
	pOTRet->pOTsbase = pOTBase;
	return pOTRet;
}

/* convert a cmd type to an internal type				(ksb)
 */
OPTTYPE *
CvtType(ch)
int ch;
{
	register OPTTYPE *pOT;

	if ('\000' == ch)
		ch = 'b';

	if (ch >= SYNTH_TYPE_BASE) {
		return SynthType(ch - SYNTH_TYPE_BASE);
	}
	for (pOT = aOTTypes; pOT->chkey != '\000'; ++pOT) {
		if (ch == pOT->chkey)
			return pOT;
	}
	return nilOT;
}

/* Copy the base type parts up into the blank type node, then backfill	(ksb)
 * the filed from the option record's fields
 */
int
SynthResolve(pOTSet)
OPTTYPE *pOTSet;
{
	register OPTION *pORParams;
	register OPTTYPE *pOTBase;
	register int fOpaque;
	register ATTR wRtn;

	if (pOTSet->chkey < SYNTH_TYPE_BASE) {
		fprintf(stderr, "%s: synthetic type inv. broken\n", progname);
		exit(MER_INV);
	}
	pORParams = ppORSynth[pOTSet->chkey-SYNTH_TYPE_BASE];
	if (0 != (pOTSet->tattr & _SYNTHX)) {
		fprintf(stderr, "%s: %s: user defined type resolution failed\n", progname, usersees(pORParams, nil));
		return 1;
	}
	pOTSet->tattr |= _SYNTHX;
	/* we never found the definition at all, check file system
	 */
	if (nilOT == (pOTBase = pOTSet->pOTsbase) || ISAUGMENT(pORParams)) {
		if (0 != TypeRequire(pORParams->pchname)) {
			fprintf(stderr, "%s: type %s: not found\n", progname, pORParams->pchname);
			return 1;
		}
		if (nilOT == (pOTBase = pOTSet->pOTsbase)) {
			fprintf(stderr, "%s: %s.m does not define the \"%s\" type\n", progname, pORParams->pchname, pORParams->pchname);
			return 1;
		}
		pORParams->oattr &= ~OPT_AUGMENT;
	}
	if (IS_SYNTH(pOTBase) && 0 != SynthResolve(pOTBase)) {
		fprintf(stderr, "%s: type %s: recursive resolution failed\n", progname, pORParams->pchname);
		return 1;
	}

	/* things to leave alone
	 */
	fOpaque = IS_OPAQUE(pOTSet);
	/* pOTSet->pfifix */

	/* convert the type into a new "real" type -- ksb
	 * first copy all the manditory junk up from the base type
	 */
	wRtn = ((IS_NO_RTN(pOTSet) ? pOTBase : pOTSet)->tattr & RTN_MASK) |
		(pOTBase->tattr & ~(RTN_MASK));
	pOTSet->tattr &= _ACTIVE|_MKEEP|_VUSED;
	pOTSet->tattr |= wRtn;

	pOTSet->pchbase = pOTBase->pchbase;
	pOTSet->pchxbase = pOTBase->pchxbase;
	pOTSet->pchdecl = pOTBase->pchdecl;
	pOTSet->pchextn = pOTBase->pchextn;
	pOTSet->pchdecl2 = pOTBase->pchdecl2;
	pOTSet->pchextn2 = pOTBase->pchextn2;
	pOTSet->pchevery = pOTBase->pchevery;
	pOTSet->pchcext = pOTBase->pchcext;
	pOTSet->pchaext = pOTBase->pchaext;

	/* then copy what might be overridden in the type option record
	 */
	pOTSet->pchmannote = (nil != pORParams->pctmannote) ? pORParams->pctmannote : pOTBase->pchmannote;
	pOTSet->pchupdate = (nil != pORParams->pctupdate) ? pORParams->pctupdate : pOTBase->pchupdate;
	pOTSet->pchevery = (nil != pORParams->pctevery) ? pORParams->pctevery : (nil != pOTSet->pchupdate) ? pOTSet->pchupdate : pOTBase->pchevery;
	pOTSet->pchdef = (nil != pORParams->pctdef) ? pORParams->pctdef : pOTBase->pchdef;
	pOTSet->pchhelp = (nil != pORParams->pcthelp) ? pORParams->pcthelp : pOTBase->pchhelp;
	pOTSet->pcharg = (nil != pORParams->pctarg) ? pORParams->pctarg : pOTBase->pcharg;
	pOTSet->pchchk = (nil != pORParams->pctchk) ? pORParams->pctchk : pOTBase->pchchk;

	/* must be from type record
	 */
	pOTSet->pchlabel = pORParams->pchname;

	pOTSet->tattr &= ~_SYNTHX;
	if (fOpaque) {
		/* This forces us to move some code into fixany and base it
		 * on the type attributes. -- ksb
		 */
		pOTSet->pOTsbase = nilOT;
	}
	return 0;
}

/* copy non-nil attributes from the type option record onto the		(ksb)
 * client option record, then call down to the sbase type
 * types cannot change how we are scaned (no exclude, end, abort)
 * just how we are converted
 *
 * If the type has a client key list add the option to the list
 * as "%r_" or "%R_"
 */
static int
SynthFix(pOR, pOTType)
OPTION *pOR;
OPTTYPE *pOTType;
{
	register OPTION *pORType;

	pORType = ppORSynth[pOTType->chkey-SYNTH_TYPE_BASE];
#if DEBUG
	fprintf(stderr, "%s: %s <=", progname, usersees(pOR, nil));
	fprintf(stderr, " %s\n", usersees(pORType, nil));
#endif

	/* What to add to the client list key so we can find this again?
	 * (This is used in a double eval'd key, e.g. "%an", or used as
	 * implicit context in the meta expander code for FOREACH)
	 */
	if ((KEY *)0 != pORType->pKVclient) {
		auto char acPtr[1024];	/* 2*509 + slop */
		register char *pcTail;

		pcTail = acPtr;
		*pcTail++ = '%';
		if (nilOR != pOR->pORallow) {
			*pcTail++ = (ISSACT(pOR->pORallow)) ? 'R' : 'r';
			*pcTail++ = pOR->pORallow->chname;
		}
		if (ISSACT(pOR)) {
			sprintf(pcTail, "R%c", pOR->chname);
		} else if (ISPPARAM(pOR)) {
			sprintf(pcTail, "r<%s>", pOR->pchname);
		} else if (ISVARIABLE(pOR)) {
			sprintf(pcTail, "r<%s>", pOR->pchname);
		} else {
			sprintf(pcTail, "r%c", pOR->chname);
		}
		ListAdd(& pORType->pKVclient->LIinputs, strdup(acPtr));
		pORType->pKVclient->wkattr |= KV_CLIENT;
	}

	/* do the copy!
	 * we don't copy named/keep/track
	 */
	pOR->oattr |= (OPT_VERIFY|OPT_TRACK|OPT_TRGLOBAL|OPT_LATE|OPT_PTR2)&pORType->oattr;
	if (nil != pORType->pchdesc && nil == pOR->pchdesc) {
		pOR->pchdesc = pORType->pchdesc;
	}
	if (nil != pORType->pchverb && nil == pOR->pchverb) {
		pOR->pchverb = pORType->pchverb;
	}
	if (nil != pORType->pchinit && nil == pOR->pchinit) {
		pOR->pchinit = pORType->pchinit;
		pOR->oattr &= ~OPT_INITMASK;
		pOR->oattr |= OPT_INITMASK & pORType->oattr;
	}
	if (nil != pORType->pcnoparam && nil == pOR->pcnoparam) {
		pOR->pcnoparam = pORType->pcnoparam;
	}
	if (nil != pORType->pcdim && nil == pOR->pcdim) {
		pOR->pcdim = pORType->pcdim;
	}
	if (nil != pORType->pchuupdate && nil == pOR->pchuupdate) {
		pOR->pchuupdate = pORType->pchuupdate;
	}
	if (nil != pORType->pchuser && nil == pOR->pchuser) {
		pOR->pchuser = pORType->pchuser;
	}
	if (nil != pORType->pchuser && nil == pOR->pchuser) {
		pOR->pchuser = pORType->pchuser;
	}
	if (nil != pORType->pchverify && nil == pOR->pchverify) {
		pOR->pchverify = pORType->pchverify;
	}
	if (nil != pORType->pchbefore && nil == pOR->pchbefore) {
		pOR->pchbefore = pORType->pchbefore;
	}
	if (nil != pORType->pchafter && nil == pOR->pchafter) {
		pOR->pchafter = pORType->pchafter;
	}
	if ((ROUTINE *)0 != pORType->pRG && (ROUTINE *)0 == pOR->pRG) {
		/* XXX ? does this do the right thing? -- ksb */
		pOR->pRG = pORType->pRG;
	}
	/* if we need an API key fake one init'd from type
	 */
	if ((KEY *)0 != pORType->pKV && (KEY *)0 == pOR->pKV) {
		register KEY *pKVNew;

		pKVNew = KeyAttach(KV_OPTION, pOR, nil, KEY_NOSHORT, & pOR->pKV);
		pKVNew->pcowner = pORType->pchname;
		pKVNew->pcversion = pORType->pKV->pcversion;
		pKVNew->uoline = pORType->pKV->uoline;
		pKVNew->pKVinit = pORType->pKV;
		pKVNew->wkattr |= KV_SYNTH|(KV_V_ONCE & pORType->pKV->wkattr);
	} else if ((KEY *)0 != pORType->pKV && (KEY *)0 == pOR->pKV->pKVinit) {
		pOR->pKV->pKVinit = pORType->pKV;
	}
	if ((KEY *)0 != pORType->pKV && (char *)0 != pORType->pKV->pcversion && (char *)0 != pOR->pKV->pcversion && atoi(pORType->pKV->pcversion) != atoi(pOR->pKV->pcversion)) {
		fprintf(stderr, "%s: %s: API key version missmatch with type (%s != %s)\n", progname, usersees(pOR, (char *)0), pOR->pKV->pcversion, pORType->pKV->pcversion);
		iExit |= MER_BROKEN;
	}
	if ((OPTTYPE *)0 == pOTType->pOTsbase) {
		return fixany(pOR);
	}
	if (fixsynth == pOTType->pOTsbase->pfifix) {
		return SynthFix(pOR, pOTType->pOTsbase);
	}
	if ((int (*)())0 != pOTType->pOTsbase->pfifix) {
		return (*pOTType->pOTsbase->pfifix)(pOR);
	}
	return 0;
}

/* make the type specific extern decls, this is a botch			(ksb)
 */
void
mktypeext(fp)
FILE *fp;
{
	register OPTTYPE *pOT;

	for (pOT = aOTTypes; '\000' != pOT->chkey; ++pOT) {
		/* we fAnsi here now */
		if (!IS_USED(pOT)) {
			continue;
		}
		if (fAnsi && nil != pOT->pchaext) {
			Emit(fp, "extern %a;", nilOR, pOT->pchaext, 1);
		}
		if (!fAnsi && nil != pOT->pchcext) {
			Emit(fp, "extern %a;", nilOR, pOT->pchcext, 1);
		}
		pOT->tattr &= ~_USED;
	}
}

/* dump the type table to a file					(ksb)
 *	Type:update:default value:keyword:C-type
 */
void
DumpTypes(fp)
FILE *fp;
{
	register OPTTYPE *pOT;
	register int w1 = 1, w2 = 1, w3 = 1, w4 = 1;
	register int l;

	for (pOT = aOTTypes; pOT->chkey != '\000'; ++pOT) {
		if ((char *)0 != pOT->pchlabel && (l = strlen(pOT->pchlabel)) > w1 && l < 40)
			w1 = l;

		if ((char *)0 != pOT->pchupdate && (l = strlen(pOT->pchupdate)) > w2 && l < 40)
			w2 = l;

		if ((char *)0 != pOT->pchdef && (l = strlen(pOT->pchdef)) > w3 && l < 40)
			w3 = l;

		if ((char *)0 != pOT->pcharg && (l = strlen(pOT->pcharg)) > w4 && l < 40)
			w4 = l;
	}
	fprintf(fp, "%*s|%*s|%*s|%*s (C Type)\n", -w1, "Name", -w2, "Update", -w3, "Default", -w4, "Keyword");
	for (pOT = aOTTypes; pOT->chkey != '\000'; ++pOT) {
		fprintf(fp, "%*s|%*s|%*s|%*s",
			-w1, nil != pOT->pchlabel ? pOT->pchlabel : "?",
			-w2, nil != pOT->pchupdate ? pOT->pchupdate : "",
			-w3, nil != pOT->pchdef ? pOT->pchdef : "",
			-w4, nil != pOT->pcharg ? pOT->pcharg : "");
		if ((char *)0 == pOT->pchbase) {
			fprintf(fp, "\n");
			continue;
		}
		fprintf(fp, " (%s %s)\n", pOT->pchbase, pOT->pchdecl);
	}
}
