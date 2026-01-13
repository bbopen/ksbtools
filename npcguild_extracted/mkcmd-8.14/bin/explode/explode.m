#!mkcmd
# $Mkcmd(*): ${mkcmd-mkcmd} %f
# mkcmd script for explode's user interface
#
from '"machine.h"'
from '"explode.h"'

require "std_help.m" "std_version.m"

%i
static char rcsid[] =
	"$Id: explode.m,v 6.1 2000/06/17 22:24:17 ksb Exp $";
%%
%h
extern void SetDelExt(/* char * */);
%%
string named "acDefIncl" {
	init 'DEFINCL'
}

getenv "EXPLODE"
basename "explode" ""

boolean 't' {
	exclude "nfisTH"
	named "fTable"
	help "provide a table of contents output (like tar)"
}
boolean 's' {
	exclude "cduTH"
	named "fWhole"
	help "output the source file rather than the parts"
	boolean 'S' {
		named "fPrint"
		help "just display the source path"
	}
}
boolean 'c' {
	exclude "st"
	named "fHeader"
	init "1"
	help "do not include any Header section in the output"
}

boolean 'f' {
	named "fOverwrite"
	initializer "0"
	help "force overwrite of files that exist"
}
boolean 'n' {
	named "fActive"
	initializer "1"
	help "report what would be done, but don't do it"
}
boolean 'v' {
	named "fVerbose"
	help "report actions as they happen"
}
char* 'e' {
	named "pcExt"
	parameter "extender"
	help "specify an extender for all created files"
}
char* 'd' {
	named "pcDel"
	parameter "delimiter"
	help "specify a different delimiter (rather than /*@)"
}
boolean 'i' {
	named "fAsk"
	help "be interactive, like rm"
}
char* 'o' {
	named "pcName"
	parameter "prefix"
	help "specify a prefix for all created filenames"
}
accum[":"] 'I' {
	named "pcIncl"
	param "dir"
	help "list of directories to search for source files"
}
accum[","] 'u' {
	named "pcUnpack"
	parameter "unpack"
	help "specify a limited number of routines to unpack"
}
augment action 'V' {
	user 'printf("%%s: default repository %%s\\n", %b, acDefIncl);'
}
action 'T' {
	named "DumpTable"
	update "%n(stdout);"
	help "list the recognized file extenders"
	abort "exit(0);"
}
action 'H' {
	named "Details"
	update "%n(stdout);"
	help "give some details of the file format expected"
	abort "exit(0);"
}

after {
	named "SetGlobals"
	update '%n();%rIn = optaccum(%rIn, acDefIncl, "%rId");'
}
every {
	named "UnPack"
	update "%n(%a, %ron);"
	parameter "files"
	help "the files to unpack"
}
zero {
	update "%Ren((char *)0, %ron);"
}
exit {
	named "RemoveTemp"
	update "%n();"
}

int variable "iDelLen" {
	init "3"
	help "escape delimiter size"
}
letter variable "cClose" {
	init "'@'"
	help "close an Explode directive"
}

# implementation details
%i
typedef struct LInode {
	char *pclang;		/* which language			*/
	char *pccomment;	/* the comment introducer		*/
	char *pcext;		/* the extender we expect		*/
	char cclose;		/* if not last character in comment	*/
} LANG_INFO;
%%

%c
static LANG_INFO LITable[] = {
	{"C",		"/*@",	".c",	'@'},
	{"Bourne Shell","#@",	".sh",	'@'},
	{"C Shell",	"#@",	".csh",	'@'},
	{"C header",	"/*@",	".h",	'@'},
	{"C++ implementation",	"//@",	".ccP",	'@'},
	{"C++ interface",	"//@",	".hP",	'@'},
	{"FORTRAN",	"c@",	".f",	'@'},
	{"include",	"/*@",	".i",	'@'},
	{"K",		"/*@",	".k",	'@'},
	{"Pascal",	"(*@",	".p",	'@'},	/* could  be {@ too @}) */
	{"Tcl",		"; @",	".tcl",	'@'},
	{"elisp",	";@",	".el",	'@'},	/* comment correct? */
	{"lex",		"/*@",	".l",	'@'},
	{"html",	"<!@",	".html",'@'},
	{"m4",		"dnl @",".m4",	'@'},
	{"make",	"#@",	".mk",	'@'},
	{"mkcmd",	"#@",	".m",	'@'},
	{"perl",	"#@",	".pl",	'@'},
	{"ray shade",	"/*@",	".ray",	'@'},
	{"tcl",		"#@",	".tcl",	'@'},
	{"X tool kit",	"#@",	".tk",	'@'},
	{"X tool kit",	"#@",	".wish",'@'},
	{"yacc",	"/*@",	".y",	'@'},
	{"nroff",	".\\\"@", ".man", '@'},
	{"nroff",	".\\\"@", ".ms",  '@'},
	{"nroff",	".\\\"@", ".me",  '@'},
	{"nroff",	".\\\"@", ".mm",  '@'},
	{"nroff",	".\\\"@", ".nro", '@'},
};
#define MAXLANG		(sizeof(LITable)/sizeof(struct LInode))

/* output the table above to the user					(ksb)
 */
static void
DumpTable(fp)
FILE *fp;
{
	register int i;
	register unsigned u1, u2;

	u1 = u2 = 7;
	for (i = 0; i < MAXLANG; ++i) {
		register unsigned u;
		u = strlen(LITable[i].pclang);
		if (u > u1) {
			u1 = u;
		}
		u = strlen(LITable[i].pccomment);
		if (u > u2) {
			u2 = u;
		}
	}
	for (i = 0; i < MAXLANG; ++i) {
		printf("%-*s %-*s %s\n", u1, LITable[i].pclang, u2, LITable[i].pccomment, LITable[i].pcext);
	}
}

static char *apchHelp[] = {
	"%sAppend file    divert text to the end of file\n",
	"%sHeader         divert text to the header file\n",
	"%sExplode file   copy header to file then divert text to file\n",
	"%sRemove         divert text to /dev/null\n",
	"%sInsert string  insert text in ouput stream\n",
	"%sShell string   sh(1) text, put ouput in stream\n",
	"%sVersion string mark the version of a module\n",
	"%sMessage        divert text to /dev/tty\n",
	(char *)0
};

/* a better help for the user						(ksb)
 * This should be enough
 */
static void
Details(fp)
register FILE *fp;
{
	register char **ppc, *pcLine, *pcTemp;

	pcTemp = (char*)0 != pcDel ? pcDel : LITable[0].pccomment;
	for (ppc = & apchHelp[0]; (char *)0 != (pcLine = *ppc); ++ppc) {
		fprintf(fp, pcLine, pcTemp);
	}
}

/* set the delimiter from the file extender				(ksb)
 */
void
SetDelExt(pcFile)
char *pcFile;
{
	static int fFakeExt = 0;
	static int fFakeDel = 0;
	register char *pc;
	register int i = MAXLANG;

	if (((char *)0 != (pc = pcExt) && !fFakeExt) ||
	    (char *)0 != (pc = strrchr(pcFile, '.'))) {
		for (i = 0; i <MAXLANG; ++i) {
			if (0 == strcmp(LITable[i].pcext, pc)) {
				break;
			}
		}
	}

	if (MAXLANG == i) {
		i = 0;			/* default to C		*/
	}

	if ((char *)0 == pcExt || fFakeExt) {
		fFakeExt = 1;
		pcExt = LITable[i].pcext;
	}

	if ((char *)0 == pcDel || fFakeDel) {
		fFakeDel = 1;
		pcDel = LITable[i].pccomment;
	}
	iDelLen = strlen(pcDel);
	if ('\000' == (cClose = LITable[i].cclose)) {
		cClose = pcDel[iDelLen - 1];
	}
}
%%
