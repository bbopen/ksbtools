/* leverage for explode							(ksb)
 * $Id: machine.h,v 6.2 2000/06/17 22:24:17 ksb Exp $
 */
#if !defined(HPUX)
#if defined(HPUX7)||defined(HPUX8)||defined(HPUX9)||defined(HPUX10)||defined(HPUX11)
#define HPUX	1
#endif
#endif

#if !defined(F_OK)
#define F_OK	0
#endif

#if !defined(DEFINCL)
#define DEFINCL "/usr/local/lib/explode:~/lib/explode:~ksb/lib/explode"
#endif

#ifndef HAVE_STRERROR
#define HAVE_STRERROR	(defined(NEXT2)||defined(IBMR2)||defined(HPUX)||defined(SUN5)||defined(FREEBSD)||defined(NETBSD)||defined(IRIX5)||defined(IRIX6)||defined(CONSENSYS)||defined(LINUX))
#endif

extern int errno;

#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif
