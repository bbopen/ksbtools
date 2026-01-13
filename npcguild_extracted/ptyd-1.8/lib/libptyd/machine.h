/* $Id: machine.h,v 1.1 1997/11/29 17:21:59 ksb Exp $
 * standard machine lever for msrc tools -- ksb
 */
#if !defined(HPUX) && (defined(HPUX7)||defined(HPUX8)||defined(HPUX9)||defined(HPUX10))
#define HPUX	1
#endif

#if !defined(IRIX) && (defined(IRIX4) || defined(IRIX5) || defined(IRIX6))
#define IRIX	1
#endif

#if !defined(ULTRIX) && (defined(ULTRIX4) || defined(ULTRIX5))
#define ULTRIX	1
#endif

#if !defined(HAVE_STRERROR)
#define HAVE_STRERROR	(defined(NEXT2)||defined(IBMR2)||defined(HPUX7)||defined(HPUX8)||defined(HPUX9)||defined(SUN5)||defined(NETBSD)||defined(IRIX5)||defined(IRIX6)||defined(FREEBSD))
#endif

#if !defined(NEED_IOCTL_COMPAT_H)
#define NEED_IOCTL_COMPAT_H	(defined(NETBSD)||defined(FREEBSD))
#endif

#if NEED_IOCTL_COMPAT_H
#include <sys/ioctl_compat.h>
#endif
#include <sys/ioctl.h>

extern int errno;
#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif
