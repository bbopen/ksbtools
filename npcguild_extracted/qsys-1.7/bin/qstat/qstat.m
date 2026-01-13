#!mkcmd
# qstat [-nv] [-d Q]
# $Compile(*): %b %o -mMkcmd %f && %b %o prog.c
# $Mkcmd(*): ${mkcmd-mkcmd} std_help.m std_version.m std_noargs.m ../common/qsys.m %f

from '<sys/time.h>'
from '<sys/file.h>'
from '<sys/types.h>'
from '<sys/stat.h>'
from '<sys/errno.h>'
from '<syslog.h>'
from '<signal.h>'
from '<errno.h>'
from '<pwd.h>'

from '"common.h"'

%i
static char rcsid[] =
	"$Id: qstat.m,v 1.2 1997/01/05 21:21:29 kb207252 Exp $";

extern char *getenv(), *mktemp(), *getlogin();
extern char *ctime();
extern int errno;
%%

basename "qstat" ""
getenv "QSTAT"

exit {
	named "Queue"
	abort "exit(0);"
}

%%
extern char *getenv(), *mktemp(), *getlogin();
extern char *ctime();
extern int errno;
#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif

#if defined(SYSV) || defined(HPUX7) || defined(HPUX8) || defined(IBMR2)
#define getwd(Mp) getcwd(Mp,MAXPATHLEN) /* return pwd           */
extern char *getcwd();
#else
extern char *getwd();
#endif

#include "qfuncs.i"

/* output a nifty table of the queue'd jobs				(ksb)
 */
static void
Queue(argc, argv)
int argc;
char **argv;
{
	register int i, l;
	auto int fd;
	auto Q QCur;
	auto char acCmd[4096], acClue[1025];
	register char *pcName;

	/* make sure it is a job queue and lock the queue
	 */
	fd = LockQ(pcQDir);
	ScanQ(pcQDir, & QCur);
	UnLockQ(fd);

	if (0 == QCur.ijobs && 0 == QCur.irun) {
		printf("zero files in queue\n");
		exit(0);
	}

	if (0 != QCur.iuser) {
		register struct passwd *pwd;
		register FILE *fpUser;
		register int c;

		setpwent();
		printf("%8s %s\n", "Who", "Locked Pid");
		/* uid_294 */
		for (i = 0; i < QCur.iuser; ++i) {
			pcName = QCur.ppDEuser[i]->d_name;
			if ((struct passwd *)0 == (pwd = getpwuid(atoi(pcName+4))))
				printf("%8s ", pcName+4);
			else
				printf("%8s ", pwd->pw_name);
			sprintf(acClue, "%s/%s", pcQDir, pcName);
			if ((FILE *)0 == (fpUser = fopen(acClue, "r"))) {
				printf("<unknown>\n");
				continue;
			}
			c = ' ';
			do {
				putchar(c);
			} while (EOF != (c = getc(fpUser)));
			(void)fclose(fpUser);
		}
		endpwent();
	}
	printf("Rank  Job   Who        Date     Nice    Command\n");
	fflush(stdout);
	for (i = 0; i < QCur.irun; ++i) {
		register int c;
		register FILE *fpClue;
		register char *pcType = "run";

		/* run_pid */
		pcName = QCur.ppDErun[i]->d_name;
		sprintf(acCmd, "%s/%s -q %s '%5d' '%3.3d'", pcQDir, pcName, pcType, atoi(pcName+4), 0);
		if (fExec)
			system(acCmd);
		else
			printf("%s\n", acCmd);
		if (!fVerbose)
			continue;

		sprintf(acClue, "%s/clue_%d", pcQDir, atoi(pcName+4));
		if ((FILE *)0 == (fpClue = fopen(acClue, "r")))
			continue;

		while (EOF != (c = getc(fpClue)))
			putchar(c);
		fclose(fpClue);
	}
	for (i = 0; i < QCur.ijobs; ++i) {
		/* iii-dd.seq */
		pcName = QCur.ppDElist[i]->d_name;
		sprintf(acCmd, "%s/%s -q '%3d' ' %3.3s ' '%3.3s'", pcQDir, pcName, i+1, pcName, pcName+3);
		if (fExec)
			system(acCmd);
		else
			printf("%s\n", acCmd);
	}
	exit(0);
}
