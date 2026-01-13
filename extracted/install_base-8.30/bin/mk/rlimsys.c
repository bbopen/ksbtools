/* $Id: rlimsys.c,v 5.0 1999/02/26 17:44:02 ksb Dist $
 * resource limited system	Kevin S Braunsdorf		(ksb)
 */
#include "machine.h"

#if RESOURCE

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#if defined(bsd)||defined(AIX)
#include <sys/wait.h>
#endif
#include <signal.h>
#include <stdio.h>
#include <ctype.h>

#include "mk.h"
#include "rlimsys.h"

#if defined(AIX)
#define	vfork	fork
#endif

extern char *progname;
extern int errno;

/*
 * resource records
 */
typedef struct RRnode {
	int rtype;		/* resource type from resource.h	*/
	int fsys;		/* done by kernel or us?		*/
	char *pchrname;		/* user name				*/
	int rlen;		/* lenght of user name			*/
	int fset;		/* was given by user?			*/
	struct rlimit ourlim;	/* rlim_cur, rlim_max			*/
} RES_REC;

#if RLIM_NLIMITS == 6
/* 4.X bsd system */
#else

#if RLIM_NLIMITS == 7
#if !defined(RLIMIT_NOFILE)
unknown_resource_configuration unknown_resource_configuration
#endif
/* Sun 4.1.1 or better */
#else
error_more_or_fewer_resources error_more_or_fewer_resources
#endif

#endif

#define MY_CLOCK	0
/*
 * resources we know about in alpha order
 */
RES_REC sbRRLimits[] = {
	{ MY_CLOCK,	0, "clock",	5 },
	{ RLIMIT_CORE,	1, "core",	4 },
	{ RLIMIT_CPU,	1, "cpu",	3 },
	{ RLIMIT_DATA,	1, "data",	4 },
	{ RLIMIT_FSIZE,	1, "fsize",	5 },
	{ RLIMIT_RSS,	1, "rss",	3 },
	{ RLIMIT_STACK,	1, "stack",	5 }
#if defined(RLIMIT_NOFILE)
	, { RLIMIT_NOFILE,1, "files",	5 }
#endif
};

/*
 * we need to know how many limits we can set
 */
#define MY_NLIMITS	(RLIM_NLIMITS+1)


/*
 * init the resource limit stuff, make mongo syscalls			(ksb)
 */
void
rinit()
{
	register int i;
	register RES_REC *pRR;

	for (i = 0; i < MY_NLIMITS; ++i) {
		pRR = & sbRRLimits[i];
		pRR->fset = 0;
		if (pRR->fsys) {
			getrlimit(pRR->rtype, & pRR->ourlim);
		} else {
			pRR->ourlim.rlim_cur = RLIM_INFINITY;
			pRR->ourlim.rlim_max = RLIM_INFINITY;
		}
	}
}

/*
 * parse a resource limit statement of the form				(ksb)
 *	<resource>=number_cur/number_max
 * (we eat leading blanks, as all good cvt routines should)
 */
char *
rparse(pchState)
register char *pchState;
{
	extern char *strchr();
	extern long atol();
	register int i;
	register char *pchTemp;
	register RES_REC *pRR;
	register int fSetMax = 0, fSetCur = 0;

	while (isspace(*pchState)) {
		++pchState;
	}
	if ((char *)0 == (pchTemp = strchr(pchState, '='))) {
		return pchState;
	}

	for (i = 0; i < MY_NLIMITS; ++i) {
		pRR = & sbRRLimits[i];
		if (0 == strncmp(pRR->pchrname, pchState, pRR->rlen)) {
			break;
		}
	}
	if (MY_NLIMITS == i) {
		return pchState;
	}

	do {
		++pchTemp;
	} while (isspace(*pchTemp));
	if (isdigit(*pchTemp)) {
		pRR->fset = 1;
		pRR->ourlim.rlim_cur = atol(pchTemp);
		fSetCur = 1;
		do {
			++pchTemp;
		} while (isdigit(*pchTemp));
	}
	if ('/' == *pchTemp)
		++pchTemp;
	while (isspace(*pchTemp))
		++pchTemp;

	if (isdigit(*pchTemp)) {
		pRR->fset = 1;
		pRR->ourlim.rlim_max = atol(pchTemp);
		fSetMax = 1;
		do {
			++pchTemp;
		} while (isdigit(*pchTemp));
	}
	while (isspace(*pchTemp))
		++pchTemp;
	if (fSetMax != fSetCur) {	/* only one set, set both same	*/
		if (fSetMax) {
			pRR->ourlim.rlim_cur = pRR->ourlim.rlim_max;
		} else {
			pRR->ourlim.rlim_max = pRR->ourlim.rlim_cur;
		}
	}
	return pchTemp;
}

int r_fTrace = 0;
static int iChildPid, fWarned;

/*
 * trap a time out for a "real time" alarm
 */
int
do_alarm()
{
	register unsigned next;

	if (kill(iChildPid, fWarned++ ? SIGALRM : SIGKILL) < 0) {
		fprintf(stderr, "%s: kill: %d: %s\n", progname, iChildPid, strerror(errno));
		exit(1);
	}
	next = (unsigned)(sbRRLimits[MY_CLOCK].ourlim.rlim_max -
		sbRRLimits[MY_CLOCK].ourlim.rlim_cur);
	if (0 == next) {
		next = 2;
	}
	alarm(next);
}

/*
 * emulate "system" with a CPU limit
 */
int
rlimsys(pchCmd)
char *pchCmd;
{
#if defined(bsd) && !defined(AIX)
	auto union wait waitbuf;
#else
	auto int waitbuf;
#endif
	register int wret;
#if SUNOS >= 40 || defined(AIX)
	register void (*atrap)(), (*itrap)(), (*qtrap)();
#else /* !SUNOS40 */
	register int (*atrap)(), (*itrap)(), (*qtrap)();
#endif /* SUNOS40 */
	register int i;
	register RES_REC *pRR;
	auto int iRetCode;

	iChildPid = vfork();
	switch (iChildPid) {
	case 0:
		for (i = 0; i < MY_NLIMITS; ++i) {
			pRR = & sbRRLimits[i];
			if (pRR->fset && pRR->fsys) {
				setrlimit(pRR->rtype, & pRR->ourlim);
			}
		}
		execl("/bin/sh", "sh", r_fTrace ? "-cx" : "-c", pchCmd, 0);
		_exit(127);
	case -1:
		fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		return 1;
	default:
		break;
	}

	itrap = signal(SIGINT, SIG_IGN);
	qtrap = signal(SIGQUIT, SIG_IGN);

	if (sbRRLimits[MY_CLOCK].fset) {
		atrap = signal(SIGALRM, do_alarm);
		alarm((unsigned)sbRRLimits[MY_CLOCK].ourlim.rlim_cur);
	}
	for (;;) {
		if (iChildPid == (wret = wait3(& waitbuf, WUNTRACED, (struct rusage *)0))) {
			if (WIFSTOPPED(waitbuf)) {
				if (kill(iChildPid, SIGCONT) < 0) {
					break;
				}
				continue;
			}
			if (WIFSIGNALED(waitbuf)) {
				iRetCode = waitbuf.w_termsig;
			} else {
				iRetCode = waitbuf.w_retcode;
			}
			break;
		}
		if (wret == -1) {
			fprintf(stderr, "%s: wait: %d: %s\n", progname, iChildPid, strerror(errno));
			iRetCode = 1;
			break;
		}
	}

	(void) signal(SIGINT, itrap);
	(void) signal(SIGQUIT, qtrap);
	if (sbRRLimits[MY_CLOCK].fset) {
		alarm((unsigned)0);
		(void) signal(SIGALRM, atrap);
	}
	return iRetCode;
}

#endif /* resource usage */
