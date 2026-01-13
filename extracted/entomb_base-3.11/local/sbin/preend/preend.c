/* $Id: preend.c,v 3.15 2008/07/10 20:44:10 ksb Exp $
 * the main loop for the preen daemon.  This code could all run as root
 * because it need to chown things to charon.charon and read disks (df).
 */
#ifndef lint
static char *rcsid=
	"$Id: preend.c,v 3.15 2008/07/10 20:44:10 ksb Exp $";
#endif

#include <stdio.h>
#include <signal.h>
#include <syslog.h>
#include <pwd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sysexits.h>

#include "machine.h"
#include "libtomb.h"
#include "fslist.h"
#include "preentomb.h"
#include "main.h"

#if NEED_FSTAB_H
#include <fstab.h>
#endif

#if HAVE_STDLIB
#include <stdlib.h>
#else
extern char *malloc();
#endif

#if USE_STRINGS
#include <strings.h>
#else
#include <string.h>
#endif

#if !HAVE_STRERROR
extern int errno;
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif

#if USE_FCNTL_H
#include <fcntl.h>
#endif

extern int errno;
#if HAS_UNIONWAIT
#include <sys/wait.h>
#endif


/* become a daemon
 */
static void
daemonize()
{
	int res, td;

	(void) signal(SIGQUIT, SIG_IGN);
	(void) signal(SIGINT,  SIG_IGN);
#if defined(BSD4_2) || defined(BSD4_3)
	(void) signal(SIGTTOU, SIG_IGN);
	(void) signal(SIGTSTP, SIG_IGN);
#endif /* BSD4_2 || BSD4_3 */

	if (-1 == (res = fork())) {
		fprintf(stderr, "%s: couldn't start\n", progname);
		fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
		exit(EX_OSERR);
	}
	if (0 != res) {			  /* if the parent */
		exit(EX_OK);
	}

	/*
	 * lose our controlling terminal
	 */

#ifdef BSD2_9
	td = open("/dev/tty", O_RDWR);
#else /* BSD2_9 */
	td = open("/dev/tty", O_RDWR, 0600);
#endif /* BSD2_9 */

#if defined(TIOCNOTTY)
	if (td >= 0) {
		(void) ioctl(td, TIOCNOTTY, (char *)NULL);
	}
#endif

	/*
	 * Further disassociate this process from the terminal
	 * Maybe this will allow you to start a daemon from rsh,
	 * i.e. with no controlling terminal.
	 */

	(void) setpgid(0, getpid());
}


static TombInfo *pTIList;
static char **ppcArgv;			/* for SetProcName		*/

/* setup all the params that we need from the running system		(ksb)
 */
int
Init(ppc)
char **ppc;
{
	register struct passwd *pwent;
	register char *pc;
	auto uid_t uidCharon;
	auto gid_t gidCharon;

	/* SetProcName will clobber our progname, *sigh*
	 */
	ppcArgv = ppc;
	if ((char *)0 == (pc = malloc(strlen(progname)+1))) {
		fprintf(stderr, "%s: out of memory\n", progname);
		exit(EX_OSERR);
	}
	progname = strcpy(pc, progname);
	pTIList = (TombInfo *)0;

	/* we try to put cores in a well-known place
	 */
	if (-1 == chdir("/")) {
		fprintf(stderr, "%s: chdir: /: %s\n", progname, strerror(errno));
		exit(EX_OSFILE);
	}

	setpwent();
	if (NULL == (pwent = getpwnam(TOMB_OWNER))) {
		fprintf(stderr, "%s: getpwname: %s: login not found\n", progname, TOMB_OWNER);
		return 1;
	}
	uidCharon = pwent->pw_uid;
	gidCharon = pwent->pw_gid;
	endpwent();

	if (TOMB_GID != gidCharon || TOMB_UID != uidCharon) {
		fprintf(stderr, "%s: owner `%s\' has the wrong numeric uid.gid (coded %d.%d != passwd %d.%d)\n", progname, TOMB_OWNER, TOMB_UID, TOMB_GID, uidCharon, gidCharon);
		return 1;
	}
	if (Debug) {
		printf("Locating tomb directories\n");
	}
	pTIList = FsList(TOMB_GID, (dev_t)NOT_A_DEV);
	return 0;
}


/* don't let little kids play with big guns, some one might get burned	(ksb)
 */
int
After()
{
	/* Let only root start this daemon
	 */
	if (0 != getuid()) {
		fprintf(stderr, "%s: must be the superuser to run this program\n", progname);
		exit(EX_NOPERM);
	}
	if (Debug && (TombInfo *)0 == pTIList) {
		fprintf(stderr, "%s: no tombs on this host?\n", progname);
	}
	return 0;
}


/* show the user what who we are, kinda
 */
int
Version()
{
	register TombInfo *pTI;
#if !USE_STATFS
	register int fd;
#endif

	printf("%s: %s\n", progname, "$Id: preend.c,v 3.15 2008/07/10 20:44:10 ksb Exp $");
	printf("%s: tombs must be owned by %s (uid %d, gid %d)\n", progname, TOMB_OWNER, TOMB_UID, TOMB_GID);
	if ((TombInfo *)0 == pTIList) {
		printf("%s: no tombs on this host\n", progname);
		return 1;
	}
	printf("%s: tombs on this host:\n", progname);
	for (pTI = pTIList; 0 != pTI; pTI = pTI->pTINext) {
		printf("%s:  %s on %s", progname, pTI->pcFull, pTI->pcDev);
		if ((char *)0 != strchr(pTI->pcDev, ':')) {
			printf(" [nfs]\n");
			continue;
		}
#if USE_STATFS
		printf(" (%2d%% full)\n", HowFull1(pTI->pcFull));
#else
		if (-1 != (fd = open(pTI->pcDev, O_RDONLY, 0600))) {
			printf(" (%2d%% full)\n", HowFull2(fd));
			(void)close(fd);
		} else {
			printf(" [missing device node?]\n");
		}
#endif
	}
	return 0;
}


/* get the tomb info for a tomb name					(ksb)
 *  "/usera/tomb" -> {"/dev/sd0a", "/usera/tomb"}
 */
TombInfo *
Find(pcName, fRm)
char *pcName;
int fRm;
{
	register TombInfo *pTI, **ppTI;

	for (ppTI = &pTIList; 0 != (pTI = *ppTI); ppTI = &pTI->pTINext) {
		if (0 != strcmp(pcName, pTI->pcDir)) {
			continue;
		}
		if (fRm) {
			*ppTI = pTI->pTINext;
		}
		return pTI;
	}
	return 0;
}

/* set up our argv0 to say what we are doing
 *	NB: argv[0]...argv[argc-1] are all trash after this.
 */
void
SetProcName(pcTitle)
char *pcTitle;
{
	static char *pcEnd = (char *)0;
	static char *pcBegin = (char *)0;
	register char *pcArgv0;
	register char *pc;

	if ((char *)0 == pcBegin) {
		pcBegin = ppcArgv[0];
		ppcArgv[0] = (char *)0;
		ppcArgv[1] = (char *)0;
	}
	if ((char *)0 == pcEnd) {
		pcEnd = strlen(pcBegin)+pcBegin;
	}
	pc = pcBegin;
	*pc++ = '-';		/* ps will show real name */

	/* fill in all the space we have
	 */
	while (pc < pcEnd) {
		if ('\000' != *pcTitle)
			*pc++ = *pcTitle++;
		else
			*pc++ = PS_FILL;
	}
}


/* get a list of tombs to prune						(ksb)
 *  look through each tomb for files to dink
 *  sleep for a while and do it again
 * NB: we could vary the sleep time based on info from the PreenTomb
 *   and/or PreenCrypt routines, but that might lead us to sleep to
 *   little (we would use the User Given sleep-time as a max in that
 *   case).
 */
int
PreenList(argc, argv)
int argc;
char **argv;
{
	register TombInfo *pTI, *pTIDo;
	register int i, iErrs;
	register char *pcTodo;
	auto int iMasterPid;
	auto char acSleep[1024], acRun[1024];
	auto struct stat stTomb;

	iErrs = 0;
	if (argc != 0) {
		pTIDo = 0;
		for (i = 0; i < argc; ++i) {
			register char *pcEnd;
			pcTodo = argv[i];
			if ((char *)0 == pcTodo) {
				fprintf(stderr, "%s: PreenOne: nil pointer\n", progname);
				continue;
			}
			/* might have given the mt point -- fix that
			 */
			if ((char *)0 == (pcEnd = strrchr(pcTodo, '/'))) {
				pcEnd = pcTodo;
			} else {
				++pcEnd;
			}
			if (0 != strcmp(pcEnd, TOMB_NAME)) {
				sprintf(acSleep, "%s/%s", pcTodo, TOMB_NAME);
				pcTodo = malloc(strlen(acSleep)+1);
				if ((char *)0 == pcTodo) {
					fprintf(stderr, "%s: out of memory\n", progname);
					exit(EX_OSERR);
				}
				argv[i] = strcpy(pcTodo, acSleep);
			}
			if (0 != (pTI = Find(pcTodo, 1))) {
				pTI->pTINext = pTIDo;
				pTIDo = pTI;
				continue;
			}

			/* oops -- what broke?
			 */
			++iErrs;
			if (-1 == stat(pcTodo, & stTomb)) {
				if (ENOENT == errno) {
					fprintf(stderr, "%s: build tomb with `install -d -m 0770 -o %s -g %s %s\'\n", progname, TOMB_OWNER, TOMB_OWNER, pcTodo);
				} else {
					fprintf(stderr, "%s: stat: %s: %s\n", progname, pcTodo, strerror(errno));
				}
				continue;
			}
			if (S_IFDIR != (S_IFMT & stTomb.st_mode)) {
				fprintf(stderr,  "%s: %s: must be a directory\n", progname, pcTodo);
				continue;
			}
			if (0770 != (01777 & stTomb.st_mode)) {
				fprintf(stderr,  "%s: %s: is the wrong mode (0770 != %04o)\n", progname, pcTodo, 0777 & stTomb.st_mode);
				continue;
			}
			if (TOMB_UID != stTomb.st_uid) {
				fprintf(stderr,  "%s: %s: must be owned by %s (tomb %d != uid %d)\n", progname, pcTodo, TOMB_OWNER, stTomb.st_uid, TOMB_UID);
				continue;
			}
			if (TOMB_GID != stTomb.st_gid) {
				fprintf(stderr,  "%s: %s: must be grouped to %s (tomb %d != gid %d)\n", progname, pcTodo, TOMB_OWNER, stTomb.st_gid, TOMB_GID);
				continue;
			}
			fprintf(stderr, "%s: %s: not at a mount point?\n", progname, pcTodo);
		}
		if (0 != iErrs) {
			return iErrs;
		}
	} else {
		register TombInfo *pTITemp;

		/* do all non-NFS mounted tombs
		 */
		pTIDo = 0;
		pTI = pTIList;
		while (0 != (pTITemp = pTI)) {
			pTI = pTI->pTINext;
#if !defined(major)
#define major(Mm)	((Mm >> 8) & 0xff)
#endif
			if (major(pTITemp->tdev) >= 0x80)	/* NFS? */
				continue;
			/* or the device has a : in is */
			if ((char *)0 != strchr(pTITemp->pcDev, ':'))
				continue;
			pTITemp->pTINext = pTIDo;
			pTIDo = pTITemp;
		}
	}
	if (0 == pTIDo) {
		if (Debug)
			fprintf(stderr, "%s: no active tombs\n", progname);
		return 0;
	}

	/* we have a list of tombs, get setup as daemon(s) and such
	 */
	if (! Debug && ! fOnce) {
		daemonize();
	}

	if (fSingle) {
		sprintf(acSleep, "sleep /");
		sprintf(acRun, "clean all tombs");
	} else {
		iMasterPid = getpid();

		/* fork of kids to do the other tombs
		 */
		for (pTI = pTIDo; 0 != pTI; pTI = pTI->pTINext) {
			register char *pcNewName;

			/* parrent becomes the last one
			 */
			if (0 != pTI->pTINext) {
				switch (fork()) {
				case -1:
					if (Debug)
						fprintf(stderr, "%s: fork: %s\n", progname, strerror(errno));
					exit(EX_OSERR);
				case 0:
					break;
				default:
					continue;
				}
			}

			/* We are the daemon for this tomb, chdir to
			 * the tomb to optimize namei not to go through
			 * as much code, as the full path name might. So:
			 *	/usera/tomb/11329/motd@007
			 * becomes:
			 *	cd /usera/tomb ; ./11329/motd@007
			 * which PreenTomb will change to:
			 *	cd /usera/tomb ; 11329/motd@007
			 */
			if (-1 == chdir(pTI->pcFull)) {
				fprintf(stderr, "%s: chdir: %s: %s\n", progname, pTI->pcDir, strerror(errno));
				exit(EX_CONFIG);
			}
			pTIDo = pTI;
			pTIDo->pTINext = 0;
			pTIDo->pcDir = ".";

			/* set progname so we see which daemon we are
			 */
			pcNewName = malloc(((strlen(progname)+2+strlen(pTI->pcFull))|7)+1);
			sprintf(pcNewName, "%s: %s", progname, pTI->pcFull);
			progname = pcNewName;
			sprintf(acSleep, "sleep %s", pTI->pcFull);
			sprintf(acRun, "clean %s", pTI->pcFull);

			/* fall into mainline again
			 */
			break;
		}
	}

	/* we might be a daemon now, so tell syslogd we are here
	 */
	(void)openlog(progname, LOG_PID, LOG_DAEMON);

	/* now sit in an infinite loop and work
	 */
	for (;;) {
		/* Actually preen the given tomb
		 */
		SetProcName(acRun);
		for (pTI = pTIDo; 0 != pTI; pTI = pTI->pTINext) {
			if (Debug) {
				fprintf(stderr, "%s: Cleaning %s\n", progname, pTI->pcDir);
			}
			PreenTomb(pTI->pcDev, pTI->pcDir);
		}
		if (fOnce)
			break;
		SetProcName(acSleep);
		if (Debug) {
			printf("%s: sleeping %d minutes\n", progname, sleeptime);
		}
		(void)sleep((unsigned) sleeptime * 60);
	}

	/* we only had to run once, 
	 */
	if (!fSingle && iMasterPid == getpid()) {
		/* ZZZ look at kids exit code? */
#if HAS_UNIONWAIT
		auto union wait wBuf;
#else
		auto int wBuf;
#endif
		SetProcName("waiting for kids");
		while (-1 != wait(& wBuf)) {
#if HAS_UNIONWAIT
			iErrs += 0 != wBuf.w_retcode;
#else
			iErrs += 0 != ((wBuf>>8) & 0xff);
#endif
		}
	}
	closelog();
	return iErrs;
}
