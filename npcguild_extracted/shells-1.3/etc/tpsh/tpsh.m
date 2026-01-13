#!mkcmd
# tape shell for loading system upgrades				(ksb)
# 02 Fed 1995

# mk(1L) targets to build this project:
#  $Compile(*): %b %o -mMkcmd %f && %b %o prog.c
#  $Mkcmd(*): ${mkcmd-mkcmd} std_help.m std_control.m std_version.m cmd.m cmd_echo.m cmd_exec.m cmd_parse.m cmd_merge.m cmd_help.m cmd_source.m cmd_version.m %f

basename "tpsh" ""
comment "%c%kCompile(*): %%b %%o -mCc setenv.c && $%{cc-cc%} %%f setenv.o -o %%F"
from "<sys/wait.h>"
from "<sys/stat.h>"
from "<unistd.h>"
from "<ctype.h>"
require "std_help.m" "std_control.m" "std_version.m" 
	"cmd.m" "cmd_echo.m" "cmd_exec.m" "cmd_parse.m" 
	"cmd_merge.m" "cmd_help.m" "cmd_source.m" "cmd_version.m"
	"tpsh.mi"

augment action 'V' {
	user 'printf("%%s: default tape device is %%s\\n", %b, acDefTape);'
}

%i
static char rcsid[] =
	"$Id: tpsh.m,v 2.8 1997/08/30 20:48:20 ksb Exp $";

#if !defined(DEF_DRIVE)
#define DEF_DRIVE	"/dev/rmt/0n"
#endif
%%

key "tpsh_utils" init {
	"dump"
	"mt"
	"rm"
	"shutdown"
	"tar"
}

key "tpsh_lookup" init {
	"%a"
}

%c
static char acDefTape[1024] = DEF_DRIVE;

extern char **environ, *getenv(), *mktemp();

/* act like a little shell						(ksb)
 * we have to get to slash to run shutdown or to remove the directory
 * we built all the stuff in.
 */
static int
ForkExec(ppcArgv, pcInto)
char **ppcArgv, *pcInto;
{
	register int iPid, i;
	register char *pcTail, *pcWhole;
	auto int wRet;

	(void)fflush(stdout);
	(void)fflush(stderr);
	switch (iPid = fork()) {
	case -1:
		return 128;
	case 0:
		break;
	default:
		/* spin for exit code */
		while (-1 != (i = wait(& wRet)) && iPid != i)
			/* spin */;
		if (WIFEXITED(wRet)) {
			return WEXITSTATUS(wRet);
		}
		return WTERMSIG(wRet);
	}

	/* in the child process we move the argv around a little
	 */
	pcWhole = ppcArgv[0];
	if ((char *)0 != (pcTail = strrchr(ppcArgv[0], '/'))) {
		ppcArgv[0] = pcTail+1;
	}
	if ((char *)0 != pcInto) {
		if (fVerbose) {
			printf("+ chdir %s\n", pcInto);
			fflush(stdout);
		}
		if (-1 == chdir(pcInto)) {
			fprintf(stderr, "%s: chdir: %s: %s\n", progname, pcInto, strerror(errno));
		}
	}
	if (fVerbose) {
		for (i = 0; (char *)0 != ppcArgv[i]; ++i) {
			printf("%s %s", (0 == i) ? "+" : "", ppcArgv[i]);
		}
		printf("\n");
		fflush(stdout);
	}
	if (fExec) {
		(void)execve(pcWhole, ppcArgv, environ);
		fprintf(stderr, "%s: execve: %s: %s\n", progname, pcWhole, strerror(errno));
		exit(errno);
	}
	exit(0);
}

/* the low level unpacker (generally hidden by cmd_load)		(ksb)
 *
 *	+ the file "Commands" may be a list of commands to add to the
 *	  current tpsh shell
 *	+ the file "unbundle" should be an extraction script that
 *	  gives the user a chance to abort
 *	+ that script can take parameters passed through the load command
 */
static int      /*ARGSUSED*/
cmd_unpack(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	register FILE *fpCmds;
	static char acAddCmds[] = "Commands";
	static char acGoForIt[] = "./unbundle";
	auto CMD *pCMMerge;
	auto unsigned int uiMerge;

	if ((FILE *)0 != (fpCmds = fopen(acAddCmds, "r"))) {
		if (0 == cmd_parse(fpCmds, & pCMMerge, & uiMerge) && 0 != uiMerge) {
			cmd_merge(pCS, pCMMerge, uiMerge, (int (*)())0);
		}
		(void)fclose(fpCmds);
	}

	argv[0] = acGoForIt;
	return ForkExec(argv, (char *)0);
}

/* to load a tape we tar extract the header on the tape			(ksb)
 * execute the unpack script.
 */
static int      /*ARGSUSED*/
cmd_load(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	register char *pcTape;
	register int iRet;
	static char *apcTar[] = {
		"/bin/tar", "xf", (char *)0, (char *)0
	};

	if ((char *)0 == (pcTape = getenv("TAPE_DEVICE"))) {
		pcTape = acDefTape;
	}
	apcTar[2] = pcTape;
	if (0 != (iRet = ForkExec(apcTar, (char *)0))) {
		fprintf(stderr, "%s: %s: tar failed to extract (code %d)\n", progname, pcTape, iRet);
		/* ZZZ: squawk to OpC or something here in real-time
		 */
	}
	return cmd_unpack(argc, argv, pCS);
}

/* eject the tape from the device					(ksb)
 */
static int      /*ARGSUSED*/
cmd_eject(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	register char *pcTape;
	static char *apcMt[] = {
		"/bin/mt", "-t", (char *)0, "offl", (char *)0
	};

	if ((char *)0 == (pcTape = getenv("TAPE_DEVICE"))) {
		pcTape = acDefTape;
	}
	apcMt[2] = pcTape;
	return ForkExec(apcMt, (char *)0);
}

/* backup the system to tape, we have a tape device and a dump level	(ksb)
 * [default 1] push the dumps out to the tape
 *
 * we are in a /tmp/tape_XXXXXX directotry, so we build a dump shell
 * script here (in place) and run it with the shell.  Nitfy and clean.
 */
static int      /*ARGSUSED*/
cmd_backup(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	register char *pcLevel, *pcTape;
	register FILE *fpFs;
	static char acFs[1024];
	static char acDumpKeys[] = "_ufds";
	static char *apcDump[] = {
#define FS_SLOT				   2
		"/etc/dump", acDumpKeys, acDefTape, "54000", "6000", acFs, (char *)0
	};
	static char *apcEject[] = {
		"eject", (char *)0, (char *)0
	};

	pcLevel = (argc == 2) ? argv[1] : "1";
	if (!isdigit(pcLevel[0]) || '\000' != pcLevel[1]) {
		fprintf(stderr, "%s: level number `%s' must be in the range 0 to 9\n", progname, pcLevel);
		return 1;
	}
	acDumpKeys[0] = pcLevel[0];

	if ((char *)0 == (pcTape = getenv("TAPE_DEVICE"))) {
		pcTape = acDefTape;
	}
	apcDump[FS_SLOT] = pcTape;

#if USE_CHECKLIST
	fpFs = popen("sed -e 's/#.*//'</etc/checklist |awk '$3 == \"hfs\" && $5 =! 0 { printf \"%s\\n\", $2;}'", "r");
#else
#if USE_FSTAB
	fpFs = popen("sed -e 's/#.*//'</etc/fstab |awk '$3 == \"ufs\" && $5 =! 0 { printf \"%s\\n\", $2;}'", "r");
#else
#if USE_FILESYSTEMS
	fpFs = popen("sed -e 's/*.*//' -e s'/[\t ]*$//'</etc/filesystems | awk '/:$/{ line = $0; }\n/jfs/{ print substr(line,1,length(line)-1); }'", "r");
#else
	fprintf(stderr, "%s: no way to read file system table\n", progname);
	return 1;
#endif
#endif
#endif
	while ((char *)0 != fgets(acFs, sizeof(acFs), fpFs)) {
		register char *pcEoln;

		if ((char *)0 == (pcEoln = strchr(acFs, '\n'))) {
			fprintf(stderr, "%s: file system mount point name too long? (%s)\n", progname, acFs);
			break;
		}
		*pcEoln = '\000';
		printf("%s: dumping %s to %s (level %s)\n", progname, acFs, pcTape, pcLevel);
		if (0 == ForkExec(apcDump, (char *)0)) {
			continue;
		}
		fprintf(stderr, "%s: dump of %s failed.  Get help.\n", progname, acFs);
		(void)pclose(fpFs);
		return 1;
	}
	(void)pclose(fpFs);
	return cmd_eject(1, apcEject, pCS);
}

/* some new daemons might need a reboot to finish installation		(ksb)
 */
static int      /*ARGSUSED*/
cmd_reboot(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	static char *apcReboot[] = {
		"/etc/shutdown", "-r", "-y", "10", (char *)0
	};

	if (argc > 1) {
		apcReboot[3] == argv[1];
	}
	return ForkExec(apcReboot, "/");
}

/* rewind the tape in the device					(ksb)
 */
static int      /*ARGSUSED*/
cmd_rewind(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	register char *pcTape;
	static char *apcMt[] = {
		"/bin/mt", "-t", (char *)0, "rew", (char *)0
	};

	if ((char *)0 == (pcTape = getenv("TAPE_DEVICE"))) {
		pcTape = acDefTape;
	}
	apcMt[2] = pcTape;
	return ForkExec(apcMt, (char *)0);
}

/* suggest a new tape device, or list of them.  Use the one		(ksb)
 * from the list with the newest access time [the one we used
 * last time, right?]
 */
static int      /*ARGSUSED*/
cmd_tape(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	register char **ppc, *pcFound;
	register time_t tLastAcc;
	auto struct stat stTape;

	if (1 >= argc) {
		fprintf(stderr, "%s: tape: must specify a list of tape devices\n", progname);
		return 1;
	}

	pcFound = (char *)0;
	tLastAcc = 0;
	for (ppc = argv; (char *)0 != *ppc; ++ppc) {
		if (0 != stat(*ppc, & stTape)) {
			continue;
		}
		if ((char *)0 == pcFound || tLastAcc < stTape.st_atime) {
			setenv("TAPE_DEVICE", *ppc, 1);
			tLastAcc = stTape.st_atime;
			pcFound = *ppc;
		}
	}
	if ((char *)0 != pcFound) {
		if (fVerbose) {
			printf("%s: tape: device is now %s\n", progname, pcFound);
		}
	} else {
		fprintf(stderr, "%s: tape: no device located\n", progname);
		/* setenv("TAPE_DEVICE", (char *)0, 1); XXX ? */
	}
	return 0;
}

/* if the driver has a problem we could biff some central admin		(ksb)
 * we should leverage this via a sendmail alias `help-desk' or
 * something like that.  That would point to BOTH a regional admin
 * and the centralized system admin (Network Control?)
 */
static int      /*ARGSUSED*/
cmd_trouble(argc, argv, pCS)
int argc;
char **argv;
CMDSET *pCS;
{
	/* ZZZ: figure out how/who to xmit/mail this report
	 */
	printf("trouble report via mail or ops center\n");
	return 0;
}

CMD aCM[] = {
	{"backup", "backup the system to tape, level is a number 0-9", cmd_backup, CMD_NULL, "[level]"},
	CMD_DEF_COMMANDS,
	CMD_DEF_ECHO,
	{"eject", "eject the tape from the drive", cmd_eject, CMD_NULL},
	{"exit", "same as quit", (int (*)())0, CMD_RET|CMD_HIDDEN},
	CMD_DEF_HELP,
	{"load", "load a tape", cmd_load, CMD_NULL},
	{"log", "same as quit", (int (*)())0, CMD_RET|CMD_HIDDEN},
	CMD_DEF_PWD,
	CMD_DEF_QUIT,
	{"reboot", "reboot this host", cmd_reboot, CMD_NULL, "[grace]"},
	{"rewind", "rewind the tape in the drive", cmd_rewind, CMD_NULL},
	CMD_DEF_SOURCE ,
	{"tape", "suggest a new tape device", cmd_tape, CMD_NULL, "device"},
	{"trouble", "file a trouble report", cmd_trouble, CMD_HIDDEN},
	{"unpack", "unpack the current directory", cmd_unpack, CMD_HIDDEN, "[unbundle-opts]"},
	CMD_DEF_VERSION,
	{"?", "quick help", cmd_commands, 0},
	CMD_DEF_COMMENT
};

CMDSET CSThis;

static char acTapeDir[1024] = "/tmp/tape_XXXXXX";

/* setup the command interp for mkcmd, and setup the environment	(ksb)
 * for our tape stuff
 */
static void
before_func()
{
	static char *apcTapes[] = {
		"tape",
		"/dev/rmt/3mn",
		"/dev/rmt/0mn",
		"/dev/rmt/3hcn",
		"/dev/update.src",
		(char *)0
	};

	cmd_init(& CSThis, aCM, sizeof(aCM)/sizeof(CMD), "OK ", 3);

	if ((char *)0 == mktemp(acTapeDir)) {
		fprintf(stderr, "%s: mktemp: %s: %s\n", progname, acTapeDir, strerror(errno));
		exit(1);
	}
	if (-1 == mkdir(acTapeDir, 0777)) {
		fprintf(stderr, "%s: mkdir: %s: %s\n", progname, acTapeDir, strerror(errno));
		exit(1);
	}
	if (-1 == chdir(acTapeDir)) {
		fprintf(stderr, "%s: chdir: %s: %s\n", progname, acTapeDir, strerror(errno));
		exit(1);
	}
	(void)setenv("TAPE_DIR", acTapeDir, 1);
	cmd_tape(sizeof(apcTapes)/sizeof(char*) - 1, apcTapes, & CSThis);
}

/* this function cleans up anything we left hanging in /tmp		(ksb)
 */
static void
exit_func()
{
	static char *apcRm[] = {
		"/bin/rm", "-rf", (char *)0, (char *)0
	};

	if ('\000' != acTapeDir[0]) {
		apcRm[2] = acTapeDir;
		(void)ForkExec(apcRm, "/");
		acTapeDir[0] = '\000';
	}
}

/* if positional parameters are provided they are already staged	(ksb)
 * directories for testing (do not delete them)
 * save me from mixed options!
 */
static void
every_func(pcDir)
char *pcDir;
{
	exit_func();	/* clean up last directory */
	if (-1 == chdir(pcDir)) {
		fprintf(stderr, "%s: chdir: %s: %s\n", progname, pcDir, strerror(errno));
		exit(1);
	}
	(void)setenv("TAPE_DIR", pcDir, 1);
	/* ZZZ: make the unpack command visible?
	 */
	cmd_from_file(& CSThis, stdin);
}

%%

before {
}

function 'c' {
	track named "cmd_from_string"
	update '%n(& CSThis, %N, (int *)0);'
	parameter "cmd"
	help "provide an interactive command"
}
file 'f' {
	track named "fpSource" "pcSource"
	user 'cmd_from_file(& CSThis, %n);'
	help "interpret commands from file"
}


zero {
	named "cmd_from_file"
	update 'if (!%rcU && !%rfU) {%n(& CSThis, stdin);}'
}

every {
	param "dir"
	help "directory already staged for us to use"
}

exit {
}
