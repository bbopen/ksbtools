#!mkcmd
# qmgr [-nv] [-d Q] jids
# $Compile(*): %b %o -mMkcmd %f && %b %o prog.c
# $Mkcmd(*): ${mkcmd-mkcmd} std_help.m std_version.m std_noargs.m ../common/qsys.m %f

from '<sys/time.h>'
from '<sys/file.h>'
from '<sys/types.h>'
from '<sys/stat.h>'
from '<sys/wait.h>'
from '<sys/errno.h>'
from '<signal.h>'
from '<errno.h>'
from '<sys/resource.h>'

from '"common.h"'

%i
static char rcsid[] =
	"$Id: qmgr.m,v 1.7 1998/02/12 14:25:49 ksb Exp $";

typedef struct PSnode {
	int iactive;
	int ipid;
	int iresource;
} TASKSTAT;
%%

basename "qmgr" ""

unsigned 's' {
	named "iPause"
	param "seconds"
	init "DEF_POLL"
	help "sleep time between polls on empty queues (default %qi)"
}

number {
	local named "iPara"
	init "0"
	param "para"
	help "number of jobs to run in parallel"
	track "iForcePara"
}

char* 'C' {
	named "pcCorrosp"
	param "variable"
	help "an environment variable to pass active resource values"

	zero {
		update 'fprintf(stderr, "%%s: no resources to manage\\n", %b);'
		abort "exit(1);"
	}
	list {
		param "resources"
		help "list of resource strings to manage"
		update "Mgr(iPara, %#, %@);"
		abort "exit(0);"
	}
}

exit {
	update "Mgr(iPara, 0, (char **)0);"
	abort "exit(0);"
}

%c
extern char *mktemp(), *getlogin();
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

static int *piReUsed;
static unsigned iReClock, iReMax;

/* setup the revolving door of round robin resource recovery		(ksb)
 * inv.: calloc put 0's in for us in piReUsed
 */
static void
ResourceInit(iHave)
unsigned int iHave;
{
	if (1 > (iReMax = iHave)) {
		return;
	}
	iReClock = 0;
	piReUsed = (int *)calloc(iReMax, sizeof(int));
	if ((int *)0 == piReUsed) {
		fprintf(stderr, "%s: calloc: %ud * %d: no more memory\n", progname, iReMax, sizeof(int));
		exit(4);
	}
}

/* get me the next round robin resource					(ksb)
 */
static int
ResourceGet()
{
	register int iRet;

	if (iReMax < 1) {
		return -1;
	}
	while (0 != piReUsed[iReClock]) {
		if (++iReClock == iReMax)
			iReClock = 0;
	}
	piReUsed[iRet = iReClock] = 1;

	if (++iReClock >= iReMax) {
		iReClock = 0;
	}
	return iRet;
}

#define ResourceFree(Mi)	(piReUsed[Mi] = 0)
/* --- end resource list abstraction --- */

static TASKSTAT aPSTask[MAXJOBS];

/* make the named task active on the named resource			(ksb)
 * resource value of (-1) means `not using one'.
 */
void
TaskStart(iSlot, iRes)
unsigned int iSlot;
int iRes;
{
	aPSTask[iSlot].iresource = iRes;
	aPSTask[iSlot].iactive = 1;
}

/* mark a task as done, free the resource it holds (if any)		(ksb)
 */
static void
TaskDone(iSlot)
unsigned iSlot;
{
	if (-1 != aPSTask[iSlot].iresource) {
		ResourceFree(aPSTask[iSlot].iresource);
		aPSTask[iSlot].iresource = -1;
	}
	aPSTask[iSlot].iactive = 0;
}

/* set the task structure up						(ksb)
 */
static void
TaskInit(iSlot)
unsigned iSlot;
{
	aPSTask[iSlot].iactive = 0;
	aPSTask[iSlot].iresource = -1;
	aPSTask[iSlot].ipid = -1;
}

/* tell me if a task is active						(ksb)
 */
#define TaskActive(Mi)	(0 != aPSTask[Mi].iactive)


/* --- end of task abstraction --- */

/* all process exits							(ksb)
 */
static void
ChildReap(iPid)
int iPid;
{
	register unsigned int i;
	auto char acWaste[MAXPATHLEN+1];

	for (i = 0; i < MAXJOBS; ++i) {
		if (!TaskActive(i) || aPSTask[i].ipid != iPid) {
			continue;
		}
		(void)sprintf(acWaste, acProtoClue, iPid);
		(void)unlink(acWaste);
		(void)sprintf(acWaste, acProtoRun, iPid);
		(void)unlink(acWaste);
		aPSTask[i].ipid = -1;
		TaskDone(i);
		return;
	}
}



static int fRunning;

static void
ShutDown()
{
	fRunning = 0;
}

/* we just need to force select to return				(ksb)
 */
static void
ForceInt()
{
#if !HAVE_STICKY_SIGNALS
	(void)signal(SIGCHLD, ForceInt);
#endif
}

/* run each job in turn, cleanup and be neat				(ksb)
 */
static void
Mgr(iJobs, iResource, ppcResources)
unsigned iJobs;
unsigned iResource;
char **ppcResources;
{
	register int i;
	register FILE *fpClue;
	register int iPid;
	auto Q QCur;
	auto char acRun[256];
	auto int fPrevIdle, fIdle, iRunning;
	auto int fd;
	auto char *pcQName, *pcTime;
	auto time_t tNow;
	auto char acHost[256];
	auto char *pcWhole, *pcRecord;

	/* open /dev/null as stdin so stuff won't block on our tty
	 */
	close(0);
	if (0 != open("/dev/null", 0, 0)) {
		fprintf(stderr, "%s: open: /dev/null: %s\n", progname, strerror(errno));
		exit(1);
	}
	if (-1 != gethostname(acHost, sizeof(acHost))) {
		register char *pcChop;

		/* XXX this assumes that all the hosts pulling from the
		 * queue are in the same local domain -- ksb
		 */
		if ((char *)0 != (pcChop = strchr(acHost, '.'))) {
			*pcChop++ = '\000';
		}
	} else {
		sprintf(acHost, "localhost");
	}

	i = MAXJOBS;
	if ((char *)0 != pcCorrosp && i > iResource) {
		i = iResource;
	}
	if (0 == iJobs) {
		iJobs = i;
	}
	if (iJobs > i) {
		fprintf(stderr, "%s: number of jobs reduced from %d to %d due to limited resources\n", progname, iJobs, i);
		iJobs = i;
	}
	if (iJobs < 1) {
		fprintf(stderr, "%s: number of jobs to run (%d) insane\n", progname, iJobs);
		exit(2);
	}

	/* we need a "var=value" buffer in pcWhole, pcRecord is
	 * just the "value" tail part
	 */
	if ((char *)0 != pcCorrosp) {
		register unsigned int uMax, uLen;

		ResourceInit(iResource);

		uLen = strlen(pcCorrosp);
		uMax = strlen(ppcResources[0]);
		for (i = 1; i < iResource; ++i) {
			register unsigned uCmp;
			uCmp = strlen(ppcResources[i]);
			if (uCmp > uMax)
				uMax = uCmp;
		}
		/* max strlen(resource string) + strlen("ENV=") + 1
		 * then round up to 8 bytes.
		 */
		uMax += uLen + 1 + 1;
		uMax |= 7;
		++uMax;

		if ((char *)0 == (pcWhole = malloc(uMax))) {
			fprintf(stderr, "%s: malloc: %ud: no more memory\n", progname, uMax);
			exit(4);
		}
		(void)sprintf(pcWhole, "%s=", pcCorrosp);
		pcRecord = pcWhole + uLen+1;
		(void)strcpy(pcRecord, "<none>");
	} else {
		pcWhole = (char *)0;
	}

	/* move to the queue
	 */
	if (-1 == chdir(pcQDir)) {
		fprintf(stderr, "%s: chdir: %s: %s\n", progname, pcQDir, strerror(errno));
		exit(1);
	}

	if (fVerbose) {
		printf("%s: managing `%s\' with %d task%s", progname, pcQDir, iJobs, 1 == iJobs ? "" : "s");
		if (0 < iResource)
			printf(" and %d resource%s", iResource, 1 == iResource ? "" : "s");
		printf("\n");
	}
	if ((char *)0 == (pcQName = strrchr(pcQDir, '/')))
		pcQName = pcQDir;
	else
		++pcQName;
	pcQDir = ".";

	/* remove the jobs left by other qmgrs with died on a system crash
	 */
	fd = LockQ(pcQDir);
	ScanQ(pcQDir, & QCur);
	for (i = 0; i < QCur.irun; ++i) {
		register char *pcName;
		pcName = QCur.ppDErun[i]->d_name;
		if (fVerbose) {
			printf("%s: %s: discarded old job %s\n", progname, pcQName, pcName);
		}
		if (fExec) {
			(void)unlink(pcName);
		}
		sprintf(acRun, acProtoClue, atoi(pcName+4));
		if (fVerbose) {
			printf("%s: %s: discarded clue file %s\n", progname, pcQName, acRun);
		}
		if (fExec) {
			(void)unlink(acRun);
		}
	}
	fflush(stdout);
	UnLockQ(fd);
	FreeQ(& QCur);

	/* setup running task list
	 */
	for (i = 0; i < MAXJOBS; ++i) {
		TaskInit(i);
	}
	if (iPause < MIN_POLL) {
		iPause = MIN_POLL;
	}

	/* enable async task cleanup and start them up
	 */
	fRunning = 1;
	(void)signal(SIGTERM, ShutDown);
	(void)signal(SIGCHLD, ForceInt);
	fPrevIdle = fIdle = 0;
	while (fRunning) {
		register char *pcName;
		register int iSlot;
		auto struct stat stCheck;
		auto struct timeval tvIdle;

		/* clean up any kids that died while we slept
		 */
		while (0 < (iPid = wait3((int *)0, WNOHANG, (struct rusage *)0))) {
			ChildReap(iPid);
		}

		/* find an empty (finished) task slot
		 * if there are none sleep for a sigchld
		 */
		iSlot = iJobs;
		for (iRunning = i = 0; i < iJobs; ++i) {
			if (TaskActive(i)) {
				++iRunning;
				continue;
			}
			iSlot = i;

			/* we have an empty slot to run in, clean out any
			 * old jobs before we launch new ones
			 */
			if (-1 == aPSTask[i].ipid)
				continue;
			write(2, "botch -1\n", 9);
			(void)sprintf(acRun, acProtoClue, aPSTask[i].ipid);
			(void)unlink(acRun);
			(void)sprintf(acRun, acProtoRun, aPSTask[i].ipid);
			(void)unlink(acRun);
			aPSTask[i].ipid = -1;
		}

		/* if no free slots, block in wait
		 * to clear a slot, then top of loop
		 */
		if (iRunning == iJobs) {
			iPid = wait((int *)0);
			if (-1 != iPid) {
				ChildReap(iPid);
			}
			continue;
		}

		/* get new jobs from the queue
		 */
		fd = LockQ(pcQDir);
		ScanQ(pcQDir, & QCur);

		for (i = 0; i < QCur.ijobs; ++i) {
			auto int afd[2];	/* pipe for kludge */

			pcName = QCur.ppDElist[i]->d_name;
			if (-1 == stat(pcName, & stCheck)) {
				continue;
			}

			/* last thing we set before the file is
			 * ready to run ...  (the mode see qadd.m)
			 */
			if (06711 != (07777 & stCheck.st_mode)){
				continue;
			}

			/* wow! we could run this one, fork a kid to do
			 * that.  Move the qfile to a run name while we
			 * run it so no one else will.
			 *
			 * We use the pipe below to sync the rename of the
			 * queue file for the qstat.
			 */
			if (-1 == pipe(afd)) {
				fprintf(stderr, "%s: pipe: %s\n", progname, strerror(errno));
				UnLockQ(fd);
				goto stall;
			}
			(void)fcntl(afd[1], F_SETFD, 1);

			if ((char *)0 != pcWhole) {
				register unsigned int iRes;
				iRes = ResourceGet();
				TaskStart(iSlot, iRes);
				(void)strcpy(pcRecord, ppcResources[iRes]);
			} else {
				TaskStart(iSlot, -1);
			}
			switch (iPid = fork()) {
				auto char acJunk[4];
			case -1:
				/* oops -- leave it in the queue while
				 * we wait for a free proc slot
				 */
				close(afd[0]), close(afd[1]);

				/* well, not done, but not started is the same
				 */
				TaskDone(iSlot);
				break;
			case 0:
				/* we are the process our mother told us
				 * about.  Build argv and exec'm
				 */
				(void)close(afd[0]);
				if (fVerbose) {
					printf("%s: running %s\n", progname, pcName);
				}
				execl(pcName, pcName, "-e", pcWhole, (char *)0);
				fprintf(stderr, "%s: execl: %s: %s\n", progname, pcName, strerror(errno));
				write(afd[1], "!\n", 2);
				/* oops */
				exit(126);
				/*NOTREACHED*/
			default:
				(void)close(afd[1]);
				(void)sprintf(acRun, acProtoRun, iPid);
				/* now the read below will return when the
				 * exec (or exit) above is done.  Yow!
				 */
				if (2 == read(afd[0], acJunk, sizeof acJunk)) {
					(void)close(afd[0]);
					TaskDone(iSlot);
					UnLockQ(fd);
					goto stall;
				}
				(void)close(afd[0]);
				if (-1 == rename(pcName, acRun)) {
					fprintf(stderr, "%s: rename: %s to %s: %s\n", progname, pcName, acRun, strerror(errno));
					/* N.B.: we have to exit or
					 * run this over and over again...
					 */
					exit(32);
				}
				UnLockQ(fd);
				time(& tNow);
				pcTime = ctime(& tNow);
				(void)sprintf(acRun, acProtoClue, iPid);
				if ((FILE *)0 != (fpClue = fopen(acRun, "w"))) {
					fprintf(fpClue, "    %-14.14s %12.12s  %2d\n", acHost, pcTime+4, atoi(pcName+3));
					fclose(fpClue);
				}
				aPSTask[iSlot].ipid = iPid;
				++iRunning;
				goto scan;
			}
			break;
		}

		/* if we get here we found nothing to run (yet)
		 */
		UnLockQ(fd);

		/* if we _were_ not idle and we are now log that
		 */
		if (!fIdle && 0 == iRunning) {
			time(& tNow);
			printf("%.19s: %s: queue idle, poll every %d second%s\n", ctime(& tNow), pcQName, iPause, 1 == iPause ? "" : "s");
			fflush(stdout);
		}
 stall:
		tvIdle.tv_sec = iPause;
		select(0, (char *)0, (char *)0, (char *)0, & tvIdle);
 scan:
		/* we launched some one find a new slot */
		FreeQ(& QCur);
		fIdle = 0 == iRunning;
	}
	printf("%s: %s: waiting for all tasks\n", progname, pcQName);
	fflush(stdout);
	while (0 < (iPid = wait((int *)0))) {
		ChildReap(iPid);
	}
}
