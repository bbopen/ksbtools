/* $Id: machine.h,v 3.16 1998/02/12 20:22:03 ksb Exp $
 */
#if !defined(HPUX) && (defined(HPUX7)||defined(HPUX8)||defined(HPUX9)||defined(HPUX10))
#define HPUX	1
#endif

#if defined(LINUX) && !defined(POSIX)
#define POSIX
#endif

#if !defined(NEED_GNU_TYPES)
#define NEED_GNU_TYPES defined(LINUX)
#endif

#if !defined(HAVE_STRERROR)
#define HAVE_STRERROR	(defined(NEXT2)||defined(IBMR2)||defined(HPUX)||defined(SUN5)||defined(NETBSD)||defined(IRIX5)||defined(IRIX6)||defined(FREEBSD)||defined(LINUX))
#endif

extern int errno;

#if !HAVE_STRERROR
extern char *sys_errlist[];
# define strerror(Me) (sys_errlist[Me])
#endif

#if !defined(HAVE_H_ERRLIST)
#define HAVE_H_ERRLIST  (defined(SUN4)||defined(SUN3)||defined(NETBSD)||defined(IRIX5)||defined(IRIX6))
#endif

#if HAVE_H_ERRLIST
extern int h_errno;
extern char *h_errlist[];
#define hstrerror(Me)   (h_errlist[Me])
#else
#define hstrerror(Me)   "host lookup error"
#endif

#if !defined(USE_STRINGS)
#define USE_STRINGS	(defined(bsd))
#endif

#if !defined(HAVE_WINSIZE)
#define HAVE_WINSIZE	(!defined(S81) && defined(SIGWINCH) && defined(TIOCGWINSZ))
#endif

#if !defined(HAVE_SELECT_H)
#define HAVE_SELECT_H	(defined(IBMR2))
#endif

#if !defined(USE_NONBLOCK_RECV)
#define USE_NONBLOCK_RECV	(! (defined(HPUX)))
#endif

#if !defined(USE_TERMIOS)
#define USE_TERMIOS	(!defined(NEXT2))
#endif

/* this must agree with labd and labwatch
 */
#if !defined(XMIT_SCALE)
#define XMIT_SCALE	4096
#endif

#if !defined(SIGRET_T)
#if S81 || VAX8800
#define SIGRET_T	int
#else
#define SIGRET_T	void
#endif
#endif

#if !defined(HAVE_STICKY_SIGNALS)
#define HAVE_STICKY_SIGNALS	(!(defined(HPUX)||defined(SUN5)))
#endif
