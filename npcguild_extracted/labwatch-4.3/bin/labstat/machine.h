/* $Id: machine.h,v 4.3 1998/02/12 20:19:27 ksb Exp $
 * the leverage in this file should be kept in sync with labd/labwatch
 */
#if !defined(IRIX)
#if IRIX5 || IRIX6
#define IRIX
#endif
#endif

#ifndef HAVE_STRERROR
#define HAVE_STRERROR	(defined(NEXT2)||defined(IBMR2)||defined(HPUX7)||defined(HPUX8)||defined(SUN5)||defined(NETBSD)||defined(IRIX5)||defined(FREEBSD)||defined(LINUX))
#endif

extern int errno;

#if !defined(USE_STRINGS)
#define USE_STRINGS		(defined(HPUX8))
#endif

#if !defined(HAVE_STRDUP)
#define HAVE_STRDUP	(defined(SUN5)||defined(SUN4)||defined(SUN3)||defined(NETBSD)||defined(AMIGA)||defined(HPUX7)||defined(HPUX8)||defined(HPUX9)||defined(IRIX5)||defined(IBMR2))
#endif

#if !HAVE_STRDUP
#define strdup(Ms)	(strcpy(malloc((strlen(Ms)|3)+1), (Ms)))
#endif

#if !defined(USE_MALLOC_H)
#define USE_MALLOC_H	(defined(PTX)||defined(NETBSD)||defined(IBMR2)||defined(SUN5)||defined(MSDOS)||defined(IRIX)||defined(ULTRIX))
#endif

#if !defined(NEED_MALLOC_EXTERN)
#define NEED_MALLOC_EXTERN	(!USE_MALLOC_H && !defined(NEXT2) && !defined(IRIX) && !defined(BSDI))
#endif

#if USE_MALLOC_H
#include <malloc.h>
#else
#if NEED_MALLOC_EXTERN
extern char *malloc(), *calloc(), *realloc();
#endif
#endif

#if !HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(Me) (sys_errlist[Me])
#endif

/* this *must* agree with labd and labwatch
 */
#if !defined(XMIT_SCALE)
#define XMIT_SCALE	4096
#endif
