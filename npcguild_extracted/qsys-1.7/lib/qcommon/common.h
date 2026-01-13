/* $Id: common.h,v 1.7 1997/01/05 22:43:43 kb207252 Exp $
 * common definitions for the simple command queue system		(ksb)
 */
#if !defined(HPUX)
#if defined(HPUX7) || defined(HPUX8) || defined(HPUX9)
#define HPUX	1
#endif
#endif

#if !defined(PATH_RSH)
#if defined(IRIX4) || defined(IRIX5)
#define PATH_RSH	"/usr/bsd/rsh"
#else
#if HPUX
#define PATH_RSH	"/usr/bin/remsh"
#else
#if defined(NETBSD) || defined(FREEBSD)
#define PATH_RSH	"/usr/bin/rsh"
#else
#define PATH_RSH	"/usr/ucb/rsh"
#endif
#endif
#endif
#endif /* find rsh */

#if !defined(MAXPATHLEN)
#define MAXPATHLEN	1024
#endif

#if !defined(MAXJOBS)
#define MAXJOBS		16
#endif

#if !defined(DEF_POLL)
#define DEF_POLL	120
#endif

#if !defined(MIN_POLL)
#define MIN_POLL	 30
#endif

#if !defined(HAVE_UCB)
#define HAVE_UCB	(defined(NETBSD) || defined(S81) || defined(SUN4))
#endif

#if !defined(HAVE_STRERROR)
#define HAVE_STRERROR	(defined(NETBSD) || defined(SUN5) || defined(FREEBSD))
#endif

#if !defined(HAS_NDIR)
#define HAS_NDIR        (defined(ETA10)||defined(V386))
#endif

#if !defined(HAS_DIRENT)
#define HAS_DIRENT      (defined(SUN5) || defined(NETBSD) || defined(FREEBSD))
#endif

#if !defined(DIRENT)
#if defined(IBMR2) || defined(SUN5) || defined(NETBSD) || defined(FREEBSD)
#define DIRENT          dirent
#else
#define DIRENT          direct
#endif
#endif  /* need to scandir */

#if !defined(USE_SETREUID)
#define USE_SETREUID	(defined(NETBSD) || defined(FREEBSD))
#endif

#if !defined(USE_FLOCK)
#define USE_FLOCK	(defined(SUN4) || defined(SUN3) || defined(NETBSD) || defined(FREEBSD))
#endif

#if !defined(USE_LOCKF)
#define USE_LOCKF	(!USE_FLOCK)
#endif

#if !defined(USE_GETCWD)
#define USE_GETCWD	(defined(SYSV) || defined(HPUX) || defined(IBMR2))
#endif
#if !defined(USE_p_pptr)
#define USE_p_pptr	(defined(NETBSD) || defined(FREEBSD))
#endif

#if !defined(USE_pcred)
#define	USE_pcred	(defined(NETBSD) || defined(FREEBSD))
#endif
