# $Id: unrm.m,v 3.7 2000/07/31 23:13:00 ksb Exp $
# mkcmd source for "unrm"
# $Compile: ${mkcmd-mkcmd} std_help.m std_version.m %f
# Matthew Bradburn, PUCC UNIX Group
from '<sys/types.h>'
from '<sys/param.h>'
from '<sys/stat.h>'

from '"libtomb.h"'
from '"main.h"'
from '"lists.h"'
from '"init.h"'
from '"findfiles.h"'
from '"listtombs.h"'

%i
static char rcsid[] =
	"$Id: unrm.m,v 3.7 2000/07/31 23:13:00 ksb Exp $";
%%

%c

/* show the customer which tombs have crypts for his files		(ksb)
 */
int
cm_Crypts(argc, argv)
int argc;
char **argv;
{
	ShowCrypts(stdout);
	return 0;
}

/* list files belonging to the user in each crypt			(ksb)
 * "list" is equivalent to "list *".
 */
int
cm_List(argc, argv)
int argc;
char **argv;
{
	if (Verbose) {
		(void)fprintf(stdout, "Files entombed for you:  (most recently removed first)\n");
	}
	ListHeader();
	if (argc < 2) {
		FindFiles("*", LsEntombed);
	} else {
		register int i;
		for (i = 1; i < argc; ++i) {
			FindFiles(argv[i], LsEntombed);
		}
	}
	(void)fflush(stdout);
	ClearMarks(pLEFilesList);
	return 0;
}

%%
from '"purge.h"'
%c
/* delete pesky unwanted files from my crypt				(ksb)
 */
int
cm_Purge(argc, argv)
int argc;
char **argv;
{
	register int i;

	if (argc < 2) {
		fprintf(stderr, "%s: purge: usage files\n", progname);
		return 1;
	}

	for (i = 1; i < argc; ++i) {
		FindFiles(argv[i], Purge);
	}
	StrikeMarks(& pLEFilesList);
	return 0;
}

/* print this program's current working directory			(ksb)
 */
int
cm_Pwd()
{
	auto char acBuf[1024];

	user();
	if (0 != getwd(acBuf)) {
		(void)puts(acBuf);
	} else {
		(void)fprintf(stderr, "%s: pwd: %s\n", progname, acBuf);
		exit(2);
	}
	charon();
	return 0;
}

%%
from '"restore.h"'
%c
/* restore a lost from to the user -- they will love us			(ksb)
 * of course this is the whole point of an entombing system, right?
 */
int
cm_Restore(argc, argv)
int argc;
char **argv;
{
	register int i;

	if (argc < 2) {
		fprintf(stderr, "%s: restore: usage files\n", progname);
		return 1;
	}
	for (i = 1; i < argc; ++i) {
		FindFiles(argv[i], Restore);
	}
	StrikeMarks(& pLEFilesList);
	return 0;
}

%%
from '"show.h"'
%c
/* show the user a file from their crypt				(ksb)
 */
int
cm_Show(argc, argv)
int argc;
char **argv;
{
	register int i;

	if (argc < 2) {
		fprintf(stderr, "%s: show: usage files\n", progname);
		return 1;
	}
	for (i = 1; i < argc; ++i) {
		FindFiles(argv[i], Show);
	}
	ClearMarks(pLEFilesList);
	return 0;
}

/* output all the tombs we can see					(ksb)
 */
int
cm_Tombs(argc, argv)
int argc;
char **argv;
{
	ShowTombs(stdout);
	return 0;
}

/* the old unrm had a verbose toggle -- I don't know why		(ksb)
 */
int
cm_Verbose(argc, argv)
int argc;
char **argv;
{
	if (argc > 2) {
		fprintf(stderr, "%s: versbose: usage [off|on]\n", progname);
		return 1;
	}
	if (argc < 2) {
		Verbose ^= 1;
	} else if (0 == strcmp("off", argv[1])) {
		Verbose = 0;
	} else if (0 == strcmp("on", argv[1])) {
		Verbose = 1;
	} else {
		fprintf(stderr, "%s: verbose: unknown state `%s'\n", progname, argv[1]);
		return 1;
	}
	/* change function pointer for '?' */
	return 0;
}

%%
from '"changedir.h"'
from '"ls.h"'
%c
/* N.B.: do NOT put CMD_DEF_SHELL here!!!!!!
 * the customer could get a shell as charon and go grave robbing
 */
CMD CMUnrm[] = {
	{"bye", "exit this program", (int (*)())0, CMD_RET},
	{"cd", "change current working directory", cm_ChDir, CMD_NULL, "[dir]"},
	{"crypts", "display the path to the private crypts for this login", cm_Crypts, CMD_NULL},
	CMD_DEF_COMMANDS,
	{"list", "list the files currently entombed", cm_List}, 
	{"ls", "list the files in the working directory", cm_Ls},
	CMD_DEF_HELP,
	{"purge", "remove files from the tomb", cm_Purge, CMD_NULL, "files"},
	{"pwd", "print the path to the working directory", cm_Pwd},
	{"quit", "exit this program", (int (*)())0, CMD_RET},
	{"restore", "restore a file to the working directory", cm_Restore, CMD_NULL, "files"},
	{"show", "invokes $PAGER on a file in the tomb", cm_Show, CMD_NULL, "files"},
	{"tombs", "show which tombs are examined", cm_Tombs},
	{"verbose", "change verbose mode", cm_Verbose, CMD_NULL, "[off | on]"},
	CMD_DEF_VERSION,
	{"?", (char *)0, cmd_commands, CMD_NULL}, 
};

CMDSET CSUnrm;
%%
getenv "UNRM"

before {
	update 'cmd_init(& CSUnrm, CMUnrm, sizeof(CMUnrm)/sizeof(CMD), (char *)0, 3);'
}

boolean 'i' {
	named "fInteract"
	track "fForceInteract"
	init "1"
	help "force interactive mode"
}

# for the arguments to the command, just restore them
every {
	named "FindFiles"
	update '%n(%N, Restore);'
	param "files"
	help "restore these file from the tombs"
}
zero {
	update "if (%rin || %riU) {cmd_from_file(& CSUnrm, stdin);}"
}

action "a" {
	named "cmd_from_string"
	help "restore all files in tomb"
	update '%rin = 0;%n(& CSUnrm, "restore *", (int *)0);'
}

action "l" {
	named "cmd_from_string"
	update '%rin = 0;%n(&CSUnrm, "list *", (int *)0);'
	help "list files in the tomb"
}

boolean "f" {
	named "Force"
	initializer "0"
	help "overwrite existing files without confirmation"
}

toggle "v" {
	named "Verbose"
	initializer "1"
	help "be verbose"
}

function 'c' {
	named "cmd_from_string"
	help "execute command as if interactive"
	param "cmd"
	update "%rin = 0;%n(& CSUnrm, %N, (int *)0);"
}
