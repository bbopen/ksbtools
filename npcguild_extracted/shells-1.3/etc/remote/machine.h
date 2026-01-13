/* leverage file for remote (the terminal login shell)			(ksb)
 * $Id: machine.h,v 1.10 1997/11/27 16:46:22 ksb Exp $
 */

#if !defined(HPUX) && (defined(HPUX7)||defined(HPUX8)||defined(HPUX9)||defined(HPUX10))
#define HPUX	1
#endif

#if !defined(IRIX) && (defined(IRIX5)||defined(IRIX5)||defined(IRIX6))
#define IRIX	1
#endif

#if !defined(USE_PUTENV)
#if SUN3 || SUN4 || SUN5 || HPUX9 || IRIX5 || IRIX6 || LINUX || BSDI || FREEBSD
#define USE_PUTENV	1
#else
#define USE_PUTENV	0
#endif
#endif

#if !defined(USE_TERMIOS)
#define USE_TERMIOS	(defined(FREEBSD)||defined(BSDI))
#endif

#if !defined(USE_TERMIO)
#define USE_TERMIO	(defined(SUN5)||defined(HPUX9)||defined(IRIX5)||defined(IRIX6)||defined(LINUX))
#endif

#if USE_TERMIOS
#if !defined(USE_TC_CALLS)
#define USE_TC_CALLS	(defined(FREEBSD)||defined(BSDI))
#endif
#endif

#ifndef HAVE_STRERROR
#define HAVE_STRERROR	(defined(NEXT2)||defined(IBMR2)||defined(HPUX)||defined(SUN5)||defined(NETBSD)||defined(FREEBSD)||defined(IRIX5)||defined(IRIX6)||defined(LINUX)||defined(BSDI))
#endif

extern int errno;

#if !defined(USE_STRINGS)
#define USE_STRINGS		(defined(HPUX8))
#endif

#if !defined(SIGRET_T)
#if SUN5 || HPUX || FREEBSD || LINUX || BSDI
#define SIGRET_T        void
#else
#define SIGRET_T        int
#endif
#endif

#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif

/*
 * REGCMP is 1 if your system has the regcmp() function.
 * This is normally the case for System 5.
 * RECOMP is 1 if your system has the re_comp() function.
 * This is normally the case for BSD.
 * REGCOMP is 1 if this is a XENIX(?) system.
 * If neither is 1, pattern matching is supported, but without metacharacters.
 */
#if defined(_SRM) || defined(SUN5) || defined(HPUX9)
# define REGCMP		1
#else
# define REGCMP		0
# define RECOMP		(!defined(FREEBSD) && !defined(NETBSD) && !defined(BSDI))
# define REGEX		(defined(FREEBSD)||defined(BSDI))
# define REGCOMP	(defined(NETBSD))
#endif

#if REGCMP && SUN5
#define USE_LIBGEN	1
#endif

#if !defined(PATH_GENERIC_X)
#if defined(IRIX5)||defined(IRIX6)
#define PATH_GENERIC_X	"/usr/bin/X11/X"
#else
#if defined(SUN5)
#define PATH_GENERIC_X	"/usr/openwin/bin/X"
#else
#if defined(FREEBSD) || defined(BSDI) || defined(NETBSD)
#define PATH_GENERIC_X	"/usr/X11R6/bin/X"
#else
#define PATH_GENERIC_X	"/usr/local/X11/Xsun"
#endif
#endif
#endif
#endif

#if !defined(PATH_MONO_X)
#if defined(SUN5)
#define PATH_MONO_X	"/usr/openwin/bin/X"
#else
#if defined(FREEBSD)||defined(BSDI)||defined(NETBSD)
#define PATH_MONO_X	"/usr/X11R6/bin/X"
#else
#define PATH_MONO_X	"/usr/local/X11/XsunMono"
#endif
#endif
#endif

#if !defined(PATH_INDIR_X)
#if defined(SUN5)
#define PATH_INDIR_X	"/usr/openwin/bin/X"
#else
#if defined(FREEBSD)||defined(BSDI)||defined(NETBSD)
#define PATH_INDIR_X	"/usr/X11R6/bin/X"
#else
#define PATH_INDIR_X	"/usr/local/X11/X"
#endif
#endif
#endif

#if !defined(PATH_KBD_MODE)
#if defined(SUN5)
#define PATH_KBD_MODE	"/usr/openwin/bin/kbd_mode"
#else
#if defined(FREEBSD)||defined(BSDI)||defined(NETBSD)
#define PATH_KBD_MODE	"/usr/X11R6/bin/kbd_mode"
#else
#define PATH_KBD_MODE	"/usr/local/X11/kbd_mode"
#endif
#endif
#endif

#if !defined(DEF_OPENWINHOME)
#if defined(SUN5)
#define DEF_OPENWINHOME	"/usr/openwin"
#endif
#endif

#if !defined(PATH_TN3270)
#if defined(IRIX5)||defined(IRIX6)
#define	PATH_TN3270	"/usr/sbin/tn3270"
#else
#if defined(FREEBSD)||defined(BSDI)||defined(NETBSD)
#define	PATH_TN3270	"/usr/bin/tn3270"
#else
#define	PATH_TN3270	"/usr/local/bin/tn3270"
#endif
#endif
#endif

#if !defined(PATH_RLOGIN)
#if defined(IRIX5)||defined(IRIX6)
#define PATH_RLOGIN	"/usr/bsd/rlogin"
#else
#if defined(SUN5) || defined(FREEBSD) || defined(BSDI)
#define PATH_RLOGIN	"/usr/bin/rlogin"
#else
#define PATH_RLOGIN	"/usr/ucb/rlogin"
#endif
#endif
#endif

#if !defined(NEED_XDMCP)
#define NEED_XDMCP	(defined(SUN5)||defined(SUN4)||defined(FREEBSD))
#endif

#if !defined(NEED_MALLOC_H)
#define NEED_MALLOC_H	(defined(IRIX5)||defined(IRIX6)||defined(LINUX))
#endif

#if NEED_MALLOC_H
#include <malloc.h>
#endif

#if !defined(NEED_STDLIB_H)
#define NEED_STDLIB_H	(defined(IBMR2)||defined(PTX)||defined(FREEBSD)||defined(ULTRIX)||defined(BSDI))
#endif
