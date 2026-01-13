/* $Id: machine.h,v 2.1 1998/02/12 20:44:58 ksb Exp $
 * machine leverage for the flock program (set the way back machine here)
 */

#if !defined(HAVE_STRERROR)
#define HAVE_STRERROR	(defined(NEXT2)||defined(IBMR2)||defined(HPUX7)||defined(HPUX8)||defined(SUN5)||defined(NETBSD)||defined(IRIX5)||defined(FREEBSD)||defined(LINUX))
#endif

#if defined(bsd)
#define strchr	index
#define strrchr	rindex
#endif

#if !defined(HAVE_UNION_WAIT)
#define	HAVE_UNION_WAIT	(defined(SUN3)||defined(SUN4)||(!defined(SUN5)&&!defined(PARAGON)&&defined(bsd)))
#endif

#if !defined(WAIT_t)
#if HAVE_UNION_WAIT
#define WAIT_t	union wait
#else
#define WAIT_t	int
#endif
#endif

extern int errno;

#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif
