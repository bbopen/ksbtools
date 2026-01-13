/*
 * $Id: machine.h,v 1.5 1997/11/29 17:58:50 ksb Exp $
 *
 * tell us about the host machine, site specific defaults
 */

#if !defined(PTY_PREFIX)
#define PTY_PREFIX	"/dev/pty"
#endif

#if !defined(LOGGING)
#define LOGGING		0
#endif

#if !defined(LASLLOG)
#define LASLLOG		1
#endif

#if LASTLOG
#include <lastlog.h>
#endif

#if !defined(LASTLOG_FILENAME)
#if defined(_PATH_LASTLOG)
#define LASTLOG_FILENAME _PATH_LASTLOG
#else
#if defined(NETBSD) || defined(FREEBSD)
#define LASTLOG_FILENAME "/var/log/lastlog"  
#else
#define LASTLOG_FILENAME "/usr/adm/lastlog"  
#endif
#endif
#endif



#if !defined(UTMP)
#define UTMP		1
#endif

#if !defined(UTMP_FILE)
#if defined(_PATH_UTMP)
#define UTMP_FILE	_PATH_UTMP
#else
#if defined(NETBSD)||defined(FREEBSD)
#define UTMP_FILE	"/var/run/utmp"
#else
#define UTMP_FILE	"/etc/utmp"
#endif
#endif
#endif

#if !defined(WTMP_FILE)
#if defined(_PATH_WTMP)
#define WTMP_FILE	_PATH_WTMP
#else
#if defined(NETBSD)||defined(FREEBSD)
#define WTMP_FILE	"/var/run/wtmp"
#else
#define WTMP_FILE	"/etc/wtmp"
#endif
#endif
#endif

#if !defined(HAVE_UTIL_LOGIN)
/* NetBSD has a routine in -lutil (or someplace today :-)
 * which does the utmp/wtmp thing for us, if so use it
 */
#define HAVE_UTIL_LOGIN	(defined(NETBSD))
#endif

/* which type signal handlers return on this machine
 */
#if !defined(SIGRET_T)
#if defined(sun) || defined(NEXT2) || defined(SUN5) || defined(PTX) || defined(IRIX5)||defined(IRIX6) || defined(IBMR2) || defined(FREEBSD) || defined(ULTRIX) || defined(LINUX) || defined(BSDI)
#define SIGRET_T	void
#else
#define SIGRET_T	int
#endif
#endif

#if defined(LINUX) && !defined(POSIX)
#define POSIX	1
#define NEED_GNU_TYPES	1
#endif

#if !defined(NEED_UNISTD_H)
#define NEED_UNISTD_H	(!defined(HPUX7))
#endif

#ifndef HAVE_STRERROR
#define HAVE_STRERROR	(defined(NEXT2)||defined(IBMR2)||defined(HPUX7)||defined(HPUX8)||defined(SUN5)||defined(NETBSD)||defined(FREEBSD)||defined(IRIX5))
#endif

extern int errno;

#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif
