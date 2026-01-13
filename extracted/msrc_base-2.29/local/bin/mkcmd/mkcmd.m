# this fragment contains the "real" options to mkcmd
# $Id: mkcmd.m,v 8.9 2000/08/17 14:31:05 ksb Exp $
from '<sys/param.h>'
from '<stdio.h>'
from '<pwd.h>'

requires "std_version.m" "std_help.m"
requires "util_ppm.m"
requires "scan.m" "mkcmd.mh"

%hi
/* $Id: mkcmd.m,v 8.9 2000/08/17 14:31:05 ksb Exp $
 */
#if !defined(MER_INV)
#define MER_INV		0x20	/* internal problem			*/
#define MER_SEMANTIC	0x10	/* strange idea, but we can't		*/
#define MER_LIMIT	0x08	/* internal limit problem		*/
#define MER_SYNTAX	0x04	/* syntax in input file			*/
#define MER_BROKEN	0x02	/* oush! what is this?			*/
#define MER_PAIN	0x01	/* we don't even know			*/
#endif

extern FILE *Search(/* char *pcFile, char *pcMode, int (**pfiClose)() */);
%%

%i
static char rcsid[] =
	"$Id: mkcmd.m,v 8.9 2000/08/17 14:31:05 ksb Exp $";

/* mkcmd itself is build with -G so we can use the same main.c
 * file on multiple source platforms,  most people should let mkcmd
 * do the hard work.  mkcmd rembers how it was built and builds all
 * subsequent parsers that way...
 */
#if !defined(MSDOS)
extern int errno;
#endif


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

%%

from '<sys/types.h>'
from '"machine.h"'

getenv "MKCMD"
basename "mkcmd" ""

string variable "acOutMem" {
	init '"%s: out of memory\\n"'
}


# the next three options show how to use an external function

before {
	update 'Init(rcsid);'
}

boolean variable "fLanguage" { }

action "c" {
	hidden update "fLanguage = 0;"
	help "generate C code"
}

boolean "K" {
	named "fKeyTable"
	help "output a table of all the keys defined for this product"
}


# C output Boolean switches
boolean 'A' {
	named "fAnsi"
	help "generate ANSI function prototype headers"
}

boolean 'G' {
	named "fWeGuess"
	init "1"
	help "do not guess a C environment, let user code guess"
}
boolean 'P' {
	named "fCppLines"
	init "1"
	help "suppress most #line control information"
}

accum "I" {
	static named "pcSearchPath"
	initializer '"."'
	parameter "directory"
	help "search directory to search for source files"
}
char* "B" {
	named "pcBootModule"
	param "boot.m"
	init '"getopt.m"'
	after '%R<every>n(%n);'
	help "our boiler plate module to find getopt and the like"
}
char* "O" {
	hidden named "pcOutPlate"
	param "output.mo"
	init '"default.mo"'
	help "the meta expander output template"
}

char* "m" {
	local named "pchManPage"
	parameter "manpage"
	help "build a manpage template in this file"
}

boolean "M" {
	named "fMaketd"
	help "output transitive dependencies information"
}

char* "n" {
	named "pchBaseName"
	parameter "name"
	initializer '"prog"'
	help "generate files under this alternate basename"
}

action 'T' {
	from '"type.h"'
	named "DumpTypes"
	update "%n(stdout);"
	aborts "exit(0);"
	help "display supported type table"
}

function 'E' {
	from '"emit.h"'
	named "EmitHelp"
	param "expander"
	update "%n(stdout, %N);"
	aborts "exit(0);"
	help "display expander help"
}

boolean 'v' {
	hidden named 'fVerbose'
	help "option to debug mkcmd"
}

string variable "acDefTDir" {
	init 'DEFDIR'
}

augment action 'V' {
	user 'printf("%%s: templates from `%%s%rId%%s\'\\n", %b, %rIn, acDefTDir);'
}

after {
	update '%rIn = optaccum(%rIn, acDefTDir, "%rIqd");'
}

every {
	from '"parser.h"'
	param "files"
	named "Config"
	update '%n(%N);'
	help "specify files containing option descriptions"
	cleanup "Cleanup();"
}

zero {
	from '"parser.h"'
	named "Config"
	update '%n("-");'
}

integer named "iExit" {
	init "0"
	help "the global exit code we've needed for so long"
}
exit {
	from '"parser.h"'
	named "Gen"
	before "%n(%rnn, %rmn);"
	update ""
	abort "exit(iExit);"
}

%c
/* Open the file, or if we can't do that try explode(1).		(ksb)
 * You won't believe what this does to reduce the number of modules
 * in /usr/local/lib/mkcmd (cmd.m becomes 20 fewer files)
 * a trailing "-module" means pull the module from the file, if it
 * exists.
 *
 * So fopen("cmd-fred.m") -> popen("explode -o - -u fred cmd.m", "r")
 * Less obvious, "cmd-pwd,cd.m" gets both due to "fine line" action,
 * of course they are intermixed a little.
 *
 * We should prefer the .e file to the .m, so
 *	cmd.m	general interface
 *	cmd.e	the optional part we can add ("cmd-pwd.m" finds it anyway)
 *	cmd.mi	the include section
 *	cmd.ei	the optional include section ("cmd-pwd.mi" finds it)
 * same for .mh (.eh) and .mc (.ec) files.
 *
 * We also try for in the last component of a module name (kind_mod.m):
 *	kind/mod.m
 * this is for hosts with very limited file names lengths.
 */
static FILE *
Explode(pcModule, pcMode, ppfiClose)
char *pcModule, *pcMode;
int (**ppfiClose)();
{
	register FILE *fp;
	register char *pcMinus, *pcBar, *pcDot, *pcCheck, *pcExt;
	auto char acCmd[MAXPATHLEN*2+500];

	*ppfiClose = fclose;
	if ((FILE *)0 != (fp = fopen(pcModule, pcMode))) {
		if (fMaketd) {
			printf("%s.c: %s\n", pchBaseName, pcModule);
		}
		return fp;
	}

	/* We prefer kind/mod.m to kind_mod.m -- strangely
	 * note that in /usr/local_1/lib/mkcmd/util_tmp.m
	 * we will only try the last "_" once.
	 */
	if ((char *)0 == (pcBar = strrchr(pcModule, '/'))) {
		pcBar = pcModule;
	}
	if ((char *)0 != (pcBar = strrchr(pcBar, '_'))) {
		*pcBar = '/';
		fp = Explode(pcModule, pcMode, ppfiClose);
		*pcBar = '_';
		if ((FILE *)0 != fp) {
			return fp;
		}
	}

	if ((char *)0 == (pcMinus = strrchr(pcModule, '-'))) {
		return (FILE *)0;
	}

	*pcMinus++ = '\000';
	if ((char *)0 != (pcDot = strrchr(pcMinus, '.'))) {
		*pcDot++ = '\000';
	}
	(void)sprintf(acCmd, "explode -o - -u %s ", pcMinus);
	pcCheck = acCmd+strlen(acCmd);
	(void)strcat(acCmd, pcModule);
	pcExt = pcCheck+strlen(pcCheck);
	if ((char *)0 != pcDot) {
		*--pcDot = '.';
		(void)strcpy(pcExt, pcDot);
		if ('m' == pcExt[1]) {
			pcExt[1] = 'e';
			if ((FILE *)0 != (fp = fopen(pcCheck, "r"))) {
				goto good;
			}
		}
		(void)strcpy(pcExt, pcDot);
	}
	*--pcMinus = '-';

	/* no explode archive to extract from
	 */
	if ((FILE *)0 == (fp = fopen(pcCheck, "r"))) {
		return (FILE *)0;
	}
 good:
	(void)fclose(fp);
	if (fMaketd) {
		printf("%s.c: %s\n", pchBaseName, pcCheck);
	}
	if (fVerbose) {
		printf("%s: explode %s|\n", progname, acCmd);
	}
	if ((FILE *)0 == (fp = popen(acCmd, pcMode))) {
		fprintf(stderr, "%s: popen: explode: %s\n", progname, strerror(errno));
	}
	*ppfiClose = pclose;
	return fp;
}

/* Search for a file in the specified path				(ksb)
 */
FILE *
Search(pchFile, pcMode, ppfiClose)
char *pchFile, *pcMode;
int (**ppfiClose)();
{
	static char acSlash[4] = "/";
	register char *pchDir, *pchNext, *pchSlash;
	register FILE *fp;
	auto char acFName[1025];
#if TILDEDIR
	register struct passwd *pPWUser;
#endif

	if ((FILE *)0 != (fp = Explode(pchFile, pcMode, ppfiClose))) {
		return fp;
	}
	if ('/' == pchFile[0]) {
		return (FILE *)0;
	}
	for (pchDir = pcSearchPath; (char *)0 != pchDir; pchDir = pchNext) {
		pchNext = strchr(pchDir, ':');
		if ((char *)0 != pchNext) {
			*pchNext = '\000';
		}
#if TILDEDIR
		if ('~' == pchDir[0]) {
			if ((char *)0 == (pchSlash = strchr(pchDir, '/')))
				pchSlash = acSlash;
			pchSlash[0] = '\000';
			if ((struct passwd *)0 == (pPWUser = getpwnam(pchDir+1))) {
				fprintf(stderr, "%s: getpwname: %s: %s\n", progname, pchDir+1, strerror(errno));
				exit(MER_BROKEN);
			}
			(void)strcpy(acFName, pPWUser->pw_dir);
			pchSlash[0] = '/';
			(void)strcat(acFName, pchSlash);
		} else {
			(void)strcpy(acFName, pchDir);
		}
#else
		(void)strcpy(acFName, pchDir);
#endif
		if ((char *)0 != pchNext) {
			*pchNext++ = ':';
		}
		pchSlash = strrchr(acFName, '/');
		if ((char *)0 != pchSlash && '\000' == pchSlash[1]) {
			do {
				if (acFName == pchSlash)
					break;
				*pchSlash-- = '\000';
			} while ('/' == pchSlash[0]);
		}
		(void)strcat(acFName, "/");
		(void)strcat(acFName, pchFile);
		if (fVerbose) {
			fprintf(stderr, "%s: looking for `%s\'\n", progname, acFName);
		}
		if ((FILE *)0 != (fp = Explode(acFName, pcMode, ppfiClose))) {
			return fp;
		}
	}
	return (FILE *)0;
}
%%
