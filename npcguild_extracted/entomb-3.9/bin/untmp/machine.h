/* $Id: machine.h,v 1.2 1998/02/12 19:54:39 ksb Exp $
 */

#if !defined(HPUX) && (defined(HPUX7)||defined(HPUX8)||defined(HPUX9))
#define HPUX	1
#endif

#if !defined(HAVE_STRERROR)
#define HAVE_STRERROR	(defined(NEXT2)||defined(IBMR2)||defined(HPUX7)||defined(HPUX8)||defined(SUN5)||defined(NETBSD)||defined(IRIX5)||defined(FREEBSD)||defined(LINUX))
#endif

extern int errno;

#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif
