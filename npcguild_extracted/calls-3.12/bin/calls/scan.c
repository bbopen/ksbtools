/*
 * $Id: scan.c,v 3.13 1996/12/23 23:04:16 kb207252 Dist $
 *
 * scan.c -- a simple scanner for C, pulls out the function calling pattern
 *
 * Copyright 1990 Purdue Research Foundation, West Lafayette, Indiana
 * 47907.  All rights reserved.
 *
 * Written by Kevin S Braunsdorf, ksb@cc.purdue.edu, purdue!ksb
 *
 * This software is not subject to any license of the American Telephone
 * and Telegraph Company or the Regents of the University of California.
 *
 * Permission is granted to anyone to use this software for any purpose on
 * any computer system, and to alter it and redistribute it freely, subject
 * to the following restrictions:
 *
 * 1. Neither the authors nor Purdue University are responsible for any
 *    consequences of the use of this software.
 *
 * 2. The origin of this software must not be misrepresented, either by
 *    explicit claim or by omission.  Credit to the authors and Purdue
 *    University must appear in documentation and sources.
 *
 * 3. Altered versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 4. This notice may not be removed or altered.
 */
#ifndef lint
static char copyright[] =
"@(#) Copyright 1990 Purdue Research Foundation.\nAll rights reserved.\n";
#endif

#if defined(BSD2_9)
#include <sys/types.h>
#endif	/* for void */

#include <sys/param.h>
#include <ctype.h>
#include <stdio.h>

#include "machine.h"
#include "scan.h"
#include "calls.h"

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif
#define strsave(X)	strcpy((char *)malloc(strlen((X))+1), (X))

int c;				/* parser look ahead			*/
int iLineNo;			/* line number in input file		*/
int iInclp = 0;			/* we are in an include file		*/
FILE *fpInput;			/* our input file pointer		*/
char *pcFilename;		/* file name we are looking for		*/
char acIncl[MAXPATHLEN+2];	/* file we are including		*/
char *pcIncl;			/* file we are including, saved		*/
HASH *pHTRoot[2] =	 	/* our list of idents			*/
	{nilHASH, nilHASH};
static char
#if HAVE_ASM
	ASM[] = "asm",
#endif
	AUTO[] = "auto",
	BREAK[] = "break",
	CASE[] = "case",
	CHAR[] = "char",
	CONST[] = "const",
	CONTINUE[] = "continue",
	DEFAULT[] = "default",
	DO[] = "do",
	DOUBLE[] = "double",
	ELSE[] = "else",
	ENUM[] = "enum",
	EXTERN[] = "extern",
	FLOAT[] = "float",
	FOR[] = "for",
	FORTRAN[] = "fortran",
	GOTO[] = "goto",
	IF[] = "if",
	INT[] = "int",
	LONG[] = "long",
	REGISTER[] = "register",
	RETURN[] = "return",
	SHORT[] = "short",
	SIGNED[] = "signed",
	SIZEOF[] = "sizeof",
	STATIC[] = "static",
	STRUCT[] = "struct",
	SWITCH[] = "switch",
	TYPEDEF[] = "typedef",
	UNION[] = "union",
	UNSIGNED[] = "unsigned",
	VOID[] = "void",
	VOLATILE[] = "volatile",
	WHILE[] = "while";


/* get a new hash node
 */
static HASH *
newHASH()
{
	register HASH *pHTRet;
	static HASH *pHTQueue = nilHASH;

	if (nilHASH == pHTQueue) {
		if (!(pHTRet = (HASH *)calloc(BUCKET, sizeof(HASH)))) {
			(void) fprintf(stderr, "%s: out of mem\n", pcProg);
			exit(2);
		}
		pHTQueue = (pHTRet+(BUCKET-1))->pHTnext = pHTRet;
	}
	pHTRet = pHTQueue;
	pHTQueue = pHTRet->pHTnext ? nilHASH : pHTRet+1;
	return pHTRet;
}


/* order the named in the list so they are easy to merge later		(ksb)
 */
static int
Order(pcLeft, pcRight)
char *pcLeft, *pcRight;
{
	register int i;
	i = strcasecmp(pcLeft, pcRight);
	if (0 == i)
		return strcmp(pcLeft, pcRight);
	return i;
}

/* translate name to hash node						(ksb)
 */
HASH *
Search(name, MustBeStatic, fInTree)
register char *name;
int MustBeStatic, fInTree;
{
	register HASH **ppHT, *pHT;
	register int i = 1;

	ppHT = & pHTRoot[1];	/* first search statics	*/
	while((pHT = *ppHT) && (i = Order(pHT->pcname, name)) <= 0) {
		if (0 == i &&
		   (0 == pHT->pcfile || 0 == strcmp(pcFilename, pHT->pcfile)))
			break;	/* found a visible static function	*/
		ppHT = & pHT->pHTnext;
		i = 1;
	}

	if (0 != i && ! MustBeStatic) {
		ppHT = & pHTRoot[0];
		while((pHT = *ppHT) && (i = Order(pHT->pcname, name)) < 0)
			ppHT = & pHT->pHTnext;
	}

	if (0 != i) {
		pHT = newHASH();
		pHT->pcname = strsave(name);
#if defined(BADCALLOC)		/* calloc does not zero mem?		*/
		pHT->iline = pHT->isrcline = pHT->isrcend = 0;
		pHT->listp = pHT->calledp = pHT->localp = pHT->inclp = 0;
#endif /* BADCALLOC */
		pHT->pcfile = (char *) 0;
		pHT->pINcalls = nilINST;
		pHT->pHTnext = *ppHT;
		pHT->intree = fInTree;
		*ppHT = pHT;
	} else {
		pHT->intree |= fInTree;
	}
	return pHT;
}


/*
 * Skip blanks, comments, "strings", 'chars' in input.
 *
 * Here we don't assume that cpp takes out comments, really
 * paranoid of us, but I think that way.
 * `f' is a flag we use to make the look ahead come out right
 * in all cases.
 */
void
eatwhite(f)
register int f;
{
	if (f)
		c = getc(fpInput);
	for(/* void */; /* c != EOF */; c = getc(fpInput)) {
		if (isspace(c) || c == '\b') {
			if ('\n' == c)
				++iLineNo;
			continue;
		} else if ('/' == c) {		/* start of comment? */
			if ('*' == (c = getc(fpInput))) {
				c = getc(fpInput);	/* eat comment */
				for(;;) {
					while (c != '*')
						c = getc(fpInput);
					if ('/' == (c = getc(fpInput)))
						break;
				}
			} else {
				ungetc(c, fpInput);
				c = '/';
				break;
			}
		} else if ('\'' == c || '"' == c) {
			while(c != (f = getc(fpInput))) {
				if ('\\' == f)
					getc(fpInput);
			}
		} else if ('#' == c) {
			register int j;

			fscanf(fpInput, " %d", &iLineNo);
			c = getc(fpInput);
			j = 0;
			if ('\"' == c) {
				for (/* j = 0 */; j < sizeof(acIncl); ++j)
					if ('\"' == (acIncl[j] = getc(fpInput)))
						break;
			}
			acIncl[j] = '\000';
			while ('\n' != c) {
				c = getc(fpInput);
			}
			iInclp = '\000' != acIncl[0] && 0 != strcmp(acIncl, pcFilename);
			if (iInclp) {
				register char *pcTemp;
				pcTemp = acIncl;
				while ('.' == pcTemp[0] && '/' == pcTemp[1])
					pcTemp += 2;
				if ((char *)0 == pcIncl || 0 != strcmp(pcIncl, pcTemp))
					pcIncl = strsave(pcTemp);
			}
			c = ' ';
		} else {
			break;
		}
	}
}

/* find balancing character
 */
void
balance(l, r)
register int l, r;
{
	register int brace = 1;

	do
		eatwhite(1);
	while ((brace += (l == c) - (r == c)) && ! feof(fpInput));
}

/* return 0 = var, 1 == func, 2 == keyword
 */
int
getid(pcId, ppcToken)
register char *pcId;
char **ppcToken;
{
	static char *keywords[] = {
#if HAVE_ASM
		ASM,
#endif
		AUTO,
		BREAK,
		CASE, CHAR, CONTINUE, CONST,
		DEFAULT, DO, DOUBLE,
		ELSE, ENUM, EXTERN,
		FLOAT, FOR, FORTRAN,
		GOTO,
		IF, INT,
		LONG,
		REGISTER, RETURN,
		SHORT, SIGNED, SIZEOF, STATIC, STRUCT, SWITCH,
		TYPEDEF,
		UNION, UNSIGNED,
		VOID, VOLATILE,
		WHILE,
		(char *)0
	};
	register int i = 0;
	register char **ppcKey = keywords;

	do {
		if (i < MAXCHARS)
			pcId[i++] = c;
		c = getc(fpInput);
	} while (isalpha(c) || isdigit(c) || '_' == c);
	pcId[i] = '\000';	/* buffer really goes to MAXCHARS+1	*/
	eatwhite(0);	/* c == next char after id */

	while (*ppcKey && 0 != strcmp(*ppcKey, pcId))
		++ppcKey;

	if (*ppcKey) {
		*ppcToken = *ppcKey;
		if (UNION == *ppcKey || STRUCT == *ppcKey) {
			while (isalpha(c) || isdigit(c) || '_' == c) {
				c = getc(fpInput);
			}
			eatwhite(0);	/* c == next char after tag */
			if ('{' == c) {
				balance(LCURLY, RCURLY);
			}
		}
		return 2;
	}

	return LPAREN == c;
}

/* eat anything that starts with any keyword listed
 */
void
eatdecl(pcId)
register char *pcId;
{
	static char *which[] = {	/* keywords mark a declaration	*/
#if HAVE_ASM
		ASM,
#endif
		AUTO,
		CHAR, CONST,
		DOUBLE,
		ENUM, EXTERN,
		FLOAT,
		INT,
		LONG,
		REGISTER,
		SIGNED, SHORT, STATIC, STRUCT,
		TYPEDEF,
		UNION, UNSIGNED,
		VOID, VOLATILE,
		(char *) 0
	};
	register char **ppc = which;

	while (*ppc && *ppc++ != pcId)
		/* nothing */;
	if (*ppc) {
		while ('=' != c && ';' != c && RPAREN != c && EOF != c) {
			if (LCURLY == c)
				balance(LCURLY, RCURLY);
			else if (LPAREN == c) {
				balance(LPAREN, RPAREN);
			}
			eatwhite(1);
		}
	}
}

/* get a new instaniation  node
 */
INST *
newINST()
{
	register INST *pINRet;
	static INST *pINQueue = nilINST;

	if (nilINST == pINQueue) {
		if (!(pINRet = (INST *)calloc(BUCKET, sizeof(INST)))) {
			(void) fprintf(stderr, "%s: out of mem\n", pcProg);
			exit(2);
		}
		pINQueue = (pINRet+(BUCKET-1))->pINnext = pINRet;
	}
	pINRet = pINQueue;
	pINQueue = pINRet->pINnext ? nilINST : pINRet+1;
	return pINRet;
}

/* inside a function looking for calls
 */
void
Level2(pHTCaller)
HASH *pHTCaller;
{
	static char buffer[MAXCHARS+1];
	register struct INnode *pINLoop;
	register int brace = 0;
	register HASH *pHTFound;
	register struct INnode **ppIN;
	register int declp = 1;		/* eating declarations		*/
	int keep;
	auto char *pcToken;

	ppIN = & pHTCaller->pINcalls;
	while (brace || declp) {
		if (isalpha(c) || '_' == c) {
			switch (keep = getid(buffer, & pcToken)) {
			case 0:
			case 1:
				pHTFound = Search(buffer, 0, 1);
				if (bReverse) {
					ppIN = & pHTFound->pINcalls;
					if (bAll) {
						goto regardless;
					}
					while (pINLoop = *ppIN) {
						if (pHTCaller == pINLoop->pHTname)
							break;
						ppIN = & pINLoop->pINnext;
					}
				} else if (bAll) {
					goto regardless;
				} else {
					pINLoop = pHTCaller->pINcalls;
					while (pINLoop) {
						if (pHTFound == pINLoop->pHTname)
							break;
						pINLoop = pINLoop->pINnext;
					}
				}
				if (! pINLoop) {
			regardless:
					pINLoop = newINST();
					pINLoop->pHTname = bReverse ? pHTCaller : pHTFound;
					pINLoop->pINnext = *ppIN;
					pINLoop->ffunc = keep;
					*ppIN = pINLoop;
					ppIN = & pINLoop->pINnext;
				}
				/* ++(bReverse ? pHTCaller : pHTFound)->calledp; */
				if (0 == keep && 0 == pHTFound->isrcend) {
					pHTFound->isrcend = -1;
				}
				if (pHTCaller != pHTFound) {
					++pHTFound->calledp;
					(bReverse ? pHTFound : pHTCaller)->intree = 1;
				}
				break;
			case 2:
				eatdecl(pcToken);
				/* fall through */
			}
		} else if (isdigit(c) || '.' == c) {
			/* this block eats the letters that can be in a
			 * number so that don't show as identifiers
			 */
			if ('0' == c && ('x' == (c = getc(fpInput)) || 'X' == c)) {
				do
					c = getc(fpInput);
				while (isxdigit(c));
			} else {
				while (isdigit(c))
					c = getc(fpInput);
				if ('.' == c) {
					do
						c = getc(fpInput);
					while (isdigit(c));
				}
				if ('e' == c || 'E' == c) {
					c = getc(fpInput);
					if ('+' == c || '-' == c) {
						do
							c = getc(fpInput);
						while (isdigit(c));
					}
				}
			}
			while ('l' == c || 'L' == c || 'u' == c || 'U' == c || 'f' == c || 'F' == c) {
				c = getc(fpInput);
			}
			eatwhite(0);
		} else if ( EOF == c ) {
			break;
		} else {
			if (LCURLY == c)
				declp = 0, ++brace;
			else if (RCURLY == c)
				--brace;
			if (! (brace || declp))	/* make line numbers correct */
				break;
			eatwhite(1);
		}
	}
}


/* in a C source program, looking for fnx(){..}
 */
void
Level1(given)
register char *given;
{
	static char buffer[MAXCHARS+1];
	static char *pcToken;
	register HASH *pHTTemp;
	register int parens = 0;
	register int Localp = 0;
	auto char *pcDef;
	auto int iStart;

	c = ' ';
	pcFilename = given;

	do {		/* looking to a function decl	*/
		if (isalpha(c) || '_' == c) {
			switch (getid(buffer, & pcToken)) {
			case 0:
				if (LOOK_VARS != fLookFor) {
					eatwhite(0);
					continue;
				}
				pHTTemp = Search(buffer, Localp, 0);
				pcDef = iInclp ? pcIncl : pcFilename;
				if ((char *)0 == pHTTemp->pcfile || pHTTemp->pcfile == acCmd) {
					pHTTemp->pcfile = pcDef;
					pHTTemp->isrcline = iLineNo;
					pHTTemp->isrcend = -1;
				}
				pHTTemp->localp = Localp;
				pHTTemp->inclp = iInclp;
				pHTTemp->pINcalls = 0;
				Localp = 0;
				eatwhite(0);
				continue;
			case 1:
				iStart = iLineNo;
				/* this breaks
				 * 	int (*signal())();
				 * parens = 0;
				 */
				while (parens += (LPAREN == c) - (RPAREN == c))
					eatwhite(1);
				for (;;) {	/* eat complex stuff	*/
					eatwhite(1);
					if (LPAREN == c) {
						balance(LPAREN, RPAREN);
						continue;
					} else if (LBRACK == c) {
						balance(LBRACK, RBRACK);
						continue;
					} else {
						break;
					}
				}
				pHTTemp = Search(buffer, Localp, 0);
				if (',' == c || ';' == c) {
					Localp = 0;
					continue;
				}
				pcDef = iInclp ? pcIncl : pcFilename;
				if ((char *)0 != pHTTemp->pcfile && !(pHTTemp->pcfile == acCmd || 0 == pHTTemp->isrcline)) {
					(void) fprintf(stderr, "%s: %s is multiply defined [%s(%d), %s(%d)]\n", pcProg, pHTTemp->pcname, pHTTemp->pcfile, pHTTemp->isrcline, pcDef, iLineNo);
				} else {
					pHTTemp->pcfile = pcDef;
				}
				pHTTemp->localp = Localp;
				pHTTemp->inclp = iInclp;
				pHTTemp->isrcline = iStart;
				Localp = 0;
				Level2(pHTTemp);
				pHTTemp->isrcend = iLineNo;
				eatwhite(1);
				continue;
			case 2:
				if (STATIC == pcToken)
					Localp = 1;
				continue;
			}
		} else if (LCURLY == c) {
			balance(LCURLY, RCURLY);
		} else if (LPAREN == c) {
			++parens;
		} else if (RPAREN == c) {
			--parens;
		} else if ('*' != c) {
			Localp = 0;
		}
		eatwhite(1);
	} while (EOF != c);
}
