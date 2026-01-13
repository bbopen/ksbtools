/* $Id: machine.h,v 3.10 1999/01/10 01:42:38 ksb Exp $
 * leverage typical of later day ksb programs				(ksb)
 */
#if !defined(IRIX)
#define IRIX	(defined(IRIX5)||defined(IRIX6))
#endif

#if !defined(HAVE_ASM)
#define HAVE_ASM	1	/* should be eat asm { ... }		*/
#endif

#if !defined(MAXPATHLEN)
#define MAXPATHLEN	1024
#endif

#if !defined(USE_STRINGS)
#define USE_STRINGS	(!defined(SUN5))
#endif

#if !defined(PATH_CPP)
#if FREEBSD || LINUX || BSDI || SUN5
#define PATH_CPP	"gcc -E"
#else
#define PATH_CPP	"/lib/cpp"
#endif
#endif

#if !defined(USE_MALLOC_H)
#define USE_MALLOC_H	(defined(PTX)||defined(NETBSD)||defined(IBMR2)||defined(SUN5)||defined(MSDOS)||defined(IRIX5)||defined(IRIX6)||defined(ULTRIX))
#endif

#if !defined(NEED_MALLOC_EXTERN)
#define NEED_MALLOC_EXTERN	(!USE_MALLOC_H && !defined(NEXT2) && !defined(IRIX) && !defined(BSDI))
#endif

#if USE_MALLOC_H
#include <malloc.h>
#else
#if NEED_MALLOC_EXTERN
extern char *malloc(), *calloc();
#endif
#endif

#if !defined(HAVE_STRERROR)
#define HAVE_STRERROR	(defined(IBMR2)||defined(ETA10)||defined(V386)||defined(SUN5)||defined(NEXT2)||defined(HPUX8)||defined(HPUX9)||defined(HPUX10)||defined(PTX)||defined(IRIX5)||defined(IRIX6)||defined(FREEBSD)||defined(LINUX))
#endif

#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif
