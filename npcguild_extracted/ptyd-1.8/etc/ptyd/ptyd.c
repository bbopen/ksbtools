/*
 * $Id: ptyd.c,v 1.23 1997/11/29 17:58:50 ksb Exp $
 *
 * ptyd -  pseudo-tty daemon
 *	
 * Copyright (C) 1988 Michael Rowan
 *
 * Everyone is granted permision to copy, modify and 
 * redistribute this code provided that 1) you leave this
 * copyright message and 2) you send changes/fixes back to
 * me so I can include them in the next release.
 *
 * by:
 *	Mike Rowan		(mtr@clam.com)?
 *	Kevin Braunsdorf	(ksb@fedex.com)
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/syslog.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <ctype.h>
#include <setjmp.h>

#include "machine.h"
#include "daemon.h"
#include "main.h"
#include <ptyd.h>

#if POSIX
#include <limits.h>
#endif

#if POSIX || NEED_UNISTD_H
#include <unistd.h>
#endif

#if NEED_GNU_TYPES
#include <gnu/types.h>
#endif

#if HAVE_SELECT_H
#include <sys/select.h>
#endif


static int uidTty, gidTty;
static int uidPty, gidPty;
static struct stat statProto;

static jmp_buf JBMain;
static int sCtl, fRunning;
static int iChildren, *piChildPids;
static int fChild;

static int iLogPri = LOG_DEBUG;
#define PTYD 1

#include "common.i"

/*
 * make the server port for the server to listen on			(ksb)
 * prototyped from screen
 */
static int
MakeServerPort(pcName)
char *pcName;
{
	register int sCtl;
	auto struct sockaddr_un run;

	(void) unlink(pcName);
	if ((sCtl = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "%s: socket: %s\n", progname, strerror(errno));
		return -1;
	}
	run.sun_family = AF_UNIX;
	(void) strcpy(run.sun_path, pcName);
	if (-1 == bind(sCtl, (struct sockaddr *)&run, strlen(pcName)+2)) {
		fprintf(stderr, "%s: bind: %s\n", progname, strerror(errno));
		return -1;
	}
	if (-1 == listen(sCtl, 6)) {
		fprintf(stderr, "%s: listen: %s\n", progname, strerror(errno));
		return -1;
	}
	chmod(pcName, 0777);
	return sCtl;
}


/*
 * scan /dev/pty?? and look for un-used ptys to reclaim			(ksb)
 */
char *
DoScan()
{
	auto char *pcOne = charone, *pcTwo = chartwo;
	auto int fd, iIndex = sizeof(PTY_PREFIX)-1;
	auto struct stat statPty, statTty;
	static char acStats[100];	/* "(2+9)/145" */
	register int iFound = 0, iClean = 0, iInUse = 0;

	for (pcOne = charone; '\000' != (acMaster[iIndex] = *pcOne); ++pcOne) {
		acSlave[iIndex] = *pcOne;
		for (pcTwo = chartwo; '\000' != (acMaster[iIndex+1] = *pcTwo); ++pcTwo) {

			if (-1 == stat(acMaster, &statPty) || major(statPty.st_dev) != major(statProto.st_dev) || S_IFCHR != (statPty.st_mode&S_IFMT)) {
				break;
			}
			acSlave[iIndex+1] = *pcTwo;
			if (-1 == stat(acSlave, & statTty)) {
				syslog(LOG_ERR, "missing slave tty %s", acSlave);
				break;
			}
			++iFound;
			if (statPty.st_uid == uidPty && statPty.st_gid == gidPty && (SAFE_MASK&statPty.st_mode) == PTYMODE && statTty.st_uid == uidTty && statTty.st_gid == gidTty && (SAFE_MASK&statTty.st_mode) == TTYMODE) {
				continue;
			}

			if (0 > (fd = open(acMaster, O_RDWR|O_NDELAY, 0))) {
				++iInUse;
				continue;
			}

			chmod(acMaster, PTYMODE);
			chmod(acSlave, TTYMODE);
			chown(acMaster, uidPty, gidPty);
			chown(acSlave, uidTty, gidTty);

			(void) delog_utmp(acSlave, (uid_t)statPty.st_uid);
			close(fd);
			++iClean;
		}
	}
	sprintf(acStats, "(%d+%d)/%d", iClean, iInUse, iFound);
	return acStats;
}


/*
 * get a pty, chmod it, chown it
 */
int
GetPty(pcUSlave, pcUMaster, uid)
char *pcUSlave, *pcUMaster;
uid_t uid;
{
	auto int fd;
	auto char *pcMaster, *pcSlave;
	
	if (-1 == (fd = getpseudotty(&pcSlave, &pcMaster))) {
		return -1;
	}

	/*
	 * chmod to 000 to protect against faked requests
	 */
	fchmod(fd, 0000);
	chmod(pcSlave, 0000);
	fchown(fd, uid, gidPty);
	chown(pcSlave, uid, gidTty);

	/*
	 * copy the string back to the main program
	 */
	(void)strcpy(pcUMaster, pcMaster);
	(void)strcpy(pcUSlave, pcSlave);
	return fd;
}


/*
 * make a utmp entry for the pty we know the user is on			(ksb)
 */
int
MkUtmp(pcPty, pcMach)
char *pcPty, *pcMach;
{
	auto char acTty[MAXPATHLEN+1];
	auto struct stat statBuf;
	auto int fd;

	if ((char *)0 == WhichTty(acTty, pcPty)) {
		return -1;
	}
	if (-1 == stat(pcPty, &statBuf)) {
		syslog(LOG_INFO, "stat: %s: %m", pcPty);
		return -1;
	}
	if (major(statBuf.st_dev) != major(statProto.st_dev)) {
		syslog(LOG_ERR, "%s: wrong major device", pcPty);
		return -1;
	}
	if (SAFE_ACK != (statBuf.st_mode&SAFE_MASK) || S_IFCHR != (statBuf.st_mode&S_IFMT)) {
		syslog(LOG_DEBUG, "%s: wrong modes", pcPty);
		return -1;
	}
	if (-1 != (fd = open(pcPty, O_RDONLY|O_NDELAY, 0))) {
		close(fd);
		return -1;
	}
	return log_utmp(acTty, (uid_t)statBuf.st_uid, pcMach);
}

/*
 * remove a utmp entry for the user
 */
int
RmUtmp(pcPty)
char *pcPty;
{
	auto char acTty[MAXPATHLEN+1];
	auto struct stat statBuf;
	auto int fd;

	if ((char *)0 == WhichTty(acTty, pcPty)) {
		return -1;
	}
	if (-1 == stat(pcPty, &statBuf)) {
		syslog(LOG_INFO, "stat: %s: %m", pcPty);
		return -1;
	}
	if (major(statBuf.st_dev) != major(statProto.st_dev)) {
		syslog(LOG_ERR, "%s: wrong major device", pcPty);
		return -1;
	}
	if (SAFE_DONE != (statBuf.st_mode&SAFE_MASK) || S_IFCHR != (statBuf.st_mode&S_IFMT)) {
		syslog(LOG_DEBUG, "%s: wrong modes", pcPty);
		return -1;
	}
	if (-1 != (fd = open(pcPty, O_RDONLY|O_NDELAY, 0))) {
		close(fd);
		return -1;
	}
	return delog_utmp(acTty, (uid_t)statBuf.st_uid);
}


/*
 * open wtmp for a pty							(ksb)
 */
int
Login(pcPty)
char *pcPty;
{
	auto struct stat statBuf;
	auto char acTty[MAXPATHLEN+1];

	if ((char *)0 == WhichTty(acTty, pcPty)) {
		return -1;
	}

	/*
	 *  Check to see that pty is mode 0600 (and used)
	 */
	if (0 != stat(pcPty, &statBuf)) {
		syslog(LOG_INFO, "stat: %s: %m", pcPty);
		return -1;
	}
	if (major(statBuf.st_dev) != major(statProto.st_dev)) {
		syslog(LOG_ERR, "%s: wrong major device", pcPty);
		return -1;
	}
	if (SAFE_ACK != (statBuf.st_mode&SAFE_MASK) || S_IFCHR != (statBuf.st_mode&S_IFMT)) {
		syslog(LOG_DEBUG, "%s: wrong modes", pcPty);
		return -1;
	}

	return log_wtmp(acTty, (uid_t)statBuf.st_uid);
}


/*
 * close wtmp for a pty							(ksb)
 */
int
Logout(pcPty)
char *pcPty;
{
	auto struct stat statBuf;
	auto char acTty[MAXPATHLEN+1];

	if ((char *)0 == WhichTty(acTty, pcPty)) {
		return -1;
	}

	/*
	 *  Check to see that pty is mode 04000 (and un-used)
	 */
	if (0 != stat(pcPty, &statBuf)) {
		syslog(LOG_INFO, "stat: %s: %m", pcPty);
		return -1;
	}
	if (major(statBuf.st_dev) != major(statProto.st_dev)) {
		syslog(LOG_ERR, "%s: wrong major device", pcPty);
		return -1;
	}
	if (SAFE_DONE != (statBuf.st_mode&SAFE_MASK) || S_IFCHR != (statBuf.st_mode&S_IFMT)) {
		syslog(LOG_DEBUG, "%s: wrong modes", pcPty);
		return -1;
	}

	return delog_wtmp(acTty, (uid_t)statBuf.st_uid);
}


/*
 * cleanup a pty							(ksb)
 */
int
CleanPty(pcPty)
char *pcPty;
{
	auto struct stat statBuf;
	auto char acTty[MAXPATHLEN+1];

	if ((char *)0 == WhichTty(acTty, pcPty)) {
		return -1;
	}

	/*
	 *  Check to see that pty is mode 04000 (and un-used)
	 */
	if (0 != stat(pcPty, &statBuf)) {
		syslog(LOG_INFO, "stat: %s: %m", pcPty);
		return -1;
	}
	if (major(statBuf.st_dev) != major(statProto.st_dev)) {
		syslog(LOG_ERR, "%s: wrong major device", pcPty);
		return -1;
	}
	if (SAFE_DONE != (statBuf.st_mode&SAFE_MASK) || S_IFCHR != (statBuf.st_mode&S_IFMT)) {
		syslog(LOG_DEBUG, "%s: wrong modes", pcPty);
		return -1;
	}

	chmod(pcPty, PTYMODE);
	chmod(acTty, TTYMODE);
	chown(pcPty, uidPty, gidPty);
	chown(acTty, uidTty, gidTty);

	return delog_utmp(acTty, (uid_t)statBuf.st_uid);
}

/*
 * make the client port, connect it to the server			(ksb)
 * prototyped from screen
 */
static int
MakeClientPort(pcName)
char *pcName;
{
	register int s;
	struct sockaddr_un run;

	if ((s = socket (AF_UNIX, SOCK_STREAM, 0)) == -1) {
		return -1;
	}
	run.sun_family = AF_UNIX;
	(void) strcpy(run.sun_path, pcName);
	if (connect (s, (struct sockaddr *)&run, strlen(pcName)+2) == -1) {
		return -1;
	}
	return s;
}

/*
 * get the pid of the running daemon					(ksb)
 * 0 == none, -1 == error, else pid
 */
static int
GetRPid()
{
	auto int sPtyd;
	auto char *pcPid;
	auto char acMsg[1025];

	if (-1 == (sPtyd = MakeClientPort(pcPort))) {
		return 0;
	}

	if (-1 == Reply(sPtyd, PTYD_PID, "?")) {
		fprintf(stderr, "%s: Reply: %s\n", progname, strerror(errno));
		return -1;
	}

	switch (Read(sPtyd, acMsg, 1024)) {
	case -1:
		fprintf(stderr, "%s: Read: %s\n", progname, strerror(errno));
		return -1;
	case 0:
		fprintf(stderr, "%s: EOF from ptyd\n", progname, strerror(errno));
		return -1;
	default:
		break;
	}
	if ((char *)0 == (pcPid = IsCmd(acMsg, PTYD_PID))) {
		return -1;
	}
	return atoi(pcPid);
}

/*
 * kill the running daemon						(ksb)
 * if fTell is 0, then we are killing the old daemon to live again
 * (in that case if there is no daemon running just return).
 */
int
KillMe()
{
	int iPid;

	iPid = GetRPid();
	switch (iPid) {
	case -1:
		return;
	case 0:
		printf("%s: no daemon running\n", progname);
		return;
	case 1:
		printf("%s: won't kill init!\n", progname);
		return;
	}

	if (fDebug) {
		printf("kill -TERM %d\n", iPid);
	}

	if (fExec) {
		if (-1 == kill(iPid, SIGTERM)) {
			fprintf(stderr, "%s: kill: %s\n", progname, strerror(errno));
		}
	}
}

/*
 * ask the daemon for status						(ksb)
 */
Status()
{
	auto int sPtyd;
	auto char acMsg[1025];

	if (-1 == (sPtyd = MakeClientPort(pcPort))) {
		printf("%s: no daemon running\n", progname);
		return;
	}

	if (-1 == Reply(sPtyd, PTYD_HELLO, "?")) {
		fprintf(stderr, "%s: Reply: %s\n", progname, strerror(errno));
		exit(1);
	}

	switch (Read(sPtyd, acMsg, 1024)) {
	case -1:
		fprintf(stderr, "%s: Read: %s\n", progname, strerror(errno));
		exit(1);
	case 0:
		fprintf(stderr, "%s: EOF from ptyd\n", progname, strerror(errno));
		exit(1);
	default:
		break;
	}
	printf("%s\n", acMsg);
}

/*
 * ask the daemon to clear some ptys					(ksb)
 */
Clean()
{
	auto int sPtyd;
	auto char acMsg[1025];

	if (-1 == (sPtyd = MakeClientPort(pcPort))) {
		printf("%s: no daemon running\n", progname);
		return;
	}

	if (-1 == Reply(sPtyd, PTYD_CLEAN, "up")) {
		fprintf(stderr, "%s: Reply: %s\n", progname, strerror(errno));
		exit(1);
	}

	switch (Read(sPtyd, acMsg, 1024)) {
	case -1:
		fprintf(stderr, "%s: Read: %s\n", progname, strerror(errno));
		exit(1);
	case 0:
		fprintf(stderr, "%s: EOF from ptyd\n", progname, strerror(errno));
		exit(1);
	default:
		break;
	}
	printf("%s\n", acMsg);
}


/*
 * tell me if the string is all digits					(ksb)
 */
int
AllDigits(pc)
char *pc;
{
	for (/* nothing */; '\000' != *pc; ++pc) {
		if (!isdigit(*pc))
			return 0;
	}
	return 1;
}

/*
 * drop our work and exit like a good guy				(ksb)
 * killing our children as we go					(bj)
 */
static SIGRET_T
DropIt()
{
	syslog(LOG_INFO, "exiting on a SIGTERM");

	if (0 != iChildren) {
		register int i;

		signal(SIGCHLD, SIG_IGN);
		for (i = 0; i < iChildren; ++i) {
			kill(piChildPids[i], SIGTERM);
		}
	}
	
	exit(0);
}

/* maybe fork off a child to do the work.  if it returns 0, the		(bj)
 * caller needs to do the work.  if it returns 1, a child has been
 * forked to handle it.
 */
static int
Split()
{
	register int pid;

	if (fChild || iMaxForks <= iChildren + 1 /* me */) {
		return 0; /* 0 == do the work */
	}
	if ((int *)0 == piChildPids && iMaxForks > 0) {
		piChildPids = (int *) calloc(iMaxForks, sizeof(int));
	}
	for (;;) {
		switch (pid = fork()) {
		case -1:
			if (EINTR == errno)
				continue;
			break;
		case 0:
			/* don't give children the power to fork */
			fChild = 1;
			iMaxForks = 0;
			break;
		default:
			piChildPids[iChildren++] = pid;
			return 1;  /* 1 == a child is doing it */
			/* NOTREACHED */
		}
		break;
	}
	return 0;  /* 0 == do the work */
}

/*
 * longjmp back to the main loop for sigpipe				(bj)
 */
static SIGRET_T
Piped()
{
	longjmp(JBMain, 1);
}

/*
 * we've been idle a while so we'll quit				(bj)
 */
static SIGRET_T
Idle()
{
	exit(0);
}

/*
 * reap a childs exit code						(ksb)
 * clean up exiting kids						(bj)
 */
static SIGRET_T
Reaper()
{
	int serrno;
	register int pid;
	auto union wait UWbuf;

	serrno = errno;
	while (-1 != (pid = wait3(& UWbuf, WNOHANG, (struct rusage *)0))) {
		if (0 == pid) {
			break;
		}
		/* stopped child is just continuted  (eek!  -- bj)
		 */
		if (WIFSTOPPED(UWbuf) && 0 == kill(pid, SIGCONT)) {
			continue;
		}
		if (WIFEXITED(UWbuf)) {
			register int i;

			for (i = 0; i < iChildren; ++i) {
				if (pid == piChildPids[i])
					break;
			}
			if (i == iChildren)
				continue;
			--iChildren;
			for (; i < iChildren; ++i) {
				piChildPids[i] = piChildPids[i + 1];
			}
		}
	}
	errno = serrno;
}


/* what to do on an unknonw signal					(ksb)
 */
static SIGRET_T
on_intr()
{
	exit(1);
}


/*
 * ptyd mainline						    (ksb&mtr)
 *  set up the server port and wait for a client...
 */
/*ARGSUSED*/
int
Server()
{
	register int sMsg, fd;
	register char *pcArg;
	auto int iOldPid;
	auto struct passwd *pwd;
	auto struct group *grp;
	auto char acMsg[100+MAXPATHLEN+1];
	auto char acMMaster[MAXPATHLEN+1];
	auto char acMSlave[MAXPATHLEN+1];
	auto struct fd_set fsHold;

	if (!fDebug && 0 != geteuid()) {
		fprintf(stderr, "%s: we must be the superuser\n", progname);
		exit(1);
	}

	(void) umask(000);

	/* kill the old daemon
	 */
	iOldPid = GetRPid();
	if (iOldPid > 2) {
		kill(iOldPid, SIGTERM);
	}

	/* go into a daemon-like mode
	 */
	FD_ZERO(& fsHold);
	if (fInetd) {
		FD_SET(0, & fsHold);
	}
	if (fDebug) {
		FD_SET(1, & fsHold);
		FD_SET(2, & fsHold);
	}
	Daemon(on_intr, & fsHold, !fDebug);

	if (-1 == stat(pcProto, & statProto)) {
		fprintf(stderr, "%s: stat: %s: %s\n", progname, pcProto, strerror(errno));
		exit(2);
	}

	/* lookup our owners and groups
	 */
	(void) setpwent();
	if ((struct passwd *)0 == (pwd = getpwnam(PTYOWNER))) {
		fprintf(stderr, "%s: getpwname: %s: %s\n", progname, PTYOWNER, strerror(errno));
		exit(3);
	}
	uidPty = pwd->pw_uid;
	if ((struct passwd *)0 == (pwd = getpwnam(TTYOWNER))) {
		fprintf(stderr, "%s: getpwname: %s: %s\n", progname, TTYOWNER, strerror(errno));
		exit(3);
	}
	uidTty = pwd->pw_uid;

	(void) setgrent();

	if ((struct group *)0 == (grp = getgrnam(PTYGROUP))) {
		fprintf(stderr, "%s: getgrname: %s: %s\n", progname, PTYGROUP, strerror(errno));
		exit(4);
	}
	gidPty = grp->gr_gid;

	if ((struct group *)0 == (grp = getgrnam(TTYGROUP))) {
		fprintf(stderr, "%s: getgrname: %s: %s\n", progname, PTYGROUP, strerror(errno));
		exit(4);
	}
	gidTty = grp->gr_gid;

	if (fInetd) {
		sCtl = 0;
	} else if (-1 == (sCtl = MakeServerPort(pcPort))) {
		exit(2);
	}

	if (fDebug) {
		iLogPri = LOG_INFO;
	}
	openlog("ptyd", LOG_PID, LOG_DAEMON);

	signal(SIGTERM, DropIt);
	signal(SIGCHLD, Reaper);
	signal(SIGALRM, Idle);
	signal(SIGPIPE, Piped);

	if (2 < iOldPid) {
		syslog(iLogPri, "replacing pid %d", iOldPid);
	}

	fRunning = 1;
	while (fRunning) {
		/* the daemon waits for a client, maybe forks a
		 * child to do it, or handles the request.
		 */
		/* quit in 5 minutes if there's no activity */
		if (fChild) {
			alarm(300);
		}
		if (-1 == (sMsg = accept(sCtl, (struct sockaddr *)0, (int *)0))) {
			if (EINTR == errno)
				continue;
			syslog(LOG_INFO, "accept: %m");
			sleep(2);
			continue;
		}
		alarm(0);  /* don't interrupt a transaction */
		/* panic point -- come back here if something goes
		 * horribly wrong, eg SIGPIPE
		 */
		if (0 != setjmp(JBMain)) {
			close(sMsg);
			continue;
		}
		if (Split()) {
			/* a child will take care of this
			 */
			close(sMsg);
			continue;
		}

		/* process one request
		 */
		while (0 < Read(sMsg, acMsg, sizeof(acMsg))) {
			if ((char *)0 != (pcArg = IsCmd(acMsg, PTYD_OPEN))) {
				if (AllDigits(pcArg)) {
					pwd = getpwuid(atoi(pcArg));
				} else {
					pwd = getpwnam(pcArg);
				}
				if ((struct passwd *)0 == pwd) {
					Reply(sMsg, PTYD_USER, pcArg);
					syslog(LOG_INFO, "getpw*: %s: unknown user", pcArg);
				} else if (-1 == (fd = GetPty(acMSlave, acMMaster, pwd->pw_uid))) {
					Reply(sMsg, PTYD_SYSERR, "getpty");
					syslog(LOG_ERR, "getpty: out of ptys");
				} else {
					ioctl(fd, TIOCNOTTY, 0);
					close(fd);
					Reply(sMsg, PTYD_TAKE, acMMaster);
				}
			} else if ((char *)0 != (pcArg = IsCmd(acMsg, PTYD_CLOSE))) {
				if (0 == CleanPty(pcArg))
					Reply(sMsg, PTYD_OK, pcArg);
				else
					Reply(sMsg, PTYD_NO, pcArg);
			} else if ((char *)0 != (pcArg = IsCmd(acMsg, PTYD_CLEAN))) {
				Reply(sMsg, PTYD_OK, DoScan());
			} else if ((char *)0 != (pcArg = IsCmd(acMsg, PTYD_PID))) {
				sprintf(acMsg, "%d", fChild ? getppid() : getpid());
				Reply(sMsg, PTYD_PID, acMsg);
			} else if ((char *)0 != (pcArg = IsCmd(acMsg, PTYD_MKUTMP))) {
				auto char *pcMach;
				if ((char *)0 != (pcMach = strchr(pcArg, '@'))) {
					*pcMach++ = '\000';
				} else {
					pcMach = pcDefMach;
				}
				if (0 == MkUtmp(pcArg, pcMach))
					Reply(sMsg, PTYD_OK, "done");
				else
					Reply(sMsg, PTYD_NO, pcArg);
			} else if ((char *)0 != (pcArg = IsCmd(acMsg, PTYD_RMUTMP))) {
				if (0 == RmUtmp(pcArg))
					Reply(sMsg, PTYD_OK, "done");
				else
					Reply(sMsg, PTYD_NO, pcArg);
			} else if ((char *)0 != (pcArg = IsCmd(acMsg, PTYD_LOGIN))) {
				if (0 == Login(pcArg))
					Reply(sMsg, PTYD_OK, "done");
				else
					Reply(sMsg, PTYD_NO, pcArg);
			} else if ((char *)0 != (pcArg = IsCmd(acMsg, PTYD_LOGOUT))) {
				if (0 == Logout(pcArg))
					Reply(sMsg, PTYD_OK, "done");
				else
					Reply(sMsg, PTYD_NO, pcArg);
			} else if ((char *)0 != (pcArg = IsCmd(acMsg, PTYD_HELLO))) {
				auto char acName[256];
				gethostname(acName, 256);
				Reply(sMsg, PTYD_IDENT, acName);
			} else {
				Reply(sMsg, PTYD_UNKNOWN, acMsg);
				syslog(LOG_NOTICE, "unknown command `%s\'", acMsg);
			}
		}

		if (-1 == close(sMsg)) {
			syslog(LOG_INFO, "close: %m");
		}
	}

	(void) close(sCtl);
	(void) closelog();

	exit(0);
}
