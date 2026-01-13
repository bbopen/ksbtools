/* leverage for explode							(ksb)
 * $Id: machine.h,v 6.5 2008/03/14 19:38:07 ksb Exp $
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
#define DEFINCL "/usr/local/lib/explode:/usr/share/lib/explode:~/lib/explode"
#endif

#if !defined(USE_STDLIB)
#define USE_STDLIB	1
#endif

#if !defined(USE_MALLOC_H)
#define USE_MALLOC_H	(!(defined(HPUX9)||defined(HPUX10)))
#endif

#if !defined(USE_MKSTEMP)
#define USE_MKSTEMP	(!(defined(HPUX9)||defined(HPUX10)))
#endif

#if !defined(NEED_MALLOC_EXTERN)
#define NEED_MALLOC_EXTERN	(!USE_MALLOC_H && !defined(NEXT2) && !defined(IRIX) && !defined(BSDI) && !defined(FREEBSD) && !defined(OPENBSD))
#endif

#if USE_STDLIB || defined(__STDC__)
#include <stdlib.h>
#endif

#if USE_MALLOC_H
#if !USE_STDLIB
#include <malloc.h>
#endif
#else
#if NEED_MALLOC_EXTERN
extern char *malloc(), *calloc(), *realloc();
#endif
#endif

#ifndef HAVE_STRERROR
#define HAVE_STRERROR	(defined(NEXT2)||defined(IBMR2)||defined(HPUX)||defined(SUN5)||defined(FREEBSD)||defined(OPENBSD)||defined(NETBSD)||defined(IRIX5)||defined(IRIX6)||defined(CONSENSYS)||defined(LINUX))
#endif

#if !HAVE_STRERROR
extern int errno;
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif
