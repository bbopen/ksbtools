/* $Id: machine.h,v 3.2 2000/02/20 19:20:47 ksb Exp $
 * meta source lens for uncurly -- which really should not need one.
 * Kevin Braunsdorf, 1996
 */
#if !defined(HPUX) && (defined(HPUX7)||defined(HPUX8)||defined(HPUX9)||defined(HPUX10)||defined(HPUX11))
#define HPUX	1
#endif

#if !defined(IRIX) && (defined(IRIX4) || defined(IRIX5) || defined(IRIX6))
#define IRIX	1
#endif

#if !defined(ULTRIX) && (defined(ULTRIX4) || defined(ULTRIX5))
#define ULTRIX	1
#endif

#if !defined(USE_MALLOC_H)
#define USE_MALLOC_H	(defined(PTX)||defined(NETBSD)||defined(IBMR2)||defined(SUN5)||defined(MSDOS)||defined(IRIX)||defined(ULTRIX)||defined(CONSENSYS)||defined(HPUX10)||defined(HPUX11))
#endif

#if !defined(NEED_MALLOC_EXTERN)
#define NEED_MALLOC_EXTERN	(!USE_MALLOC_H && !defined(NEXT2) && !defined(IRIX) && !defined(BSDI))
#endif

#if !defined(USE_STDLIB)
#if IBMR2||PTX||FREEBSD||MSDOS||ULTRIX||BSDI
#define USE_STDLIB	1
#else
#define USE_STDLIB	0
#endif
#endif

#if !defined(USE_UNISTD_H)
#define USE_UNISTD_H	(defined(FREEBSD)||defined(HPUX9)||defined(HPUX10)||defined(HPUX11))
#endif

#if USE_MALLOC_H
#include <malloc.h>
#endif

#if NEED_MALLOC_EXTERN
extern char *malloc(), *calloc(), *realloc();
#endif

#if USE_STDLIB || defined(__STDC__)
#include <stdlib.h>
#else
extern char *getenv();
#endif

#if USE_UNISTD_H
#include <unistd.h>
#endif
