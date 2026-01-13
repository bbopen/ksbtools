/* $Id: logutmp.c,v 1.9 1994/02/04 10:32:01 ksb Exp $
 *	This logs and un-logs (;-) utmp entries
 *
 *	Copyright (C) 1988 Michael Rowan
 *
 * 	Everyone is granted permision to copy, modify and
 *	redistribute this code provided that 1) you leave this
 *	copyright message and 2) you send changes/fixes back to
 *	me so I can include them in the next release.
 *
 *	Mike Rowan (in care of ksb@cc.purdue.edu)
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/syslog.h>
#include <utmp.h>
#include <stdio.h>
#include <ttyent.h>
#include <pwd.h>

#include "machine.h"

extern time_t time();
extern char *strrchr(), *strncpy();

static char acWtmp[] = WTMP_FILE;
static char acUtmp[] = UTMP_FILE;


/*
 * find a tty slot							(mtr)
 * only takes last path component
 */
static int
myttyslot(pchT)
	char	*pchT;
{
	register struct ttyent *ty;
	register int iCnt;

	(void) setttyent();
	iCnt = 0;
	while ((ty = getttyent()) != NULL) {
		iCnt++;
		if (strcmp(ty->ty_name, pchT) == 0) {
			endttyent();
			return iCnt;
		}
	}
	(void) endttyent();
	return -1;
}

extern off_t lseek();

/*
 * build a utmp entry for the uid					(mtr)
 */
int
log_utmp(ttyn, uid, pcMach)
char *ttyn, *pcMach;
uid_t uid;
{
	register int iSlot;
	register char *pcTty;
	int fdUtmp;
	struct utmp utmp;
	struct passwd *pw;

	if ((char *)0 == (pcTty = strrchr(ttyn+1, '/'))) {
		pcTty = ttyn;
	} else {
		++pcTty;
	}
	if (0 > (iSlot = myttyslot(pcTty))) {
		syslog(LOG_NOTICE, "%s: no ttyslot", pcTty);
		return -1;
	}

	(void) strncpy(utmp.ut_host, pcMach, sizeof(utmp.ut_host));
	utmp.ut_host[sizeof(utmp.ut_host)-1] = '\000';

	pw = getpwuid((int) uid);
	(void) strncpy(utmp.ut_name, pw->pw_name, sizeof(utmp.ut_name));
	(void) time(&utmp.ut_time);

#if HAVE_UTIL_LOGIN
	(void) strncpy(utmp.ut_line, pcTty, sizeof(utmp.ut_line));
	login(&utmp);
#else
	if (NULL == (fdUtmp = open(acUtmp, O_RDWR, 0))) {
		syslog(LOG_INFO, "open: utmp: %m");
	} else if (-1 == flock(fdUtmp, LOCK_EX)) {
		syslog(LOG_INFO, "flock: %m");
	} else if (-1 == lseek(fdUtmp, (off_t)(iSlot*sizeof(utmp)), 0)) {
		syslog(LOG_INFO, "lseek: %m");
	} else {
		(void) strncpy(utmp.ut_line, pcTty, sizeof(utmp.ut_line));
		(void) write(fdUtmp, (char *)&utmp, sizeof(utmp));
		(void) flock(fdUtmp, LOCK_UN);
		(void) close(fdUtmp);
	}
#endif
	return 0;
}


/*
 *  Delogging isn't as important as logging - once the pty is close
 *	someone else will get it and thier utmp entry will overwrite
 *	ours.  [[hehehe -- not always. So we do it anyway. (ksb)]]
 */
int
delog_utmp(ttyn, uid)
	char	*ttyn;
	uid_t	uid;
{
#if HAVE_UTIL_LOGIN
	logout(ttyn);
#else
	register int 	iSlot;
	int fdUtmp;
	struct	utmp 	utmp;
	char *pcTty;

	if ((char *)0 == (pcTty = strrchr(ttyn+1, '/'))) {
		pcTty = ttyn;
	} else {
		++pcTty;
	}
	if (0 > (iSlot = myttyslot(pcTty))) {
		syslog(LOG_NOTICE, "%s: no ttyslot", pcTty);
		return -1;
	}

	if (NULL == (fdUtmp = open(acUtmp, O_RDWR))) {
		syslog(LOG_INFO, "open: utmp: %m");
	}
	if (-1 == flock(fdUtmp, LOCK_EX)) {
		syslog(LOG_INFO, "flock: %m");
	} else if (-1 == lseek(fdUtmp, (off_t)(iSlot*sizeof(utmp)), 0)) {
		syslog(LOG_INFO, "lseek: %m");
	} else {
		(void) read(fdUtmp, (char *)&utmp, sizeof(utmp));
		(void) lseek(fdUtmp, (off_t)(iSlot*sizeof(utmp)), 0);
		utmp.ut_name[0] = 0;
		utmp.ut_host[0] = 0;
		(void) write(fdUtmp, (char *)&utmp, sizeof(utmp));
		(void) flock(fdUtmp, LOCK_UN);
	}
	(void) close(fdUtmp);
#endif
	return 0;
}


/*
 * find an active utmp entry, make it a close wtmp entry an append to	(ksb)
 * wtmp
 */
int
log_wtmp(ttyn, uid)
char *ttyn;
uid_t uid;
{
#if ! HAVE_UTIL_LOGIN
	return 0;
#else
	register int 	iSlot;
#if LASTLOG
	static char pcLastLogFile[] = LASTLOG_FILENAME;
	auto struct lastlog LLlog;
	int fdLast;
#endif 
	struct	utmp 	utmp;
	struct  passwd	*pw;
	char *pcTty;
	int fdUtmp, fdWtmp;

	if ((char *)0 == (pcTty = strrchr(ttyn+1, '/'))) {
		pcTty = ttyn;
	} else {
		++pcTty;
	}
	if (0 > (iSlot = myttyslot(pcTty))) {
		syslog(LOG_NOTICE, "%s: no ttyslot", pcTty);
		return -1;
	}

	pw = getpwuid((int) uid);

	if (NULL == (fdUtmp = open(acUtmp, O_RDONLY, 0))) {
		syslog(LOG_INFO, "open: utmp: %m");
	} else if (-1 == flock(fdUtmp, LOCK_EX)) {
		syslog(LOG_INFO, "flock: %m");
	} else if (-1 == lseek(fdUtmp, (off_t)(iSlot*sizeof(utmp)), 0)) {
		syslog(LOG_INFO, "lseek: %m");
	} else {
		read(fdUtmp, (char *)&utmp, sizeof(utmp));
		if (0 == strncmp(pw->pw_name, utmp.ut_name, strlen(pw->pw_name))) {
			if (-1 != (fdWtmp = open(acWtmp, O_APPEND|O_WRONLY, 0))) {
				(void) write(fdWtmp, (char *)&utmp, sizeof(utmp));
				close(fdWtmp);
			}
#if LASTLOG
			bzero((char *) &LLlog, sizeof(struct lastlog));
			(void) strncpy(LLlog.ll_line, pcTty, sizeof(LLlog.ll_line));
			(void) strncpy(LLlog.ll_host, utmp.ut_host, sizeof(LLlog.ll_host));
			if (0 > (fdLast = open(pcLastLogFile, O_WRONLY, 0))) {
				(void) strncpy(LLlog.ll_host, pcHost, sizeof(LLlog.ll_host));
				(void) time(&LLlog.ll_time);
				if (-1 != lseek(fdLast, ((long)pw->pw_uid * sizeof(struct lastlog)), 0))
					(void)write(fdLast, (char *)&LLlog, sizeof (struct lastlog));
				close(fdLast);
			}
#endif 
		}
		(void) flock(fdUtmp, LOCK_UN);
		(void) close(fdUtmp);
	}
	return 0;
#endif
}


/*
 * find an active utmp entry, make it a close wtmp entry an append to	(ksb)
 * wtmp
 */
int
delog_wtmp(ttyn, uid)
char *ttyn;
uid_t uid;
{
#if !HAVE_UTIL_LOGIN
	register int 	iSlot;
	struct	utmp 	utmp;
	struct  passwd	*pw;
	char *pcTty;
	int fdUtmp, fdWtmp;

	if ((char *)0 == (pcTty = strrchr(ttyn+1, '/'))) {
		pcTty = ttyn;
	} else {
		++pcTty;
	}
	if (0 > (iSlot = myttyslot(pcTty))) {
		syslog(LOG_NOTICE, "%s: no ttyslot", pcTty);
		return -1;
	}

	pw = getpwuid((int) uid);

	if (NULL == (fdUtmp = open(acUtmp, O_RDONLY, 0))) {
		syslog(LOG_INFO, "open: utmp: %m");
	} else if (-1 == flock(fdUtmp, LOCK_EX)) {
		syslog(LOG_INFO, "flock: %m");
	} else if (-1 == lseek(fdUtmp, (off_t)(iSlot*sizeof(utmp)), 0)) {
		syslog(LOG_INFO, "lseek: %m");
	} else {
		(void) read(fdUtmp, (char *)&utmp, sizeof(utmp));
		if (0 == strncmp(pw->pw_name, utmp.ut_name, strlen(pw->pw_name))) {
			utmp.ut_name[0] = 0;
			utmp.ut_host[0] = 0;
			(void) time(&utmp.ut_time);
			if (-1 != (fdWtmp = open(acWtmp, O_APPEND|O_WRONLY, 0))) {
				(void) write(fdWtmp, (char *)&utmp, sizeof(utmp));
				close(fdWtmp);
			}
		}
		(void) flock(fdUtmp, LOCK_UN);
		(void) close(fdUtmp);
	}
#endif
	return 0;
}
