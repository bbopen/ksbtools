/* $Id: scan.c,v 8.3 2009/10/14 16:18:03 ksb Exp $
 * scan mkcmd confg files
 *	$Compile: ${CC-cc} -c ${DEBUG--g} -I/usr/include/local %f
 */
#include <stdio.h>
#include <ctype.h>
#include <sys/param.h>

#include "machine.h"
#include "main.h"
#include "scan.h"
#include "list.h"
#include "mkcmd.h"
#include "stracc.h"
#include "parser.h"

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

static TENTRY aTETokens[] = {
	{ SEMI,		";",		1 }, /* end set			*/
	{ TABORTS,	"aborts",	3 }, /* we abort on this option	*/
	{ TACCUM,	"accumulate",	3 }, /* collent all values	*/
	{ TACTION,	"action",	3 }, /* action on switch	*/
	{ TAFTER,	"after",	3 }, /* action after parameters	*/
	{ TAUGMENT,	"augment",	3 }, /* add to option attrs	*/
	{ TBADOPT,	"badoption",	3 }, /* illegal option letter	*/
	{ TBASENAME,	"basename",	3 }, /* get basename of argv[0]	*/
	{ TBEFORE,	"before",	3 }, /* action to init program	*/
	{ TBOOLEAN,	"boolean",	3 }, /* make a boolean switch	*/
	{ TCHARP,	"char*",	3 }, /* make a char *		*/
	{ TCLEANUP,	"cleanup",	3 }, /* undo an init pre-exit	*/
	{ TCOMMENT,	"comment",	3 }, /* added a header comment	*/
	{ TDOUBLE,	"double",	3 }, /* make a float parameter	*/
	{ TDYNAMIC,	"dynamic",	3 }, /* init is runtime expr	*/
	{ TENDS,	"ends",		3 }, /* worse than stops	*/
	{ TESCAPE,	"escape",	3 }, /* for more/vi pr/sort	*/
	{ TEVERY,	"every",	3 }, /* action for every argument*/
	{ TEXCLUDES,	"excludes",	3 }, /* option excludes others	*/
	{ TEXIT,	"exit",		3 }, /* action before exit	*/
	{ TFD,		"fd",		2 }, /* base type for fds	*/
	{ TFILE,	"file",		3 }, /* base type for FILEs	*/
	{ TFROM,	"from",		3 }, /* file it import/include	*/
	{ TFUNCTION,	"function",	3 }, /* make a function		*/
	{ TGETENV,	"getenv",	3 }, /* name env var to read	*/
	{ TGLOBAL,	"global",	3 }, /* opposite of local	*/
	{ THELP,	"help",		3 }, /* set help text for option*/
	{ THIDDEN,	"hidden",	3 }, /* option is hidden	*/
	{ TINITIALIZER,	"initializer",	3 }, /* set the C initializer	*/
	{ TINTEGER,	"integer",	3 }, /* make an int parameter	*/
	{ TKEEP,	"keep",		3 }, /* the unconverted value	*/
	{ TKEY,		"key",		3 }, /* API key definition	*/
	{ TLATE,	"late",		3 }, /* postpone conversion	*/
	{ TLEFT,	"left",		3 }, /* left justified args	*/
	{ TLETTER,	"letter",	3 }, /* base type for awk -F:	*/
	{ TLIST,	"list",		3 }, /* pass the user the list	*/
	{ TLOCAL,	"local",	3 }, /* make an opt var local	*/
	{ TLONG,	"long",		3 }, /* make a long int		*/
	{ TMIX,		"mix",		3 }, /* mix options and args	*/
	{ TNAMED,	"named",	3 }, /* name option paramter var*/
	{ TNOPARAM,	"noparameter",	3 }, /* no parameter found	*/
	{ TNUMBER,	"number",	3 }, /* -number, like nice has	*/
	{ TONCE,	"once",		3 }, /* option given only once	*/
	{ TOTHERWISE,	"otherwise",	3 }, /* give an otherwise switch*/
	{ TPARAMETER,	"parameter",	3 }, /* name the parameter	*/
	{ TPOINTER,	"pointer",	3 }, /* make a pointer to char	*/
	{ TPROTOTYPE,	"prototype",	3 }, /* give a prototype string	*/
	{ TREQUIRES,	"requires",	3 }, /* force another source mod*/
	{ TRIGHT,	"right",	3 }, /* right justified args	*/
	{ TROUTINE,	"routine",	3 }, /* change function name	*/
	{ TSTATIC,	"static",	3 }, /* opt var or init, static	*/
	{ TSTOPS,	"stops",	3 }, /* we stop parsing options */
	{ TSTRING,	"string",	3 }, /* make a string option	*/
	{ TTEMPLATE,	"template",	3 }, /* give a template		*/
	{ TTERSE,	"terse",	3 }, /* give terse help name	*/
	{ TTOGGLE,	"toggle",	3 }, /* make a toggle		*/
	{ TTRACK,	"track",	3 }, /* set when data provided	*/
	{ TTYPE,	"type",		3 }, /* library type line	*/
	{ TUNSIGNED,	"unsigned",	3 }, /* unsigned type		*/
	{ TUPDATE,	"update",	3 }, /* give update action	*/
	{ TUSAGE,	"usage",	3 }, /* the usage function	*/
	{ TUSER,	"user",		3 }, /* after update do this	*/
	{ TVARIABLE,	"variable",	3 }, /* a variable to declare	*/
	{ TVECTOR,	"vector",	3 }, /* help vector		*/
	{ TVERIFY,	"verify",	3 }, /* verify parameter data	*/
	{ TZERO,	"zero",		3 }, /* on zero arguments	*/
	{ LCURLY,	"{",		1 }, /* begin definition	*/
	{ RCURLY,	"}",		1 }  /* end definition		*/
};

FILE
	*fpDivert,	/* file to accumulate user C code in		*/
	*fpHeader,	/* file to accumulate user header code in	*/
	*fpTop;		/* file to accumulate user C includes in	*/


/* look for a token in the above list, return which one it is,		(ksb)
 * or abort with an error.  (binary search is fast enough)
 */
static int
LookUp(pch, n)
char *pch;
int n;
{
	register int i;
	register int cf;

	for (i = 0; i < sizeof(aTETokens)/sizeof(TENTRY); ++i) {
		if (0 == (cf = strncmp(pch, aTETokens[i].acname, n))) {
			if (n < aTETokens[i].iunq) {
				fprintf(stderr, "%s: %s(%d) not enough characters make unique keyword '%s'\n", progname, pchScanFile, iLine, pch);
				exit(MER_SYNTAX);
			}
			return aTETokens[i].tvalue;
		}
		if (0 > cf) {
			break;
		}
	}
	fprintf(stderr, "%s: %s(%d) unknown keyword '%s'\n", progname, pchScanFile, iLine, pch);
	exit(MER_SYNTAX);
	/*NOTREACHED*/
}

static int
	c = '\n';
static char
	acStdin[] = "stdin";
static FILE
	*fpIn = (FILE *)0;

int
	iLine = 0;
char
	*pchScanFile = acStdin;

/* set a file name and stream						(ksb)
 * prime the LL1 look-ahead token
 */
void
SetFile(fpFile, pchEak)
FILE *fpFile;		/* file pointer to stream to read		*/
char *pchEak;		/* what name to print for errors		*/
{
	static char acNote[] = "/* from %s */\n";
	c = '\n';
	iLine = 0;
	pchScanFile = (char *)0 == pchEak ? acStdin : pchEak;
	fpIn = (FILE *)0 == fpFile ? stdin : fpFile;
	fprintf(fpDivert, acNote, pchEak);
	fprintf(fpHeader, acNote, pchEak);
	fprintf(fpTop, acNote, pchEak);
}

/* This is a not-so-simple scanner					(ksb)
 *  read keywords, strings ("...", '...') and EOF, passed meta markup
 */
TOKEN
GetTok()
{
	static char acIdent[90] = "ksb";
	static char acHackMark[] = "#line %d \"%s\"\n";
	static int fEof = 0;
	register int oc, iMask;
	register FILE *fp_Divert, *fp_Header, *fp_Top, *fp_Mkcmd;
	auto int i;
	auto char acMakeTemp[MAXPATHLEN+4];

	while (EOF != c && !fEof) {
		if (isspace(c) || ',' == c) {
			do {
				if ('\n' == c)
					++iLine;
				c = getc(fpIn);
			} while (isspace(c) || ',' == c);
			continue;
		}
		if ('#' == c) {
			do {
				c = getc(fpIn);
			} while ('\n' != c);
			continue;
		}
		if (isalpha(c) || '_' == c || '*' == 'c') {
			for (i = 0; i < 32 && (isalnum(c) || '_' == c || '*' == c); ++i) {
				acIdent[i] = c;
				c = getc(fpIn);
			}
			if (i < 32)
				acIdent[i] = '\000';
			return LookUp(acIdent, i);
		}
		if (isdigit(c)) {
			FirstChar(c, 32);
			c = getc(fpIn);
			while (isdigit(c)) {
				AddChar(c, 8);
				c = getc(fpIn);
			}
			AddChar('\000', 8);
			return INTEGER;
		}
		oc = c;
		c = getc(fpIn);
		switch (oc) {
		case '[':
			return LBRACKET;
		case ']':
			return RBRACKET;
		case '{':
			return LCURLY;
		case '}':
			return RCURLY;
		case ';':
			return SEMI;
		case '"':
		case '\'':
			i = 0;
			while (c != oc) {
				if (EOF == c) {
					fprintf(stderr, "%s: %s(%d) unexpected end of file in string\n", progname, pchScanFile, iLine);
					exit(MER_SYNTAX);
				}
				if ('\\' == c) {
					c = getc(fpIn);
				}
				if (0 == i) {
					FirstChar(c, 32);
					++i;
				} else {
					AddChar(c, 80);
				}
				if ('\n' == c) {
					++iLine;
				}
				c = getc(fpIn);
			}
			if (0 == i) {
				FirstChar('\000', 16);
			} else {
				AddChar('\000', 1);
			}
			c = ' ';
			return STRING;
		case '%':		/* pseudo end of file mark	*/
			iMask = 0;
			while ('\n' != c) {
				if ('@' == c) {	/* allow %@c */
					iMask |= 0x10;
					c = getc(fpIn);
				}
				if ('c' == c || '%' == c) {
					iMask |= 0x01;
				} else if ('h' == c) {
					iMask |= 0x02;
				} else if ('i' == c) {
					iMask |= 0x04;
				} else if ('m' == c) {
					iMask |= 0x08;
				} else {
					fprintf(stderr, "%s: %s(%d) bad character '%c' in %% token\n", progname, pchScanFile, iLine, c);
					exit(MER_SYNTAX);
				}
				c = getc(fpIn);
			}
			++iLine;
			if (0 == iMask || 0x10 == iMask) {
				fprintf(stderr, "%s: %s(%d) %% token missing qualifier\n", progname, pchScanFile, iLine);
				exit(MER_SYNTAX);
			}
			fp_Divert = fpDivert;
			fp_Header = fpHeader;
			fp_Top = fpTop;
			fp_Mkcmd = stderr;
			if (0 != (0x10 & iMask)) {
				if (0 != (1 & iMask)) {
					strcpy(acMakeTemp, "/tmp/mkmcXXXXXX");
					fp_Divert = MakeDivert(strdup(acMakeTemp), 'c');
				}
				if (0 != (2 & iMask)) {
					strcpy(acMakeTemp, "/tmp/mkmhXXXXXX");
					fp_Header = MakeDivert(strdup(acMakeTemp), 'h');
				}
				if (0 != (4 & iMask)) {
					strcpy(acMakeTemp, "/tmp/mkmiXXXXXX");
					fp_Top = MakeDivert(strdup(acMakeTemp), 'i');
				}
				/* %@ means %m (aka %@m)
				 */
				if (0 == (7 & iMask)) {
					iMask |= 8;
				}
			}
			if (0 != (1 & iMask)) {
				fprintf(fp_Divert, acHackMark, iLine, pchScanFile);
			}
			if (0 != (2 & iMask)) {
				fprintf(fp_Header, acHackMark, iLine, pchScanFile);
			}
			if (0 != (4 & iMask)) {
				fprintf(fp_Top, acHackMark, iLine, pchScanFile);
			}
			if (0 != (8 & iMask)) {
				strcpy(acMakeTemp, "/tmp/mkmmXXXXXX");
				fp_Mkcmd = MakeDivert(strdup(acMakeTemp), 'm');
			}
			while (EOF != (c = getc(fpIn))) {
 kmp1:
				if (0 != (1 & iMask)) {
					putc(c, fp_Divert);
				}
				if (0 != (2 & iMask)) {
					putc(c, fp_Header);
				}
				if (0 != (4 & iMask)) {
					putc(c, fp_Top);
				}
				if (0 != (8 & iMask)) {
					putc(c, fp_Mkcmd);
				}
				if ('\n' != c) {
					continue;
				}
				++iLine;
				if (EOF == (c = getc(fpIn))) {
					break;
				}
				if ('%' != c) {
					goto kmp1;
				}
				if (EOF == (c = getc(fpIn))) {
					if (0 != (1 & iMask)) {
						fputc('%', fp_Divert);
					}
					if (0 != (2 & iMask)) {
						fputc('%', fp_Header);
					}
					if (0 != (4 & iMask)) {
						fputc('%', fp_Top);
					}
					if (0 != (8 & iMask)) {
						fputc('%', fp_Mkcmd);
					}
					break;
				}
				if ('%' != c) {
					if (0 != (1 & iMask)) {
						fputc('%', fp_Divert);
					}
					if (0 != (2 & iMask)) {
						fputc('%', fp_Header);
					}
					if (0 != (4 & iMask)) {
						fputc('%', fp_Top);
					}
					if (0 != (8 & iMask)) {
						fputc('%', fp_Mkcmd);
					}
					goto kmp1;
				}
				c = getc(fpIn);
				break;
			}
			if (fp_Divert != fpDivert) {
				fclose(fp_Divert);
			}
			if (fp_Header != fpHeader) {
				fclose(fp_Header);
			}
			if (fp_Top != fpTop) {
				fclose(fp_Top);
			}
			if (fp_Mkcmd != stderr) {
				fclose(fp_Mkcmd);
			}
			if (EOF == c) {
				break;
			}
			continue;
		case '/':
			if ('*' != c) {
				fprintf(stderr, "%s: %s(%d) error in C comment\n", progname, pchScanFile, iLine);
				exit(MER_SYNTAX);
			}
			fputc('/', fpTop);
			fputc('*', fpTop);
			i = iLine;
			c = ' ';
			do {
				oc = c;
				if (EOF == (c = getc(fpIn))) {
					fprintf(stderr, "%s: %s(%d) unclosed C comment (from line %d)\n", progname, pchScanFile, iLine, i);
					exit(MER_SYNTAX);
				}
				if ('\n' == c) {
					++iLine;
				}
				putc(c, fpTop);
			} while ('*' != oc && '/' != c);
			fputc('\n', fpTop);
			c = ' ';
			continue;
		default:
			/* unknown character XXX */
			break;
		}
	}
	return TEOF;
}

/* turn a token back into text for error messages			(ksb)
 * only used in error messages -- so it can be slow
 */
char *
TextTok(iTok)
int iTok;
{
	register int i;

	switch (iTok) {
	case INTEGER:
		return "<integer>";
	case STRING:
		return "\"string\"";
	case TEOF:
		return "[EOF]";
	default:
		break;
	}
	for (i = 0; i < sizeof(aTETokens)/sizeof(TENTRY); ++i) {
		if (iTok == aTETokens[i].tvalue)
			return aTETokens[i].acname;
	}
	return "unknown";
}
