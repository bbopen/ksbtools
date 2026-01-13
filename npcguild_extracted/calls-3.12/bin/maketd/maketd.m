#!mk -mMkcmd
# $Mkcmd: ${mkcmd-mkcmd} std_help.m std_version.m %f
#
from '<sys/types.h>'
from '<sys/signal.h>'

from '"machine.h"'
from '"abrv.h"'
from '"srtunq.h"'

basename "maketd" ""
basename "mkdep" "-P"

init "init_sigs();"
init "srtinit(&abrv);"

%h
extern void
	RestoreFiles(),		/* clean up files on errors		*/
	OutOfMemory();		/* ran out of memory			*/
%%

%i
static char rcsid[] =
	"$Id: maketd.m,v 4.10 1997/11/10 19:06:24 ksb Exp $";

DependInfo FileInfo = {	/* our template for all files			*/
	0, 0,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	(DependInfo *)0
};
DependInfo **ppDICursor = & pDIList;

extern int errno;
%%

augment action 'V' {
	user 'printf("%%s: cpp command \\"%%s\\"\\n", %b, PATH_CPP);'
}

%c
/* This routine is called whenever there is a fatal error or signal.	(ksb)
 * It attempts to restore the origional makefile from the backup if the
 * file has been backed up. It leaves the backup file sitting around
 * either way.
 */
void
RestoreFiles()
{
	static char sbNoFiles[] = "%s: No files to save\n";

	if ((char *)0 == pcOrig) {
		fprintf(stderr, sbNoFiles, progname);
		exit(3);
		/*NOTREACHED*/
	}
	fprintf(stderr, "%s: Restoring %s from %s\n", progname, pcOrig, pchBackup);
	(void)unlink(pcOrig);
	if (0 != link(pchBackup, pcOrig)) {
		fprintf(stderr, "%s: link: %s to %s: %s\n", progname, pchBackup, pcOrig, strerror(errno));
		exit(4);
	}
	(void)unlink(pchBackup);
	pcOrig = (char *)0;
	exit(1);
}

/* oops we ran out of memeory, how could that happen?			(ksb)
 */
void
OutOfMemory()
{
	fprintf(stderr, "%s: out of memory\n", progname);
	RestoreFiles();
}

/* SetCPPFlags
 * This function sets the CPP flags. It is given two arguments, the
 * option (CDIU), and the argument given. If the option is C, it
 * clears the previous CPP flags. Otherwise, it just catentiates a
 * string of options.
 */
static void
SetCPPFlags(option, argument)
int option;
char *argument;
{
	register char *pch = NULL;
	register unsigned len;
	static char sbOption[4] = " -?";

	if ('C' == option) {
		if (NULL != FileInfo.cppflags) {
			free(FileInfo.cppflags);
			FileInfo.cppflags = NULL;
		}
		return;
	}
	pch = FileInfo.cppflags;
	len = strlen(argument)+4;
	if (NULL == pch) {
		FileInfo.cppflags = malloc(len);
		FileInfo.cppflags[0] = '\000';
	} else {
		len += strlen(pch);
		FileInfo.cppflags = realloc(FileInfo.cppflags, len);
	}
	sbOption[2] = option;
	strcat(FileInfo.cppflags, sbOption);
	strcat(FileInfo.cppflags, argument);

	/* insert -I files includes into abbreviation list
	 */
	if ('I' == option) {
		/* if we end in '/' take it off */
		pch = strrchr(argument, '/');
		if ((char *)0 != pch && '\000' == pch[1])
			pch[1] = '\000';
		if ((char *)0 == srtin(&abrv, hincl(argument), lngsrt)) {
			OutOfMemory();
		}
		if (0 != verbose)
			fprintf(stderr, "%s: inserted abbr %s\n", progname, argument);
	}
}

/* keep the flags for this file						(ksb)
 */
static int
FileArgs(filename)
char *filename;
{
	register DependInfo *pDI;

	pDI = (DependInfo *)malloc((unsigned)sizeof(DependInfo));
	*ppDICursor = pDI;
	ppDICursor = & pDI->next;
	*ppDICursor = (DependInfo *)0;

	pDI->localo = FileInfo.localo;
	pDI->binarydep = FileInfo.binarydep;
	pDI->filename = STRSAVE(filename);
	pDI->inlib = inlib;

	if (FileInfo.preprocessor)
		pDI->preprocessor = FileInfo.preprocessor;
	else
		pDI->preprocessor = PATH_CPP;

	pDI->explicit = explicit;	/* from main.c parser	*/

	pDI->cppflags = pDI->destdir = pDI->suffix = pDI->basename = (char *)0;
	if (NULL != FileInfo.cppflags)
		pDI->cppflags = STRSAVE(FileInfo.cppflags);
	if (NULL != FileInfo.destdir)
		pDI->destdir = STRSAVE(FileInfo.destdir);
	if (NULL != FileInfo.suffix)
		pDI->suffix = STRSAVE(FileInfo.suffix);
	if (NULL != FileInfo.basename)
		pDI->basename = STRSAVE(FileInfo.basename);
	FileInfo.basename = NULL;
}
%%

char* named 'pcOrig' {
	param "none"
	hidden help "the original file name to put back"
}

boolean 'P' {
	hidden named "fMkdep"
	exclude "bLjm"
	char* 'f' {
		local named "pcDepend"
		init '".depend"'
		param "depfile"
		help "force mkdep compatible output in this file"
	}
	help "force maketd to emulate mkdep"
}

# ouch! mix is really evil.
# this feature is hardly ever used (in maketd or mkcmd) and we should Nuke it.
mix

# command line switches
action '4' {
	from '"maketd.h"'
	named ""
	update "FileInfo.preprocessor = PATH_M4;"
	help "use m4 to generate dependencies"
}
action 'C' {
	from '"maketd.h"'
	named "SetCPPFlags"
	update "%n(%w, (char *)0);"
	help "cancel all cpp flags"
}
function 'D' {
	from '"maketd.h"'
	update "%rCn(%w, %N);"
	param "define"
	help "specify defines, as in cpp"
}
function 'I' {
	from '"maketd.h"'
	update "%rCn(%w, %N);"
	param "includedir"
	help "specify include directory, as in cpp"
}
function 'U' {
	from '"maketd.h"'
	update "%rCn(%w, %N);"
	param "undefine"
	help "specify undefines, as in cpp"
}
char* 'L' {
	named "inlib"
	param "lib.a"
	update "%n = '\\000' == %a[0] ? (char *)0 : %a;"
	help "specify a library to contain targets"
}
boolean 'a' {
	named "alldep"
	help "do all dependencies, including /usr/include"
}
action 'bp' {
	from '"maketd.h"'
	update "FileInfo.binarydep = 1;"
	help "generate binary, rather than object related dependencies"
}
augment action 'p' {
	help "force maketd's binary mode for compat with mkdep"
}
function 's' {
	from '"maketd.h"'
	update "FileInfo.suffix = STRSAVE(%N);FileInfo.binarydep = 0;"
	param "suffix"
	help "change suffix target's suffix: a.suffix: foo.h"
}

action 'c' {
	from '"maketd.h"'
	update "FileInfo.preprocessor = PATH_CPP;"
	help "use cpp to generate dependencies"
}
boolean 'd' {
	named 'use_stdout'
	help "dependencies to stdout, rather than Makefile"
}
boolean 'H' {
	named 'force_head'
	help "force header/trailer (use with -d)"
}
char* 'j' {
	named "exten"
	param "extender"
	help "supply a new extender for old makefile"
}
action 'l' {
	from '"maketd.h"'
	update "FileInfo.localo = 0;"
	help "targets live in current Makefiles directory, turn off -n"
}
char* 'm' {
	static named "pcMakeFile" "pcRawFile"
	param "makefile"
	help "specify makefile for update"
}
action 'n' {
	from '"maketd.h"'
	update "FileInfo.localo = 1;"
	help "targets live in their source directory"
}
function 'o' {
	from '"maketd.h"'
	update "FileInfo.destdir = '.' == %N[0] && '\\000' == %N[1] ? (char *)0 : STRSAVE(%N);"
	param "dir"
	help "prepend DIR to target: DIR/a.o: foo.h"
}
boolean 'r' {
	named "replace"
	help "replace dependencies for targets"
}
function 't' {
	from '"maketd.h"'
	update "FileInfo.basename = %N;"
	param "target"
	help "change target's basename: target.o: foo.h"
}
boolean 'v' {
	named "verbose"
	help "print extra verbose (debug) output to stderr"
}
boolean 'x' {
	init "1"
	named "shortincl"
	help "don't abbreviate includes"
}

# then for each file
every {
	named "FileArgs"
	param "files"
	help "C source files to process for depends"
}
exit {
	named "MakeTD"
	update "%n(%rmn, %rfn);"
}

# secret options for ksb only...
action 'E' {
	hidden
	update "%ren = (char *)0;"
	help "set no explicit rule"
}
function 'F' {
	hidden
	from '"maketd.h"'
	update "FileInfo.preprocessor = ((char *)0 == %N || '\\000' != %N[0]) ? %N : (char *)0;"
	param "cmd"
	help "provide a shell command to generate depends"
}
char* 'e' {
	hidden
	named "explicit"
	update "%n = '\\000' == %a[0] ? (char *)0 : %a;"
	param "rule"
	help "force an explicit rule"
}

%c
/* any signal calls us to clean up (with its name as our argument)
 */
int
sigtrap(pch)
char *pch;
{
	fprintf(stderr, "%s: SIG%s received\n", progname, pch);
	RestoreFiles();
	/*NOTREACHED*/
}

SIGRET_T
errhup()
{
	sigtrap("HUP");
}

SIGRET_T
errint()
{
	sigtrap("INT");
}

SIGRET_T
errill()
{
	sigtrap("ILL");
}

SIGRET_T
errtrap()
{
	sigtrap("TRAP");
}

SIGRET_T
erriot()
{
	sigtrap("IOT");
}

SIGRET_T
erremt()
{
	sigtrap("EMT");
}

SIGRET_T
errfpe()
{
	sigtrap("FPE");
}

SIGRET_T
errbus()
{
	sigtrap("BUS");
}

SIGRET_T
errsegv()
{
	sigtrap("SEGV");
}

SIGRET_T
errsys()
{
	sigtrap("SYS");
}

SIGRET_T
errpipe()
{
	sigtrap("PIPE");
}

SIGRET_T
erralrm()
{
	sigtrap("ALRM");
}

SIGRET_T
errterm()
{
	sigtrap("TERM");
}

/* Stock 2.9BSD has all the below, but not as many as 4.2BSD: Want
 * SIGQUIT to drop core. Not worrying about:	SIGXCPU, SIGXFSZ,
 * SIGVTALRM, SIGPROF cannot be caught: SIGKILL, SIGSTOP Leaving
 * alone: SIGURG, SIGTSTP, SIGCONT, SIGCHLD, SIGTTIN, SIGTTOU, SIGIO 
 */
static void
init_sigs()
{
	signal(SIGHUP, errhup);
	signal(SIGINT, errint);
	signal(SIGILL, errill);
	signal(SIGTRAP, errtrap);
	signal(SIGIOT, erriot);
#if defined(SIGEMT)
	signal(SIGEMT, erremt);
#endif
	signal(SIGFPE, errfpe);
#if !defined(DEBUG)
	signal(SIGBUS, errbus);
	signal(SIGSEGV, errsegv);
#endif /* !DEBUG */
#if defined(SIGSYS)
	signal(SIGSYS, errsys);
#endif
	signal(SIGPIPE, errpipe);
	signal(SIGALRM, erralrm);
	signal(SIGTERM, errterm);
}
%%
