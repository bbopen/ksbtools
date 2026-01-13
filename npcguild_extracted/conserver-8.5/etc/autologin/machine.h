/* $Id: machine.h,v 2.8 1997/11/10 17:10:34 ksb Exp $
 * leverage in liew of L7
 */

#if !defined(HPUX) && (defined(HPUX7)||defined(HPUX8)||defined(HPUX9)||defined(HPUX10))
#define HPUX	1
#endif

#if S81
#include <sys/vlimit.h>
#else
#include <limits.h>
#endif

#if !defined(NEED_STDLIB)
#define NEED_STDLIB	(defined(HPUX9)||defined(HPUX10)||defined(LINUX))
#endif

#if !defined(USE_OLD_UTENT)
#define USE_OLD_UTENT	(defined(SUN4)||defined(FREEBSD)||defined(NETBSD)||defined(S81))
#endif

#if !defined(USE_UTENT)
#define USE_UTENT	(defined(SUN5)||defined(EPIX)||defined(PARAGON)||defined(IBMR2)||defined(HPUX9)||defined(HPUX10)||defined(LINUX))
#endif

#if !defined(NEED_PUTENV)
#define NEED_PUTENV	(!(defined(IBMR2) || defined(EPIX) || defined(SUNOS) || defined(SUN5) || defined(HPUX9)||defined(HPUX10)||defined(LINUX)))
#endif

#if !defined(NEED_GETDTABLESIZE)
#define NEED_GETDTABLESIZE	(defined(SUN5) || defined(HPUX9))
#endif

/* if we do not have old style tty emulation use termios.h
 */
#if !defined(USE_TERMIO)
#define USE_TERMIO	(defined(ETA10)||defined(V386))
#endif
#if !defined(USE_TERMIOS)
#define USE_TERMIOS	(defined(HPUX)||defined(SUN5)||defined(PTX)||defined(IRIX5)||defined(LINUX))
#endif
#if !defined(USE_TCBREAK)
#define USE_TCBREAK	(defined(SUN4)||defined(PTX))
#endif

#if USE_TERMIOS
#if defined(HPUX7)||defined(HPUX8)||defined(HPUX9)
#if !defined(TCGETS)
#define TCGETS  _IOR('T', 16, struct termios)
#endif
#if !defined(TCSETS)
#define TCSETS  _IOW('T', 17, struct termios)
#endif
#endif
#if defined(PTX2)
#define TCGETS  TCGETP
#define TCSETS  TCSETP
#endif
#endif

/* if we have <strings.h> define this to 1, else define to 0
 */
#if !defined(USE_STRINGS)
#define USE_STRINGS	(defined(DYNIX)||defined(EPIX)||defined(IRIX5))
#endif

#if !defined(USE_TC)
#define USE_TC		(defined(EPIX)||defined(IBMR2)||defined(V386)||defined(S81)||defined(PARAGON))
#endif

#if !defined(HAVE_GETUSERATTR)
#define HAVE_GETUSERATTR 	(defined(IBMR2))
#endif

#if !defined(USE_IOCTL)
#define USE_IOCTL	(defined(V386)||defined(S81)||defined(NETBSD)||defined(FREEBSD))
#endif


#if !defined(HAVE_STRERROR)
#define HAVE_STRERROR	(defined(FREEBSD)||defined(NEXT2)||defined(IBMR2)||defined(HPUX)||defined(SUN5)||defined(NETBSD)||defined(IRIX5)||defined(LINUX))
#endif

/* yucky kludges because L7 doesn't even know how to do this yet
 */
#ifdef EPIX
#include "/bsd43/usr/include/ttyent.h"
#include <posix/sys/termios.h>
#define NGROUPS_MAX 8
#define getdtablesize() 64
struct timeval {
        long    tv_sec;         /* seconds */
        long    tv_usec;        /* and microseconds */
};
typedef int mode_t;
extern struct passwd *getpwnam();
extern struct group *getgrnam();
#else 

#if defined IBMR2
#include <termios.h>
#define setsid()	getpid()
#else

#if defined V386
typedef int mode_t;
#define getdtablesize()	OPEN_MAX
#define setsid()	getpid()
#define setgroups(x, y)	0
#include <sys/ttold.h>
#else

#if defined S81
typedef int mode_t;
#define setsid()	getpid()
#else

#if NETBSD
#include <sys/uio.h>
#include <sys/proc.h>
#include <sys/ioctl_compat.h>
#define setsid()	getpid()
#define UTMP_PATH	"/var/run/utmp"
#else

#if FREEBSD
#include <sys/uio.h>
#include <sys/proc.h>
#include <sys/ioctl_compat.h>
#define setsid()	getpid()
#else

#endif	/* NETBSD */
#endif	/* 386bsd or equiv */
#endif	/* sequent */
#endif	/* intel v386 */
#endif	/* find termios */
#endif	/* find any term stuff */


#ifdef SUNOS
#include <ttyent.h>
#define setsid()	getpid()
#endif

#if !defined(HAVE_GETSID)
#if HPUX
#define HAVE_GETSID	(defined(HPUX10)||defined(LINUX))
#else
#if PARAGON || SUNOS || SUN4 || SUN5 || NETBSD || S81 || V386 || IBMR2 || EPIX
#define HAVE_GETSID	1
#else
#define HAVE_GETSID	0
#endif
#endif
#endif

#if ! HAVE_GETSID
#define getsid(Mp)	(Mp)
#endif

#if ! (defined(V386) || defined(LINUX))
#include <sys/vnode.h>
#endif

#if IBMR2
#include <usersec.h>
#endif	/* IBMR2 */

#if !defined(TTYMODE)
#define	TTYMODE		0600
#endif

#ifndef O_RDWR
#define O_RDWR		2
#endif

#if !defined(NGROUPS_MAX)
#define NGROUPS_MAX	8
#endif

#if !defined(UTMP_FILE)
#if defined(_PATH_UTMP)
#define UTMP_FILE	_PATH_UTMP
#else
#define UTMP_FILE	"/etc/utmp"
#endif
#endif
