# rmfile - help users delete/rename files with funny names.
# $Mkcmd(*): ${mkcmd-mkcmd} %f
#

from '<sys/param.h>'
from '<sys/stat.h>'
from '<sys/types.h>'
from '<sys/ioctl.h>'
from '"machine.h"'

%i
static char rcsid[] =
	"$Id: rmfile.m,v 2.5 1998/05/24 16:02:30 ksb Exp $";

#if HAS_NDIR
#include <ndir.h>
#else
#if HAS_DIRENT
#include <dirent.h>
#else
#include <sys/dir.h>
#endif
#endif
%%

require "std_help.m" "std_version.m"
require "cmd.m" "cmd_cd.m" "cmd_exec.m" "cmd_version.m" "cmd_help.m"

basename "rmfile" ""

char* variable "pcStart" {
	init '"."'
	help "initial directory, if not current"
	param "directory"
}
left [ "pcStart" ] {
}

%i
#if !defined(S_IFLNK)
#define lstat	stat
#endif

static struct DIRENT **dirlst;		/* our list of files		*/
static int ndirents;			/* orig number of files		*/
static int iPageRows = 23;		/* default # of lines for screen */
static int iPageCols = 79;		/* default width of terminal */
%%

%c
/* count # print columns for a string assume output using unctlstr()	(ksb)
 */
static int
uncstrlen(str)
register char *str;
{
	register int i = 0;
	register c;

	while ((char *)0 != str && (c = *str++) != '\0') {
		++i;
		if ((c & 0177) < 32 || (c & 0177) == 0177)
			++i;
	}
	return i;
}

/* output a string in the format cat -v does				(ksb)
 */
static void
unctlstr(fp, str)
register FILE *fp;
register char *str;
{
	register char c;

	while (str != (char *)0 && '\000' != (c = *str++)) {
		if (0 != (0200 & c)) {
			fputc('M', fp);
			fputc('-', fp);
			c &= 0177;
		}
		if (c < ' ') {			/* < space is control char */
			fputc('^', fp);
			fputc(c + '@', fp);
			continue;
		}
		if (c == 0177) {		/* rubout = ^? */
			fputc('^', fp);
			fputc('?', fp);
			continue;
		}
		fputc(c, fp);
	}
}

/* skipdot - skip . & ..						(ksb)
 */
static int
skipdot(dir)
struct DIRENT *dir;
{
	register char *p;

	if ((struct DIRENT *)0 == dir) {
		return 0;
	}
	p = dir->d_name;
	return !(p[0] == '.' && (p[1] == '\000' || (p[1] == '.' && p[2] == '\000')));
}

/* initial read of directory						(?)
 *	spiff up name (./xx -> xx) and "" -> ".")
 *	scan directory for names
 *	setup prefix for convert back to path
 */
static void
init_dir()
{
	register int i;
	static int inited = 0;
	extern int alphasort();

	if (inited) {
		for (i = 0; i < ndirents; i++) {
			if ((struct DIRENT *)0 != dirlst[i])
				free(dirlst[i]);
		}
		free(dirlst);
	}
	ndirents = scandir(".", &dirlst, skipdot, alphasort);
	if (ndirents < 0) {
		fprintf(stderr, "%s: scandir: .: %s\n", progname, strerror(errno));
		exit(1);
	}
	inited = 1;
}

/* convert the input "file number" to a subscript into the array	(ksb)
 * of files in question
 */
static int
cvtfnum(pcIn)
char *pcIn;
{
	register int dn;

	dn = atoi(pcIn);
	--dn;
	if (dn < 0 || dn >= ndirents || (struct DIRENT *)0 == dirlst[dn]) {
		fprintf(stderr, "%s: %s: bad file number\n", progname, pcIn);
		return -1;
	}
	return dn;
}

/* convert the file number back into a path name			(ksb)
 */
char *
cvtname(pcFile, iFile)
char *pcFile;
int iFile;
{
	register char *pc;
	pc = dirlst[iFile]->d_name;
	if ((char *)0 == pcFile) {
		return pc;
	}
	return strcpy(pcFile, pc);
}

/* print directory list: columnate & pagenate				(ksb)
 *  we need to pack them across like
 * NNNtFileNameBNNNtFileNameB...
 * where NNN is a number, t is the type character, FileName is a
 * filename in cat -v format and "B" is at least one space
 */
static int
list()
{
	auto struct stat stThis;	/* what kind of file */
	register int i;			/* tmp, col count */
	register int k;			/* tmp, character count of an entry */
	register int iTab, iChar;	/* move by tabs counters */
	register int iAcross;		/* cols in a line */
	register int iMaxLabel;		/* longest print file name length */
	register int iNumWidth;		/* number width + 1 for type */
	register int iInCol;		/* column we are in */
	register int iLine;			/* current line on page */

	/* compute the width of the number + type
	 */
	auto char acWidth[32];
	sprintf(acWidth, "%d", ndirents);
	iNumWidth = strlen(acWidth) + 1;

	/* compute the longest print length, we start at iNumWidth because
	 * less than that looks silly, trust me.
	 */
	iMaxLabel = iNumWidth;
	for (k = 0; k < ndirents; k++) {
		if ((struct DIRENT *)0 == dirlst[k])
			continue;
		if (iMaxLabel < (i = uncstrlen(dirlst[k]->d_name)))
			iMaxLabel = i;
	}

	iMaxLabel += iNumWidth + 1;	/* include total for a unit */
	iAcross = 0;
	i = iPageCols;
	do {
		++iAcross;
		i -= iMaxLabel;
	} while (i >= iMaxLabel);

	iLine = 0;				/* start at line 0 */
	iChar = iInCol = 0;
	for (i = 0; i < ndirents; i++) {
		if ((struct DIRENT *)0 == dirlst[i])
			continue;
		fprintf(stdout, "%*d", iNumWidth, i + 1);
		if (lstat(cvtname((char *)0, i), &stThis) < 0)
			fputc(' ', stdout);
#if defined(S_IFLNK)
		else if ((stThis.st_mode & S_IFMT) == S_IFLNK)
			fputc('@', stdout);
#endif
		else if ((stThis.st_mode & S_IFMT) == S_IFDIR)
			fputc('/', stdout);
		else if ((stThis.st_mode & S_IFMT) == S_IEXEC)
			fputc('*', stdout);
		else
			fputc(' ', stdout);
		unctlstr(stdout, dirlst[i]->d_name);
		if (++iInCol >= iAcross) {
			auto char acBuf[8];
			fputc('\n', stdout);
			iChar = iInCol = 0;
			if (++iLine < iPageRows) {
				continue;
			}
			fputs("newline to continue, EOF to quit:", stdout);
			if ((char *)0 == fgets(acBuf, sizeof(acBuf), stdin))
				exit(0);
			iLine = 0;
			continue;
		}
		k = uncstrlen(dirlst[i]->d_name) + 1 + iNumWidth;
		iChar += k;

		/* move by tabs, then spaces to get there
		 */
		while ((iTab = 8-(iChar & 7)), k+iTab < iMaxLabel) {
			fputc('\t', stdout);
			k += iTab;
			iChar += iTab;
		}
		for (/* above */; k++ < iMaxLabel; ++iChar) {
			fputc(' ', stdout);
		}
	}
	if (iInCol != 0) {
		fputc('\n', stdout);
	}
	return 0;
}

/* delete a file							(ksb)
 */
static int
del(dn)
int dn;					/* directory number */
{
	auto char acThis[MAXPATHLEN+1];

	cvtname(acThis, dn);
	if (unlink(acThis) < 0) {
		fprintf(stderr, "%s: unlink: ", progname);
		unctlstr(stderr, acThis);
		fprintf(stderr, ": %s\n", strerror(errno));
		return 1;
	}
	unctlstr(stdout, acThis);
	printf(" deleted.\n");
	dirlst[dn] = (struct DIRENT *)0;
	return 0;
}

/* fixname - rename a file, the obvious name conflicts.			(uitti)
 * we try to intuit
 *	rename foo /tmp/bar
 * from
 *	rename foo bar
 */
static int
fixname(dn, pcTo)
int dn;
char *pcTo;
{
	auto char acFrom[MAXPATHLEN+1];

	cvtname(acFrom, dn);
	if (rename(acFrom, pcTo) < 0) {
		fprintf(stderr, "%s: rename: '", progname);
		unctlstr(stderr, acFrom);
		fprintf(stderr, "' to '");
		unctlstr(stderr, pcTo);
		fprintf(stderr, "': %s\n", strerror(errno));
		return 1;
	}
	unctlstr(stdout, acFrom);
	printf(" renamed to ");
	unctlstr(stdout, pcTo);
	printf("\n");

	/* if we just moved it out of our name-space hide it
	 */
	(void)free(dirlst[dn]);
	if ((char *)0 != strchr(pcTo, '/')) {
		dirlst[dn] = (struct DIRENT *)0;
		return 0;
	}

	/* ...else build a place holder for it
	 * (and we don't care about inode numbers or reclens)
	 */
	dirlst[dn] = (struct DIRENT *) malloc(sizeof (struct DIRENT) + 1);
	if ((struct DIRENT *)0 == dirlst[dn]) {
		fprintf(stderr, "%s: malloc: %s\n", progname, strerror(errno));
		return 1;
	}
	(void)strncpy(dirlst[dn]->d_name, pcTo, MAXNAMLEN);
#if HAVE_DIRECT
	dirlst[dn]->d_namlen = strlen(pcTo);
#endif
	return 0;
}

#ifdef BSD2_9
/* simulate 4.2's rename syscall.					(uitti)
 */
int
rename(pcFrom, pcTo)
char *pcFrom, *pcTo;
{
	if (link(pcFrom, pcTo) < 0)
		return -1;
	return unlink(pcFrom);
}
#endif /* BSD2_9 missing rename(2) */

/* now the interactive commands mkcmd's cmd level calls...
 */

/* set # of columns							(ksb)
 */
static int
f_SCOL(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	if (argc < 2 || 0 >= (iPageCols = atoi(argv[1])))
		iPageCols = 1;
	return 0;
}

/* set page length							(ksb)
 */
static int
f_SPAGE(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	if (argc < 2 || 0 >= (iPageRows = atoi(argv[1])))
		iPageRows = 1;
	return 0;
}

/* delete files								(ksb)
 */
static int
f_DELETE(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	register int i, dn;

	if (argc < 2) {
		printf("%s: %s: usage file-number\n", progname, argv[0]);
		return 1;
	}
	for (i = 1; i < argc; ++i) {
		dn = cvtfnum(argv[i]);
		if (-1 != dn) {
			del(dn);
		}
	}
	return 0;
}

/* we should get these from "../../lib/libtomb/libtomb.h"
 */
#if !defined(NO_TOMB)
#define NO_TOMB		4
#endif
#if !defined(NOT_ENTOMBED)
#define NOT_ENTOMBED	2
#endif

/* entomb the file in the crypt on this filesystem			(ksb)
 * argv[0] is the magic exec cookie to make cmd_fs_exec run the
 * entomb libexec binary.  We filter the arguments and call down.
 *
 * We take the "copy it out and remove it _only_ if the tomb got it"
 * tactic here.  In other words if we can't entomb it don't unlink it.
 */
static int
f_ENTOMB(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	register int iCode, i, dn;
	auto char acThis[MAXPATHLEN+1];
	auto char *apcDown[5];

	apcDown[0] = argv[0];	/* exec cookie */

	/* allow entomb -V or -h or -E
	 */
	if (2 == argc && '-' == argv[1][0]) {
		apcDown[1] = argv[1];
		apcDown[2] = (char *)0;
		return cmd_fs_exec(2, apcDown, pCS);
	}

	apcDown[1] = "-c";	/* entomb -c ... */
	/* fill-in in loop */
	apcDown[3] = (char *)0;

	for (i = 1; i < argc; ++i) {
		if (-1 == (dn = cvtfnum(argv[i]))) {
			continue;
		}
		apcDown[2] = cvtname(acThis, dn);
		iCode = cmd_fs_exec(3, apcDown, pCS);
		if (0 == iCode) {
			del(dn);
			continue;
		}
		if (0 != (NO_TOMB & iCode)) {
			fprintf(stderr, "%s: no tomb for this directory (file not removed)\n", progname);
			return 1;
		}
		if (0 != (NOT_ENTOMBED & iCode)) {
			fprintf(stderr, "%s: %s: entomb failed\n", progname, acThis);
			continue;
		}
	}
	return 0;
}

/* list the working directory						(ksb)
 */
static int
f_LIST(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	return list();
}

/* rename file# newname							(ksb)
 */
static int
f_RENAME(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	register int dn;

	if (3 != argc) {
		fprintf(stderr, "%s: %s: usage file# new-name\n", progname, argv[0]);
	}
	if (-1 == (dn = cvtfnum(argv[1])))
		return 1;
	return fixname(dn, argv[2]);
}

/* change dir and reread it.						(ksb)
 * not that we must have NO_FREE set in the command table because we
 * keep a pointer to one of our arguments (sloppy -- no we can do that).
 */
static int
f_CD(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	register int iRet;

	if (0 != (iRet = cmd_cd(argc, argv, pCS))) {
		return iRet;
	}
	init_dir(".");
	return list();
}

/* if the Customer types just a number change to that directory		(ksb)
 */
%%
init 8 "cmd_unknown = f_unknown;"
%c
static int
f_unknown(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	register int dn;
	register char *pcDown;
	static int _cmd_unknown();

	if (1 != argc || -1 == (dn = cvtfnum(argv[0]))) {
		return _cmd_unknown(argc, argv, pCS);
	}
	pcDown = cvtname((char *)0, dn);
	if (-1 == chdir(pcDown)) {
		fprintf(stderr, "%s: chdir: %s: %s\n", progname, pcDown, strerror(errno));
		return errno;
	}
	init_dir(".");
	return list();
}


static CMD aCMDs[] = {
	{ "cd", "change directory", f_CD, CMD_NULL, "[directory]"},
	CMD_DEF_COMMANDS,
	{"columns", "set width of terminal", f_SCOL, CMD_NULL, "[cols]"},
	{"delete", "delete a file", f_DELETE, CMD_NULL, "numbers"},
	{"exit", "quit this program", (int (*)())0, CMD_RET },
	CMD_DEF_HELP,
	{"entomb\000/usr/local/lib/entomb", "transfer a file to the entomb system's tomb then delete it", f_ENTOMB, CMD_NULL, "numbers | -E | -V"},
	{"list", "list the current directory", f_LIST, CMD_NULL },
	{"pagelength", "set of lines per page", f_SPAGE, CMD_NULL, "[length]"},
	CMD_DEF_PWD,
	CMD_DEF_QUIT,
	{"rename", "rename a file", f_RENAME, CMD_NULL, "number new-name"},
	{"rm", (char *)0, f_DELETE, CMD_NULL, "numbers"},
	CMD_DEF_VERSION,
	{"?", (char *)0, cmd_commands, CMD_NULL}
};

static CMDSET CSRm;

/* init the interactive stuff and read commands until EOF		(ksb)
 */
static void
Driver(argc, argv)
int argc;
char **argv;
{
#if defined(TIOCSWINSZ)
	auto struct winsize WITty;
#endif

	if (0 != argc) {
		fprintf(stderr, "%s: no other positional parameters allowed\n", progname);
		exit(1);
	}
	if (-1 == chdir(pcStart)) {
		fprintf(stderr, "%s: chdir: %s: %s\n", progname, pcStart, strerror(errno));
		exit(1);
	}
	init_dir(".");
#if defined(TIOCSWINSZ)
	if (-1 != ioctl(1, TIOCGWINSZ, &WITty)) {
		iPageCols = WITty.ws_col < 1 ? 23 : WITty.ws_col-1;
		iPageRows = WITty.ws_row < 1 ? 79 : WITty.ws_row-1;
	}
#endif
	list();
	cmd_from_file(& CSRm, stdin);
}
%%

init "cmd_init(& CSRm, aCMDs, sizeof(aCMDs)/sizeof(CMD), (char *)0, 2);"

list {
	param ""
	named "Driver"
}
