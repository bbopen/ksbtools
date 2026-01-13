/* $Id: key.c,v 8.13 1998/09/19 21:15:30 ksb Exp $
 * API keys.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
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
#include "key.h"
#include "routine.h"
#include "emit.h"

#if USE_SYS_PARAM_H
#include <sys/param.h>
#endif

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#if !defined(S_ISREG)
#define S_ISREG(Mst_mode)       ((Mst_mode & S_IFMT) == S_IFREG)
#endif

#if !defined(KEY_GROWS)
#define KEY_GROWS	16		/* not too many api keys	*/
#endif

/* we use this structure to fake a dynamic key list			(ksb)
 * put an alias for each key in the (calloc/realloc'd) list of
 * keys.  EscCvt likes what it sees and all is right with the
 * world.
 */
static EMIT_HELP EHFake = {
	"dynamic key map", "%K",
	0,				/* uhas, current slots		*/
	(EMIT_MAP *)0,			/* pER, slots for EscCvt	*/
	(char *)0,
	(EMIT_HELP *)0
};
static KEY **ppKVRoot = (KEY **)0;	/* list of slots		*/
static unsigned uVecAlloc = 0;		/* number of slots available	*/
static unsigned uVecCur = 0;		/* next slot to fill		*/
static KEY *pKVAll = (KEY *)0;		/* threaded list of keys to check */


KEY *pKVDeclare,			/* types that declare data	*/
    *pKVVerify;				/* types which use "verify" action*/

/* look up a key given the name, and maybe the letter			(ksb)
 * resolve the pKV->pcowner, or dump and error
 *
 * pass KEY_NOSHORT for no short name
 * we expand pERExport as needed, but the order doesn't change
 * so the index in the key structure is enough to find them.
 */
KEY *
KeyFind(pcName, cName, fAdd)
char *pcName;
int cName, fAdd;
{
	register unsigned u;
	register KEY *pKVInit;
	register EMIT_MAP *pERInit;

	if ((char *)0 == pcName && KEY_NOSHORT == cName) {
		fprintf(stderr, "%s: KeyFind: no name to find!\n", progname);
		exit(MER_INV);
	}
	for (u = 0; u < uVecCur; ++u) {
		if ((char *)0 != pcName && (char *)0 != EHFake.pER[u].pcword && 0 == strcmp(EHFake.pER[u].pcword, pcName)) {
			/* found by long name */
			if (KEY_NOSHORT == EHFake.pER[u].cekey && KEY_NOSHORT != cName) {
				EHFake.pER[u].cekey = cName;
			}
		} else if (KEY_NOSHORT != cName && EHFake.pER[u].cekey == cName) {
			/* found by short name */
			if ((char *)0 == EHFake.pER[u].pcword && (char *)0 != pcName) {
				EHFake.pER[u].pcword = pcName;
			}
		} else {
			continue;
		}
		return ppKVRoot[u];
	}

	if (!fAdd)
		return (KEY *)0;

	/* can't find it, build it...
	 *
	 * if we don't have space grow the slots by KEY_GROWS (we do not
	 * expect many keys in a mkcmd run (1-3/API at most)
	 */
	if (uVecAlloc <= uVecCur) {
		register KEY **ppKVTemp;
		register EMIT_MAP *pERTemp;
		register unsigned uAdd;

		uAdd = uVecCur - uVecAlloc + KEY_GROWS;
		if ((KEY **)0 == ppKVRoot) {
			ppKVTemp = (KEY **)calloc(uAdd, sizeof(KEY *));
		} else {
			ppKVTemp = (KEY **)realloc((void *)ppKVRoot, (uVecAlloc + uAdd) * sizeof(KEY *));
		}
		if ((EMIT_MAP *)0 == EHFake.pER) {
			pERTemp = (EMIT_MAP *)calloc(uAdd, sizeof(EMIT_MAP));
		} else {
			pERTemp = (EMIT_MAP *)realloc((void *)EHFake.pER, (uVecAlloc + uAdd) * sizeof(EMIT_MAP));
		}
		if ((EMIT_MAP *)0 == pERTemp || (KEY **)0 == ppKVTemp) {
			fprintf(stderr, acOutMem, progname);
			exit(MER_LIMIT);
		}
		/* commit changes */
		uVecAlloc += uAdd;
		ppKVRoot = ppKVTemp;
		EHFake.pER = pERTemp;
	}

	/* install it
	 */
	ppKVRoot[uVecCur] = (KEY *)malloc(sizeof(KEY));
	if ((KEY *)0 == ppKVRoot[uVecCur]) {
		fprintf(stderr, acOutMem, progname);
		exit(MER_LIMIT);
	}
	pKVInit = ppKVRoot[uVecCur];
	pERInit = EHFake.pER + uVecCur;

	/* build it
	 */
	pERInit->cekey = cName;
	pERInit->pcword = pcName;
	pERInit->pcdescr = "dynamic";

	pKVInit->wkattr = KV_NAMED;
	pKVInit->uindex = uVecCur;
	pKVInit->pcowner = pKVInit->pcclient = (char *)0;
	pKVInit->uoline = pKVInit->ucline = 0;
	pKVInit->pORin = nilOR;
	pKVInit->pcversion = (char *)0;
	ListInit(& pKVInit->LIinputs, 0);
	ListInit(& pKVInit->LIaugments, 0);
	pKVInit->pKVinit = (KEY *)0;
	pKVInit->pKVnext = pKVAll;
	pKVAll = pKVInit;

	EHFake.uhas = ++uVecCur;
	return pKVInit;
}

/* build a key we can bind to an option letter				(ksb)
 * or any sub structure of the option
 */
static KEY *
KeyOption(pOR)
OPTION *pOR;
{
	register KEY *pKVInit;

	pKVInit = (KEY *)malloc(sizeof(KEY));
	if ((KEY *)0 == pKVInit) {
		fprintf(stderr, acOutMem, progname);
		exit(MER_LIMIT);
	}

	pKVInit->wkattr = KV_OPTION;
	pKVInit->uindex = 0x0a1dead;	/* should SEGV nicely */
	pKVInit->pcowner = pKVInit->pcclient = (char *)0;
	pKVInit->uoline = pKVInit->ucline = 0;
	pKVInit->pORin = pOR;
	pKVInit->pcversion = (char *)0;
	ListInit(& pKVInit->LIinputs, 0);
	ListInit(& pKVInit->LIaugments, 0);
	pKVInit->pKVinit = (KEY *)0;
	pKVInit->pKVnext = pKVAll;
	pKVAll = pKVInit;

	return pKVInit;
}

/* Attach a key to anything.						(ksb)
 * If it is named don't put it in the install slot, unless we are
 * added a named client key.
 */
KEY *
KeyAttach(iKeyType, pOR, pcName, cKey, ppKVInstall)
int iKeyType, cKey;
OPTION *pOR;
char *pcName;
KEY **ppKVInstall;
{
	register KEY *pKVRet;

	if ((char *)0 != pcName) {
		pKVRet = KeyFind(pcName, cKey, 1);
		if (KV_CLIENT == iKeyType) {
			pKVRet->wkattr &= ~KV_MASK;
			pKVRet->wkattr |= KV_CLIENT;
		}
		if ((KEY **)0 != ppKVInstall) {
			*ppKVInstall = pKVRet;
		}
		return pKVRet;
	}
	if (KV_NAMED == iKeyType || KV_CLIENT == iKeyType) {
		fprintf(stderr, "%s: %s(%d) unnamed keys are only allowed within triggers and routines\n", progname, pchScanFile, iLine);
		exit(MER_SEMANTIC);
	}
	if ((KEY *)0 != (pKVRet = *ppKVInstall)) {
		if (iKeyType != (pKVRet->wkattr & KV_MASK)) {
			fprintf(stderr, "%s: internal key mismatch\n", progname);
			exit(MER_PAIN);
		}
		return pKVRet;
	}
	pKVRet = KeyOption(pOR);
	pKVRet->wkattr &= ~KV_MASK;
	pKVRet->wkattr |= iKeyType;
	return *ppKVInstall = pKVRet;
}

/* produce a name the user would recognize for this key			(ksb)
 */
char *
KeyName(pKV, pcBuf)
KEY *pKV;
char *pcBuf;
{
	static char acDef[80];
	register char *pcS;

	if (nil == pcBuf) {
		pcBuf = acDef;
	}
	if (KEY_SYNTH(pKV)) {
		return usersees(pKV->pORin, pcBuf);
	}
	if (KEY_CLIENT(pKV)) {
		return EHFake.pER[pKV->uindex].pcword;
	}
	if (KEY_ROUTINE(pKV)) {
		pcS = pcBuf;
		*pcS++ = 'r';
		*pcS++ = 't';
		*pcS++ = 'n';
		*pcS++ = ' ';
		usersees(pKV->pORin, pcS);
		return pcBuf;
	}
	if (KEY_OPT_BOUND(pKV)) {
		return usersees(pKV->pORin, pcBuf);
	}
	return EHFake.pER[pKV->uindex].pcword;
}

/* complete pending augment operations and check to be sure that	(ksb)
 * everything is cool.
 */
static int
KeyCheck(pKV)
KEY *pKV;
{
	register char **ppcAugment, *pcAdd, **ppcInit;
	auto LIST_INDEX wAugment, wWhich, wInit;
	register LIST_INDEX wLoop;
	register int iFail;
	register KEY *pKVInit;

	if (KEY_CUR_INIT(pKV)) {
		fprintf(stderr, "%s: %s: key initializer infinite loops\n", progname, KeyName(pKV, nil));
		iExit |= MER_SEMANTIC;
		return 2;
	}
	if (KEY_DONE(pKV)) {
		return 0;
	}
	if ((KEY *)0 != (pKVInit = pKV->pKVinit)) {
		pKV->pKVinit = (KEY *)0;
		pKV->wkattr |= KV_CUR_INIT;
		iFail = KeyCheck(pKVInit);
		if (2 == iFail) {
			fprintf(stderr, "%s: back to %s\n", progname, KeyName(pKV, nil));
		}
		pKV->wkattr &= ~KV_CUR_INIT;
		if (0 != iFail) {
			return iFail;
		}
		if ((char *)0 == pKVInit->pcversion) {
			/* XXX warning API supplied no version string */
		} else if ((char *)0 == pKV->pcversion) {
			fprintf(stderr, "%s: key %s: should have version %s\n", progname, KeyName(pKV, nil), pKVInit->pcversion);
		} else if (atoi(pKVInit->pcversion) != atoi(pKV->pcversion)) {
			fprintf(stderr, "%s: key %s: version %s != %s", progname, KeyName(pKV, nil), pKV->pcversion, pKVInit->pcversion);
			fprintf(stderr, " (on initializer key %s)\n", KeyName(pKVInit, nil));
			iExit |= MER_SEMANTIC;
			return 1;
		}
	}
	if ((char *)0 == pKV->pcowner) {
		fprintf(stderr, "%s: %s(%d) %s: key has no owner declared, mark the definition with the keyword \"initializer\"\n", progname, pKV->pcclient, pKV->ucline, KeyName(pKV, nil));
		iExit |= MER_SEMANTIC;
		return 1;
	}

	ListInit(& pKV->LIvalues, KEY_V_ONCE(pKV));

	/* copy in any init key
	 */
	if ((KEY *)0 != pKVInit) {
		ppcInit = ListCur(& pKVInit->LIvalues, & wInit);
	} else {
		ppcInit = (char **)0;
		wInit = 0;
	}

	ppcAugment = ListCur(& pKV->LIinputs, & wAugment);
	iFail = 0;
	wWhich = 0;
	wLoop = (wInit > wAugment) ? wInit : wAugment;
	while (++wWhich, 0 != wLoop--) {
		if (0 != wAugment) {
			pcAdd = *ppcAugment++, --wAugment;
		} else {
			pcAdd = (char *)0;
		}
		if ((char *)0 == pcAdd && (char **)0 != ppcInit && wWhich <= wInit) {
			pcAdd = ppcInit[wWhich-1];
		}
		if ((char *)0 == pcAdd) {
			fprintf(stderr, "%s: %s(%d) key %s: missing value %d\n", progname, pKV->pcowner, pKV->uoline, KeyName(pKV, nil), wWhich);
			++iFail;
			continue;
		}
		ListAdd(& pKV->LIvalues, pcAdd);
	}
	if (0 != iFail) {
		iExit |= MER_SEMANTIC;
		return 1;
	}

	ppcAugment = ListCur(& pKV->LIaugments, & wAugment);
	while (0 != wAugment--) {
		if ((char *)0 == (pcAdd = *ppcAugment++)) {
			continue;
		}
		ListAdd(& pKV->LIvalues, pcAdd);
	}
	pKV->wkattr &= ~KV_AUGMENT;
	pKV->wkattr |= KV_DONE;
	return 0;
}

/* fix all the keys we found in the input spec.				(ksb)
 * Report on all keys used if fKeyTable is set.
 */
void
KeyFix()
{
	register KEY *pKV;
	register int iRet, iFWidth, iKWidth, iCWidth, i;
	auto LIST_INDEX wHas;
	auto char acName[48];

	iRet = 0;
	iCWidth = 0;
	iFWidth = 0, iKWidth = 0;
	for (pKV = pKVAll; (KEY *)0 != pKV; pKV = pKV->pKVnext) {
		if (0 != KeyCheck(pKV)) {
			++iRet;
			continue;
		}
		i = strlen(pKV->pcowner);
		if (i > iFWidth) {
			iFWidth = i;
		}
		i = strlen(KeyName(pKV, nil));
		if (i > iKWidth) {
			iKWidth = i;
		}
		if (! KEY_OPT_BOUND(pKV) && KEY_NOSHORT != (i = EHFake.pER[pKV->uindex].cekey)) {
			CSpell(acName, i, 0);
			i = strlen(acName);
			if (i < iCWidth)
				iCWidth = i;
		}
	}
	if (0 != iRet) {
		exit(MER_SEMANTIC);
	}
	/* is fKeyTable is set we should print out a nifty table
	 * of all the keys, their owner file names and how many
	 * values they have.
	 */
	if (!fKeyTable || 0 == iFWidth) {
		return;
	}
	if (0 != iCWidth && iCWidth < 4) {
		iCWidth = 4;
	}
	printf("%-.*s %-*s %-*s  Count Version\n", iCWidth, "Abr.", iKWidth, "Name", iFWidth+4, "Location");
	for (pKV = pKVAll; (KEY *)0 != pKV; pKV = pKV->pKVnext) {
		if (KEY_OPT_BOUND(pKV) || KEY_NOSHORT == EHFake.pER[pKV->uindex].cekey) {
			acName[0] = '\000';
		} else {
			CSpell(acName, EHFake.pER[pKV->uindex].cekey, 0);
		}
		(void)ListCur(& pKV->LIvalues, & wHas);
		printf("%s%-*s %-*s %*s(%4d) %4d  %s\n", iCWidth != 0 ? " " : "", iCWidth, acName, iKWidth, KeyName(pKV, nil), iFWidth, pKV->pcowner, pKV->uoline, wHas, (char *)0 == pKV->pcversion ? "" : pKV->pcversion);
	}
}

/* pull a key name off a string and return the key data, or nilKV	(ksb)
 */
KEY *
KeyParse(ppcCursor)
char **ppcCursor;
{
	auto int iError;

	/* EscCvt records the index into the EMIT_MAP of what it found
	 * so we can find the value data (EHFake.ufound) -- report
	 * other botches and hold the results.
	 */
	switch (EscCvt(& EHFake, ppcCursor, & iError)) {
	case 0:
		fprintf(stderr, "%s: API key: %d: what number?\n", progname, iError);
		iExit |= MER_SEMANTIC;
		return (KEY *)0;
	case -1:
		fprintf(stderr, "%s: API key: %c: unknown key short cut\n", progname, iError);
		iExit |= MER_SEMANTIC;
		return (KEY *)0;
	case -2:
		fprintf(stderr, "%s: API key: %*.*s: unknown long key name\n", progname, iError, iError, *ppcCursor);
		iExit |= MER_SEMANTIC;
		return (KEY *)0;
	}
	return ppKVRoot[EHFake.ufound];
}

/* ideas in selector set:
 *	"f" first file found, or set current to 0
 */
static EMIT_MAP aERKey[] = {
	{'%', "top", "recurse to top level..."},
	{'A', "argument", "set the special contexts argument..."},
	{'B', "buffer", "find a conversion variable by name..."},
	{'C', "in", "option that contains us..."},
	{'d', "cat", "values as a list"},
	{'f', "call", "first value is a function name, others params"},
	{'F', "file", "the name of the module that owns this key"},
	{'h', "here", "literal /string/ to output, keep going"},
	{'H', "hold", "hold the value as the special contexts argument"},
	/* I reserved for interface modules (output.c) */
	{'j', "totalcolumn", "total count of value columns"},
	{'J', "column", "current column"},
	{'K', "key", "look up value as a key..."},
	{'l', "list", "values as a comma separated list"},
	{'n', "nop", "no operation to separate numbers"},
	{'N', "quit", "quit this key escape"},
	{'o', "output", "output current value in expansion, keep going"},
	{'r', "reverse", "reverse the value list's order"},
	{'R', "control", "look up value as a control point..."},
	{'s', "shift", "shift the values of the key left one for this escape"},
	{'t', "truncate", "remove the right most value from the key"},
	{'v', "value", "output current value and return"},
	{'V', "version", "API version number"},
	{'w', "string", "values as a white-space list"},
	{'x', "loop", "output current and redo this expansion for next value"},
	{'y', "type", "look up value as a type..."},
	{'z', "error", (char *)0},
	{'Z', "option", "look up value as an option or buffer..."},
	{'$', "last", "move to last value in key"},
	{'.', "op", "force control down to value operator level"},
	{'/', "apply", "apply value operator to each member of the key"},
	{'?', "best", "apply value operator to each keep only successfuls"},
	{'!', "cut", "mark the start of a %x loop"},
	{'=', "next", "do not ouptut the value, loop anyway"}
};

/* The value operators only make sense if you know how the API		(ksb)
 * culture uses them.  For example the PATH thing (p) looks totally out
 * of control until you think about all the applications
 * that need to know where "chown" or "X" live.  These programs move
 * around but always take the same interface.  Now we can have a
 * "path_chown" that finds it (I do know about _PATH_CHOWN under BSD,
 * great idea if they knew _all_ the binary files I needed).
 *
 * Other string ideas:
 *	n-m make the number thing do a range here? (keep or remove?)
 *	-m remove character from the end back m positions (-2 takes off .c)
 * network things
 *	H  FQDN name for host (what about c-name?)
 *	I  ip address for hostname?
 *	O  port for service
 *	P  protocol number
 * UNIX things (instck and modecanon could use these)
 *	l  login name from uid -- what user "7" ?
 *	u  uid from login name -- what uid "charon" ?
 *	g  group name from gid -- what group "0" ?
 *	n  gid from group name -- what gid "wheel" ?
 *
 * For example entombing could use "charon" as a value and find the uid at
 * compile time, rather than the bogus -V checks to make sure they match.
 * Console should use "console" as a value and lookup the FDQN to use a
 * local c-name to find the consolse server host -- but that can wait
 * until run time so you can move it... I guess.  Maybe we just need to
 * know that there _is_ one to build the product.
 */
static EMIT_MAP aERValue[] = {
	{'a', "angle", "enclose the value in angle braces"},
	{'b', "basename", "remove last .extender",},
	{'c', "character", "turn the value into a C character constant"},
	{'d', "extender", "keep just the last .extender",},
	{'e', "expand", "replace value with its expansion..."},
	{'f', "filename", "keep just the filename (w/o a path)",},
	{'i', "insert", "prefix value with string"},
	{'k', "kill", "remove value (put in (char *)0)"},
	{'l', "lowercase", "turn value into lowercase"},
	{'p', "path", "full path found in $PATH or %K<PATH> for value"},
	{'q', "quote", "turn the value into a quoted C string"},
	{'u', "unexpand", "protect the value from the expander"},
	{'U', "uppercase", "turn value into uppercase"},
	{'w', "dirname", "keep the directory part of the path"},
	{'X', "expand", (char *)0},
	{'z', "error", (char *)0},
	{'+', "plus", "add string to the end of value"},
	{'~', "swapcase", "toggle case in value"},
};

EMIT_HELP EHKey = {
	"key expander", "%K<map> or %ZK",
	sizeof(aERKey)/sizeof(EMIT_MAP),
	aERKey,
	"select value from the list (rather than 1)",
	(EMIT_HELP *)0
};
EMIT_HELP EHValue = {
	"operator level", "%K<map>. or %K<map>/",
	sizeof(aERValue)/sizeof(EMIT_MAP),
	aERValue,
	"reduce value to only this character",
	& EHKey
};

/* look in the colon separates path for the file in question		(ksb)
 * parser.c:Search is a function we should merge with -- ksb
 */
static char *
PathSearch(pcFile, pcBuf, pcPath)
char *pcFile, *pcBuf, *pcPath;
{
	register char *pcDir, *pcNext;
	auto struct stat stPath;

	for (pcDir = pcPath; (char *)0 != pcDir; pcDir = pcNext) {
		pcNext = strchr(pcDir, ':');
		if ((char *)0 != pcNext) {
			*pcNext = '\000';
		}
		(void)strcpy(pcBuf, pcDir);
		if ((char *)0 != pcNext) {
			*pcNext++ = ':';
		}
		if ('\000' == *pcBuf) {
			(void)strcpy(pcBuf, ".");
		}
		(void)strcat(pcBuf, "/");
		(void)strcat(pcBuf, pcFile);
		if (-1 != stat(pcBuf, & stPath) && S_ISREG(stPath.st_mode)) {
			return pcBuf;
		}
	}
	return (char *)0;
}

/* silly, but this leverage saves me all sorts of work			(ksb)
 * search for the file in $PATH (if -G is not set).
 * Always search %K<PATH> first if we have one.
 */
static char *
KeyExecl(pcTail, pcBuf)
char *pcTail, *pcBuf;
{
	register KEY *pKVPath;
	register char *pcEnvPath, **ppc;
	auto LIST_INDEX w;
	static char acPath[] = "PATH";

	pKVPath = KeyFind(acPath, KEY_NOSHORT, 0);
	if ((KEY *)0 != pKVPath) {
		ppc = ListCur(& pKVPath->LIvalues, & w);
		for (/* nada */; 0 != w--; ++ppc) {
			if ((char *)0 != PathSearch(pcTail, pcBuf, *ppc)) {
				return pcBuf;
			}
		}
	}
	if (!fWeGuess || (char *)0 == (pcEnvPath = getenv(acPath))) {
		return (char *)0;
	}
	return PathSearch(pcTail, pcBuf, pcEnvPath);
}

static char acStomp[] =		/* magic point to map to nil		*/
	"";
static int iValTrap = 0;	/* divert errors to %K<..>? level	*/

/* map a value to a new value given an escape to execute		(ksb)
 * we return the new value in malloc'd space
 * We know that the enclosing code will put the real value back before
 * we finish the whole key escape.
 * But the state _is_ held in the LInode, so recursive
 * calls (though any expander) see the enclosed string.
 *
 * outline:
 *	find about enough space, (the smallest thing we'll allocate is 8 bytes)
 *	build the value,
 *	let the caller install.
 *
 * There are escapes I think we should do in the future:
 *	do many value escapes (v/.../)
 *	xapply's list buster ([sep count ...])
 */
char *
ValueEsc(ppcCursor, pcScan, pcWhere, iWhere, pOR, pEO)
char **ppcCursor, *pcScan, *pcWhere;
OPTION *pOR;
EMIT_OPTS *pEO;
int iWhere;
{
	register int cOp, iLen, cEnd;
	register char *pcMem, *pcCopy;
	register LIST_INDEX w;
	auto int iNumber, iRight;
	auto char acError[8];
	static EMIT_MAP aERRange[] = {
		{'$', "end", "the end of the string"}
	};
	static EMIT_HELP EHRange = {
		"range level", "",
		sizeof(aERRange)/sizeof(EMIT_MAP),
		aERRange,
		"reduce value to only this character",
		(EMIT_HELP *)0
	};

	switch (cOp = EscCvt(& EHValue, ppcCursor, & iNumber)) {
	case 0:		/* select a single character from the string */
		pcMem = (char *)0;

		/* 3	keep char 3
		 * 1-3	keep chars 1-3
		 * 2-$	keep everything but character 1
		 */
		if ('-' != ppcCursor[0][0]) {
			iRight = iNumber;
		} else switch (++*ppcCursor, cEnd = EscCvt(& EHRange, ppcCursor, & iRight)) {
		case 0:
			break;
		case '$':	/* end of string */
			iRight = strlen(pcScan);
			break;
		/* find first 'x'? */
		/* find Last 'y'? */
		case -1:	/* not a numnber */
			CSpell(acError, iRight, 0);
			fprintf(stderr, "%s: %s: bad right range\n", progname, acError);
			iExit |= MER_SEMANTIC;
			return (char *)0;
		case -2:
			fprintf(stderr, "%s: %*.*s: bad right range\n", progname, iRight, iRight, *ppcCursor);
			iExit |= MER_SEMANTIC;
			return (char *)0;
		default:
			CSpell(acError, cEnd, 0);
			fprintf(stderr, "%s: %s: unknown desination token (internal error)\n", progname, acError);
			iExit |= MER_SEMANTIC;
			return (char *)0;
		}
		pcMem = malloc(((iRight-iNumber+1)|7)+1);
		if ((char *)0 == pcMem) {
			fprintf(stderr, acOutMem, progname);
			exit(MER_LIMIT);
		}
		iLen = strlen(pcScan);
		if (iLen < iNumber || iNumber < 1 || iLen < iRight || iRight < 1) {
			if (0 == iValTrap) {
				fprintf(stderr, "%s: %s[%d]: \"%s\" no column %d in value (length = %d)\n", progname, pcWhere, iWhere, pcScan, iNumber, iLen);
				iExit |= MER_SEMANTIC;
			}
			free(pcMem);
			pcMem = (char *)0;
		} else {
			--iNumber;
			iRight -= iNumber;
			(void)strncpy(pcMem, pcScan+iNumber, iRight);
			pcMem[iRight] = '\000';
		}
		break;
	case 'a':	/* enclose the string in <> for links */
		pcMem = malloc((strlen(pcScan)|3)+5);
		if ((char *)0 == pcMem) {
			fprintf(stderr, acOutMem, progname);
			exit(MER_LIMIT);
		}
		*pcMem = '<';
		(void)strcpy(pcMem+1, pcScan);
		(void)strcat(pcMem, ">");
		break;
	case 'b':	/* leave basename */
		if ((char *)0 == (pcCopy = strrchr(pcScan, '/'))) {
			pcCopy = pcScan;
		}
		if ((char *)0 == (pcCopy = strrchr(pcCopy, '.'))) {
			return pcScan;
		}
		pcMem = strdup(pcScan);
		if ((char *)0 == (pcCopy = strrchr(pcMem, '/'))) {
			pcCopy = pcMem;
		}
		pcCopy = strrchr(pcCopy, '.');
		*pcCopy = '\000';
		break;
	case 'c':	/* turn into a C character constant */
		pcMem = malloc(8);
		if ((char *)0 == pcMem) {
			fprintf(stderr, acOutMem, progname);
			exit(MER_LIMIT);
		}
		CSpell(pcMem, pcScan[0], 1);
		break;
	case 'd':	/* leave dot extender */
		if ((char *)0 == (pcCopy = strrchr(pcScan, '/'))) {
			pcCopy = pcScan;
		}
		if ((char *)0 == (pcCopy = strrchr(pcCopy, '.'))) {
			return (char *)0;
		}
		pcMem = pcCopy+1;
		break;
	case 'f':	/* leave filename */
		if ((char *)0 == (pcCopy = strrchr(pcScan, '/'))) {
			return pcScan;
		}
		return pcCopy+1;
	case 'i':	/* insert string -- arbitrary quotes */
	case '+':	/* append string */
		pcCopy = *ppcCursor + 1;
		iLen = 0;
		while ('\000' != *pcCopy && **ppcCursor != *pcCopy) {
			++iLen, ++pcCopy;
		}
		iNumber = strlen(pcScan);
		pcMem = malloc(((iLen+iNumber)|7)+1);
		if ((char *)0 == pcMem) {
			fprintf(stderr, acOutMem, progname);
			exit(MER_LIMIT);
		}
		if ('+' == cOp) {
			(void)strcpy(pcMem, pcScan);
			(void)strncpy(pcMem+iNumber, *ppcCursor+1, iLen);
		} else {
			(void)strncpy(pcMem, *ppcCursor+1, iLen);
			(void)strcpy(pcMem+iLen, pcScan);
		}
		pcMem[iLen+iNumber] = '\000';
		*ppcCursor = pcCopy+1;
		break;
	case 'k':
		pcMem = acStomp;
		break;
	case 'l':	/* lower case */
		pcMem = malloc((strlen(pcScan)|3)+1);
		(void)strcpy(pcMem, pcScan);
		for (pcCopy = pcMem; '\000' != *pcCopy; ++pcCopy) {
			if (! isupper(*pcCopy))
				continue;
			*pcCopy = tolower(*pcCopy);
		}
		break;
	case 'p':	/* full path of value */
		pcMem = malloc(MAXPATHLEN+8);
		if ((char *)0 == pcMem) {
			fprintf(stderr, acOutMem, progname);
			exit(MER_LIMIT);
		}
		if ((char *)0 == KeyExecl(pcScan, pcMem)) {
			if (0 == iValTrap) {
				fprintf(stderr, "%s: %s[%d]: %s not found from PATH\n", progname, pcWhere, iWhere, pcScan);
				iExit |= MER_SEMANTIC;
			}
			pcMem = (char *)0;
			break;
		}
		break;
	case 'q':	/* make a C string constant out of it current */
		/* worst case is 4 chatacters/cell (\001) + 2 for the quotes
		 * plus the null on the end (like that *ever* happens)
		 */
		iNumber = strlen(pcScan);
		pcMem = malloc((iNumber*4)+4);
		if ((char *)0 == pcMem) {
			fprintf(stderr, acOutMem, progname);
			exit(MER_LIMIT);
		}
		pcCopy = pcMem;
		*pcCopy++ = '\"';
		for (w = 0; w < iNumber; ++w) {
			CSpell(pcCopy, pcScan[w], 0);
			pcCopy += strlen(pcCopy);
		}
		*pcCopy++ = '\"';
		*pcCopy = '\000';
		break;
	case 'u':	/* protect from % expansion one level */
		/* worst case is 2 characters/cell (%%, %;, %{, %}, %+) + \000
		 */
		iNumber = strlen(pcScan);
		pcMem = malloc((iNumber+1)*2);
		if ((char *)0 == pcMem) {
			fprintf(stderr, acOutMem, progname);
			exit(MER_LIMIT);
		}
		pcCopy = pcMem;
		for (w = 0; w < iNumber; ++w) {
			switch (*pcCopy = pcScan[w]) {
			case '%':
			case '{':
			case '}':
			case ';':
				*pcCopy++ = '%';
				*pcCopy++ = pcScan[w];
				continue;
			case '\n':
				*pcCopy++ = '%';
				*pcCopy = '+';
				/* fallthrough */
			default:
				++pcCopy;
				continue;
			}
		}
		*pcCopy = '\000';
		break;
	case 'U':	/* upper case */
		pcMem = strdup(pcScan);
		for (pcCopy = pcMem; '\000' != *pcCopy; ++pcCopy) {
			if (! islower(*pcCopy))
				continue;
			*pcCopy = toupper(*pcCopy);
		}
		break;
	case 'w':	/* path prefix (or ".") */
		if ((char *)0 == (pcCopy = strrchr(pcScan, '/'))) {
			return ".";
		}
		if (pcCopy == pcScan) {
			return "/";
		}
		pcMem = strdup(pcScan);
		pcCopy = strrchr(pcMem, '/');
		*pcCopy = '\000';
		break;
	case 'e':	/* replace value with its expansion */
	case 'X':
		pcMem = StrExpand(pcScan, pOR, pEO, 10240);
		if ((char *)0 == pcMem && 0 == iValTrap) {
			fprintf(stderr, "%s: %s[%d]: failed to expand\n", progname, pcWhere, iWhere);
			iExit |= MER_SEMANTIC;
		}
		break;
	case '~':	/* toggle case */
		pcMem = malloc((strlen(pcScan)|3)+1);
		(void)strcpy(pcMem, pcScan);
		for (pcCopy = pcMem; '\000' != *pcCopy; ++pcCopy) {
			if (isupper(*pcCopy)) {
				*pcCopy = tolower(*pcCopy);
				continue;
			}
			if (islower(*pcCopy)) {
				*pcCopy = toupper(*pcCopy);
			}
		}
		break;

	case -1:
		CSpell(acError, iNumber, 0);
		fprintf(stderr, "%s: %s: unknown percent escape, use '-E key' and '-E value' for help\n", progname, acError);
		iExit |= MER_SYNTAX;
		return (char *)0;
	case -2:
		fprintf(stderr, "%s: %*.*s: unknown long escape, use '-E key' and '-E value' for help\n", progname, iNumber, iNumber, *ppcCursor);
		iExit |= MER_SYNTAX;
		return (char *)0;
	default:
	case 'z':
		(void)CSpell(acError, cOp, 0);
		fprintf(stderr, "%s: value level: %%%s: expand error\n", progname, acError);
		iExit |= MER_PAIN;
		return (char *)0;
	}

	return pcMem;
}

/* do the std emit thing for API keys					(ksb)
 */
int
KeyEsc(ppcCursor, pOR, pKVThis, pEO, pcDst)
char **ppcCursor, *pcDst;
OPTION *pOR;
register KEY *pKVThis;
EMIT_OPTS *pEO;
{
	register int cKey, iRet;
	register char **ppcValues, *pcBack, *pcWhere;
	register char **ppcKeep;
	register LIST_INDEX w, wCenter;
	auto LIST_INDEX wMany, wStart;
	auto int iColumn, iNumber;
	auto KEY *pKVDown;
	auto OPTION *pORDown;
	auto OPTTYPE *pOTDown;
	auto char *pcScan, *pcMem, *pcLoop;
	static char acError[] = "%s: %s[%d]: %s: no values\n";
	auto char acWhere[56], acSpell[8];
	auto char *apcStall[2];

	apcStall[0] = pEO->pcarg1;
	apcStall[1] = pEO->pcarg2;
	*pcDst = '\000';
	iRet = 0;
	ppcValues = ListCur(& pKVThis->LIvalues, & wStart);
	pcWhere = KeyName(pKVThis, acWhere);

	/* we save the current values to restore for %a hacking
	 */
	if ((char **)0 == (ppcKeep = (char **)calloc(wStart+1, sizeof(char *)))) {
		fprintf(stderr, acOutMem, progname);
		exit(MER_LIMIT);
	}
	for (w = 0; w < wStart; ++w) {
		ppcKeep[w] = ppcValues[w];
	}
	/* N.B. do not add a "return" statement from here {...
	 */

	/* prepare for the std expander loop:
	 *	current value is 1
	 */
	wMany = wStart;
	iColumn = (wMany > 0) ? 1 : 0;
	pcLoop = *ppcCursor;
	for (;;) { switch ((pcBack = *ppcCursor), cKey = EscCvt(& EHKey, ppcCursor, & iNumber)) {
	case 0:		/* number selects column in key */
		if (iNumber < 1 || iNumber > wMany) {
			fprintf(stderr, "%s: %s: %d: ", progname, pcWhere, iNumber);
			if (0 == wMany) {
				fprintf(stderr, "no values available\n");
			} else if (1 == wMany) {
				fprintf(stderr, "only 1 value\n");
			} else {
				fprintf(stderr, "only values 1 to %d available\n", wMany);
			}
			iRet = 1;
			break;
		}
		iColumn = iNumber;
		continue;
	case '$':	/* move to last value in the key */
		if (0 == wMany) {
			fprintf(stderr, "%s: %s: %%$: no values\n", progname, pcWhere);
			iRet = 1;
			break;
		}
		iColumn = wMany;
		continue;
	case 'f':
		if (0 == wMany) {
			sprintf(pcDst, "/* %s: no function to call */", pKVThis->pcowner);
			break;
		}
		for (w = 0; w < wMany; ++w) {
			if (0 == w) {
				/* nothing */
			} else if (1 == w) {
				*pcDst++ = '(';
			} else {
				*pcDst++ = ',';
				*pcDst++ = ' ';
			}
			pcScan = ppcValues[w];
			if ((char *)0 == pcScan) {
				(void)strcpy(pcDst, "(char *)0");
			} else {
				(void)strcpy(pcDst, pcScan);
			}
			pcDst += strlen(pcDst);
		}
		if (1 == w) {
			*pcDst++ = '(';
		}
		*pcDst++ = ')';
		*pcDst = '\000';
		break;
	case 'F':	/* the name of the module that owns this key */
		(void)strcpy(pcDst, pKVThis->pcowner);
		break;
	case 'j':	/* current count of value columns */
		(void)sprintf(pcDst, "%d", wMany);
		break;
	case 'J':	/* current column, continue */
		(void)sprintf(pcDst, "%d", iColumn);
		pcDst += strlen(pcDst);
		continue;
	case 'd':	/* values with no spaces */
	case 'l':	/* values as a list of strings */
	case 'w':	/* values with just white space */
		for (w = 0; w < wMany; ++w) {
			if (0 != w) {
				if ('l' == cKey) {
					*pcDst++ = ',';
				}
				if ('d' != cKey) {
					*pcDst++ = ' ';
				}
			}
			pcScan = ppcValues[w];
			if ((char *)0 == pcScan) {
				(void)strcpy(pcDst, "(char *)0");
			} else {
				(void)strcpy(pcDst, pcScan);
			}
			pcDst += strlen(pcDst);
		}
		*pcDst++ = '\000';
		break;
	case 'n':	/* no-operation to pad %K<Ksb>1.1n2.1n3.1l */
		continue;
	case 'N':	/* finish this escape */
		break;
	case 'r':	/* reverse the value list order */
		if (wMany < 2) {
			continue;
		}
		wCenter = wMany-1;
		for (w = 0; w < wCenter; ++w, --wCenter) {
			register char *pcSwap;

			pcSwap = ppcValues[w];
			ppcValues[w] = ppcValues[wCenter];
			ppcValues[wCenter] = pcSwap;
		}
		continue;
	case 's':	/* shift for this escape */
		if (0 == wMany) {
			fprintf(stderr, "%s: API key %s: %%s: no value to delete\n", progname, pcWhere);
			iRet = 1;
			break;
		}
		for (w = 1; w < wMany; ++w) {
			ppcValues[w-1] = ppcValues[w];
		}
		--wMany;
		if (iColumn > wMany) {
			iColumn = wMany;
		}
		/* put back at the end */
		continue;
	case 't':	/* truncate -- chop one value off the right end */
		if (0 == wMany) {
			fprintf(stderr, "%s: API key %s: %%t: no value to delete\n", progname, pcWhere);
			iRet = 1;
			break;
		}
		--wMany;
		if (iColumn > wMany) {
			iColumn = wMany;
		}
		continue;
	case 'v':	/* expand just this value */
		if (0 == iColumn || (char *)0 == (pcScan = ppcValues[iColumn-1])) {
			fprintf(stderr, acError, progname, pcWhere, iColumn, "named");
			iRet = 1;
			break;
		}
		(void)strcpy(pcDst, pcScan);
		break;
	case 'V':
		if ((char *)0 == pKVThis->pcversion) {
			fprintf(stderr, "%s: API key %s: has no version string\n", progname, pcWhere);
			iRet = 1;
			break;
		}
		(void)strcpy(pcDst, pKVThis->pcversion);
		break;

	case 'h':	/* here text (holerith) %K...h"i = "oh';'... */
		pcScan = *ppcCursor;
		if ('\000' == (cKey = *pcScan++)) {
			break;
		}
		while (cKey != *pcScan && '\000' != *pcScan) {
			*pcDst++ = *pcScan++;
		}
		if (cKey == *pcScan)
			++pcScan;
		*ppcCursor = pcScan;
		continue;

	/* chain to other expanders */
	case '%':	/* go back to top level */
		iRet = TopEsc(ppcCursor, pOR, pEO, pcDst);
		continue;
	case 'A':
	case 'H':
		if (0 == iColumn || (char *)0 == (pcScan = ppcValues[iColumn-1])) {
			fprintf(stderr, acError, progname, pcWhere, iColumn, "argument");
			iRet = 1;
			break;
		}
		pEO->pcarg2 = pEO->pcarg1;
		pEO->pcarg1 = pcScan;
		if ('H' == cKey) {
			continue;
		}
		iRet = TopEsc(ppcCursor, pOR, pEO, pcDst);
		break;
	case 'B':	/* find a variable buffer by name */
		if (0 == iColumn || (char *)0 == (pcScan = ppcValues[iColumn-1])) {
			fprintf(stderr, acError, progname, pcWhere, iColumn, "buffer");
			iRet = 1;
			break;
		}
		pORDown = FindVar(pcScan, pORDecl);
		if ((OPTION *)0 == pORDown) {
			fprintf(stderr, "%s: %s: %s: no such buffer\n", progname, pcWhere, ppcValues[iColumn-1]);
			iRet = 1;
		} else {
			iRet = TopEsc(ppcCursor, pORDown, pEO, pcDst);
		}
		break;
	case 'C':	/* option that contains this key ... */
		if (nilOR == pKVThis->pORin) {
			fprintf(stderr, "%s: %s: expander: no option enclosing this key\n", progname, pcWhere);
			iRet = 1;
		} else {
			iRet = TopEsc(ppcCursor, pKVThis->pORin, pEO, pcDst);
		}
		break;
	case 'K':	/* look up value as a key... */
		if (0 == iColumn || (char *)0 == (pcScan = ppcValues[iColumn-1])) {
			fprintf(stderr, acError, progname, pcWhere, iColumn, "key");
			iRet = 1;
			break;
		}
		pKVDown = KeyParse(& pcScan);
		if ((KEY *)0 == pKVDown) {
			fprintf(stderr, "%s: %s: %s: no such API key\n", progname, pcWhere, ppcValues[iColumn-1]);
			iRet = 1;
		} else {
			iRet = KeyEsc(ppcCursor, pOR, pKVDown, pEO, pcDst);
		}
		break;
	case 'Z':	/* look up value as an option... */
		if (0 == iColumn || (char *)0 == (pcScan = ppcValues[iColumn-1])) {
			fprintf(stderr, acError, progname, pcWhere, iColumn, "respect");
			iRet = 1;
			break;
		}
		pORDown = OptParse(& pcScan, pOR);
		if (nilOR == pORDown) {
			/* already gave a message on stderr */
			iRet = 1;
		} else {
			iRet = TopEsc(ppcCursor, pORDown, pEO, pcDst);
		}
		break;
	case 'R':	/* look up value as a control point... */
		if (0 == iColumn || (char *)0 == (pcScan = ppcValues[iColumn-1])) {
			fprintf(stderr, acError, progname, pcWhere, iColumn, "control");
			iRet = 1;
			break;
		}
		pORDown = SactParse(& pcScan, pOR, & iNumber);
		if ((OPTION *)0 == pORDown) {
			fprintf(stderr, "%s: %s: not a defined control point\n", progname, sactstr(iNumber));
			iRet = 1;
		} else {
			iRet = TopEsc(ppcCursor, pORDown, pEO, pcDst);
		}
		break;
	case 'x':	/* strange bird, lets you loop over the values */
	case 'o':
		/* XXX should this skip (char *)0 slots?
		 * will any operator but 'k' take a null slot?
		 */
		if (0 == iColumn) {
			break;
		}
		if ((char *)0 != (pcScan = ppcValues[iColumn-1])) {
			(void)strcpy(pcDst, pcScan);
			pcDst += strlen(pcDst);
		}
		if ('o' == cKey) {
			continue;
		}
	case '=':	/* next w/o an output */
		if (++iColumn > wMany) {
			/* if we continue here what else would we do?
			 */
			break;
		}
		*ppcCursor = pcLoop;
		continue;
	case '!':
		pcLoop = *ppcCursor;
		continue;
	case 'y':	/* look up value as a type... */
		if (0 == iColumn || (char *)0 == (pcScan = ppcValues[iColumn-1])) {
			fprintf(stderr, acError, progname, pcWhere, iColumn, "type");
			iRet = 1;
			break;
		}
		pOTDown = TypeParse(& pcScan);
		if ((OPTTYPE *)0 == pOTDown) {
			iRet = 1;
		} else {
			iRet = TypeEsc(ppcCursor, pOTDown, pEO, pcDst);
		}
		break;

	default:
	case 'z':	/* force an internal error */
		(void)CSpell(acSpell, cKey, 0);
		fprintf(stderr, "%s: key level: %s: %%%s: expander error\n", progname, acWhere, acSpell);
		iExit |= MER_PAIN;
		iRet = 1;
		break;

	case '/':	/* apply to all in list ... */
		pcBack = *ppcCursor;
		for (w = 0; w < wMany; ++w) {
			*ppcCursor = pcBack;
			if ((char *)0 == (pcScan = ppcValues[w])) {
				continue;
			}
			pcMem = ValueEsc(ppcCursor, pcScan, pcWhere, w+1, pOR, pEO);

			if ((char *)0 == pcMem) {
				iRet = 1;
				break;
			}
			ppcValues[w] = acStomp == pcMem ? (char *)0 : pcMem;
		}
		if (1 == iRet) {
			break;
		}
		continue;
	case '?':	/* find all (from here) that expands this */
		if (0 == iColumn) {
			fprintf(stderr, acError, progname, pcWhere, iColumn, "best");
			iRet = 1;
			break;
		}
		pcBack = *ppcCursor;
		pcMem = (char *)0;
		wCenter = iColumn-1;
		++iValTrap;
		for (w = wCenter; w < wMany; ++w) {
			*ppcCursor = pcBack;
			if ((char *)0 == (pcScan = ppcValues[w])) {
				continue;
			}
			pcMem = ValueEsc(ppcCursor, pcScan, pcWhere, w+1, pOR, pEO);
			if ((char *)0 == pcMem || acStomp == pcMem) {
				continue;
			}
			ppcValues[wCenter++] = pcMem;
		}
		--iValTrap;
		wMany = wCenter;
		continue;

	case -1:	/* try current one as a value operator on current */
	case -2:
		*ppcCursor = pcBack;
		/* fall through */
	case '.':
		if (0 == iColumn || (char *)0 == (pcScan = ppcValues[iColumn-1])) {
			fprintf(stderr, acError, progname, pcWhere, iColumn, "angle");
			iRet = 1;
			break;
		}
		pcMem = ValueEsc(ppcCursor, pcScan, pcWhere, iColumn, pOR, pEO);
		if ((char *)0 == pcMem) {
			iExit |= MER_SEMANTIC;
			iRet = 1;
			break;
		}
		ppcValues[iColumn-1] = pcMem;
		continue;
	} break; }

	/* put the real values back after %a messed them up
	 */
	for (w = 0; w < wStart; ++w) {
		ppcValues[w] = ppcKeep[w];
	}
	/* we should free the ones that are different from here
	 * before we turn the list loose -- eh?
	 */
	free((char *)ppcKeep);
	pEO->pcarg2 = apcStall[1];
	pEO->pcarg1 = apcStall[0];
	/* no return stataments before we get to here: ...}
	 */

	if (0 != iRet) {
		iExit |= MER_SEMANTIC;
	}
	return iRet;
}
