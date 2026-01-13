/* $Id: machine.h,v 5.7 2008/10/03 15:58:25 ksb Exp $
 */
#if defined(HPUX7)||defined(HPUX8)||defined(HPUX9)||defined(HPUX10)||defined(HPUX11)
#if !defined(HPUX)
#define HPUX 1
#endif
#endif

#if defined(SYSV)
#define getwd(Mb)	getcwd(Mb, MAXPATHLEN)
extern char *getcwd();
#else
extern char *getwd();
#endif

/* The lines below control bsd style (struct rusage) wait3 utilization.
 * Zero (0) implies not code for resource limits,
 * one (1) implies produce code for limitting resources.
 */
#if defined(bsd)||defined(dynix)||defined(AIX)
#define	RESOURCE	!defined(HPUX7)
#else
#define RESOURCE	0
#endif

#if defined(bsd)
#define strchr	index
#define strrchr	rindex
#endif /* under bsd these names are wrong */

#if !defined(HPUX) && (defined(HPUX7)||defined(HPUX8)||defined(HPUX9))
#define HPUX	1
#endif

#if !defined(HAVE_STRERROR)
#define HAVE_STRERROR	(defined(NEXT2)||defined(IBMR2)||defined(HPUX)||defined(SUN5)||defined(NETBSD)||defined(IRIX5)||defined(IRIX6)||defined(FREEBSD)||defined(LINUX)||defined(OPENBSD))
#endif

#if !defined(USE_MALLOC_H)
#define USE_MALLOC_H (!(defined(HPUX9)||defined(HPUX10)))
#endif

#if !defined(USE_STDLIB)
#define USE_STDLIB	1
#endif

#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif

#include "match.h"

#if !defined(EMU_SETENV)
#define EMU_SETENV	!(defined(HPUX7)||defined(FREEBSD)||defined(OPENBSD)||defined(IBMR2))
#endif

#if EMU_SETNV
#include "setenv.h"
#endif

/* configure the mixer to use (...) rather than <...>
 */
#undef MIXER_RECURSE
#undef MIXER_END
#define MIXER_RECURSE	'('
#define MIXER_END	')'
