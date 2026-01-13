#!mkcmd
# qadd [-nice] [-m address] cmd
# $Compile(*): %b %o -mMkcmd %f && %b %o prog.c
# $Mkcmd(*): ${mkcmd-mkcmd} std_help.m std_version.m ../common/qsys.m %f
from '<sys/time.h>'
from '<sys/file.h>'
from '<sys/types.h>'
from '<sys/stat.h>'
from '<sys/errno.h>'
from '<syslog.h>'
from '<signal.h>'
from '<errno.h>'
from '<ctype.h>'

from '"common.h"'
require "qsys.m" "std_help.m" "std_version.m"

%i
static char rcsid[] =
	"$Id: qadd.m,v 1.4 1997/01/06 17:22:43 kb207252 Exp $";

static char *pcMailPath;
%%

basename "qadd" ""
getenv "QADD"

# a kinda cryptic way to ask mkcmd to find a mail agent for us.
# search $PATH or %K<PATH> for one, take the first, and quote it.
key "mail" 1 init {
	"mailx" "Mail" "mail"
}
init "pcMailPath = %K<mail>?p1qv;"

boolean 'E' {
	named "fTrace"
	init "0"
	help "produce the C code for the queue file on stdout, only"
}

boolean 'x' {
	named "fX"
	init "0"
	help "trace commands as in sh(1)"
}

number {
	named "iNice"
	param "nice"
	initial "10"
	help "set the new processes nice value (default %qi)"
}

char* 'm' {
	param "address"
	named "pcAddr"
	help "mail address for exit status report"
	char* 's' {
		named "pcSubj"
		param "subject"
		init '"queue job %s complete"'
		help "printf format for mail mesage subject"
	}
}

char* 'N' {
	param "name"
	named "pcName"
	help "hide the commands given with this name"
}

from "<time.h>"
from "<pwd.h>"
from "<errno.h>"
from "<sys/param.h>"
list {
	named "Add"
	param "cmd"
	help "shell command for execution (mind your quotes)"
}

%%
extern char *getenv(), *mktemp(), *getlogin();
extern char *ctime();
extern int errno;
#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif

#if USE_GETCWD
#define getwd(Mp) getcwd(Mp,MAXPATHLEN) /* return pwd           */
extern char *getcwd();
#else
extern char *getwd();
#endif

/* output a positional parameter in C source form			(ksb)
 *	a"b"		-> "a\"b\""
 */
static void
CPutStr(fp, pc)
FILE *fp;
unsigned char *pc;
{
	(void)putc('\"', fp);
	for (;'\000' != *pc;++pc) {
		switch (*pc) {
		case '\t':
			putc('\\', fp);
			putc('t', fp);
			continue;
		case '\n':
			putc('\\', fp);
			putc('n', fp);
			continue;
		default:
			if (*pc < ' ' || *pc > 127) {
				fprintf(fp, "\\%03o", *pc);
			}
			putc(*pc, fp);
			continue;
		case '\\':
		case '\'':
		case '\"':
			putc('\\', fp);
			putc(*pc, fp);
			continue;
		}
	}
	(void)putc('\"', fp);
}

#include "qfuncs.i"
#include <sys/stat.h>

/* Build the queue C file we can run as nobody.qmgr to setuid and run	(ksb)
 * the required job.
 *
 * Since the queue is mode 770 (daemon.qmgr) others can't run these
 * setuid (and setgid) files to start the jobs.
 */
static void
Add(argc, argv)
int argc;
char **argv;
{
	register FILE *fpCode;
	static char acTempDir[MAXPATHLEN+1] = "/tmp/qXXXXX";
	register int i;
	auto char acCmd[4096];
	auto time_t tNow;
	auto char *pcWho, *pcTail;
	auto struct passwd *pwd;
	register char *pcNow, *pcTemp;
	auto Q QInto;
	auto char acMyId[32];
	auto int fdLock, fdTo;
	auto struct stat stEnv, stDot;
	auto int fIntr = 0;
	extern char **environ;

	if (argc < 1 || 1 == argc && '-' == argv[0][0] && '\000' == argv[0][1]) {
		static char *apcIntr[] = {
			"for c\ndo echo \"$c\"\ndone |sh",
			"qexec",
			(char *)0
		};
		static char acDriveX[] =
			"for c\ndo echo \"$c\"\ndone |sh -x";
		if (fX) {
			apcIntr[0] = acDriveX;
		}
		argc = 2;
		argv = apcIntr;
		fIntr = 1;
	}
	if ((char *)0 == mktemp(acTempDir)) {
		fprintf(stderr, "%s: mktemp: %s: %s\n", progname, acTempDir, strerror(errno));
		exit(1);
	}
	if (-1 == mkdir(acTempDir, 0770)) {
		fprintf(stderr, "%s: mkdir: %s: %s\n", progname, acTempDir, strerror(errno));
		exit(1);
	}
	(void)chown(acTempDir, -1, geteuid());
	pcTail = strlen(acTempDir) + acTempDir;

	/* open the C source file
	 */
	(void)strcpy(pcTail, "/q.c");
	if (fTrace) {
		fpCode = stdout;
	} else if ((FILE *)0 == (fpCode = fopen(acTempDir, "w"))) {
		fprintf(stderr, "%s: fopen: %s: %s\n", progname, acTempDir, strerror(errno));
		exit(1);
	}

	/* gather info about what to build
	 */
	pcWho = getlogin();
	if ((char *)0 == pcWho && (char *)0 == (pcWho = getenv("LOGNAME"))) {
		pcWho = getenv("USER");
	}
	if ((char *)0 == pcWho || (struct passwd *)0 == (pwd = getpwnam(pcWho)) || pwd->pw_uid != getuid()) {
		pwd = getpwuid(getuid());
	}
	pcWho = pwd->pw_name;

	time(& tNow);
	pcNow = ctime(& tNow);

	/* write file.  this C program is compiled and named
	 *	jid[+-]pri.position
	 * where jid is a `job id'
	 * and pri is a numeric nice value (-20 to +20)
	 * and position is the position this sits in the queue (1.. ???)
	 * we subtract the lowest file in the queue from all the subsequent
	 * positions (and add 1) so it looks like the queue starts at 1.
	 */

	fprintf(fpCode, "/* machine generated queue file for batch system\n");
	fprintf(fpCode, " *  by %s on %s", pcWho, pcNow);
	fprintf(fpCode, " */\n\n#include <stdio.h>\n");
	fprintf(fpCode, "#include <ctype.h>\n");
	fprintf(fpCode, "#include <strings.h>\n");
	fprintf(fpCode, "#include <sys/types.h>\n");

	/* build argv and envp arrays
	 */
	fprintf(fpCode, "static char *shargv[] = {\n");
	fprintf(fpCode, "\t\"sh\",\n");
	if (fX && !fIntr) {
		fprintf(fpCode, "\t\"-x\",\n");
	}
	fprintf(fpCode, "\t\"-c\"");
	for (i = 0; i < argc; ++i) {
		fprintf(fpCode, ",\n\t");
		CPutStr(fpCode, argv[i]);
	}
	if (fIntr) {
		auto char acLine[1024];
		register char *pcNl;
		register int fPrompt;

		fPrompt = isatty(0);
		for (;;) {
			if (fPrompt) {
				fputs("qadd> ", stdout);
				fflush(stdout);
			}
			if ((char *)0 == fgets(acLine, sizeof(acLine), stdin)) {
				break;
			}
			if ((char *)0 != (pcNl = strrchr(acLine, '\n')))
				*pcNl = '\000';
			fprintf(fpCode, ",\n\t");
			CPutStr(fpCode, acLine);
		}
		if (fPrompt)
			fputs("[EOT]\n", stdout);
	}
	fprintf(fpCode, ",\n\t(char *)0\n};\n");

	fprintf(fpCode, "\nstatic char *shenv[] = {\n");
	for (i = 0; (char *)0 != environ[i]; ++i) {
		fprintf(fpCode, "\t");
		CPutStr(fpCode, environ[i]);
		fprintf(fpCode, ",\n");
	}
	fprintf(fpCode, "#define QPASS %d\n\t(char *)0,\n", i);
	fprintf(fpCode, "\t(char *)0\n};\n");

	fprintf(fpCode, "\nint\nmain(argc, argv)\nint argc;\nchar **argv;\n");
	fprintf(fpCode, "{\n");
	if ((char *)0 == pcName) {
		fprintf(fpCode, "\tregister int w, i, j;\n");
	}
	fprintf(fpCode, "\tauto time_t tNow;\n");
	fprintf(fpCode, "\tstatic char acBinSh[] = \"/bin/sh\";\n");

	fprintf(fpCode, "\n\tif (argc < 2 || '-' != argv[1][0]) {\n");
	fprintf(fpCode, "\t\tfprintf(stderr, \"%%s: usage -e [variable]\\n\", argv[0]);\n");
	fprintf(fpCode, "\t\tfprintf(stderr, \"%%s: usage -q rank id nice\\n\", argv[0]);\n");
	fprintf(fpCode, "\t\texit(1);\n");
	fprintf(fpCode, "\t}\n");
	fprintf(fpCode, "\tswitch (argv[1][1]) {\n");
	fprintf(fpCode, "\tcase \'e': /* run the command */\n");
	fprintf(fpCode, "\t\tbreak;\n");
	fprintf(fpCode, "\tcase \'q\': /* output a status line */\n");
	fprintf(fpCode, "\t\tif (5 != argc) {\n");
	fprintf(fpCode, "\t\t\tfprintf(stderr, \"%%s: -q: needs 3 arguments\\n\", argv[0]);\n");
	fprintf(fpCode, "\t\t\texit(2);\n");
	fprintf(fpCode, "\t\t}\n");
	/* Sun Sep 16 01:03:52 1973\n\0
	 * 01234567890123456789012345 6
	 * Sep 16 01:03
	 */
	fprintf(fpCode, "\t\tprintf(\"%%3.3s %%5s %-8s %3.3s %2.2s %5.5s %%3d \", argv[2], argv[3], atoi(argv[4]));\n", pcWho, pcNow+4, pcNow+8, pcNow+11);
	if ((char *)0 == pcName) {
		fprintf(fpCode, "\t\tfor (w = 100, i = %d; w > 0  && (char *)0 != shargv[i]; ++i, --w) {\n", fIntr ? 4 : (2 + fX));
		fprintf(fpCode, "\t\t\tfor (j = 0; w > 1 && \'\\000\' != shargv[i][j]; ++j, --w) {\n");
		fprintf(fpCode, "\t\t\t\tputc(isprint(shargv[i][j]) ? shargv[i][j] : ' ', stdout);\n");
		fprintf(fpCode, "\t\t\t}\n");
		fprintf(fpCode, "\t\t\tputc(\'%c\', stdout);\n", fIntr ? ';' : ' ');
		fprintf(fpCode, "\t\t}\n");
		fprintf(fpCode, "\t\tputchar(\'\\n\');\n");
	} else {
		fprintf(fpCode, "\t\tputs(");
		CPutStr(fpCode, pcName);
		fprintf(fpCode, ");\n");
	}
	fprintf(fpCode, "\t\texit(0);\n");
	fprintf(fpCode, "\t}\n");

	/* copy any (single) environment variable from the qmgr
	 * for resources allocation (qmgr -Cvariable)
	 * How this code works, for example:
	 *  12345678...
	 * "MODEM=/dev/ttyc56"
	 *  ^    ^---pcEq (0x309)
	 *  pcVar  (0x304)
	 *
	 * 0x309 - 0x304 + 1 == 6
	 */
	fprintf(fpCode, "\n\tif ((char *)0 != argv[2]) {\n");
	fprintf(fpCode, "\t\tregister char *pcVar, *pcEq, **ppcScan;\n");
	fprintf(fpCode, "\t\tregister unsigned int uLen;\n");
	fprintf(fpCode, "\t\tif ((char *)0 != (pcEq = strchr(pcVar = argv[2], '='))) {\n");
	fprintf(fpCode, "\t\t\tuLen = (pcEq - pcVar) + 1;\n");
	fprintf(fpCode, "\t\t\tfor (ppcScan = shenv; (char *)0 != *ppcScan; ++ppcScan) {\n");
	fprintf(fpCode, "\t\t\t\tif (0 != strncmp(pcVar, *ppcScan, uLen))\n");
	fprintf(fpCode, "\t\t\t\t\tbreak;\n");
	fprintf(fpCode, "\t\t\t}\n\t\t} else {\n");
	fprintf(fpCode, "\t\t\tppcScan = & shenv[QPASS];\n\t\t}\n");
	fprintf(fpCode, "\t\t*ppcScan = pcVar;\n");
	fprintf(fpCode, "\t}\n\n");

	/* put a start message in the stdout log
	 */
	fprintf(fpCode, "\t(void)time(& tNow);\n");
	fprintf(fpCode, "\t(void)printf(\"%%24.24s %%s: for %s, %%7.2lf hour delay", pcWho);
	if ((char *)0 != pcAddr) {
		fprintf(fpCode, ", mail to %s", pcAddr);
	}
	fprintf(fpCode, "\\n\", ctime(& tNow), argv[0], (double)(tNow - %ld)/3600.0);\n", tNow);
	fprintf(fpCode, "\tfflush(stdout);\n");

	/* restore our umask since we might make files
	 */
	i = umask(077);
	fprintf(fpCode, "\n\t(void)umask(%04o);\n", i);
	(void)umask(i);

	/* restore our real and effective uids and gids
	 */
#if USE_SETREUID
	fprintf(fpCode, "\n\t(void)setreuid(geteuid(), geteuid());\n");
	fprintf(fpCode, "\t(void)setregid(getegid(), getegid());\n");
#else
	fprintf(fpCode, "\n\t(void)setuid(geteuid());\n");
	fprintf(fpCode, "\t(void)setgid(getegid());\n");
#endif

	/* restore our current working directory
	 */
	fprintf(fpCode, "\n\t(void)chdir(");
	if ((char *)0 != (pcTemp = getenv("PWD")) && -1 != stat(pcTemp, &stEnv) && -1 != stat(".", &stDot) && stEnv.st_ino == stDot.st_ino && stEnv.st_dev == stDot.st_dev) {
		CPutStr(fpCode, pcTemp);
	} else if ((char *)0 == getwd(acCmd)) {
		fprintf(stderr, "%s: getwd: %s\n", progname, acCmd);
		exit(6);
	} else {
		CPutStr(fpCode, acCmd);
	}
	fprintf(fpCode, ");\n");

	if ((char *)0 != pcAddr) {
		fprintf(fpCode, "\n\tdo {\n\t\tint afd[2];\t\tchar acSubj[256+32];\n\n\t\tif (-1 == pipe(afd)) {\n");
		fprintf(fpCode, "\t\t\tfprintf(stderr, \"%%s: no pipe\\n\", argv[0]);\n");
		fprintf(fpCode, "\t\t\tbreak;\n\t\t}\n\t\tswitch (fork()) {\n");
		fprintf(fpCode, "\t\tcase -1:\n\t\t\tclose(afd[0]), close(afd[1]);\n");
		fprintf(fpCode, "\t\t\tbreak;\n\t\tcase 0:\n\t\t\tclose(afd[1]);\n");
		fprintf(fpCode, "\t\t\tclose(0);\n\t\t\tdup(afd[0]);\n\t\t\tclose(afd[0]);\n");
		fprintf(fpCode, "\t\tsprintf(acSubj, ");
		CPutStr(fpCode, pcSubj);
		fprintf(fpCode, ", argv[0], argv[0]);\n");
		fprintf(fpCode, "\t\t\texecl(\"%s\", \"mail\", \"-s\", acSubj, ", pcMailPath);
		CPutStr(fpCode, pcAddr);
		fprintf(fpCode, ", (char *)0);\n\t\t\texecl(\"/bin/cat\", \"cat\", \"-\", (char *)0);\n");
		fprintf(fpCode, "\t\t\texit(1);\n\t\tdefault:\n\t\t\tclose(afd[0]);\n");
		fprintf(fpCode, "\t\t\tclose(1);\n\t\t\tdup(afd[1]);\n\t\t\tclose(2);\n");
		fprintf(fpCode, "\t\t\tdup(afd[1]);\n\t\t\tclose(afd[1]);\n");
		fprintf(fpCode, "\t\t\tbreak;\n\t\t}\n\t} while (0);\n\n");
	}

	/* set the nice value requested
	 */
	fprintf(fpCode, "\n\t(void)nice(%d);\n", iNice);

	/* now run the command we were given
	 */
	fprintf(fpCode, "\n\t(void)execve(acBinSh, shargv, shenv);\n");
	fprintf(fpCode, "\tfprintf(stderr, \"%%s: cannot find the shell (%%s)!\\n\", argv[0], acBinSh);\n");
	fprintf(fpCode, "\texit(1);\n}\n");

	if (stdout == fpCode) {
		fflush(stdout);
	} else {
		(void)fclose(fpCode);
	}

	/* compile file
	 */
	*pcTail = '\000';
	sprintf(acCmd, "cd %s && cc -o qfile q.c", acTempDir);
	if (fTrace) {
		fprintf(fpCode, "%s: %s\n", progname, acCmd);
	} else if (0 != system(acCmd)) {
		fprintf(stderr, "%s: compile of queue program failed\n", progname);
		exit(4);
	}
	(void)strcpy(pcTail, "/qfile");

	/* lock the queue and insert the job
	 */
	fdLock = LockQ(pcQDir);
	ScanQ(pcQDir, & QInto);

	(void)strcpy(acMyId, pcQDir);
	pcTemp = acMyId + strlen(acMyId);
	*pcTemp++ = '/';
	QName(&QInto, pcTemp, *pcWho, iNice);

	sprintf(acCmd, "mv %s %s", acTempDir, acMyId);
	if (fTrace) {
		fprintf(fpCode, "%s: %s\n", progname, acCmd);
		exit(0);
	} else {
		register int fdFrom, cc;
		auto char acBuf[8092];

		if (-1 == (fdFrom = open(acTempDir, 0, 0))) {
			fprintf(stderr, "%s: open: %s: %s\n", progname, acTempDir, strerror(errno));
			exit(1);
		}
		if (-1 == (fdTo = creat(acMyId, 0600))) {
			fprintf(stderr, "%s: create: %s: %s\n", progname, acMyId, strerror(errno));
			exit(4);
		}
		while (0 < (cc = read(fdFrom, acBuf, sizeof(acBuf)))) {
			if (cc != write(fdTo, acBuf, cc)) {
				fprintf(stderr, "%s: write: %d: %s\n", progname, fdTo, strerror(errno));
				exit(4);
			}
		}
		close(fdFrom);
		(void)unlink(acTempDir);
	}
	(void)fsync(fdTo);
	setgid(getgid());
	if (-1 == fchown(fdTo, getuid(), getgid()) || -1 == fchmod(fdTo, 06711)) {
		fprintf(stderr, "%s: chown or chmod: %s: %s\n", progname, acMyId, strerror(errno));
		(void)unlink(acMyId);
		exit(8);
	}

	UnLockQ(fdLock);

	*pcTail = '\000';
	sprintf(acCmd, "rm -rf %s", acTempDir);
	system(acCmd);

	exit(0);
}
